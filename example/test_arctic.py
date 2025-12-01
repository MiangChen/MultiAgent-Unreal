"""
Arctic 地图测试脚本
测试 Arctic 场景的基本功能，包括环境初始化、智能体交互等

使用方法:
    python example/test_arctic.py -e UnrealTrack-Arctic-ContinuousColor-v0
    python example/test_arctic.py -e UnrealNavigation-Arctic-MixedColor-v0
    python example/test_arctic.py --keyboard  # 键盘控制模式
"""

import argparse
import gym
import gym_unrealcv
import cv2
import time
import numpy as np
import os

# 如果环境变量未设置，可以在这里指定
# os.environ['UnrealEnv'] = '/path/to/your/UnrealEnv'

from gym_unrealcv.envs.wrappers import time_dilation, configUE, augmentation, agents
from gym_unrealcv.envs.tracking.baseline import PoseTracker


class RandomAgent:
    """随机动作智能体"""
    def __init__(self, action_space):
        self.action_space = action_space

    def act(self, observation=None):
        return self.action_space.sample()


def setup_keyboard_control():
    """设置键盘控制"""
    try:
        from pynput import keyboard
        
        key_state = {
            'i': False, 'j': False, 'k': False, 'l': False,
            'space': False, 'head_up': False, 'head_down': False
        }
        
        def on_press(key):
            try:
                if key.char in key_state:
                    key_state[key.char] = True
            except AttributeError:
                if key == keyboard.Key.space:
                    key_state['space'] = True
                elif key == keyboard.Key.up:
                    key_state['head_up'] = True
                elif key == keyboard.Key.down:
                    key_state['head_down'] = True

        def on_release(key):
            try:
                if key.char in key_state:
                    key_state[key.char] = False
            except AttributeError:
                if key == keyboard.Key.space:
                    key_state['space'] = False
                elif key == keyboard.Key.up:
                    key_state['head_up'] = False
                elif key == keyboard.Key.down:
                    key_state['head_down'] = False
        
        listener = keyboard.Listener(on_press=on_press, on_release=on_release)
        listener.start()
        return key_state
    except ImportError:
        print("⚠️ pynput 未安装，键盘控制不可用。请运行: pip install pynput")
        return None


def get_keyboard_action(key_state):
    """根据键盘状态获取动作"""
    action = [[0, 0], 0, 0]
    
    if key_state['i']:
        action[0][1] = 100
    if key_state['k']:
        action[0][1] = -100
    if key_state['j']:
        action[0][0] = -30
    if key_state['l']:
        action[0][0] = 30
    if key_state['space']:
        action[2] = 1
    if key_state['head_up']:
        action[1] = 1
    if key_state['head_down']:
        action[1] = 2
    
    return (tuple(action[0]), action[1], action[2])


