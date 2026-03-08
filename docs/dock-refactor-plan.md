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

This change set hardens the restore path with a validation/repair layer:
- added `dockmodelvalidate.*` for main-host layout validation before rebuild;
- repairs soft issues such as normalized names, pane kinds, captions, and invalid active-tab names;
- rejects hard-invalid layouts such as unknown persistent views, duplicate singleton views, or missing workspace.

This makes `docklayout.dat` far less brittle across refactors and gives the restore path a clear place to handle backward compatibility rules.
