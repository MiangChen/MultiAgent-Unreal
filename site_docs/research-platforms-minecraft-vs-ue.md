# Minecraft 与 UE 平台科研任务场景梳理

本页整理截至 `2026-03` 仍常见于科研原型、公开 benchmark 或工程仿真的两类平台：

- `Minecraft` 平台族：更偏开放世界、长时程决策、语言与知识驱动任务。
- `Unreal Engine` 平台族：更偏高保真场景、传感器、物理、机器人控制与多智能体协同。

这不是安装手册，而是一页选型文档，回答两个问题：

- 什么研究任务更适合放在 Minecraft？
- 什么研究任务更适合放在 UE？

## 1. 一页结论

| 维度 | Minecraft 平台族 | UE 平台族 |
|---|---|---|
| 核心优势 | 开放世界探索、长时程 sparse reward、技能链、语言任务、知识驱动任务 | 高保真视觉、刚体物理、约束、传感器、多机器人、多载具、可定制场景 |
| 常见研究主题 | RL、IL、human feedback、open-ended agent、embodied planning | 自动驾驶、UAV/UGV 控制、机器人操作、多智能体协同、数字孪生、HITL |
| 典型对象语义 | `block / item / entity / inventory` | `actor / component / rigid body / constraint / sensor` |
| 物理能力 | 适合“采集-合成-放置”这类游戏规则；不擅长精细刚体操控 | 适合碰撞、抓取、搬运、装配、车辆动力学、复杂传感器仿真 |
| benchmark 标准化 | 强。MineRL、MineDojo、DiamondEnv 等任务边界清晰 | 中等。更像“引擎 + 领域模拟器”，需要自己定义任务与评测 |
| 开发成本 | 任务定义快，环境物理边界硬 | 场景和资产成本更高，但可表达能力更强 |

简化判断：

- 如果你研究的是“开放世界中的长链条行为”，优先看 Minecraft。
- 如果你研究的是“现实感物理、机器人执行、传感器和场景交互”，优先看 UE。

## 2. 先区分三个概念

这一点很重要，不然后面会把平台、benchmark、算法混在一起。

| 名称 | 类型 | 在本文中的定位 |
|---|---|---|
| `Project Malmo` | Minecraft 研究平台 | 早期基础设施，偏任务定制与 AI 实验 |
| `MineRL` | Minecraft benchmark + 数据集 + 环境 | 样本效率 RL、模仿学习、BASALT 人类反馈任务 |
| `MineDojo` | Minecraft 大规模 benchmark + 知识库 | 开放任务、语言条件、互联网知识增强 |
| `DreamerV3` | 算法 | 不是平台；它在 Minecraft 上的代表实验跑在 `DiamondEnv` |
| `DiamondEnv` | Minecraft benchmark | DreamerV3 用的“采钻石”长时程稀疏奖励环境 |
| `MineStudio` | Minecraft 工程封装 | 基于 MineRL 的新一层 simulator 封装，偏快速自定义 |
| `Unreal Engine` | 引擎 | 本身不是 benchmark，而是很多科研仿真平台的底座 |
| `CARLA` | UE 自动驾驶平台 | 感知、规划、闭环驾驶评测 |
| `AirSim` | UE 载具平台 | UAV / Car 控制、RL、感知 |
| `UnrealCV` | UE 感知平台 | 视觉、合成数据、可控相机与场景 |
| `MultiAgent-Unreal` | UE 多机器人平台 | 本仓库所对应的平台，偏异构机器人协同与 HITL |

## 3. Minecraft 平台族适合什么研究

### 3.1 Project Malmo

`Project Malmo` 的价值在于“把 Minecraft 变成一个可编程实验平台”。它支持 mission 级别的任务定义，可以：

- 生成世界和方块结构。
- 放置 `DrawBlock / DrawItem / DrawEntity`。
- 选择离散或连续动作。
- 观测图像、奖励、命令、位置等信息。

