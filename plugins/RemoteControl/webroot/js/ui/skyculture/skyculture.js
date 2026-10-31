/* ========================================================================
 * skyculture.js - Sky Culture Manager (Main Entry Point)
 * ========================================================================
 * 
 * This is the main entry point for the Sky Culture module. It orchestrates
 * all sub-modules to provide the complete Sky Culture UI.
 * 
 * RESPONSIBILITIES:
 * -----------------
 * - Module initialization (init)
 * - Culture loading and switching
 * - Main data loading orchestration (loadAllCultureData)
 * - Panel rendering dispatch (delegates to sub-modules)
 * - Tour toolbar creation
 * - Public API surface
 * 
 * ARCHITECTURE:
 * -------------
 * 
 *   skyculture.js (this file)
 *        │
 *        ├── skyculture-config     (constants)
 *        ├── skyculture-state      (state container)
 *        ├── skyculture-api        (server communication)
 *        ├── skyculture-data       (index.json parsing)
 *        ├── skyculture-filters    (filtering/sorting/grouping)
 *        ├── skyculture-render     (DOM rendering)
 *        ├── skyculture-multiselect(multi-select mode)
 *        ├── skyculture-sync       (bidirectional sync)
 *        ├── skyculture-artwork    (artwork panel)
 *        ├── skyculture-tours      (tour script generation)
 *        └── skyculture-help       (control panels + help modals)
 * 
 * DATA FLOW:
 * ----------
 * 
 *   User clicks culture button
 *        │
 *        ▼
 *   setCulture(cultureId)
 *        │
 *        ├─ api.fetchCultures (list)
 *        ├─ api.fetchIndexJson (data)
 *        ├─ data.extract* (parse)
 *        ├─ render.* (display)
 *        └─ sync.setup (listeners)
 * 
 * @module skyculture
 * @requires jquery
 * @requires api/properties
 * @requires api/remotecontrol
 * @requires ui/stellarium-utils
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @requires ./skyculture-api
 * @requires ./skyculture-data
 * @requires ./skyculture-filters
 * @requires ./skyculture-render
 * @requires ./skyculture-multiselect
 * @requires ./skyculture-sync
 * @requires ./skyculture-artwork
 * @requires ./skyculture-tours
 * @requires ./skyculture-help
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-10-28
 * @license GPLv2+
 * @version 5.0.0 - Modular architecture (supersedes 4.2.0 monolithic)
 * @previous 4.2.0 - Direct index.json loading (2026-07-24)
 * ======================================================================== */

