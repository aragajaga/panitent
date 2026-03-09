# Dock Polish Plan

This document is the current source of truth for polishing the Panit.ent dock system from the present architecture baseline.

It is intentionally different from `docs/dock-refactor-plan.md`:
- `dock-refactor-plan.md` is the historical refactor log;
- this file is the forward-looking execution plan.

Work on the dock system should follow this file unless a later document replaces it explicitly.

## 1. Current Baseline

The current system already has:
- a separated dock model layer;
- model-first docking mutations for supported tool and document docking flows;
- transactional named layout apply/reset with rollback coverage;
- persistence for main dock layout, floating tool layout, floating document arrangement, document session, and recovery;
- a decomposed dock host runtime instead of a single monolithic `dockhost.c`;
- runtime integration coverage for mixed tool/document/floating scenarios.

The current system is good enough to extend safely, but not yet polished enough to be treated like a mature docking framework.

## 2. Current Gaps

The remaining gaps are no longer "basic docking works" problems. They are polish and discipline problems:

1. Transactional parity is incomplete.
- Some direct restore helpers are already strict and rollback-safe.
- Others are still best-effort or only indirectly covered through `WindowLayoutManager`.

2. Runtime host architecture is much better, but still concentrated in a few large files.
- `dockhostruntime.c`
- `dockhostmodelapply.c`
- `floatingdocumenthost.c`

3. Windowing lifecycle is still weaker than the dock architecture.
- Ownership and teardown are safer than before, but still not as disciplined as the reference frameworks.

4. Performance and stress behavior are not yet first-class acceptance criteria.
- The system passes functional tests, but there is still no explicit leak/stress/perf gate.

5. UX parity with Visual Studio-style docking is incomplete.
- Most core behaviors exist.
- The remaining work is edge-case polish, not foundation work.

## 3. Definition Of Polished

The dock system should be considered "polished" only when all of the following are true:

1. All supported dock/layout/session commands are transactional.
- No partial live state after failure.
- No orphan floating windows.
- No broken layout/profile catalog state.

2. Every public restore/apply helper is idempotent.
- Repeated restore of the same model does not duplicate windows.
- Repeated apply of the same named layout does not drift.

3. Runtime identity is stable.
- Main workspace identity is preserved when possible.
- Floating reuse logic preserves live document/workspace state correctly.
- Known singleton tool views never duplicate.

4. Runtime tree invariants always hold after every successful operation.
- No empty structural nodes.
- No invalid active tab pointers.
- No illegal tool/document policy mixes.

5. Named layouts behave like a professional feature, not a serialization demo.
- Save is atomic.
- Apply is rollback-safe.
- Move/rename/delete are transactional.
- Reset is deterministic.

6. Direct helper APIs are as reliable as the menu commands that wrap them.

7. The system has an explicit stress bar.
- repeated apply/reset cycles
- repeated direct restore cycles
- floating tool/document churn
- no duplicate shells or leaked windows

## 4. Non-Goals

The following are not part of the dock polish target:
- document pixel/history persistence inside named layouts
- general app settings persistence
- brush/tool/theme state as part of named layout state
- rewriting the entire Win32 framework into a new UI toolkit
- cloning every Visual Studio visual detail pixel-for-pixel

## 5. Phase Plan

## Phase 1: Transactional Parity

Goal:
- every direct restore/apply helper that mutates live layout state must have strict rollback-safe behavior.

Required work:
- finish strict rollback semantics for remaining direct restore helpers;
- eliminate best-effort partial success where the contract should be all-or-nothing;
- keep helper-level behavior aligned with `WindowLayoutManager` orchestration semantics.

Acceptance criteria:
- helper-level restore paths have strict-mode rollback coverage;
- no direct helper leaves partial floating state on failure;
- runtime tests exist for active-state rollback, not just empty/default-state rollback.

## Phase 2: Runtime Identity And Reuse Discipline

Goal:
- live windows, workspaces, and floating hosts reuse correctly and predictably across repeated restore/apply cycles.

Required work:
- continue unifying reuse helpers for floating document and floating tool paths;
- remove remaining open-coded reuse/capture/restore plumbing;
- tighten node-id and workspace binding assumptions where order-based fallback still exists.

Acceptance criteria:
- repeated direct restore of layout/session helpers is idempotent;
- repeated named-layout apply is idempotent;
- reused workspaces preserve correct document state after restore;
- no duplicate floating shells appear under repeated apply/restore cycles.

## Phase 3: Runtime Host Consolidation

Goal:
- finish turning the dock runtime into coordinated subsystems instead of large runtime hubs.

Priority files:
- `src/dockhostruntime.c`
- `src/dockhostmodelapply.c`
- `src/floatingdocumenthost.c`

Required work:
- continue moving mixed responsibilities out of oversized runtime files;
- keep public APIs thin and explicit;
- reduce duplicated wiring between tool-side and document-side helpers.

Acceptance criteria:
- no new public behavior is introduced directly in `dockhost.c`;
- host/runtime layers are split by responsibility rather than by historical origin;
- remaining large files have a clear scope and no obvious duplicate logic blocks.

## Phase 4: Named Layout UX Hardening

Goal:
- make named layouts feel safe and predictable under all command flows.

Required work:
- complete runtime coverage for catalog/profile edge cases;
- harden manage-dialog operations against late save failures;
- ensure profile bundle/catalog consistency for all mutation commands.

Acceptance criteria:
- save/apply/reset/move/rename/delete all have late-failure coverage;
- no orphan profile bundles or broken catalog state;
- command-driven flows behave the same as direct helper flows.

## Phase 5: Stress And Leak Gates

Goal:
- make the dock system robust under repeated operations, not only under one-shot functional tests.

Required work:
- add repeated mixed-profile switching stress scenarios;
- add repeated direct helper restore stress scenarios;
- inspect remaining repeated-operation regressions and make them deterministic;
- optionally add lightweight leak checks or handle-count sanity checks around runtime tests.

Acceptance criteria:
- repeated mixed profile switching remains stable;
- repeated floating restore cycles remain stable;
- no duplicate windows or invalid tree states emerge after stress sequences.

## Phase 6: UX Polish Against Reference Behavior

Goal:
- close the remaining gap between "architecturally strong" and "feels polished in use".

Focus areas:
- auto-hide polish;
- dock guide behavior under edge cases;
- floating/document transition predictability;
- tab/group cleanup behavior;
- window layout UX consistency.

Acceptance criteria:
- no major behavioral mismatches remain in common Visual Studio-style workflows;
- the remaining differences are intentional, documented tradeoffs rather than bugs or drift.

## 6. Working Rules

When implementing against this plan:

1. Prefer one focused step per commit.

2. Each step should do one of:
- remove one duplication point;
- make one operation transactional;
- add one missing runtime invariant test;
- tighten one public helper contract.

3. Do not mix speculative architecture work with unrelated UI changes.

4. Every new public helper should have one clear owner layer.

5. If a new step makes the runtime test suite unstable, back it out or narrow it before moving on.

## 7. Immediate Backlog

The next recommended work items, in order:

1. Finish transactional parity for remaining direct floating document session restore paths.

2. Continue consolidating floating document/session restore plumbing into shared helper layers where duplication still exists.

3. Extend runtime stress coverage for repeated mixed profile switching and repeated direct helper restore cycles.

4. Continue shrinking the responsibility surface of `dockhostruntime.c` and `dockhostmodelapply.c`.

5. After the above, shift effort from architecture cleanup to UX polish and edge-case behavior.
