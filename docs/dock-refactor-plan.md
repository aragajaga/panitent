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

### Phase 3. Introduce Pane Groups

Goal:
- distinguish pane-group semantics from raw split geometry.

Tasks:
- add explicit tool pane groups;
- add explicit document pane groups;
- make docking mutations operate on groups first, then derive split layout.

### Phase 4. Unify Floating Hosts by Policy

Goal:
- keep tool/document differences as docking policy, not as unrelated procedural flows.

Tasks:
- define a shared floating host contract;
- restrict document floating hosts to workspace/document targets by policy.

### Phase 5. Serialize Only the Model

Goal:
- rebuild runtime/view state from a saved model instead of serializing live HWND trees.

## First Increment Applied

This first step is intentionally small:
- explicit `DockNodeRole` metadata;
- explicit zone-side metadata;
- `dockpolicy` and part of `dockhost` now resolve semantics from typed roles first, then fall back to names for compatibility.

This does not replace the split-tree architecture yet, but it removes one of the biggest blockers to doing that safely.
