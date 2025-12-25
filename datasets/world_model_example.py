#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
极简版任务上下文管理器 - 基于事件驱动的节点和边管理

核心功能：
1. 提供对外查询接口（nodes和edges的快速查询）
2. 所有数据修改操作必须通过事件机制进行
3. 支持异步事件处理和并发控制
4. 提供完整的类型提示和详细注释

设计原则：
- 查询操作：直接方法调用，提供快速访问
- 修改操作：必须通过DataModificationEvent事件进行
- 数据一致性：通过事件机制保证数据同步
- 并发安全：使用异步锁保护关键数据结构

事件支持：
- DataModificationEvent: 数据修改事件（add, update, remove）
- 支持node和edge两种实体类型
- 自动订阅和处理数据修改事件
- 查询操作通过直接方法调用，无需事件机制
"""

import asyncio
import json
import logging
import uuid
import math
import re
from typing import Any, Dict, List, Optional, Union, Literal, Set, Tuple

from modules.config.events import EventType, DataModificationEvent

# 导入事件系统
from modules.events.event_bus import (
    subscribe_event,
    unsubscribe_event,
    publish_event_sync,
    publish_reply_event,
)
from modules.world_model.utils.graph_utils import (
    find_path as _nav_find_path,
    find_side_edges_near_path as _nav_find_side_edges_near_path,
    find_candidate_locations as _nav_find_candidate_locations,
)


class SimplifiedTaskContext:
    """任务上下文管理器 - 基于事件驱动的节点和边管理（单例模式）

    这个类提供了一个轻量级的任务上下文管理器，专注于：
    1. 快速的节点和边查询接口
    2. 基于事件的数据修改操作
    3. 异步安全的数据访问
    4. 完整的事件驱动架构支持
    5. 单例模式确保全局唯一实例

    所有数据修改操作都必须通过DataModificationEvent进行，
    确保数据一致性和事件驱动的架构原则。

    Attributes:
        _instance: 单例实例
        _nodes: 节点数据列表
        _edges: 边数据列表
        _data_lock: 异步锁，保护数据结构的并发访问
        _event_subscriptions: 事件订阅列表
        _is_initialized: 初始化状态标志
    """

    # 单例模式相关属性
    _instance: Optional["SimplifiedTaskContext"] = None
    _lock = asyncio.Lock()

    def __new__(cls, *args, **kwargs):
        """单例模式实现

        确保整个应用程序中只有一个SimplifiedTaskContext实例。

        Returns:
            SimplifiedTaskContext: 全局唯一实例
        """
        if cls._instance is None:
            cls._instance = super(SimplifiedTaskContext, cls).__new__(cls)
        return cls._instance

    def __init__(
        self,
        initial_nodes: Optional[List[Dict[str, Any]]] = None,
        initial_edges: Optional[List[Dict[str, Any]]] = None,
        initial_goal: Optional[str] = None,
        new_case: Optional[List[Dict[str, Any]]] = None,
    ):
        """初始化极简任务上下文

        初始化数据存储、异步锁和事件订阅。
        自动订阅DataModificationEvent和QueryEvent。
        由于单例模式，避免重复初始化。

        Args:
            initial_nodes: 初始节点列表
            initial_edges: 初始边列表
            initial_goal: 初始目标描述
            new_case: 新情况
        """
        # 避免重复初始化
        if hasattr(self, "_is_initialized") and self._is_initialized:
            return

        # 场景图数据 - 核心数据结构
        self._nodes: List[Dict[str, Any]] = initial_nodes or []
        self._edges: List[Dict[str, Any]] = initial_edges or []
        self._goal: Optional[Any] = initial_goal if initial_goal else None
        self._new_case: Optional[List[Dict[str, Any]]] = new_case or []

        # 新情况弹出计数器，用于限制每次重规划只弹出一种新情况
        self._new_case_pop_count: int = 0
        self._max_pop_per_replan: int = 1  # 每次重规划最多弹出的新情况数量

        # 如果没有提供初始数据，则加载默认数据
        if not self._nodes or not self._edges:
            raise RuntimeError("必须提供初始节点和边数据，极简任务上下文不支持默认数据加载")
        # 异步锁保护数据结构
        self._data_lock = asyncio.Lock()

        # 事件订阅管理
        self._event_subscriptions: List[str] = []
        self._is_initialized = False

        # 日志
        self.logger = logging.getLogger(f"simplified_task_context.{self.get_id()}")

        # 初始化事件订阅
        self._setup_event_subscriptions()

        self.logger.info(
            f"极简任务上下文初始化完成，节点数: {len(self._nodes)}, 边数: {len(self._edges)}"
        )

    def _load_default_data(
        self, scenario_type: str = "cybertown", scenario_id: str = "scenario_1"
    ) -> bool:
        """
        加载默认数据

        如果没有提供初始节点和边，则加载默认数据。
        从预定义的配置文件或数据库加载场景数据。

        Args:
            scenario_type (str): 场景类型，默认为 "cybertown"
            scenario_id (str): 场景ID，默认为 "scenario_1"

        Returns:
            bool: 加载成功返回True，失败返回False
        """
        # 加载数据集
        dataset = load_dataset()
        if not dataset:
            self.logger.log("Failed to load dataset", level="error")
            return False

        # 提取配置
        scenario_config = dataset.scenarios[scenario_type][scenario_id]
        scene_graph = scenario_config["scene_graph"]
        scene_config = scene_graph["scene_config"]
        gridmap_config = scene_graph["gridmap_config"]

        self._nodes.extend(scene_config.get("nodes", []))
        self._edges.extend(scene_config.get("edges", []))

    

    # ==================== 新情况查询接口 ====================
    def get_new_case(self) -> Optional[List[Dict[str, Any]]]:
        """获取新情况数据

        Returns:
            新情况数据列表，如果没有设置新情况则返回None
        """
        if self._new_case:
            return self._new_case.copy()
        return None

    def has_new_case(self) -> bool:
        """判断是否存在 newcase"""
        return bool(self._new_case)

    def pop_new_case(self) -> Optional[Dict[str, Any]]:
        """弹出末尾的新情况中的一个数据

        注意：每次重新规划（replanning）只允许弹出一个新情况，
        确保系统能够逐步处理多个新情况而不会同时处理多个，
        这样可以保证重新规划的稳定性和可控性。

        使用计数器限制每次重规划周期内最多只能弹出指定数量的新情况。

        Returns:
            Dict[str, Any]: 弹出的新情况数据，如果没有新情况或已达到弹出限制则返回None
        """
        # 检查是否已达到本次重规划的弹出限制
        if self._new_case_pop_count >= self._max_pop_per_replan:
            return None

        if self._new_case:
            # 增加弹出计数
            self._new_case_pop_count += 1
            return self._new_case.pop()  # 弹出列表最后一个元素，每次只处理一个新情况
        return None

    def reset_new_case_pop_count(self) -> None:
        """重置新情况弹出计数器

        在新的重规划周期开始时调用此方法，将计数器重置为0，
        允许在新的重规划周期中再次弹出新情况。

        这确保了每次重规划周期都有机会处理新情况，
        同时维持每次重规划只处理有限数量新情况的限制。
        """
        self._new_case_pop_count = 0

    def get_new_case_pop_count(self) -> int:
        """获取当前重规划周期内已弹出的新情况数量

        Returns:
            int: 当前重规划周期内已弹出的新情况数量
        """
        return self._new_case_pop_count

    def get_max_pop_per_replan(self) -> int:
        """获取每次重规划允许弹出的最大新情况数量

        Returns:
            int: 每次重规划允许弹出的最大新情况数量
        """
        return self._max_pop_per_replan

    def set_max_pop_per_replan(self, max_count: int) -> None:
        """设置每次重规划允许弹出的最大新情况数量

        Args:
            max_count (int): 每次重规划允许弹出的最大新情况数量，必须大于0

        Raises:
            ValueError: 当max_count小于等于0时抛出异常
        """
        if max_count <= 0:
            raise ValueError("每次重规划允许弹出的最大新情况数量必须大于0")
        self._max_pop_per_replan = max_count

    # ==================== 目标查询接口 ====================

    def get_goal_description(self) -> Optional[str]:
        """获取目标描述

        Returns:
            目标描述字符串
        """
        return self._goal["goal"]["description"]

    def get_goal(self) -> Optional[Dict[str, Any]]:
        """获取目标数据

        Returns:
            目标数据字典，如果没有设置目标则返回None
        """
        if self._goal:
            return {"goal": self._goal}
        return None

    def evaluate_goal(self) -> bool:
        """评估目标是否达成

        Returns:
            bool: 目标是否达成
        """
        if self._goal:
            world_state = {
                "entities": self._nodes,
            }
            return self._goal.evaluate(world_state)

        return False

    # ==================== 节点查询接口 ====================

    def get_all_nodes(self) -> List[Dict[str, Any]]:
        """获取所有节点

        Returns:
            所有节点的副本列表
        """
        return [node.copy() for node in self._nodes]

    def get_node_by_id(self, node_id: str) -> Optional[Dict[str, Any]]:
        """根据ID获取节点

        Args:
            node_id: 节点ID

        Returns:
            节点数据的副本，如果不存在则返回None
        """
        for node in self._nodes:
            if str(node.get("id")) == str(node_id):
                return node.copy()
        return None

    def get_node_by_label(self, label: str) -> Optional[Dict[str, Any]]:
        """根据label获取节点

        Args:
            label: 节点的label

        Returns:
            节点数据的副本，如果不存在则返回None
        """
        for node in self._nodes:
            if node.get("properties", {}).get("label") == label:
                return node.copy()
        return None

    def get_nodes_by_category(self, category: str) -> List[Dict[str, Any]]:
        """根据类别获取节点

        Args:
            category: 节点类别 (robot, prop, building, goal等)

        Returns:
            匹配类别的节点列表
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("category") == category
        ]

    def get_nodes_by_type(self, node_type: str) -> List[Dict[str, Any]]:
        """根据类型获取节点

        Args:
            node_type: 节点类型

        Returns:
            匹配类型的节点列表
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("type") == node_type
        ]

    def search_nodes(self, **criteria) -> List[Dict[str, Any]]:
        """根据条件搜索节点

        Args:
            **criteria: 搜索条件，支持嵌套属性查询

        Returns:
            匹配条件的节点列表
        """
        result = []
        for node in self._nodes:
            match = True
            for key, value in criteria.items():
                if "." in key:
                    # 支持嵌套属性查询，如 'properties.category'
                    keys = key.split(".")
                    node_value = node
                    for k in keys:
                        if isinstance(node_value, dict) and k in node_value:
                            node_value = node_value[k]
                        else:
                            node_value = None
                            break
                    if node_value != value:
                        match = False
                        break
                else:
                    if node.get(key) != value:
                        match = False
                        break
            if match:
                result.append(node.copy())
        return result

    def get_node_info_by_label(self, label: str) -> Optional[Dict[str, str]]:
        """
        根据给定的节点 label，返回该节点的 category 和 type 信息。

        Args:
            label (str): 节点的 label 名称。

        Returns:
            Optional[Dict[str, str]]: 包含该节点 category 和 type 的字典。
                                    若未找到对应节点，则返回 None。

        示例输出:
            {
                'category': 'building',
                'type': 'semantic'
            }
        """
        for node in self._nodes:
            properties = node.get("properties", {})
            if properties.get("label") == label:
                category = properties.get("category")
                node_type = properties.get("type")  # 假设 type 也是在 properties 中
                if category is not None and node_type is not None:
                    return {"category": category, "type": node_type}
                else:
                    return None  # 如果有字段缺失，也可以改为返回部分字段
        return None  # 没有找到对应 label

    # ==================== 边查询接口 ====================

    def get_all_edges(self) -> List[Dict[str, Any]]:
        """获取所有边

        Returns:
            所有边的副本列表
        """
        return [edge.copy() for edge in self._edges]

    def get_edges_by_source(self, source_id: str) -> List[Dict[str, Any]]:
        """获取指定源节点的所有边

        Args:
            source_id: 源节点ID

        Returns:
            从指定节点出发的边列表
        """
        return [
            edge.copy()
            for edge in self._edges
            if str(edge.get("source")) == str(source_id)
        ]

    def get_edges_by_target(self, target_id: str) -> List[Dict[str, Any]]:
        """获取指定目标节点的所有边

        Args:
            target_id: 目标节点ID

        Returns:
            指向指定节点的边列表
        """
        return [
            edge.copy()
            for edge in self._edges
            if str(edge.get("target")) == str(target_id)
        ]

    def get_edge(self, source_id: str, target_id: str) -> Optional[Dict[str, Any]]:
        """获取指定的边

        Args:
            source_id: 源节点ID
            target_id: 目标节点ID

        Returns:
            边数据的副本，如果不存在则返回None
        """
        for edge in self._edges:
            if str(edge.get("source")) == str(source_id) and str(
                edge.get("target")
            ) == str(target_id):
                return edge.copy()
        return None

    def get_neighbors(self, node_id: str, direction: str = "both") -> List[str]:
        """获取节点的邻居节点ID列表

        Args:
            node_id: 节点ID
            direction: 方向 ('in', 'out', 'both')

        Returns:
            邻居节点ID列表
        """
        neighbors = set()

        if direction in ["out", "both"]:
            # 出边的目标节点
            for edge in self._edges:
                if str(edge.get("source")) == str(node_id):
                    neighbors.add(str(edge.get("target")))

        if direction in ["in", "both"]:
            # 入边的源节点
            for edge in self._edges:
                if str(edge.get("target")) == str(node_id):
                    neighbors.add(str(edge.get("source")))

        return list(neighbors)

    def get_neighbors_by_relation(
        self, node_id: Union[int, str], relation: str
    ) -> List[Union[int, str]]:
        """
        根据给定的节点ID和关系类型，查找并返回所有直接关联的邻居节点的ID列表。

        这个函数会双向查找：
        1. 查找以 node_id 为起点（source）的边。
        2. 查找以 node_id 为终点（target）的边。

        Args:
            node_id (Union[int, str]): 要查询的节点的唯一ID。
            relation (str): 要筛选的边的关系类型 (例如, 'stationed_at', 'carrying', 'stored_at')。

        Returns:
            List[Union[int, str]]: 包含所有符合条件的邻居节点ID的列表，如果没有找到则返回空列表。
        """
        # 使用集合（set）来自动处理可能出现的重复ID
        neighbor_ids: Set[Union[int, str]] = set()

        for edge in self._edges:
            edge_type = edge.get("type")

            # 1. 检查边的类型是否匹配
            if edge_type == relation:
                source = edge.get("source")
                target = edge.get("target")

                # 2. 双向检查节点ID是否参与该边
                if source == str(node_id):
                    # 如果node_id是起点，则邻居是终点
                    neighbor_ids.add(target)
                elif target == str(node_id):
                    # 如果node_id是终点，则邻居是起点
                    neighbor_ids.add(source)

        return list(neighbor_ids)

    def get_edges_from_node(
        self, node_id: Union[int, str], edge_type: Optional[str] = None
    ) -> List[Dict[str, Any]]:
        """
        获取从指定节点出发的边，可选择按类型过滤

        Args:
            node_id: 源节点ID
            edge_type: 边的类型，如果指定则只返回该类型的边

        Returns:
            从指定节点出发的边列表（返回边的副本）
        """
        edges = []
        node_id_str = str(node_id)

        for edge in self._edges:
            if str(edge.get("source")) == node_id_str:
                # 如果指定了边类型，则进行过滤
                if edge_type is None or edge.get("type") == edge_type:
                    edges.append(edge.copy())

        return edges

    def get_edges_by_type(self, edge_type: str) -> List[Dict[str, Any]]:
        """
        获取指定类型的所有边

        Args:
            edge_type: 边的类型

        Returns:
            指定类型的边列表
        """
        return [edge.copy() for edge in self._edges if edge.get("type") == edge_type]

    def update_edge_target(
        self,
        source_id: Union[int, str],
        old_target_id: Union[int, str],
        new_target_id: Union[int, str],
        edge_type: Optional[str] = None,
    ) -> bool:
        """
        更新边的目标节点

        Args:
            source_id: 源节点ID
            old_target_id: 旧的目标节点ID
            new_target_id: 新的目标节点ID
            edge_type: 边的类型（可选，用于更精确的匹配）

        Returns:
            是否成功更新
        """
        source_str = str(source_id)
        old_target_str = str(old_target_id)
        new_target_str = str(new_target_id)

        for i, edge in enumerate(self._edges):
            if (
                str(edge.get("source")) == source_str
                and str(edge.get("target")) == old_target_str
            ):
                if edge_type is None or edge.get("type") == edge_type:
                    # 创建新边（保持原有属性，只更改target）
                    new_edge = edge.copy()
                    new_edge["target"] = new_target_id  # 保持原始类型
                    self._edges[i] = new_edge
                    return True

        return False

    def remove_edges_from_node(
        self, node_id: Union[int, str], edge_type: Optional[str] = None
    ) -> int:
        """
        删除从指定节点出发的所有边

        Args:
            node_id: 源节点ID
            edge_type: 边的类型，如果指定则只删除该类型的边

        Returns:
            删除的边数量
        """
        node_id_str = str(node_id)
        edges_to_remove = []

        for edge in self._edges:
            if str(edge.get("source")) == node_id_str:
                if edge_type is None or edge.get("type") == edge_type:
                    edges_to_remove.append(edge)

        # 删除找到的边
        for edge in edges_to_remove:
            self._edges.remove(edge)

        return len(edges_to_remove)

    def has_edge_of_type(
        self, source_id: Union[int, str], target_id: Union[int, str], edge_type: str
    ) -> bool:
        """
        检查两个节点之间是否存在指定类型的边

        Args:
            source_id: 源节点ID
            target_id: 目标节点ID
            edge_type: 边的类型

        Returns:
            是否存在该类型的边
        """
        source_str = str(source_id)
        target_str = str(target_id)

        for edge in self._edges:
            if (
                str(edge.get("source")) == source_str
                and str(edge.get("target")) == target_str
                and edge.get("type") == edge_type
            ):
                return True

        return False

    # ==================== 实体查询接口 ====================

    def get_robot(self, robot_id: str) -> Optional[Dict[str, Any]]:
        """根据ID获取机器人节点

        Args:
            robot_id: 机器人ID

        Returns:
            机器人节点数据的副本，如果不存在则返回None
        """
        for node in self._nodes:
            if (
                str(node.get("id")) == str(robot_id)
                and node.get("properties", {}).get("category") == "robot"
            ):
                return node.copy()
        return None

    def get_prop(self, prop_id: str = None) -> Optional[Dict[str, Any]]:
        """根据ID获取道具节点

        Args:
            prop_id: 道具ID

        Returns:
            道具节点数据的副本，如果不存在则返回None
        """
        if not prop_id:
            # 输出所有的道具节点信息
            return [
                node.copy()
                for node in self._nodes
                if node.get("properties", {}).get("category") == "prop"
            ]
        else:
            for node in self._nodes:
                if (
                    str(node.get("id")) == str(prop_id)
                    and node.get("properties", {}).get("category") == "prop"
                ):
                    return node.copy()
            return None

    def get_building(self, building_id: str) -> Optional[Dict[str, Any]]:
        """根据ID获取建筑节点

        Args:
            building_id: 建筑ID

        Returns:
            建筑节点数据的副本，如果不存在则返回None
        """
        for node in self._nodes:
            if (
                str(node.get("id")) == str(building_id)
                and node.get("properties", {}).get("category") == "building"
            ):
                return node.copy()
        return None

    def get_all_robots(self) -> List[Dict[str, Any]]:
        """获取所有的机器人节点

        Returns:
            机器人节点数据副本的列表（可能为空）
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("category") == "robot"
        ]

    def get_all_props(self) -> List[Dict[str, Any]]:
        """获取所有的道具节点

        Returns:
            道具节点数据副本的列表（可能为空）
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("category") == "prop"
        ]

    def get_all_buildings(self) -> List[Dict[str, Any]]:
        """获取所有的建筑节点

        Returns:
            建筑节点数据副本的列表（可能为空）
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("category") == "building"
        ]

    def get_all_trans_facilitys(self) -> List[Dict[str, Any]]:
        """获取所有的基础设施节点

        Returns:
            建筑节点数据副本的列表（可能为空）
        """
        return [
            node.copy()
            for node in self._nodes
            if node.get("properties", {}).get("category") == "trans_facility"
        ]

    def get_goal_node(self, goal_id: str) -> Optional[Dict[str, Any]]:
        """根据ID获取目标节点

        Args:
            goal_id: 目标ID

        Returns:
            目标节点数据的副本，如果不存在则返回None
        """
        for node in self._nodes:
            if (
                str(node.get("id")) == str(goal_id)
                and node.get("properties", {}).get("category") == "goal"
            ):
                return node.copy()
        return None

    # ==================== 位置获取接口 ====================

    def _get_node_position(self, node: Dict) -> List[float]:
        """
        获取节点的代表位置：
        - shape.type == 'point'      -> center
        - shape.type == 'circle'     -> center
        - shape.type == 'rectangle'  -> bbox 中点
        - shape.type == 'polygon'    -> 顶点平均值（简单质心近似）
        - shape.type  == 'linestring' -> 折线弧长一半处的点
        其余情况返回 [0, 0]
        """
        shape = node.get("shape") or {}
        stype = shape.get("type")

        # 1) 点 / 圆
        if stype == "point":
            c = shape.get("center", [0.0, 0.0])
            return [float(c[0]), float(c[1])]
        if stype == "circle":
            c = shape.get("center", [0.0, 0.0])
            return [float(c[0]), float(c[1])]

        # 2) 矩形：中心点
        if stype == "rectangle":
            minc = shape.get("min_corner", [0.0, 0.0])
            maxc = shape.get("max_corner", [0.0, 0.0])
            return [
                (float(minc[0]) + float(maxc[0])) / 2.0,
                (float(minc[1]) + float(maxc[1])) / 2.0,
            ]

        # 3) 多边形：顶点平均值（区域中心的简化版）
        if stype == "polygon":
            verts = shape.get("vertices", [])
            if verts:
                sx = sy = 0.0
                n = 0
                for v in verts:
                    if v and len(v) >= 2:
                        sx += float(v[0])
                        sy += float(v[1])
                        n += 1
                if n > 0:
                    return [sx / n, sy / n]

        # 4) 折线：取弧长中点
        if stype == "linestring":
            pts = shape.get("points", [])
            if not pts:
                return [0.0, 0.0]
            if len(pts) == 1:
                p = pts[0]
                return [float(p[0]), float(p[1])]
            # 计算总长度
            seg_lens = []
            total = 0.0
            for i in range(len(pts) - 1):
                x1, y1 = float(pts[i][0]), float(pts[i][1])
                x2, y2 = float(pts[i + 1][0]), float(pts[i + 1][1])
                L = math.hypot(x2 - x1, y2 - y1)
                seg_lens.append(L)
                total += L
            if total <= 1e-9:
                # 退化：用首末点中点
                x1, y1 = float(pts[0][0]), float(pts[0][1])
                x2, y2 = float(pts[-1][0]), float(pts[-1][1])
                return [(x1 + x2) / 2.0, (y1 + y2) / 2.0]

            half = total / 2.0
            acc = 0.0
            for i, L in enumerate(seg_lens):
                if acc + L >= half:
                    # 在第 i 段内插
                    x1, y1 = float(pts[i][0]), float(pts[i][1])
                    x2, y2 = float(pts[i + 1][0]), float(pts[i + 1][1])
                    t = (half - acc) / (L if L > 1e-12 else 1.0)
                    return [x1 + (x2 - x1) * t, y1 + (y2 - y1) * t]
                acc += L
            # 兜底：返回末点
            pe = pts[-1]
            return [float(pe[0]), float(pe[1])]

        # 其它未知类型
        return [0.0, 0.0]

    def get_position_by_id(self, node_id: int) -> Optional[List[float]]:
        """
        根据节点ID获取单个实体的位置。

        Args:
            node_id (int): 要查询的节点的唯一ID。

        Returns:
            Optional[List[float]]: 实体的位置坐标 [x, y]，如果未找到则返回 None。
        """
        node = self.get_node_by_id(node_id)
        return self._get_node_position(node)

    def get_position_by_label(self, node_label: str) -> Optional[List[float]]:
        """
        根据节点Label获取单个实体的位置。假定Label是唯一的。

        Args:
            node_label (str): 要查询的节点的标签。

        Returns:
            Optional[List[float]]: 实体的位置坐标 [x, y]，如果未找到则返回 None。
        """
        # 假设 'label' 存储在 'properties' 字段中
        nodes_with_label = self.search_nodes(**{"properties.label": node_label})
        if nodes_with_label:
            return self._get_node_position(nodes_with_label[0])
        return None

    def get_positions_by_entity_type(
        self, entity_categories: List[str] = ["building", "robot"]
    ) -> Dict[str, Dict[str, Dict[str, List[float]]]]:
        """
        批量获取指定实体类别的位置,并以结构化的字典返回。

        Args:
            entity_categories (List[str]): 希望查询的实体类别列表,
                                         默认为 ['building', 'robot']。

        Returns:
            Dict[str, Dict[str, Dict[str, List[float]]]]: 包含位置信息的嵌套字典。
        """
        results = {}
        for category in entity_categories:
            # 获取该类别的所有节点
            nodes_in_category = self.get_nodes_by_category(category)
            if not nodes_in_category:
                continue

            # 为该类别初始化一个字典
            results[category] = {}

            for node in nodes_in_category:
                # 安全地获取子类型和标签
                properties = node.get("properties", {})
                subtype = properties.get("type")
                label = properties.get("label")

                # 如果是 robot 类别，检查 status 是否为 error
                if category == "robot":
                    status = properties.get("status")
                    if status == "error":
                        continue  # 跳过状态为 error 的机器人

                # 提取位置
                position = self._get_node_position(node)

                # 必须同时有子类型、标签和位置信息才进行记录
                if not (subtype and label and position):
                    continue

                # 如果子类型是第一次出现,为其初始化一个字典
                if subtype not in results[category]:
                    results[category][subtype] = {}

                # 记录该实体的位置,以label作为唯一标识符
                results[category][subtype][label] = position

        return results

    def get_boundary_features(
        self,
        district: Optional[str] = None,
    ) -> Dict[str, Dict[str, Any]]:
        """
        提取边界特征，仅考虑 category ∈ {area, trans_facility, building}。
        返回: { label: { "kind": "area|line|point|rectangle|circle", ... } }
        - area / line / point / rectangle: 使用 "coords": [[x,y], ...]
        - circle: 使用 "center": [x,y], "radius": float
        """
        results: Dict[str, Dict[str, Any]] = {}
        allowed = {"area", "trans_facility", "building"}

        for node in getattr(self, "_nodes", []):
            props = node.get("properties") or {}
            if not isinstance(props, dict):
                continue
            if props.get("category") not in allowed:
                continue
            if district is not None and props.get("district") != district:
                continue

            label = str(
                props.get("label")
                or node.get("label")
                or node.get("id")
                or props.get("id")
            )
            shape = node.get("shape") or {}
            t = isinstance(shape, dict) and shape.get("type")
            if not t:
                continue

            feature: Dict[str, Any] = {}
            try:
                if t == "polygon":
                    verts = shape.get("vertices") or []
                    coords = [
                        [float(v[0]), float(v[1])] for v in verts if v and len(v) >= 2
                    ]
                    if coords:
                        feature = {"kind": "area", "coords": coords}

                elif t == "rectangle":
                    a = shape.get("min_corner") or []
                    b = shape.get("max_corner") or []
                    x1, y1 = float(a[0]), float(a[1])
                    x2, y2 = float(b[0]), float(b[1])
                    feature = {
                        "kind": "rectangle",
                        "coords": [[x1, y1], [x2, y1], [x2, y2], [x1, y2]],
                    }

                elif t == "circle":
                    c = shape.get("center") or []
                    r = shape.get("radius")
                    cx, cy, rr = float(c[0]), float(c[1]), float(r)
                    feature = {"kind": "circle", "center": [cx, cy], "radius": rr}

                elif t == "point":
                    c = shape.get("center") or []
                    cx, cy = float(c[0]), float(c[1])
                    feature = {"kind": "point", "coords": [[cx, cy]]}

                elif t in ("linestring", "polyline"):
                    pts = shape.get("points") or []
                    coords = [
                        [float(p[0]), float(p[1])] for p in pts if p and len(p) >= 2
                    ]
                    if coords:
                        feature = {"kind": "line", "coords": coords}
            except Exception:
                feature = {}

            if feature:
                results[label] = feature

        return results

    # ==================== 路径与位置查询接口 ====================
    def find_path(self, src_id: str, dst_id: str) -> List[str]:
        return _nav_find_path(self, src_id, dst_id)

    def find_side_edges_near_path(
        self, src_id: str, dst_id: str
    ) -> List[Dict[str, Any]]:
        return _nav_find_side_edges_near_path(self, src_id, dst_id)

    def find_candidate_locations(
        self, object_or_carrier_id: str, prefer_parking: bool = True
    ) -> List[Dict[str, Any]]:
        return _nav_find_candidate_locations(self, object_or_carrier_id, prefer_parking)

    def has_path(self, src_id: str, dst_id: str) -> bool:
        path = _nav_find_path(self, src_id, dst_id)
        return len(path) > 0

    # ==================== 其它查询接口 ====================

    def _get_node_location_label(
        self, node: Dict[str, Any]
    ) -> Optional[Union[str, int]]:
        """获取节点的位置ID"""
        if not node:
            return None

        node_category = node.get("properties", {}).get("category")

        # 建筑物的位置就是它自己
        if node_category in ("building", "trans_facility", "area"):
            return node.get("properties", {}).get("label")

        # 其他节点的位置是其location属性
        location_label = node.get("properties", {}).get("location", {}).get("label")
        if location_label:
            return location_label

        return None

    def get_available_robots(
        self, types: Optional[List[str]] = None, concise: bool = False
    ) -> Dict[str, Dict[str, Any]]:
        def canonicalize(t: Optional[str]) -> Optional[str]:
            if not t:
                return None
            t_norm = t.strip().lower().replace(" ", "").replace("-", "_")
            aliases = {
                # core
                "uav": "UAV",
                "fw_uav": "FW_UAV",
                "fwuav": "FW_UAV",
                "fixedwing": "FW_UAV",
                "fixed_wing": "FW_UAV",
                "ugv": "UGV",
                "quadruped": "Quadruped",
                "quadrupte": "Quadruped",  # common typo
                "quad": "Quadruped",
                "humanoid": "Humanoid",
            }
            return aliases.get(t_norm)

        id_tail_re = re.compile(r"(\d+)$")

        def extract_numeric_tail(label: str) -> Optional[int]:
            m = id_tail_re.search(label.strip())
            return int(m.group(1)) if m else None

        # types filter
        include_set: Optional[Set[str]] = None
        if types is not None:
            include_set = {c for c in (canonicalize(t) for t in types) if c is not None}

        # storage
        detailed: Dict[str, Dict[str, Any]] = {}

        # iterate graph nodes
        for node in self._nodes:
            props = node.get("properties", {})
            if props.get("category") != "robot":
                continue
            # must be healthy & with sufficient battery
            if (
                props.get("status") == "error"
                or props.get("battery_level") < 10.0
                or props.get("comm") == "jammed"
            ):
                continue

            robot_type_raw = props.get("type")
            robot_label = props.get("label")
            canonical_type = canonicalize(robot_type_raw)
            if not canonical_type or not robot_label:
                continue

            # apply type filter if provided
            if include_set is not None and canonical_type not in include_set:
                continue

            # bucket
            if canonical_type not in detailed:
                detailed[canonical_type] = {"labels": [], "num": 0}
            detailed[canonical_type]["labels"].append(robot_label)
            detailed[canonical_type]["num"] += 1

        if not concise:
            return detailed

        # concise mapping: type -> list of numeric suffixes
        concise_map: Dict[str, List[int]] = {}
        for rtype, info in detailed.items():
            nums: List[int] = []
            for label in info.get("labels", []):
                n = extract_numeric_tail(label)
                if n is not None:
                    nums.append(n)
            if nums:  # 仅在解析到编号时返回该类型
                concise_map[rtype] = nums

        return concise_map

    def get_robot_states(self) -> dict:
        """
        从节点列表中提取机器人状态信息

        Args:
            nodes (list): 包含机器人节点的列表

        Returns:
            dict: 机器人状态字典，键为robot_label，值为状态信息
        """
        robot_states = {}

        # 获取所有节点
        all_nodes = self._nodes

        for node in all_nodes:
            properties = node.get("properties", {})

            if properties.get("category") != "robot":
                continue

            robot_label = properties.get("label")
            robot_type = properties.get("type")

            if robot_label and robot_type:
                robot_states[robot_label] = {
                    "type": robot_type,
                    "status": properties.get("status", "unknown"),
                    "location": properties.get("location", "unknown"),
                }

        return robot_states

    def get_node_map(
        self, map_type: Literal["id_to_label", "label_to_id"] = "id_to_label"
    ) -> Dict[Union[str, int], Union[str, int]]:
        """
        为所有节点生成 ID 和 Label 之间的映射字典。

        Args:
            map_type (str): 要生成的映射类型。
                            - 'id_to_label' (默认): 生成从 node ID 到其 label 的映射。
                            - 'label_to_id': 生成从 node label 到其 ID 的映射。

        Returns:
            Dict: 一个表示所请求映射的字典。

        Raises:
            ValueError: 如果提供了无效的 map_type。
        """
        if map_type not in ["id_to_label", "label_to_id"]:
            raise ValueError("无效的 map_type。必须是 'id_to_label' 或 'label_to_id'。")

        node_map = {}
        for node in self._nodes:
            # 安全地从节点结构中获取 ID 和 Label
            # .get('properties', {}) 确保即使没有 'properties' 键也不会出错
            node_id = node.get("id")
            label = node.get("properties", {}).get("label")

            # 仅当 ID 和 Label 都存在时才创建映射条目
            if node_id is not None and label is not None:
                if map_type == "id_to_label":
                    node_map[node_id] = label
                else:  # 此处 map_type 必定是 'label_to_id'
                    node_map[label] = node_id

        return node_map

    def get_all_node_categories(self) -> Dict[str, str]:
        """
        为所有节点生成其 Label 到其 Category 的映射字典。

        此函数会遍历所有节点，提取它们的 'label' 和 'category' 属性，
        并返回一个将每个标签映射到其对应类别的字典。

        Returns:
            Dict[str, str]: 一个表示 "node label -> category" 映射的字典。
                            如果某个节点缺少 label 或 category，则不会被包含在内。

        示例输出:
            {
                'Parking Lot-1': 'building',
                'vehicle-F-3803': 'prop'
            }
        """
        category_map: Dict[str, str] = {}
        for node in self._nodes:
            # 安全地从节点结构中获取 properties 字典
            properties = node.get("properties", {})

            # 从 properties 中获取 label 和 category
            label = properties.get("label")
            category = properties.get("category")

            # 仅当 label 和 category 都存在时，才创建映射条目
            if label is not None and category is not None:
                category_map[label] = category

        return category_map

    # ==================== 事件驱动的增删改接口 ====================

    def handle_node_event(self, event_type: str, node_data: Dict[str, Any]) -> bool:
        """处理节点事件

        Args:
            event_type: 事件类型 ('add', 'update', 'remove')
            node_data: 节点数据，对于remove事件只需要包含id

        Returns:
            操作是否成功
        """
        try:
            if event_type == "add":
                return self._add_node(node_data)
            elif event_type == "update":
                return self._update_node(node_data)
            elif event_type == "remove":
                node_id = node_data.get("id")
                if node_id:
                    return self._remove_node(str(node_id))
                return False
            else:
                self.logger.warning(f"未知的节点事件类型: {event_type}")
                return False
        except Exception as e:
            self.logger.error(f"处理节点事件失败: {e}")
            return False

    def handle_edge_event(self, operation: str, edge_data: Dict[str, Any]) -> bool:
        """处理边事件

        这个方法处理通过DataModificationEvent传递的边操作。
        所有边的增删改操作都应该通过事件机制调用此方法。

        Args:
            operation: 操作类型 ('add', 'update', 'remove')
            edge_data: 边数据，必须包含source和target字段

        Returns:
            bool: 操作是否成功

        Note:
            此方法不应直接调用，而应通过发布DataModificationEvent来触发。
        """
        try:
            self.logger.debug(
                f"Processing edge event: {operation} for edge {edge_data.get('id')}"
            )

            if operation == "add":
                return self._add_edge(edge_data)
            elif operation == "update":
                return self._update_edge(edge_data)
            elif operation == "remove":
                edge_id = edge_data.get("id") or edge_data.get("edge_id")
                if not edge_id:
                    # 尝试从source和target构建edge_id
                    source = edge_data.get("source")
                    target = edge_data.get("target")
                    if source and target:
                        edge_id = f"{source}->{target}"
                    else:
                        self.logger.error(
                            "Remove operation requires edge id or source/target"
                        )
                        return False
                return self._remove_edge(edge_id)
            else:
                self.logger.warning(f"Unknown edge operation: {operation}")
                return False

        except Exception as e:
            self.logger.error(f"Failed to handle edge event: {e}")
            return False

    # ==================== 内部实现方法 ====================

    def _add_node(self, node_data: Dict[str, Any]) -> bool:
        """添加节点

        Args:
            node_data: 节点数据

        Returns:
            是否添加成功
        """
        # 确保有ID
        if "id" not in node_data:
            node_data["id"] = str(uuid.uuid4())

        node_id = str(node_data["id"])

        # 检查ID是否已存在
        if self.get_node_by_id(node_id):
            self.logger.warning(f"节点ID {node_id} 已存在")
            return False

        # 添加节点
        self._nodes.append(node_data.copy())
        self.logger.info(f"添加节点: {node_id}")
        return True

    def _update_node(self, node_data: Dict[str, Any]) -> bool:
        """更新节点

        Args:
            node_data: 节点数据，必须包含id

        Returns:
            是否更新成功
        """
        node_id = node_data.get("id")
        if not node_id:
            self.logger.warning("更新节点时缺少ID")
            return False

        node_id = str(node_id)

        # 查找并更新节点
        for i, node in enumerate(self._nodes):
            if str(node.get("id")) == node_id:
                # 深度合并更新
                updated_node = node.copy()
                self._deep_update(updated_node, node_data)
                self._nodes[i] = updated_node
                self.logger.info(f"更新节点: {node_id}")
                return True

        self.logger.warning(f"未找到要更新的节点: {node_id}")
        return False

    def _remove_node(self, node_id: str) -> bool:
        """移除节点

        Args:
            node_id: 节点ID

        Returns:
            是否移除成功
        """
        # 移除节点
        for i, node in enumerate(self._nodes):
            if str(node.get("id")) == str(node_id):
                self._nodes.pop(i)

                # 移除相关的边
                self._edges = [
                    edge
                    for edge in self._edges
                    if (
                        str(edge.get("source")) != node_id
                        and str(edge.get("target")) != node_id
                    )
                ]

                self.logger.info(f"移除节点: {node_id}")
                return True

        self.logger.warning(f"未找到要移除的节点: {node_id}")
        return False

    def _add_edge(self, edge_data: Dict[str, Any]) -> bool:
        """添加边

        Args:
            edge_data: 边数据，必须包含source和target

        Returns:
            是否添加成功
        """
        source_id = edge_data.get("source")
        target_id = edge_data.get("target")

        if not source_id or not target_id:
            self.logger.warning("添加边时缺少source或target")
            return False

        # 验证节点是否存在
        if not self.get_node_by_id(source_id) or not self.get_node_by_id(target_id):
            self.logger.warning(f"边的源节点({source_id})或目标节点({target_id})不存在")
            return False

        # 检查边是否已存在
        if self.get_edge(source_id, target_id):
            self.logger.warning(f"边 {source_id} -> {target_id} 已存在")
            return False

        # 添加边
        self._edges.append(edge_data.copy())
        self.logger.info(f"添加边: {source_id} -> {target_id}")
        return True

    def _update_edge(self, edge_data: Dict[str, Any]) -> bool:
        """更新边

        Args:
            edge_data: 边数据，必须包含source和target

        Returns:
            是否更新成功
        """
        # source_id = edge_data.get('source')
        # target_id = edge_data.get('target')

        # if not source_id or not target_id:
        #     self.logger.warning("更新边时缺少source或target")
        #     return False

        # source_id = str(source_id)
        # target_id = str(target_id)

        # # 查找并更新边
        # for i, edge in enumerate(self._edges):
        #     if (str(edge.get('source')) == source_id and
        #             str(edge.get('target')) == target_id):
        #         # 深度合并更新
        #         updated_edge = edge.copy()
        #         self._deep_update(updated_edge, edge_data)
        #         self._edges[i] = updated_edge
        #         self.logger.info(f"更新边: {source_id} -> {target_id}")
        #         return True

        # self.logger.warning(f"未找到要更新的边: {source_id} -> {target_id}")
        # return False

        source_id = edge_data.get("source")
        new_target_id = edge_data.get("target")
        edge_type = edge_data.get("type")

        if not all([source_id, new_target_id, edge_type]):
            self.logger.warning(
                f"更新边时缺少必要的键: 'source', 'target', 或 'type'。提供的数据: {edge_data}"
            )
            return False

        for i, edge in enumerate(self._edges):
            if (
                str(edge.get("source")) == str(source_id)
                and str(edge.get("type")) == edge_type
            ):
                self._edges[i]["target"] = new_target_id
                return True

        return False

    def _remove_edge(self, edge_id: str) -> bool:
        """移除边

        Args:
            edge_id: 边ID，格式为 'source_id->target_id' 或直接的边ID

        Returns:
            是否移除成功
        """
        # 尝试解析edge_id
        if "->" in edge_id:
            source_id, target_id = edge_id.split("->", 1)
            source_id = source_id.strip()
            target_id = target_id.strip()

            # 按source和target查找
            for i, edge in enumerate(self._edges):
                if str(edge.get("source")) == str(source_id) and str(
                    edge.get("target")
                ) == str(target_id):
                    self._edges.pop(i)
                    self.logger.info(f"移除边: {source_id} -> {target_id}")
                    return True
        else:
            # 按边ID查找
            for i, edge in enumerate(self._edges):
                if str(edge.get("id", "")) == str(edge_id):
                    self._edges.pop(i)
                    self.logger.info(f"移除边: {edge_id}")
                    return True

        self.logger.warning(f"未找到要移除的边: {edge_id}")
        return False

    # def _deep_update(self, target: Dict[str, Any], source: Dict[str, Any]) -> None:
    #     """深度更新字典

    #     Args:
    #         target: 目标字典
    #         source: 源字典
    #     """
    #     for key, value in source.items():
    #         if key in target and isinstance(target[key], dict) and isinstance(value, dict):
    #             self._deep_update(target[key], value)
    #         else:
    #             target[key] = value

    def _deep_update(self, target_dict: Dict[str, Any], source: Dict[str, Any]) -> bool:
        """
        在 target_dict 中“就地”查找与 source 同名的键并覆盖其值；
        若当前层未命中且该值为 dict，则递归向下继续寻找。
        命中时无论旧值/新值是否为 dict，均直接整体替换（不做合并）。
        """
        updated = False

        for key, value in list(target_dict.items()):
            # 情况1：当前层命中 -> 直接覆盖（包括 dict 也整体替换）
            if key in source:
                target_dict[key] = source[key]
                updated = True
            # 情况2：未命中，但当前值是 dict -> 向更深层继续查找/覆盖
            elif isinstance(value, dict):
                if self._deep_update(value, source):
                    updated = True

        return updated

    # ==================== 工具方法 ====================

    def get_id(self) -> str:
        """获取上下文ID"""
        return f"simplified_context_{id(self)}"

    def get_scene_config(self) -> Dict[str, Any]:
        """获取场景配置"""
        return {"nodes": self.get_all_nodes(), "edges": self.get_all_edges()}

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "id": self.get_id(),
            "nodes": self.get_all_nodes(),
            "edges": self.get_all_edges(),
            "statistics": {
                "total_nodes": len(self._nodes),
                "total_edges": len(self._edges),
                "nodes_by_category": self._get_node_statistics(),
            },
        }

    def _get_node_statistics(self) -> Dict[str, int]:
        """获取节点统计信息"""
        stats = {}
        for node in self._nodes:
            category = node.get("properties", {}).get("category", "unknown")
            stats[category] = stats.get(category, 0) + 1
        return stats

    def to_json(self) -> str:
        """转换为JSON字符串"""
        return json.dumps(self.to_dict(), indent=2, ensure_ascii=False)

    def clear(self) -> None:
        """清空所有数据"""
        self._nodes.clear()
        self._edges.clear()
        self.logger.info("清空所有节点和边")

    def load_from_dict(self, data: Dict[str, Any]) -> bool:
        """从字典加载数据

        Args:
            data: 包含nodes和edges的字典

        Returns:
            是否加载成功
        """
        try:
            self._nodes = data.get("nodes", [])
            self._edges = data.get("edges", [])
            self.logger.info(
                f"从字典加载数据: {len(self._nodes)} 个节点, {len(self._edges)} 条边"
            )
            return True
        except Exception as e:
            self.logger.error(f"从字典加载数据失败: {e}")
            return False

    def __str__(self) -> str:
        """字符串表示"""
        return (
            f"SimplifiedTaskContext(nodes={len(self._nodes)}, edges={len(self._edges)})"
        )

    def __repr__(self) -> str:
        """详细字符串表示"""
        stats = self._get_node_statistics()
        return (
            f"SimplifiedTaskContext(id={self.get_id()}, "
            f"nodes={len(self._nodes)}, edges={len(self._edges)}, "
            f"categories={stats})"
        )

    def __del__(self):
        """析构函数，清理事件订阅"""
        try:
            for subscription_id in self._event_subscriptions:
                unsubscribe_event(subscription_id)
            self.logger.info("Event subscriptions cleaned up")
        except Exception as e:
            self.logger.error(f"Error cleaning up event subscriptions: {e}")

    async def cleanup(self):
        """异步清理方法

        清理事件订阅和释放资源。
        建议在不再使用上下文时调用此方法。
        """
        try:
            for subscription_id in self._event_subscriptions:
                unsubscribe_event(subscription_id)
            self._event_subscriptions.clear()
            self._is_initialized = False
            self.logger.info("SimplifiedTaskContext cleanup completed")
        except Exception as e:
            self.logger.error(f"Error during cleanup: {e}")
            raise

    # ==================== 事件驱动的数据修改接口 ====================

    async def add_node_async(
        self, node_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步添加节点（通过事件机制）

        Args:
            node_data: 节点数据
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            event = DataModificationEvent(
                operation="add",
                entity_type="node",
                entity_id=node_data.get("id", ""),
                data=node_data,
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish add node event: {e}")
            return False

    async def update_node_async(
        self, node_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步更新节点（通过事件机制）

        Args:
            node_data: 节点数据
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            event = DataModificationEvent(
                operation="update",
                entity_type="node",
                entity_id=node_data.get("id", ""),
                data=node_data,
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish update node event: {e}")
            return False

    async def remove_node_async(
        self, node_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步删除节点（通过事件机制）

        Args:
            node_id: 节点ID
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            node_id = node_data.get("id")
            if not node_id:
                self.logger.warning("Remove node requires node ID")
                return False

            event = DataModificationEvent(
                operation="remove",
                entity_type="node",
                entity_id=node_id,
                data={"id": node_id},
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish remove node event: {e}")
            return False

    async def add_edge_async(
        self, edge_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步添加边（通过事件机制）

        Args:
            edge_data: 边数据
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            edge_id = (
                edge_data.get("id")
                or f"{edge_data.get('source')}->{edge_data.get('target')}"
            )
            event = DataModificationEvent(
                operation="add",
                entity_type="edge",
                entity_id=edge_id,
                data=edge_data,
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish add edge event: {e}")
            return False

    async def update_edge_async(
        self, edge_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步更新边（通过事件机制）

        Args:
            edge_data: 边数据
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            edge_id = (
                edge_data.get("id")
                or f"{edge_data.get('source')}->{edge_data.get('target')}"
            )
            event = DataModificationEvent(
                operation="update",
                entity_type="edge",
                entity_id=edge_id,
                data=edge_data,
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish update edge event: {e}")
            return False

    async def remove_edge_async(
        self, edge_data: Dict[str, Any], request_id: Optional[str] = None
    ) -> bool:
        """异步删除边（通过事件机制）

        Args:
            source_id: 源节点ID
            target_id: 目标节点ID
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            source_id = edge_data.get("source")
            target_id = edge_data.get("target")
            if not source_id or not target_id:
                self.logger.warning("Remove edge requires source and target IDs")
                return False

            edge_id = f"{source_id}->{target_id}"
            event = DataModificationEvent(
                operation="remove",
                entity_type="edge",
                entity_id=edge_id,
                data={"source": source_id, "target": target_id, "id": edge_id},
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish remove edge event: {e}")
            return False

    async def remove_edge_by_id_async(
        self, edge_id: str, request_id: Optional[str] = None
    ) -> bool:
        """异步删除边（通过边ID，使用事件机制）

        Args:
            edge_id: 边ID
            request_id: 请求ID，用于追踪操作结果

        Returns:
            bool: 操作是否成功发布
        """
        try:
            event = DataModificationEvent(
                operation="remove",
                entity_type="edge",
                entity_id=edge_id,
                data={"id": edge_id},
                request_id=request_id or str(uuid.uuid4()),
            )
            await publish_event_sync(event)
            return True
        except Exception as e:
            self.logger.error(f"Failed to publish remove edge by id event: {e}")
            return False


# ==================== 全局单例管理 ====================

# 全局SimplifiedTaskContext实例管理
_global_simplified_task_context: Optional[SimplifiedTaskContext] = None
_context_lock = asyncio.Lock()


def get_global_simplified_task_context() -> SimplifiedTaskContext:
    """获取全局SimplifiedTaskContext实例

    Returns:
        SimplifiedTaskContext: 全局唯一实例
    """
    global _global_simplified_task_context
    if _global_simplified_task_context is None:
        _global_simplified_task_context = SimplifiedTaskContext()
    return _global_simplified_task_context


async def initialize_global_simplified_task_context(
    initial_nodes: Optional[List[Dict[str, Any]]] = None,
    initial_edges: Optional[List[Dict[str, Any]]] = None,
    initial_goal: Optional[str] = None,
    new_case: Optional[List[Dict[str, Any]]] = None,
) -> SimplifiedTaskContext:
    """初始化全局SimplifiedTaskContext实例

    Args:
        initial_nodes: 初始节点列表
        initial_edges: 初始边列表
        initial_goal: 初始目标
        new_case: 新情况

    Returns:
        SimplifiedTaskContext: 全局唯一实例
    """
    async with _context_lock:
        global _global_simplified_task_context
        if _global_simplified_task_context is None:
            _global_simplified_task_context = SimplifiedTaskContext(
                initial_nodes=initial_nodes,
                initial_edges=initial_edges,
                initial_goal=initial_goal,
                new_case=new_case,
            )
        else:
            print(f"SimplifiedTaskContext already initialized: {_global_simplified_task_context}")
        return _global_simplified_task_context


async def cleanup_global_simplified_task_context():
    """清理全局SimplifiedTaskContext实例

    清理事件订阅和释放资源。
    """
    async with _context_lock:
        global _global_simplified_task_context
        if _global_simplified_task_context:
            await _global_simplified_task_context.cleanup()
            _global_simplified_task_context = None


def reset_global_simplified_task_context():
    """重置全局SimplifiedTaskContext实例

    强制重新创建实例，主要用于测试场景。
    注意：使用前请确保已经清理了旧实例的资源。
    """
    global _global_simplified_task_context
    SimplifiedTaskContext._instance = None
    _global_simplified_task_context = None