"""
UnrealZoo 机器狗 Demo

机器狗使用 BP_animal_C 蓝图，通过外观 ID 切换不同模型：
  0: Dog (默认)
  1: Dog Variant 1
  2: Dog Variant 2
  3: Wolf
  4: Fox
  5: Cat
  6: Horse
  ...

命令:
  list                    - 列出所有 agent
  pos <name>              - 查询位置
  nav <name> <dx> <dy> <dz>  - 导航
  app <name> <id>         - 切换外观
  dog <id>                - 快速切换动物外观
  spawnobj <type>         - 生成可交互物体 (pickup/grab/interact/usable)
  carry <player>          - 拾取物体
  drop <player>           - 放下物体
  door <player> <0|1>     - 开/关门
  quit                    - 退出
"""
import argparse
import gym_unrealcv
import gym
import cv2
import time
from gym_unrealcv.envs.wrappers import time_dilation, configUE
from gym_unrealcv.envs.utils import misc


# BP_animal_C 外观 ID 映射 (真实动物)
ANIMAL_APPEARANCES = {
    0: "Dog (Default)",
    1: "Dog Variant 1", 
    2: "Dog Variant 2",
    3: "Wolf",
    4: "Fox",
    5: "Cat",
    6: "Horse",
    7: "Zebra",
    8: "Donkey",
    9: "Deer",
    10: "Cow",
    11: "Elephant",
    12: "Rhino",
    13: "Hippo",
    14: "Bear",
    15: "Lion",
    16: "Tiger",
}

# 机器狗骨骼网格 (存在于资产中，但可能没有对应蓝图)
ROBOTDOG_MESHES = [
    "SK_RobotDog1",
    "SK_RobotDog2", 
    "SK_RobotDog3",
    "SK_RobotDog4",
    "SK_RobotDog5",
    "SK_RobotDog6",
    "SK_RobotDog7",
    "SK_RobotDog8",
    "SK_RobotDog_Ambulance",
    "SK_RobotDog_Cyber",
    "SK_RobotDog_Military",
    "SK_RobotDog_Police",
    "SK_RobotDog_Rust",
    "SK_RobotDog_NoRust",
]