它适合：

- 经典 RL / planning / navigation 任务。
- 任务生成与课程学习。
- 多智能体或协作式 AI 原型。
- 规则明确的认知实验。

它不适合：

- 高保真机器人动力学。
- 精细抓取与刚体搬运。
- 需要真实接触建模的 manipulation。

### 3.2 MineRL

`MineRL` 是 Minecraft 研究里最经典的一条 benchmark 线，长期围绕两个方向：

- `ObtainDiamond`：样本效率 RL、模仿学习、长时程稀疏奖励。
- `BASALT`：没有明确 reward 的“近似生活化任务”，用人类反馈或演示来学。

它适合：

- Sample-efficient RL。
- Human priors / imitation learning。
- Human feedback / preference learning。
- 长链条 tech-tree 任务。

典型任务场景：

- 从零采到钻石。
- 找洞穴。
- 建村屋。
- 搭瀑布。
- 建动物围栏。

它不适合：

- 真实机器人抓取、稳定放置、关节接触控制。
- 需要精确刚体状态和连续接触力的任务。

### 3.3 MineDojo

`MineDojo` 把 Minecraft 进一步推向了 open-ended benchmark。它的特点不是单个难题，而是“任务数量大、任务形式多、观测和知识更多”。

它适合：

- Open-ended embodied agent。
- Language-conditioned planning。
- 多任务学习与持续学习。
- 视频、Wiki、Reddit 知识增强 agent。
- 从像素、inventory、voxel、位置和自然语言联合学习。

从任务结构看，它比 MineRL 更适合：

- “做很多不同任务”。
- “根据文字描述完成目标”。
- “利用外部知识缩短探索链条”。

但它仍然主要服务于：

- 探索。
- 采集。
- 战斗。
- 建造。
- 生存。

而不是 UE 意义下的精细刚体操作。

### 3.4 DreamerV3 与 DiamondEnv

`DreamerV3` 需要单独拎出来，因为它代表了 Minecraft 研究里的一个重要结论：

- 它证明了“只靠稀疏奖励、不用人类演示，也能在 Minecraft 里学到采钻石”。

但要注意：

- `DreamerV3` 是算法，不是平台。
- 它在 Minecraft 的代表实验依赖 `DiamondEnv`。

`DiamondEnv` 的任务定义很清楚：

- 12 个 milestone。
- 稀疏奖励。
- `64x64 RGB + inventory + health/hunger` 观测。
- 25 个离散动作。
- 目标是沿着 `log -> planks -> stick -> crafting_table -> ... -> diamond` 的链条走到钻石。

这说明 DreamerV3 适合的科研问题是：

- 长时程 sparse reward RL。
- 世界模型在复杂开放世界中的 credit assignment。
- 泛化到随机世界。

它不直接回答的问题是：

- 如何抓住一个桌子。
- 如何稳定搬运一个天线。
- 如何进行接触丰富的 manipulation。

### 3.5 MineStudio

如果你的目标不是参加某个经典 benchmark，而是“先快点搭一个 Minecraft 实验环境”，`MineStudio` 是值得关注的新工程栈。它基于 `MineRL` 做了一层 Gym 风格封装，并提供 callback 机制，方便：

- 自定义 reward。
- 快速 reset。
- 发送命令初始化世界。
- 录制 trajectory。
- 做 imitation 数据生成。

它更像工程加速层，不是像 MineRL / MineDojo 那样的经典 benchmark 本体。

## 4. UE 平台族适合什么研究

### 4.1 UE 原生能力

UE 的科研价值，首先不在“某一个 benchmark”，而在它本身就是一个可编程仿真引擎。

以 UE5 的 `Chaos` 物理和 `Physics Constraint` 为例，UE 天然更适合：

- 刚体物理。
- 关节与约束。
- 车辆。
- 碰撞。
- 破坏。
- 多传感器场景。

