# main.py
import os
import time
import unrealcv
from multiagent_unreal.launcher import UnrealLauncher


def main():
    project_path = os.path.expanduser("~/Documents/Unreal Projects/MyProjectC/MyProjectC.uproject")

    launcher = UnrealLauncher()
    ue_process = None

    try:
        # 1. 启动引擎
        print("🚀 启动 Unreal Engine...")
        ue_process = launcher.launch(project_path, headless=False)
        print(f"✅ 进程已启动,PID: {ue_process.pid}")

        # 2. 等待 UE 完全启动(包括 UnrealCV Server)
        wait_time = 15
        print(f"⏳ 等待 {wait_time} 秒让 UE 和 UnrealCV 完全启动...")
        time.sleep(wait_time)

        # 3. 连接 UnrealCV 客户端
        print("🔌 正在连接 UnrealCV...")
        client = unrealcv.Client(('127.0.0.1', 9000))
        client.connect(timeout=10)

        if client.isconnected():
            print("✅ UnrealCV 连接成功!")
            
            # 测试命令
            status = client.request('vget /unrealcv/status')
            print(f"状态: {status}")
            
            loc = client.request('vget /camera/0/location')
            print(f"相机位置: {loc}")
            
            # 你的业务逻辑
            print("\n执行你的任务...")
            time.sleep(5)
            
            client.disconnect()
            print("✅ 已断开连接")
        else:
            print("❌ 连接失败")

    except Exception as e:
        print(f"❌ 发生异常: {e}")
        import traceback
        traceback.print_exc()

    finally:
        # 4. 清理
        print("\n🛑 正在清理...")
        if ue_process:
            ue_process.kill()
            ue_process.wait()
            print("✅ 引擎进程已终止")


if __name__ == "__main__":
    main()
