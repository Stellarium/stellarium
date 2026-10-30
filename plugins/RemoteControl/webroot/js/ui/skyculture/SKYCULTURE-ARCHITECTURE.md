# SKYCULTURE-ARCHITECTURE.md

Below is the complete, production-grade architecture documentation for the modular Sky Culture subsystem. It is written in English, matches the actual files you now have, and is structured so that future maintainers, translators, and reviewers can understand the module without reading every line of code.

I deliberately kept the *code* references accurate (function names, file names, IDs, event names, endpoints) so this document can be verified against the codebase line-by-line.

---

```markdown
# Sky Culture Module — Architecture & Developer Guide

> Stellarium Remote Control · Sky Culture Subsystem  
> Version: 5.0.0 (modular)  
> License: GPLv2+  
> Author: kutaibaa akraa (GitHub: @kutaibaa-akraa)

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Design Goals](#2-design-goals)
3. [Module Map](#3-module-map)
4. [Layered Architecture](#4-layered-architecture)
5. [Data Flow — From Server to Screen](#5-data-flow--from-server-to-screen)
6. [Work Flow — User Interaction Lifecycle](#6-work-flow--user-interaction-lifecycle)
7. [Multi-Select Lifecycle](#7-multi-select-lifecycle)
8. [Visibility Filter and Countdown](#8-visibility-filter-and-countdown)
9. [Tour Script Generation](#9-tour-script-generation)
10. [API Endpoints Used](#10-api-endpoints-used)
11. [Centralized State](#11-centralized-state)
12. [Filtering, Sorting, Grouping](#12-filtering-sorting-grouping)
13. [Rendering](#13-rendering)
14. [Synchronization (Bidirectional)](#14-synchronization-bidirectional)
15. [Help System](#15-help-system)
16. [Internationalization (i18n)](#16-internationalization-i18n)
17. [Error Handling and Edge Cases](#17-error-handling-and-edge-cases)
18. [Performance Notes](#18-performance-notes)
19. [Extension Guide](#19-extension-guide)
20. [File-by-File Reference](#20-file-by-file-reference)
21. [Appendix A — Public API](#appendix-a--public-api)
22. [Appendix B — ID Contract](#appendix-b--id-contract)
23. [Appendix C — Glossary](#appendix-c--glossary)

---

## 1. Purpose and Scope

The Sky Culture module is the interactive UI inside Stellarium Remote Control that lets a user:

- Browse the sky cultures installed in Stellarium.
- Explore the constellations, asterisms, zodiac signs, lunar mansions, and named stars of the current culture.
- Navigate Stellarium's sky view to any of those objects.
- Filter, sort, and group them.
- Select multiple items and generate ready-to-run **Stellarium Script (`.ssc`)** tours.
- See live documentation of the observer's location, time, and horizon snapshot when a visibility filter is applied.

This document is a **developer-facing** reference. A separate **User Guide** (LaTeX) describes the same features from an end-user point of view; the two documents are intentionally complementary and do not duplicate each other.

---

## 2. Design Goals

The module was refactored from a single ~8900-line file into a set of focused sub-modules. The refactor was guided by the following principles:

| Goal | Meaning |
|------|---------|
| **Single source of truth** | Culture data comes from `index.json`. UI state comes from `skyculture-state.js`. No duplicate caches. |
| **Layered pipeline** | `config → state → api → data → filters → render`. Each layer has one reason to change. |
| **Unified abstractions** | One `fetchObjectInfo`, one `applyAll`, one multi-select engine, one sync normalizer. |
| **Predictable state** | All mutable state lives in `State.raw`. Sub-modules read from it, they do not own parallel state. |
| **No hidden globals** | DOM references are cached explicitly; nothing relies on document-wide selectors except for the documented ID contract. |
| **i18n friendly** | Every user-facing string goes through `rc.tr()`. No string concatenation inside `tr()`. |
| **Testable in isolation** | Each sub-module is a RequireJS module with an explicit public API. |
| **Safe by default** | Every async operation has `.catch`. Every `undefined` field has a fallback. Every visible filter has a manual override. |

---

## 3. Module Map

```
js/skyculture/
├── skyculture.js               Main entry point — init, public API, orchestration
├── skyculture-config.js        Constants: endpoints, FOV defaults, TTLs, limits
├── skyculture-state.js         Centralized mutable state container
├── skyculture-api.js           Unified AJAX layer (fetch, batch, timeout, cache)
├── skyculture-data.js          index.json extraction (constellations, stars, ...)
├── skyculture-filters.js       Filter / sort / group pipeline (pure functions)
├── skyculture-render.js        DOM rendering of button grids, counters, modals
├── skyculture-multiselect.js   Unified multi-select engine (4 types)
├── skyculture-sync.js          Bidirectional sync with Stellarium selection
├── skyculture-artwork.js       Artwork panel (grid + modal + sync)
├── skyculture-tours.js         SSC script generators + tour toolbars
├── skyculture-help.js          Control panels + help modals
└── docs/
    └── SKYCULTURE-ARCHITECTURE.md    This file
```

### Dependency graph

```
                      ┌──────────────────┐
                      │  skyculture.js   │  (entry)
                      └────────┬─────────┘
        ┌──────────────────────┼───────────────────────┐
        ▼                      ▼                       ▼
  ┌──────────┐          ┌──────────┐            ┌──────────┐
  │  config  │◄─────────│  state   │───────────►│  api     │
  └────┬─────┘          └────┬─────┘            └────┬─────┘
       │                     │                       │
       │                     ▼                       ▼
       │               ┌──────────┐            ┌──────────┐
       │               │  data    │            │  filters │
       │               └────┬─────┘            └────┬─────┘
       │                    │                       │
       │                    ▼                       ▼
       │               ┌──────────┐            ┌──────────┐
       │               │  render  │◄───────────│  sync    │
       │               └────┬─────┘            └────┬─────┘
       │                    │                       │
       │                    ▼                       │
       │               ┌──────────┐                 │
       │               │multi-select◄──────────────┘
       │               └────┬─────┘
       │                    │
       ▼                    ▼
  ┌──────────┐        ┌──────────┐
  │  tours   │◄───────│ artwork  │
  └────┬─────┘        └──────────┘
       │
       ▼
  ┌──────────┐
  │   help   │
  └──────────┘
