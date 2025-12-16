
\subsection{多机器人仿真平台}\label{subsec:platform}

多机器人仿真平台的开发面临着一个矛盾：\textbf{保真度与可扩展性难以兼得}。现有平台要么支持大规模智能体但缺乏视觉真实性，要么提供高保真渲染但受限于计算开销而无法扩展。

\subsubsection{大规模集群仿真平台}

为支持大规模多智能体研究，一系列平台采用简化的2D环境以换取可扩展性。

受OpenAI Gym~\cite{brockman2016openai}标准化API的启发，\textbf{PettingZoo}~\cite{terry2021pettingzoo}引入了具有统一接口的连续2D环境，支持捕食者-猎物追逐、群体行为等经典场景，可容纳约100个智能体。\textbf{MAgent2}~\cite{zheng2018magent, magent2020}进一步将规模拓展到数千个像素智能体，主要用于大规模对抗博弈研究。在符号规划领域，\textbf{PDDLGym}~\cite{silver2020pddlgym}将PDDL与Gym风格接口桥接，\textbf{HDDLGym}~\cite{hddlgym2025}将其扩展到层次规划，支持Overcooked等多智能体协作场景。游戏环境如\textbf{StarCraft Multi-Agent Challenge}~\cite{samvelyan2019starcraft}也被广泛用于MARL研究，支持数十个单位的协调控制。

这类平台的核心优势在于计算效率：2D空间下状态空间和动力学都经过了大量简化，使得大规模并行训练成为可能。但2D或像素场景的缺陷是表示无法提供RGB-D图像、深度感知等VLM和VLA所需的感知输入，并且智能体之间的空间结构关系和真实世界存在维度差距，导致在这些平台上训练的算法难以迁移到真实世界。

\subsubsection{高保真平台}

为弥合仿真与现实的差距，另一类平台追求高保真的3D渲染和物理仿真。

在单智能体领域，\textbf{AI2-THOR}~\cite{kolve2017ai2thor}开创了具有丰富物体状态变化的交互式3D仿真，为ALFRED~\cite{ALFRED20}等具身智能基准奠定基础。\textbf{iGibson}系列~\cite{shen2021igibson1, li2021igibson2}强调物理真实性，支持BEHAVIOR~\cite{li2022behaviork}等长时程任务基准。\textbf{Habitat}~\cite{savva2019habitat}通过优化的C++渲染实现约10,000 FPS的训练吞吐量。这些仿真器展示了3D环境训练具身智能体的巨大潜力。

将高保真仿真扩展到多智能体场景的尝试也在进行中。\textbf{Habitat 3.0}~\cite{puig2023habitat3}增加了多智能体和人机交互支持，但可扩展性限制在约10个智能体。基于Isaac Gym构建的\textbf{MQE}和\textbf{MAPush}支持四足机器狗进行MARL, 完成推箱子/放牧等任务。基于Isaac Sim构建的\textbf{GRUtopia}~\cite{wang2024grutopia}提供高质量城市场景，支持多种异构机器人, UGV, Humanoid完成社会导航任务。但由于Isaac的精确物理解算的开销，这几个仿真场景中实际可用规模约为5个智能体.

基于Unreal Engine的\textbf{CARLA}~\cite{dosovitskiy2017carla}和\textbf{AirSim}~\cite{shah2018airsim}分别支持自动驾驶和飞行器仿真，可扩展至约50和10个智能体, 并且分别关注车辆和无人机的视觉相关任务, 而非异构集群或通用任务规划。基于UnrealCV~\cite{qiu2017unrealcv}构建的\textbf{UnrealZoo}~\cite{unrealzoo2025}引入了丰富的智能体, 如人类, 无人机, 四足狗, 动物等, 但其适用任务仅支持视觉导航。

这类平台的优势在于缩小了仿真到现实的缝隙：逼真的渲染和物理交互使得算法得到了更充分的验证, 更容易部署到真实机器人。但高保真也带来了严重的计算开销, 每个智能体都需要独立的物理解算、碰撞检测和渲染管线，计算成本随智能体数量呈线性增长, 制约了可拓展性。

\subsubsection{保真度-可扩展性矛盾}

表~\ref{tab:platform_comparison}从多机器人任务规划研究的关键维度总结了现有平台。

\begin{table}[t]
\centering
\caption{多机器人仿真平台对比}
\label{tab:platform_comparison}
\begin{tabular}{lccccc}
\toprule
\textbf{平台} & \textbf{3D} & \textbf{多智能体} & \textbf{规模} & \textbf{类型} & \textbf{技能} \\
\midrule
PDDLGym~\cite{silver2020pddlgym} & \xmark & \xmark & 1 & G & \cmark \\
HDDLGym~\cite{hddlgym2025} & \xmark & \cmark & $\sim$10 & G & \cmark \\
PettingZoo~\cite{terry2021pettingzoo} & \xmark & \cmark & $\sim$100 & G & \xmark \\
MAgent2~\cite{zheng2018magent, magent2020} & \xmark & \cmark & 1000+ & G & \xmark \\
AI2-THOR~\cite{kolve2017ai2thor} & \cmark & \xmark & 1 & G & \cmark \\
iGibson~\cite{li2021igibson2} & \cmark & \xmark & 1 & G & \cmark \\
Habitat 3.0~\cite{puig2023habitat3} & \cmark & \cmark & $\sim$10 & G+H & \cmark \\
GRUtopia~\cite{wang2024grutopia} & \cmark & \cmark & $\sim$5 & G+A+H & \cmark \\
CARLA~\cite{dosovitskiy2017carla} & \cmark & \cmark & $\sim$50 & G & \xmark \\
AirSim~\cite{shah2018airsim} & \cmark & \cmark & $\sim$10 & G+A & \xmark \\
UnrealZoo~\cite{unrealzoo2025} & \cmark & \cmark & $\sim$10 & G+A+H & \xmark \\
\midrule
\textbf{Ours} & \cmark & \cmark & \textbf{100+} & \textbf{G+A+H} & \cmark \\
\bottomrule
\multicolumn{6}{l}{\footnotesize G: 地面机器人, A: 空中机器人, H: 人形/人类。} \\
\end{tabular}
\end{table}

上述分析揭示了当前仿真平台发展中的\textbf{保真度-可扩展性矛盾}（Fidelity-Scalability Trade-off）：

\begin{itemize}
    \item \textbf{高可扩展、低保真}：2D平台通过简化表示实现大规模仿真，但无法提供视觉输入和三维模拟，限制了算法向真实世界的迁移。
    \item \textbf{高保真、低可扩展}：3D仿真器提供逼真的视觉和物理，但计算开销随智能体数量急剧增长，实际可用规模通常限制在5-10个智能体。
\end{itemize}

这一矛盾的根源在于：现有高保真仿真器多为单智能体场景设计，追求全方位的保真——精确的刚体动力学、接触力学、传感器噪声模型等。然而，对于多机器人\textit{任务规划}研究，这种全方位保真有些过度设计。任务规划的关注点在于高层决策与协调策略，而非底层动力学细节, 大量经典的集群算法研究也是基于2D粒子模型完成验证的~\cite{reynolds1987flocks, vicsek1995novel, yu2022surprising}。

基于这一观察，本文提出\textbf{选择性保真}（Selective Fidelity）的设计原则：提供逼真的图形渲染以支持人在回路监督，同时在动力学层面采用简化模型以降低计算开销。这一原则指导了MultiAgent-Unreal平台的设计，具体方法将在第~\ref{sec:method}节详述。
