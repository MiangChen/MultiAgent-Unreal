# MultiAgent-Unreal

基于 Unreal Engine 5 的多智能体仿真框架，支持 UAV、UGV、四足机器人、人形机器人等异构智能体的协同仿真。通过 HTTP 接口与外部规划器（Python 端）交互，实现自然语言指令到机器人动作的转换。

## 特性

- 多种机器人类型：多旋翼无人机、固定翼无人机、无人车、四足机器人、人形机器人
- 技能系统：Navigate、Search、Follow、Place、Charge、TakeOff、Land 等
- 三层架构：指令调度层 → 技能管理层 → 技能内核层
- HTTP 通信接口：与 Python 端规划器实时交互
- 能量系统：电量管理、自动充电

## 环境要求

- Unreal Engine 5.5（源码编译版本）
- Linux / Windows
- Python 3.8+（用于外部规划器）

## 快速开始

### 1. 克隆代码

```bash
git clone https://github.com/WindyLab/MultiAgent-Unreal.git
cd MultiAgent-Unreal
```

### 2. 下载资产

项目资产（约 5GB）托管在 HuggingFace，需要单独下载。UE5 要求 Content 目录与 `.uproject` 文件在同一级目录。

#### 方法 A：直接放在项目内（简单）

```bash
cd MultiAgent-Unreal

# 安装 huggingface_hub
pip install huggingface_hub

# 下载压缩包到项目目录
huggingface-cli download WindyLab/MultiAgent-Content MultiAgent_Content.tar.gz \
    --repo-type dataset --local-dir .

# 解压到 unreal_project 目录
tar -xzvf MultiAgent_Content.tar.gz -C unreal_project

# 重命名为 Content
mv unreal_project/MultiAgent_Content unreal_project/Content

# 清理压缩包（可选）
rm MultiAgent_Content.tar.gz
```

#### 方法 B：放在项目外 + 软链接（推荐）

将资产放在项目外可以避免 Git 跟踪大文件，多个项目也可以共享资产。

```bash
# 1. 创建资产存放目录（示例路径，可自定义）
mkdir -p ~/UE5_Assets

# 2. 下载压缩包
huggingface-cli download WindyLab/MultiAgent-Content MultiAgent_Content.tar.gz \
    --repo-type dataset --local-dir ~/UE5_Assets

# 3. 解压
cd ~/UE5_Assets
tar -xzvf MultiAgent_Content.tar.gz

# 4. 在项目中创建软链接
cd /path/to/MultiAgent-Unreal
ln -s ~/UE5_Assets/MultiAgent_Content unreal_project/Content

# 5. 验证软链接
ls -la unreal_project/Content  # 应显示指向实际目录的链接
```

#### Windows 用户

Windows 不支持 `ln -s`，使用以下方式：

```powershell
# 方法 1：管理员权限下创建符号链接
mklink /D unreal_project\Content C:\UE5_Assets\MultiAgent_Content

# 方法 2：直接复制（不推荐，占用空间）
xcopy /E /I C:\UE5_Assets\MultiAgent_Content unreal_project\Content
```

#### 目录结构验证

正确配置后，目录结构应为：

```
unreal_project/
├── MultiAgent.uproject
├── Source/
├── Content/              # 软链接或实际目录
│   ├── Characters/
│   ├── Map/
│   ├── Robot/
│   └── ...
└── ...
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
│   └── Content/                # 资产（软链接）
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
