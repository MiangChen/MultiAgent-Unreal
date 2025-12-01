"""
Agent 类 - 封装 UE 中的智能体对象
"""
from agent.transform import Location, Rotation, Scale, Transform


class Agent:
    """
    Agent 类，封装 UE 中的智能体操作
    """

    def __init__(self, client, name: str, class_name: str, transform: Transform):
        """
        初始化 Agent

        Args:
            client: UnrealCv_API 客户端
            name: 对象名称
            class_name: 蓝图类名
            transform: 变换信息
        """
        self.client = client
        self.name = name
        self.class_name = class_name
        self.transform = transform
        self._is_alive = True

    def get_location(self) -> Location:
        """获取当前位置"""
        loc = self.client.get_obj_location(self.name)
        return Location.from_list(loc)

    def set_location(self, location: Location) -> None:
        """设置位置"""
        x, y, z = location.to_list()
        cmd = f"vset /object/{self.name}/location {x} {y} {z}"
        self.client.client.request(cmd)
        self.transform.location = location

    def get_rotation(self) -> Rotation:
        """获取当前旋转"""
        rot = self.client.get_obj_rotation(self.name)
        return Rotation.from_list(rot)

    def set_rotation(self, rotation: Rotation) -> None:
        """设置旋转"""
        pitch, yaw, roll = rotation.to_list()
        cmd = f"vset /object/{self.name}/rotation {pitch} {yaw} {roll}"
        self.client.client.request(cmd)
        self.transform.rotation = rotation

    def set_scale(self, scale: Scale) -> None:
        """设置缩放"""
        sx, sy, sz = scale.to_list()
        cmd = f"vset /object/{self.name}/scale {sx} {sy} {sz}"
        self.client.client.request(cmd)
        self.transform.scale = scale

    def get_transform(self) -> Transform:
        """获取完整变换"""
        return Transform(
            location=self.get_location(),
            rotation=self.get_rotation(),
            scale=self.transform.scale,
        )

    def destroy(self) -> bool:
        """销毁 Agent"""
        if not self._is_alive:
            return False
        cmd = f"vset /object/{self.name}/destroy"
        self.client.client.request(cmd)
        self._is_alive = False
        print(f"   🗑️ Destroyed: {self.name}")
        return True

    def is_alive(self) -> bool:
        """检查 Agent 是否存活"""
        return self._is_alive

    def __repr__(self):
        return f"Agent(name='{self.name}', class='{self.class_name}')"
