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
        print(f"   已创建 agents: {spawned}")

        # 9. 保持运行，等待用户输入
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
