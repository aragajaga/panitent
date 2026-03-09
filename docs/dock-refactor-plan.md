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

The current forward-looking polish plan is documented in:
- `docs/dock-polish-plan.md`

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
- follow-up cleanup removed the legacy live-tree fallback for tool docking itself, so tool-pane docking now relies on the model-first runtime path rather than keeping two active implementations.
- follow-up cleanup also added a generic model-first remove path for docked windows, so `dockhostmutate.*` no longer branches manually between tool/document removal before falling back to raw `DestroyWindow`.

## Document Group Model Increment Applied

This change set starts lifting document-group runtime/layout behavior toward the same model-first boundary:
- main-layout validation now accepts multiple `WorkspaceContainer` nodes instead of treating workspace as a singleton;
- `windowlayoutmanager` now preserves multiple live workspace windows during transactional apply/reset;
- `windowlayoutmanager` now matches preserved workspaces by model node identity instead of only traversal order, with a single-workspace fallback for default reset/apply;
- `dockmodelops.*` gained a pure workspace split operation around an anchor node;
- `dockhostmodelapply.*` gained a model-first runtime path for docking incoming document workspaces around an existing workspace;
- empty docked document-group cleanup now also routes through a model-first workspace removal path instead of direct live-tree surgery;
- undocking a docked document group to a floating window now preserves document floating policy instead of falling back to tool-window behavior;
- undocking a docked document group to a floating window now also routes through a transactional model-first helper, so a floating-host creation failure can roll the live dock layout back instead of leaving the group detached;
- shared document docking transitions now route through a common helper for center/side docking into target workspaces, reducing duplication between floating document and workspace flows;
- pure and runtime tests now cover multi-workspace layout validation and live document-workspace side docking.
- follow-up cleanup removed the remaining legacy live-tree fallback for document docking inside `dockhostmutate.*`, so supported document dock paths now go through the model-first runtime flow instead of a parallel tree-surgery implementation.
- follow-up cleanup also removed the remaining live-tree fallback for tool/root/local docking from `dockhostmutate.*`, leaving docking mutations themselves as model-first paths instead of a mixed implementation.

## Runtime Integration Test Increment Applied

This change set starts covering real runtime docking/layout behavior instead of only pure model and persistence round-trips:
- added a dedicated `panitent_runtime_tests` target;
- added hidden-host Win32 runtime scenarios in `tests/dockruntime_tests.c`;
- added coverage for model-first tool docking against a live `DockHostWindow`;
- added coverage for invalid layout apply rollback through `windowlayoutmanager`;
- added coverage for apply/reset preserving the live workspace window while changing tool arrangement.
- added coverage for named layout profile switching through real saved bundles (`A -> B -> A`).
- added coverage for mixed runtime apply with floating tool layout plus floating document arrangement.
- added coverage for named layout profile switching with mixed floating tool and floating document arrangement.

This does not finish the full runtime test matrix yet, but it turns integration/runtime testing into a real, executable part of the plan instead of a future-only requirement.
- runtime tests now also cover rollback when a document-group undock-to-floating transition fails during floating-host creation.
- runtime tests now also cover rollback when single-document float from a workspace fails during floating-host creation.
- workspace single-document float rollback now preserves original tab order and active document when floating-host creation fails.
- runtime tests now also cover rollback when `Apply Window Layout` fails during floating tool restore.
- runtime tests now also cover rollback when `Apply Window Layout` fails during floating document arrangement restore, not only during main layout validation.
- runtime tests now also cover rollback when floating document layout restore fails partway through a multi-entry target arrangement.
- runtime tests now also cover repeated application of the same mixed floating layout bundle to check idempotence and prevent duplicate floating windows.
- runtime tests now also cover repeated named-profile switching across mixed arrangements (`A -> B -> A -> B -> Reset`) to check profile-level idempotence and cleanup.
- runtime tests now also cover rollback when reapplying a mixed floating layout fails while the same mixed arrangement is already active.
- document-side runtime tests now also cover rollback for side-dock failure when a floating document host must merge workspaces before docking.
- document-side runtime tests now also cover the successful side-dock merge path for a floating document host with multiple workspaces.
- runtime tests now also cover menu-command application of saved named layouts, not only direct bundle-apply helpers.
- runtime tests now also cover menu-command reset from a mixed named-layout state, not only direct reset helpers.
- runtime tests now also cover menu-command apply failure rollback without modal UI by routing window-layout error reporting through a testable message sink.
- runtime tests now also cover menu-command apply failure rollback while a mixed floating layout is already active.
- runtime tests now also cover menu-command save and overwrite semantics for named layouts through a testable name-prompt hook.
- command-driven save now also has an explicit failure path: if profile bundle save fails, the catalog entry is not persisted, and runtime tests cover that path through a testable save hook.
- command-driven save now also covers the later failure path where bundle save succeeds but catalog persistence fails; runtime tests verify that the newly created bundle is removed instead of being orphaned.
- direct floating document layout restore is now also covered for repeated multi-workspace restore, not only via `WindowLayoutManager` apply paths.
- direct floating document layout restore tests now also verify that live documents already present in reused floating workspaces survive repeated layout-only restore.
- runtime tests now also cover repeated floating document session restore for both single-workspace and multi-workspace floating document hosts.
- floating tool restore is now treated as idempotent too, and runtime tests cover repeated direct `PanitentDockFloating_RestoreModel(...)` without duplicate floating tool windows.
- floating document session restore now also reuses/destroys existing live floating document workspaces through the same host helpers used by floating document layout restore, and runtime tests cover repeated session restore idempotence.

