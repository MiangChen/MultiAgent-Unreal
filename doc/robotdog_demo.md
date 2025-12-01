# RobotDog Demo 使用说明

## 启动

```bash
python example/robotdog_demo.py -e UnrealTrack-Old_Town-ContinuousColor-v0
```

---

## 命令参考

### 移动控制

| 命令 | 说明 | 示例 |
|------|------|------|
| `list` | 列出所有 agent | `list` |
| `pos <name>` | 查询位置 | `pos BP_Character_C_1` |
| `nav <name> <dx> <dy> <dz>` | 相对导航 | `nav BP_Character_C_1 0 500 0` |
| `goto <name> <x> <y> <z>` | 绝对位置导航 | `goto BP_Character_C_1 1200 20000 26800` |

### 外观切换

| 命令 | 说明 | 示例 |
|------|------|------|
| `dog <id>` | 切换动物外观 | `dog 0` |
| `dogs` | 显示所有外观选项 | `dogs` |
| `app <name> <id>` | 设置指定对象外观 | `app BP_animal_C_1 3` |
| `spawn <id>` | 生成新动物 | `spawn 1` |

**动物外观 ID:**
```
0: Dog (默认)      5: Cat         10: Cow
1: Dog Variant 1   6: Horse       11: Elephant
2: Dog Variant 2   7: Zebra       12: Rhino
3: Wolf            8: Donkey      13: Hippo
4: Fox             9: Deer        14-16: Bear/Lion/Tiger
```

### 物体交互

| 命令 | 说明 | 示例 |
|------|------|------|
| `spawnobj <type>` | 生成可交互物体 | `spawnobj pickup` |
| `carry <player>` | 拾取/搬运物体 | `carry BP_Character_C_1` |
| `drop <player>` | 放下物体 | `drop BP_Character_C_1` |
| `iscarry <player>` | 检查是否在搬运 | `iscarry BP_Character_C_1` |
| `door <player> <0\|1>` | 开/关门 | `door BP_Character_C_1 1` |

**可交互物体类型:**
- `pickup` - BP_PickUpObject
- `grab` - BP_GrabbableObject
- `interact` - BP_InteractableObject
- `usable` - BP_UsableObject

### 其他

| 命令 | 说明 |
|------|------|
| `robotdogs` | 显示机器狗模型列表 (无蓝图) |
| `help` | 显示帮助 |
| `quit` | 退出程序 |

---

## 使用示例

### 1. 基本导航
```
> list                              # 查看所有 agent
> nav BP_Character_C_1 0 500 0      # 人物向 Y 方向移动 5 米
> nav BP_animal_C_1 100 0 0         # 动物向 X 方向移动 1 米
```

### 2. 切换动物外观
```
> dogs                              # 查看所有外观
> dog 0                             # 切换为狗
> dog 3                             # 切换为狼
> dog 6                             # 切换为马
```

### 3. 生成多个动物
```
> spawn 0                           # 生成一只狗
> spawn 3                           # 生成一只狼
> spawn 5                           # 生成一只猫
```

### 4. 物体交互
```
> spawnobj pickup                   # 在人物前方生成可拾取物体
> nav BP_Character_C_1 100 0 0      # 人物走向物体
> carry BP_Character_C_1            # 拾取物体
> iscarry BP_Character_C_1          # 检查是否拾取成功
> drop BP_Character_C_1             # 放下物体
```

### 5. 开关门
```
> door BP_Character_C_1 1           # 开门
> door BP_Character_C_1 0           # 关门
```

---

## 注意事项

1. **导航**: 使用 NavMesh，只有 Character 和 Animal 支持，Drone 不支持
2. **拾取**: 需要人物靠近物体才能拾取
3. **机器狗模型**: `SK_RobotDog*` 存在于资产中，但没有对应蓝图，无法直接使用
4. **外观 ID**: 0-4 为狗类，推荐用于机器狗仿真

---

## 配置文件

`gym_unrealcv/envs/setting/Track/Old_Town_robotdog.json`

包含:
- `player` (BP_Character_C_1) - 人类角色
- `robotdog` (BP_animal_C_1) - 动物/机器狗
