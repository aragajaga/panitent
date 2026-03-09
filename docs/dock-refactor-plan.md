# Dock Refactor Plan

This roadmap turns the comparison with AvalonDock, DockPanelSuite, MFC/BCG, and Codejock into an incremental refactor plan for Panit.ent.

## Summary

Panit.ent already has the right UX direction:
- central workspace/document area;
- edge dock zones;
- floating tool windows;
- floating document windows;
- auto-hide overlays;
- Visual Studio-like dock guides.

The main architectural gap is different:
- one mutable split tree still mixes model meaning, runtime geometry, HWND ownership, and paint/hit-test behavior.

Reference systems separate those concerns much more aggressively.

## Named Layout Contract

Named window layouts now have an explicit architectural contract:
- they are arrangement-shell persistence, not document-content persistence;
- they should eventually include main dock layout, floating tool layout, floating document arrangement, and document group arrangement;
- they must stay separate from document session and recovery layers.

See:
- `docs/window-layout-contract.md`

The concrete contract and follow-up plan are documented in `docs/window-layout-contract.md`.

## Current Risks

The following risks are still considered active architectural debt and must be treated as part of the plan:

1. Layout apply/reset is not yet transactional enough.
- The current flow still mutates the live runtime tree during apply/reset.
- The target direction is: load -> validate -> build -> switch.
- If attach/build fails mid-flight, the old layout should still be recoverable.

2. `dockhost.c` is still too monolithic.
- It still mixes:
  - live tree ownership
  - geometry/layout
  - HWND ownership
  - paint
  - hit-test
  - docking mutations
  - auto-hide and floating glue

3. Runtime is not fully model-first yet.
- The project already has a good model/persistence boundary.
- But live editing still happens primarily against the mutable runtime tree, not against a pure layout model with materialization.

4. The base window framework is still low-discipline in lifecycle/ownership terms.
- The `PostQuitMessage` child-window bug was one concrete example.
- More lifecycle invariants should move out of ad-hoc per-window behavior.

5. Named layouts do not yet represent the full VS-like arrangement target.
- Current implementation covers main dock layout and floating tool layout.
- The intended target also includes floating document arrangement and document group arrangement.

6. Tests are still biased toward model/persistence layers.
- Unit coverage is already strong for snapshot/io/build/validate/session round-trip.
- Coverage is still weak for runtime mutation flows and menu-driven layout switching.

## Mandatory Next Steps

The next mandatory architecture steps are:

1. Make layout apply/reset transactional.
- Build and validate the target runtime layout before switching.
- Add rollback behavior if attach/switch fails.

2. Decompose `dockhost.c`.
- At minimum split out:
  - layout engine
  - paint/render
  - docking mutations
  - auto-hide runtime

3. Push runtime toward model-first editing.
- Not only save/load through the model.
- Runtime docking mutations should increasingly be expressed as model operations followed by materialization/update.

4. Finish the named layout scope.
- Include floating document host arrangement.
- Include document group arrangement.
- Keep document session and recovery as separate persistence layers.

5. Add integration/runtime tests.
- Rearrange panels.
- Save layout A.
- Save layout B.
- Apply A.
- Apply B.
- Reset layout.
- Verify runtime tree invariants after each step.

## Model-First Increment Applied

This change set starts introducing a pure model mutation layer:
- added `dockmodelops.*` for clone/find/remove/zone-append operations on `DockModelNode`;
- expanded the layer with pure panel-split docking around an anchor node;
- expanded the layer with pure root-side docking that can create or use edge zones in model space;
- added unit tests for model mutation behavior;
- this is groundwork for moving runtime docking mutations toward model-first editing instead of direct live-tree surgery everywhere.

## Default Model Increment Applied

This change set introduces a pure default layout model:
- added `dockdefaultlayoutmodel.*` as a canonical builder for the default main shell in `DockModelNode` form;
- `Reset Window Layout` now uses a model-first default layout path instead of building a runtime tree and snapshotting it back into a model;
- added tests that validate the default model contains the expected workspace and core tool views.

## Runtime Model-Apply Increment Applied