class RobotDogController:
    def __init__(self, unrealcv, player_list, agents):
        self.unrealcv = unrealcv
        self.player_list = player_list
        self.agents = agents
        self.running = True
        self.robotdog_name = None
        
        # 找到机器狗
        for p in player_list:
            if agents[p]['agent_type'] == 'robotdog':
                self.robotdog_name = p
                break
    
    def print_help(self):
        print("\n可用命令:")
        print("【移动】")
        print("  list                    - 列出所有 agent")
        print("  pos <name>              - 查询位置")
        print("  nav <name> <dx> <dy> <dz>  - 导航 (NavMesh)")
        print("  goto <name> <x> <y> <z> - 绝对位置导航")
        print("【外观】")
        print("  app <name> <id>         - 设置外观")
        print("  dog <id>                - 切换动物外观 (0-16)")
        print("  dogs                    - 显示所有动物外观")
        print("  spawn <id>              - 生成新动物")
        print("【交互】")
        print("  spawnobj <type>         - 生成可交互物体")
        print("  carry <player>          - 拾取/搬运物体")
        print("  drop <player>           - 放下物体")
        print("  iscarry <player>        - 检查是否在搬运")
        print("  door <player> <0|1>     - 开/关门")
        print("【其他】")
        print("  robotdogs               - 显示机器狗模型")
        print("  help                    - 显示帮助")
        print("  quit                    - 退出")
    
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
            loc = self.unrealcv.get_obj_location(parts[1])
            print(f"{parts[1]}: {loc}")
        
        elif action == 'nav':
            if len(parts) < 5:
                print("用法: nav <name> <dx> <dy> <dz>")
                return
            name = parts[1]
            dx, dy, dz = float(parts[2]), float(parts[3]), float(parts[4])
            loc = self.unrealcv.get_obj_location(name)
            goal = [loc[0] + dx, loc[1] + dy, loc[2] + dz]
            print(f"导航 {name}: {loc} -> {goal}")
            self.unrealcv.nav_to_goal(name, goal)
        
        elif action == 'goto':
            if len(parts) < 5:
                print("用法: goto <name> <x> <y> <z>")
                return
            name = parts[1]
            x, y, z = float(parts[2]), float(parts[3]), float(parts[4])
            print(f"导航 {name} -> [{x}, {y}, {z}]")
            self.unrealcv.nav_to_goal(name, [x, y, z])
        
        elif action == 'app':
            if len(parts) < 3:
                print("用法: app <name> <id>")
                return
            name = parts[1]
            app_id = int(parts[2])
            print(f"设置 {name} 外观为 {app_id}")
            self.unrealcv.set_appearance(name, app_id)
        
        elif action == 'dog' or action == 'animal':
            if len(parts) < 2:
                print("用法: dog <id>  (0-16)")
                return
            if not self.robotdog_name:
                print("未找到 agent!")
                return
            app_id = int(parts[1])
            app_name = ANIMAL_APPEARANCES.get(app_id, f"Unknown ({app_id})")
            print(f"切换外观: {app_id} - {app_name}")
            self.unrealcv.set_appearance(self.robotdog_name, app_id)
        
        elif action == 'dogs' or action == 'animals':
            print("\nBP_animal_C 外观选项 (真实动物):")
            for id, name in ANIMAL_APPEARANCES.items():
                marker = " <-- 狗类" if id <= 4 else ""
                print(f"  {id}: {name}{marker}")
            print("\n注意: 机器狗模型 (SK_RobotDog*) 存在于资产中，")
            print("      但没有对应的蓝图，无法直接使用。")
            print("      需要在 UE 编辑器中创建 BP_RobotDog 蓝图。")
        
        elif action == 'robotdogs':
            print("\n机器狗骨骼网格 (存在但无蓝图):")
            for i, mesh in enumerate(ROBOTDOG_MESHES):
                print(f"  {i}: {mesh}")
            print("\n这些模型需要在 UE 中创建蓝图才能使用。")
        
        elif action == 'spawn':
            if len(parts) < 2:
                print("用法: spawn <appearance_id>")
                return
            app_id = int(parts[1])
            # 生成新狗
            dog_name = f"Dog_spawned_{int(time.time())}"
            base_loc = self.unrealcv.get_obj_location(self.robotdog_name) if self.robotdog_name else [1045, 19755, 26802]
            spawn_loc = [base_loc[0] + 150, base_loc[1], base_loc[2]]
            
            print(f"生成新狗: {dog_name} (外观={app_id}) @ {spawn_loc}")
            self.unrealcv.new_obj("BP_animal_C", dog_name, spawn_loc)
            self.unrealcv.set_appearance(dog_name, app_id)
            self.unrealcv.set_obj_scale(dog_name, [1, 1, 1])
        
        elif action == 'spawnobj':
            # 生成可交互物体
            # 注意: spawn 命令需要蓝图类名加 _C 后缀
            obj_types = {
                "pickup": "BP_PickUpObject_C",
                "grab": "BP_GrabbableObject_C", 
                "interact": "BP_InteractableObject_C",
                "usable": "BP_UsableObject_C",
                "crate": "BP_crate_C",
                "bag": "BP_bag_C",
                "box": "BP_Wooden_Box_2_C",
            }
            if len(parts) < 2:
                print("用法: spawnobj <type>")
                print("可用类型:", list(obj_types.keys()))
                return
            obj_type = parts[1].lower()
            if obj_type not in obj_types:
                print(f"未知类型: {obj_type}")
                print("可用类型:", list(obj_types.keys()))
                return
            
            class_name = obj_types[obj_type]
            obj_name = f"{obj_type}_{int(time.time())}"
            
            # 在 player 前方生成
            player_name = None
            for p in self.player_list:
                if self.agents[p]['agent_type'] == 'player':
                    player_name = p
                    break
            
            if player_name:
                loc = self.unrealcv.get_obj_location(player_name)
                spawn_loc = [loc[0] + 100, loc[1], loc[2]]
            else:
                spawn_loc = [1145, 19755, 26802]
            
            print(f"生成物体: {obj_name} ({class_name}) @ {spawn_loc}")
            try:
                self.unrealcv.new_obj(class_name, obj_name, spawn_loc)
                # 验证是否生成成功
                time.sleep(0.1)
                new_loc = self.unrealcv.get_obj_location(obj_name)
                if new_loc and new_loc != [0, 0, 0]:
                    print(f"成功! 物体位置: {new_loc}")
                else:
                    print("警告: 物体可能未成功生成，请检查蓝图类名是否正确")
            except Exception as e:
                print(f"失败: {e}")
        
        elif action == 'carry':
            if len(parts) < 2:
                print("用法: carry <player_name>")
                return
            player = parts[1]
            print(f"{player} 尝试拾取物体...")
            self.unrealcv.carry_body(player)
        
        elif action == 'drop':
            if len(parts) < 2:
                print("用法: drop <player_name>")
                return
            player = parts[1]
            print(f"{player} 放下物体...")
            self.unrealcv.drop_body(player)
        
        elif action == 'iscarry':
            if len(parts) < 2:
                print("用法: iscarry <player_name>")
                return
            player = parts[1]
            result = self.unrealcv.is_carrying(player)
            print(f"{player} 正在搬运: {result}")
        
        elif action == 'door':
            if len(parts) < 3:
                print("用法: door <player_name> <0|1>  (0=关, 1=开)")
                return
            player = parts[1]
            state = int(parts[2])
            print(f"{player} {'开' if state else '关'}门...")
            self.unrealcv.set_open_door(player, state)
        
        else:
            print(f"未知命令: {action}")
            self.print_help()


