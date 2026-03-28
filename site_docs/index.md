---
hide:
  - navigation
  - toc
---

<div class="home-landing">
  <div class="home-shell">
    <section class="home-hero">
      <div class="home-topbar">
        <div class="home-nav">
          <a class="home-brand" href=".">MultiAgent-Unreal</a>
          <div class="home-nav-links">
            <a href="#overview">Overview</a>
            <a href="#framework">Framework</a>
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
            <strong>Open-source simulation stack for:</strong> UAV, UGV, Quadruped, and Humanoid collaboration<br>
            <strong>Current focus:</strong> large-scale task execution, web-driven planning review, and multi-robot skill orchestration
          </div>
          <div class="home-link-row">
            <a class="home-link-pill" href="https://github.com/MiangChen/MultiAgent-Unreal" target="_blank">Code</a>
            <a class="home-link-pill" href="startup-and-examples/">Run Example</a>
            <a class="home-link-pill home-link-pill-light" href="api-reference/">API</a>
            <a class="home-link-pill home-link-pill-light" href="architecture/">Architecture</a>
            <a class="home-link-pill home-link-pill-light" href="keybindings/">Keybindings</a>
          </div>
        </div>

        <div class="home-feature-panel">
          <span class="home-feature-label">Live Workflow</span>
          <h2>Plan, review, allocate, execute.</h2>
          <p>
            The current stack already supports Web-side task submission, UE-side human-in-the-loop review,
            skill allocation visualization, and runtime execution for UAV, UGV, Quadruped, and Humanoid agents.
          </p>
          <div class="home-stats">
            <span class="home-stat">UE5</span>
            <span class="home-stat">Web Console</span>
            <span class="home-stat">Task Graph</span>
            <span class="home-stat">Skill Allocation</span>
          </div>
          <figure class="home-figure">
            <img src="assets/images/home/fig1_framework.png" alt="MultiAgent-Unreal framework overview" loading="eager">
          </figure>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="overview">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Abstract</span>
          <h2>Multi-robot simulation as a controllable project page, not just a docs site.</h2>
          <p>
            This site now serves two roles at once: a research-style homepage and an operational entry point.
            The platform connects external planning outputs, web-side review, and Unreal runtime execution.
          </p>
        </div>

        <div class="home-single-card home-abstract-card">
          <p>
            MultiAgent-Unreal is built around a practical loop: external planners emit task structures,
            the web console presents them for review, Unreal receives and visualizes the task graph and skill allocation,
            and heterogeneous robots execute the resulting skills in a shared 3D environment. The project is designed
            to support extendable robot types, object semantics, and task workflows while preserving a strong visual story
            for demos, papers, and external communication.
          </p>
        </div>
      </div>
    </section>

    <section class="home-section" id="featured-media">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Featured Media</span>
          <h2>Main System Story</h2>
          <p>
            The hero visual language should carry the same information as the paper figures: architecture, control loop,
            and runtime behavior.
          </p>
        </div>

        <div class="home-featured-media">
          <video autoplay muted loop controls preload="auto" playsinline>
            <source src="assets/videos/home/human_robot_scheduling_demo.mp4" type="video/mp4">
          </video>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="overview">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Overview</span>
          <h2>System Overview</h2>
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

    <section class="home-section" id="framework">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Execution</span>
          <h2>Execution Snapshots</h2>
          <p>
            Current experiments already cover transport, collaborative task execution, and multi-task scheduling views.
          </p>
        </div>

        <div class="home-snapshot-viewer" data-snapshot-viewer>
          <article class="home-snapshot-stage">
            <figure class="home-snapshot-stage-media">
              <img
                src="assets/images/home/fig6_transport_snapshots.png"
                alt="Transport snapshots"
                loading="lazy"
                data-snapshot-image
              >
            </figure>
            <div class="home-card-copy home-snapshot-stage-copy">
              <h3 data-snapshot-title>Transport Workflow</h3>
              <p data-snapshot-description>
                Humanoid and UGV coordination for object movement and placement inside the simulated environment.
              </p>
            </div>
          </article>

          <div class="home-snapshot-list" role="tablist" aria-label="Execution snapshot selection">
            <button
              class="home-snapshot-item is-active"
              type="button"
              role="tab"
              aria-selected="true"
              data-snapshot-src="assets/images/home/fig6_transport_snapshots.png"
              data-snapshot-alt="Transport snapshots"
              data-snapshot-title="Transport Workflow"
              data-snapshot-description="Humanoid and UGV coordination for object movement and placement inside the simulated environment."
            >
              <span class="home-snapshot-item-index">01</span>
              <span class="home-snapshot-item-copy">
                <strong>Transport Workflow</strong>
                <span>Humanoid and UGV coordination for object movement and placement.</span>
              </span>
            </button>

            <button
              class="home-snapshot-item"
              type="button"
              role="tab"
              aria-selected="false"
              data-snapshot-src="assets/images/home/fig7_multi_task_snapshots.png"
              data-snapshot-alt="Multi-task snapshots"
              data-snapshot-title="Multi-task Snapshots"
              data-snapshot-description="Concurrent tasks across heterogeneous robots, with role-specific skills and runtime execution views."
            >
              <span class="home-snapshot-item-index">02</span>
              <span class="home-snapshot-item-copy">
                <strong>Multi-task Snapshots</strong>
                <span>Concurrent tasks across heterogeneous robots with role-specific skills.</span>
              </span>
            </button>

            <button
              class="home-snapshot-item"
              type="button"
              role="tab"
              aria-selected="false"
              data-snapshot-src="assets/images/home/fig7_multi_task_snapshots_xu.png"
              data-snapshot-alt="Additional multi-task snapshots"
              data-snapshot-title="Cross-scene Execution"
              data-snapshot-description="Different tasks and map layouts can be driven by the same control and skill-allocation pipeline."
            >
              <span class="home-snapshot-item-index">03</span>
              <span class="home-snapshot-item-copy">
                <strong>Cross-scene Execution</strong>
                <span>Different tasks and map layouts driven by the same control pipeline.</span>
              </span>
            </button>
          </div>
        </div>
      </div>
    </section>

    <section class="home-section home-surface" id="results">
      <div class="home-container">
        <div class="home-section-heading">
          <span class="home-section-kicker">Results</span>
          <h2>Planning and Evaluation</h2>
          <p>
            The current figure set already supports a paper-style presentation: benchmark setup, general planning statistics,
            and dynamic replanning behavior.
          </p>
        </div>

        <div class="home-grid home-grid-2">
          <article class="home-media-card">
            <img src="assets/images/home/fig5_cyberbench.png" alt="CyberBench figure" loading="lazy">
            <div class="home-card-copy">
              <h3>Benchmark Setting</h3>
              <p>Structured evaluation scenarios for multi-robot collaboration, web control, and simulation task flow.</p>
            </div>
          </article>
          <article class="home-media-card">
            <img src="assets/images/home/fig8_statistic_general_task_planning.png" alt="General task planning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>General Task Planning</h3>
              <p>Aggregate planning outcomes can be presented directly on the site as paper-ready static figures.</p>
            </div>
          </article>
          <article class="home-media-card">
            <img src="assets/images/home/fig9_statistic_dynamic_replanning.png" alt="Dynamic replanning statistics" loading="lazy">
            <div class="home-card-copy">
              <h3>Dynamic Replanning</h3>
              <p>Runtime replanning behavior and replanning statistics fit naturally into the same project-page layout.</p>
            </div>
          </article>
          <article class="home-media-card">
            <img src="assets/images/home/fig1_framework.png" alt="Framework figure repeated for visual anchor" loading="lazy">
            <div class="home-card-copy">
              <h3>Architecture as a First-Class Story</h3>
              <p>The site can present both research results and system architecture without switching visual language.</p>
            </div>
          </article>
        </div>
      </div>
    </section>

    <section class="home-section">
      <div class="home-container">
        <div class="home-callout">
          <div>
            <h2>Start from the docs, then move into live control.</h2>
            <p>
              The project already includes architecture notes, startup instructions, API references, and a web-driven
              runtime workflow. This homepage is the landing layer; the deeper operational material stays in the docs.
            </p>
            <div class="home-chip-list">
              <span class="home-chip">macOS / Linux</span>
              <span class="home-chip">UAV / UGV / Quadruped / Humanoid</span>
              <span class="home-chip">Task Graph + HITL</span>
              <span class="home-chip">Mock Backend + Web Demo</span>
            </div>
          </div>
          <div class="home-actions">
            <a class="home-button home-button-dark" href="startup-and-examples/">Startup Guide</a>
            <a class="home-button home-button-light" href="architecture/">See Architecture</a>
          </div>
        </div>
      </div>
    </section>
  </div>
</div>
