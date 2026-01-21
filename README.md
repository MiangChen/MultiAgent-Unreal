# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，支持 UAV、UGV、四足机器人、人形机器人等异构智能体的协同仿真。通过 HTTP 接口与外部规划器（Python 端）交互，实现自然语言指令到机器人动作的转换。

## 特性

- 多种机器人类型：多旋翼无人机、固定翼无人机、无人车、四足机器人、人形机器人
- 技能系统：Navigate、Search、Follow、Place、Charge、TakeOff、Land 等
- 三层架构：指令调度层 → 技能管理层 → 技能内核层
- HTTP 通信接口：与 Python 端规划器实时交互
- 能量系统：电量管理、自动充电

## 环境要求

- Unreal Engine 5.5～5.7（源码编译版本）
- Linux / Windows / Mac
- Python 3.8+（用于外部规划器）

## 快速开始

### 1. 克隆代码

```bash
git clone https://github.com/WindyLab/MultiAgent-Unreal.git
cd MultiAgent-Unreal
```

### 2. 获取 Content 资产

Content 资产存储在 Hugging Face，使用 Git LFS 管理。

```bash
# 安装 Git LFS（如未安装）
git lfs install

# 克隆 Content 到项目目录
cd unreal_project
git clone https://huggingface.co/datasets/WindyLab/MultiAgent-Content Content
```

**中国用户使用镜像：**

```bash
git clone https://hf-mirror.com/datasets/WindyLab/MultiAgent-Content Content
```

**更新 Content：**

```bash
cd unreal_project/Content
git pull
```

**提交修改：**

```bash
cd unreal_project/Content
git add .
git commit -m "描述"
git push
```

### 3. 配置脚本

复制示例脚本并修改路径：

```bash
cp example_compile.sh compile.sh
cp example_start_mac.sh start.sh
chmod +x compile.sh start.sh
```

编辑 `compile.sh` 和 `start.sh`，修改 `UE_ROOT` / `UE5_ROOT` 为你的 UE5 安装路径。

### 4. 编译

```bash
# 默认编译 (Development)
./compile.sh

# Debug 配置
./compile.sh -c Debug

# 完全重新编译
./compile.sh -r

# 查看帮助
./compile.sh -h
```

### 5. 运行

```bash
./start.sh
```

## 机器人类型

| 类型 | 类名 | 可用技能 |
|------|------|----------|
| UAV | MAUAVCharacter | Navigate, Search, Follow, TakeOff, Land, Charge |
| FixedWingUAV | MAFixedWingUAVCharacter | Navigate, Search, TakeOff, Land, Charge |
| UGV | MAUGVCharacter | Navigate, Carry, Charge |
| Quadruped | MAQuadrupedCharacter | Navigate, Search, Follow, Charge |
| Humanoid | MAHumanoidCharacter | Navigate, Place, Charge |

## 项目结构

```
MultiAgent-Unreal/
├── config/                     # 配置文件
│   ├── simulation.json         # 仿真参数、服务器地址
│   ├── agents.json             # 机器人配置
│   └── environment.json        # 环境配置（充电站等）
├── scripts/                    # Python 脚本
│   ├── send_skill_list.py      # 发送技能列表
│   └── world_query.py          # 世界状态查询
├── unreal_project/             # UE5 项目
│   ├── Source/MultiAgent/      # C++ 源码
│   │   ├── Core/               # 核心子系统
│   │   ├── Agent/              # 机器人、技能
│   │   ├── Environment/        # 环境实体
│   │   └── UI/                 # 界面
│   └── Content/                # 资产（Hugging Face 独立仓库）
├── doc/                        # 文档
├── compile.sh                  # 编译脚本
└── start.sh                    # 启动脚本
```

## 文档

| 文档 | 说明 |
|------|------|
| [doc/ARCHITECTURE.md](doc/ARCHITECTURE.md) | 系统架构、三层设计、数据流 |
| [doc/KEYBINDINGS.md](doc/KEYBINDINGS.md) | 按键说明 |
| [doc/guide/NEW_FEATURE_GUIDE.md](doc/guide/NEW_FEATURE_GUIDE.md) | 新功能开发指南 |
| [doc/README.md](doc/README.md) | 文档索引 |

## 通信接口

UE5 端通过 HTTP 轮询与 Python 端通信，支持以下消息类型：

| 方向 | 类型 | 说明 |
|------|------|------|
| 入站 | SkillList | 技能列表（按时间步组织） |
| 入站 | QueryRequest | 世界状态查询 |
| 出站 | TaskFeedback | 任务执行反馈 |
| 出站 | WorldState | 世界状态响应 |

技能列表示例：
```json
{
    "0": {
        "UAV_0": { "skill": "navigate", "params": { "dest_position": [1000, 2000, 500] } }
    },
    "1": {
        "UAV_0": { "skill": "search", "params": { "search_area": [[0,0], [1000,0], [1000,1000], [0,1000]] } }
    }
}
```

## 启用的 UE5 插件

- GameplayAbilities
- StateTree
- MassEntity / MassGameplay / MassAI

## License

MIT License
