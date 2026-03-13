# Repository Instructions

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
- If a term is already established in code, update scripts/docs/data to match it rather than introducing synonyms.
- Do not keep both an old name and a new name active at the same time unless explicitly requested.