## Dock Host Render Increment Applied

This change set continues the structural decomposition of `dockhost.c`:
- moved split-rect/orientation helpers into `dockhostlayout.*`;
- added `dockhostpaint.*` for zone-tab hit-testing, split-grip rendering, and root/watermark paint content;
- `dockhost.c` now delegates more of its rendering/content path instead of mixing host orchestration with paint implementation.

## Dock Host Drag Increment Applied

This change set continues the same decomposition on the drag/runtime side:
- added `dockhostdrag.*` for dock-target guide hit-testing, drag overlay rendering, and undock-to-floating drag flow;
- `dockhost.c` now delegates caption-drag overlay and dock-target hit-testing through that module;
- the host file keeps message orchestration, while drag/overlay behavior is separated from the main host implementation.

## Dock Host Input Increment Applied

This change set continues the same decomposition on the input side:
- added `dockhostinput.*` for mouse/capture/context-menu interaction flow;
- moved caption button state management, split-drag interaction, caption-drag initiation, and inspector context-menu handling out of `dockhost.c`;
- `dockhost.c` now routes window messages to a dedicated input layer instead of owning those interaction details directly.
- follow-up cleanup removed the duplicated legacy input helpers from `dockhost.c`, leaving those interaction paths only in the extracted layer.

## Dock Host Auto-Hide Host Increment Applied

This change set continues the same decomposition on the auto-hide side:
- moved auto-hide overlay host creation, paint, and host-window procedure logic into `dockhostautohide.*`;
- `dockhost.c` no longer owns the overlay host window implementation details directly;
- auto-hide behavior now sits more coherently beside the rest of the overlay/layout logic instead of being split between modules.

## Dock Host Runtime Utility Increment Applied

This change set continues the same decomposition on the runtime utility side:
- added `dockhostruntime.*` for host content rect, pinning, pane-kind resolution, theme refresh, and layout tree teardown/preserve logic;
- `dockhost.c` no longer owns those runtime utility details directly;
- the host file is now closer to lifecycle/message coordination than to general-purpose dock runtime plumbing.
- follow-up cleanup also moved `DockData_*` rect helpers and `DockHostWindow_Rearrange()` behind the same runtime-oriented boundary, with dock-host metrics isolated from the host file.
- follow-up cleanup also moved `DockHostWindow_OnCreate`, `OnDestroy`, `OnPaint`, and `OnSize` into the runtime layer, so `dockhost.c` now mostly holds registration/init/message dispatch rather than host lifecycle implementation details.
- follow-up cleanup also moved `DockHostWindow_PreRegister`, `PreCreate`, `Init`, `Create`, and root accessors into the runtime layer, leaving `dockhost.c` as a much thinner dispatch facade.
- follow-up cleanup also moved `DockData_Create` and `DockNode_Create` into the runtime layer, so basic dock node construction is no longer split between the host facade and runtime support code.
- follow-up cleanup also moved `DockHostWindow_OnCommand` and `DockHostWindow_UserProc` into the runtime layer, leaving `dockhost.c` closer to a pure public wrapper file than an implementation hub.

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

## Floating Document Host Creation Increment Applied

