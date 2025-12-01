"""
简单的 UE 启动脚本
演示如何读取 JSON 配置并启动 Unreal Engine

使用方法:
    python main.py

配置参数统一在 config/config_parameter.yaml 中设置
"""
# Standard library imports
import sys
from pathlib import Path

# Third-party library imports
import cv2

# 添加项目根目录到 Python 路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

# Third-party library imports
from unrealcv.launcher import RunUnreal
from unrealcv.api import UnrealCv_API

# Local project imports
from config.config_manager import config_manager
from map.map_grid import MapGrid
from agent.agent_manager import AgentManager


def main():
    print("=" * 60)
    print("🎮 简单 UE 启动脚本")
    print("=" * 60)


    # 1. 加载 JSON 配置
    config = config_manager.load_env_config()

    print(f"\n📋 配置信息:")
    print(f"   Agent 配置: {config_manager.get_agent_config_name()}")
    print(f"   UE 地图: {config_manager.get_env_map()}")
    print(f"   UE Binary: {config_manager.get_ue_binary_path()}")
    print(f"   智能体类型: {list(config['agents'].keys())}")
    print(f"   出生点数量: {len(config['safe_start'])}")

    # 2. 从配置提取启动参数
    env_bin = config_manager.get_ue_binary_path()
    env_map = config_manager.get_env_map()
    resolution = tuple(config_manager.get("rendering.resolution", [640, 480]))

    # 3. 创建 RunUnreal 启动器
    print(f"\n🚀 准备启动 Unreal Engine...")
    print(f"   Binary: {env_bin}")
    print(f"   Map: {env_map}")

    launcher = RunUnreal(ENV_BIN=env_bin, ENV_MAP=env_map)

    try:
        # 4. 启动 UE
        print(f"\n⏳ 启动中，请稍候...")
        ip, port = launcher.start(
            docker=False,
            resolution=resolution,
            offscreen=False,  # 显示窗口
            sleep_time=10
        )
        print(f"✅ UE 启动成功!")
        print(f"   IP: {ip}, Port: {port}")

        # 5. 创建 UnrealCV Client 连接
        print(f"\n🔌 连接 UnrealCV Server...")
        client = UnrealCv_API(port, ip, resolution, mode='tcp')
        print(f"✅ 连接成功!")

        # 6. 测试一些基本命令
        print(f"\n📡 测试 UnrealCV 命令:")

        # 获取相机数量
        cam_num = client.get_camera_num()
        print(f"   相机数量: {cam_num}")

        # 获取场景对象
        objects = client.get_objects()
        print(f"   场景对象数量: {len(objects)}")

        # 显示智能体
        agents_in_scene = [obj for obj in objects if 'BP_Character' in obj or 'BP_animal' in obj or 'BP_Drone' in obj]
        print(f"   场景中的智能体: {agents_in_scene[:5]}...")  # 只显示前5个

        # 获取第一个相机的图像
        print(f"\n📷 获取相机图像...")
        img = client.get_image(0, 'lit', 'bmp')
        if img is not None:
            print(f"   图像尺寸: {img.shape}")

        # 7. 获取 NavMesh 信息并可视化
        print(f"\n🗺️ 获取 NavMesh 信息...")
        map_grid = MapGrid(config)
        map_grid.load_from_client(client, objects)
        if map_grid.navmesh_info:
            map_grid.visualize()

        # 8. Spawn agents from config
        print(f"\n🤖 Spawn Agents...")
        agent_manager = AgentManager(client, config)
        spawned = agent_manager.spawn_agents_from_config()
        print(f"   已创建 {len(spawned)} 个 agents")

        # 9. 为每个 drone 创建 camera sensor（CARLA 风格）
        # 一半 RGB，一半 Depth
        print(f"\n📷 为 Drones 绑定 Camera Sensors...")
        output_dir = Path("/home/ubuntu/Pictures")
        output_dir.mkdir(parents=True, exist_ok=True)

        agents_with_cam = [a for a in spawned if a.cam_id >= 0]
        half = len(agents_with_cam) // 2

        for i, agent in enumerate(agents_with_cam):
            # 前一半用 RGB，后一半用 Depth
            if i < half:
                sensor_type = "sensor.camera.rgb"
                sensor = agent_manager.spawn_sensor(sensor_type, attach_to=agent)
                img = sensor.get_image()
                if img is not None:
                    filename = output_dir / f"{agent.name}_rgb.png"
                    cv2.imwrite(str(filename), cv2.cvtColor(img, cv2.COLOR_RGB2BGR))
                    print(f"   {agent.name}: RGB 已保存 {filename}")
            else:
                sensor_type = "sensor.camera.depth"
                sensor = agent_manager.spawn_sensor(sensor_type, attach_to=agent)
                depth = sensor.get_depth()
                if depth is not None:
                    # 深度图归一化后保存
                    depth_normalized = (depth / depth.max() * 255).astype("uint8")
                    filename = output_dir / f"{agent.name}_depth.png"
                    cv2.imwrite(str(filename), depth_normalized)
                    print(f"   {agent.name}: Depth 已保存 {filename}")

        # 10. 演示 sensor 的使用
        print(f"\n🎯 Sensor 使用示例:")
        first_agent = spawned[0] if spawned else None
        if first_agent and first_agent.has_sensor():
            sensor = first_agent.get_sensor("camera.rgb")
            print(f"   Agent: {first_agent.name}")
            print(f"   Sensor: {sensor}")
            print(f"   Camera Location: {sensor.get_location()}")
            print(f"   Camera Rotation: {sensor.get_rotation()}")

        # 11. 保持运行，等待用户输入
        print(f"\n" + "=" * 60)
        print("✅ UE 环境已就绪!")
        print("   按 Enter 键关闭环境...")
        print("=" * 60)
        input()

    except Exception as e:
        print(f"\n❌ 错误: {e}")
        import traceback
        traceback.print_exc()

    finally:
        # 8. 清理
        print(f"\n🛑 关闭 UE...")
        try:
            client.client.disconnect()
        except:
            pass
        launcher.close()
        print("✅ 已关闭")


if __name__ == '__main__':
    main()