这让 UE 很适合：

- 机器人抓取与搬运。
- 物体装配。
- 复杂场景导航。
- 数字孪生。
- 多机器人协作。
- 高保真视觉和传感器数据生成。

相对 Minecraft，UE 的关键差异是：

- 研究对象通常不是“方块规则”，而是“带几何、质量、碰撞、关节和传感器的对象”。

### 4.2 CARLA

`CARLA` 是 UE 系里最标准化、最成熟的科研平台之一，聚焦自动驾驶。

它适合：

- 端到端自动驾驶。
- 传感器融合。
- 局部规划与行为规划。
- 闭环驾驶评测。
- 交通规则遵守与安全指标评估。

它的强项非常明确：

- 路线完成率。
- 碰撞和违规处罚。
- 交通参与体交互。
- 多天气、多地图、多场景。

如果你的任务主体是 `ego vehicle`，首选往往不是原生 UE，而是直接上 CARLA。

### 4.3 AirSim

`AirSim` 适合载具控制，尤其是：

- UAV 飞控与路径跟踪。
- 视觉导航。
- 目标跟随。
- 巡检。
- Car / drone 的 RL 试验。

它提供 Gym 风格 RL 包装思路，也非常适合：

- 控制策略训练。
- perception-in-the-loop。
- inspection / exploration 类 UAV 任务。

### 4.4 UnrealCV

`UnrealCV` 更偏计算机视觉研究，不是完整机器人平台。它的核心价值是：

- 用 UE 构造可控虚拟世界。
- 从外部程序控制相机和场景。
- 生成图像、深度、分割等视觉数据。

它适合：

- 合成数据集生成。
- 视觉感知算法评估。
- Domain randomization。
- 相机位姿和视角受控实验。

### 4.5 MultiAgent-Unreal

本仓库对应的平台更接近“UE 原生科研平台”这一侧，当前更适合：

- 异构机器人协同。
- UAV / UGV / Quadruped / Humanoid 混合编队。
- 任务图驱动的执行。
- HITL 审核。
- 场景对象与语义编辑。

可参考本仓库已有文档：

- [功能性介绍](index.md)
- [架构](architecture.md)

## 5. 任务集合表

下面这张表按三类任务组织：

- 左列是 `UE` 更自然的任务。
- 中列是 `UE` 和 `Minecraft` 都能做的任务。
- 右列是 `Minecraft` 更自然的任务。
- `置信度` 表示“这个任务归类到该列”的把握，不是任务成功率。

