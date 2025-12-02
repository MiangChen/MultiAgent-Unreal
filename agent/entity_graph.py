"""
EntityGraph - 基于 NetworkX 的多智能体图管理
"""
from enum import Enum
from typing import Any, Dict, List, Optional, Tuple

import networkx as nx

from agent.agent import Agent


class EntityType(str, Enum):
    """实体类型"""
    DRONE = "drone"
    CHARACTER = "character"
    ANIMAL = "animal"
    OBJECT = "object"
    LANDMARK = "landmark"
    VEHICLE = "vehicle"


class RelationType(str, Enum):
    """关系类型"""
    NEAR = "near"
    TRACKING = "tracking"
    VISIBLE = "visible"
    OWNS = "owns"
    COLLISION = "collision"
    FOLLOW = "follow"


# 颜色常量
TYPE_COLORS = {
    EntityType.DRONE: "#3498db",
    EntityType.CHARACTER: "#e74c3c",
    EntityType.ANIMAL: "#2ecc71",
    EntityType.OBJECT: "#95a5a6",
    EntityType.LANDMARK: "#9b59b6",
    EntityType.VEHICLE: "#f39c12",
}

EDGE_COLORS = {
    RelationType.TRACKING: "#e74c3c",
    RelationType.NEAR: "#bdc3c7",
    RelationType.VISIBLE: "#3498db",
    RelationType.OWNS: "#2ecc71",
    RelationType.COLLISION: "#e67e22",
    RelationType.FOLLOW: "#9b59b6",
}

# 重要关系（实线显示）
IMPORTANT_RELATIONS = {RelationType.TRACKING, RelationType.OWNS}


