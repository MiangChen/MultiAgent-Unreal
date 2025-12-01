"""
UnrealZoo Demo - 加载 agent + 交互式命令

移动方式说明:
- Character/Animal: 使用 nav_to_goal (NavMesh 导航)
- Drone: 使用 set_move (直接速度控制) [vx, vy, vz, vyaw]
"""
import argparse
import gym_unrealcv
import gym
import cv2
import time
from gym_unrealcv.envs.wrappers import time_dilation, configUE
from gym_unrealcv.envs.utils import misc


class CommandHandler:
    """处理用户输入的命令"""
    
    def __init__(self, unrealcv, player_list, agents):
        self.unrealcv = unrealcv
        self.player_list = player_list
        self.agents = agents
        self.running = True
    
    def print_help(self):
        print("\n可用命令:")
        print("  list                        - 列出所有 agent")
        print("  pos <name>                  - 查询位置")
        print("  nav <name> <dx> <dy> <dz>   - NavMesh导航 (Character/Animal)")
        print("  goto <name> <x> <y> <z>     - 绝对位置导航 (Character/Animal)")
        print("  move <name> <p1> <p2> ...   - 直接移动控制")
        print("       Character/Animal: move <name> <角速度> <线速度>")
        print("       Drone: move <name> <vx> <vy> <vz> <vyaw>")
        print("  stop <name>                 - 停止移动")
        print("  fly <name> <dx> <dy> <dz>   - 无人机飞行到相对位置")
        print("  app <name> <id>             - 设置外观")
        print("  help                        - 显示帮助")
        print("  quit                        - 退出")
    
    def get_agent_type(self, name):
        """获取 agent 类型"""
        if name in self.agents:
            return self.agents[name]['agent_type']
        # 动态添加的对象
        if 'Dog' in name or 'animal' in name.lower():
            return 'animal'
        if 'Drone' in name or 'drone' in name.lower():
            return 'drone'
        return 'unknown'
    
    def handle_command(self, cmd):
        parts = cmd.strip().split()
        if not parts:
            return
        
        action = parts[0].lower()
        
        if action == 'help':
            self.print_help()
        
        elif action == 'quit' or action == 'exit':
            self.running = False
        
        elif action == 'list':
            print("\nAgent 列表:")
            for p in self.player_list:
                agent_type = self.agents[p]['agent_type']
                loc = self.unrealcv.get_obj_location(p)
                print(f"  {p} ({agent_type}): {loc}")
        
        elif action == 'pos':
            if len(parts) < 2:
                print("用法: pos <name>")
                return
            name = parts[1]
            loc = self.unrealcv.get_obj_location(name)
            print(f"{name}: {loc}")
        
        elif action == 'nav':
            # NavMesh 导航 - 适用于 Character 和 Animal
            if len(parts) < 5:
                print("用法: nav <name> <dx> <dy> <dz>")
                return
            name = parts[1]
            dx, dy, dz = float(parts[2]), float(parts[3]), float(parts[4])
            loc = self.unrealcv.get_obj_location(name)
            goal = [loc[0] + dx, loc[1] + dy, loc[2] + dz]
            
            agent_type = self.get_agent_type(name)
            if agent_type == 'drone':
                print(f"警告: Drone 不支持 NavMesh，请使用 'fly' 或 'move' 命令")
                return
            
            print(f"导航 {name} ({agent_type}): {loc} -> {goal}")
            self.unrealcv.nav_to_goal(name, goal)
        
        elif action == 'goto':
            if len(parts) < 5:
                print("用法: goto <name> <x> <y> <z>")
                return
            name = parts[1]
            x, y, z = float(parts[2]), float(parts[3]), float(parts[4])
            
            agent_type = self.get_agent_type(name)
            if agent_type == 'drone':
                print(f"警告: Drone 不支持 NavMesh，请使用 'fly' 或 'move' 命令")
                return
            
            print(f"导航 {name} -> [{x}, {y}, {z}]")
            self.unrealcv.nav_to_goal(name, [x, y, z])
        
        elif action == 'move':
            # 直接移动控制
            if len(parts) < 3:
                print("用法: move <name> <params...>")
                print("  Character/Animal: move <name> <角速度> <线速度>")
                print("  Drone: move <name> <vx> <vy> <vz> <vyaw>")
                return
            name = parts[1]
            params = [float(p) for p in parts[2:]]
            
            agent_type = self.get_agent_type(name)
            print(f"移动 {name} ({agent_type}): params={params}")
            self.unrealcv.set_move_bp(name, params)
        
        elif action == 'stop':
            # 停止移动
            if len(parts) < 2:
                print("用法: stop <name>")
                return
            name = parts[1]
            agent_type = self.get_agent_type(name)
            
            if agent_type == 'drone':
                self.unrealcv.set_move_bp(name, [0, 0, 0, 0])
            else:
                self.unrealcv.set_move_bp(name, [0, 0])
            print(f"停止 {name}")
        
        elif action == 'fly':
            # 无人机飞行到相对位置
            if len(parts) < 5:
                print("用法: fly <name> <dx> <dy> <dz>")
                return
            name = parts[1]
            dx, dy, dz = float(parts[2]), float(parts[3]), float(parts[4])
            
            agent_type = self.get_agent_type(name)
            if agent_type != 'drone':
                print(f"警告: {name} 不是 Drone，请使用 'nav' 命令")
                return
            
            loc = self.unrealcv.get_obj_location(name)
            goal = [loc[0] + dx, loc[1] + dy, loc[2] + dz]
            print(f"飞行 {name}: {loc} -> {goal}")
            
            # 直接设置位置（简单实现）
            self.unrealcv.set_obj_location(name, goal)
        
        elif action == 'app':
            if len(parts) < 3:
                print("用法: app <name> <id>")
                return
            name = parts[1]
            app_id = int(parts[2])
            print(f"设置 {name} 外观为 {app_id}")
            self.unrealcv.set_appearance(name, app_id)
        
        else:
            print(f"未知命令: {action}")
            self.print_help()