This change set continues floating-host unification on the document side:
- added `floatingdocumenthost.*` as a shared creation path for floating document windows;
- `workspacecontainer`, document undock drag flow, and floating document restore paths now use the same helper instead of open-coded `FloatingWindowContainer` setup;
- floating document layout/session restore paths now also share a common pinned dock-host restore helper instead of duplicating `DockHostWindow` bootstrap and attach flow;
- floating document layout/session capture paths now also share a common child-layout capture helper instead of duplicating `DockModel` snapshot logic for workspace/document-host children;
- floating document layout/session capture and live-window filtering now also share a common enumeration/filter helper for pinned floating document windows;
- floating document layout/session capture paths now also share a common pinned-window state helper for window bounds, child layout capture, and workspace collection;
- main document session restore and floating document session workspace restore now also share a common workspace-entry capture/restore helper instead of duplicating active/non-active document open order and recovery/file entry logic;
- floating document layout restore now also uses shared live-workspace recycle/resolve/dispose helpers from `floatingdocumenthost.*` instead of keeping that workflow open-coded inside the layout persistence layer;
- floating document layout restore and floating document session restore now also share a common reused-workspace dock-host bootstrap helper instead of wiring `RestorePinnedDockHost(...)` separately at each call site;
- runtime tests now cover single-document float creation through that shared helper path.

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

## Document Dock Transition Increment Applied

This change set removes another document-side duplication point:
- added `documentdocktransition.*` as a shared helper for center-dock and side-dock transitions into target workspaces;
- added `floatingdocumentdock.*` so floating document source-context lookup, target hit-testing, and dock attempts no longer live as a separate open-coded branch inside `floatingwindowcontainer.c`;
- `WorkspaceContainer_TryDockFloating(...)` and floating document dock-back flows now route through the same document transition path.

This keeps document docking behavior consistent across workspace-driven and floating-document-driven flows without changing the existing overlay guide behavior.

## Dock Host Thin Facade Increment Applied

This change set removes the last thin public wrapper implementations from `dockhost.c`:
- dock hit-test entrypoints now live in `dockhostdrag.*`;
- dock mutate/remove entrypoints now live in `dockhostmutate.*`;
- `dockhost.c` is reduced further toward a minimal facade over specialized runtime layers.

This does not change public API shape, but it removes another leftover reason for `dockhost.c` to exist as a concrete implementation file instead of a thin coordinator.

## Floating Document Reuse Preparation Increment Applied

This change set centralizes preparation of live workspace reuse for floating document restore:
- added `FloatingDocumentHost_PrepareWorkspaceReuse(...)` to combine workspace collection and optional viewport clearing;
- `floatingdocumentlayoutpersist` now uses the same reuse preparation helper as `floatingdocumentsessionpersist`;
- session restore no longer open-codes viewport clearing for reused workspaces.

This keeps layout restore and session restore aligned around one workspace-reuse lifecycle entrypoint.

## Floating Tool Host Restore Coverage Increment Applied

This change set extends runtime coverage for direct floating tool restore:
- added an idempotence test for `FLOAT_DOCK_CHILD_TOOL_HOST` restore with multiple known panes;
- the runtime matrix now covers both direct floating tool panels and floating tool hosts;
- repeated direct restore must keep exactly one floating tool host and preserve its child pane layout.

This closes another restore-side blind spot between direct floating panel semantics and dock-host tool restore semantics.

## Floating Tool Host Helper Increment Applied

This change set introduces `floatingtoolhost.*` as the shared helper layer for floating tool windows:
- capture of pinned floating tool windows is centralized in `FloatingToolHost_CapturePinnedWindowState(...)`;
- direct restore of tool panels and tool hosts is centralized in `FloatingToolHost_RestoreEntry(...)`;
- `dockfloatingpersist.c` now uses shared enumeration, destruction, capture, and restore helpers instead of open-coded logic.

This makes tool-side floating persistence closer in structure to the existing `floatingdocumenthost.*` path.

## Strict Floating Tool Restore Increment Applied

This change set makes strict direct floating tool restore transactional:
- `PanitentDockFloating_RestoreModelEx(..., TRUE)` now captures the current floating tool state before restore;
- if a partial restore fails, existing floating tool windows are cleared and the previous floating tool state is restored;
- runtime coverage now includes partial-failure rollback from an already active floating tool state.

This closes another direct-helper transaction gap outside `WindowLayoutManager`.

## Strict Floating Document Layout Restore Increment Applied

This change set makes strict direct floating document layout restore transactional:
- `PanitentFloatingDocumentLayout_RestoreModelEx(..., TRUE)` now captures the current floating document arrangement before restore;
- if a partial restore fails, the helper restores the previous floating document arrangement instead of leaving a partial state behind;
- runtime coverage now includes partial-failure rollback from an already active floating document state with live document content.

This brings direct floating document layout restore in line with the stricter semantics already used for floating tool restore and layout-apply orchestration.

## Named Layout Catalog Operation Increment Applied