```

**Rules:**
- `config` depends on nothing.
- `state` depends on `config` only.
- `api` depends on `config` and `state`.
- `data`, `filters`, `render` depend on `config` and `state` only.
- `sync`, `multiselect`, `artwork`, `tours`, `help` may depend on any of the above.
- **`skyculture.js` depends on all of them** but is the only module that calls them together in a workflow.

There are no circular dependencies.

---

## 4. Layered Architecture

Each layer answers exactly one question.

| Layer | Module | Question it answers |
|-------|--------|---------------------|
| **Configuration** | `skyculture-config.js` | "What are the tunable constants?" |
| **State** | `skyculture-state.js` | "What does the user currently have selected / filtered / loaded?" |
| **Transport** | `skyculture-api.js` | "How do we talk to the server?" |
| **Extraction** | `skyculture-data.js` | "What does `index.json` mean in our terms?" |
| **Transformation** | `skyculture-filters.js` | "Given data + state, what should be displayed?" |
| **Presentation** | `skyculture-render.js` | "How do we turn a pattern into a button / image / badge?" |
| **Interaction** | `skyculture-multiselect.js`, `skyculture-sync.js` | "How does the user select and how do we mirror Stellarium's state?" |
| **Domain features** | `skyculture-artwork.js`, `skyculture-tours.js`, `skyculture-help.js` | "Artwork panel, tour generation, help dialogs." |
| **Orchestration** | `skyculture.js` | "In what order do all of the above run?" |

The **orchestrator never implements business logic.** It calls the layers, in the correct order, with the correct arguments.

---

## 5. Data Flow — From Server to Screen

This section describes the *data pipeline*. It is the same for every type (constellations, asterisms, zodiac, lunar, stars, artwork), with a single branch point at the multi-select step (§7).

### 5.1 High level

```
┌─────────────────────────────┐
│ User selects a sky culture  │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ setCulture(cultureId)       │   ← skyculture.js
│  • Save new id in State     │
│  • Tell Stellarium server   │
│  • Reload all culture data  │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ loadAllCultureData()        │   ← skyculture.js
│  • Show loading placeholders│
│  • Clear caches & state     │
│  • api.fetchIndexJson()     │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ data.extract*()             │   ← skyculture-data.js
│  • extractConstellations    │
│  • extractAsterisms         │
│  • extractZodiac            │
│  • extractLunarMansions     │
│  • extractStars             │
│  • extractArtwork           │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ Store in State              │   ← skyculture-state.js
│  State.raw.data.*           │
│  State.raw.original.*       │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────┐
│ render*Panel()              │   ← skyculture.js → render panel
└──────────────┬──────────────┘
               │
               ▼
        (see §5.2 below)
```

### 5.2 Per-panel pipeline (`renderPanel`)

Each panel is rendered through the **same unified function**, which decides whether an API fetch is required.

```
renderPanel(opts)
   │
   ├── Empty state? → renderPlaceholder()
   │
   ├── needsFetch(patterns, filterState, type)?
   │       │
   │       ├── NO  → buildInfoMapFromCache()           [fast path]
   │       │          → filters.applyAll()
   │       │          → renderResult()
   │       │
   │       └── YES → api.fetchVisibilityBatch()
   │                   → cacheInfoOnPatterns()
   │                   → filters.applyAll()
   │                   → renderResult()
   │
   └── sync.rebuildIndex()   (after every render)
```

**`renderResult()`** chooses between `render.renderPatternGrid()` (flat) and `render.renderGroupedStars()` (grouped, stars only), and attaches multi-select handlers if the mode is active.

### 5.3 When is a fetch required?

`needsFetch()` implements a **lazy default** strategy:

| Condition | Result |
|-----------|--------|
| No `_info` at all **and** filter state is fully default | `fetch: false` — use `pattern.name` directly. |
| `_info` missing for one pattern | `fetch: true` — full fetch. |
| Visibility filter enabled and `_aboveHorizonAt` is zero | `fetch: true` — manual invalidation (Refresh button). |
| Visibility filter enabled and `_aboveHorizonAt` older than TTL | `fetch: true` — stale data. |
| A field required by the current filter is missing | `fetch: true` — e.g. user just enabled "Sort by magnitude". |
| All required fields cached | `fetch: false` — instant. |

**Consequence:** the first switch to a non-default filter triggers one batch fetch. Every subsequent toggle of the same filter is instant.

---

## 6. Work Flow — User Interaction Lifecycle

### 6.1 Selecting a culture

```
User clicks a culture button
  ↓
render.renderCultureButtons() captures click
  ↓
skyculture.setCulture(id)
  ↓
1. Update State.raw.culture.id
2. propApi.setStelProp('StelSkyCultureMgr.currentSkyCultureID', id)
3. Clear selections (State.clearSelection, MultiSelect.clearAll)
4. Update info iframe with ?culture=id&t=Date.now()
5. Reload all culture data (loadAllCultureData)
```

**Why update local state first?** The `stelPropertyChanged` listener fires when the server echoes the change back. If we didn't update `State.raw.culture.id` before calling `setStelProp`, the listener would trigger a redundant reload.

### 6.2 Clicking a pattern (single-select mode)

```
User clicks a pattern button
  ↓