This change set starts using the pure model mutation layer in a live runtime path:
- added `dockhostmodelapply.*`;
- tool-pane docking in `dockhostmutate.*` now attempts a model-first mutation/apply path before falling back to the legacy live-tree path;
- tool-pane remove/undock flows now also start using the model-first runtime apply path where possible;
- this gives the project its first rollback-capable runtime docking flow that is driven by `DockModelOps` instead of only direct tree surgery.

## Runtime Integration Test Increment Applied

This change set starts covering real runtime docking/layout behavior instead of only pure model and persistence round-trips:
- added a dedicated `panitent_runtime_tests` target;
- added hidden-host Win32 runtime scenarios in `tests/dockruntime_tests.c`;
- added coverage for model-first tool docking against a live `DockHostWindow`;
- added coverage for invalid layout apply rollback through `windowlayoutmanager`;
- added coverage for apply/reset preserving the live workspace window while changing tool arrangement.

This does not finish the full runtime test matrix yet, but it turns integration/runtime testing into a real, executable part of the plan instead of a future-only requirement.

## Target Layers

### 1. Layout Model

Pure semantic model:
- `DockLayoutRoot`
- `DockSideSet`
- `ToolPaneGroup`
- `DocumentPaneGroup`
- `SplitNode`
- `FloatingHost`
- `AutoHideStrip`

### 2. Runtime Layout State

Transient state only:
- split ratios;
- active tab state;
- collapsed/auto-hide state;
- drag-preview state;
- cached overlay visuals.

### 3. Host/View Layer

Win32 hosts that render the model:
- main dock host;
- floating tool host;
- floating document host;
- auto-hide popup host.

## Refactor Phases

### Phase 1. Remove Name-Based Semantics

Goal:
- stop using `lpszName` as the real docking model.

Tasks:
- add explicit node roles;
- add explicit zone-side metadata;
- move policy checks from string prefixes to typed role checks;
- keep name-based fallback only as a transition layer.

Status:
- started in this change set.

### Phase 2. Isolate Shell Topology

Goal:
- move hard-coded shell construction out of `PanitentApp_DockHostInit(...)` into a dedicated shell builder/model module.

Tasks:
- describe root, zones, and workspace through typed constructors;
- stop treating `DockShell.*` names as implicit structure.

Status:
- started in this change set.

### Phase 3. Introduce Pane Groups

Goal:
- distinguish pane-group semantics from raw split geometry.

Tasks:
- add explicit tool pane groups;
- add explicit document pane groups;
- make docking mutations operate on groups first, then derive split layout.

Status:
- started in this change set.

### Phase 4. Unify Floating Hosts by Policy

Goal:
- keep tool/document differences as docking policy, not as unrelated procedural flows.

Tasks:
- define a shared floating host contract;
- restrict document floating hosts to workspace/document targets by policy.

Status:
- started in this change set.

### Phase 5. Serialize Only the Model

Goal:
- rebuild runtime/view state from a saved model instead of serializing live HWND trees.

Status:
- started in this change set.

## First Increment Applied

This first step is intentionally small:
- explicit `DockNodeRole` metadata;
- explicit zone-side metadata;
- `dockpolicy` and part of `dockhost` now resolve semantics from typed roles first, then fall back to names for compatibility.

This does not replace the split-tree architecture yet, but it removes one of the biggest blockers to doing that safely.

## Second Increment Applied

This change set starts Phase 2:
- added a dedicated `dockshell.*` module;
- moved root/zone/workspace typed constructors into that module;
- moved shell topology assembly out of `PanitentApp_DockHostInit(...)`;
- reused the same root/workspace builder for local document dock hosts.

This still uses the existing split tree internally, but the shell topology is now explicit and no longer hand-built inline inside app startup code.

## Third Increment Applied

This change set starts Phase 3:
- added a dedicated `dockgroup.*` semantic layer;
- introduced explicit `tool` vs `document` pane kind metadata;
- introduced explicit `tool pane group` vs `document pane group` detection;
- switched part of local docking policy to group-based checks instead of ad-hoc anchor handling.