<table>
  <thead>
    <tr>
      <th style="background:#dceafb;">UE 更自然</th>
      <th style="background:#e7f7e9;">两者都可以</th>
      <th style="background:#fff0cc;">Minecraft 更自然</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="background:#f4f9ff;"><strong>U1</strong> 先派 <code>UAV</code> 建图，再让 <code>UGV</code> 进入局部巡检，最后由 <code>humanoid</code> 执行近距离确认。<br/>置信度 <code>90%</code></td>
      <td style="background:#f6fff7;"><strong>C1</strong> 视觉导航到指定目标点。<br/>置信度 <code>95%</code></td>
      <td style="background:#fff9ea;"><strong>M1</strong> 从零开始采木头、做工作台、做镐、挖铁、熔炼，最后采到钻石。<br/>置信度 <code>95%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U2</strong> 无人机在厂区、管廊或园区中做低空视觉巡检，同时考虑禁飞区、相机姿态稳定和碰撞余量。<br/>置信度 <code>95%</code></td>
      <td style="background:#f6fff7;"><strong>C2</strong> 未知区域探索并提高地图覆盖率。<br/>置信度 <code>90%</code></td>
      <td style="background:#fff9ea;"><strong>M2</strong> 在不能“往地里直挖”的约束下，探索并找到洞穴。<br/>置信度 <code>95%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U3</strong> <code>UAV + UGV + humanoid</code> 对同一目标做跨视角接力跟踪，处理遮挡、丢失和重识别。<br/>置信度 <code>90%</code></td>
      <td style="background:#f6fff7;"><strong>C3</strong> 搜索指定目标物或目标体。<br/>置信度 <code>90%</code></td>
      <td style="background:#fff9ea;"><strong>M3</strong> 在村庄旁建一座风格一致的村屋，并按要求展示成果。<br/>置信度 <code>92%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U4</strong> <code>UGV</code> 依据 <code>UAV</code> 的先验地图进入建筑内部，穿越坡道、楼梯、门洞并完成近距检查。<br/>置信度 <code>88%</code></td>
      <td style="background:#f6fff7;"><strong>C4</strong> 单目标主动视觉跟踪。<br/>置信度 <code>75%</code></td>
      <td style="background:#fff9ea;"><strong>M4</strong> 在山地中造一个瀑布，并移动视角拍出合格画面。<br/>置信度 <code>92%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U5</strong> 无人机发现异常后，引导地面机器人到达指定点并完成 rendezvous / handoff。<br/>置信度 <code>85%</code></td>
      <td style="background:#f6fff7;"><strong>C5</strong> 多 agent 协同搜索并在指定地点会合。<br/>置信度 <code>80%</code></td>
      <td style="background:#fff9ea;"><strong>M5</strong> 在村庄附近围一个动物栏，抓两只同类动物进去，且尽量不破坏村庄结构。<br/>置信度 <code>95%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U6</strong> 异构集群在带车辆、行人、动态障碍的场景中执行巡逻、避障与重规划。<br/>置信度 <code>92%</code></td>
      <td style="background:#f6fff7;"><strong>C6</strong> 语言指令驱动的多步任务执行。<br/>置信度 <code>88%</code></td>
      <td style="background:#fff9ea;"><strong>M6</strong> 围绕 <code>wood -&gt; stone -&gt; iron -&gt; diamond</code> 这类 tech-tree 做长链条 crafting / planning。<br/>置信度 <code>94%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U7</strong> 使用 <code>RGB + Depth + LiDAR</code> 做融合建图，再用于探索、定位和回环。<br/>置信度 <code>93%</code></td>
      <td style="background:#f6fff7;"><strong>C7</strong> 中央决策器基于结构化状态做任务分配与子目标下发。<br/>置信度 <code>90%</code></td>
      <td style="background:#fff9ea;"><strong>M7</strong> 以“发现尽可能多新物品或技能”为目标做 open-ended exploration。<br/>置信度 <code>90%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U8</strong> 机械臂抓取盒子并放到车上，或从货架取物后交给移动机器人。<br/>置信度 <code>98%</code></td>
      <td style="background:#f6fff7;"><strong>C8</strong> 把资源、样本或简单任务载荷从 <code>A</code> 点送到 <code>B</code> 点。<br/>置信度 <code>78%</code></td>
      <td style="background:#fff9ea;"><strong>M8</strong> 围绕 inventory、recipe、smelting 做长时程资源规划。<br/>置信度 <code>93%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U9</strong> 两台机器人协同搬运长杆、天线等细长物体，穿过狭窄门洞并保持姿态稳定。<br/>置信度 <code>98%</code></td>
      <td style="background:#f6fff7;"><strong>C9</strong> 巡逻、重访关键区域、按路线依次检查。<br/>置信度 <code>85%</code></td>
      <td style="background:#fff9ea;"><strong>M9</strong> 根据多模态提示完成 build、break、farm、smelt 这类 TeamCraft 风格任务。<br/>置信度 <code>88%</code></td>
    </tr>
    <tr>
      <td style="background:#f4f9ff;"><strong>U10</strong> 基于真实几何、质量、碰撞和约束完成 pick-up、place、carry、dock、charge。<br/>置信度 <code>97%</code></td>
      <td style="background:#f6fff7;"><strong>C10</strong> 简单环境改造以满足任务目标。<br/>置信度 <code>70%</code></td>
      <td style="background:#fff9ea;"><strong>M10</strong> 在 MineLand 一类环境里做“有限感知 + 生存需求 + 多 agent 协作”的生态任务。<br/>置信度 <code>85%</code></td>
    </tr>
  </tbody>