class EntityGraph:
    """基于 NetworkX 的实体图管理器"""

    def __init__(self):
        self.graph = nx.DiGraph()
        self._type_index: Dict[EntityType, set] = {}

    # ==================== 节点操作 ====================

    def add_entity(self, agent: Agent, entity_type: EntityType, **props) -> None:
        """添加实体"""
        self.graph.add_node(agent.name, agent=agent, entity_type=entity_type, **props)
        self._type_index.setdefault(entity_type, set()).add(agent.name)

    def remove_entity(self, name: str) -> bool:
        """移除实体"""
        if name not in self.graph:
            return False
        entity_type = self.graph.nodes[name].get('entity_type')
        if entity_type in self._type_index:
            self._type_index[entity_type].discard(name)
        self.graph.remove_node(name)
        return True

    def get_entity(self, name: str) -> Optional[Agent]:
        """获取实体"""
        return self.graph.nodes[name].get('agent') if name in self.graph else None

    def get_all_entities(self) -> List[Agent]:
        """获取所有实体"""
        return [d['agent'] for _, d in self.graph.nodes(data=True)]

    def get_entities_by_type(self, entity_type: EntityType) -> List[Agent]:
        """按类型获取"""
        names = self._type_index.get(entity_type, set())
        return [self.graph.nodes[n]['agent'] for n in names if n in self.graph]

    # ==================== 边操作 ====================

    def add_relation(self, source: str, target: str, relation: RelationType, **attrs) -> bool:
        """添加关系"""
        if source not in self.graph or target not in self.graph:
            return False
        self.graph.add_edge(source, target, relation=relation, **attrs)
        return True

    def remove_relation(self, source: str, target: str, relation: RelationType = None) -> bool:
        """移除关系"""
        if not self.graph.has_edge(source, target):
            return False
        if relation is None or self.graph.edges[source, target].get('relation') == relation:
            self.graph.remove_edge(source, target)
            return True
        return False

    # ==================== 查询 ====================

    def get_targets(self, source: str, relation: RelationType = None) -> List[str]:
        """获取目标"""
        if source not in self.graph:
            return []
        return [t for t in self.graph.successors(source)
                if relation is None or self.graph.edges[source, t].get('relation') == relation]

    def get_sources(self, target: str, relation: RelationType = None) -> List[str]:
        """获取源"""
        if target not in self.graph:
            return []
        return [s for s in self.graph.predecessors(target)
                if relation is None or self.graph.edges[s, target].get('relation') == relation]

    def get_neighbors(self, name: str, relation: RelationType = None) -> List[str]:
        """获取邻居（双向）"""
        return list(set(self.get_targets(name, relation)) | set(self.get_sources(name, relation)))

    # ==================== 空间关系 ====================

    def update_spatial_relations(self, distance_threshold: float = 500.0, relation: RelationType = RelationType.NEAR) -> int:
        """更新空间关系"""
        # 清除旧关系
        self.graph.remove_edges_from([
            (u, v) for u, v, d in self.graph.edges(data=True) if d.get('relation') == relation
        ])

        nodes = list(self.graph.nodes)
        count = 0
        for i, n1 in enumerate(nodes):
            a1 = self.graph.nodes[n1].get('agent')
            if not a1:
                continue
            try:
                loc1 = a1.get_location()
            except Exception:
                continue

            for n2 in nodes[i + 1:]:
                a2 = self.graph.nodes[n2].get('agent')
                if not a2:
                    continue
                try:
                    dist = loc1.distance_to(a2.get_location())
                except Exception:
                    continue

                if dist < distance_threshold:
                    self.graph.add_edge(n1, n2, relation=relation, distance=dist)
                    self.graph.add_edge(n2, n1, relation=relation, distance=dist)
                    count += 2
        return count

    # ==================== 图算法 ====================

    def shortest_path(self, source: str, target: str) -> Optional[List[str]]:
        try:
            return nx.shortest_path(self.graph, source, target)
        except nx.NetworkXNoPath:
            return None

    def get_connected_components(self) -> List[List[str]]:
        return [list(c) for c in nx.connected_components(self.graph.to_undirected())]

    # ==================== 序列化 ====================

    def to_dict(self) -> Dict[str, Any]:
        """导出为字典"""
        nodes = []
        for name, data in self.graph.nodes(data=True):
            agent = data.get('agent')
            node = {'name': name, 'entity_type': str(data.get('entity_type', ''))}
            if agent:
                node['class_name'] = agent.class_name
                try:
                    node['location'] = agent.get_location().to_list()
                except Exception:
                    pass
            nodes.append(node)

        edges = [{'source': u, 'target': v, 'relation': str(d.get('relation', '')),
                  **{k: val for k, val in d.items() if k != 'relation'}}
                 for u, v, d in self.graph.edges(data=True)]

        return {'nodes': nodes, 'edges': edges}

    # ==================== 魔术方法 ====================

    def __len__(self) -> int:
        return len(self.graph)

    def __contains__(self, name: str) -> bool:
        return name in self.graph

    def __repr__(self) -> str:
        types = {t.value: len(n) for t, n in self._type_index.items() if n}
        return f"EntityGraph(nodes={len(self.graph)}, edges={self.graph.number_of_edges()}, types={types})"

    # ==================== 可视化 ====================

    def _get_layout(self, layout: str) -> dict:
        """获取布局"""
        if layout == "circular":
            return nx.circular_layout(self.graph)
        if layout == "kamada_kawai" and len(self.graph) > 0:
            return nx.kamada_kawai_layout(self.graph)
        if layout == "shell":
            shells = [list(n) for n in self._type_index.values() if n]
            return nx.shell_layout(self.graph, nlist=shells) if shells else nx.spring_layout(self.graph)
        return nx.spring_layout(self.graph, k=2, iterations=50)

    def _get_node_colors(self) -> List[str]:
        """获取节点颜色列表"""
        return [TYPE_COLORS.get(self.graph.nodes[n].get('entity_type'), "#95a5a6")
                for n in self.graph.nodes]

    def visualize(self, figsize=(12, 8), save_path=None, show=True, title="Entity Graph", layout="spring"):
        """可视化实体图"""
        import matplotlib.pyplot as plt
        from matplotlib.lines import Line2D

        fig, ax = plt.subplots(figsize=figsize)
        pos = self._get_layout(layout)

        # 绘制节点
        for etype, names in self._type_index.items():
            nodes = [n for n in names if n in self.graph]
            if nodes:
                nx.draw_networkx_nodes(self.graph, pos, nodelist=nodes,
                                       node_color=TYPE_COLORS.get(etype, "#95a5a6"),
                                       node_size=800, alpha=0.9, label=etype.value, ax=ax)

        # 按关系类型绘制边
        edges_by_rel: Dict[RelationType, List] = {}
        for u, v, d in self.graph.edges(data=True):
            rel = d.get('relation', RelationType.NEAR)
            edges_by_rel.setdefault(rel, []).append((u, v))

        for rel, edges in edges_by_rel.items():
            is_important = rel in IMPORTANT_RELATIONS
            nx.draw_networkx_edges(self.graph, pos, edgelist=edges,
                                   edge_color=EDGE_COLORS.get(rel, "#bdc3c7"),
                                   style="solid" if is_important else "dashed",
                                   width=2.0 if is_important else 1.0,
                                   alpha=0.7, arrows=True, arrowsize=15,
                                   connectionstyle="arc3,rad=0.1", ax=ax)

        nx.draw_networkx_labels(self.graph, pos, font_size=8, font_weight="bold", ax=ax)
        ax.legend(loc="upper left", fontsize=10)

        # 边图例
        if edges_by_rel:
            legend_lines = [Line2D([0], [0], color=EDGE_COLORS.get(r, "#bdc3c7"),
                                   linestyle="-" if r in IMPORTANT_RELATIONS else "--", label=r.value)
                           for r in edges_by_rel]
            ax2 = ax.twinx()
            ax2.axis('off')
            ax2.legend(handles=legend_lines, loc="upper right", fontsize=9, title="Relations")

        ax.set_title(title, fontsize=14, fontweight="bold")
        ax.axis("off")
        plt.tight_layout()

        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"   📊 Graph saved to {save_path}")
        plt.show() if show else plt.close()

    def visualize_multi_view(self, figsize=(16, 12), save_path=None, show=True):
        """多视图可视化"""
        import matplotlib.pyplot as plt
        from matplotlib.patches import Patch

        fig, axes = plt.subplots(2, 2, figsize=figsize)
        node_colors = self._get_node_colors()
        pos_spring = nx.spring_layout(self.graph, k=2, iterations=50)

        # 1. 完整图
        nx.draw(self.graph, pos_spring, ax=axes[0, 0], node_color=node_colors,
                node_size=600, with_labels=True, font_size=7, edge_color="#bdc3c7", arrows=True)
        axes[0, 0].set_title("Complete Graph (Spring)", fontweight="bold")

        # 2. Shell 布局
        pos_shell = self._get_layout("shell")
        nx.draw(self.graph, pos_shell, ax=axes[0, 1], node_color=node_colors,
                node_size=600, with_labels=True, font_size=7, edge_color="#bdc3c7", arrows=True)
        axes[0, 1].set_title("By Type (Shell)", fontweight="bold")

        # 3. TRACKING 关系
        self._draw_relation_subgraph(axes[1, 0], pos_spring, RelationType.TRACKING, "#e74c3c")

        # 4. NEAR 关系
        self._draw_relation_subgraph(axes[1, 1], pos_spring, RelationType.NEAR, "#3498db", style="dashed")

        # 图例
        patches = [Patch(color=c, label=t.value) for t, c in TYPE_COLORS.items()
                   if t in self._type_index and self._type_index[t]]
        fig.legend(handles=patches, loc="lower center", ncol=len(patches), fontsize=10)
        plt.suptitle(f"Entity Graph: {len(self.graph)} nodes, {self.graph.number_of_edges()} edges",
                     fontsize=14, fontweight="bold")
        plt.tight_layout(rect=[0, 0.05, 1, 0.95])

        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"   📊 Multi-view saved to {save_path}")
        plt.show() if show else plt.close()

    def _draw_relation_subgraph(self, ax, pos, relation, color, style="solid"):
        """绘制特定关系的子图"""
        edges = [(u, v) for u, v, d in self.graph.edges(data=True) if d.get('relation') == relation]
        ax.set_title(f"{relation.value.upper()} ({len(edges)} edges)", fontweight="bold")
        if edges:
            subgraph = self.graph.edge_subgraph(edges)
            colors = [TYPE_COLORS.get(self.graph.nodes[n].get('entity_type'), "#95a5a6") for n in subgraph.nodes]
            sub_pos = {n: pos[n] for n in subgraph.nodes}
            nx.draw(subgraph, sub_pos, ax=ax, node_color=colors, node_size=600,
                    with_labels=True, font_size=7, edge_color=color, width=2, style=style, arrows=True)

    def visualize_spatial(self, figsize=(10, 10), save_path=None, show=True):
        """空间视图（基于 UE 坐标）"""
        import matplotlib.pyplot as plt

        fig, ax = plt.subplots(figsize=figsize)

        # 获取实际位置
        pos = {}
        for name in self.graph.nodes:
            agent = self.graph.nodes[name].get('agent')
            try:
                loc = agent.get_location() if agent else None
                pos[name] = (loc.x, loc.y) if loc else (0, 0)
            except Exception:
                pos[name] = (0, 0)

        # 绘制节点
        for etype, names in self._type_index.items():
            nodes = [n for n in names if n in self.graph]
            if nodes:
                nx.draw_networkx_nodes(self.graph, pos, nodelist=nodes,
                                       node_color=TYPE_COLORS.get(etype, "#95a5a6"),
                                       node_size=500, alpha=0.9, label=etype.value, ax=ax)

        # 绘制边
        for u, v, d in self.graph.edges(data=True):
            rel = d.get('relation', RelationType.NEAR)
            nx.draw_networkx_edges(self.graph, pos, edgelist=[(u, v)],
                                   edge_color=EDGE_COLORS.get(rel, "#bdc3c7"),
                                   style="solid" if rel == RelationType.TRACKING else "dashed",
                                   alpha=0.5, arrows=True, arrowsize=10, ax=ax)

        nx.draw_networkx_labels(self.graph, pos, font_size=7, ax=ax)
        ax.legend(loc="upper left")
        ax.set_xlabel("X (UE)")
        ax.set_ylabel("Y (UE)")
        ax.set_title("Spatial View (Top-Down)", fontweight="bold")
        ax.grid(True, alpha=0.3)
        ax.set_aspect('equal')

        if save_path:
            plt.savefig(save_path, dpi=150, bbox_inches="tight")
            print(f"   📊 Spatial view saved to {save_path}")
        plt.show() if show else plt.close()
