# AGENTS.md — Android Game Harness

**Meta-framework** for AI Agent-driven development. Four defense layers: Rules → Skills → Agents+Workflow → Scripts+human.

## Command system (single entrypoints)

| Command | Action |
|---------|--------|
| `/agh-clarify` | Scene detect + clarify + output_type + mode → proposal |
| `/agh-propose` | BA→SA→TE→PM→SR1 |
| `/agh-apply` | DE TDD → TE audit → SR2 → TE final → SR3 |
| `/agh-archive` | Spec merge + archive → SR4 |
| `/agh-run` | Auto-advance all phases (no skip approval gates) |
| `/agh-android-game` | Shorthand for Android game feature dev |
| `/agh-openspec` | 通用制品版本管理协议 → 各角色通过 OpenSpec 管理产出物版本、追溯和变更（specs/ + changes/ + drafts/ 工作流） |
| `/openspec-init` | SnakeShot 游戏特性规格初始化（生成设计规格文档） |

Sequence is strict: clarify → propose → apply → archive. No skipping.

## Architecture

- **Paradigm**: Agent orchestrates Skill → Skill drives data-operation Skill → data-operation Skill reads/writes data layer (see `feature_spec_knowledge.md`)
- **Roles (agents)**: PM (scheduler + dynamic-sync), BA (requirements), SA (design), DE (TDD dev + 3-phase Code-Review), TE (black-box + white-box test), UX (visual/structure)
- **Data-operation Skills** (data layer access, shared by all executing roles):
  - `FeatureService` — Feature config (switch/param/dependency/scope/lifecycle, Android SystemFeatures)
  - `SpecService` — Spec requirement read/write (bidirectional transform with Knowledge)
  - `KnowledgeService` — domain knowledge; Hybrid = LLM Wiki (semantic) + Relation Graph (deterministic), 3-tier cache (L1 hot / L2 context / L3 persist)
  - `CodeGraphService` — code structure; L1 static index (Clang AST/CG/CFG/#ifdef) + L2 LLM semantic + L3 incremental
- **Role isolation**: info passes through PM via handoff files only; non-PM roles never reference other roles' reasoning; executing roles never touch data layer directly (must go through data-operation Skills)
- **Two platform modes**: Claude Code (SubAgent for physical isolation) vs Cline (file protocol + explicit role switching)
- **State**: `.state.md` (single source of truth, schema in `templates/state-template.md`)
- **Process log**: `log/process.log`, format `[{timestamp}] [{role}] {event}`
- **Dynamic sync**: Feature/Spec/Knowledge sync via Event Bus; versioned changelog + consistency snapshots (`{Spec}_{Feature}_{Knowledge}.snap`)

## Key files

| File | Role |
|------|------|
| `CLAUDE.md` | Highest rules constraint, all agents must obey |
| `.clinerules` | Cline-specific supplements (role switching protocol, command routing) |
| `skills/*.md` | SOPs - one per command + dev-test + post-verify |
| `agents/*.md` | Role contracts (identity, duties, inputs, outputs, blockers, prohibitions) |
| `templates/handoff-template.md` | Inter-role handoff format |
| `templates/state-template.md` | `.state.md` authoritative schema |
| `skills/agh-openspec.md` | OpenSpec 协议 → 所有角色制品的版本管理、追溯矩阵、变更追溯|

## Verification scripts (exit code = truth)

```bash
npm run check        # scripts/check-harness.sh → framework integrity
npm run verify       # scripts/verify.sh [A|B|C|all] [REQ-ID] → deliverable verification
npm run baseline     # scripts/baseline.sh [REQ-ID] → baseline diff check
# Android: gradle test + ctest  # Native+JVM dual verification
```

Verify check types: A=file existence, B=phase+output_type-aware completeness, C=process consistency.

## DE dev cycle (mandatory)

1. TDD: **R1 Pre-Review** → red (black-box test fails) → green + **R2 Implementation Review** → white-box → refactor + **R3 Refactoring Review**
2. Three-phase Code-Review is mandatory: R1 (before red), R2 (after green), R3 (during refactor) — recorded in code-report.md
3. Run `skills/dev-test.md` (routes by tech_stack.language: black-box 100% + white-box ≥80% → lint → build)
4. Run `skills/post-verify.md` (`verify.sh A` + handoff output check + 3-phase review check + no-scope-creep check)
5. Data access only via `FeatureService/SpecService/KnowledgeService/CodeGraphService`
6. DE self-repair cap: 3 internal attempts (each re-runs TDD cycle incl. R2/R3) before reporting to PM
7. PM-level repair loop: max 5 rounds, tracked in `.state.md` `repair_round`

## TE audit (driven by test_strategy)

| strategy | method |
|----------|--------|
| e2e | Playwright/Selenium; degrade to engineering check if `env.browser_available=false` |
| unit | Full suite, → 0% coverage or warn |
| integration | API contract / module interaction tests |
| smoke | Build + basic functionality |
| manual | Generate checklist, label `[MANUAL VERIFICATION]` |
| none | Lint + build only |

## Mode / output_type matrix

| mode | flow |
|------|------|
| fast | PM plan → DE dev → TE light audit → SR2 → archive |
| standard | SA design → TE cases → DE → TE → SR2+SR3 |
| full | BA requirements + SA design + TE cases → SR1 → DE → SR2+SR3 → SR4 |

output_type (`android-app`, `backend-api`, `cli-tool`, `data-pipeline`, `infrastructure`, `documentation`, `game`, `library`, `custom`) drives UX deliverables and TE strategy. Orthogonal to mode.

## Recovery rules

- PM restores from `.state.md` + handoff files only → never from conversation history
- If handoff is `pending` and `last_updated` > 30 min, auto-redispatch
- `repair_round` increments each repair loop, resets to 0 on pass

## Constraints

- Never modify upstream artifacts (delivered handoffs, approved baselines)
- Handoff files are immutable → retry creates new file with round suffix
- Any file write must be verified (exists + non-empty)
- Delivery verdict = script exit code, not agent self-report
