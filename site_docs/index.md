---
hide:
  - navigation
  - toc
---

<div class="home-landing">
  <div class="home-shell">
    <nav class="home-side-nav" aria-label="Section navigation">
      <a href="#integration">Demo</a>
      <a href="#abstract">Abstract</a>
      <a href="#method">Method</a>
      <a href="#benchmark">Benchmark</a>
      <a href="#results">Results</a>
      <a href="#citation">Citation</a>
    </nav>
    <div class="home-main">
    <section class="home-hero home-hero-mosaic">
      <div class="home-hero-grid" aria-hidden="true">
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/EvidenceCollection.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/area_search.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/assembly_4.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/emergence_response.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/evidence_collection_2.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/guidance_2.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/patrol.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/target_following.mp4" type="video/mp4"></video>
        <video autoplay muted loop playsinline preload="auto"><source src="assets/videos/home/hero/transport_neo.mp4" type="video/mp4"></video>
      </div>

      <div class="home-hero-inner">
        <div class="home-copy home-copy-hero">
          <h1 class="home-title">General Task Planning for Multi-Robot<br>Teams in Open-World Environments</h1>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="integration">
      <div class="home-container">
        <article class="home-media-card home-media-card-wide">
          <div class="home-card-copy home-card-copy-header">
            <h3>System Integration</h3>
          </div>
          <img src="assets/images/home/fig1_framework.png" alt="System integration overview" loading="lazy">
        </article>

        <div class="home-video-panel">
          <video autoplay muted loop controls preload="auto" playsinline>
            <source src="assets/videos/home/hero/transport_neo.mp4" type="video/mp4">
          </video>
        </div>
      </div>
    </section>

    <section class="home-section" id="abstract">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Abstract</span>
        </div>
        <div class="home-single-card home-abstract-card">
          <p>
            Task planning is a key enabler for general multi-robot systems. However, existing approaches are often constrained
            to specific tasks and exhibit limited reliability, making them inadequate for open-world scenarios involving diverse
            tasks, contingencies, and human-robot interaction. In this work, we propose a general multi-robot task planning
            framework with three layers: a task planning layer, a world model layer, and an execution platform layer. At the task
            planning layer, we introduce GT-Planner, a general planner that integrates the generalization capabilities of large
            language models (LLMs) with the reliability and optimality of optimization-based methods. We design a generic
            prompting scheme and fine-tune a small-parameter LLM to balance LLM generality with improved accuracy. We
            further propose the TANGO algorithm for task allocation, which supports a broader class of constraints, including
            pre-specified robot execution. At the world model layer, we maintain a dynamic world model that assesses task
            progress and translates upper-layer plans for the lower layer. At the execution platform layer, we present the
            CyberCity simulation platform: CyberCity-Unreal enables high-fidelity task simulation, while CyberCity-Semantic
            supports large-scale evaluation and LLM fine-tuning. We also develop a human-robot interaction interface that
            accommodates multiple interaction modalities. Finally, we release CyberBench, the first open-world multi-robot
            task-planning benchmark, featuring five heterogeneous robot types, 29+ environmental element types, 10 core
            goal tasks, 19 contingency templates, and 105 tasks.
          </p>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="method">
      <div class="home-container">
        <div class="home-single-card">
          <div class="home-card-copy home-card-copy-header">
            <h3 class="home-card-title">GT-Planner, TANGO, and the three-layer architecture</h3>
          </div>
          <figure class="home-figure">
            <img src="assets/images/home/fig2_method.png" alt="Method overview" loading="lazy">
          </figure>
          <div class="home-card-copy">
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
        <article class="home-media-card home-media-card-wide">
          <div class="home-card-copy home-card-copy-header">
            <h3>Benchmark Setting</h3>
          </div>
          <img src="assets/images/home/fig5_cyberbench.png" alt="CyberBench figure" loading="lazy">
          <div class="home-card-copy">
            <p>CyberBench covers four heterogeneous robot types, more than 50 environment element types, 10 core goal tasks, 19 contingency-event templates, and on the order of 105 evaluation tasks.</p>
          </div>
        </article>
      </div>
    </section>

    <section class="home-section home-surface" id="results">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Quantitative Comparison</span>
        </div>

        <div class="home-media-stack">
          <article class="home-media-card home-media-card-wide">
            <div class="home-card-copy home-card-copy-header">
              <h3>General Planning</h3>
            </div>
            <img src="assets/images/home/fig8_statistic_general_task_planning.png" alt="General task planning statistics" loading="lazy">
            <div class="home-card-copy">
              <p>These results compare overall success rate, efficiency, planning quality, and energy consumption across methods. The paper reports GT-Planner as the strongest overall method for general task planning.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <div class="home-card-copy home-card-copy-header">
              <h3>Dynamic Replanning</h3>
            </div>
            <img src="assets/images/home/fig9_statistic_dynamic_replanning.png" alt="Dynamic replanning statistics" loading="lazy">
            <div class="home-card-copy">
              <p>These results evaluate replanning across difficulty levels and show that GT-Planner maintains stronger robustness and lower performance degradation as contingencies increase.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <div class="home-card-copy home-card-copy-header">
              <h3>Fine-Tuning Placeholder</h3>
            </div>
            <img src="assets/images/home/fig1_framework.png" alt="Fine-tuning placeholder figure" loading="lazy">
            <div class="home-card-copy">
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

        <div class="home-link-row home-link-row-footer">
          <a class="home-link-pill" href="https://github.com/MiangChen/MultiAgent-Unreal" target="_blank">Code</a>
          <a class="home-link-pill" href="startup-and-examples/">Run Example</a>
          <a class="home-link-pill" href="api-reference/">API</a>
          <a class="home-link-pill" href="architecture/">Architecture</a>
          <a class="home-link-pill" href="keybindings/">Keybindings</a>
        </div>
      </div>
    </section>
    </div>
  </div>
</div>