</table>

## 6. 关于“搬运盒子、天线、桌子”的具体判断

这是 Minecraft 和 UE 的核心分界点。

### 6.1 Minecraft 能做什么

Minecraft 天然支持的是：

- 捡起 `item` 到 inventory。
- 放置 `block` 到世界。
- 破坏方块、合成、装备、使用。

因此下面这些是自然的：

- 搬一块方块。
- 捡起掉落物。
- 把资源从 A 地运到 B 地。
- 建一个结构再拆掉重建。

### 6.2 Minecraft 不自然的地方

如果你想搬的是“一个整体物体”：

- 一个完整的盒子模型。
- 一个桌子。
- 一个天线。

那么标准 Minecraft 研究环境里通常会遇到这些问题：

- 对象语义更接近 `block/item/entity`，不是通用刚体。
- 没有 UE 那样自然的抓取-附着-受力-稳定放置链条。
- 多块结构对象通常会被拆解成若干 blocks。
- 视觉上看起来像物体，不等于物理上可被稳定搬运。

换句话说：

- `Minecraft` 擅长“资源搬运”和“方块放置”。
- `UE` 擅长“对象搬运”和“刚体操控”。

### 6.3 DreamerV3 在这个问题上的边界

DreamerV3 很强，但它只能在环境给定的动作和对象语义里学习。

如果环境没有：

- `grasp`
- `attach`
- `carry rigid body`
- `stable placement under physics`

那么 DreamerV3 不会自动长出 UE 式 manipulation 能力。它在 DiamondEnv 里学到的是：

- 探索。
- 采集。
- 合成。
- 工具升级。
- 长时程规划。

不是“搬桌子”。

## 7. 什么时候选 Minecraft，什么时候选 UE

### 7.1 选 Minecraft

优先选 Minecraft，如果你的论文问题接近下面这些：

- 世界模型是否能解决长时程 sparse reward？
- agent 能否在开放世界里形成 reusable skills？
- 人类演示、偏好、语言能否减少探索成本？
- 大模型能否利用互联网知识完成开放任务？

### 7.2 选 UE

优先选 UE，如果你的论文问题接近下面这些：

- 机器人能否搬运、装配、抓取真实对象？
- 多机器人如何在复杂场景中协同执行任务？
- 视觉、深度、激光雷达能否支持闭环控制？
- 场景中需要车辆、人、机器人、传感器和物理共同作用？

### 7.3 两者串联

如果你的总目标很大，也可以分层：

- 用 Minecraft 做高层 agent：任务分解、技能发现、语言与知识推理。
- 用 UE 做低层执行：物理控制、搬运、路径执行、传感器闭环。

这类组合式路线往往比“强行让 Minecraft 承担 manipulation”更稳妥。

## 8. 并行数量结论

如果只关心“并行环境数量级”，不关心内部实现细节，那么可以把常见路线压缩成下面这张表。

| 方案 | 参考并行数量级 | 备注 |
|---|---|---|
| `DreamerV3 + DiamondEnv` | `16` 个训练环境 + `4` 个评估环境 | 这是官方配置里直接给出的默认并行数，代表 `O(10)` 级并行环境。 |
| `CARLA` | 单节点通常按 `O(1~10)` 个独立环境来规划 | 官方明确支持 `multi-simulation`、`multi-GPU`、`off-screen`，但没有像 DreamerV3 那样给固定默认值。对于视觉任务，更合理的预期是单节点个位数到低双位数。 |
| `Isaac Lab` | `O(10^3)` 级并行环境 | 这条路线是 GPU 向量化仿真，官方文档直接给出约 `4096` 个并行环境的示例量级。 |