This change set extracts transactional catalog operations out of the manage-dialog code:
- added public, testable `WindowLayoutManager_MoveCatalogEntry(...)`, `WindowLayoutManager_RenameCatalogEntry(...)`, and `WindowLayoutManager_DeleteCatalogEntry(...)`;
- manage-dialog move/rename/delete now route through those operations instead of mutating and saving the catalog open-coded;
- delete now saves the updated catalog before removing profile bundle files, so save failure cannot leave a broken catalog/profile pair.

This reduces `windowlayoutmanager.c` UI coupling and closes another late-failure gap in named layout management.

## Named Layout Catalog Failure Coverage Increment Applied

This change set extends runtime coverage for transactional catalog operations:
- move rollback on `SaveCatalog` failure;
- rename rollback on `SaveCatalog` failure;
- delete rollback on `SaveCatalog` failure remains covered.

This keeps the whole named layout catalog mutation surface under explicit late-failure tests, not just the save/apply commands.

## Named Layout Catalog Success Coverage Increment Applied

This change set extends runtime coverage for the positive path of transactional catalog operations:
- successful move persists the new ordering;
- successful rename persists the new name;
- successful delete removes both the catalog entry and the profile bundle files.

This complements the existing late-failure coverage and keeps named layout catalog management covered on both success and rollback paths.

## Mixed Layout Stress Coverage Increment Applied

This change set starts Phase 5 stress coverage with a repeated mixed layout cycle test:
- repeatedly applies a mixed tool/document floating arrangement;
- repeatedly resets back to the default arrangement;
- verifies stable workspace identity, stable floating counts, and stable docked `GLWindow` placement across the cycle.

This adds an explicit repeated-operation gate instead of relying only on one-shot runtime scenarios.

## Mixed Profile Command Stress Increment Applied

This change set extends stress coverage from direct layout apply to public command-driven profile switching:
- repeatedly applies named profile `B` with mixed floating arrangement;
- repeatedly switches back to named profile `A`;
- finishes with `Reset Window Layout`;
- verifies stable workspace identity, stable floating counts, and stable `GLWindow` placement across the full cycle.

This strengthens Phase 5 using the actual command path that end users trigger, not only helper-level apply calls.

## Mixed Profile Failure Stress Increment Applied

This change set extends command-driven stress coverage with failure injection:
- repeatedly activates the mixed named profile;
- repeatedly re-applies the same mixed profile with a forced floating-document restore failure;
- verifies rollback to the already-active mixed state;
- then switches back to the default profile and repeats.

This adds a repeated failure-path gate on the real user-facing command path, not only on one-shot rollback scenarios.

## Direct Floating Helper Stress Increment Applied

This change set extends Phase 5 stress coverage to direct helper APIs:
- repeatedly applies direct floating tool restore;
- repeatedly applies direct floating document layout restore;
- resets back to the default layout after each cycle;
- verifies stable workspace identity, stable floating counts, and stable docked `GLWindow` placement.

This closes another gap between helper-level semantics and command-level semantics under repeated operation.

## Floating Document Create Layer Increment Applied

This change set splits the creation/bootstrap side of `floatingdocumenthost.c` into `floatingdocumentcreate.c`:
- pinned floating document window creation;
- pinned floating dock-host bootstrap;
- create-hook plumbing used by runtime tests.

This leaves `floatingdocumenthost.c` focused on restore wiring and reduces one more runtime hub in the floating document path.

## Floating Document Restore Layer Increment Applied

This change set moves the restore wiring side of `floatingdocumenthost.c` into `floatingdocumentrestore.c`:
- pinned floating dock-host restore;
- reuse-aware restore wrapper.

After this split, the floating document path is explicitly separated into create, reuse/capture, and restore layers.

## Floating Tool Capture Layer Increment Applied

This change set splits the enumeration/capture side of `floatingtoolhost.c` into `floatingtoolcapture.c`:
- pinned floating tool window enumeration;
- destroy-existing helper;
- pinned-window capture for direct panel and tool-host entries.

This leaves `floatingtoolhost.c` focused on restore wiring, mirroring the ongoing cleanup of the floating document path.

## Floating Tool Restore Layer Increment Applied

This change set moves the restore wiring side of `floatingtoolhost.c` into `floatingtoolrestore.c`:
- direct floating tool panel restore;
- floating tool host restore.

After this split, the tool-side floating path is explicitly separated into capture and restore layers.

## Dock Host Teardown Layer Increment Applied

This change set moves layout teardown and live tree destruction out of `dockhostruntime.c` into `dockhostteardown.c`:
- preserve-window filtering during teardown;
- recursive dock tree destruction;
- `DockHostWindow_ClearLayout(...)`.

This reduces the remaining responsibility surface of `dockhostruntime.c` and gives teardown its own explicit owner layer.