render.renderPatternGrid attaches click handler
  ↓
onClick({ id, name, searchName, type })
  ↓
handler in skyculture.js:
   - if active → clear selection, clear buttons, clear Stellarium highlight
   - else      → State.setSelection(...), highlight button, call stelUtils.goToX()
  ↓
stelUtils emits 'objectSelected' event
  ↓
sync.onObjectSelected() normalizes the event
  ↓
highlights matching button(s), including artwork
```

### 6.3 Filter changes

```
User toggles "Visible above horizon only"
  ↓
Help.installControlPanel binds change → controlHandlers.onFilterChange
  ↓
State.raw.filters[type].visible = value
  ↓
syncVisibilityTimer()            (§8)
  ↓
refreshAllVisibilitySnapshots()  (§8.4)
  ↓
reRenderPanelForType(type)
  ↓
renderPanel() → needsFetch() → (maybe) fetch → render
```

### 6.4 Sort / name-mode changes

```
User changes "Sort by" + clicks Apply
  ↓
controlHandlers.onSortChange + onApplySort
  ↓
State.raw.filters[type].sortBy = ...
  ↓
reRenderPanelForType(type)
  ↓
renderPanel() → filters.applyAll() → renderResult()
```

Sorting is **instant** after the first fetch, because `needsFetch()` sees the cache is complete.

---

## 7. Multi-Select Lifecycle

Multi-select is the single most complex interaction in the module. It is implemented once in `skyculture-multiselect.js` and driven by a single `TYPE_CONFIG` table.

### 7.1 Type configuration

```
TYPE_CONFIG = {
  constellation: { containerKey, dataKey, filterKey, cssClass, idPrefix, generateBtnId },
  asterism:      { ... },
  lunar:         { ... },
  star:          { ... }
}
```

The four types share the exact same engine. Adding a fifth type would only require a new entry in `TYPE_CONFIG` and a matching `<div>` in the HTML.

### 7.2 Enter / exit

```
User checks "Multi-Select" in a tab
  ↓
controlHandlers.onMultiSelectChange(type, true)
  ↓
MultiSelect.enter(type)
  ├── clear previous type (if any)
  ├── State.raw.multiSelect.active = true
  ├── State.raw.multiSelect.mode   = type
  └── updateSelectionCount(type)
  ↓
reRenderPanelForType(type)
  ↓
renderResult detects MultiSelect.isActive(type) → attaches toggle handlers
```

Exit is the symmetric call `MultiSelect.exit(type)`, which clears the set and restores the normal click handler on the next render.

### 7.3 Toggle

```
User clicks a button in multi-select mode
  ↓
MultiSelect.attachHandlers → toggle(type, id, $btn)
  ├── if set.has(id) → set.delete(id); removeSelectedStyle()
  └── else           → set.add(id);    applySelectedStyle()
  ↓
updateSelectionCount(type)
  ├── updates "N selected" badge
  ├── disables generate button if count === 0
  └── syncs dynamic-constellation button (§7.4)
```

### 7.4 Constellation dynamic button lock

The Constellations tab has two generate buttons:

- `#btn-generate-const-static` — honors multi-select.
- `#btn-generate-const-dynamic` — **does not** honor multi-select (it fetches all constellations from Stellarium at runtime).

To avoid user confusion, `syncDynamicButtonState()` **disables** the dynamic button while multi-select is active, and **restores** it when multi-select is turned off. The original `title` is captured once and restored verbatim.

### 7.5 Selection preservation across re-renders

Every re-render (filter change, sort, name-mode) calls `saveState(type)` before and `restoreState(type, savedState)` after. This keeps the multi-select set intact and re-applies the CSS classes to the re-rendered buttons.

### 7.6 Order preservation in generated scripts

`getSelectedPatterns(type)` returns the selected patterns **in the on-screen order**, not in `State.raw.data` order. This is what guarantees that a generated script lists items in the same sequence the user sees them, even after sorting by magnitude or grouping by constellation.

The mechanism is:

1. `renderPanel` stores the last filtered + sorted result in `State.raw.displayed[typeKey]`.
2. `getSelectedPatterns` reads `displayed`, filters by `Set.has(id)`, and returns the result.
3. If `displayed` is empty (e.g. user selected before the first render finished), it falls back to `Filters.applyAll(State.raw.data, filterState, ...)`.

---

## 8. Visibility Filter and Countdown

The visibility filter is the only filter that depends on time and location. It is therefore the only one that requires an auto-refresh mechanism.

### 8.1 Two timers

| Timer | Owner | Period | Purpose |
|-------|-------|--------|---------|
| `visibilityTimer` | `skyculture.js` | TTL (60s / 180s / 420s) | Invalidates `_aboveHorizonAt` on patterns and re-renders. |
| `visibilityCountdownTimer` | `skyculture.js` | 1 s | Updates the "⏱ N s" display only. |

They are separate so the countdown can update every second without triggering a fetch.

### 8.2 Lifecycle

```
getVisibilityTTL() == 0 (manual)  →  startCountdownDisplay() only
getVisibilityTTL() > 0            →  stop + restart timer, then startCountdownDisplay()

No type has visible === true      →  stop both timers (nothing to refresh)
```

The **single entry point** for all of this is `syncVisibilityTimer()`. All control handlers call it; nothing calls `startVisibilityTimer()` directly.

### 8.3 Timer tick

```
visibilityTimer tick
  ├── refreshAllVisibilitySnapshots()    (one fetch → broadcast to all active tabs)
  ├── for each type with visible === true:
  │     - invalidate _aboveHorizonAt on all patterns
  │     - reRenderPanelForType()
  └── resetCountdown()
```

