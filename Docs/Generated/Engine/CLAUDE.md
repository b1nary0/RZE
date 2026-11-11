# Docs diagrams (Mermaid)

`Docs/Generated/Engine` is read in Obsidian, which ships **Mermaid 11.13.0**. Failures are silent in the source, so
render and look.
- Start every flowchart, graph, state and sequence diagram with the shared init line. Copy it from an existing doc. It
  sets subgraph title margins and the label backgrounds and halos that keep lines from showing through labels. Don't
  add fallback colours.
- Sequence diagrams: no `;` anywhere in text, no keyword participant aliases (`loop`, `alt`, `end`), notes spanning two
  participants (`Note over A,B:`), short participant names.
- Gantt: `dateFormat x`, `axisFormat %L`, tasks as `id, start, 4ms` or `id, after other, 4ms`.
- Prefer `flowchart TD` to long LR chains; use `direction TB` inside side-by-side subgraphs. Avoid several edges
  between the same two nodes and self-links in class diagrams. Keep quadrant points away from y≈0.5 and the titles.
- Before committing, render every block with the Mermaid CLI pinned to Mermaid 11.13.0
  (`@mermaid-js/mermaid-cli` 11.12.0 with `"overrides": { "mermaid": "11.13.0" }`), dark theme, a `-C` CSS file setting
  `--background-primary`, and look at each PNG.