实用结论：

- `DreamerV3` 代表“十几个环境并行 + replay / imagination 提高数据效率”。
- `CARLA` 代表“UE 系平台的 server-level 并行”，更接近自研 UE 多智能体平台的现实目标。
- `Isaac` 代表“原生大规模 GPU 向量化并行”，不应直接作为 UE 平台的并行数量预期。

## 9. 参考链接

- Project Malmo 论文与文档：
  - [The Malmo Platform for Artificial Intelligence Experimentation (IJCAI 2016)](https://www.ijcai.org/Proceedings/16/Papers/643.pdf)
  - [Malmo MissionSpec / Mission Handlers 文档](https://microsoft.github.io/malmo/0.17.0/Documentation/classmalmo_1_1_mission_spec.html)
  - [Malmo XML Schema: DrawBlock / DrawItem / DrawEntity](https://microsoft.github.io/malmo/0.21.0/Schemas/MissionHandlers.html)
- MineRL：
  - [The MineRL 2019 Competition on Sample Efficient Reinforcement Learning using Human Priors](https://arxiv.org/abs/1904.10079)
  - [MineRL Versions 文档](https://minerl.readthedocs.io/en/latest/notes/versions.html)
  - [MineRL BASALT benchmark repository](https://github.com/minerllabs/basalt-benchmark)
- MineDojo：
  - [MineDojo 文档首页](https://docs.minedojo.org/)
  - [MineDojo Benchmarking Suite](https://docs.minedojo.org/sections/core_api/sim.html)
  - [MineDojo Observation Space](https://docs.minedojo.org/sections/core_api/obs_space.html)
  - [MineDojo Action Space](https://docs.minedojo.org/sections/core_api/action_space.html)
- DreamerV3 / DiamondEnv：
  - [DreamerV3 项目页](https://danijar.com/project/dreamerv3/)
  - [DreamerV3 官方配置 `envs=16`, `eval_envs=4`](https://github.com/danijar/dreamerv3/blob/main/dreamerv3/configs.yaml)
  - [DreamerV3 并行运行时 `parallel.py`](https://github.com/danijar/dreamerv3/blob/main/embodied/run/parallel.py)
  - [DiamondEnv README](https://github.com/danijar/diamond_env)
- MineStudio：
  - [MineStudio Simulator 文档](https://craftjarvis.github.io/MineStudio/v1.0.6/simulator/index.html)
- Unreal Engine / UE 生态：
  - [Chaos Scene Queries and Rigid Body Engine in UE5](https://www.unrealengine.com/en-US/tech-blog/chaos-scene-queries-and-rigid-body-engine-in-ue5)
  - [Physics Constraint Component User Guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/physics-constraint-component-user-guide-in-unreal-engine)
  - [CARLA first steps](https://carla.readthedocs.io/en/0.9.15/tuto_first_steps/)
  - [CARLA Traffic Manager: Multi-simulation](https://carla.readthedocs.io/en/latest/adv_traffic_manager/)
  - [CARLA Multi-GPU](https://carla.readthedocs.io/en/latest/adv_multigpu/)
  - [CARLA Rendering Options](https://carla.readthedocs.io/en/latest/adv_rendering_options/)
  - [CARLA Leaderboard evaluation metrics](https://leaderboard.carla.org/evaluation_v1_0/)
  - [AirSim reinforcement learning](https://microsoft.github.io/AirSim/reinforcement_learning/)
  - [UnrealCV](https://unrealcv.org/)
- Isaac：
  - [Isaac Lab envs API](https://isaac-sim.github.io/IsaacLab/v2.0.0/source/api/lab/isaaclab.envs.html)
  - [TorchRL IsaacLab integration](https://docs.pytorch.org/rl/main/reference/isaaclab.html)