define([
    "jquery",
    "api/properties",
    "api/remotecontrol",
    "ui/stellarium-utils",
    "./skyculture-config",
    "./skyculture-state",
    "./skyculture-api",
    "./skyculture-data",
    "./skyculture-filters",
    "./skyculture-render",
    "./skyculture-multiselect",
    "./skyculture-sync",
    "./skyculture-artwork",
    "./skyculture-tours",
    "./skyculture-help"
], function(
    $,
    propApi,
    rc,
    stelUtils,
    Config,
    State,
    Api,
    Data,
    Filters,
    Render,
    MultiSelect,
    Sync,
    Artwork,
    Tours,
    Help
) {
    "use strict";

    var tr = rc.tr;

    /**
    * VISIBILITY AUTO-REFRESH TIMER
    * =====================================================================
    * Holds the reference to the auto-refresh timer. Managed by
    * startVisibilityTimer / stopVisibilityTimer / syncVisibilityTimer.
    * 
    * IMPORTANT: This is runtime state, NOT configuration. It must live
    * in skyculture.js (or another state module), never in config.js.
    */
    var visibilityTimer = null;

    /**
     * TTL currently used by the running `visibilityTimer`, in milliseconds.
     * 0 means "no timer running".
     * 
     * This is used by `syncVisibilityTimer()` to decide whether the timer
     * needs to be restarted (when TTL changed) or can be left alone (when
     * the TTL is unchanged but a new visibility filter was just enabled).
     * 
     * @type {number}
     */
    var visibilityTimerTTL = 0;

    /**
		* COUNTDOWN DISPLAY TIMER
    * =====================================================================
    * Separate timer that updates the visible countdown display every
    * second. The actual refresh is handled by `visibilityTimer`, which
    * ticks at the full TTL. This timer just reflects the remaining time.
    * 
    * `visibilityLastTickAt` tracks when the last tick occurred, so we
    * can compute the remaining time.
		*/
    var visibilityCountdownTimer = null;
    var visibilityLastTickAt = 0;

    /**
     * Cached status from /api/main/status.
     * 
     * Populated when the visibility filter is active. Used to display
     * the location/time/JDay snapshot in the control panel, and also
     * consumed by skyculture-tours.js when generating scripts (via
     * the module-level `_lastStatus` there, set by the Async wrappers).
     * 
     * @type {Object|null}
     */
    var _lastStatusCache = null;
		
    // =====================================================================
    // DOM REFERENCE CACHING
    // =====================================================================

    /**
     * Cache DOM references into State.raw.ui for use by all sub-modules.
     * 
     * Accepts either strings (selectors) or jQuery objects, and normalizes
     * everything to jQuery objects before caching.
     * 
     * @param {Object} containers - Map of containerKey → selector or jQuery object
     * @param {string|jQuery} iframeSelector - Info frame selector or jQuery object
     * @param {Object} options - Additional options {cultureCount: selector}
     */
    function cacheDomReferences(containers, iframeSelector, options) {
        var ui = State.raw.ui;

        /**
         * Normalize a value to a jQuery object.
         */
        function toJQuery(val, fallbackSelector) {
            if (val instanceof $) return val;
            if (typeof val === 'string' && val.length) return $(val);
            if (fallbackSelector) return $(fallbackSelector);
            return $();
        }

        ui.containers.cultures       = toJQuery(containers.cultures,       '#skyculture-buttons-container');
        ui.containers.constellations = toJQuery(containers.constellations, '#constellations-buttons-container');
        ui.containers.asterisms      = toJQuery(containers.asterisms,      '#asterisms-buttons-container');
        ui.containers.zodiac         = toJQuery(containers.zodiac,         '#zodiac-buttons-container');
        ui.containers.lunar          = toJQuery(containers.lunar,          '#lunar-buttons-container');
        ui.containers.stars          = toJQuery(containers.stars,          '#stars-buttons-container');
        ui.containers.artwork        = toJQuery(containers.artwork,        '#artwork-container');

        ui.infoFrame    = toJQuery(iframeSelector, '#vo_skycultureinfo');
        ui.cultureCount = toJQuery(options && options.cultureCount, '#skyculture-culture-count');
    }

    // =====================================================================
    // CULTURE LOADING
    // =====================================================================

    /**
     * Load the list of available sky cultures and render buttons.
     * 
     * @returns {Promise<boolean>} True if successful
     */
    function loadSkyCultures() {
        return Api.fetchCultures().then(function(cultures) {
            State.raw.culture.list = cultures;

            // Determine current culture
            var currentId = null;
            try {
                currentId = propApi.getStelProp('StelSkyCultureMgr.currentSkyCultureID');
            } catch (e) {
                console.warn('[SkyCulture] Could not read current culture from propApi');
            }

            if (!currentId && cultures.length > 0) {
                currentId = cultures[0].id;
            }

            State.raw.culture.id = currentId;

            // Find culture name
            var name = currentId;
            for (var i = 0; i < cultures.length; i++) {
                if (cultures[i].id === currentId) {
                    name = cultures[i].name;
                    break;
                }
            }
            State.raw.culture.name = name;

            // Render culture buttons
            Render.renderCultureButtons(
                cultures,
                currentId,
                function(id) { setCulture(id); },
                tr
            );

            // Update info frame
            if (State.raw.ui.infoFrame && State.raw.ui.infoFrame.length) {
                State.raw.ui.infoFrame.attr('src',
                    Config.API_ENDPOINTS.skyCultureDescription);
            }

            return true;
        }).catch(function(error) {
            console.error('[SkyCulture] Failed to load cultures:', error);
            Render.renderPlaceholder(
                State.raw.ui.containers.cultures,
                tr("Error loading sky cultures")
            );
            return false;
        });
    }

    // =====================================================================
    // MAIN DATA LOADING
    // =====================================================================

    /**
     * Load all data for the current culture and render all panels.
     * 
     * @returns {Promise<boolean>}
     */
    function loadAllCultureData() {
        var cultureId = State.raw.culture.id;
        if (!cultureId) return Promise.resolve(false);

        // Show loading placeholders in all panels
        showAllLoadingPlaceholders();

        // Clear caches and reset state
        State.clearCaches();
        State.clearConstellationCache(cultureId);
        State.resetFilters();
        State.resetData();
        MultiSelect.clearAll();
        // Clear the displayed order (will be repopulated by the renders below)
        State.raw.displayed = {
            constellations: [],
            asterisms:      [],
            lunar:          [],
            stars:          []
        };

        // Clear all snapshot displays (will be repopulated if filter re-enabled)
        ['constellation', 'asterism', 'lunar', 'star'].forEach(function(t) {
            clearVisibilitySnapshotForType(t);
        });
        _lastStatusCache = null;
				
        // -----------------------------------------------------------------
        // Stop the visibility timer: resetFilters() cleared all `visible`
        // flags, so no type is filtering by visibility anymore. The timer
        // must not keep ticking against empty filters.
        // -----------------------------------------------------------------
        stopVisibilityTimer();
				
        // Reset counters
        Config.PATTERN_TYPES.forEach(function(type) {
            Render.updatePatternCount(type, 0);
        });
        Render.updatePatternCount('artwork', 0);

        // Fetch index.json
        return Api.fetchIndexJson().then(function(rawData) {
            State.raw.data.raw = rawData;

            // Extract all pattern types
            var constellations = Data.extractConstellations(rawData);
            var asterisms      = Data.extractAsterisms(rawData);
            var zodiac         = Data.extractZodiac(rawData, cultureId);
            var lunar          = Data.extractLunarMansions(rawData);
            var stars          = Data.extractStars(rawData);
            var artworks       = Data.extractArtwork(rawData, constellations);

            // Store original (unfiltered) data
            State.raw.original.constellations = constellations.slice();
            State.raw.original.asterisms      = asterisms.slice();
            State.raw.original.lunar          = lunar ? lunar.slice() : [];
            State.raw.original.stars          = stars ? stars.slice() : [];

            // Store current data
            State.setData('constellations', constellations);
            State.setData('asterisms', asterisms);
            State.setData('zodiac', zodiac);
            State.setData('lunar', lunar);
            State.setData('stars', stars);
            State.setData('artwork', artworks);

            // Render all panels
            renderConstellationsPanel(constellations);
            renderAsterismsPanel(asterisms);
            renderZodiacPanel(zodiac);
            renderLunarPanel(lunar);
            renderStarsPanel(stars);
            Artwork.render(artworks, tr);

            // Rebuild the button index for fast sync lookups
            Sync.rebuildIndex();

            console.log('[SkyCulture] All panels rendered for culture:', cultureId);
            return true;
        }).catch(function(error) {
            console.error('[SkyCulture] Failed to load index.json:', error);
            renderAllEmpty();
            return false;
        });
    }

    /**
     * Show loading placeholders in all panels.
     */
    function showAllLoadingPlaceholders() {
        var containers = State.raw.ui.containers;
        Render.renderPlaceholder(containers.constellations, tr("Loading constellations..."));
        Render.renderPlaceholder(containers.asterisms, tr("Loading asterisms..."));
        Render.renderPlaceholder(containers.zodiac, tr("Loading zodiac signs..."));
        Render.renderPlaceholder(containers.lunar, tr("Loading lunar mansions..."));
        Render.renderPlaceholder(containers.stars, tr("Loading star names..."));
        Render.renderPlaceholder(containers.artwork, tr("Loading artwork..."));
    }

    /**
     * Render empty state in all panels (on error).
     */
    function renderAllEmpty() {
        renderConstellationsPanel([]);
        renderAsterismsPanel([]);
        renderZodiacPanel(null);
        renderLunarPanel(null);
        renderStarsPanel(null);
        Artwork.render([], tr);
    }

    /**
     * Start the countdown display timer.
     * Updates all `.visibility-countdown` elements every 1 second.
     */
    function startCountdownDisplay() {
        stopCountdownDisplay();
        
        visibilityCountdownTimer = setInterval(function() {
            updateCountdownDisplay();
        }, 1000);
        
        // Immediate update
        updateCountdownDisplay();
    }

    /**
     * Stop the countdown display timer.
     */
    function stopCountdownDisplay() {
        if (visibilityCountdownTimer) {
            clearInterval(visibilityCountdownTimer);
            visibilityCountdownTimer = null;
        }
    }

    /**
     * Update all countdown displays based on the remaining time until
     * the next automatic refresh.
     * 
     * Behavior:
     *  - Manual mode (ttl === 0): shows "—" (no countdown).
     *  - Otherwise: shows "⏱ Ns" where N is seconds remaining.
     * 
     * When the countdown reaches 0, it resets to the full TTL (the actual
     * refresh is handled by visibilityTimer, which may have a slightly
     * different phase due to setInterval drift).
     */
    function updateCountdownDisplay() {
        var ttl = getVisibilityTTL();
        var $counters = $('.visibility-countdown');
        
        if (!$counters.length) return;
        
        // Manual mode OR no active visibility filter → show "—" no countdown
        if (ttl === 0 || !anyTypeHasVisibilityFilter()) {
            if ($counters.text() !== '—') {
                $counters.text('—');
            }
            return;
        }
        
        // Initialize last tick if not set
        if (!visibilityLastTickAt) {
            visibilityLastTickAt = Date.now();
        }
        
        var elapsed = Date.now() - visibilityLastTickAt;
        var remainingMs = ttl - elapsed;
        var remainingSec = Math.max(0, Math.ceil(remainingMs / 1000));
        
        // Format as "Xs" or "Xm Ys" for longer durations
        var text;
        if (remainingSec >= 60) {
            var mins = Math.floor(remainingSec / 60);
            var secs = remainingSec % 60;
            text = '⏱ ' + mins + 'm ' + (secs > 0 ? secs + 's' : '');
        } else {
            text = '⏱ ' + remainingSec + 's';
        }
        
        $counters.text(text);
        
        // When countdown reaches 0, reset (the main timer will re-tick soon)
        if (remainingSec <= 0) {
            visibilityLastTickAt = Date.now();
        }
    }

    /**
     * Reset the countdown to full TTL.
     * Called when the timer ticks or when the TTL changes.
     */
    function resetCountdown() {
        visibilityLastTickAt = Date.now();
        updateCountdownDisplay();
    }

    /**
     * Stop the visibility auto-refresh timer if running.
     * 
     * Also stops the countdown display timer and resets
     * `visibilityTimerTTL` so that the next start knows it begins
     * from a clean slate.
     */
    function stopVisibilityTimer() {
        if (visibilityTimer) {
            clearInterval(visibilityTimer);
            visibilityTimer = null;
            console.log('[SkyCulture] Visibility timer stopped');
        }
        visibilityTimerTTL = 0;
        stopCountdownDisplay();
    }

    /**
     * Reconcile the visibility timer with the current user state.
     * 
     * This is the SINGLE ENTRY POINT that all control handlers should
     * call when the visibility filter or the refresh mode changes.
     * It decides:
     * 
     *   1. Should a timer run at all?
     *      - NO if mode is manual (ttl === 0).
     *      - NO if no type has `visible === true`.
     *      - YES otherwise.
     * 
     *   2. If yes, does it need to be restarted?
     *      - NO if a timer is already running with the SAME ttl.
     *        (Preserves countdown phase across filter toggles.)
     *      - YES if no timer is running, or the ttl changed.
     * 
     * When the timer must NOT run, this function stops any running timer
     * and starts the countdown display in "—" mode (so the user still
     * sees the control, just with no active countdown).
     * 
     * USAGE:
     *   - Call from onFilterChange (checkbox visible)
     *   - Call from onVisibilityModeChange (radio TTL / manual)
     *   - Call from init() after control panels are installed
     *   - Call from loadAllCultureData() after resetFilters()
     */
    function syncVisibilityTimer() {
        var ttl = getVisibilityTTL();
        var hasActive = anyTypeHasVisibilityFilter();
        var shouldRun = (ttl > 0) && hasActive;
        
        console.log('[SkyCulture] syncVisibilityTimer:', {
            ttl: ttl,
            hasActive: hasActive,
            shouldRun: shouldRun,
            currentTimerTTL: visibilityTimerTTL,
            timerRunning: !!visibilityTimer
        });
        
        if (!shouldRun) {
            // No timer should run. Stop any existing one and show "—".
            stopVisibilityTimer();
            // Show "—" in the countdown (startCountdownDisplay handles
            // ttl === 0 by writing "—" and returning early on each tick).
            startCountdownDisplay();
            return;
        }
        
        // A timer should run. Decide if it needs restarting.
        if (visibilityTimer && visibilityTimerTTL === ttl) {
            // Already running with the same TTL → leave it alone.
            // This preserves the countdown phase when the user merely
            // enables the visibility filter on an additional type.
            console.log('[SkyCulture] Visibility timer already running with same TTL — no restart');
            return;
        }
        
        // Either no timer, or TTL changed → (re)start.
        startVisibilityTimer();
				
        // ✅ Single fetch, broadcast to all active tabs
        refreshAllVisibilitySnapshots();
    }

    /**
     * Start (or restart) the visibility auto-refresh timer based on the
     * current radio mode and the current visibility-filter state.
     * 
     * Behavior:
     *  - If mode is "manual" (TTL = 0) → no timer; countdown shows "—".
     *  - If no type has the visibility filter enabled → no timer;
     *    countdown shows "—" (nothing to refresh).
     *  - Otherwise → start a setInterval with the selected TTL.
     * 
     * IMPORTANT: This function ALWAYS stops the previous timer first.
     * Callers that want to preserve the running timer (when TTL is
     * unchanged) should use `syncVisibilityTimer()` instead — it
     * performs the "should I restart?" decision before calling this.
     * 
     * The timer checks every "tick" whether any type has visibility
     * active. For active types, it invalidates `_aboveHorizonAt` on
     * all patterns and re-renders the panel. `needsFetch` will then
     * trigger a fetch.
     * 
     * This is more robust than scheduling a re-render at exactly TTL
     * milliseconds, because the user may change filters between ticks.
     */
    function startVisibilityTimer() {
        // Always stop the previous timer first
        stopVisibilityTimer();
        
        var ttl = getVisibilityTTL();
        
        // Manual mode or invalid TTL → no timer
        if (!ttl || ttl <= 0) {
            console.log('[SkyCulture] Visibility timer not started (manual mode)');
            // Still start the countdown display so it shows "—"
            startCountdownDisplay();
            return;
        }
        
        // No type is filtering by visibility → timer has nothing to do
        if (!anyTypeHasVisibilityFilter()) {
            console.log('[SkyCulture] Visibility timer not started (no visible filter active)');
            startCountdownDisplay();
            return;
        }
        
        console.log('[SkyCulture] Visibility timer started with TTL:', ttl + 'ms');
        
        // Remember which TTL the timer is running with, so that
        // syncVisibilityTimer() can decide whether to restart later.
        visibilityTimerTTL = ttl;
        
        // Reset the countdown to start from full TTL
        resetCountdown();
        
        // Start the countdown display (updates every 1s)
        startCountdownDisplay();
        
        visibilityTimer = setInterval(function() {
            console.log('[SkyCulture] Visibility timer tick');

            // ✅ Single fetch, broadcast to all active tabs
            refreshAllVisibilitySnapshots();
            
            var filterKeys = ['constellation', 'asterism', 'lunar', 'star'];
            var refreshed = false;
            
            for (var i = 0; i < filterKeys.length; i++) {
                var key = filterKeys[i];
                var filterState = State.raw.filters[key];
                
                if (!filterState || !filterState.visible) continue;

                var patterns = getPatternsForType(key);
                if (patterns && patterns.length) {
                    for (var p = 0; p < patterns.length; p++) {
                        if (patterns[p]._info) {
                            patterns[p]._info._aboveHorizonAt = 0;
                        }
                    }
                }
                
                reRenderPanelForType(key);
                refreshed = true;
            }
            
            // Reset countdown after each tick
            resetCountdown();
            
            if (!refreshed) {
                console.log('[SkyCulture] Visibility timer tick — no active panels');
            }
        }, ttl);
    }

    /**
     * Fetch fresh status and update the snapshot display for a given type.
     * 
     * Called from:
     *   - onFilterChange (when checkbox is turned ON)
     *   - visibilityTimer tick
     *   - onManualRefresh
     * 
     * The result is cached in `_lastStatusCache` for consumers that
     * need it synchronously (e.g. the display, or a follow-up render).
     * 
     * @param {string} type - Object type ('constellation', 'asterism', 'lunar', 'star')
     * @returns {Promise<Object|null>} The fetched status (or null on error)
     */
    function refreshVisibilitySnapshot(type) {
        var prefix = getPrefixFromType(type);
        if (!prefix) return Promise.resolve(null);
        
        return Api.fetchStatus().then(function(status) {
            _lastStatusCache = status;
            Help.updateVisibilitySnapshot(prefix, status);
            return status;
        }).catch(function(err) {
            console.warn('[SkyCulture] refreshVisibilitySnapshot failed:', err);
            Help.updateVisibilitySnapshot(prefix, null);
            return null;
        });
    }

    /**
     * Fetch fresh status ONCE and update the snapshot display for ALL
     * active visibility-filtered types.
     * 
     * This is the preferred entry point for:
     *   - visibilityTimer tick (60s / 180s / 420s)
     *   - syncVisibilityTimer (mode change, initial start)
     *   - onFilterChange (when enabling a filter)
     * 
     * Rationale: a single /api/main/status request produces one
     * consistent snapshot of (location + time + jday). Broadcasting
     * that same snapshot to all active tabs avoids:
     *   - N redundant AJAX requests (one per active type)
     *   - Slight inconsistencies between tabs (different jday values)
     * 
     * @returns {Promise<Object|null>} The fetched status (or null)
     */
    function refreshAllVisibilitySnapshots() {
        var activeTypes = [];
        var filterKeys = ['constellation', 'asterism', 'lunar', 'star'];
        for (var i = 0; i < filterKeys.length; i++) {
            var fs = State.raw.filters[filterKeys[i]];
            if (fs && fs.visible) {
                activeTypes.push(filterKeys[i]);
            }
        }
        
        if (!activeTypes.length) {
            // Nothing to refresh — leave displays as they are ("—")
            return Promise.resolve(null);
        }
        
        return Api.fetchStatus().then(function(status) {
            _lastStatusCache = status;
            
            // Broadcast the same snapshot to every active tab
            for (var j = 0; j < activeTypes.length; j++) {
                var prefix = getPrefixFromType(activeTypes[j]);
                if (prefix) {
                    Help.updateVisibilitySnapshot(prefix, status);
                }
            }
            return status;
        }).catch(function(err) {
            console.warn('[SkyCulture] refreshAllVisibilitySnapshots failed:', err);
            // Clear displays on all active tabs
            for (var k = 0; k < activeTypes.length; k++) {
                var prefix = getPrefixFromType(activeTypes[k]);
                if (prefix) {
                    Help.updateVisibilitySnapshot(prefix, null);
                }
            }
            return null;
        });
    }
		
    /**
     * Clear the snapshot display for a given type.
     * Called when the visibility filter is turned OFF.
     * 
     * @param {string} type - Object type
     */
    function clearVisibilitySnapshotForType(type) {
        var prefix = getPrefixFromType(type);
        if (prefix) {
            Help.clearVisibilitySnapshot(prefix);
        }
    }
		
    // =====================================================================
    // VISIBILITY / CACHE HELPERS
    // =====================================================================
    // 
    // These helpers implement the smart caching strategy that avoids
    // unnecessary API fetches:
    // 
    //  - `_info` is cached on each pattern after the first fetch.
    //  - Fields other than `aboveHorizon` are immutable and never need
    //    to be re-fetched.
    //  - `aboveHorizon` is refreshed only when the user has enabled the
    //    "Visible above horizon only" filter, and only after the TTL
    //    selected by the user has elapsed.
    //  - When the culture changes, all `_info` is discarded.
    // 
    // The result: filtering by name mode, sort, or star type is INSTANT
    // after the first fetch, and visibility refresh is throttled by the
    // user's chosen TTL.
    // =====================================================================

    /**
     * Get the current visibility cache TTL based on the selected radio mode.
     * 
     * Reads the value from the shared radio group. If no radio is selected,
     * falls back to the default (30 seconds).
     * 
     * @returns {number} TTL in milliseconds. 0 means "manual mode" (no
     *                   automatic refresh).
     */
    function getVisibilityTTL() {
        var $selected = $('input[name="visibility-refresh-mode"]:checked');
        if (!$selected.length) {
            return Config.VISIBILITY_CACHE_TTL_DEFAULT;
        }
        
        var mode = $selected.val();
        var ttl = Config.VISIBILITY_CACHE_MODES[mode];
        
        return (typeof ttl === 'number') ? ttl : Config.VISIBILITY_CACHE_TTL_DEFAULT;
    }

    /**
     * Check whether ANY of the pattern types currently has the
     * "visible above horizon only" filter enabled.
     * 
     * The visibility timer only makes sense when at least one type is
     * actively filtering by visibility. When no type is filtering,
     * running the timer would just re-render panels without fetching
     * anything useful — wasting CPU and confusing the user with a
     * countdown that has no effect.
     * 
     * @returns {boolean} True if at least one type has `visible === true`
     */
    function anyTypeHasVisibilityFilter() {
        var keys = ['constellation', 'asterism', 'lunar', 'star'];
        for (var i = 0; i < keys.length; i++) {
            var fs = State.raw.filters[keys[i]];
            if (fs && fs.visible) return true;
        }
        return false;
    }
		
    /**
     * Get the list of `_info` fields required by the current filter state.
     * 
     * This is the core of the smart caching logic. We only fetch if a
     * required field is missing from the cache.
     * 
     * @param {Object} filterState - Filter state for the current type
     * @param {string} type - Object type ('constellation', 'asterism', ...)
     * @returns {Array<string>} List of required field names
     */
    function getRequiredFields(filterState, type) {
        var fields = ['found'];   // Always required
        
        if (filterState.visible) {
            fields.push('aboveHorizon');
        }
        
        if (filterState.nameMode === 'localized') {
            fields.push('localizedName');
        }
        if (filterState.nameMode === 'cultural') {
            fields.push('culturalNames');
        }
        
        if (filterState.sortBy === 'constellation') {
            fields.push('iauConstellation');
        }
        if (filterState.sortBy === 'magnitude') {
            fields.push('vmag');
        }
        if (filterState.sortBy === 'type') {
            fields.push('objectType');
        }
        
        if (type === 'star' && filterState.starType && filterState.starType !== 'all') {
            fields.push('starType', 'variableStar');
        }
        
        return fields;
    }

    /**
     * Decide whether an API fetch is needed for the given patterns and
     * filter state.
     * 
     * Rules (in order of priority):
     * 
     *  1. If ANY pattern lacks `_info` → fetch all.
     *     (First time loading this culture.)
     * 
     *  2. If visibility filter is active AND the TTL is not manual:
     *     - If ANY `_info` is missing `_aboveHorizonAt` → fetch all.
     *     - If ANY `_info` is older than the TTL → fetch all.
     *     (Visibility changes with time.)
     * 
     *  3. If ANY required field is missing from `_info` → fetch all.
     *     (User enabled a new filter that needs a field not yet fetched.)
     * 
     *  4. Otherwise → no fetch needed.
     * 
     * @param {Array} patterns - Pattern array
     * @param {Object} filterState - Filter state for the type
     * @param {string} type - Object type
     * @returns {{fetch: boolean, reason: string, scope: string}}
     */
		function needsFetch(patterns, filterState, type) {
				// ============================================================
				// RULE 0: LAZY LOADING
				// ============================================================
				// If the filter state is fully "default" (no visibility, no special
				// name mode, no special sort), we DON'T need any API data.
				// The rendering layer will fall back to `pattern.name`, which is
				// already available from index.json.
				// 
				// This prevents the initial page-load storm of more than 400+ requests for
				// large cultures.
				// ============================================================
				var needsAnyApi = filterState.visible ||
												 (filterState.nameMode && filterState.nameMode !== 'default') ||
												 (filterState.sortBy && filterState.sortBy !== 'name') ||
												 (type === 'star' && filterState.starType && filterState.starType !== 'all');
				
				if (!needsAnyApi) {
						return { fetch: false, reason: 'lazy_default', scope: 'none' };
				}
				
				// ============================================================
				// RULE 1: Missing _info
				// ============================================================
				for (var i = 0; i < patterns.length; i++) {
						if (patterns[i]._info === undefined) {
								return { fetch: true, reason: 'missing_info', scope: 'all' };
						}
				}
				
				// ============================================================
				// RULE 2: Visibility (above-horizon)
				// ============================================================
				// NOTE: This rule runs even when ttl === 0 (manual mode), to
				// detect the "invalidate" signal from the manual Refresh button
				// (which sets _aboveHorizonAt = 0).
				// ============================================================
				if (filterState.visible) {
						var ttl = getVisibilityTTL();
						var now = Date.now();
						
						for (var i = 0; i < patterns.length; i++) {
								var info = patterns[i]._info;
								if (!info) continue;   // already covered by Rule 1
								
								// ---- Manual invalidation (works even in manual mode) ----
								if (!info._aboveHorizonAt || info._aboveHorizonAt === 0) {
										return { fetch: true, reason: 'missing_visibility', scope: 'all' };
								}
								
								// ---- Time-based staleness (only if TTL > 0) ----
								if (ttl > 0 && (now - info._aboveHorizonAt > ttl)) {
										return { fetch: true, reason: 'stale_visibility', scope: 'all' };
								}
						}
				}
				
				// ============================================================
				// RULE 3: Missing required field
				// ============================================================
				var required = getRequiredFields(filterState, type);
				for (var i = 0; i < patterns.length; i++) {
						var info = patterns[i]._info;
						if (!info) continue;
						
						for (var f = 0; f < required.length; f++) {
								if (info[required[f]] === undefined) {
										return { fetch: true, reason: 'missing_field:' + required[f], scope: 'all' };
								}
						}
				}
				
				// ============================================================
				// RULE 4: All cached
				// ============================================================
				return { fetch: false, reason: 'all_cached', scope: 'none' };
		}

    /**
     * Build an infoMap from the cached `_info` on patterns.
     * 
     * Used when no fetch is needed — the caller already has all the
     * data required for filtering and sorting.
     * 
     * @param {Array} patterns - Pattern array with `_info` cached
     * @returns {Object.<string, Object>} Map of patternId → info
     */
    function buildInfoMapFromCache(patterns) {
        var infoMap = {};
        for (var i = 0; i < patterns.length; i++) {
            var p = patterns[i];
            infoMap[p.id] = p._info || null;
        }
        return infoMap;
    }

    /**
     * Store fetched `_info` on patterns and stamp the cache metadata.
     * 
     * @param {Array<{pattern, info}>} enrichedData - Result from fetch
     */
    function cacheInfoOnPatterns(enrichedData) {
        var now = Date.now();
        for (var i = 0; i < enrichedData.length; i++) {
            var item    = enrichedData[i];
            var pattern = item.pattern || item.item;
            var info    = item.info;
            
            if (!pattern) continue;
            
            if (info) {
                // Stamp cache metadata on the info object
                info._fetchedAt      = now;
                info._aboveHorizonAt = now;
                pattern._info        = info;
            } else {
                // Mark as "tried, no result" so we don't retry infinitely
                pattern._info = null;
            }
        }
    }

    /**
     * Trigger a re-render of the given type's panel.
     * Used by control handlers.
     * 
     * @param {string} type - Object type
     */
    function reRenderPanelForType(type) {
        switch (type) {
            case 'constellation':
                renderConstellationsPanel(State.raw.original.constellations);
                break;
            case 'asterism':
                renderAsterismsPanel(State.raw.original.asterisms);
                break;
            case 'lunar':
                renderLunarPanel(State.raw.original.lunar);
                break;
            case 'star':
                renderStarsPanel(State.raw.original.stars);
                break;
            case 'zodiac':
                renderZodiacPanel(State.raw.data.zodiac);
                break;
        }
    }
    
    /**
     * Map a filterIdPrefix to the corresponding type key.
     * 
     * @param {string} prefix - Filter ID prefix
     * @returns {string|null}
     */
    function prefixToType(prefix) {
        var map = {
            'constellation': 'constellation',
            'asterisms':     'asterism',
            'lunar':         'lunar',
            'stars':         'star',
            'zodiac':        'zodiac'
        };
        return map[prefix] || null;
    }
		
    // =====================================================================
    // UNIFIED PANEL RENDER HELPER
    // =====================================================================

    /**
     * Generic panel renderer with smart caching.
     * 
     * Renders any pattern type (constellation, asterism, lunar, star, zodiac)
     * using a unified pipeline:
     * 
     *  1. Decide if an API fetch is needed (via `needsFetch`).
     *  2. If needed, fetch and cache `_info` on each pattern.
     *  3. Build an infoMap (from cache or fresh fetch).
     *  4. Apply filters via `Filters.applyAll`.
     *  5. Render the result (grid or grouped) with multi-select support.
     *  6. Rebuild the Sync button index.
     * 
     * This replaces the previous implementation that always fetched when
     * any filter was active, causing ~19-second delays for large cultures.
     * 
     * @param {Object} opts - Render options:
     *   - type:            'constellation' | 'asterism' | 'lunar' | 'star' | 'zodiac'
     *   - countKey:        Counter badge ID suffix (e.g. 'constellations')
     *   - $container:      Target container
     *   - patterns:        Pattern array
     *   - filterState:     Filter state object
     *   - isGrouped:       Whether grouped rendering is supported (stars only)
     *   - onClick:         Click handler for pattern buttons
     *   - placeholderText: Empty-state message
     */
    function renderPanel(opts) {
        var $container = opts.$container;
        var patterns   = opts.patterns;
        var type       = opts.type;
        var filterState= opts.filterState;
        var countKey   = opts.countKey || type + 's';

        // -----------------------------------------------------------------
        // Empty state
        // -----------------------------------------------------------------
        if (!patterns || !patterns.length) {
            Render.renderPlaceholder($container, opts.placeholderText);
            Render.updatePatternCount(countKey, 0);
            Sync.rebuildIndex();
            return;
        }

        var multiSelectActive = MultiSelect.isActive(type);

        // -----------------------------------------------------------------
        // Decide if fetch is needed
        // -----------------------------------------------------------------
        var decision = needsFetch(patterns, filterState, type);
        console.log('[SkyCulture] renderPanel:', type, '→', decision.reason);

        if (!decision.fetch) {
						// ============================================================
						// FAST PATH
						// ============================================================
						// Two sub-cases:
						//  A. decision.reason === 'lazy_default'
						//     → `_info` is undefined for all patterns. `buildInfoMapFromCache`
						//       will return null for each, and Filters will fall back to
						//       `pattern.name`. No API calls made.
						//  B. decision.reason === 'all_cached'
						//     → `_info` is cached and fresh. All filtering/sorting is done
						//        from cache. No API calls made.
						// ============================================================
            var infoMap = buildInfoMapFromCache(patterns);
            var result  = Filters.applyAll(patterns, filterState, infoMap, type);
						// ✅ Save the displayed order for MultiSelect to use later
						if (State.raw.displayed) {
								var DISPLAYED_WRITE_KEY = {
										constellation: 'constellations',
										asterism:      'asterisms',
										lunar:         'lunar',
										star:          'stars'
								};
								var dk = DISPLAYED_WRITE_KEY[type];
								if (dk && State.raw.displayed) {
										State.raw.displayed[dk] = result.patterns.slice();
								}
						}            
            // Update tab-header counter
            if (filterState.visible) {
                Render.updatePatternCount(countKey,
                    result.visibleCount + '/' + result.totalCount);
            } else {
                Render.updatePatternCount(countKey, result.patterns.length);
            }
						
            // Update control-panel item counter
            var prefix = getPrefixFromType(type);
            if (filterState.visible) {
                Render.updateVisibilityItemCount(prefix,
                    result.visibleCount, result.totalCount, true);
            } else {
                Render.updateVisibilityItemCount(prefix, 0, 0, false);
            }
            
            // Render
            renderResult($container, result, type, opts, multiSelectActive);
            
            // Rebuild Sync index
            Sync.rebuildIndex();
            return;
        }

        // -----------------------------------------------------------------
        // SLOW PATH: Fetch needed
        // -----------------------------------------------------------------
        // Show loading indicator with progress
        var loadingLabel = (decision.reason === 'stale_visibility' ||
                            decision.reason === 'missing_visibility')
            ? tr("Refreshing visibility...")
            : (decision.reason === 'missing_info')
                ? tr("Loading...")
                : tr("Loading...");

        Render.renderPlaceholder($container, loadingLabel + ' 0/' + patterns.length);

        Api.fetchVisibilityBatch(patterns, function(loaded, total) {
            Render.renderPlaceholder($container,
                loadingLabel + ' ' + loaded + '/' + total);
        }).then(function(enrichedData) {
            // Cache `_info` on each pattern
            cacheInfoOnPatterns(enrichedData);
            
            // Build infoMap from freshly cached `_info`
            var infoMap = buildInfoMapFromCache(patterns);
            
            // Apply filters
            var result = Filters.applyAll(patterns, filterState, infoMap, type);
        // ✅ Same save here
        if (State.raw.displayed) {
						var DISPLAYED_WRITE_KEY = {
								constellation: 'constellations',
								asterism:      'asterisms',
								lunar:         'lunar',
								star:          'stars'
						};
						var dk = DISPLAYED_WRITE_KEY[type];
						if (dk && State.raw.displayed) {
								State.raw.displayed[dk] = result.patterns.slice();
						}
        }            
            // Update counter
            if (filterState.visible) {
                Render.updatePatternCount(countKey,
                    result.visibleCount + '/' + result.totalCount);
            } else {
                Render.updatePatternCount(countKey, result.patterns.length);
            }

            // Update control-panel item counter
            var prefix = getPrefixFromType(type);
            if (filterState.visible) {
                Render.updateVisibilityItemCount(prefix,
                    result.visibleCount, result.totalCount, true);
            } else {
                Render.updateVisibilityItemCount(prefix, 0, 0, false);
            }
            
            // Render
            renderResult($container, result, type, opts, multiSelectActive);
            
            // Rebuild Sync index
            Sync.rebuildIndex();
            
        }).catch(function(error) {
            console.error('[SkyCulture] Panel render error:', type, error);
            Render.renderPlaceholder($container, tr("Error loading data"));
            Sync.rebuildIndex();
        });
    }

    /**
     * Map a type key to its filter ID prefix.
     * Used in update control-panel item counter
		 *
     * @param {string} type - Object type
     * @returns {string|null}
     */
    function getPrefixFromType(type) {
        var map = {
            'constellation': 'constellation',
            'asterism':      'asterisms',
            'lunar':         'lunar',
            'star':          'stars',
            'zodiac':        'zodiac'
        };
        return map[type] || null;
    }

    /**
     * Render the result of applying filters.
     * Handles both grouped and flat rendering with multi-select support.
     * 
     * @param {jQuery} $container - Target container
     * @param {Object} result - Result from Filters.applyAll
     * @param {string} type - Object type
     * @param {Object} opts - Original render options
     * @param {boolean} multiSelectActive - Whether multi-select mode is active
     */
    function renderResult($container, result, type, opts, multiSelectActive) {
        if (opts.isGrouped && result.grouped) {
            // Grouped rendering (stars only)
            Render.renderGroupedStars($container, result.grouped, {
                tr:      tr,
                onClick: opts.onClick
            });
            if (multiSelectActive) {
                MultiSelect.attachHandlers(type, $container);
            }
        } else {
            // Flat rendering
            Render.renderPatternGrid($container, result.patterns, type, {
                tr:             tr,
                onClick:        opts.onClick,
                disableHandler: multiSelectActive
            });
            if (multiSelectActive) {
                MultiSelect.attachHandlers(type, $container);
            }
        }
    }

    // =====================================================================
    // PANEL RENDERERS
    // =====================================================================

    /**
     * Render the Constellations panel.
     * 
     * @param {Array} patterns - Constellation patterns
     */
    function renderConstellationsPanel(patterns) {
        renderPanel({
            type:            'constellation',
            countKey:        'constellations',
            $container:      State.raw.ui.containers.constellations,
            patterns:        patterns,
            filterState:     State.raw.filters.constellation,
            onClick:         handleConstellationClick,
            placeholderText: tr("No constellations available for this culture")
        });
    }

    /**
     * Handle click on a constellation button.
     */
    function handleConstellationClick(data) {
        if (data.isActive) {
            State.clearSelection();
            Render.clearAllActiveButtons();
            stelUtils.clearConstellationHighlight();
        } else {
            State.setSelection(data.name, data.id, 'constellation');
            Render.highlightButton(State.raw.ui.containers.constellations, data.id, data.name);
            stelUtils.toggleConstellationHighlight(
                data.searchName || data.name,
                data.id,
                State.raw.fov.constellation
            );
        }
    }

    /**
     * Render the Asterisms panel.
     */
    function renderAsterismsPanel(patterns) {
        renderPanel({
            type:            'asterism',
            countKey:        'asterisms',
            $container:      State.raw.ui.containers.asterisms,
            patterns:        patterns,
            filterState:     State.raw.filters.asterism,
            onClick:         handleAsterismClick,
            placeholderText: tr("No asterisms available for this culture")
        });
    }

    /**
     * Handle click on an asterism button.
     */
    function handleAsterismClick(data) {
        if (data.isActive) {
            State.clearSelection();
            Render.clearAllActiveButtons();
            stelUtils.clearSelection();
        } else {
            State.setSelection(data.name, data.id, 'asterism');
            Render.highlightButton(State.raw.ui.containers.asterisms, data.id, data.name);
            stelUtils.goToAsterism(
                data.searchName || data.name,
                data.id,
                State.raw.fov.asterism
            );
        }
    }

    /**
     * Render the Zodiac panel.
     */
    function renderZodiacPanel(patterns) {
        renderPanel({
            type:            'zodiac',
            countKey:        'zodiac',
            $container:      State.raw.ui.containers.zodiac,
            patterns:        patterns,
            filterState:     State.raw.filters.zodiac || { visible: false, nameMode: 'default' },
            onClick:         handleZodiacClick,
            placeholderText: tr("No zodiac signs available for this culture")
        });
    }

    /**
     * Handle click on a zodiac button.
     */
    function handleZodiacClick(data) {
        if (data.isActive) {
            State.clearSelection();
            Render.clearAllActiveButtons();
            stelUtils.clearSelection();
        } else {
            State.setSelection(data.name, data.id, 'zodiac');
            Render.highlightButton(State.raw.ui.containers.zodiac, data.id, data.name);
            stelUtils.goToZodiacSign(
                data.searchName || data.name,
                data.id
            );
        }
    }

    /**
     * Render the Lunar Mansions panel.
     */
    function renderLunarPanel(patterns) {
        renderPanel({
            type:            'lunar',
            countKey:        'lunar',
            $container:      State.raw.ui.containers.lunar,
            patterns:        patterns,
            filterState:     State.raw.filters.lunar,
            onClick:         handleLunarClick,
            placeholderText: tr("No lunar mansions available for this culture")
        });
    }

    /**
     * Handle click on a lunar mansion button.
     */
    function handleLunarClick(data) {
        if (data.isActive) {
            State.clearSelection();
            Render.clearAllActiveButtons();
            stelUtils.clearSelection();
        } else {
            State.setSelection(data.name, data.id, 'lunar');
            Render.highlightButton(State.raw.ui.containers.lunar, data.id, data.name);
            stelUtils.goToLunarMansion(
                data.searchName || data.name,
                data.id,
                State.raw.fov.lunar
            );
        }
    }

    /**
     * Render the Stars panel.
     */
    function renderStarsPanel(patterns) {
        renderPanel({
            type:            'star',
            countKey:        'stars',
            $container:      State.raw.ui.containers.stars,
            patterns:        patterns,
            filterState:     State.raw.filters.star,
            isGrouped:       true,
            onClick:         handleStarClick,
            placeholderText: tr("No star names available for this culture")
        });
    }

    /**
     * Handle click on a star button.
     */
    function handleStarClick(data) {
        if (data.isActive) {
            State.clearSelection();
            Render.clearAllActiveButtons();
            stelUtils.clearSelection();
        } else {
            State.setSelection(data.name, data.id, 'star');
            Render.highlightButton(State.raw.ui.containers.stars, data.id, data.name);
            stelUtils.goToObject(
                data.id,
                State.raw.fov.star,
                'star',
                State.raw.data.raw
            );
        }
    }

    // =====================================================================
    // CULTURE SWITCHING
    // =====================================================================

    /**
     * Set the current sky culture.
     * 
     * @param {string} cultureId - Culture ID
     */
		function setCulture(cultureId) {
				if (!cultureId || cultureId === State.raw.culture.id) return;

				console.log('[SkyCulture] User-initiated culture change to:', cultureId);

				// -----------------------------------------------------------------
				// 1. Update LOCAL state FIRST (before server call)
				// -----------------------------------------------------------------
				// This prevents the stelPropertyChanged listener from firing
				// redundantly when the server echoes our own change back.
				// -----------------------------------------------------------------
				State.raw.culture.id = cultureId;
				
				var cultures = State.raw.culture.list || [];
				State.raw.culture.name = cultureId;   // fallback
				for (var i = 0; i < cultures.length; i++) {
						if (cultures[i].id === cultureId) {
								State.raw.culture.name = cultures[i].name;
								break;
						}
				}

				// -----------------------------------------------------------------
				// 2. Update UI: culture buttons active state
				// -----------------------------------------------------------------
				Render.updateCultureButtonStates(cultureId);

				// -----------------------------------------------------------------
				// 3. Update info iframe with cache buster
				// -----------------------------------------------------------------
				if (State.raw.ui.infoFrame && State.raw.ui.infoFrame.length) {
						var baseSrc = Config.API_ENDPOINTS.skyCultureDescription;
						var newSrc = baseSrc + '?culture=' + encodeURIComponent(cultureId) +
												 '&t=' + Date.now();
						State.raw.ui.infoFrame.attr('src', newSrc);
				}

				// -----------------------------------------------------------------
				// 4. Clear selections
				// -----------------------------------------------------------------
				State.clearSelection();
				MultiSelect.clearAll();
				Render.clearAllActiveButtons();

				// -----------------------------------------------------------------
				// 5. Send to server (AFTER local state is updated)
				// -----------------------------------------------------------------
				try {
						propApi.setStelProp('StelSkyCultureMgr.currentSkyCultureID', cultureId);
				} catch (e) {
						console.warn('[SkyCulture] Could not set culture via propApi:', e);
				}

				// -----------------------------------------------------------------
				// 6. Reload all data
				// -----------------------------------------------------------------
				loadAllCultureData();
		}

    // =====================================================================
    // CONTROL PANEL HANDLERS
    // =====================================================================

    /**
     * Map control panel type key to filter key in State.
     * 
     * @param {string} type
     * @returns {string}
     */
    function typeKeyToFilter(type) {
        var map = {
            'constellation': 'constellation',
            'asterism':      'asterism',
            'lunar':         'lunar',
            'star':          'star'
        };
        return map[type] || type;
    }

    /**
     * Control panel event handlers passed to Help.installControlPanel.
     * 
     * These handlers are called when the user interacts with the
     * filter/sort/name-mode controls. They update the filter state
     * and trigger a re-render of the affected panel.
     * 
     * IMPORTANT: Re-rendering does NOT trigger a fetch unless:
     *   - The `visible` filter is being toggled and the cache is stale.
     *   - A required field for the new filter state is missing.
     * 
     * This means filtering is INSTANT for all cases except the first
     * time visibility is enabled.
     */
    var controlHandlers = {
        /**
         * Called when the "Visible above horizon only" checkbox is toggled.
         * 
         * This is the ONLY filter that may trigger a fetch (because
         * aboveHorizon changes with time). It also drives the lifecycle
         * of the auto-refresh timer: when the first type enables the
         * filter, the timer starts; when the last type disables it,
         * the timer stops.
         */
        onFilterChange: function(type, value) {
            var filterKey = typeKeyToFilter(type);
            if (filterKey && State.raw.filters[filterKey]) {
                State.raw.filters[filterKey].visible = value;
            }
            
            // Reconcile the auto-refresh timer with the new filter state.
            // This starts the timer on first enable, and stops it when
            // the last active type is disabled.
            syncVisibilityTimer();

            // Refresh or clear snapshot displays
            if (value) {
                // Turning ON → fetch once, broadcast to all active tabs
                // (including the one just enabled)
                refreshAllVisibilitySnapshots();
            } else {
                // Turning OFF → clear this type's display only
                clearVisibilitySnapshotForType(type);
            }
            
            // Re-render the affected panel (triggers a fetch if needed).
            reRenderPanelForType(type);
        },

        /**
         * Called when the name display mode is changed.
         * 
         * No fetch is triggered if the corresponding field is already
         * cached. Otherwise, a single fetch will populate it.
         */
        onNameModeChange: function(type, value) {
            var filterKey = typeKeyToFilter(type);
            if (filterKey && State.raw.filters[filterKey]) {
                State.raw.filters[filterKey].nameMode = value;
            }
            reRenderPanelForType(type);
        },

        /**
         * Called when sort criterion or direction is changed.
         * Never triggers a fetch if data is cached.
         */
        onSortChange: function(type, sortBy, sortDir) {
            var filterKey = typeKeyToFilter(type);
            var fs = State.raw.filters[filterKey];
            if (fs) {
                fs.sortBy  = sortBy;
                fs.sortDir = sortDir;
            }
        },

        /**
         * Called when the user clicks "Apply" to apply sort.
         */
        onApplySort: function(type) {
            reRenderPanelForType(type);
        },

        /**
         * Called when the Navigation FOV spinner changes.
         * Only updates state — no re-render needed.
         */
        onFovChange: function(type, value) {
            var fovKey = type === 'star' ? 'star' : type;
            if (typeof State.raw.fov[fovKey] !== 'undefined') {
                State.raw.fov[fovKey] = value;
            }
        },

        /**
         * Called when the multi-select toggle is changed.
         */
        onMultiSelectChange: function(type, value) {
            if (value) {
                MultiSelect.enter(type);
            } else {
                MultiSelect.exit(type);
            }
            reRenderPanelForType(type);
        },

        /**
         * Called when the star type filter is changed.
         * No fetch if data is cached.
         */
        onStarTypeChange: function(type, value) {
            if (State.raw.filters.star) {
                State.raw.filters.star.starType = value;
            }
            reRenderPanelForType(type);
        },

        /**
         * Called when the visibility refresh mode radio is changed.
         * 
         * Behavior:
         *  - Reconcile the timer with the new TTL / manual mode.
         *  - For non-manual modes, immediately refresh any active panels
         *    whose data is now considered stale under the new TTL.
         * 
         * @param {string} mode - New mode ('60s', '180s', '420s', 'manual')
         */
        onVisibilityModeChange: function(mode) {
            console.log('[SkyCulture] Visibility mode changed to:', mode);
            
            // Reconcile the timer: this handles BOTH cases:
            //  - mode === 'manual'  → stops the timer
            //  - mode === '60s' etc → starts/restarts with the new TTL,
            //    but only if at least one type has the filter enabled.
            syncVisibilityTimer();
            
            // For non-manual modes, check each active panel and refresh
            // if stale under the new TTL.
            if (mode !== 'manual') {
                var filterKeys = ['constellation', 'asterism', 'lunar', 'star'];
                for (var i = 0; i < filterKeys.length; i++) {
                    var key = filterKeys[i];
                    if (State.raw.filters[key] && State.raw.filters[key].visible) {
                        reRenderPanelForType(key);
                    }
                }
            }
        },

        /**
         * Called when the manual "Refresh" button is clicked.
         * Forces a re-fetch of visibility data.
         * 
         * @param {string} prefix - Filter ID prefix (e.g. 'constellation')
         */
        onManualRefresh: function(prefix) {
            var type = prefixToType(prefix);
            if (!type) return;
            
            console.log('[SkyCulture] Manual visibility refresh for:', type);

            // ✅ Refresh the snapshot display
            refreshVisibilitySnapshot(type);
            
            // Invalidate `_aboveHorizonAt` on all patterns of this type
            // so that the next render will trigger a fetch.
            var patterns = getPatternsForType(type);
            if (patterns && patterns.length) {
                for (var i = 0; i < patterns.length; i++) {
                    if (patterns[i]._info) {
                        patterns[i]._info._aboveHorizonAt = 0;
                    }
                }
            }
            // Reset countdown after manual refresh
            resetCountdown();            
            // Re-render will now fetch fresh data
            reRenderPanelForType(type);
        }
    };

    /**
     * Get the original (unfiltered) patterns for a type.
     * 
     * @param {string} type - Object type
     * @returns {Array}
     */
    function getPatternsForType(type) {
        switch (type) {
            case 'constellation': return State.raw.original.constellations;
            case 'asterism':      return State.raw.original.asterisms;
            case 'lunar':         return State.raw.original.lunar;
            case 'star':          return State.raw.original.stars;
            case 'zodiac':        return State.raw.data.zodiac;
            default:              return [];
        }
    }

    // =====================================================================
    // TOUR TOOLBARS
    // =====================================================================

    /**
     * Build the shared tour toolbar content for a given type.
     * 
     * The generated HTML is bound via $toolbar.find() so that the
     * handlers are attached to the correct (in-memory) elements.
     * 
     * @param {string} type - Tour type key
     * @param {jQuery} $toolbar - Toolbar container (already in DOM)
     * @param {Function} generateFn - Tour generation function
     */
    function buildTourToolbarContent(type, $toolbar, generateFn) {
        var configs = {
            constellation: {
                buttonId:    'btn-generate-const-static',
                buttonLabel: tr("Generate Constellations Tour Script"),
                buttonTitle: tr("Generates script with embedded constellation name array"),
                useSaveId:   'const-tour-use-savestate',
                genNotif:    'Constellations tour script generated'
            },
            asterism: {
                buttonId:    'btn-generate-aster-static',
                buttonLabel: tr("Generate Asterisms Tour Script"),
                buttonTitle: tr("Generates script with embedded asterism name array"),
                useSaveId:   'aster-tour-use-savestate',
                genNotif:    'Asterisms tour script generated'
            },
            lunar: {
                buttonId:    'btn-generate-lunar-static',
                buttonLabel: tr("Generate Lunar Mansions Tour Script"),
                buttonTitle: tr("Generates script with embedded lunar mansion name array"),
                useSaveId:   'lunar-tour-use-savestate',
                genNotif:    'Lunar mansions tour script generated'
            },
            star: {
                buttonId:    'btn-generate-stars-static',
                buttonLabel: tr("Generate Stars Tour Script"),
                buttonTitle: tr("Generates script with embedded star/object array"),
                useSaveId:   'stars-tour-use-savestate',
                genNotif:    'Stars tour script generated'
            }
        };

        var cfg = configs[type];
        if (!cfg) return;

        // Bind the generate button - using $toolbar.find() so the handler
        // is attached to the correct in-memory element
        $toolbar.find('#' + cfg.buttonId).off('click.tourGen').on('click.tourGen', function(e) {
            e.preventDefault();
            e.stopPropagation();

            console.log('[SkyCulture:Tours] Generate clicked for type:', type);

            try {
                var script = generateFn();
                if (script) {
                    Tours.notify(cfg.genNotif);
                } else {
                    Tours.notify(tr("No data available for this tour"));
                }
            } catch (error) {
                console.error('[SkyCulture:Tours] Error generating script:', error);
                Tours.notify(tr("Error generating tour script"));
            }
        });
    }

		/**
		 * Create the Constellations tour toolbar.
		 * 
		 * Uses the unified buildTourToolbarContent to generate HTML, with
		 * two generate buttons:
		 * - btn-generate-const-dynamic → generateConstellationsTourDynamic
		 * - btn-generate-const-static  → generateConstellationsTourStatic
		 * 
		 * All numeric inputs use the jQuery UI spinner pattern and are
		 * initialized automatically by Tours.createToolbar() after the HTML
		 * is inserted into the DOM.
		 * 
		 * Color groups include:
		 * - RGB sliders with live preview swatch
		 * - Numeric value display (0.00, 0.00, 0.00 format)
		 * - Dynamic section header tinting (text + border-left) via
		 *   data-section-id on the wrapper
		 * 
		 * @function createConstellationsTourToolbar
		 */
		function createConstellationsTourToolbar() {
				Tours.createToolbar({
						containerId:              'const-tour-controls-container',
						sectionTitle:             tr("Constellations Tour Generator"),
						helpTargetId:             'help-constellations',
						helpLabel:                tr("Tour Generator Help"),
						tabSelector:              '#patterns-tab-constellations',
						buttonsContainerSelector: '#constellations-buttons-container',
						tourType:                 'constellation',
						tr:                       tr,
						
						buildContent: function($toolbar, tr) {
								
								// =========================================================
								// COLUMN 1: Script Generation + Timing + Artwork
								// =========================================================
								var col1 = '<div class="tour-column">';
								
								// Row 1: Generate buttons + save_state
								col1 += '<div class="tour-row-separator">' +
										'<button id="btn-generate-const-dynamic" ' +
												'class="jquerybutton pattern-btn" ' +
												'title="' + tr("Uses ConstellationMgr.getConstellationsEnglishNames() - works with any culture") + '">' +
												tr("Generate Dynamic Constellations Tour Script") +
										'</button>' +
										'<button id="btn-generate-const-static" ' +
												'class="jquerybutton pattern-btn" ' +
												'title="' + tr("Embeds constellation names as a static array - portable and self-contained") + '">' +
												tr("Generate Static Array Constellations Tour Script") +
										'</button>' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="const-tour-use-savestate" />' +
												'<label class="tour-control-label" style="font-weight:bold;color:#A6E22E;" for="const-tour-use-savestate">' +
														tr("Use save_state.inc") + ' <span class="tour-multiselect-hint">(Save/restore state via include file)</span>' +
												'</label>' +
										'</div>' +
								'</div>';
								
								// Row 2: Timing
								col1 += '<div class="tour-row">' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="const-tour-duration">' +
														tr("Duration:") +
												'</label>' +
												'<input id="const-tour-duration" ' +
														'class="tour-input-number" ' +
														'value="4" ' +
														'data-min="1" data-max="15" data-step="0.5" ' +
														'data-numberformat="n1"/>' +
												'<span>' + tr("sec") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="const-zoom-fov">' +
														tr("Zoom FOV:") +
												'</label>' +
												'<input id="const-zoom-fov" ' +
														'class="tour-input-number" ' +
														'value="40" ' +
														'data-min="5" data-max="90" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="const-overview-fov">' +
														tr("Overview FOV:") +
												'</label>' +
												'<input id="const-overview-fov" ' +
														'class="tour-input-number" ' +
														'value="90" ' +
														'data-min="20" data-max="120" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
								'</div>';
								
								// Row 3: Selection mode
								col1 += '<div class="tour-row-subtle">' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="const-tour-isolate" checked>' +
												'<label class="tour-control-label" for="const-tour-isolate">' +
														tr("Isolate Selected") +
												'</label>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="const-tour-pick-last">' +
												'<label class="tour-control-label" for="const-tour-pick-last">' +
														tr("Select Last Only") +
												'</label>' +
										'</div>' +
								'</div>';
								
								// ARTWORK section
								col1 += '<div id="const-art-section" class="tour-section-artwork">' +
										'<div class="tour-section-header">' + tr("ARTWORK") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="const-show-art" checked>' +
												'<label class="tour-control-label" for="const-show-art">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="const-art-intensity">' +
														tr("Intensity:") +
												'</label>' +
												'<input type="range" id="const-art-intensity" class="tour-input-range" min="0" max="100" value="80">' +
												'<label class="tour-control-label" for="const-art-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="const-art-fade" ' +
														'class="tour-input-number" ' +
														'value="2.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
								'</div>';
								
								col1 += '</div>';
								
								// =========================================================
								// COLUMN 2: Lines + Labels
								// =========================================================
								var col2 = '<div class="tour-column">';
								
								// LINES section
								col2 += '<div id="const-lines-section" class="tour-section-lines">' +
										'<div class="tour-section-header">' + tr("LINES") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="const-show-lines" checked>' +
												'<label class="tour-control-label" for="const-show-lines">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="const-line-thickness">' +
														tr("Thick:") +
												'</label>' +
												'<input id="const-line-thickness" ' +
														'class="tour-input-number" ' +
														'value="2" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="const-lines-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="const-lines-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="const-lines-section">' +
												'<span class="tour-color-group-label">' + tr("Lines Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="const-lines-color-r" class="tour-color-slider" min="0" max="100" value="30" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="const-lines-color-g" class="tour-color-slider" min="0" max="100" value="60" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="const-lines-color-b" class="tour-color-slider" min="0" max="100" value="90" step="1">' +
														'<div id="const-lines-color-preview" class="tour-color-preview"></div>' +
														'<span id="const-lines-color-value" class="tour-color-value">0.30, 0.60, 0.90</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								// LABELS section
								col2 += '<div id="const-labels-section" class="tour-section-labels">' +
										'<div class="tour-section-header">' + tr("LABELS") + '</div>' +
										'<div class="tour-row">' +
												'<label class="tour-control-label" for="const-font-size">' +
														tr("Size:") +
												'</label>' +
												'<input id="const-font-size" ' +
														'class="tour-input-number" ' +
														'value="16" ' +
														'data-min="10" data-max="30" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="const-labels-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="const-labels-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="const-labels-section">' +
												'<span class="tour-color-group-label">' + tr("Labels Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="const-labels-color-r" class="tour-color-slider" min="0" max="100" value="0" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="const-labels-color-g" class="tour-color-slider" min="0" max="100" value="50" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="const-labels-color-b" class="tour-color-slider" min="0" max="100" value="0" step="1">' +
														'<div id="const-labels-color-preview" class="tour-color-preview"></div>' +
														'<span id="const-labels-color-value" class="tour-color-value">0.00, 0.50, 0.00</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								col2 += '</div>';
								
								// =========================================================
								// COLUMN 3: Boundaries + Hulls
								// =========================================================
								var col3 = '<div class="tour-column">';
								
								// BOUNDARIES section
								col3 += '<div id="const-boundaries-section" class="tour-section-boundaries">' +
										'<div class="tour-section-header">' + tr("BOUNDARIES") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="const-show-boundaries">' +
												'<label class="tour-control-label" for="const-show-boundaries">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="const-boundaries-thickness">' +
														tr("Thick:") +
												'</label>' +
												'<input id="const-boundaries-thickness" ' +
														'class="tour-input-number" ' +
														'value="1" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="const-boundaries-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="const-boundaries-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="const-boundaries-section">' +
												'<span class="tour-color-group-label">' + tr("Boundaries Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="const-boundaries-color-r" class="tour-color-slider" min="0" max="100" value="80" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="const-boundaries-color-g" class="tour-color-slider" min="0" max="100" value="30" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="const-boundaries-color-b" class="tour-color-slider" min="0" max="100" value="30" step="1">' +
														'<div id="const-boundaries-color-preview" class="tour-color-preview"></div>' +
														'<span id="const-boundaries-color-value" class="tour-color-value">0.80, 0.30, 0.30</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								// HULLS section
								col3 += '<div id="const-hulls-section" class="tour-section-hulls">' +
										'<div class="tour-section-header">' + tr("HULLS (Areas)") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="const-show-hulls">' +
												'<label class="tour-control-label" for="const-show-hulls">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="const-hulls-thickness">' +
														tr("Thick:") +
												'</label>' +
												'<input id="const-hulls-thickness" ' +
														'class="tour-input-number" ' +
														'value="1" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="const-hulls-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="const-hulls-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="const-hulls-section">' +
												'<span class="tour-color-group-label">' + tr("Hulls Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="const-hulls-color-r" class="tour-color-slider" min="0" max="100" value="100" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="const-hulls-color-g" class="tour-color-slider" min="0" max="100" value="70" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="const-hulls-color-b" class="tour-color-slider" min="0" max="100" value="40" step="1">' +
														'<div id="const-hulls-color-preview" class="tour-color-preview"></div>' +
														'<span id="const-hulls-color-value" class="tour-color-value">1.00, 0.70, 0.40</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								col3 += '</div>';
								
								// =========================================================
								// ASSEMBLE HTML
								// =========================================================
								$toolbar.html(col1 + col2 + col3);
								
								// =========================================================
								// BIND: Generate Dynamic button
								// =========================================================
								$toolbar.find('#btn-generate-const-dynamic')
										.off('click.constDyn')
										.on('click.constDyn', function(e) {
												e.preventDefault();
												e.stopPropagation();
												
												console.log('[SkyCulture:Tours] Generate constellations script (DYNAMIC)');
												
												try {
														var script = Tours.generateConstellationsTourDynamic();
														if (script) {
																Tours.outputScript(script, 'constellation');
																//Tours.notify(tr("Dynamic constellations tour script generated"));
														}
												} catch (err) {
														console.error('[SkyCulture:Tours] Dynamic error:', err);
														Tours.notify(tr("Error generating dynamic script"));
												}
										});								
								// =========================================================
								// BIND: Generate Static button
								// =========================================================
								/* $toolbar.find('#btn-generate-const-static')
										.off('click.constStatic')
										.on('click.constStatic', function(e) {
												e.preventDefault();
												e.stopPropagation();
												
												console.log('[SkyCulture:Tours] Generate constellations script (STATIC)');
												
												try {
														var script = Tours.generateConstellationsTourStatic();
														if (script) {
																Tours.outputScript(script, 'constellation');
																//Tours.notify(tr("Static constellations tour script generated"));
														}
												} catch (err) {
														console.error('[SkyCulture:Tours] Static error:', err);
														Tours.notify(tr("Error generating static script"));
												}
										});*/
										$toolbar.find('#btn-generate-const-static')
												.off('click.constStatic')
												.on('click.constStatic', function(e) {
														e.preventDefault();
														e.stopPropagation();
												console.log('[SkyCulture:Tours] Generate constellations script (STATIC)');
														Tours.generateConstellationsTourStaticAsync()
																.then(function(script) {
																		if (script) {
																				Tours.outputScript(script, 'constellation');
																		}
																})
																.catch(function(err) {
																		console.error('[SkyCulture:Tours] Error:', err);
																		Tours.notify(tr("Error generating tour script"));
																});
												});										
						}
				});
		}

		/**
		 * Create the Asterisms tour toolbar.
		 * 
		 * Uses the unified Tours.createToolbar flow, with three organized
		 * sections (columns), each with a colored header:
		 * 
		 *   Column 1: Script generation + Timing + Selection + LINES section
		 *   Column 2: LABELS section
		 *   Column 3: RAY HELPERS section
		 * 
		 * Each section uses data-section-id on its color group wrapper, so
		 * the section header text and border-left color are dynamically tinted
		 * by initColorSliders based on the selected color.
		 * 
		 * All numeric inputs use the jQuery UI spinner pattern (via
		 * Tours.initTourToolbar, called automatically by createToolbar).
		 * 
		 * @function createAsterismsTourToolbar
		 */
		function createAsterismsTourToolbar() {
				Tours.createToolbar({
						containerId:              'aster-tour-controls-container',
						sectionTitle:             tr("Asterisms Tour Generator"),
						helpTargetId:             'help-asterisms',
						helpLabel:                tr("Tour Generator Help"),
						tabSelector:              '#patterns-tab-asterisms',
						buttonsContainerSelector: '#asterisms-buttons-container',
						tourType:                 'asterism',
						tr:                       tr,
						
						buildContent: function($toolbar, tr) {
								
								// =========================================================
								// COLUMN 1: Script Generation + Timing + Selection + LINES
								// =========================================================
								var col1 = '<div class="tour-column">';
								
								// --- Row 1: Generate button + save_state ---
								col1 += '<div class="tour-row-separator">' +
										'<button id="btn-generate-aster-static" ' +
												'class="jquerybutton pattern-btn" ' +
												'title="' + tr("Generates script with embedded asterism name array") + '">' +
												tr("Generate Asterisms Tour Script") +
										'</button>' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="aster-tour-use-savestate" />' +
												'<label class="tour-control-label" for="aster-tour-use-savestate" style="font-weight:bold;color:#A6E22E;">' +
														tr("Use save_state.inc") + ' <span class="tour-multiselect-hint">(Save/restore state via include file)</span>' +
												'</label>' +
										'</div>' +
								'</div>';
								
								// --- Row 2: Timing controls ---
								col1 += '<div class="tour-row">' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="aster-tour-duration">' +
														tr("Duration:") +
												'</label>' +
												'<input id="aster-tour-duration" ' +
														'class="tour-input-number" ' +
														'value="3" ' +
														'data-min="1" data-max="10" data-step="0.5" ' +
														'data-numberformat="n1"/>' +
												'<span>' + tr("sec") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="aster-zoom-fov">' +
														tr("Zoom FOV:") +
												'</label>' +
												'<input id="aster-zoom-fov" ' +
														'class="tour-input-number" ' +
														'value="25" ' +
														'data-min="5" data-max="60" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="aster-overview-fov">' +
														tr("Overview FOV:") +
												'</label>' +
												'<input id="aster-overview-fov" ' +
														'class="tour-input-number" ' +
														'value="60" ' +
														'data-min="25" data-max="90" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
								'</div>';
								
								// --- Row 3: Selection options ---
								col1 += '<div class="tour-row-subtle">' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="aster-tour-isolate" checked>' +
												'<label class="tour-control-label" for="aster-tour-isolate">' +
														tr("Isolate Selected") +
												'</label>' +
										'</div>' +
								'</div>';
								
								// --- Section: LINES ---
								col1 += '<div id="aster-lines-section" class="tour-section-lines">' +
										'<div class="tour-section-header">' + tr("LINES") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="aster-show-lines" checked>' +
												'<label class="tour-control-label" for="aster-show-lines">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="aster-line-thickness">' +
														tr("Thick:") +
												'</label>' +
												'<input id="aster-line-thickness" ' +
														'class="tour-input-number" ' +
														'value="2" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="aster-lines-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="aster-lines-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="aster-lines-section">' +
												'<span class="tour-color-group-label">' + tr("Lines Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="aster-lines-color-r" class="tour-color-slider" min="0" max="100" value="100" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="aster-lines-color-g" class="tour-color-slider" min="0" max="100" value="0" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="aster-lines-color-b" class="tour-color-slider" min="0" max="100" value="20" step="1">' +
														'<div id="aster-lines-color-preview" class="tour-color-preview"></div>' +
														'<span id="aster-lines-color-value" class="tour-color-value">1.00, 0.00, 0.20</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								col1 += '</div>';   // close column 1
								
								// =========================================================
								// COLUMN 2: LABELS section
								// =========================================================
								var col2 = '<div class="tour-column">';
								
								col2 += '<div id="aster-labels-section" class="tour-section-labels">' +
										'<div class="tour-section-header">' + tr("LABELS") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="aster-show-labels" checked>' +
												'<label class="tour-control-label" for="aster-show-labels">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="aster-font-size">' +
														tr("Size:") +
												'</label>' +
												'<input id="aster-font-size" ' +
														'class="tour-input-number" ' +
														'value="16" ' +
														'data-min="10" data-max="30" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="aster-labels-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="aster-labels-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="aster-labels-section">' +
												'<span class="tour-color-group-label">' + tr("Labels Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="aster-labels-color-r" class="tour-color-slider" min="0" max="100" value="0" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="aster-labels-color-g" class="tour-color-slider" min="0" max="100" value="100" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="aster-labels-color-b" class="tour-color-slider" min="0" max="100" value="100" step="1">' +
														'<div id="aster-labels-color-preview" class="tour-color-preview"></div>' +
														'<span id="aster-labels-color-value" class="tour-color-value">0.00, 1.00, 1.00</span>' +
												'</div>' +
										'</div>' +
								'</div>' +
								
								//col2 += 
								//'</div>';   // close column 2
								
								// =========================================================
								// COLUMN 3: RAY HELPERS section
								// =========================================================
								//var col3 = '<div class="tour-column">';
								
								//col3 += 
								'<div id="aster-rayhelpers-section" class="tour-section-rayhelpers">' +
										'<div class="tour-section-header">' + tr("RAY HELPERS") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="aster-show-rayhelpers" checked>' +
												'<label class="tour-control-label" for="aster-show-rayhelpers">' +
														tr("Show") +
												'</label>' +
												'<label class="tour-control-label" for="aster-rayhelper-thickness">' +
														tr("Ray Thick:") +
												'</label>' +
												'<input id="aster-rayhelper-thickness" ' +
														'class="tour-input-number" ' +
														'value="1" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<label class="tour-control-label" for="aster-rayhelpers-fade">' +
														tr("Fade:") +
												'</label>' +
												'<input id="aster-rayhelpers-fade" ' +
														'class="tour-input-number" ' +
														'value="1.0" ' +
														'data-min="0" data-max="5" data-step="0.1" ' +
														'data-numberformat="n1"/>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="aster-rayhelpers-section">' +
												'<span class="tour-color-group-label">' + tr("Ray Helpers Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="aster-rayhelpers-color-r" class="tour-color-slider" min="0" max="100" value="50" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="aster-rayhelpers-color-g" class="tour-color-slider" min="0" max="100" value="100" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="aster-rayhelpers-color-b" class="tour-color-slider" min="0" max="100" value="50" step="1">' +
														'<div id="aster-rayhelpers-color-preview" class="tour-color-preview"></div>' +
														'<span id="aster-rayhelpers-color-value" class="tour-color-value">0.50, 1.00, 0.50</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								col2 += '</div>';   // close column 3
								
								// =========================================================
								// ASSEMBLE
								// =========================================================
								$toolbar.html(col1 + col2 );
								
								// =========================================================
								// BIND: Generate button
								// =========================================================
/*								$toolbar.find('#btn-generate-aster-static')
										.off('click.asterGen')
										.on('click.asterGen', function(e) {
												e.preventDefault();
												e.stopPropagation();
												
												console.log('[SkyCulture:Tours] Generate asterisms script clicked');
												
												try {
														var script = Tours.generateAsterismsTour();
														if (script) {
																Tours.outputScript(script, 'asterism');
																//Tours.notify(tr("Asterisms tour script generated"));
														}
												} catch (err) {
														console.error('[SkyCulture:Tours] Error:', err);
														Tours.notify(tr("Error generating tour script"));
												}
										});*/
										$toolbar.find('#btn-generate-aster-static')
												.off('click.asterGen')
												.on('click.asterGen', function(e) {
														e.preventDefault();
														e.stopPropagation();
												console.log('[SkyCulture:Tours] Generate asterisms script clicked');												
														Tours.generateAsterismsTourAsync()
																.then(function(script) {
																		if (script) {
																				Tours.outputScript(script, 'asterism');
																		}
																})
																.catch(function(err) {
																		console.error('[SkyCulture:Tours] Error:', err);
																		Tours.notify(tr("Error generating tour script"));
																});
												});
						}
				});
		}

		/**
		 * Create the Lunar Mansions tour toolbar.
		 * 
		 * Organized into three columns:
		 *   Column 1: Script generation + Timing + Screen Labels section
		 *   Column 2: Lunar System Lines section (with color + value display)
		 *   Column 3: Informational note about lunar mansions
		 * 
		 * The LUNAR SYSTEM LINES section uses data-section-id, so its header
		 * text and border-left are dynamically tinted by the color slider.
		 * 
		 * All numeric inputs use the jQuery UI spinner pattern (via
		 * Tours.initTourToolbar, called automatically by createToolbar).
		 * 
		 * @function createLunarTourToolbar
		 */
		function createLunarTourToolbar() {
				Tours.createToolbar({
						containerId:              'lunar-tour-controls-container',
						sectionTitle:             tr("Lunar Mansions Tour Generator"),
						helpTargetId:             'help-lunar',
						helpLabel:                tr("Tour Generator Help"),
						tabSelector:              '#patterns-tab-lunar',
						buttonsContainerSelector: '#lunar-buttons-container',
						tourType:                 'lunar',
						tr:                       tr,
						
						buildContent: function($toolbar, tr) {
								
								// =========================================================
								// COLUMN 1: Script Generation + Timing + Screen Labels
								// =========================================================
								var col1 = '<div class="tour-column">';
								
								// --- Row 1: Generate button ---
								col1 += '<div class="tour-row-separator">' +
										'<button id="btn-generate-lunar-static" ' +
												'class="jquerybutton pattern-btn" ' +
												'title="' + tr("Generates script with embedded lunar mansion name array") + '">' +
												tr("Generate Lunar Mansions Tour Script") +
										'</button>' +
								'</div>';
								
								// --- Row 2: Timing controls ---
								col1 += '<div class="tour-row">' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="lunar-tour-duration">' +
														tr("Duration:") +
												'</label>' +
												'<input id="lunar-tour-duration" ' +
														'class="tour-input-number" ' +
														'value="4" ' +
														'data-min="1" data-max="15" data-step="0.5" ' +
														'data-numberformat="n1"/>' +
												'<span>' + tr("sec") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="lunar-zoom-fov">' +
														tr("Zoom FOV:") +
												'</label>' +
												'<input id="lunar-zoom-fov" ' +
														'class="tour-input-number" ' +
														'value="30" ' +
														'data-min="5" data-max="90" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="lunar-overview-fov">' +
														tr("Overview FOV:") +
												'</label>' +
												'<input id="lunar-overview-fov" ' +
														'class="tour-input-number" ' +
														'value="90" ' +
														'data-min="20" data-max="120" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
								'</div>';
								
								// --- Section: SCREEN LABELS ---
								col1 += '<div id="lunar-labels-section" class="tour-section-labels">' +
										'<div class="tour-section-header">' + tr("SCREEN LABELS") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="lunar-show-labels" checked>' +
												'<label class="tour-control-label" for="lunar-show-labels">' +
														tr("Show on Screen") +
												'</label>' +
												'<label class="tour-control-label" for="lunar-font-size">' +
														tr("Font Size:") +
												'</label>' +
												'<input id="lunar-font-size" ' +
														'class="tour-input-number" ' +
														'value="16" ' +
														'data-min="10" data-max="30" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("px") + '</span>' +
										'</div>' +
								'</div>';
								
								col1 += '</div>';   // close column 1
								
								// =========================================================
								// COLUMN 2: Lunar System Lines
								// =========================================================
								var col2 = '<div class="tour-column">';
								
								col2 += '<div id="lunar-lines-section" class="tour-section-lines">' +
										'<div class="tour-section-header">' + tr("LUNAR SYSTEM LINES") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="lunar-show-lines" checked>' +
												'<label class="tour-control-label" for="lunar-show-lines">' +
														tr("Show Lines") +
												'</label>' +
												'<label class="tour-control-label" for="lunar-line-thickness">' +
														tr("Thickness:") +
												'</label>' +
												'<input id="lunar-line-thickness" ' +
														'class="tour-input-number" ' +
														'value="2" ' +
														'data-min="1" data-max="5" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("px") + '</span>' +
										'</div>' +
										'<div class="tour-color-group" data-section-id="lunar-lines-section">' +
												'<span class="tour-color-group-label">' + tr("Lines Color") + '</span>' +
												'<div class="tour-color-sliders">' +
														'<span class="tour-color-channel tour-color-channel-r">R</span>' +
														'<input type="range" id="lunar-lines-color-r" class="tour-color-slider" min="0" max="100" value="50" step="1">' +
														'<span class="tour-color-channel tour-color-channel-g">G</span>' +
														'<input type="range" id="lunar-lines-color-g" class="tour-color-slider" min="0" max="100" value="78" step="1">' +
														'<span class="tour-color-channel tour-color-channel-b">B</span>' +
														'<input type="range" id="lunar-lines-color-b" class="tour-color-slider" min="0" max="100" value="50" step="1">' +
														'<div id="lunar-lines-color-preview" class="tour-color-preview"></div>' +
														'<span id="lunar-lines-color-value" class="tour-color-value">0.50, 0.78, 0.50</span>' +
												'</div>' +
										'</div>' +
								'</div>';
								
								col2 += '</div>';   // close column 2
								
								// =========================================================
								// COLUMN 3: Informational note
								// =========================================================
								var col3 = '<div class="tour-column">';
								
								col3 += '<div class="tour-info-note">' +
										'<div class="info-title">' + tr("About Lunar Mansions") + '</div>' +
										'<div class="info-text">' +
												tr("Lunar mansions (also called lunar stations or lunar houses) are divisions of the Moon's path across the sky. Different cultures have different systems:") +
												'<br>' +
												'• ' + tr("Arabic: 28 mansions (Manazil al-Qamar)") +
												'<br>' +
												'• ' + tr("Chinese: 28 mansions (Xiu)") +
												'<br>' +
												'• ' + tr("Indian: 28 mansions (Nakshatras)") +
												'<br><br>' +
												'<span style="color:#81C784;">' +
														tr("Lines color can be customized above. Screen labels use a fixed color (#ffaa66) for readability.") +
												'</span>' +
										'</div>' +
								'</div>';
								
								col3 += '</div>';   // close column 3
								
								// =========================================================
								// ASSEMBLE
								// =========================================================
								$toolbar.html(col1 + col2 + col3);
								
								// =========================================================
								// BIND: Generate button
								// =========================================================
								/*$toolbar.find('#btn-generate-lunar-static')
										.off('click.lunarGen')
										.on('click.lunarGen', function(e) {
												e.preventDefault();
												e.stopPropagation();
												
												console.log('[SkyCulture:Tours] Generate lunar script clicked');
												
												try {
														var script = Tours.generateLunarTour();
														if (script) {
																Tours.outputScript(script, 'lunar');
																//Tours.notify(tr("Lunar mansions tour script generated"));
														}
												} catch (err) {
														console.error('[SkyCulture:Tours] Error:', err);
														Tours.notify(tr("Error generating tour script"));
												}
										});*/
										$toolbar.find('#btn-generate-lunar-static')
										.off('click.lunarGen')
										.on('click.lunarGen', function(e) {
												e.preventDefault();
												e.stopPropagation();
												console.log('[SkyCulture:Tours] Generate lunar script clicked');
												Tours.generateLunarTourAsync()
														.then(function(script) {
																if (script) {
																		Tours.outputScript(script, 'lunar');
																}
														})
														.catch(function(err) {
																console.error('[SkyCulture:Tours] Error:', err);
																Tours.notify(tr("Error generating tour script"));
														});
										});										
						}
				});
		}

		/**
		 * Create the Stars tour toolbar.
		 * 
		 * Organized into three columns:
		 *   Column 1: Script generation + Timing + Star Labels section
		 *   Column 2: Star Appearance section (scales, twinkle, spiky, halos, mag)
		 *   Column 3: (reserved for future use / notes)
		 * 
		 * Unlike the other toolbars, Stars does not have color pickers (no
		 * line/label colors apply to individual stars in Stellarium). Instead,
		 * it exposes numerous display-tuning sliders, each with a live value
		 * display (id: stars-*-value).
		 * 
		 * All numeric inputs use the jQuery UI spinner pattern (via
		 * Tours.initTourToolbar, called automatically by createToolbar).
		 * 
		 * @function createStarsTourToolbar
		 */
		function createStarsTourToolbar() {
				Tours.createToolbar({
						containerId:              'stars-tour-controls-container',
						sectionTitle:             tr("Stars Tour Generator"),
						helpTargetId:             'help-stars',
						helpLabel:                tr("Tour Generator Help"),
						tabSelector:              '#patterns-tab-stars',
						buttonsContainerSelector: '#stars-buttons-container',
						tourType:                 'star',
						tr:                       tr,
						
						buildContent: function($toolbar, tr) {
								
								// =========================================================
								// COLUMN 1: Script Generation + Timing + Star Labels
								// =========================================================
								var col1 = '<div class="tour-column">';
								
								// --- Row 1: Generate + save_state ---
								col1 += '<div class="tour-row-separator">' +
										'<button id="btn-generate-stars-static" ' +
												'class="jquerybutton pattern-btn" ' +
												'title="' + tr("Generates script with embedded star/object array") + '">' +
												tr("Generate Stars Tour Script") +
										'</button>' +
										'<div class="tour-control-group">' +
												'<input type="checkbox" id="stars-tour-use-savestate" />' +
												'<label class="tour-control-label" for="stars-tour-use-savestate" style="font-weight:bold;color:#A6E22E;">' +
														tr("Use save_state.inc") + ' <span class="tour-multiselect-hint">(Save/restore state via include file)</span>' +
												'</label>' +
										'</div>' +
								'</div>';
								
								// --- Row 2: Timing ---
								col1 += '<div class="tour-row">' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="stars-tour-duration">' +
														tr("Duration:") +
												'</label>' +
												'<input id="stars-tour-duration" ' +
														'class="tour-input-number" ' +
														'value="3" ' +
														'data-min="1" data-max="15" data-step="0.5" ' +
														'data-numberformat="n1"/>' +
												'<span>' + tr("sec") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="stars-zoom-fov">' +
														tr("Zoom FOV:") +
												'</label>' +
												'<input id="stars-zoom-fov" ' +
														'class="tour-input-number" ' +
														'value="10" ' +
														'data-min="5" data-max="60" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
										'<div class="tour-control-group">' +
												'<label class="tour-control-label-bold" for="stars-overview-fov">' +
														tr("Overview FOV:") +
												'</label>' +
												'<input id="stars-overview-fov" ' +
														'class="tour-input-number" ' +
														'value="60" ' +
														'data-min="20" data-max="120" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("deg") + '</span>' +
										'</div>' +
								'</div>';
								
								col1 += '</div>';   // close column 1
								
								// =========================================================
								// COLUMN 2: Star Appearance
								// =========================================================
								var col2 = '<div class="tour-column">';
								
								col2 += '<div id="stars-appearance-section" class="tour-section-artwork">' +
										'<div class="tour-section-header">' + tr("STAR APPEARANCE") + '</div>';
								
								// --- Scales row ---
								col2 += '<div class="tour-row">' +
										'<label class="tour-control-label" for="stars-absolute-scale">' +
												tr("Absolute Scale:") +
										'</label>' +
										'<input type="range" id="stars-absolute-scale" ' +
												'class="tour-input-range" value="1.0" min="0.3" max="3.0" step="0.01">' +
										'<span id="stars-absolute-scale-value" class="tour-range-value">1.00</span>' +
								'</div>';
								
								col2 += '<div class="tour-row">' +
										'<label class="tour-control-label" for="stars-relative-scale">' +
												tr("Relative Scale:") +
										'</label>' +
										'<input type="range" id="stars-relative-scale" ' +
												'class="tour-input-range" value="1.0" min="0.5" max="2.5" step="0.01">' +
										'<span id="stars-relative-scale-value" class="tour-range-value">1.00</span>' +
								'</div>';
								
								// --- Twinkle row ---
								col2 += '<div class="tour-row">' +
										'<input type="checkbox" id="stars-show-twinkle" checked>' +
										'<label class="tour-control-label" for="stars-show-twinkle">' +
												tr("Twinkle") +
										'</label>' +
										'<label class="tour-control-label" for="stars-twinkle-amount">' +
												tr("Amount:") +
										'</label>' +
										'<input type="range" id="stars-twinkle-amount" ' +
												'class="tour-input-range" value="0.3" min="0" max="1.5" step="0.01">' +
										'<span id="stars-twinkle-amount-value" class="tour-range-value">0.30</span>' +
								'</div>';
								
								// --- Effects row ---
								col2 += '<div class="tour-row">' +
										'<input type="checkbox" id="stars-show-spiky">' +
										'<label class="tour-control-label" for="stars-show-spiky">' +
												tr("Spiky Stars") +
										'</label>' +
										'<input type="checkbox" id="stars-show-halo" checked>' +
										'<label class="tour-control-label" for="stars-show-halo">' +
												tr("Star Halos") +
										'</label>' +
								'</div>';
								
								// --- Magnitude limit row ---
								col2 += '<div class="tour-row">' +
										'<input type="checkbox" id="stars-use-mag-limit">' +
										'<label class="tour-control-label" for="stars-use-mag-limit">' +
												tr("Use Magnitude Limit") +
										'</label>' +
										'<label class="tour-control-label" for="stars-mag-limit">' +
												tr("Limit:") +
										'</label>' +
										'<input type="range" id="stars-mag-limit" ' +
												'class="tour-input-range" value="8.0" min="1" max="10" step="0.1">' +
										'<span id="stars-mag-limit-value" class="tour-range-value">8.0</span>' +
								'</div>';
								
								col2 += '</div>';   // close section
								col2 += '</div>';   // close column 2
								
								// =========================================================
								// COLUMN 3: (reserved)
								// =========================================================
								var col3 = '<div class="tour-column">';
								
								// --- Section: STAR LABELS ---
								col3 += '<div id="stars-labels-section" class="tour-section-labels">' +
										'<div class="tour-section-header">' + tr("STAR LABELS") + '</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="stars-show-labels" checked>' +
												'<label class="tour-control-label" for="stars-show-labels">' +
														tr("Show Labels") +
												'</label>' +
												'<label class="tour-control-label" for="stars-font-size">' +
														tr("Font Size:") +
												'</label>' +
												'<input id="stars-font-size" ' +
														'class="tour-input-number" ' +
														'value="16" ' +
														'data-min="10" data-max="30" data-step="1" ' +
														'data-numberformat="n0"/>' +
												'<span>' + tr("px") + '</span>' +
										'</div>' +
										'<div class="tour-row">' +
												'<label class="tour-control-label" for="stars-labels-amount">' +
														tr("Labels Amount:") +
												'</label>' +
												'<input type="range" id="stars-labels-amount" ' +
														'class="tour-input-range" value="6" min="0" max="10" step="0.1">' +
												'<span id="stars-labels-amount-value" class="tour-range-value">6.0</span>' +
										'</div>' +
										'<div class="tour-row">' +
												'<input type="checkbox" id="stars-show-additional-names" checked>' +
												'<label class="tour-control-label" for="stars-show-additional-names">' +
														tr("Additional Names") +
												'</label>' +
												'<input type="checkbox" id="stars-show-dbl-stars">' +
												'<label class="tour-control-label" for="stars-show-dbl-stars">' +
														tr("Double Stars") +
												'</label>' +
												'<input type="checkbox" id="stars-show-var-stars">' +
												'<label class="tour-control-label" for="stars-show-var-stars">' +
														tr("Variable Stars") +
												'</label>' +
												'<input type="checkbox" id="stars-show-hip-numbers">' +
												'<label class="tour-control-label" for="stars-show-hip-numbers">' +
														tr("HIP Numbers") +
												'</label>' +
										'</div>' +
								'</div>';								
								
								col3 += '</div>'; // close column 3
								
								// =========================================================
								// ASSEMBLE
								// =========================================================
								$toolbar.html(col1 + col2 + col3);
								
								// =========================================================
								// BIND: Generate button
								// =========================================================
								/*$toolbar.find('#btn-generate-stars-static')
										.off('click.starsGen')
										.on('click.starsGen', function(e) {
												e.preventDefault();
												e.stopPropagation();												
												console.log('[SkyCulture:Tours] Generate stars script clicked');												
												try {
														var script = Tours.generateStarsTour();
														if (script) {
																Tours.outputScript(script, 'star');
																//Tours.notify(tr("Stars tour script generated"));
														}
												} catch (err) {
														console.error('[SkyCulture:Tours] Error:', err);
														Tours.notify(tr("Error generating tour script"));
												}
										});*/
										$toolbar.find('#btn-generate-stars-static')
										.off('click.starsGen')
										.on('click.starsGen', function(e) {
												e.preventDefault();
												e.stopPropagation();
												console.log('[SkyCulture:Tours] Generate stars script clicked');
												Tours.generateStarsTourAsync()
														.then(function(script) {
																if (script) {
																		Tours.outputScript(script, 'star');
																}
														})
														.catch(function(err) {
																console.error('[SkyCulture:Tours] Error:', err);
																Tours.notify(tr("Error generating tour script"));
														});
										});
								
								// =========================================================
								// BIND: Range slider value displays
								// =========================================================
								$toolbar.find('#stars-labels-amount').on('input', function() {
										$toolbar.find('#stars-labels-amount-value')
												.text(parseFloat($(this).val()).toFixed(1));
								});
								
								$toolbar.find('#stars-absolute-scale').on('input', function() {
										$toolbar.find('#stars-absolute-scale-value')
												.text(parseFloat($(this).val()).toFixed(2));
								});
								
								$toolbar.find('#stars-relative-scale').on('input', function() {
										$toolbar.find('#stars-relative-scale-value')
												.text(parseFloat($(this).val()).toFixed(2));
								});
								
								$toolbar.find('#stars-twinkle-amount').on('input', function() {
										$toolbar.find('#stars-twinkle-amount-value')
												.text(parseFloat($(this).val()).toFixed(2));
								});
								
								$toolbar.find('#stars-mag-limit').on('input', function() {
										$toolbar.find('#stars-mag-limit-value')
												.text(parseFloat($(this).val()).toFixed(1));
								});
								
								// =========================================================
								// BIND: Enable/disable mag limit slider
								// =========================================================
								$toolbar.find('#stars-use-mag-limit').on('change', function() {
										var isEnabled = $(this).is(':checked');
										$toolbar.find('#stars-mag-limit').prop('disabled', !isEnabled);
										$toolbar.find('#stars-mag-limit-value').css('opacity', isEnabled ? 1 : 0.5);
								});
								
								// Initialize mag limit slider state
								$toolbar.find('#stars-mag-limit').prop('disabled',
										!$toolbar.find('#stars-use-mag-limit').is(':checked'));
						}
				});
		}

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the Sky Culture module.
     * 
     * @param {string} cultureContainerSelector - Culture buttons container
     * @param {Object} patternsContainers - Pattern containers
     * @param {string} iframeSelector - Info frame selector
     * @param {Object} options - Additional options
     */
    function init(cultureContainerSelector, patternsContainers, iframeSelector, options) {
        if (State.raw.flags.isInitialized) {
            console.warn('[SkyCulture] Already initialized');
            return;
        }

        // Cache DOM references
        cacheDomReferences(
            $.extend({ cultures: $(cultureContainerSelector) }, patternsContainers),
            iframeSelector,
            options || {}
        );

        // Initialize sub-tabs (jQuery UI)
        if ($('#skyculture-patterns-tabs').length) {
            $('#skyculture-patterns-tabs').tabs({ heightStyle: 'content' });
        }

        // Install control panels for each tab
        Help.installControlPanel('constellation', tr, controlHandlers);
        Help.installControlPanel('asterism', tr, controlHandlers);
        Help.installControlPanel('lunar', tr, controlHandlers);
        Help.installControlPanel('star', tr, controlHandlers);

        // =====================================================================
        // INITIALIZE VISIBILITY TIMER STATE
        // =====================================================================
        // At this point, all control panels are installed and the radio
        // group has its default value (180s). All visibility checkboxes
        // are unchecked by default, so `syncVisibilityTimer()` will:
        //  - Decide NOT to start the timer (no type has `visible === true`)
        //  - Start the countdown display in "—" mode
        // 
        // This replaces the previously commented-out `startVisibilityTimer()`
        // call and correctly handles the "no filter active" case.
        // =====================================================================
        syncVisibilityTimer();
				
        // Initialize help modals
        Help.initHelpModals(tr);
        Help.bindHelpButtons(tr);
				
				// Setup synchronization for objectSelected events only
				// (culture change is handled separately via propApi listener above)
				Sync.setup();

				// =====================================================================
				// CULTURE CHANGE LISTENER (from Stellarium desktop or server)
				// =====================================================================
				// 
				// IMPORTANT: propApi is a plain JS object, NOT a jQuery object.
				// We use $(propApi) to wrap it and attach our handler to its jQuery
				// event system. The check is only for propApi existence, NOT for .on().
				// 
				// This listener fires when the culture is changed from:
				// - Stellarium desktop application
				// - Another Remote Control client
				// - Any external source
				// 
				// It does NOT fire when the culture is changed by OUR OWN setCulture()
				// call, because in that case the local state is already updated
				// before propApi.setStelProp() is called.
				// =====================================================================
				if (propApi) {
						$(propApi).off('stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID.skyculture')
											.on('stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID.skyculture',
								function(evt, data) {
										var newCultureId = data && data.value;
										if (!newCultureId) return;
										if (newCultureId === State.raw.culture.id) return;   // already synced
										
										console.log('[SkyCulture] Culture changed externally to:', newCultureId);
										
										// -----------------------------------------------------------------
										// 1. Update internal state
										// -----------------------------------------------------------------
										State.raw.culture.id = newCultureId;
										
										var cultures = State.raw.culture.list || [];
										State.raw.culture.name = newCultureId;   // fallback
										for (var i = 0; i < cultures.length; i++) {
												if (cultures[i].id === newCultureId) {
														State.raw.culture.name = cultures[i].name;
														break;
												}
										}
										
										// -----------------------------------------------------------------
										// 2. Update UI: culture buttons active state
										// -----------------------------------------------------------------
										Render.updateCultureButtonStates(newCultureId);
										
										// -----------------------------------------------------------------
										// 3. Update info iframe with cache buster
										// -----------------------------------------------------------------
										if (State.raw.ui.infoFrame && State.raw.ui.infoFrame.length) {
												var baseSrc = Config.API_ENDPOINTS.skyCultureDescription;
												var newSrc = baseSrc + '?culture=' + encodeURIComponent(newCultureId) +
																		 '&t=' + Date.now();
												State.raw.ui.infoFrame.attr('src', newSrc);
										}
										
										// -----------------------------------------------------------------
										// 4. Clear selections
										// -----------------------------------------------------------------
										State.clearSelection();
										MultiSelect.clearAll();
										Render.clearAllActiveButtons();
										
										// -----------------------------------------------------------------
										// 5. Reload all data
										// -----------------------------------------------------------------
										loadAllCultureData();
								}
						);
				}

        // Load cultures
        loadSkyCultures().then(function(success) {
            if (!success) return;

            // Load all culture data
            loadAllCultureData();

            // Create tour toolbars
            createConstellationsTourToolbar();
            createAsterismsTourToolbar();
            createLunarTourToolbar();
            createStarsTourToolbar();
						
						$(window).on('beforeunload.skycultureVisTimer', function() {
								stopVisibilityTimer();
						});

            // Initialize description direction
            if (stelUtils && typeof stelUtils.initDescriptionDirection === 'function') {
                stelUtils.initDescriptionDirection();
            }

            // Bind direction toggle
            $(document).off('click.skycultureDir').on(
                'click.skycultureDir',
                '#btn-toggle-description-direction',
                function(e) {
                    e.preventDefault();
                    stelUtils.toggleDescriptionDirection();
                }
            );

            State.raw.flags.isInitialized = true;
            console.log('[SkyCulture] Module initialized successfully v5.0.0');
        });
    }

    /**
     * Refresh all data.
     */
    function refresh() {
        loadSkyCultures().then(function() {
            loadAllCultureData();
        });
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        init:                init,
        refresh:             refresh,
        setCulture:          setCulture,
        getCurrentCulture:   function() { return State.raw.culture.id; },
        getAvailableCultures:function() { return (State.raw.culture.list || []).slice(); },
        getSelectedPattern:  function() { return State.raw.selection.pattern; },
        getSelectedPatternId:function() { return State.raw.selection.id; },
        refreshData:         loadAllCultureData,

        // Sync control
        isSyncEnabled:       function() { return State.raw.flags.syncEnabled; },
        setSyncEnabled:      function(enabled) { State.raw.flags.syncEnabled = enabled; },
				/**
				 * Get the most recently fetched server status.
				 * 
				 * Returns the payload of the last successful `/api/main/status`
				 * call, or `null` if no status has been fetched yet.
				 * 
				 * The status includes:
				 *   - `location`: { name, planet, latitude, longitude, altitude }
				 *   - `time`:     { jday, utc, isTimeNow, ... }
				 *   - `view`:     { fov }
				 * 
				 * This is the same snapshot that the visibility control panels
				 * display. It is useful for advanced integrations that want to
				 * display the same snapshot outside the control panels.
				 * 
				 * @returns {Object|null}
				 */				
        getLastStatus: function() { return _lastStatusCache; }				
    };
});