### 8.4 Visibility snapshot display

When the filter is active, the control panel shows a snapshot group:

```
 Location: Paris (48.8534°, 2.3488°)
 Time:     2026-10-20 01:57:38 UTC
 JDay:     2461333.581698
```

These values come from `GET /api/main/status` and are refreshed on the **exact same cadence** as the visibility data (i.e. on every timer tick and on every manual Refresh click).

**`refreshAllVisibilitySnapshots()`** makes a single request and broadcasts the same snapshot to every active tab, guaranteeing that the displayed time is identical across tabs.

---

## 9. Tour Script Generation

The `skyculture-tours.js` module produces four types of `.ssc` tours. All of them share the same skeleton.

### 9.1 Common structure

```
// --- header (name, culture, options, generated date) ---

include("i18n.inc");

// --- culture verification ---
if (core.getSkyCulture() !== requiredCulture) core.setSkyCulture(requiredCulture);

// --- locked snapshot (only if visibility was active) ---
//   • setSkyCulture (implicitly — already done above)
//   • setObserverLocation(lon, lat, alt, 0, name, planet)
//   • setJDay(jday)
//   • optional: setTimeRate(0) — commented out
//   • optional: save original state — commented out

// --- initial setup ---
core.clear("starchart");
LandscapeMgr.setFlagLandscape(false);
core.setGuiVisible(false);
StelMovementMgr.setFlagTracking(false);

// --- save / restore display state (save_state.inc or manual) ---
// --- appearance configuration (colors, thickness, fade) ---
// --- welcome message (3 labels) ---

// --- the object list (hard-coded array OR dynamic retrieval) ---

// --- tour loop (per object: select → move → zoom → label → wait → reset) ---

// --- reset and cleanup (restore state, restore grid, show GUI) ---

// --- optional: restore original position — commented out ---
```

### 9.2 When the visibility filter is active

Two extra pieces of content are emitted:

1. **Metadata block** in the header:
   ```
   // VISIBILITY FILTER ACTIVE
   // Objects included: 408 / 660
   // Observer location: Paris (Earth)
   // Snapshot time (UTC): 2026-10-20T01:57:38.740Z
   ```

2. **Locked snapshot block** in the script body:
   ```ssc
   core.setObserverLocation(2.34879994, 48.85340881, 42.0, 0, "Paris", "Earth");
   core.setJDay(2461333.5816983795);
   ```

The rationale is simple: a filtered list without a locked location/time would reproduce a *different* sky when run later or elsewhere. Locking the snapshot makes the script **reproducible**.

An optional pair of commented-out save/restore blocks allows users to return to their original state; see §9.3.

### 9.3 Save / restore original state

The save block is emitted **commented out**:

```ssc
// var __sc_oldLocationId = core.getObserverLocation();
// var __sc_oldJDay       = core.getJDay();
// var __sc_oldCulture    = core.getSkyCulture();
```

And the matching restore block is emitted at the very end of the script, also commented out:

```ssc
// core.setSkyCulture(__sc_oldCulture);
// core.setObserverLocation(__sc_oldLocationId, 1);
// core.setJDay(__sc_oldJDay);
```

The user uncomments **both** blocks to enable round-trip restore. This is intentionally opt-in: if the user stops the script mid-way, Stellarium stays at the locked snapshot, which is the safest default.

### 9.4 Async wrappers and `/api/main/status`

Generation is synchronous, but the visibility metadata requires the current location and time. The module therefore exposes **async wrappers**:

```
generateStarsTourAsync()           // == withStatus(generateStarsTour)
generateConstellationsTourStaticAsync()
generateConstellationsTourDynamicAsync()
generateAsterismsTourAsync()
generateLunarTourAsync()
```

Each calls `Api.fetchStatus()`, caches the result in `_lastStatus`, calls the sync generator, and clears `_lastStatus` in a `finally`. The sync generators remain callable for tests and for fallback.

### 9.5 `save_state.inc` versus manual save

The tour generators support two save strategies, chosen by a checkbox:

| Mode | Effect |
|------|--------|
| **Use `save_state.inc`** | Includes the standard Stellarium include file and calls `saveState()` / `restoreState()`. Restores a *broad* set of display options (grids, nebulae, landscape, projection, night mode, observer location). Ideal for sharing scripts across machines. |
| **Manual save** | Saves and restores only the options that the tour itself modifies. Self-contained, minimal side effects. Ideal for running on your own machine. |

The choice is documented in the script header (`// Use save_state.inc: YES/NO`).

---

## 10. API Endpoints Used

| Endpoint | Method | Purpose | Notes |
|----------|--------|---------|-------|
| `/api/view/listskyculture` | GET | List installed cultures | Returns `{ id: translatedName }` |
| `/api/view/skyculturedescription/index.json` | GET | Complete culture data | **Single source of truth** |
| `/api/view/skyculturedescription/` | GET | HTML / file server | Accepts `?path=...` or `?file=...` |
| `/api/objects/info` | GET | Object info for one object | `?name=...&format=json` |
| `/api/main/status` | GET | Observer location + time + view | Used by visibility snapshot |
| `/api/main/focus` | POST | Focus the sky on a target | Used by `stelUtils` |
| `/api/stellarium/property/set` | POST | Set StelProperty | Used for culture change |
| `/api/scripts/direct` | POST | Execute SSC script | Used by Script Editor |

### 10.1 Response shape of `/api/objects/info`

For all object types the response includes (in addition to physical data):