def main():
    parser = argparse.ArgumentParser(description='UnrealZoo RobotDog Demo')
    parser.add_argument("-e", "--env_id", default='UnrealTrack-Old_Town-ContinuousColor-v0')
    parser.add_argument("-r", '--render', action='store_true')
    parser.add_argument("-t", '--time_dilation', default=10)
    args = parser.parse_args()
    
    print("=" * 60)
    print("UnrealZoo 机器狗 Demo")
    print("=" * 60)
    
    # 创建环境
    env = gym.make(args.env_id)
    
    # 加载机器狗配置
    setting = misc.load_env_setting('Track/Old_Town_robotdog.json')
    env.unwrapped.agent_configs = setting['agents']
    env.unwrapped.agents = misc.convert_dict(setting['agents'])
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
    
    # 保留 player 和 robotdog
    env.unwrapped.agents_category = ['player', 'robotdog']
    
    env = configUE.ConfigUEWrapper(env, offscreen=False, resolution=(240, 240))
    if int(args.time_dilation) > 0:
        env = time_dilation.TimeDilationWrapper(env, args.time_dilation)
    
    print("\n启动环境...")
    obs = env.reset()
    
    unrealcv = env.unwrapped.unrealcv
    base_pos = [1045.767, 19755.0, 26802.335]
    
    # 设置位置
    print("\n设置 agent 位置:")
    for player in env.unwrapped.player_list:
        agent_type = env.unwrapped.agents[player]['agent_type']
        
        if agent_type == 'robotdog':
            pos = base_pos.copy()
        elif agent_type == 'player':
            pos = base_pos.copy()
            pos[0] += 150  # 人在狗旁边
        else:
            pos = base_pos.copy()
        
        unrealcv.set_obj_location(player, pos)
        print(f"  {player} ({agent_type}) -> {pos}")
    
    # 设置机器狗默认外观 (狗)
    robotdog_name = None
    for p in env.unwrapped.player_list:
        if env.unwrapped.agents[p]['agent_type'] == 'robotdog':
            robotdog_name = p
            unrealcv.set_appearance(p, 0)  # 默认狗外观
            print(f"\n机器狗: {robotdog_name} (外观=0: Dog)")
            break
    
    # 创建控制器
    controller = RobotDogController(unrealcv, env.unwrapped.player_list, env.unwrapped.agents)
    controller.print_help()
    
    print("\n" + "=" * 60)
    print("提示: 输入 'dog <id>' 切换机器狗外观 (0-4 推荐)")
    print("      输入 'dogs' 查看所有外观选项")
    print("=" * 60)
    
    print("\n输入命令控制机器狗:")
    
    try:
        while controller.running:
            try:
                cmd = input("\n> ")
                controller.handle_command(cmd)
            except EOFError:
                break
            
    except KeyboardInterrupt:
        print("\n退出...")
    finally:
        env.close()
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