def test_arctic(args):
    """测试 Arctic 环境"""
    print(f"🏔️ 正在初始化 Arctic 环境: {args.env_id}")
    print("=" * 50)
    
    # 创建环境
    env = gym.make(args.env_id)
    env = configUE.ConfigUEWrapper(env, offscreen=args.offscreen, resolution=(args.resolution, args.resolution))
    env.unwrapped.agents_category = ['player']
    
    # 等待 UE 启动
    print(f"⏳ 等待 Unreal Engine 启动 ({args.wait_time}秒)...")
    time.sleep(args.wait_time)
    
    # 添加时间膨胀（控制仿真速度）
    if args.time_dilation > 0:
        env = time_dilation.TimeDilationWrapper(env, args.time_dilation)
    
    # 添加智能体
    env = augmentation.RandomPopulationWrapper(env, args.min_agents, args.max_agents, random_target=False)
    
    if not args.keyboard:
        env = agents.NavAgents(env, mask_agent=True)
    
    env.seed(args.seed)
    
    # 设置键盘控制
    key_state = None
    if args.keyboard:
        key_state = setup_keyboard_control()
        if key_state:
            print("\n🎮 键盘控制已启用:")
            print("   I/K - 前进/后退")
            print("   J/L - 左转/右转")
            print("   Space - 跳跃")
            print("   ↑/↓ - 抬头/低头")
            print("   Q - 退出\n")
    
    # 创建智能体
    tracker = None
    random_agent = None
    
    print(f"✅ 环境初始化完成")
    print(f"   分辨率: {args.resolution}x{args.resolution}")
    print(f"   智能体数量: {args.min_agents}-{args.max_agents}")
    print(f"   模式: {'键盘控制' if args.keyboard else '自动追踪'}")
    print("=" * 50)
    
    total_rewards = 0
    episode_count = args.episodes
    
    try:
        for eps in range(1, episode_count + 1):
            print(f"\n📍 Episode {eps}/{episode_count}")
            obs = env.reset()
            
            agents_num = len(env.action_space) if hasattr(env.action_space, '__len__') else 1
            print(f"   智能体数量: {agents_num}")
            
            # 初始化追踪器
            if not args.keyboard and tracker is None:
                tracker = PoseTracker(env.action_space[0] if agents_num > 1 else env.action_space)
            if random_agent is None:
                random_agent = RandomAgent(env.action_space[0] if agents_num > 1 else env.action_space)
            
            step_count = 0
            episode_reward = 0
            t0 = time.time()
            
            while True:
                # 获取动作
                if args.keyboard and key_state:
                    action = get_keyboard_action(key_state)
                    actions = [action]
                elif args.random:
                    actions = [random_agent.act() for _ in range(agents_num)]
                else:
                    # 使用追踪器
                    obj_poses = env.unwrapped.obj_poses
                    if agents_num > 1:
                        actions = [tracker.act(obj_poses[0], obj_poses[1])]
                    else:
                        actions = [random_agent.act()]
                
                # 执行动作
                obs, rewards, done, info = env.step(actions)
                
                # 统计
                step_count += 1
                if isinstance(rewards, (list, np.ndarray)):
                    episode_reward += rewards[0] if len(rewards) > 0 else 0
                else:
                    episode_reward += rewards
                
                # 显示画面
                if args.render:
                    if isinstance(obs, list) and len(obs) > 0:
                        display_img = obs[0]
                    else:
                        display_img = obs
                    cv2.imshow('Arctic Environment', display_img)
                    key = cv2.waitKey(1) & 0xFF
                    if key == ord('q'):
                        print("\n👋 用户退出")
                        raise KeyboardInterrupt
                
                if done:
                    fps = step_count / (time.time() - t0)
                    total_rewards += episode_reward
                    print(f"   ✅ 完成! Steps: {step_count}, FPS: {fps:.1f}, Reward: {episode_reward:.2f}")
                    break
                
                # 限制最大步数
                if step_count >= args.max_steps:
                    fps = step_count / (time.time() - t0)
                    print(f"   ⏱️ 达到最大步数. Steps: {step_count}, FPS: {fps:.1f}")
                    break
        
        print("\n" + "=" * 50)
        print(f"🎉 测试完成!")
        print(f"   总 Episodes: {episode_count}")
        print(f"   平均奖励: {total_rewards / episode_count:.2f}")
        
    except KeyboardInterrupt:
        print("\n⚠️ 测试被中断")
    finally:
        env.close()
        cv2.destroyAllWindows()
        print("✅ 环境已关闭")


def main():
    parser = argparse.ArgumentParser(description='Arctic 地图测试脚本')
    
    # 环境设置
    parser.add_argument('-e', '--env_id', type=str, 
                        default='UnrealTrack-Grass_Hills-ContinuousColor-v0',
                        help='环境 ID (默认: UnrealTrack-Grass_Hills-ContinuousColor-v0)')
    parser.add_argument('-s', '--seed', type=int, default=42, help='随机种子')
    parser.add_argument('--resolution', type=int, default=240, help='图像分辨率')
    parser.add_argument('--offscreen', action='store_true', help='离屏渲染模式')
    
    # 智能体设置
    parser.add_argument('--min_agents', type=int, default=2, help='最小智能体数量')
    parser.add_argument('--max_agents', type=int, default=4, help='最大智能体数量')
    parser.add_argument('--keyboard', action='store_true', help='启用键盘控制')
    parser.add_argument('--random', action='store_true', help='使用随机动作')
    
    # 仿真设置
    parser.add_argument('-t', '--time_dilation', type=int, default=10, 
                        help='时间膨胀因子 (控制仿真速度, -1 禁用)')
    parser.add_argument('--episodes', type=int, default=3, help='测试 episode 数量')
    parser.add_argument('--max_steps', type=int, default=500, help='每个 episode 最大步数')
    parser.add_argument('-r', '--render', action='store_true', default=True, 
                        help='显示渲染画面')
    parser.add_argument('--no-render', dest='render', action='store_false',
                        help='不显示渲染画面')
    parser.add_argument('--wait_time', type=int, default=5,
                        help='等待 UE 启动的时间（秒）')
    
    args = parser.parse_args()
    
    print("\n" + "=" * 50)
    print("🏔️  Arctic 环境测试脚本")
    print("=" * 50)
    
    # 检查环境变量
    unreal_env = os.environ.get('UnrealEnv')
    if unreal_env:
        print(f"✅ UnrealEnv: {unreal_env}")
    else:
        print("⚠️ UnrealEnv 环境变量未设置")
        print("   请设置: export UnrealEnv=/path/to/your/UnrealEnv")
    
    test_arctic(args)


if __name__ == '__main__':
    main()