| Field | Available for | Meaning |
|-------|--------------|---------|
| `found` | all | Whether Stellarium recognized the identifier. |
| `above-horizon` | all | Boolean. |
| `localized-name` | all | Translated name in the UI language. |
| `iauConstellation` | all (except for constellations themselves, where it is the constellation ID) | Parent IAU constellation. |
| `object-type` | all | `"constellation"`, `"asterism"`, `"star"`, `"eruptive variable star"`, `"double star"`, … |
| `star-type` | stars | `"star"`, `"double-star"`, `"variable"`. |
| `variable-star` | stars | `"no"`, `"eruptive"`, … |
| `vmag` | stars, DSOs | Real value. `99` for constellations and asterisms. |
| `cultural-names` | stars only | Array of strings with control characters to strip. |

**Consequence:** the same `normalizeObjectInfo` in `skyculture-api.js` serves every panel. There is no per-type fetch.

---

## 11. Centralized State

`skyculture-state.js` is the only place in the module where mutable state is stored.

### 11.1 Shape

```
State.raw = {
  selection:   { pattern, id, type },
  multiSelect: { active, mode, selected: { constellation: Set, ... } },
  filters:     { constellation: { visible, nameMode, sortBy, sortDir },
                 asterism: { ... },
                 lunar: { ... },
                 star: { ..., starType } },
  fov:         { constellation, asterism, zodiac, lunar, star },
  data:        { constellations, asterisms, zodiac, lunar, stars, artwork, raw },
  original:    { constellations, asterisms, lunar, stars },     // unfiltered
  displayed:   { constellations, asterisms, lunar, stars },     // last rendered (filtered + sorted)
  cache:       { constellation, star, displayName },
  culture:     { id, name, list },
  ui:          { containers, infoFrame, cultureCount },
  flags:       { isLoadingConstellationData, isSortingStars,
                 isArtworkClickLocked, isInitialized, syncEnabled },
  timestamps:  { lastConstellationFetch }
}
```

### 11.2 Contract

- **Reads** are allowed from any module.
- **Writes** must go through the helper functions (`setSelection`, `clearSelection`, `setData`, `resetData`, `resetFilters`, `clearCaches`, ...).
- **`State.raw.original.*` is never modified** after `loadAllCultureData` completes; it is the fallback when a filter is turned off.
- **`State.raw.displayed.*` is rewritten on every render**, and is the source of truth for multi-select ordering.

---

## 12. Filtering, Sorting, Grouping

`skyculture-filters.js` is a library of **pure functions** with no DOM access, no AJAX, and no side effects (other than attaching `_info` and `_displayName` to the patterns passed in).

### 12.1 The `applyAll` pipeline

```
applyAll(patterns, filterState, infoMap, type)
  1. Visibility filter       → applyVisibility()
  2. Star-type filter        → applyStarTypeFilter()   (stars only)
  3. Name-mode application   → applyNameMode()
  4. Sort                    → applySort()
  5. Grouping                → applyGrouping()         (stars only)
```

### 12.2 Guarantees

| Property | Guarantee |
|----------|-----------|
| `name` sort | Uses `localeCompare` and respects `sortDir`. |
| `constellation` sort | `"Unknown"` always last. |
| `magnitude` sort | Missing `vmag` → treated as `999`. |
| `type` sort | Groups by `object-type`. |
| Visibility filter | Patterns without `_info` are **kept** (safe default). |
| Star-type filter | Patterns without `_info` are **kept** (safe default). |
| Name-mode fallback | `default` when `_info` is null or the field is missing. |

The pipeline is **deterministic**: for a given input and `filterState`, the output is always the same.

---

## 13. Rendering

`skyculture-render.js` owns **all DOM creation**. It never calls the API and never touches `State.raw.data`.

### 13.1 Public renderers

| Function | Purpose |
|----------|---------|
| `renderCultureButtons(cultures, activeId, onClick, tr)` | Grid of culture buttons. |
| `updateCultureButtonStates(activeId)` | Re-applies `.active` after a culture change. |
| `renderPatternGrid($container, patterns, type, opts)` | Flat grid; used by every panel except grouped stars. |
| `renderGroupedStars($container, groupedData, opts)` | Grouped stars with optional "Select All". |
| `renderArtworkGrid($container, artworks, opts)` | Artwork thumbnails. |
| `showArtworkModal(src, name, tr)` | Full-screen artwork viewer. |
| `renderPlaceholder($container, text)` | Empty / loading state. |
| `updatePatternCount(type, count)` | Tab-header counter. |
| `updateVisibilityItemCount(prefix, v, t, filtered)` | Control-panel counter (visible/total). |
| `clearAllActiveButtons()` | Reset every `.active` class. |
| `highlightButton($container, id, name)` | Activate one button by ID (name fallback). |
| `escapeHtml(str)` | XSS-safe string escaping. |
| `toJQuery(val)` | Defensive normalization: selector → jQuery, `null` → empty. |

### 13.2 Safety rules

- Every user-facing string is passed through `escapeHtml()` before insertion.
- `renderPatternGrid` **never** attaches the click handler when `disableHandler === true`; multi-select attaches its own.
- `renderGroupedStars` is the only function that renders `.star-group-header`; the "Select All" button is only inserted in multi-select mode.

---

## 14. Synchronization (Bidirectional)

`skyculture-sync.js` is the module that mirrors Stellarium's selection state onto the UI buttons, and vice versa.

### 14.1 The `objectSelected` event

`stelUtils` emits:

```js
$(document).trigger('objectSelected', { name, id, type });
```

`sync.onObjectSelected(evt, data)` handles it.

### 14.2 Normalization

`sync.normalize(rawId, rawName, typeHint)` returns `{ type, id, name, pattern }` by:

1. Exact ID match in `State.raw.data[typeKey]`.
2. `CON` / `AST` prefix fallback (the last token of the ID).
3. Name-based fallback across all groups.