This still is not the final typed layout model, but docking rules are now beginning to depend on explicit pane/group semantics instead of only tree shape.

## Fourth Increment Applied

This change set starts Phase 4:
- added a dedicated `floatingdockpolicy.*` module;
- added explicit floating child-host kinds;
- started classifying floating children through policy instead of open-coded class-name branches;
- moved dock-command eligibility and part of floating document/tool decisions into policy helpers.

This does not finish floating-host unification yet, but it creates a shared semantic layer for the next step.

## Fifth Increment Applied

This change set continues Phase 4:
- added a dedicated `floatingchildhost.*` adapter module;
- moved floating child-host classification and document-host operations out of `floatingwindowcontainer.c`;
- routed document source resolution, document move/merge logic, and target workspace dock-host wrapping through the adapter.

The floating container still owns the UI flow, but it no longer needs to know as much about the internal structure of every possible child host.

## Sixth Increment Applied

This change set starts Phase 5:
- added a dedicated `dockmodel.*` snapshot module;
- introduced capture of the live dock tree into a pure model without `HWND` or `RECT`;
- kept semantic data in the snapshot: node role, pane kind, dock side, split metadata, collapsed state, and active-tab name.

This is not wired into settings persistence yet, but it establishes the model boundary required to serialize layout without depending on live runtime handles.

## Seventh Increment Applied

This change set continues Phase 5:
- added a dedicated `dockmodelio.*` round-trip module;
- introduced versioned binary file serialization for the pure dock model;
- added round-trip tests that validate `model -> file -> model` without involving live Win32 handles.

This still is not connected to settings persistence or runtime rebuild, but it completes the first half of Phase 5: the layout model can now exist outside the running dock host.

## Eighth Increment Applied

This change set continues Phase 5:
- added a dedicated `dockmodelbuild.*` rebuild module;
- introduced `dockmodel -> TreeNode/DockData` reconstruction without using live Win32 windows;
- added tests for `model -> tree -> model` round-trip, including active-tab restoration semantics through synthetic handles.

At this point the project has both halves of the layout boundary:
- live dock tree -> pure model
- pure model -> runtime tree

The next step is to wire that boundary into actual layout persistence and restore.

## Ninth Increment Applied

This change set wires Phase 5 into the application lifecycle for the main dock host:
- added `docklayoutpersist.*` as an app-level persistence layer;
- saves the main dock host layout to `docklayout.dat` on shutdown;
- restores the main dock host layout on startup with fallback to the default shell when restore fails.

Current limitation:
- this step restores the main dock host structure and known tool/workspace windows;
- it does not yet restore floating windows or document contents across sessions.

## Tenth Increment Applied

This change set removes duplication between default shell bootstrap and layout restore:
- added `dockviewcatalog.*` for pure mapping from `(role, name)` to known persistent views;
- added `dockviewfactory.*` for actual node/window creation of those views;
- switched both startup shell construction and layout restore to the same factory path.

This does not yet restore floating windows or document contents, but it removes one of the main blockers to extending persistence beyond the main dock host.

## Eleventh Increment Applied

This change set adds file-backed document session persistence for the main workspace:
- added `documentsessionmodel.*` for versioned session-file round-trip;
- added `documentsessionpersist.*` for app-level save/restore of file-backed documents;
- added document/workspace APIs needed to reopen documents into a target workspace instead of only the default one.

Current limitation:
- file-backed, dirty file-backed, and recovered unsaved documents in the main workspace are restored;
- dirty file-backed documents now restore from recovery snapshots that preserve their original file path.

## Twelfth Increment Applied

This change set extends document persistence to floating document windows:
- added `floatingdocumentsessionmodel.*` for versioned floating-document session round-trip;
- added `floatingdocumentsessionpersist.*` for app-level save/restore of file-backed floating document sessions;
- restores file-backed floating document sessions into new floating document windows after the main workspace is restored.

Current limitation:
- floating document sessions now preserve workspace split structure inside a floating document host;
- nested known tool panes inside floating document hosts are restored through the shared dock-host restore path;
- unsaved and dirty file-backed floating documents are restored through recovery snapshots.

