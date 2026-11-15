# Scripts Generator Module — Architecture & Developer Guide

> Stellarium Remote Control · Scripts Generator Subsystem  
> Version: 4.0.0  
> License: GPLv2+  
> Author: kutaibaa akraa (GitHub: @kutaibaa-akraa)

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Design Goals](#2-design-goals)
3. [Module Map](#3-module-map)
4. [Layered Architecture](#4-layered-architecture)
5. [Data Flow](#5-data-flow)
6. [Type Families](#6-type-families)
7. [Visibility Filter & Timer](#7-visibility-filter--timer)
8. [Lazy Fetching & Caching](#8-lazy-fetching--caching)
9. [Multi-Select](#9-multi-select)
10. [Script Generation](#10-script-generation)
11. [API Endpoints Used](#11-api-endpoints-used)
12. [Centralized State](#12-centralized-state)
13. [ID Contract](#13-id-contract)
14. [Extension Guide](#14-extension-guide)
15. [File-by-File Reference](#15-file-by-file-reference)
16. [Glossary](#16-glossary)

---

## 1. Purpose and Scope

The Scripts Generator module is the interactive UI inside Stellarium Remote Control that lets a user:

- Browse **all +88 object types** exposed by Stellarium's `/api/objects/listobjecttypes` endpoint.
- Explore the objects of any type (e.g. Messier Catalogue, Nebulae, Solar System planets, Supernovae, Asteroids).
- Filter, sort, and group those objects — using the same machinery as the Sky Culture tab.
- Select specific objects (multi-select) for a custom tour.
- Generate a ready-to-run **Stellarium Script (`.ssc`)** tour that can be edited and executed in the Script Editor.

This document is a **developer-facing** reference. A user guide (LaTeX) is maintained separately.

---

## 2. Design Goals

| Goal | Meaning |
|------|---------|
| **Single generator** | One `generateObjectTypeTour()` function serves all +88 types, no per-type code. |
| **Reuse over duplication** | Filters, sorting, rendering, multi-select, and help modals are reused from Sky Culture. |
| **Adaptive by family** | The `core.clear()` preset, DSO setup, and landscape handling adapt to the *family* of the selected type. |
| **Lazy loading** | `_info` is only fetched when a filter actually needs it. |
| **Race-condition safe** | Batch fetching (`fetchObjectsBatch`) prevents request storms. |
| **Predictable caching** | `_info` survives re-renders; only visibility TTL or manual refresh destroys it. |
| **UI/script sync** | The same family detection runs on both sides (UI + SSC) so options and output never diverge. |

---

## 3. Module Map

```
js/scripteditor/
└── scriptsGenerator.js              ← THE module (this document's subject)

js/ui/skyculture/                       ← Reused modules
├── skyculture-api.js                → fetchObjectInfo, fetchObjectsBatch, fetchStatus
├── skyculture-render.js             → renderPlaceholder, renderPatternGrid
├── skyculture-tours.js              → generateObjectTypeTour (SSC side)
├── skyculture-help.js               → bindHelpButtons
├── skyculture-filters.js            → applyAll
└── skyculture-config.js             → (not imported by v4.x — see §14)

ui/
└── stellarium-utils.js              → goToObject, clearSelection
```

### Dependency graph

```
                    ┌──────────────────────────┐
                    │  scriptsGenerator.js     │  (entry)
                    └────────────┬─────────────┘
         ┌───────────────────────┼───────────────────────┐
         ▼                       ▼                       ▼
  ┌────────────┐         ┌──────────────┐         ┌────────────┐
  │   api      │         │    render    │         │   tours    │
  └────────────┘         └──────────────┘         └────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
  ┌────────────┐         ┌──────────────┐         ┌────────────┐
  │  filters   │         │    help      │         │ stellarium-│
  └────────────┘         └──────────────┘         │   utils    │
                                                  └────────────┘
```

---

## 4. Layered Architecture

| Layer | Module | Question it answers |
|-------|--------|---------------------|
| **Transport** | `skyculture-api.js` | "How do we talk to the server?" |
| **Transformation** | `skyculture-filters.js` | "Given data + filterState, what should be displayed?" |
| **Presentation** | `skyculture-render.js` + local `renderGroupedWithSelectAll` | "How do we turn a pattern into a button?" |
| **Navigation** | `stellarium-utils.js` | "How do we move the view to an object?" |
| **Domain** | `skyculture-tours.js` | "How do we build a tour script?" |
| **Orchestration** | `scriptsGenerator.js` | "In what order do all of the above run?" |

The orchestrator **never** implements business logic. It calls the layers, in the correct order, with the correct arguments.

---

## 5. Data Flow

### 5.1 High level

```
┌─────────────────────────────┐
│ User selects an object type │
└──────────────┬──────────────┘
               │
               ▼
┌─────────────────────────────-┐
│ onTypeSelected(typeKey)      │
│  • detectTypeFamily          │
│  • reset state               │
│  • updateDsoVisibility       │
│  • autoEnableLandscapeIfSolar│
│  • fetchObjectsByType        │
└──────────────┬──────────────-┘
               │
               ▼
┌─────────────────────────────-┐
│ state.patterns = [...]       │
│   each: { id, name,          │
│           _info: undefined } │
└──────────────┬──────────────-┘
               │
               ▼
┌─────────────────────────────┐
│ renderObjects()              │
│  needsFetch()?               │
└──────────────┬──────────────┘
               │
     ┌─────────┴─────────┐
     ▼                   ▼
┌───────────────┐   ┌────────────────┐
│ NO (fast path)│   │ YES (slow path)│
│               │   │ fetchMissingInfo│
└──────┬────────┘   └────────┬───────┘
       │                     │
       ▼                     ▼
┌───────────────────────────────────┐
│ applyFiltersAndRender()           │
│  • Filters.applyAll               │
│  • renderGroupedWithSelectAll     │
│    OR renderPatternGrid           │
└───────────────────────────────────┘
```

### 5.2 When is a fetch required?

`needsFetch()` implements a **lazy default** strategy:

| Condition | Result |
|-----------|--------|
| No `_info` at all **and** filter state is default (name sort, no visibility) | `fetch: false` — render immediately |
| `_info` missing for one pattern **and** a filter needs it | `fetch: true` — full batch fetch |
| Visibility filter enabled and `_aboveHorizonAt` = 0 | `fetch: true` — manual invalidation |
| Visibility filter enabled and `_aboveHorizonAt` older than TTL | `fetch: true` — stale data |
| All required fields cached | `fetch: false` — instant |

**Consequence:** the first switch to a non-default filter triggers one batch fetch. Every subsequent toggle is instant until TTL expires.

---

## 6. Type Families

Every type key from `/api/objects/listobjecttypes` is classified into one of five **families**. This classification drives two decisions: (a) which `core.clear()` preset to use in the generated script, (b) whether to show the DSO options in the UI.

`detectTypeFamily(typeKey)` — implemented identically in both `scriptsGenerator.js` and `skyculture-tours.js`:

| Family | Detection rule | `core.clear` preset | DSO options visible? |
|--------|---------------|---------------------|----------------------|
| `dso` | `NebulaMgr*`, `Supernovae`, `Novae` | `deepspace` | ✅ Yes |
| `star` | `StarMgr*` | `deepspace` | ❌ No |
| `constellation` | `ConstellationMgr` | `starchart` | ❌ No |
| `solar` | `SolarSystem*` | `natural` | ❌ No (landscape auto-on) |
| `other` | anything else | `starchart` | ❌ No |

The UI side (`updateDsoVisibility()`) uses the family to show/hide `fieldset#scriptsgenerator-dso-options`. The SSC side (`pickClearPreset()`) uses it to pick the preset.

**Adding a new family:** add a case to both `detectTypeFamily()` functions and to `pickClearPreset()`. No other change is needed.

---

## 7. Visibility Filter & Timer

### 7.1 Two timers

| Timer | Owner | Period | Purpose |
|-------|-------|--------|---------|
| `visibilityTimer` | `scriptsGenerator.js` | TTL (60s / 180s / 420s) | Invalidates `_info` and re-renders. |
| `countdownTimer` | `scriptsGenerator.js` | 1 s | Updates the "⏱ N s" display only. |

They are separate so the countdown updates every second without triggering a fetch.

### 7.2 Lifecycle

```
getVisibilityTTL() == 0 (manual)   →  no auto-refresh; "⏱ —"
getVisibilityTTL() > 0            →  start timer + countdown
filterState.visible === false     →  stop both timers
```

The **single entry point** for all of this is `syncVisibilityTimer()`. All control handlers call it; nothing calls `startVisibilityTimer()` directly.

### 7.3 Timer tick

```
visibilityTimer tick
  ├── invalidateVisibilityCache()      ← sets _info = undefined
  ├── refreshSnapshot()                 ← one fetch → update display
  ├── renderObjects()                   ← triggers fetchMissingInfo
  └── lastVisibilityTick = Date.now()
```

### 7.4 Snapshot display

When the filter is active, the control panel shows:

```
 Location: Paris (48.8534°, 2.3488°)
 Time:     2026-11-08 01:57:38 UTC
 JDay:     2461344.581698
```

These values come from `GET /api/main/status` and are refreshed on **exactly the same cadence** as the visibility data.

---

## 8. Lazy Fetching & Caching

### 8.1 The `_info` cache

Each `state.patterns[i]` has an `_info` field:

| Value | Meaning |
|-------|---------|
| `undefined` | **Never fetched** — a fetch is required if a filter needs it. |
| `null` | **Fetch attempted, no data** — do NOT retry (avoids infinite loops). |
| `{...}` | **Cached info** — includes `_fetchedAt` and `_aboveHorizonAt` timestamps. |

### 8.2 Fetch strategy

`fetchMissingInfo()` builds a list of patterns with `_info === undefined`, then calls:

```js
Api.fetchObjectsBatch(items, onProgressCallback)
```

This method (from `skyculture-api.js`) uses a **concurrency-bounded batch loop** (default concurrency = 10, configurable via `Config.FETCH_CONCURRENCY`). It prevents request storms (the original `Promise.all` design was abandoned because 1842 simultaneous requests caused ~30% failures).

### 8.3 Cache invalidation

| Trigger | Effect |
|---------|--------|
| TTL tick | `_info = undefined` on every pattern |
| Manual Refresh click | Same |
| Change of refresh mode | Same if new TTL makes cache stale |
| Change of type | `state.patterns = []` (fresh arrays) |

**Never** partial-invalidate. Simplicity > micro-optimization.

---

## 9. Multi-Select

### 9.1 Two rendering modes

| Mode | Trigger | Function |
|------|---------|----------|
| **Flat** | `sortBy !== 'constellation'` | `Render.renderPatternGrid` |
| **Grouped** | `sortBy === 'constellation'` | `renderGroupedWithSelectAll` |

`renderGroupedWithSelectAll` is a local function (not from `skyculture-render`) because we need the "Select All" button to be controlled by our `state.multiSelectActive`, not by `State.raw.multiSelect.active` (which belongs to Sky Culture and is therefore `false` in our tab).

### 9.2 Selection set

`state.selectedObjects` is a `Set<string>` holding the object IDs (as returned by `/api/objects/listobjectsbytype`). It survives re-renders because `applyFiltersAndRender` reads it to determine which buttons should have the `active` class.

### 9.3 Group "Select All" button

When `state.multiSelectActive === true` AND rendering is grouped, each group header gets:

```html
<button class="star-group-select-all" data-group="Ori">Select All</button>
```

Click behavior:
- If **all** items in the group are already selected → deselect them; label becomes "Select All".
- Otherwise → select all; label becomes "Deselect All".

### 9.4 Global bulk actions

Two buttons in the header:
- **Select All** — selects every visible object (across all groups), updates all group buttons to "Deselect All".
- **Clear** — clears the selection set, updates all group buttons to "Select All".

---

## 10. Script Generation

### 10.1 Entry point

`generateScript()` collects all UI option values and calls:

```js
Tours.generateObjectTypeTour({
    typeKey, typeName, typeNameI18n,
    objects, options, status
})
```

### 10.2 The `options` object

```js
{
    // Base (all types)
    duration:      4,      // seconds per object
    zoomFov:       15,     // degrees
    overviewFov:   90,     // degrees
    fontSize:      16,     // pixels
    showLabels:    true,
    showMarkers:   true,
    useSaveState:  false,
    lockObserver:  false,
    showLandscape: false,  // v4.0.0

    // DSO-only (only read when family === 'dso')
    deepSky: {
        showHints:         true,
        hintsAmount:       8,
        labelsAmount:      7,
        showOutlines:      false,
        proportionalHints: false,
        catalogMessier:    false,   // mutually exclusive with catalogStandard
        catalogStandard:   false,
        useDss:            false,   // ToastMgr DSS background
        dssDelay:          8        // seconds of DSS visibility per object
    }
}
```

### 10.3 SSC generated structure (by family)

| Family | `core.clear()` | Special blocks |
|--------|---------------|----------------|
| `dso` | `deepspace` | `NebulaMgr.setFlagShow/Hints/Labels`; `ToastMgr.setFlagShow` (if DSS); `setCatalogFilters` (if selected) |
| `star` | `deepspace` | nothing extra |
| `constellation` | `starchart` | nothing extra |
| `solar` | `natural` | `LandscapeMgr.setFlagLandscape(true)` + `setFlagAtmosphere(true)` |
| `other` | `starchart` | nothing extra |

### 10.4 The tour loop

```javascript
for (var i = 0; i < objects.length; i++) {
    // Select (if markers enabled)
    core.selectObjectByName(objName, true);
    // Center
    core.moveToObject(objName, 2);
    core.wait(1);
    // Zoom
    StelMovementMgr.zoomTo(ZOOM_FOV, 2);
    core.wait(1);
    // DSS delay (only DSO + DSS enabled)
    core.wait(DSS_DELAY);
    // Label
    LabelMgr.labelScreen(objName, ...);
    core.wait(DURATION);
    LabelMgr.deleteLabel(...);
    // Deselect
    core.selectObjectByName("", false);
    // Overview
    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);
    core.wait(1);
}
```

---

## 11. API Endpoints Used

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/objects/listobjecttypes` | GET | List of 88+ types |
| `/api/objects/listobjectsbytype` | GET | Objects of a given type (`?type=...&english=0/1`) |
| `/api/objects/info` | GET | Detailed info for one object (used for `_info` cache) |
| `/api/main/status` | GET | Observer location + time + view (for snapshot) |

Two of these are exposed through `skyculture-api.js`:
- `Api.fetchObjectInfo(name)` → wraps `/api/objects/info`
- `Api.fetchStatus()` → wraps `/api/main/status`

The first two are called directly with `$.ajax` in `scriptsGenerator.js` because they aren't otherwise used by Sky Culture.

---

## 12. Centralized State

All mutable state lives in `state`:

```js
state = {
    allTypes: [],              // 88+ types from API
    selectedType: null,         // { key, name, name_i18n }
    currentObjects: [],         // ["M1", "M2", ...]
    patterns: [],               // [{ id, name, _info, ... }]
    selectedObjects: Set(),     // multi-select set
    multiSelectActive: false,
    filterState: {
        visible, sortBy, sortDir, nameMode
    },
    lastStatus: null,
    lastGeneratedScript: '',
    initialized: false,
    apiReady: false,
    visibilityTimerTTL: 0,
    currentTypeFamily: 'other'  // v4.0.0
}
```

### Contract

- **Reads** are allowed from any function in the module.
- **Writes** happen only inside the orchestrator functions (`onTypeSelected`, `toggleMultiSelect`, `applyFiltersAndRender`, etc.).
- **`state.patterns`** is replaced entirely when a type changes. It is modified in place when `_info` is invalidated or populated.

---

## 13. ID Contract

The module depends on this HTML contract. Any new UI element must follow it.

### Containers

| ID | Purpose |
|----|---------|
| `#scriptsgenerator-types-container` | Holds the 88 type buttons |
| `#scriptsgenerator-objects-container` | Holds the object buttons |
| `#scriptsgenerator-dso-options` | DSO fieldset (hidden unless family === 'dso') |
| `#scriptsgenerator-preview` | `<pre>` for the SSC preview |
| `#scriptsgenerator-api-status` | Sidebar API status |
| `#scriptsgenerator-summary` | Sidebar selection summary |

### Control panel (built by `initControlPanel`)

| ID | Purpose |
|----|---------|
| `#sg-filter-visible` | Visibility checkbox |
| `#sg-visibility-refresh-mode` (radio group) | TTL radio group |
| `#sg-visibility-refresh` | Manual refresh button |
| `#sg-visibility-count` | "visible/total" badge |
| `#sg-countdown` | "⏱ N s" countdown |
| `#sg-show-landscape` | Horizon checkbox (v4.0.0) |
| `#sg-sort-by` | Sort criterion select |
| `#sg-sort-direction` | Sort direction select |
| `#sg-apply-sort` | Apply button |
| `#sg-nav-fov` | Navigation FOV spinner |
| `#sg-vis-location`, `#sg-vis-time`, `#sg-vis-jday` | Snapshot values |

### DSO options (from HTML)

| ID | Purpose |
|----|---------|
| `#scriptsgenerator-show-hints` | Show DSO hints checkbox |
| `#scriptsgenerator-hints-amount` | Hints amount spinner |
| `#scriptsgenerator-labels-amount` | Labels amount spinner |
| `#scriptsgenerator-use-dss` | Enable DSS checkbox |
| `#scriptsgenerator-dss-delay-group` | Delay spinner wrapper (hidden until DSS enabled) |
| `#scriptsgenerator-dss-delay` | DSS visibility time spinner |
| `#scriptsgenerator-show-outlines` | Outlines checkbox |
| `#scriptsgenerator-proportional-hints` | Proportional hints checkbox |
| `#scriptsgenerator-catalog-messier` | Messier-only filter (mutually exclusive) |
| `#scriptsgenerator-catalog-standard` | Standard catalogs filter |

---

## 14. Extension Guide

### 14.1 Adding a new object type family

1. Add a case to `detectTypeFamily()` in **both** `scriptsGenerator.js` and `skyculture-tours.js`.
2. Add a case to `pickClearPreset()` in `skyculture-tours.js`.
3. (Optional) If the family needs new DSO-style options:
   - Add a new fieldset in `index.html`.
   - Show/hide it in `updateDsoVisibility()`.
   - Read its values in `generateScript()`.
4. (Optional) If it needs new setup blocks, add a `build<Family>Setup()` function in `skyculture-tours.js`.

### 14.2 Adding a new DSO option

1. Add the checkbox/input to `fieldset#scriptsgenerator-dso-options` in `index.html`.
2. Add a jQuery reference in `cacheDom()` (`$dom.newOption`).
3. Read it in `generateScript()` inside the `if (state.currentTypeFamily === 'dso')` block.
4. Consume it in `buildDeepSkySetup()` in `skyculture-tours.js`.
5. Add JSDoc to the new option in both `generateScript()` and `buildDeepSkySetup()`.

### 14.3 Adding a new object type (not a new family)

No code change is needed. The type will appear in the top grid automatically as soon as Stellarium's `/api/objects/listobjecttypes` returns it. Unless you want custom behavior for it, it will fall into the `other` family.

### 14.4 Changing the default visibility TTL

Edit `DEFAULT_VISIBILITY_TTL` at the top of `scriptsGenerator.js`. The map `VISIBILITY_TTL_MAP` can also be extended with new modes (e.g. `'900s': 900 * 1000`) — then add a new `<label><input type="radio" ...></label>` in `initControlPanel()`.

### 14.5 Adding a new family-specific option to the UI

Since the DSO fieldset is already styled with `.control-group`, `.tour-row`, `.tour-input-number`, and `.spinner`, adding a new row is a 3-line HTML change:

```html
<div class="tour-row">
    <label class="tour-control-group" style="cursor:pointer;">
        <input type="checkbox" id="scriptsgenerator-new-option" />
        <span class="tour-control-label">
            <?= tr("My new option")?>
        </span>
    </label>
</div>
```

Then `cacheDom`, `generateScript`, and the relevant `build*Setup()` function.

---

## 15. File-by-File Reference

### `scriptsGenerator.js` (this module)

- **Responsibilities:** UI orchestration, state, rendering dispatch, option collection.
- **Key exports:** `init`, `_state`.
- **Owns:** all module state, all timers, all DOM references.

### `skyculture-api.js` (reused)

- **Responsibilities:** all server communication.
- **Key methods used:** `fetchObjectInfo`, `fetchObjectsBatch`, `fetchStatus`.
- **Note:** `fetchObjectsBatch` handles concurrency internally — do not wrap it in `Promise.all`.

### `skyculture-render.js` (reused)

- **Responsibilities:** DOM rendering of buttons, placeholders, counters.
- **Key functions used:** `renderPlaceholder`, `renderPatternGrid`.

### `skyculture-tours.js` (reused — SSC generator)

- **Responsibilities:** building the actual SSC script.
- **Key functions used:** `generateObjectTypeTour`, `outputScript`, `notify`.
- **Internal helpers (v2):** `detectTypeFamily`, `pickClearPreset`, `buildDeepSkySetup`, `buildDssSetup`, `buildLandscapeSetup`.

### `skyculture-help.js` (reused)

- **Responsibilities:** help modal binding.
- **Key functions used:** `bindHelpButtons`.

### `skyculture-filters.js` (reused)

- **Responsibilities:** filter/sort/group logic.
- **Key function used:** `applyAll`.

### `stellarium-utils.js` (reused)

- **Responsibilities:** navigation to objects, notification toasts.
- **Key functions used:** `goToObject`, `clearSelection`.

---

## 16. Glossary

| Term | Meaning |
|------|---------|
| **Type** | An object from `/api/objects/listobjecttypes`; carries `key`, `name`, `name_i18n`. |
| **Family** | The abstract class of a type: `dso` / `star` / `constellation` / `solar` / `other`. Drives `core.clear()` preset and UI visibility. |
| **Pattern** | A single object in `state.patterns`: `{ id, name, _info, _displayName, ... }`. |
| **`_info` cache** | The object returned by `/api/objects/info`, stored on each pattern. |
| **`_aboveHorizonAt`** | Timestamp (ms) of the last successful `above-horizon` fetch. Used by TTL checks. |
| **TTL** | Time-to-live for visibility cache (60s / 180s / 420s / manual). |
| **Snapshot** | The (`Location`, `Time`, `JDay`) triple shown when visibility filter is active. |
| **DSS** | Digitized Sky Survey — photographic background provided by `ToastMgr`. |
| **Family-specific setup** | The block of SSC statements emitted by `build<Family>Setup()` (e.g. `buildDeepSkySetup`). |
| **SSC** | Stellarium Script (`.ssc`). |
| **`core.clear`** | Stellarium SSC function resetting display to a preset (`natural`, `starchart`, `deepspace`, `galactic`, `supergalactic`). |

---

*End of document.*