This one function replaces the three earlier copies of "match this event to a button".

### 14.3 Button index

`sync.rebuildIndex()` builds a `Map<patternId, jQuery>` of every button in every container. It is called **after every render** and provides O(1) lookups for `highlightById`.

### 14.4 Cross-panel sync

- **Constellation ↔ Artwork**: selecting a constellation highlights the matching artwork thumbnail, and vice versa.
- **Stars grouped view**: selecting a star via external event highlights its button even when the stars panel is in grouped mode.
- **Culture change**: the `stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID` listener updates the whole UI (see §6.1).

### 14.5 Why there is no cross-type sync

A user might wonder why clicking a constellation does not scroll the artwork tab to the matching thumbnail. The answer is that the four panels are **independent explorers** of the same culture, not a single linked view. Cross-type sync would create surprising behavior (e.g. clicking a star would hide the constellation panel), and would not match how users actually navigate the data. Artwork is the only panel that is *naturally* tied to constellations, and that link is honored.

---

## 15. Help System

`skyculture-help.js` provides **two kinds of help**:

### 15.1 Controls help

Each control panel (one per tab) contains a **? Controls Help** button. It opens a modal (`#help-controls-<type>`) that explains:

- Visible above horizon only + refresh interval + snapshot
- Name display mode
- Sort and group (where applicable)
- Star type filter (stars only)
- Multi-select workflow
- Navigation FOV
- Isolation semantics (constellations only)
- Generated script metadata (stars only; see §9)

The same modal framework is initialized **once** and reused. `initHelpModals()` wraps the content in `.tour-help-content` and calls `.dialog()`.

### 15.2 Tour Generator help

Each Tour Generator toolbar contains its own **Tour Generator Help** button. It opens a modal (`#help-<type>`) that explains:

- Dynamic vs static variants (constellations only)
- Multi-select interaction
- Save-state option (checked vs unchecked)
- Additional options (duration, FOV, colors, fade)
- Visibility-filter metadata (stars only)

### 15.3 ID contract for help buttons

Every help button uses the `data-help-target` attribute:

```html
<button class="pattern-btn jquerybutton tour-help-btn"
        data-help-target="help-constellations">…</button>
```

`bindHelpButtons(tr)` attaches a single delegated `click` handler on `document` for `.tour-help-btn`, opens the modal with matching ID, and initializes it lazily on first use. This means:

- Adding a new help modal only requires a new `<div id="help-xxx">` in the HTML and a new button with `data-help-target="help-xxx"`.
- No extra JS wiring is needed.

---

## 16. Internationalization (i18n)

### 16.1 Rules

- Every user-facing string is passed through `rc.tr()` at the **point of use**.
- No strings are concatenated *inside* `tr()`. Instead:
  ```js
  tr("Visiting constellation") + " " + currentNum + "/" + total
  ```
  This lets the translator reorder words while keeping numeric placeholders.
- The `tour-help-content` and `help-controls-*` modal bodies are inside `indexplugin.html` and are wrapped in `<?= tr(...) ?>` so they are picked up by `update_translationdata.py` automatically.
- `console.log` / `console.warn` messages are developer-only and are **not** translated.

### 16.2 Adding a translation

1. Add the English string in the JS (`rc.tr("...")`) or in the HTML (`<?= tr("...") ?>`).
2. Run `update_translationdata.py` from the Stellarium repo.
3. Translate the new entry in `translationdata_<lang>.js`.

No code change is required to add a new language.

---

## 17. Error Handling and Edge Cases

| Situation | Handling |
|-----------|----------|
| `listskyculture` fails | Show placeholder in culture container. |
| `index.json` fails | Show placeholder in every panel; buttons are cleared. |
| `objects/info` times out | The batch continues; the pattern keeps `_info = null`. |
| Culture has no `lunar_system` | Lunar panel shows "No lunar mansions available". |
| Culture has no `zodiac` | Zodiac panel shows "No zodiac data available". |
| Culture has > 3200 named stars | Stars are truncated to 3200 with a console warning. |
| Generated script > 50 KB | Script is offered as a file download instead of being inserted. |
| `fetchStatus` fails during script generation | Generation proceeds without location/time metadata. |
| User toggles a filter while a fetch is in progress | `isLoading…` flag prevents a duplicate request; the next render uses the fresh data. |
| Culture changes during a fetch | `State.clearCaches()` and `State.resetData()` invalidate the in-flight data. |
| Non-ASCII control characters in `localized-name` | `cleanName` strips them before display. |
| Missing `common_name` on a pattern | Display falls back to `pattern.id`. |

---

## 18. Performance Notes

### 18.1 Caching layers

| Cache | Scope | Invalidated by |
|-------|-------|----------------|
| `_info` on each pattern | Per-pattern | Culture change, manual Refresh, TTL expiry |
| `State.raw.cache.constellation` | Per-culture | Culture change |
| `State.raw.cache.star` | Per-culture | Culture change |
| `State.raw.cache.displayName` | Per-(culture, id, mode) | Culture change, name-mode change |
| `State.raw.displayed[type]` | Per-render | Next render |

### 18.2 Lazy loading

The default state (no filter, no special name mode, sort by name) triggers **zero** API calls. The first non-default filter triggers one batch. Every subsequent change to the same filter is instant.

### 18.3 Button index

`sync.rebuildIndex()` builds a single `Map<patternId, jQuery>` after every render. `highlightById` is O(1) instead of O(n × 7).

### 18.4 Concurrency

`Api.fetchObjectsBatch` processes patterns in batches of `Config.FETCH_CONCURRENCY` (16). The batch size is a single constant, not scattered through the code.

