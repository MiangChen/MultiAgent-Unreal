"""
地图栅格模块 - 用于获取和可视化 NavMesh 信息
"""
# Standard library imports
from typing import Any, Dict, List, Optional

# Third-party library imports
import matplotlib.pyplot as plt
import numpy as np


class MapGrid:
    """地图栅格类，用于管理 NavMesh 和栅格地图"""

    def __init__(self, env_config: Dict[str, Any]):
        self.env_config = env_config
        self.navmesh_info: Optional[Dict] = None
        self.grid: Optional[np.ndarray] = None
        self.resolution = 1.0  # 栅格分辨率 (米/格)

    def load_from_client(self, client, objects: List[str]) -> Optional[Dict]:
        """从 UnrealCV client 获取 NavMesh 信息"""
        self.navmesh_info = get_navmesh_info(client, objects)
        return self.navmesh_info

    def get_reset_area(self) -> List[float]:
        """获取重置区域边界 [x_min, x_max, y_min, y_max, z_min, z_max]"""
        return self.env_config.get("reset_area", [0, 0, 0, 0, 0, 0])

    def get_safe_starts(self) -> List[List[float]]:
        """获取安全出生点列表"""
        return self.env_config.get("safe_start", [])

    def visualize(self, save_path: str = "navmesh_visualization.png", show: bool = True):
        """可视化地图"""
        visualize_navmesh(self.navmesh_info, self.env_config, save_path, show)


def get_navmesh_info(client, objects: List[str]) -> Optional[Dict]:
    """
    获取 NavMesh 信息

    Args:
        client: UnrealCv_API 客户端
        objects: 场景对象列表

    Returns:
        NavMesh 信息字典，包含 name, bbox, location, area
    """
    for obj in objects:
        if "RecastNavMesh" in obj:
            uclass = client.get_obj_uclass(obj)
            bbox = client.get_obj_size(obj)
            # 转换为米 (UE 单位是厘米)
            bbox = [b / 100.0 for b in bbox]
            location = client.get_obj_location(obj)

            print(f"   NavMesh 对象: {obj}")
            print(f"   类型: {uclass}")
            print(f"   边界框 (米): {bbox}")
            print(f"   位置: {location}")
            print(f"   面积: {bbox[0] * bbox[1]:.2f} m²")

            return {
                "name": obj,
                "bbox": bbox,
                "location": location,
                "area": bbox[0] * bbox[1],
            }
    print("   未找到 NavMesh")
    return None


def visualize_navmesh(
    navmesh_info: Optional[Dict],
    env_config: Dict[str, Any],
    save_path: str = "navmesh_visualization.png",
    show: bool = True,
):
    """
    可视化 NavMesh 和出生点

    Args:
        navmesh_info: NavMesh 信息字典
        env_config: 环境配置
        save_path: 保存路径
        show: 是否显示图像
    """
    reset_area = env_config.get("reset_area", [0, 0, 0, 0, 0, 0])
    safe_starts = env_config.get("safe_start", [])

    fig, ax = plt.subplots(1, 1, figsize=(10, 10))

    # 绘制 reset_area 边界
    x_min, x_max = reset_area[0] / 100, reset_area[1] / 100
    y_min, y_max = reset_area[2] / 100, reset_area[3] / 100

    # 绘制可行区域边界框
    rect = plt.Rectangle(
        (x_min, y_min),
        x_max - x_min,
        y_max - y_min,
        fill=True,
        facecolor="lightgreen",
        edgecolor="green",
        linewidth=2,
        alpha=0.3,
        label="Reset Area",
    )
    ax.add_patch(rect)

    # 绘制出生点
    if safe_starts:
        starts_x = [p[0] / 100 for p in safe_starts]
        starts_y = [p[1] / 100 for p in safe_starts]
        ax.scatter(
            starts_x,
            starts_y,
            c="red",
            s=100,
            marker="*",
            label="Safe Start Points",
            zorder=5,
        )

    ax.set_xlabel("X (m)")
    ax.set_ylabel("Y (m)")
    ax.set_title(f"NavMesh Visualization - {env_config.get('env_name', 'Unknown')}")
    ax.legend()
    ax.set_aspect("equal")
    ax.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(save_path, dpi=150)
    print(f"   📊 NavMesh 可视化已保存: {save_path}")

    if show:
        plt.show()
    plt.close()
