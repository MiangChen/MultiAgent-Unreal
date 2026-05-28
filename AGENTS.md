# Repository Instructions

This file is the canonical guide for AI assistants working on `MiangChen/MultiAgent-Unreal`. If a user asks an AI to reproduce or modify this repository, provide this file first.

## Project Purpose

MultiAgent-Unreal is an Unreal Engine 5 simulation framework for heterogeneous multi-agent robotics. The runtime connects UE agents with an external planner over HTTP. The planner can send task graphs, skill allocations, and skill lists; UE executes skills, renders the world, and reports feedback.

The repository is source-first. Large UE assets are stored outside GitHub and restored into `unreal_project/Content/` when a runnable local environment is needed.

## Repository Layout

- `unreal_project/Source/`: UE C++ source for agents, navigation, sensing, skills, state tree, communication, camera, scene graph, and UI.
- `unreal_project/Config/`: UE and simulation configuration.
- `unreal_project/Plugins/`: project plugins that are source controlled.
- `config/`: external runtime configuration and map settings.
- `datasets/`: small planner, task graph, scene graph, and response examples.
- `scripts/`: mock backend, dataset utilities, architecture checks, and web UI helpers.
- `site_docs/`: MkDocs source for the project documentation site.
- `README.md`: human-facing project overview.

## What Must Not Be Committed

Do not add generated Unreal output, local caches, or large assets to Git:

- `unreal_project/Content/`
- `unreal_project/Intermediate/`
- `unreal_project/Binaries/`
- `unreal_project/Saved/`
- `unreal_project/DerivedDataCache/`
- `unreal_project/Plugins/*/Binaries/`
- `unreal_project/Plugins/*/Intermediate/`
- `site/`
- `tmp/`
- `.idea/`, `.vscode/`, `.DS_Store`
- generated code graph files such as `*.cc.json.gz`

Do not re-enable Git LFS tracking for `*.uasset` or `*.umap` in this repository. Unreal Content assets belong in the external asset source, not in this GitHub repo.

## Reproducing A Local Environment

### 1. Clone The Source Repository

```bash
git clone https://github.com/MiangChen/MultiAgent-Unreal.git
cd MultiAgent-Unreal
```

### 2. Restore Unreal Content Assets

The GitHub repository intentionally excludes `unreal_project/Content/`. Restore it separately before opening the UE project.

Preferred asset source:

```bash
git lfs install
cd unreal_project
git clone https://huggingface.co/datasets/WindyLab/MultiAgent-Content Content
```

China mainland mirror:

```bash
cd unreal_project
git clone https://hf-mirror.com/datasets/WindyLab/MultiAgent-Content Content
```

Alternative script:

```bash
./scripts/dataset/setup_hf_content.sh
```

If the asset repository moves, ask the project owner for the current Content package. The code repository can be edited without Content, but the full UE scene cannot be reproduced without it.

### 3. Install Runtime Requirements

Expected local environment:

- macOS or Linux
- Unreal Engine 5.7.x preferred
- Python 3.8+
- Git LFS only for the external Content asset source

For documentation preview:

```bash
python3 -m pip install --user mkdocs mkdocs-material
mkdocs serve
```

### 4. Configure Unreal Engine Path

On macOS, check `mac_compile_and_start.sh`.

Default expected path:

```bash
UE5_ROOT="/Users/Shared/Epic Games/UE_5.7"
```

If UE is installed elsewhere, update `UE5_ROOT` before compiling.

### 5. Configure Planner Backend

The default planner endpoint is in `config/simulation.json`:

```json
"server": {
  "planner_url": "http://localhost:8081"
}
```

Use the mock backend for local development:

```bash
python3 scripts/mock_backend.py --port 8081
```

Then open:

```text
http://localhost:8081
```

### 6. Build And Run UE

In another terminal:

```bash
./mac_compile_and_start.sh
```

Useful variants:

```bash
./mac_compile_and_start.sh -c Debug
./mac_compile_and_start.sh -r
./mac_compile_and_start.sh -- -ResX=1920 -ResY=1080
```

## Core Configuration Files

- `config/simulation.json`: global simulation settings, planner URL, navigation, flight, follow, guide, and skill parameters.
- `config/maps/*.json`: per-map configuration, map path, spectator camera, agents, and scene graph folders.
- `unreal_project/Config/DefaultEngine.ini`: UE engine settings.
- `unreal_project/Config/DefaultInput.ini`: input binding configuration.
- `mkdocs.yml`: documentation site configuration.

See `config/README.md` for the detailed configuration reference.

## Local Interaction Basics

Common controls:

- Left click: select agent.
- Ctrl + left click: add or remove selection.
- Middle click: navigate selected agents to clicked location.
- Right-drag: rotate camera.
- `M`: toggle Edit mode.
- `,`: toggle Modify mode.
- `Z`: task graph view.
- `N`: skill allocation view.
- `9`: decision view when a decision notification exists.
- `Tab` / `C`: switch to next agent camera.
- `0` / `V`: return to spectator camera.
- `P`: pause or resume skill execution.

For the full key list, read `site_docs/keybindings.md`.

## Architecture Preferences

- Prefer a single canonical naming scheme across the project.
- Do not add compatibility aliases, fallback mappings, dual naming, or runtime shims unless explicitly requested.
- When names, types, or concepts are inconsistent, normalize them to one canonical term across code, scripts, docs, and data.
- Avoid adding `if/else` branches only to preserve old behavior unless explicitly required.
- Prefer removing obsolete paths over preserving backward compatibility.
- If a compatibility layer seems necessary, stop and explain why before implementing it.
- Prefer structural cleanup over incremental patching when the inconsistency is local and fixable.
- Treat long-term consistency as more important than minimal local changes.
- Keep external scripts, datasets, docs, and in-engine/runtime terminology aligned.

## Naming Policy

- Use the project’s existing canonical runtime names and types as the source of truth.
- If a term is already established in code, update scripts, docs, and data to match it rather than introducing synonyms.
- Do not keep both an old name and a new name active at the same time unless explicitly requested.

## Validation Before Push

Before pushing, check that no ignored large directories entered the index:

```bash
git status --short
git ls-files | grep -E '^(unreal_project/(Content|Intermediate|Binaries|Saved|DerivedDataCache)/|site/|tmp/)' && echo "bad files found"
git lfs ls-files --all
```

Expected result for `git lfs ls-files --all` in this repository is no output.