### 18.5 Grouping cost

Grouping stars by constellation requires walking the sorted array once (O(n)) and is only performed when `sortBy` is `"constellation"` or `"type"`. No nested loop.

---

## 19. Extension Guide

### 19.1 Adding a new pattern type

1. Add a new entry to `Config.PATTERN_TYPES`.
2. Add a new `extract*()` function in `skyculture-data.js`.
3. Add a new `TYPE_CONFIG` entry in `skyculture-multiselect.js`.
4. Add a new `CONTROL_PANEL_CONFIG` entry in `skyculture-help.js`.
5. Add a new tab in `indexplugin.html` with:
   - `<div id="patterns-tab-<type>">` container
   - `<div id="<type>-buttons-container">` inside it
   - Count badge `<span id="<type>-count">`
6. Add `Help.installControlPanel('<type>', tr, controlHandlers)` in `skyculture.js#init`.
7. Add the new type to `renderPanel`'s switch if it needs special handling.

**Nothing else needs to change.** Filters, sorting, name-mode, multi-select, sync, and the tour infrastructure are already generic.

### 19.2 Adding a new filter

1. Add the control to the relevant `CONTROL_PANEL_CONFIG` entry (or a new one).
2. Add the corresponding field to `State.raw.filters[type]`.
3. Extend `Filters.applyAll` with the new stage.
4. Extend `needsFetch` in `skyculture.js` with the required-field check.
5. Add the field to `getRequiredFields` if it comes from the API.

### 19.3 Adding a new tour type

1. Add a `generate<Type>Tour*` function in `skyculture-tours.js`.
2. Add a `generate<Type>Tour*Async` wrapper using `withStatus()`.
3. Add a `create<Type>TourToolbar()` function that calls `Tours.createToolbar({...})`.
4. Add the modal `<div id="help-<type>">` to `indexplugin.html`.
5. Call `create<Type>TourToolbar()` from `skyculture.js#init`.

### 19.4 Adding a new help modal

Just add a `<div id="help-xxx" style="display:none;" title="…">` inside `indexplugin.html` (before `</body>`), wrap its content in `<div class="tour-help-content">`, and reference it with `data-help-target="help-xxx"` on a button. The JS handles it automatically.

---

## 20. File-by-File Reference

### `skyculture.js` (entry point)

- **Responsibilities:** orchestration, public API, timer management.
- **Key exports:** `init`, `refresh`, `setCulture`, `getCurrentCulture`, `getAvailableCultures`, `refreshData`, `isSyncEnabled`, `setSyncEnabled`.
- **Owns:** `visibilityTimer`, `visibilityCountdownTimer`, `_lastStatusCache`.

### `skyculture-config.js`

- **Responsibilities:** constants only. No logic.
- **Key exports:** `DEFAULT_FOV`, `FOV_RANGES`, `CONSTELLATION_CACHE_TTL`, `VISIBILITY_CACHE_TTL_DEFAULT`, `VISIBILITY_CACHE_MODES`, `FETCH_CONCURRENCY`, `FETCH_TIMEOUT`, `OBJECT_INFO_TIMEOUT`, `MAX_STAR_NAMES`, `MAX_DISPLAY_NAME_LENGTH`, `MAX_SCRIPT_SIZE`, `DEBOUNCE_DELAY`, `ANIMATION_DURATION`, `NOTIFICATION_DURATION`, `STATUS_CLEAR_DELAY`, `API_ENDPOINTS`, `OBJECT_TYPES`, `PATTERN_TYPES`, `NAME_DISPLAY_MODES`, `STAR_SORT_OPTIONS`, `SORT_DIRECTIONS`.

### `skyculture-state.js`

- **Responsibilities:** state container + mutation helpers.
- **Key exports:** `raw`, `setSelection`, `clearSelection`, `getSelectedSet`, `toggleSelected`, `clearSelected`, `clearAllSelected`, `getFilter`, `resetFilters`, `setData`, `getData`, `resetData`, `clearCaches`, `clearPatternsInfoCache`, `clearConstellationCache`.

### `skyculture-api.js`

- **Responsibilities:** all server communication.
- **Key exports:** `ajax`, `fetchCultures`, `fetchIndexJson`, `fetchObjectInfo`, `fetchObjectsBatch`, `fetchVisibilityBatch`, `getConstellationInfo`, `normalizeObjectInfo`, `focusObject`, `runScriptDirect`, `fetchStatus`.

### `skyculture-data.js`

- **Responsibilities:** parsing `index.json`.
- **Key exports:** `extractConstellations`, `extractAsterisms`, `extractZodiac`, `extractLunarMansions`, `extractStars`, `extractArtwork`, `getSolarSystemDefaultName`, `SOLAR_SYSTEM_DEFAULT_NAMES`.

### `skyculture-filters.js`

- **Responsibilities:** pure filter / sort / group functions.
- **Key exports:** `cleanName`, `buildInfoMap`, `getDisplayName`, `applyNameMode`, `applyVisibility`, `applySort`, `applyGrouping`, `applyStarTypeFilter`, `applyAll`.

### `skyculture-render.js`

- **Responsibilities:** DOM rendering.
- **Key exports:** `toJQuery`, `escapeHtml`, `renderPlaceholder`, `getPlaceholder`, `updatePatternCount`, `renderCultureButtons`, `updateCultureButtonStates`, `renderPatternGrid`, `renderGroupedStars`, `renderArtworkGrid`, `showArtworkModal`, `updateVisibilityItemCount`, `clearAllActiveButtons`, `highlightButton`.

### `skyculture-multiselect.js`

