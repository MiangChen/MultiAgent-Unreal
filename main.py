"""
UE 多智能体环境启动脚本

使用图论 (NetworkX) 管理多智能体

使用方法:
    python main.py
"""
import sys
import time
from pathlib import Path

import cv2

project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from unrealcv.launcher import RunUnreal
from unrealcv.api import UnrealCv_API

from config.config_manager import config_manager
from map.map_grid import MapGrid
from agent.agent_manager import AgentManager
from agent.entity_graph import EntityType, RelationType
from agent.transform import Location, Scale, Transform
from agent.character import Character


def main():
    print("=" * 60)
    print("🎮 UE 多智能体环境 (图论管理)")
    print("=" * 60)

    # 1. 加载配置
    config = config_manager.load_env_config()
    print(f"\n📋 配置:")
    print(f"   Agent 配置: {config_manager.get_agent_config_name()}")
    print(f"   UE 地图: {config_manager.get_env_map()}")

    env_bin = config_manager.get_ue_binary_path()
    env_map = config_manager.get_env_map()
    resolution = tuple(config_manager.get("rendering.resolution", [640, 480]))

    # 2. 启动 UE
    print(f"\n🚀 启动 Unreal Engine...")
    launcher = RunUnreal(ENV_BIN=env_bin, ENV_MAP=env_map)

    try:
        ip, port = launcher.start(
            docker=False,
            resolution=resolution,
            offscreen=False,
            sleep_time=10
        )
        print(f"✅ UE 启动成功! IP={ip}, Port={port}")

        # 3. 连接 UnrealCV
        print(f"\n🔌 连接 UnrealCV...")
        client = UnrealCv_API(port, ip, resolution, mode='tcp')
        print(f"✅ 连接成功!")

        # 4. 初始化 AgentManager (图论管理)
        manager = AgentManager(client, config)

        # 5. 清理场景
        print(f"\n🧹 清理场景...")
        manager.cleanup_scene()

        # 6. Spawn 所有实体
        print(f"\n🤖 Spawn 实体...")
        manager.spawn_from_config()

        # 7. 展示图论查询能力
        print(f"\n📊 图论查询示例:")

        # 按类型查询
        drones = manager.get_by_type(EntityType.DRONE)
        characters = manager.get_by_type(EntityType.CHARACTER)
        print(f"   无人机: {[d.name for d in drones]}")
        print(f"   角色: {[c.name for c in characters]}")

        # 建立追踪关系: 所有无人机追踪第一个角色
        if drones and characters:
            target = characters[0]
            print(f"\n🎯 建立追踪关系 -> {target.name}")
            for drone in drones:
                manager.add_relation(drone.name, target.name, RelationType.TRACKING)
                print(f"   {drone.name} --[TRACKING]--> {target.name}")

            # 查询谁在追踪 target
            trackers = manager.get_sources(target.name, RelationType.TRACKING)
            print(f"   追踪 {target.name} 的实体: {trackers}")

        # 更新空间关系
        print(f"\n🗺️ 更新空间关系 (阈值=2000)...")
        edge_count = manager.update_spatial_relations(distance_threshold=2000)
        print(f"   更新了 {edge_count} 条 NEAR 边")

        # 查询邻居
        if drones:
            first_drone = drones[0]
            neighbors = manager.get_neighbors(first_drone.name, RelationType.NEAR)
            print(f"   {first_drone.name} 附近: {neighbors}")

        # 导出图数据
        graph_data = manager.export_graph()
        print(f"\n📈 图数据:")
        print(f"   节点: {len(graph_data['nodes'])}")
        print(f"   边: {len(graph_data['edges'])}")

        # 8. 为无人机绑定传感器并保存图像
        print(f"\n📷 绑定传感器...")
        output_dir = Path("/home/ubuntu/Pictures")
        output_dir.mkdir(parents=True, exist_ok=True)

        for i, drone in enumerate(drones):
            if drone.cam_id < 0:
                continue

            sensor_type = "sensor.camera.rgb" if i % 2 == 0 else "sensor.camera.depth"
            sensor = manager.spawn_sensor(sensor_type, attach_to=drone)

            if sensor is None:
                continue

            if "rgb" in sensor_type:
                img = sensor.get_image()
                if img is not None:
                    filename = output_dir / f"{drone.name}_rgb.png"
                    cv2.imwrite(str(filename), cv2.cvtColor(img, cv2.COLOR_RGB2BGR))
                    print(f"   {drone.name}: RGB -> {filename}")
            else:
                depth = sensor.get_depth()
                if depth is not None and depth.max() > 0:
                    depth_norm = (depth / depth.max() * 255).astype("uint8")
                    filename = output_dir / f"{drone.name}_depth.png"
                    cv2.imwrite(str(filename), depth_norm)
                    print(f"   {drone.name}: Depth -> {filename}")

        # 9. 在角色附近生成物体
        if characters:
            char = characters[0]
            char_loc = char.get_location()
            print(f"\n🧊 在 {char.name} 附近生成物体...")

            item = manager.spawn_static(
                name="grabbable_item",
                transform=Transform(
                    location=Location(char_loc.x + 200, char_loc.y, char_loc.z),
                    scale=Scale(1, 1, 1),
                ),
                shape="object",
            )

            if item:
                # 建立 NEAR 关系
                manager.add_relation(char.name, item.name, RelationType.NEAR, distance=200)
                print(f"   {char.name} --[NEAR]--> {item.name}")

        # 10. 角色拾取物体演示
        if characters and "grabbable_item" in manager:
            print(f"\n⏳ 等待 3 秒后拾取...")
            time.sleep(3)

            char = characters[0]
            character = Character(client, char)
            item = manager["grabbable_item"]
            item_loc = item.get_location()

            print(f"   导航到物体...")
            character.nav_to(item_loc.x, item_loc.y, item_loc.z)
            time.sleep(2)

            print(f"   执行拾取...")
            character.carry()
            time.sleep(1)

            if character.is_carrying():
                print(f"   ✅ 拾取成功!")
                # 更新关系: 角色拥有物体
                manager.add_relation(char.name, item.name, RelationType.OWNS)
            else:
                print(f"   ❌ 拾取失败")

        # 11. 最终图状态
        print(f"\n📊 最终图状态: {manager.graph}")
        final_data = manager.export_graph()
        print(f"   关系:")
        for edge in final_data['edges'][:10]:  # 只显示前10条
            print(f"     {edge['source']} --[{edge['relation']}]--> {edge['target']}")

        # 12. 可视化图
        print(f"\n📈 可视化实体图...")
        vis_dir = Path("/home/ubuntu/Pictures")
        
        # 单视图
        manager.graph.visualize(
            save_path=str(vis_dir / "entity_graph.png"),
            show=False,
            title="Entity Graph"
        )
        
        # 多视图
        manager.graph.visualize_multi_view(
            save_path=str(vis_dir / "entity_graph_multi.png"),
            show=False
        )
        
        # 空间视图 (基于实际 UE 坐标)
        manager.graph.visualize_spatial(
            save_path=str(vis_dir / "entity_graph_spatial.png"),
            show=False
        )

        # 等待用户
        print(f"\n" + "=" * 60)
        print("✅ 环境就绪! 按 Enter 关闭...")
        print("=" * 60)
        input()

    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()

    finally:
        print(f"\n🛑 关闭...")
        try:
            client.client.disconnect()
        except:
            pass
        launcher.close()
        print("✅ 已关闭")


if __name__ == '__main__':
    main()
