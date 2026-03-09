# Window Layout Contract

This document fixes the scope and architectural contract for named window layouts in Panit.ent.

Future work on docking, layout persistence, and window layout UX should follow this document.

## 1. Scope

Named window layouts are for window arrangement, not for document content persistence.

They should model the Visual Studio style split between:
- window layout
- document session
- recovery state

## 2. Named Layout Includes

A named window layout must include:
- main dock host layout
- edge dock zones
- split ratios
- tab groups
- active tabs
- collapsed and auto-hide state
- floating tool windows
- floating tool host layout
- floating document host arrangement
- document group arrangement
- active document group per host
- floating host bounds

In practical terms, named layouts are responsible for the arrangement shell.

## 3. Named Layout Excludes

A named window layout must not include:
- document pixel content
- history stack
- brush state
- current selected tool
- theme settings
- custom frame settings
- compact menu settings
- general application preferences
- recovery payload blobs

These belong to other persistence layers.

## 4. Separate Persistence Layers

Panit.ent should keep these layers separate:

1. Window Layout
- arrangement only

2. Document Session
- which documents are open
- which document is active
- file-backed vs recovery-backed document entries

3. Recovery
- unsaved and dirty payload snapshots

This separation is mandatory. Named layouts must not become a mixed session dump.

## 5. Runtime Apply Contract

Applying a named layout must:
- preserve the live workspace window when possible
- preserve currently open documents unless the command explicitly targets session restore
- rebuild tool window arrangement without terminating the app
- avoid destructive first-step mutations when rollback is still possible

The target architecture is transactional:
- load and validate the target layout first
- build the runtime tree first
- switch only after the new layout is ready

If that is not yet fully implemented, every step should move toward it.

## 6. Reset Window Layout Contract

Reset Window Layout means:
- rebuild the standard default window arrangement
- keep the live workspace window
- keep current document session
- recreate tool window arrangement as if the app started with the default shell

Reset is a layout command, not a document/session wipe.

## 7. Current Implementation Boundary

Current named layout support is allowed to ship in stages:

Stage 1:
- main dock layout
- floating tool layout

Stage 2:
- floating document host arrangement
- document group arrangement

Stage 3:
- transactional apply with rollback

The intended target is Stage 2 plus Stage 3.

## 8. Next Plan

The next planned architecture steps are:

1. Extend named layouts to include floating document host arrangement
- persist floating document host shell separately from document session payload

2. Extend named layouts to include document group arrangement
- persist tab groups and active groups as layout-only state

3. Make apply/reset transactional
- prepare the target layout before destroying the current one
- add rollback path on attach failure

4. Reduce direct mutation logic in dock host
- move more rebuild/apply logic out of `dockhost.c`

5. Add integration tests
- save layout A
- save layout B
- apply A
- apply B
- reset
- verify arrangement invariants

## 9. Decision

Until this document is explicitly replaced, Panit.ent should treat named layouts as:
- arrangement shell persistence
- separate from document session persistence
- separate from recovery persistence
