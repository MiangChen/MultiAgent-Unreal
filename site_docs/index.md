---
hide:
  - navigation
  - toc
---

<div class="home-landing">
  <div class="home-shell">
    <section class="home-hero home-hero-mosaic">
      <div class="home-hero-grid" aria-hidden="true">
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4"></video>
      </div>

      <div class="home-topbar">
        <div class="home-nav">
          <a class="home-brand" href=".">MultiAgent-Unreal</a>
          <div class="home-nav-links">
            <a href="#integration">Demo</a>
            <a href="#method">Method</a>
            <a href="#benchmark">Benchmark</a>
            <a href="#results">Results</a>
            <a href="architecture/">Architecture</a>
            <a href="startup-and-examples/">Startup</a>
          </div>
        </div>
      </div>

      <div class="home-hero-inner">
        <div class="home-copy">
          <span class="home-kicker">Anonymous Submission</span>
          <h1 class="home-title">General Task Planning for Multi-Robot Teams in Open-World Environments</h1>
          <p class="home-subtitle">
            A general multi-robot task planning framework with a task planning layer, a world model layer,
            and an execution platform layer for heterogeneous robot teams in open-world environments.
          </p>
          <p class="home-meta">
            GT-Planner · TANGO · CyberCity · CyberBench · Human-robot interaction
          </p>
          <div class="home-authors">
            <strong>Submission status:</strong> anonymous draft aligned with the current paper version.
          </div>
          <div class="home-link-row">
            <a class="home-link-pill" href="https://github.com/MiangChen/MultiAgent-Unreal" target="_blank">Code</a>
            <a class="home-link-pill" href="startup-and-examples/">Run Example</a>
            <a class="home-link-pill home-link-pill-light" href="api-reference/">API</a>
            <a class="home-link-pill home-link-pill-light" href="architecture/">Architecture</a>
            <a class="home-link-pill home-link-pill-light" href="keybindings/">Keybindings</a>
          </div>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="integration">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">System Integration</span>
          <h2>CyberCity-Unreal Runtime Demo</h2>
          <p>
            CyberCity-Unreal provides high-fidelity task simulation, integrating web-side review,
            world-model updates, and runtime execution for heterogeneous robot teams.
          </p>
        </div>

        <div class="home-video-panel">
          <video autoplay muted loop controls preload="auto" playsinline>
            <source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4">
          </video>
        </div>
      </div>
    </section>

    <section class="home-section" id="abstract">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Abstract</span>
          <h2>Task Planning as the Core of General Multi-Robot Systems</h2>
          <p>
            The paper frames multi-robot task planning as the key path toward general open-world deployment.
          </p>
        </div>

        <div class="home-single-card home-abstract-card">
          <p>
            Task planning is a key enabler for general multi-robot systems, yet existing approaches are often constrained
            to specific tasks and exhibit limited reliability in open-world scenarios involving diverse tasks, contingencies,
            and human-robot interaction. This work proposes a three-layer framework with a task planning layer,
            a world model layer, and an execution platform layer. At the task planning layer, GT-Planner combines
            the generalization capabilities of large language models with the reliability and optimality of optimization-based
            methods, while TANGO handles allocation with broader temporal and coherence constraints. At the platform layer,
            CyberCity-Unreal supports high-fidelity execution and CyberCity-Semantic supports large-scale evaluation and
            fine-tuning. CyberBench provides the benchmark setting for open-world multi-robot task planning.
          </p>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="method">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Method</span>
          <h2>Method Framework</h2>
          <p>
            The paper organizes the system into a task planning layer, a world model layer,
            and an execution platform layer.
          </p>
        </div>

        <div class="home-single-card">
          <figure class="home-figure">
            <img src="assets/images/home/fig2_method.png" alt="Method overview" loading="lazy">
          </figure>
          <div class="home-card-copy">
            <h3 class="home-card-title">GT-Planner, TANGO, and the three-layer architecture</h3>
            <p>
              As presented in the paper, GT-Planner interprets natural-language instructions and constructs a task
              dependency graph, TANGO performs task allocation under richer constraints, the world model tracks task
              progress and feedback, and CyberCity executes the resulting atomic skills in simulation.
            </p>
          </div>
        </div>
      </div>
    </section>

    <section class="home-section" id="benchmark">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Benchmark</span>
          <h2>CyberBench Benchmark Setting</h2>
          <p>
            CyberBench is the benchmark section of the paper for general open-world multi-robot task planning.
          </p>
        </div>

        <article class="home-media-card home-media-card-wide">
          <img src="assets/images/home/fig5_cyberbench.png" alt="CyberBench figure" loading="lazy">
          <div class="home-card-copy">
            <h3>Benchmark Setting</h3>
            <p>CyberBench covers four heterogeneous robot types, more than 50 environment element types, 10 core goal tasks, 19 contingency-event templates, and on the order of 105 evaluation tasks.</p>
          </div>
        </article>
      </div>
    </section>

    <section class="home-section home-surface" id="results">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Quantitative Comparison</span>
          <h2>General Planning, Dynamic Replanning, and Fine-Tuning</h2>
          <p>
            The quantitative section follows the paper structure: general task planning, dynamic replanning,
            and a temporary fine-tuning placeholder.
          </p>
        </div>

        <div class="home-media-stack">
          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig8_statistic_general_task_planning.png" alt="General task planning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>General Planning</h3>
              <p>These results compare overall success rate, efficiency, planning quality, and energy consumption across methods. The paper reports GT-Planner as the strongest overall method for general task planning.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig9_statistic_dynamic_replanning.png" alt="Dynamic replanning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>Dynamic Replanning</h3>
              <p>These results evaluate replanning across difficulty levels and show that GT-Planner maintains stronger robustness and lower performance degradation as contingencies increase.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig1_framework.png" alt="Fine-tuning placeholder figure" loading="lazy">
            <div class="home-card-copy">
              <h3>Fine-Tuning Placeholder</h3>
              <p>The current paper text describes a two-stage fine-tuning strategy: supervised fine-tuning for initialization, followed by RL-based fine-tuning for generalization. This slot currently reuses the framework figure and can be replaced with a dedicated fine-tuning result later.</p>
            </div>
          </article>
        </div>
      </div>
    </section>

    <section class="home-section" id="citation">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Citation</span>
          <h2>Cite This Project</h2>
          <p>
            The homepage is now aligned with the current anonymous paper draft. This citation block uses the paper title and can be updated with the final publication metadata later.
          </p>
        </div>

        <div class="home-citation-card">
          <pre class="home-citation-block"><code>@misc{general_task_planning_multi_robot_2026,
  title={General Task Planning for Multi-Robot Teams in Open-World Environments},
  author={Anonymous submission},
  year={2026},
  note={Project page and code release aligned with the current anonymous draft},
  url={https://miangchen.github.io/MultiAgent-Unreal/}
}</code></pre>
        </div>
      </div>
    </section>
  </div>
</div>