def main():
    parser = argparse.ArgumentParser(description='UnrealZoo Drone Demo')
    parser.add_argument("-e", "--env_id", default='UnrealTrack-Old_Town-ContinuousColor-v0')
    parser.add_argument("-r", '--render', action='store_true')
    parser.add_argument("-s", '--seed', default=0)
    parser.add_argument("-t", '--time_dilation', default=10)
    args = parser.parse_args()
    
    print("=" * 60)
    print("UnrealZoo Demo - 交互式控制")
    print("=" * 60)
    
    # 创建环境
    env = gym.make(args.env_id)
    
    # 加载配置
    drone_setting = misc.load_env_setting('Track/Old_Town_drone_quadruped.json')
    env.unwrapped.agent_configs = drone_setting['agents']
    env.unwrapped.agents = misc.convert_dict(drone_setting['agents'])
    env.unwrapped.player_list = list(env.unwrapped.agents.keys())
    env.unwrapped.cam_list = [env.unwrapped.agents[p]['cam_id'] for p in env.unwrapped.player_list]
    
    env.unwrapped.action_space = [
        env.unwrapped.define_action_space(env.unwrapped.action_type, env.unwrapped.agents[obj]) 
        for obj in env.unwrapped.player_list
    ]
    env.unwrapped.observation_space = [
        env.unwrapped.define_observation_space(env.unwrapped.cam_list[i], env.unwrapped.observation_type, env.unwrapped.resolution)
        for i in range(len(env.unwrapped.player_list))
    ]
    
    env.unwrapped.agents_category = ['player', 'animal', 'drone']
    
    env = configUE.ConfigUEWrapper(env, offscreen=False, resolution=(240, 240))
    if int(args.time_dilation) > 0:
        env = time_dilation.TimeDilationWrapper(env, args.time_dilation)
    
    env.seed(int(args.seed))
    
    print("\n启动环境...")
    obs = env.reset()
    
    unrealcv = env.unwrapped.unrealcv
    base_pos = [1045.767, 19755.0, 26802.335]
    
    # 设置位置
    print("\n设置 agent 位置:")
    for player in env.unwrapped.player_list:
        agent_type = env.unwrapped.agents[player]['agent_type']
        
        if agent_type == 'animal':
            pos = base_pos.copy()
        elif agent_type == 'drone':
            pos = base_pos.copy()
            pos[2] += 500
        elif agent_type == 'player':
            pos = base_pos.copy()
            pos[0] += 100
        else:
            pos = base_pos.copy()
        
        unrealcv.set_obj_location(player, pos)
        print(f"  {player} ({agent_type}) -> {pos}")
    
    # 添加 3 只狗
    print("\n添加 3 只狗:")
    dog_config = {"class_name": "BP_animal_C", "scale": [1, 1, 1]}
    
    for i, app_id in enumerate([0, 1, 2]):
        dog_name = f"Dog_{app_id}"
        dog_pos = base_pos.copy()
        dog_pos[0] += 200 + (i * 100)
        
        unrealcv.new_obj(dog_config['class_name'], dog_name, dog_pos)
        unrealcv.set_appearance(dog_name, app_id)
        unrealcv.set_obj_scale(dog_name, dog_config['scale'])
        print(f"  {dog_name} (appearance={app_id}) -> {dog_pos}")
    
    # 创建命令处理器
    handler = CommandHandler(unrealcv, env.unwrapped.player_list, env.unwrapped.agents)
    handler.print_help()
    
    print("\n" + "=" * 60)
    print("移动方式说明:")
    print("  - Character/Animal: nav/goto (NavMesh自动寻路)")
    print("  - Drone: fly (直接设置位置) 或 move (速度控制)")
    print("=" * 60)
    
    print("\n输入命令控制 agent:")
    
    try:
        while handler.running:
            try:
                cmd = input("\n> ")
                handler.handle_command(cmd)
            except EOFError:
                break
            
    except KeyboardInterrupt:
        print("\n退出...")
    finally:
        env.close()
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
