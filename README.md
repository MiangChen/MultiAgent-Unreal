# MultiAgent-Unreal

MultiAgent-Unreal is an Unreal Engine 5 simulation project for studying heterogeneous multi-agent systems. It provides a runtime where UAVs, UGVs, quadrupeds, and humanoid agents can be spawned, commanded, observed, and evaluated in shared 3D environments.

The project is designed as a bridge between an external planner and an embodied UE simulation. A planner can send task graphs, skill allocations, and skill lists through HTTP; the UE side renders the world, executes agent skills, and returns runtime feedback.

## What This Repository Contains

This repository contains the code and configuration needed to run the simulation framework:

- UE5 C++ source under `unreal_project/Source/`
- UE project configuration under `unreal_project/Config/`
- Python scripts for mock backends, dataset utilities, and architecture checks
- JSON examples for task graphs, scene graphs, and skill allocation
- MkDocs documentation under `site_docs/`
- Repository instructions for AI assistants in `AGENTS.md`

Large Unreal assets are intentionally not stored in GitHub. The local `unreal_project/Content/` directory is ignored because it is too large for normal Git history and should be restored from the external asset source described in `AGENTS.md`.

## What The Simulation Does

The runtime focuses on coordinated robot behavior in complex scenes:

- Spawn and manage heterogeneous agents.
- Navigate ground and aerial agents through UE maps.
- Execute high-level skills such as search, follow, guide, takeoff, land, take photo, broadcast, charge, place, and return home.
- Receive structured planner outputs from a backend service.
- Display human-in-the-loop task graph, skill allocation, and decision views.
- Maintain scene graph and task graph data for planning workflows.
- Provide a web/mock backend for local integration tests and demos.

The goal is not just to render robots in UE. The project is a testbed for connecting symbolic planning, runtime skill execution, scene understanding, and human supervision inside one simulation loop.

## How To Reproduce The Project

We strongly recommend using an AI coding assistant to configure the local environment. Give the assistant this file:

```text
AGENTS.md
```

`AGENTS.md` is the canonical reproduction guide for this repository. It tells the assistant which files belong in Git, which Unreal directories must stay ignored, how to restore `Content`, how to configure UE paths, and how to start the mock backend plus UE runtime.

The short version is:

```bash
git clone https://github.com/MiangChen/MultiAgent-Unreal.git
cd MultiAgent-Unreal

# Restore Unreal Content assets separately.
# See AGENTS.md for the current asset source and exact commands.

python3 scripts/mock_backend.py --port 8081
./mac_compile_and_start.sh
```

## Repository Boundary

This repository should stay source-first. Do not commit generated Unreal output or large local assets:

- `unreal_project/Content/`
- `unreal_project/Intermediate/`
- `unreal_project/Binaries/`
- `unreal_project/Saved/`
- `unreal_project/DerivedDataCache/`
- `site/`
- `tmp/`

If a collaborator needs the full runnable environment, share the GitHub repository plus the external Content asset package. The code repository alone is enough to understand and modify the framework, but not enough to open every map with all assets present.

## License

MIT License
