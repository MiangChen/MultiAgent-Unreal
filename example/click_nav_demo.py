"""
点击导航 Demo - 实时同步 UE 相机视图，点击位置导航

功能:
1. 实时从 UE 获取相机图像并显示
2. 点击图像任意位置，人物导航到该位置
3. 支持切换相机视角
"""
import argparse
import gym_unrealcv
import gym
import cv2
import numpy as np
import threading
import time
from gym_unrealcv.envs.wrappers import time_dilation, configUE
from gym_unrealcv.envs.utils import misc


class RealtimeClickNavController:
    def __init__(self, unrealcv, player_name, cam_id=0):
        self.unrealcv = unrealcv
        self.player_name = player_name
        self.cam_id = cam_id
        self.window_name = "Realtime View - Click to Navigate"
        self.img = None
        self.running = True
        self.click_pos = None  # 最近点击的位置
        
        self.resolution = (640, 480)
        self.fov = 90
        self.ground_height = 26800  # 地面高度估计
        
        # 缓存相机参数
        self.cam_loc = None
        self.cam_rot = None
        
    def update_camera_params(self):
        """更新相机参数"""
        self.cam_loc = self.unrealcv.get_cam_location(self.cam_id)
        self.cam_rot = self.unrealcv.get_cam_rotation(self.cam_id)
        
    def get_image(self):
        """获取当前相机图像"""
        img = self.unrealcv.get_image(self.cam_id, 'lit', 'bmp')
        if img is not None:
            img = cv2.resize(img, self.resolution)
        return img
    
    def pixel_to_world_topview(self, px, py):
        """
        俯视相机: 像素坐标转世界坐标
        假设相机朝下 (pitch=-90)
        """
        if self.cam_loc is None:
            self.update_camera_params()
        
        cam_x, cam_y, cam_z = self.cam_loc
        
        # 计算视野范围
        view_height = cam_z - self.ground_height
        if view_height <= 0:
            view_height = 1000  # 默认值
        
        half_fov_rad = np.radians(self.fov / 2)
        view_range = 2 * view_height * np.tan(half_fov_rad)
        
        # 像素归一化
        nx = (px / self.resolution[0]) - 0.5
        ny = (py / self.resolution[1]) - 0.5
        
        # 转换 (俯视图)
        world_x = cam_x - ny * view_range
        world_y = cam_y + nx * view_range
        world_z = self.ground_height
        
        return [world_x, world_y, world_z]
    
    def pixel_to_world_perspective(self, px, py):
        """
        透视相机: 使用深度图计算世界坐标
        """
        if self.cam_loc is None:
            self.update_camera_params()
        
        # 获取深度
        depth_img = self.unrealcv.get_image(self.cam_id, 'depth', 'npy')
        if depth_img is None:
            print("无法获取深度图，使用估计值")
            return self.pixel_to_world_topview(px, py)
        
        # 调整深度图大小
        depth_img = cv2.resize(depth_img, self.resolution)
        
        # 获取点击位置的深度
        depth = depth_img[int(py), int(px)]
        if depth <= 0 or depth > 10000:
            depth = 500  # 默认深度
        
        # 相机内参
        fx = self.resolution[0] / (2 * np.tan(np.radians(self.fov / 2)))
        fy = fx
        cx, cy = self.resolution[0] / 2, self.resolution[1] / 2
        
        # 像素到相机坐标
        x_cam = (px - cx) * depth / fx
        y_cam = (py - cy) * depth / fy
        z_cam = depth
        
        # 相机坐标到世界坐标 (简化，假设相机朝向)
        cam_x, cam_y, cam_z = self.cam_loc
        pitch, yaw, roll = self.cam_rot if self.cam_rot else (0, 0, 0)
        
        # 简单旋转 (只考虑 yaw)
        yaw_rad = np.radians(yaw)
        world_x = cam_x + z_cam * np.cos(yaw_rad) - x_cam * np.sin(yaw_rad)
        world_y = cam_y + z_cam * np.sin(yaw_rad) + x_cam * np.cos(yaw_rad)
        world_z = cam_z - y_cam
        
        return [world_x, world_y, world_z]
    
    def on_mouse_click(self, event, x, y, flags, param):
        """鼠标点击回调"""
        if event == cv2.EVENT_LBUTTONDOWN:
            self.click_pos = (x, y)
            
            # 更新相机参数
            self.update_camera_params()
            
            # 判断是否为俯视相机
            pitch = self.cam_rot[0] if self.cam_rot else 0
            if pitch < -60:  # 俯视
                world_pos = self.pixel_to_world_topview(x, y)
            else:  # 透视
                world_pos = self.pixel_to_world_perspective(x, y)
            
            print(f"\n点击: ({x}, {y})")
            print(f"相机位置: {self.cam_loc}")
            print(f"相机旋转: {self.cam_rot}")
            print(f"世界坐标: [{world_pos[0]:.1f}, {world_pos[1]:.1f}, {world_pos[2]:.1f}]")
            
            # 导航
            print(f"导航 {self.player_name} -> 目标位置")
            self.unrealcv.nav_to_goal(self.player_name, world_pos)
    
    def image_update_loop(self):
        """图像更新线程"""
        while self.running:
            try:
                img = self.get_image()
                if img is not None:
                    self.img = img.copy()
                    
                    # 标记点击位置
                    if self.click_pos:
                        cv2.circle(self.img, self.click_pos, 10, (0, 255, 0), 2)
                    
                    # 显示相机信息
                    self.update_camera_params()
                    if self.cam_loc and self.cam_rot:
                        info = f"Cam{self.cam_id} Pos:[{self.cam_loc[0]:.0f},{self.cam_loc[1]:.0f},{self.cam_loc[2]:.0f}]"
                        cv2.putText(self.img, info, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
                        info2 = f"Rot:[{self.cam_rot[0]:.0f},{self.cam_rot[1]:.0f},{self.cam_rot[2]:.0f}]"
                        cv2.putText(self.img, info2, (10, 45), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
                    
                time.sleep(0.1)  # 10 FPS
            except Exception as e:
                print(f"图像更新错误: {e}")
                time.sleep(0.5)
    
    def run(self):
        """运行"""
        cv2.namedWindow(self.window_name)
        cv2.setMouseCallback(self.window_name, self.on_mouse_click)
        
        print("\n" + "=" * 50)
        print("实时点击导航")
        print("  - 左键点击: 导航到该位置")
        print("  - 按 1-9: 切换相机")
        print("  - 按 ESC: 退出")
        print("=" * 50)
        
        # 启动图像更新线程
        update_thread = threading.Thread(target=self.image_update_loop, daemon=True)
        update_thread.start()
        
        while self.running:
            if self.img is not None:
                cv2.imshow(self.window_name, self.img)
            
            key = cv2.waitKey(50)
            
            if key == 27:  # ESC
                self.running = False
                break
            elif ord('1') <= key <= ord('9'):
                # 切换相机
                new_cam = key - ord('0')
                print(f"\n切换到相机 {new_cam}")
                self.cam_id = new_cam
                self.click_pos = None
                self.update_camera_params()
        
        cv2.destroyWindow(self.window_name)


def main():
    parser = argparse.ArgumentParser(description='Realtime Click Navigation Demo')
    parser.add_argument("-e", "--env_id", default='UnrealTrack-Old_Town-ContinuousColor-v0')
    parser.add_argument("-t", '--time_dilation', default=10)
    parser.add_argument("-c", '--cam', type=int, default=0, help='初始相机ID')
    args = parser.parse_args()
    
    print("=" * 60)
    print("实时点击导航 Demo")
    print("=" * 60)
    
    # 创建环境
    env = gym.make(args.env_id)
    
    # 加载配置 (人、马、无人机)
    setting = misc.load_env_setting('Track/Old_Town_drone_quadruped.json')
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
    
    env.unwrapped.agents_category = ['player', 'animal', 'drone']
    
    env = configUE.ConfigUEWrapper(env, offscreen=False, resolution=(240, 240))
    if int(args.time_dilation) > 0:
        env = time_dilation.TimeDilationWrapper(env, args.time_dilation)
    
    print("\n启动环境...")
    obs = env.reset()
    
    unrealcv = env.unwrapped.unrealcv
    
    # 找到 player
    player_name = None
    for p in env.unwrapped.player_list:
        if env.unwrapped.agents[p]['agent_type'] == 'player':
            player_name = p
            break
    
    if not player_name:
        print("未找到 player!")
        env.close()
        return
    
    print(f"控制角色: {player_name}")
    print(f"初始相机: {args.cam}")
    
    # 创建控制器
    controller = RealtimeClickNavController(unrealcv, player_name, cam_id=args.cam)
    
    try:
        controller.run()
    except KeyboardInterrupt:
        pass
    finally:
        controller.running = False
        env.close()
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
