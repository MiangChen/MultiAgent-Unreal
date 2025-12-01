# UnrealZoo 命令参考

## 1. 启动环境

### 交互式控制 Demo
```bash
python example/drone_demo.py -e UnrealTrack-Old_Town-ContinuousColor-v0 -r
```

### 点击导航 Demo
```bash
python example/click_nav_demo.py -e UnrealTrack-Old_Town-ContinuousColor-v0 -c 0
```
- `-c 0`: 使用俯视相机
- `-c 1`: 使用 Player 相机
- `-c 3`: 使用 Drone 相机

---

## 2. 交互式命令 (drone_demo.py)

启动后在终端输入命令：

### 查询
```
> list                          # 列出所有 agent
> pos BP_Character_C_1          # 查询位置
```

### 导航 (Character/Animal)
```
> nav BP_Character_C_1 0 500 0      # 相对导航 (Y+500, 约5米)
> goto BP_Character_C_1 1200 20000 26800  # 绝对位置导航
> nav BP_animal_C_1 100 0 0         # 马导航
```

### 移动控制
```
> move BP_Character_C_1 30 100      # 人: [角速度, 线速度]
> move BP_Drone01_C_1 0 0 1 0       # 无人机: [vx, vy, vz, vyaw]
> stop BP_Character_C_1             # 停止
```

### 无人机
```
> fly BP_Drone01_C_1 0 0 200        # 飞到相对位置 (上升200)
```

### 外观
```
> app Dog_0 3                       # 设置外观 ID
> app BP_animal_C_1 5               # 马换成猫 (ID=5)
```

### 退出
```
> quit
```

---

## 3. 点击导航 (click_nav_demo.py)

| 操作 | 说明 |
|------|------|
| 左键点击 | 导航到点击位置 |
| 按 1-9 | 切换相机 |
| 按 ESC | 退出 |

---

## 4. Agent 类型与移动方式

| 类型 | 蓝图类 | 移动方式 | 命令 |
|------|--------|----------|------|
| Character (人) | bp_character_C | NavMesh | `nav`, `goto` |
| Animal (马/狗) | BP_animal_C | NavMesh | `nav`, `goto` |
| Drone (无人机) | BP_drone01_C | 直接控制 | `fly`, `move` |

---

## 5. 外观 ID 参考 (BP_animal_C)

| ID | 名称 |
|----|------|
| 0 | Dog (默认) |
| 1 | Dog Variant 1 |
| 2 | Dog Variant 2 |
| 3 | Wolf |
| 4 | Fox |
| 5 | Cat |
| 6 | Horse |
| 7 | Zebra |

完整列表见 `asset/Unreal_animal_appearances.json`

---

## 6. 配置文件

主要配置: `gym_unrealcv/envs/setting/Track/Old_Town_drone_quadruped.json`

包含:
- player (BP_Character_C_1)
- animal (BP_animal_C_1) 
- drone (BP_Drone01_C_1)