## Thirteenth Increment Applied

This change set adds a dedicated cleanup policy for recovery snapshots:
- added `recoverystore.*` as a pure file cleanup helper;
- added `recoverystorepersist.*` as the app-level AppData wrapper;
- main and floating document session save paths now clear stale `recovery_*.pdr` files before writing a new session snapshot.

Current limitation:
- recovery files are cleaned on save and orphaned recovery files are garbage-collected on startup based on live session references;
- there is still no age-based retention policy beyond current file references and naming patterns.

## Fourteenth Increment Applied

This change set adds startup garbage collection for orphaned recovery snapshots:
- added `recoverystoregc.*` as a startup-time GC pass;
- collects referenced recovery files from `documentsession.dat` and `floatingdocumentsession.dat`;
- deletes only unreferenced `recovery_*.pdr` files after restore completes.

This closes the main recovery lifecycle loop: create -> reference from session -> restore -> garbage collect when orphaned.

## Sixteenth Increment Applied

This change set adds age-based retention to recovery garbage collection:
- startup GC now keeps recent orphaned recovery files for a retention window instead of deleting everything immediately;
- only unreferenced recovery snapshots older than the retention threshold are deleted;
- this makes crash recovery less fragile when session files are temporarily missing or invalid.

Current limitation:
- retention is fixed-window based, not user-configurable;
- persisted file formats still reset on incompatibility instead of migrating forward.

## Fifteenth Increment Applied

This change set adds explicit load-status handling for persisted files:
- model loaders now distinguish `missing`, `invalid format`, and `io error`;
- restore paths automatically delete incompatible persisted files instead of repeatedly retrying broken data forever;
- this creates a concrete migration/reset policy for future file-format changes.

Current limitation:
- incompatible files are reset, not migrated;
- there is still no multi-version upgrade path beyond format rejection and fallback.

## Seventeenth Increment Applied

This change set adds real backward-compatible loading for legacy session formats:
- `documentsession.dat`, `dockfloating.dat`, and `floatingdocumentsession.dat` now write `v2`;
- loaders still understand legacy `v1` payloads and upgrade them into the current in-memory model;
- migration tests cover `v1 -> current model` for all three session formats.

This replaces simple rejection with an actual compatibility path for the most important persisted files.

## Eighteenth Increment Applied

This change set adds stable model node identity to the dock model boundary:
- `dockmodel.*`, `dockmodelio.*`, and `dockmodelbuild.*` now preserve persistent `node id`;
- floating document workspace sessions bind to workspace layout nodes by id instead of traversal order;
- `docklayout.dat` now writes `v2` node-aware payloads while still loading legacy `v1`.

This removes one of the more fragile assumptions from floating document restore: workspace matching is now structural, not incidental.

## Nineteenth Increment Applied

This change set upgrades invalid-file handling from delete-only to quarantine:
- added `persistfile.*` to quarantine incompatible persisted files next to the original path;
- restore paths now move bad files to `*.invalid.<timestamp>.bak` before falling back;
- the restore pipeline is now: load, migrate if possible, quarantine if not.

This keeps invalid persisted files available for diagnosis instead of silently discarding them.

## Twelfth Increment Applied

This change set starts extending persistence beyond the main dock host:
- added `dockfloatingpersist.*` for known floating tool panes;
- introduced a separate `dockfloating.dat` file with versioned floating-window entries;
- restores floating tool sessions after the main dock host is restored.

Current limitation:
- floating tool sessions now preserve direct panes and dock-host tool layouts;
- unsaved documents are still not restored.

## Eleventh Increment Applied

This change set hardens the restore path with a validation/repair layer:
- added `dockmodelvalidate.*` for main-host layout validation before rebuild;
- repairs soft issues such as normalized names, pane kinds, captions, and invalid active-tab names;
- rejects hard-invalid layouts such as unknown persistent views, duplicate singleton views, or missing workspace.

This makes `docklayout.dat` far less brittle across refactors and gives the restore path a clear place to handle backward compatibility rules.