- **Responsibilities:** unified multi-select engine.
- **Key exports:** `TYPE_CONFIG`, `TYPE_KEYS`, `isActive`, `enter`, `exit`, `getSelectedSet`, `getSelectedPatterns`, `toggle`, `selectAll`, `deselectAll`, `clear`, `clearAll`, `updateSelectionCount`, `saveState`, `restoreState`, `attachHandlers`, `syncDynamicButtonState`.

### `skyculture-sync.js`

- **Responsibilities:** bidirectional sync with Stellarium.
- **Key exports:** `setup`, `rebuildIndex`, `normalize`, `onObjectSelected`, `highlightById`, `highlightArtwork`, `clearAllActive`, `createCultureChangeHandler`.

### `skyculture-artwork.js`

- **Responsibilities:** artwork panel logic.
- **Key exports:** `render`, `attachHandlers`, `highlight`, `clearHighlight`.

### `skyculture-tours.js`

- **Responsibilities:** SSC script generation + tour toolbars.
- **Key exports:**
  - Sync: `generateConstellationsTourStatic`, `generateConstellationsTourDynamic`, `generateAsterismsTour`, `generateLunarTour`, `generateStarsTour`.
  - Async: `generateConstellationsTourStaticAsync`, `generateConstellationsTourDynamicAsync`, `generateAsterismsTourAsync`, `generateLunarTourAsync`, `generateStarsTourAsync`.
  - Toolbars: `createToolbar`, `initTourToolbar`, `initColorSliders`, `initSpinners`.
  - Helpers: `outputScript`, `notify`, `uiNum`, `uiBool`, `uiInt`, `uiColor`, `fmt`.

### `skyculture-help.js`

- **Responsibilities:** control panels + help modals.
- **Key exports:** `installControlPanel`, `initHelpModals`, `bindHelpButtons`, `updateVisibilityRefreshButtons`, `initVisibilityRefreshButtons`, `updateVisibilitySnapshot`, `clearVisibilitySnapshot`, `CONTROL_PANEL_CONFIG`, `HELP_MODALS`.

---

## Appendix A — Public API

The module returns a small facade:

```js
{
  init(cultureContainerSelector, patternsContainers, iframeSelector, options),
  refresh(),
  setCulture(cultureId),
  getCurrentCulture()        → string|null,
  getAvailableCultures()     → Array<{id, name}>,
  getSelectedPattern()       → string|null,
  getSelectedPatternId()     → string|null,
  refreshData(),
  isSyncEnabled()            → boolean,
  setSyncEnabled(bool)
}
```

**Everything else is internal.** If you need a new public entry, add it here — do not expose sub-modules directly.

---

## Appendix B — ID Contract

The whole module depends on this contract. Any new tab must use the same convention.

### Tab header counter

```html
<span id="<type>-count" class="pattern-count"></span>
```

`<type>` ∈ { `constellations`, `asterisms`, `zodiac`, `lunar`, `stars`, `artwork` }.

### Buttons container

```html
<div id="<type>-buttons-container" class="patterns-buttons-container"></div>
```

### Control panel (per tab)

The `filterIdPrefix` differs from the container key for asterisms and stars:

| Type | Container key | Filter prefix |
|------|---------------|---------------|
| constellation | `constellations` | `constellation` |
| asterism | `asterisms` | `asterism` |
| zodiac | `zodiac` | `zodiac` |
| lunar | `lunar` | `lunar` |
| star | `stars` | `star` |

Resulting IDs:

```
#<prefix>-filter-visible
#<prefix>-name-display
#<prefix>-nav-fov
#<prefix>-multi-select-mode
#<prefix>-selection-count
#<prefix>-sort-by           (stars only)
#<prefix>-sort-direction    (stars only)
#<prefix>-apply-sort        (stars only)
#<prefix>-star-type         (stars only)
```

### Multi-select CSS classes

```
.selected-constellation
.selected-asterism
.selected-lunar
.selected-star
```

### Tour controls

```
#<short>-tour-controls-container      (const / aster / lunar / stars)
#btn-generate-<short>-static          (const / aster / lunar / stars)
#btn-generate-const-dynamic
```

### Help modals

```
#help-controls-<type>   (constellation / asterism / lunar / star / zodiac)
#help-<type>            (constellations / asterisms / lunar / stars)
```

---

## Appendix C — Glossary

| Term | Meaning |
|------|---------|
| **Sky Culture** | A coherent set of constellations, asterisms, star names, and artwork associated with a cultural tradition. |
| **Pattern** | A generic term for any item that appears as a button (constellation, asterism, zodiac sign, lunar mansion, star). |
| **`index.json`** | The JSON file served by Stellarium that contains the full cultural dataset. It is the *single source of truth* for patterns. |
| **`_info` cache** | The per-pattern object `{ aboveHorizon, localizedName, iauConstellation, vmag, starType, objectType, … }` fetched from `/api/objects/info` and stored on each pattern. |
| **`_displayName`** | The name shown on a button, which may differ from `pattern.name` depending on the name mode. |
| **Multi-Select** | The mode in which users build a custom selection set; the generated tour includes only the selected items. |
| **Snapshot** | The (`Location`, `Time`, `JDay`) triple shown when the visibility filter is active. |
| **Locked snapshot** | The `core.setObserverLocation()` + `core.setJDay()` block emitted at the top of a generated script when the visibility filter was active. |
| **Visibility timer** | The auto-refresh timer that invalidates `_aboveHorizonAt` on the current TTL. |
| **Countdown display** | The "⏱ N s" indicator in the control panel. |
| **Tour Generator** | The panel (one per tab) that produces SSC scripts. |
| **SSC** | Stellarium Script (`.ssc`). |
| **`save_state.inc`** | A standard Stellarium include file providing `saveState()` / `restoreState()`. |

---

*End of document.*
