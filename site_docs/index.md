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
          <span class="home-kicker">UE5 Multi-Agent Simulation</span>
          <h1 class="home-title">MultiAgent-Unreal</h1>
          <p class="home-subtitle">
            A web-controlled multi-robot simulation platform for heterogeneous teams, task-graph review,
            skill allocation, and coordinated execution inside large Unreal Engine environments.
          </p>
          <p class="home-meta">
            Heterogeneous robots · Task graph review · Skill allocation · HITL execution · Scene-graph aware simulation
          </p>
          <div class="home-authors">
            <strong>Author information:</strong> temporarily anonymized in the current draft.
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
          <h2>Transport Demo in the Runtime Loop</h2>
          <p>
            The current stack already supports web-side submission, Unreal-side review, skill allocation visualization,
            and coordinated execution for heterogeneous robot teams.
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
          <h2>A Practical Loop from External Planning to Robot Execution</h2>
          <p>
            MultiAgent-Unreal connects external planning outputs, web-side review, and Unreal runtime execution
            into one operational project page.
          </p>
        </div>

        <div class="home-single-card home-abstract-card">
          <p>
            MultiAgent-Unreal is built around a practical loop: external planners emit task structures,
            the web console presents them for review, Unreal receives and visualizes the task graph and skill allocation,
            and heterogeneous robots execute the resulting skills in a shared 3D environment. The platform is designed
            to support extendable robot types, object semantics, and task workflows while preserving a coherent visual
            story for demos, papers, and external communication.
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
            The platform centers on a UE runtime, a local web console, and a structured task pipeline:
            task graph review, skill allocation review, and final executable skill lists.
          </p>
        </div>

        <div class="home-single-card">
          <figure class="home-figure">
            <img src="assets/images/home/fig2_method.png" alt="Method overview" loading="lazy">
          </figure>
          <div class="home-card-copy">
            <h3 class="home-card-title">From task intent to robot execution</h3>
            <p>
              The external planner, task graph representation, web review flow, and UE execution runtime form a
              single closed loop. The project is designed to support multiple robot categories and extendable object semantics.
            </p>
          </div>
        </div>
      </div>
    </section>

    <section class="home-section" id="benchmark">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Benchmark</span>
          <h2>Structured Benchmark Setting</h2>
          <p>
            The benchmark setup captures evaluation scenarios for multi-robot collaboration, web control,
            and simulation-driven task workflows.
          </p>
        </div>

        <article class="home-media-card home-media-card-wide">
          <img src="assets/images/home/fig5_cyberbench.png" alt="CyberBench figure" loading="lazy">
          <div class="home-card-copy">
            <h3>Benchmark Setting</h3>
            <p>Structured evaluation scenarios for multi-robot collaboration, web control, and simulation task flow.</p>
          </div>
        </article>
      </div>
    </section>

    <section class="home-section home-surface" id="results">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Quantitative Comparison</span>
          <h2>Planning, Replanning, and Fine-Tuning</h2>
          <p>
            The quantitative section is arranged as a sequence of figures, covering general planning,
            dynamic replanning, and a temporary fine-tuning placeholder.
          </p>
        </div>

        <div class="home-media-stack">
          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig8_statistic_general_task_planning.png" alt="General task planning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>General Planning</h3>
              <p>Aggregate planning outcomes can be presented directly on the site as paper-ready static figures.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig9_statistic_dynamic_replanning.png" alt="Dynamic replanning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>Dynamic Replanning</h3>
              <p>Runtime replanning behavior and replanning statistics fit naturally into the same project-page layout.</p>
            </div>
          </article>

          <article class="home-media-card home-media-card-wide">
            <img src="assets/images/home/fig1_framework.png" alt="Fine-tuning placeholder figure" loading="lazy">
            <div class="home-card-copy">
              <h3>Fine-Tuning Placeholder</h3>
              <p>This slot currently reuses the framework figure and can be replaced with a dedicated fine-tuning result later.</p>
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
            If you use MultiAgent-Unreal in research, demos, or internal evaluation, cite the project entry below.
            If a formal paper version is released later, this block can be replaced with the paper BibTeX.
          </p>
        </div>

        <div class="home-citation-card">
          <pre class="home-citation-block"><code>@misc{multiagentunreal2026,
  title={MultiAgent-Unreal: A Web-Controlled Multi-Robot Simulation Platform for Heterogeneous Teams in Unreal Engine},
  author={Chen, Miang and contributors},
  year={2026},
  howpublished={GitHub repository},
  url={https://github.com/MiangChen/MultiAgent-Unreal},
  note={Project page: https://miangchen.github.io/MultiAgent-Unreal/}
}</code></pre>
        </div>
      </div>
    </section>
  </div>
</div>
