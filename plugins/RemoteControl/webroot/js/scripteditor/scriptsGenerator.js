/* ========================================================================
 * scriptsGenerator.js - Unified Script Generator for Any Object Type
 * ========================================================================
 *
 * @file    Scripts Generator module for Stellarium Remote Control.
 *          Powers the "Scripts Generator" sub-tab inside the Actions panel,
 *          allowing users to generate SSC tour scripts for ANY object type
 *          exposed by Stellarium's /api/objects/listobjecttypes endpoint
 *          (currently 88 types).
 *
 * @module  scripteditor/scriptsGenerator
 * @version 3.0.0
 * @license GPLv2+
 * @author  Stellarium Remote Control contributors
 *
 * @requires jquery
 * @requires api/remotecontrol
 * @requires ui/stellarium-utils
 * @requires ui/skyculture/skyculture-api
 * @requires ui/skyculture/skyculture-render
 * @requires ui/skyculture/skyculture-tours
 * @requires ui/skyculture/skyculture-help
 * @requires ui/skyculture/skyculture-filters
 * @requires ui/skyculture/skyculture-config
 *
 * ========================================================================
 * ARCHITECTURE
 * ========================================================================
 *
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 1. Load 88 types from /api/objects/listobjecttypes          │
 *   └──────────────────────────┬──────────────────────────────────┘
 *                              │ (user clicks a type)
 *                              ▼
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 2. Fetch objects via /api/objects/listobjectsbytype         │
 *   │    → state.currentObjects = ["M1", "M2", ...]               │
 *   └──────────────────────────┬──────────────────────────────────┘
 *                              │
 *                              ▼
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 3. LAZY: Only if a filter needs `_info`, fetch via batch    │
 *   │    → Api.fetchObjectsBatch(items, progressCb)               │
 *   │    → cached on pattern._info forever (until invalidated)    │
 *   └──────────────────────────┬──────────────────────────────────┘
 *                              │
 *                              ▼
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 4. Filters.applyAll() → filtered + sorted patterns          │
 *   └──────────────────────────┬──────────────────────────────────┘
 *                              │
 *                              ▼
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 5. Render:
 *   │    • If sortBy === "constellation" → grouped by IAU const.  │
 *   │      Uses Render.renderGroupedStars with per-group "Select  │
 *   │      All" button when Multi-Select is active.               │
 *   │    • Otherwise → flat grid via Render.renderPatternGrid.    │
 *   └──────────────────────────┬──────────────────────────────────┘
 *                              │
 *                              ▼
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │ 6. User clicks Generate → Tours.generateObjectTypeTour()    │
 *   │    → SSC shown in preview + send to Script Editor           │
 *   └─────────────────────────────────────────────────────────────┘
 *
 * ========================================================================
 * CACHING
 * ========================================================================
 *
 *   - `_info` on each pattern is IMMUTABLE except for `aboveHorizon`.
 *   - It is fetched ONCE per (type) combination.
 *   - It is RE-FETCHED only when:
 *       a) The user enables "Visible above horizon" AND cache is stale
 *       b) The user clicks manual Refresh button
 *       c) The visibility timer ticks (60s / 180s / 420s)
 *       d) The user changes the type (new type = new cache)
 *
 * ========================================================================
 * VISIBILITY TIMER
 * ========================================================================
 *
 *   - 60s / 180s / 420s / Manual (matches tab_skyculture exactly)
 *   - Only runs when: filterState.visible === true AND TTL > 0
 *   - On each tick: invalidate `_aboveHorizonAt` → fetch → re-render
 *   - Manual mode: user clicks Refresh button to trigger fetch
 *
 * ======================================================================== */

define([
    "jquery",
    "api/remotecontrol",
    "ui/stellarium-utils",
    "ui/skyculture/skyculture-api",
    "ui/skyculture/skyculture-render",
    "ui/skyculture/skyculture-tours",
    "ui/skyculture/skyculture-help",
    "ui/skyculture/skyculture-filters",
    "ui/skyculture/skyculture-config"
], function(
    $, rc, stelUtils,
    Api,
    Render,
    Tours,
    Help,
    Filters,
    Config
) {
    "use strict";

    var tr = rc.tr;

    // =====================================================================
    // CONSTANTS
    // =====================================================================

    /**
     * Maximum number of objects to fetch `info` for at once.
     * Prevents browser lockup for catalogues with thousands of objects.
     * @constant {number}
     */
    var MAX_OBJECTS_TO_FETCH = 5000;

    /**
     * Default visibility cache TTL in milliseconds (used if no radio is selected).
     * @constant {number}
     */
    var DEFAULT_VISIBILITY_TTL = 180 * 1000;

    /**
     * Visibility refresh modes → TTL in milliseconds.
     * A value of 0 means "manual" mode (no automatic refresh).
     * @constant {Object.<string, number>}
     */
    var VISIBILITY_TTL_MAP = {
        '60s':    60 * 1000,
        '180s':   180 * 1000,
        '420s':   420 * 1000,
        'manual': 0
    };

    // =====================================================================
    // MODULE STATE
    // =====================================================================

    /**
     * Central state container. All mutable module data lives here.
     * @type {Object}
     */
    var state = {
        /** @type {Array<{key:string, name:string, name_i18n:string}>} */
        allTypes: [],

        /** @type {Object|null} */
        selectedType: null,

        /** @type {Array<string>} Object names from listobjectsbytype */
        currentObjects: [],

        /**
         * Patterns with `_info` cache.
         * @type {Array<Object>}
         */
        patterns: [],

        /** @type {Set<string>} */
        selectedObjects: new Set(),

        /** @type {boolean} */
        multiSelectActive: false,

        /**
         * Filter state. `sortBy` accepts ONLY: name, constellation, magnitude.
         * @type {Object}
         */
        filterState: {
            visible:  false,
            sortBy:   'name',
            sortDir:  'asc',
            nameMode: 'default'
        },

        /** @type {Object|null} */
        lastStatus: null,

        /** @type {string} */
        lastGeneratedScript: '',

        /** @type {boolean} */
        initialized: false,

        /** @type {boolean} */
        apiReady: false,

        /** @type {number} */
        visibilityTimerTTL: 0
    };

    // =====================================================================
    // TIMERS
    // =====================================================================

    /** @type {number|null} */
    var visibilityTimer = null;

    /** @type {number|null} */
    var countdownTimer = null;

    /** @type {number} */
    var lastVisibilityTick = 0;

    // =====================================================================
    // DOM CACHE
    // =====================================================================

    /** @type {Object} */
    var $dom = {};

    /**
     * Cache jQuery references to avoid repeated lookups in hot paths.
     * Called once in `init()`.
     *
     * @function cacheDom
     * @returns {void}
     */
    function cacheDom() {
        $dom = {
            typesContainer:   $("#scriptsgenerator-types-container"),
            typesCount:       $("#scriptsgenerator-types-count"),
            useI18nCheckbox:  $("#scriptsgenerator-use-i18n"),
            objectsContainer: $("#scriptsgenerator-objects-container"),
            objectsCount:     $("#scriptsgenerator-objects-count"),
            selectedTypeLabel:$("#scriptsgenerator-selected-type-label"),
            multiSelectCheck: $("#scriptsgenerator-multi-select"),
            selectionCount:   $("#scriptsgenerator-selection-count"),
            selectAllBtn:     $("#scriptsgenerator-select-all"),
            clearBtn:         $("#scriptsgenerator-clear-selection"),
            duration:         $("#scriptsgenerator-duration"),
            zoomFov:          $("#scriptsgenerator-zoom-fov"),
            overviewFov:      $("#scriptsgenerator-overview-fov"),
            fontSize:         $("#scriptsgenerator-font-size"),
            showLabels:       $("#scriptsgenerator-show-labels"),
            showMarkers:      $("#scriptsgenerator-show-markers"),
            useSaveState:     $("#scriptsgenerator-use-savestate"),
            lockObserver:     $("#scriptsgenerator-lock-observer"),
            lockObserverInfo: $("#scriptsgenerator-lock-observer-info"),
            generateBtn:      $("#scriptsgenerator-generate"),
            copyBtn:          $("#scriptsgenerator-copy"),
            sendToEditorBtn:  $("#scriptsgenerator-send-to-editor"),
            status:           $("#scriptsgenerator-status"),
            preview:          $("#scriptsgenerator-preview"),
            apiStatus:        $("#scriptsgenerator-api-status"),
            summary:          $("#scriptsgenerator-summary"),
            filterVisible:    $("#sg-filter-visible"),
            visibilityCount:  $("#sg-visibility-count"),
            visSnapLocation:  $("#sg-vis-location"),
            visSnapTime:      $("#sg-vis-time"),
            visSnapJday:      $("#sg-vis-jday"),
            countdown:        $("#sg-countdown")
        };
    }

    // =====================================================================
    // HELPERS
    // =====================================================================

    /**
     * Escape HTML special characters to prevent XSS.
     *
     * @function esc
     * @param {*} str - Value to escape
     * @returns {string} Safe HTML string
     */
    function esc(str) {
        if (str === null || str === undefined) return "";
        return String(str)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    }

    /**
     * Update the status text next to the Generate button.
     *
     * @function setStatus
     * @param {string} text
     * @param {boolean} [isError=false]
     * @returns {void}
     */
    function setStatus(text, isError) {
        if (!$dom.status || !$dom.status.length) return;
        $dom.status.text(text || "");
        $dom.status.css("color", isError ? "#ff8888" : "#FFE0C6");
    }

    /**
     * Show a temporary toast notification.
     *
     * @function notify
     * @param {string} message
     * @returns {void}
     */
    function notify(message) {
        if (Tours && typeof Tours.notify === "function") {
            Tours.notify(message);
        } else if (stelUtils && typeof stelUtils.showNotification === "function") {
            stelUtils.showNotification(message);
        }
    }

    /**
     * Pick the display name for a type entry (i18n vs English).
     *
     * @function pickTypeName
     * @param {Object} typeObj
     * @param {boolean} useI18n
     * @returns {string}
     */
    function pickTypeName(typeObj, useI18n) {
        if (!typeObj) return "";
        if (useI18n) {
            return (typeObj.name_i18n && typeObj.name_i18n.trim())
                ? typeObj.name_i18n
                : (typeObj.name || typeObj.key || "");
        }
        return (typeObj.name && typeObj.name.trim())
            ? typeObj.name
            : (typeObj.name_i18n || typeObj.key || "");
    }

    /**
     * Read a numeric value from a jQuery input.
     *
     * @function readNum
     * @param {jQuery} $input
     * @param {number} fallback
     * @returns {number}
     */
    function readNum($input, fallback) {
        if (!$input || !$input.length) return fallback;
        var v = parseFloat($input.val());
        return isNaN(v) ? fallback : v;
    }

    /**
     * Read a boolean from a jQuery checkbox.
     *
     * @function readBool
     * @param {jQuery} $input
     * @param {boolean} fallback
     * @returns {boolean}
     */
    function readBool($input, fallback) {
        if (!$input || !$input.length) return fallback;
        return $input.is(":checked");
    }

    /**
     * Get the currently selected visibility refresh TTL (in ms).
     * Reads from the shared radio group `sg-visibility-refresh-mode`.
     *
     * @function getVisibilityTTL
     * @returns {number} 0 = manual mode
     */
    function getVisibilityTTL() {
        var $selected = $('input[name="sg-visibility-refresh-mode"]:checked');
        if (!$selected.length) return DEFAULT_VISIBILITY_TTL;
        var mode = $selected.val();
        return (typeof VISIBILITY_TTL_MAP[mode] === 'number')
            ? VISIBILITY_TTL_MAP[mode]
            : DEFAULT_VISIBILITY_TTL;
    }

    // =====================================================================
    // API LAYER
    // =====================================================================

    /**
     * Fetch the list of all object types.
     * Endpoint: GET /api/objects/listobjecttypes
     *
     * @function fetchAllTypes
     * @returns {Promise<Array<{key, name, name_i18n}>>}
     */
    function fetchAllTypes() {
        return new Promise(function(resolve, reject) {
            $.ajax({
                url: "/api/objects/listobjecttypes",
                type: "GET", dataType: "json", timeout: 15000,
                success: function(data) {
                    Array.isArray(data) ? resolve(data) : reject(new Error("Bad shape"));
                },
                error: function(xhr) { reject(new Error("HTTP " + xhr.status)); }
            });
        });
    }

    /**
     * Fetch object names for a type.
     * Endpoint: GET /api/objects/listobjectsbytype
     *
     * @function fetchObjectsByType
     * @param {string} typeKey
     * @param {boolean} useI18n
     * @returns {Promise<Array<string>>}
     */
    function fetchObjectsByType(typeKey, useI18n) {
        return new Promise(function(resolve, reject) {
            $.ajax({
                url: "/api/objects/listobjectsbytype",
                type: "GET",
                data: { type: typeKey, english: useI18n ? 0 : 1 },
                dataType: "json", timeout: 30000,
                success: function(data) {
                    Array.isArray(data) ? resolve(data) : reject(new Error("Bad shape"));
                },
                error: function(xhr) { reject(new Error("HTTP " + xhr.status)); }
            });
        });
    }

    // =====================================================================
    // RENDER TYPES
    // =====================================================================

    /**
     * Render the 88 type buttons into `#scriptsgenerator-types-container`.
     *
     * @function renderTypes
     * @returns {void}
     */
    function renderTypes() {
        var $c = $dom.typesContainer;
        if (!$c || !$c.length) return;
        $c.empty();

        if (!state.allTypes || state.allTypes.length === 0) {
            Render.renderPlaceholder($c, tr("No object types available"));
            updateTypesCount(0);
            return;
        }

        var useI18n = readBool($dom.useI18nCheckbox, true);
        var html = '<div class="scriptsgenerator-buttons-grid skyculture-buttons-grid">';

        for (var i = 0; i < state.allTypes.length; i++) {
            var t = state.allTypes[i];
            var label = pickTypeName(t, useI18n);
            var isActive = (state.selectedType && state.selectedType.key === t.key);
            html += '<button type="button" class="scriptsgenerator-btn skyculture-btn' +
                (isActive ? ' active' : '') + '" ' +
                'data-type-key="' + esc(t.key) + '" ' +
                'title="' + esc(t.key) + '">' + esc(label) + '</button>';
        }
        html += '</div>';
        $c.html(html);

        $c.find(".scriptsgenerator-btn").off("click.sgType").on("click.sgType", function() {
            var key = $(this).attr("data-type-key");
            if (key) onTypeSelected(key);
        });

        updateTypesCount(state.allTypes.length);
    }

    /**
     * Update the types counter badge.
     *
     * @function updateTypesCount
     * @param {number} n
     * @returns {void}
     */
    function updateTypesCount(n) {
        if ($dom.typesCount && $dom.typesCount.length) {
            $dom.typesCount.text("(" + n + ")");
        }
    }

    // =====================================================================
    // TYPE SELECTION
    // =====================================================================

    /**
     * Called when the user clicks a type button.
     * Fetches the type's object list, resets caches, and renders.
     *
     * @function onTypeSelected
     * @param {string} typeKey
     * @returns {void}
     */
    function onTypeSelected(typeKey) {
        var typeObj = null;
        for (var i = 0; i < state.allTypes.length; i++) {
            if (state.allTypes[i].key === typeKey) { typeObj = state.allTypes[i]; break; }
        }
        if (!typeObj) return;

        $dom.typesContainer.find(".scriptsgenerator-btn").removeClass("active");
        $dom.typesContainer.find('.scriptsgenerator-btn[data-type-key="' + typeKey + '"]').addClass("active");

        // Reset state
        state.selectedType = typeObj;
        state.currentObjects = [];
        state.patterns = [];
        state.selectedObjects.clear();
        state.multiSelectActive = false;
        state.lastStatus = null;

        stopVisibilityTimer();

        // Reset UI
        if ($dom.multiSelectCheck && $dom.multiSelectCheck.length) {
            $dom.multiSelectCheck.prop("checked", false);
        }
        toggleMultiSelectButtons(false);
        updateSelectionCount();
        updateControlPanel();
        clearVisibilitySnapshot();

        var useI18n = readBool($dom.useI18nCheckbox, true);
        var displayName = pickTypeName(typeObj, useI18n);
        if ($dom.selectedTypeLabel && $dom.selectedTypeLabel.length) {
            $dom.selectedTypeLabel.text("— " + displayName);
        }

        Render.renderPlaceholder($dom.objectsContainer, tr("Loading objects..."));
        updateObjectsCount(0);
        updateSummary();

        // Fetch object names
        fetchObjectsByType(typeKey, useI18n)
            .then(function(objects) {
                state.currentObjects = objects || [];
                state.patterns = state.currentObjects.map(function(name) {
                    return {
                        id: name,
                        name: name,
                        searchName: name,
                        _displayName: name,
                        _info: undefined
                    };
                });
                renderObjects();
            })
            .catch(function(err) {
                Render.renderPlaceholder($dom.objectsContainer, tr("Failed to load objects."));
                setStatus(tr("Error: ") + err.message, true);
            });
    }

    // =====================================================================
    // CONTROL PANEL
    // =====================================================================

    /**
     * Build and install the control panel (visibility, sort, nav-FOV, help).
     * Mirrors the layout of `skyculture-help.js` for stars.
     *
     * @function initControlPanel
     * @returns {void}
     */
    function initControlPanel() {
        var html = '<div class="skyculture-controls" id="scriptsgenerator-controls">';

        // ─── Fieldset 1: Visibility control ─────────────────────────
        html += '<fieldset class="control-group visibility-control-group">';
        html += '<legend>Visibility control</legend>';
        html += '<div class="block_row">';
        html += '<label class="option-label" for="sg-filter-visible">';
        html += '<input type="checkbox" id="sg-filter-visible" title="' +
                tr('Show only objects currently above horizon') + '"/>';
        html += '<span>' + tr('Visible above horizon only') + '</span></label>';
        html += '<span class="visibility-countdown" id="sg-countdown" title="' +
                tr('Time until next automatic refresh') + '">⏱ —</span>';
        html += '<span class="visibility-item-count" id="sg-visibility-count" title="' +
                tr('Visible items / total items') + '">—/—</span>';
        html += '</div>';

        // Refresh mode radios
        html += '<div class="visibility-refresh-group" data-prefix="sg">';
        html += '<span class="visibility-refresh-label">' + tr('Refresh visibility:') + '</span>';
        html += '<label class="visibility-radio-label"><input type="radio" name="sg-visibility-refresh-mode" value="60s"/><span>60s</span></label>';
        html += '<label class="visibility-radio-label"><input type="radio" name="sg-visibility-refresh-mode" value="180s" checked/><span>180s</span></label>';
        html += '<label class="visibility-radio-label"><input type="radio" name="sg-visibility-refresh-mode" value="420s"/><span>7min</span></label>';
        html += '<label class="visibility-radio-label"><input type="radio" name="sg-visibility-refresh-mode" value="manual"/><span>' + tr('Manual') + '</span></label>';
        html += '<button type="button" class="jquerybutton visibility-refresh-btn" id="sg-visibility-refresh" title="' + tr('Refresh visibility data now') + '"><span class="ui-icon ui-icon-refresh"></span> ' + tr('Refresh') + '</button>';
        html += '</div>';

        // Snapshot group
        html += '<div class="visibility-snapshot-group" id="sg-visibility-snapshot">';
        html += '<span class="visibility-snapshot-item"><span class="snapshot-icon ui-icon ui-icon-pin-s"></span><span class="snapshot-label">' + tr('Location:') + '</span><span class="snapshot-value" id="sg-vis-location">—</span></span>';
        html += '<span class="visibility-snapshot-item"><span class="snapshot-icon ui-icon ui-icon-calendar"></span><span class="snapshot-label">' + tr('Time:') + '</span><span class="snapshot-value" id="sg-vis-time">—</span></span>';
        html += '<span class="visibility-snapshot-item"><span class="snapshot-icon ui-icon ui-icon-calendar"></span><span class="snapshot-label">JDay:</span><span class="snapshot-value" id="sg-vis-jday">—</span></span>';
        html += '</div>';
        html += '</fieldset>';

        // ─── Fieldset 2: Order and filter (WITHOUT "Object Type") ───
        html += '<fieldset class="control-group visibility-control-group">';
        html += '<legend>Order and filter</legend>';
        html += '<div class="control-group"><label for="sg-sort-by">' + tr('Sort by:') + '</label>';
        html += '<select id="sg-sort-by" class="stars-sort-select">';
        html += '<option value="name">' + tr('Name (Alphabetical)') + '</option>';
        html += '<option value="constellation">' + tr('Constellation') + '</option>';
        html += '<option value="magnitude">' + tr('Brightness (Magnitude)') + '</option>';
        html += '</select></div>';
        html += '<div class="control-group"><label for="sg-sort-direction">' + tr('Order:') + '</label>';
        html += '<select id="sg-sort-direction" class="stars-sort-select">';
        html += '<option value="asc">' + tr('Ascending') + '</option>';
        html += '<option value="desc">' + tr('Descending') + '</option>';
        html += '</select></div>';
        html += '<div class="control-group"><button id="sg-apply-sort" class="jquerybutton pattern-btn">' + tr("Apply") + '</button></div>';
        html += '</fieldset>';

        // ─── Fieldset 3: Navigation control ─────────────────────────
        html += '<fieldset class="control-group visibility-control-group">';
        html += '<legend>Navigation Control</legend>';
        html += '<div class="control-group"><label for="sg-nav-fov">' + tr('Navigation FOV:') + '</label>';
        html += '<input id="sg-nav-fov" class="fov-input-number" value="10" data-min="0.1" data-max="120" data-step="0.1"/>';
        html += '<span>' + tr('deg') + '</span></div>';
        html += '</fieldset>';

        // ─── Help button ────────────────────────────────────────────
        html += '<div class="control-group control-help-group">';
        html += '<button type="button" class="pattern-btn jquerybutton tour-help-btn" ' +
                'data-help-target="help-controls-scriptsgenerator" title="' + tr('How to use the controls') + '">' +
                '<span class="ui-icon ui-icon-help"></span> ' + tr('Controls Help') + '</button>';
        html += '</div>';

        html += '</div>';   // end #scriptsgenerator-controls

        if ($dom.objectsContainer && $dom.objectsContainer.length) {
            $dom.objectsContainer.before(html);
        }

        // Re-cache
        $dom.filterVisible   = $("#sg-filter-visible");
        $dom.visibilityCount = $("#sg-visibility-count");
        $dom.visSnapLocation = $("#sg-vis-location");
        $dom.visSnapTime     = $("#sg-vis-time");
        $dom.visSnapJday     = $("#sg-vis-jday");
        $dom.countdown       = $("#sg-countdown");

        bindControlPanelEvents();

        // Init checkboxradio for radios
        $('input[name="sg-visibility-refresh-mode"]').each(function() {
            var $r = $(this);
            if (!$r.checkboxradio("instance")) $r.checkboxradio({ icon: false });
        });

        updateManualRefreshVisibility();

        if ($.fn.spinner) {
            $("#sg-nav-fov").spinner({ min: 0.1, max: 120, step: 0.1 });
        }
    }

    /**
     * Bind all control panel events (visibility, sort, refresh modes).
     *
     * @function bindControlPanelEvents
     * @returns {void}
     */
    function bindControlPanelEvents() {
        // Visibility checkbox
        $dom.filterVisible.off("change.sgVis").on("change.sgVis", function() {
            state.filterState.visible = $(this).is(":checked");
            if (state.filterState.visible) refreshSnapshot();
            else clearVisibilitySnapshot();
            syncVisibilityTimer();
            renderObjects();
        });

        // Sort Apply
        $("#sg-apply-sort").off("click.sgApply").on("click.sgApply", function() {
            state.filterState.sortBy  = $("#sg-sort-by").val();
            state.filterState.sortDir = $("#sg-sort-direction").val();
            renderObjects();
        });

        // Radio group (delegated)
        $(document).off("change.sgVisRadio").on(
            "change.sgVisRadio",
            'input[name="sg-visibility-refresh-mode"]',
            function() {
                var selectedValue = $(this).val();
                $('input[name="sg-visibility-refresh-mode"]').each(function() {
                    var $r = $(this);
                    var shouldBe = ($r.val() === selectedValue);
                    if ($r.prop("checked") !== shouldBe) {
                        $r.prop("checked", shouldBe);
                        if ($r.checkboxradio("instance")) $r.checkboxradio("refresh");
                    }
                });
                updateManualRefreshVisibility();
                syncVisibilityTimer();
                if (selectedValue !== "manual" && state.filterState.visible && needsFetch()) {
                    renderObjects();
                }
            }
        );

        // Manual refresh button
        $("#sg-visibility-refresh").off("click.sgManual").on("click.sgManual", function() {
            console.log("[ScriptsGen] Manual refresh clicked");
            invalidateVisibilityCache();
            refreshSnapshot();
            renderObjects();
        });
    }

    /**
     * Show/hide the manual Refresh button based on selected mode.
     *
     * @function updateManualRefreshVisibility
     * @returns {void}
     */
    function updateManualRefreshVisibility() {
        var selected = $('input[name="sg-visibility-refresh-mode"]:checked').val();
        $("#sg-visibility-refresh").toggle(selected === "manual");
    }

    /**
     * Reset the control panel state when the user changes type.
     *
     * @function updateControlPanel
     * @returns {void}
     */
    function updateControlPanel() {
        state.filterState = {
            visible:  false,
            sortBy:   'name',
            sortDir:  'asc',
            nameMode: 'default'
        };

        if ($dom.filterVisible && $dom.filterVisible.length) {
            $dom.filterVisible.prop("checked", false);
        }
        $("#sg-sort-by").val("name");
        $("#sg-sort-direction").val("asc");
        $('input[name="sg-visibility-refresh-mode"][value="180s"]').prop("checked", true);
        $('input[name="sg-visibility-refresh-mode"]').each(function() {
            if ($(this).checkboxradio("instance")) $(this).checkboxradio("refresh");
        });
        updateManualRefreshVisibility();
        if ($dom.visibilityCount && $dom.visibilityCount.length) {
            $dom.visibilityCount.text("—/—");
        }
    }

    // =====================================================================
    // VISIBILITY CACHE & TIMER
    // =====================================================================

    /**
     * Invalidate `_info` so the next render triggers a fresh fetch.
     * Sets `_info = undefined` on every pattern, forcing refetch.
     *
     * @function invalidateVisibilityCache
     * @returns {void}
     */
    function invalidateVisibilityCache() {
        if (!state.patterns || !state.patterns.length) return;
        var count = 0;
        for (var i = 0; i < state.patterns.length; i++) {
            if (state.patterns[i]._info) {
                state.patterns[i]._info = undefined;
                count++;
            }
        }
        console.log("[ScriptsGen] Invalidated cache for " + count + " patterns");
    }

    /**
     * Stop all timers.
     *
     * @function stopVisibilityTimer
     * @returns {void}
     */
    function stopVisibilityTimer() {
        if (visibilityTimer) { clearInterval(visibilityTimer); visibilityTimer = null; }
        if (countdownTimer) { clearInterval(countdownTimer); countdownTimer = null; }
        state.visibilityTimerTTL = 0;
        updateCountdownDisplay();
    }

    /**
     * Start (or restart) the visibility timer based on current mode.
     *
     * @function startVisibilityTimer
     * @returns {void}
     */
    function startVisibilityTimer() {
        stopVisibilityTimer();
        var ttl = getVisibilityTTL();
        if (!ttl || ttl <= 0 || !state.filterState.visible) return;

        state.visibilityTimerTTL = ttl;
        lastVisibilityTick = Date.now();

        visibilityTimer = setInterval(function() {
            console.log("[ScriptsGen] Visibility tick");
            invalidateVisibilityCache();
            refreshSnapshot();
            renderObjects();
            lastVisibilityTick = Date.now();
        }, ttl);

        countdownTimer = setInterval(updateCountdownDisplay, 1000);
    }

    /**
     * Reconcile the timer with the current state.
     *
     * @function syncVisibilityTimer
     * @returns {void}
     */
    function syncVisibilityTimer() {
        var ttl = getVisibilityTTL();
        var hasActive = state.filterState.visible;
        if (!((ttl > 0) && hasActive)) { stopVisibilityTimer(); return; }
        if (visibilityTimer && state.visibilityTimerTTL === ttl) return;
        startVisibilityTimer();
    }

    /**
     * Update the countdown display ("⏱ N s").
     *
     * @function updateCountdownDisplay
     * @returns {void}
     */
    function updateCountdownDisplay() {
        if (!$dom.countdown || !$dom.countdown.length) return;
        var ttl = getVisibilityTTL();
        if (ttl === 0 || !state.filterState.visible) {
            $dom.countdown.text("⏱ —");
            return;
        }
        var remainingSec = Math.ceil(Math.max(0, ttl - (Date.now() - lastVisibilityTick)) / 1000);
        if (remainingSec >= 60) {
            var m = Math.floor(remainingSec / 60);
            var s = remainingSec % 60;
            $dom.countdown.text("⏱ " + m + "m" + (s > 0 ? " " + s + "s" : ""));
        } else {
            $dom.countdown.text("⏱ " + remainingSec + "s");
        }
    }

    // =====================================================================
    // SNAPSHOT
    // =====================================================================

    /**
     * Fetch fresh status and update the snapshot display.
     *
     * @function refreshSnapshot
     * @returns {Promise<Object|null>}
     */
    function refreshSnapshot() {
        return Api.fetchStatus().then(function(status) {
            state.lastStatus = status;
            updateSnapshotDisplay(status);
            return status;
        }).catch(function() {
            updateSnapshotDisplay(null);
            return null;
        });
    }

    /**
     * Update the snapshot display (location, time, jday).
     *
     * @function updateSnapshotDisplay
     * @param {Object|null} status
     * @returns {void}
     */
    function updateSnapshotDisplay(status) {
        if (!$dom.visSnapLocation || !$dom.visSnapLocation.length) return;
        if (!status || !status.location || !status.time) {
            $dom.visSnapLocation.text("—");
            $dom.visSnapTime.text("—");
            $dom.visSnapJday.text("—");
            return;
        }
        var loc = status.location;
        var t   = status.time;
        var lat = Number(loc.latitude).toFixed(4);
        var lon = Number(loc.longitude).toFixed(4);
        var planet = (loc.planet && loc.planet !== "Earth") ? " (" + loc.planet + ")" : "";
        $dom.visSnapLocation.text((loc.name || "Unknown") + planet + " (" + lat + "°, " + lon + "°)");
        $dom.visSnapTime.text((t.utc || "").replace("T", " ").replace("Z", " UTC"));
        $dom.visSnapJday.text(Number(t.jday).toFixed(6));
    }

    /**
     * Clear the snapshot display.
     *
     * @function clearVisibilitySnapshot
     * @returns {void}
     */
    function clearVisibilitySnapshot() {
        if ($dom.visSnapLocation && $dom.visSnapLocation.length) {
            $dom.visSnapLocation.text("—");
            $dom.visSnapTime.text("—");
            $dom.visSnapJday.text("—");
        }
        if ($dom.visibilityCount && $dom.visibilityCount.length) {
            $dom.visibilityCount.text("—/—");
        }
        if ($dom.countdown && $dom.countdown.length) {
            $dom.countdown.text("⏱ —");
        }
    }

    // =====================================================================
    // LAZY INFO FETCHING
    // =====================================================================

    /**
     * Determine whether the current filter state requires `_info` data.
     *
     * @function filterNeedsInfo
     * @returns {boolean}
     */
    function filterNeedsInfo() {
        return state.filterState.visible
            || (state.filterState.sortBy && state.filterState.sortBy !== "name");
    }

    /**
     * Check if a fetch is needed for the current patterns.
     *
     * @function needsFetch
     * @returns {boolean}
     */
    function needsFetch() {
        if (!state.patterns || !state.patterns.length) return false;
        if (!filterNeedsInfo()) return false;

        for (var i = 0; i < state.patterns.length; i++) {
            if (state.patterns[i]._info === undefined) return true;
        }

        if (state.filterState.visible) {
            var ttl = getVisibilityTTL();
            var now = Date.now();
            for (var j = 0; j < state.patterns.length; j++) {
                var info = state.patterns[j]._info;
                if (!info) continue;
                if (!info._aboveHorizonAt || info._aboveHorizonAt === 0) return true;
                if (ttl > 0 && (now - info._aboveHorizonAt > ttl)) return true;
            }
        }
        return false;
    }

    /**
     * Fetch `_info` for all patterns that lack it.
     *
     * @function fetchMissingInfo
     * @returns {Promise<void>}
     */
    function fetchMissingInfo() {
        var missing = [];
        for (var i = 0; i < state.patterns.length; i++) {
            if (state.patterns[i]._info === undefined) {
                missing.push({ item: state.patterns[i], apiId: state.patterns[i].id });
            }
        }
        if (missing.length === 0) return Promise.resolve();

        var toFetch = missing.slice(0, MAX_OBJECTS_TO_FETCH);
        console.log("[ScriptsGen] Fetching _info for " + toFetch.length + " objects");

        var onProgress = function(loaded, total) {
            Render.renderPlaceholder(
                $dom.objectsContainer,
                tr("Loading data...") + " " + loaded + "/" + total
            );
        };

        return Api.fetchObjectsBatch(toFetch, onProgress).then(function(results) {
            var now = Date.now();
            for (var j = 0; j < results.length; j++) {
                var r = results[j];
                var pattern = r.item;
                if (!pattern) continue;
                if (r.info) {
                    r.info._fetchedAt      = now;
                    r.info._aboveHorizonAt = now;
                    pattern._info = r.info;
                } else {
                    pattern._info = null;
                }
            }
            console.log("[ScriptsGen] Fetched " + results.length + " objects");
        });
    }

    // =====================================================================
    // RENDER OBJECTS (grouped or flat)
    // =====================================================================

    /**
     * Render the object buttons for the currently selected type.
     * 
     * Flow:
     *   1. If filter needs `_info` and cache is stale → fetch first.
     *   2. Apply Filters.applyAll.
     *   3. Render buttons (grouped by constellation OR flat grid).
     *   4. Update counters.
     *
     * @function renderObjects
     * @returns {void}
     */
    function renderObjects() {
        var $c = $dom.objectsContainer;
        if (!$c || !$c.length) return;

        if (!state.patterns || state.patterns.length === 0) {
            $c.empty();
            Render.renderPlaceholder($c, tr("No objects found for this type"));
            updateObjectsCount(0);
            return;
        }

        if (needsFetch()) {
            Render.renderPlaceholder($c, tr("Loading data... 0/" + state.patterns.length));
            fetchMissingInfo().then(function() {
                applyFiltersAndRender($c);
            }).catch(function(err) {
                console.error("[ScriptsGen] Fetch failed:", err);
                Render.renderPlaceholder($c, tr("Error loading object data."));
            });
        } else {
            applyFiltersAndRender($c);
        }
    }

    /**
     * Apply filters via `Filters.applyAll` and render the result.
     *
     * Decides between:
     *   - Grouped rendering (when `sortBy === "constellation"`)
     *   - Flat grid rendering (otherwise)
     *
     * When multi-select is active AND rendering is grouped, a "Select All"
     * button is injected in each group header. Clicking it toggles all
     * objects of that group.
     *
     * @function applyFiltersAndRender
     * @param {jQuery} $c - The objects container
     * @returns {void}
     */
    function applyFiltersAndRender($c) {
        // 1. Build infoMap from cache
        var infoMap = {};
        for (var i = 0; i < state.patterns.length; i++) {
            infoMap[state.patterns[i].id] = state.patterns[i]._info || null;
        }

        // 2. Apply filters
        var result = Filters.applyAll(state.patterns, state.filterState, infoMap, "star");

        // 3. Decide: grouped or flat?
        var isGrouped = (state.filterState.sortBy === "constellation");
        var useMultiSelect = state.multiSelectActive;

        // Shared click handler
        var clickHandler = function(data) {
            var name = data.id;
            var $btn = data.$button;
            if (!name) return;

            if (!useMultiSelect) {
                // Single-select: clear all other actives
                $c.find(".pattern-btn").removeClass("active").css({
                    "background": "", "border-color": "", "color": ""
                });
                $c.data("active-name", null);
            }

            var wasActive = $btn.hasClass("active");

            if (wasActive && !useMultiSelect) {
                stelUtils.clearSelection();
            } else {
                $btn.addClass("active").css({
                    "background": "linear-gradient(#DCDBDA, #8A8C8E)",
                    "border-color": "#000000",
                    "color": "#000000"
                });
                if (!useMultiSelect) $c.data("active-name", name);

                if (useMultiSelect) {
                    toggleObjectSelection(name, $btn);
                } else {
                    navigateToObject(name);
                }
            }
        };

        if (isGrouped && result.grouped) {
            // ─── Grouped rendering (built manually to include Select All) ───
            renderGroupedWithSelectAll($c, result.grouped, {
                tr: tr,
                useMultiSelect: useMultiSelect,
                onClick: clickHandler
            });
        } else {
            // ─── Flat rendering ───────────────────────────────────
            Render.renderPatternGrid($c, result.patterns, "star", {
                tr: tr,
                onClick: clickHandler,
                disableHandler: useMultiSelect
            });

            if (useMultiSelect) {
                $c.find(".pattern-btn").off("click.sgObj").on("click.sgObj", function() {
                    clickHandler({
                        id: $(this).attr("data-pattern-id"),
                        $button: $(this)
                    });
                });
            }
        }

        // Update counters
        updateObjectsCount(result.patterns.length);
        updateVisibilityCount(result);
        updateSummary();
    }

    /**
     * Render grouped patterns (by constellation) with explicit
     * per-group "Select All / Deselect All" button.
     *
     * This function mirrors `skyculture-render.renderGroupedStars` but
     * adds the Select All button **explicitly** based on the local
     * `useMultiSelect` flag — it does NOT depend on
     * `State.raw.multiSelect.active` (which belongs to Sky Culture).
     *
     * @function renderGroupedWithSelectAll
     * @param {jQuery} $container - Target container
     * @param {Object} grouped - { groupName: { name, items[] } }
     * @param {Object} opts - { tr, useMultiSelect, onClick }
     * @returns {void}
     */
    function renderGroupedWithSelectAll($container, grouped, opts) {
        opts = opts || {};
        var tr = opts.tr;
        var useMultiSelect = opts.useMultiSelect;
        var onClick = opts.onClick;

        $container.empty();

        if (!grouped || !Object.keys(grouped).length) {
            Render.renderPlaceholder($container, tr("No objects found"));
            return;
        }

        var html = '';
        var groupKeys = Object.keys(grouped);

        for (var g = 0; g < groupKeys.length; g++) {
            var groupName = groupKeys[g];
            var group = grouped[groupName];
            var items = group.items || [];
            if (items.length === 0) continue;

            // ─── Group header ──────────────────────────────────────
            html += '<div class="star-group-header" data-group="' + esc(groupName) + '">';
            html += '<span class="star-group-name">' + esc(groupName) + '</span>';
            html += '<span class="pattern-count">(' + items.length + ' ' +
                    esc(tr('objects')) + ')</span>';

            // Select All button (only in multi-select)
            if (useMultiSelect) {
                // Determine if all items in this group are selected
                var allSelected = true;
                for (var k = 0; k < items.length; k++) {
                    if (!state.selectedObjects.has(items[k].id)) {
                        allSelected = false;
                        break;
                    }
                }
                var btnLabel = allSelected ? tr("Deselect All") : tr("Select All");
                html += '<button type="button" class="star-group-select-all jquerybutton" ' +
                        'data-group="' + esc(groupName) + '" ' +
                        'style="margin-left:10px;padding:1px 10px;font-size:10px;' +
                        'background:linear-gradient(#5D5F62, #3A3C3E);' +
                        'border:1px solid #2A2C2E;border-radius:3px;color:#000;cursor:pointer;">' +
                        esc(btnLabel) + '</button>';
            }

            html += '</div>';

            // ─── Group buttons ─────────────────────────────────────
            html += '<div class="star-group-buttons" data-group="' + esc(groupName) + '">';
            for (var i = 0; i < items.length; i++) {
                var p = items[i];
                var text = p._displayName || p.name || p.id;
                var isSelected = state.selectedObjects.has(p.id);
                var isActive = useMultiSelect ? isSelected : ($container.data("active-name") === p.id);

                var classNames = "pattern-btn";
                var style = "";
                if (isSelected || isActive) {
                    classNames += " active";
                    style = ' style="background:linear-gradient(#DCDBDA, #8A8C8E);' +
                            'border-color:#000000;color:#000000;"';
                }

                html += '<button type="button" class="' + classNames + '" ' +
                    'data-pattern-id="' + esc(p.id) + '" ' +
                    'data-pattern-name="' + esc(p.name || '') + '" ' +
                    'data-pattern-type="star" ' +
                    'data-search-name="' + esc(p.searchName || p.name || '') + '" ' +
                    'data-display-name="' + esc(text) + '" ' +
                    'data-group="' + esc(groupName) + '"' +
                    ' title="' + esc(p.id) + '"' +
                    style + '>' +
                    esc(text) + '</button>';
            }
            html += '</div>';
        }

        $container.html(html);

        // ─── Bind individual button clicks ─────────────────────
        $container.find(".pattern-btn").off("click.sgObj").on("click.sgObj", function() {
            var $btn = $(this);
            if (onClick) {
                onClick({
                    id: $btn.attr("data-pattern-id"),
                    name: $btn.attr("data-pattern-name"),
                    $button: $btn
                });
            }
        });

        // ─── Bind group "Select All" clicks (multi-select only) ───
        if (useMultiSelect) {
            $container.find(".star-group-select-all").off("click.sgGroupAll").on("click.sgGroupAll", function(e) {
                e.preventDefault();
                e.stopPropagation();

                var $groupBtn = $(this);
                var groupName = $groupBtn.data("group");
                var $groupButtons = $container.find(
                    '.star-group-buttons[data-group="' + groupName + '"] .pattern-btn'
                );

                // Determine current state
                var allSelected = true;
                $groupButtons.each(function() {
                    if (!state.selectedObjects.has($(this).attr("data-pattern-id"))) {
                        allSelected = false;
                    }
                });

                if (allSelected) {
                    // Deselect all
                    $groupButtons.each(function() {
                        var name = $(this).attr("data-pattern-id");
                        state.selectedObjects.delete(name);
                        $(this).removeClass("active").css({
                            "background": "", "border-color": "", "color": ""
                        });
                    });
                    $groupBtn.text(tr("Select All"));
                } else {
                    // Select all
                    $groupButtons.each(function() {
                        var name = $(this).attr("data-pattern-id");
                        state.selectedObjects.add(name);
                        $(this).addClass("active").css({
                            "background": "linear-gradient(#DCDBDA, #8A8C8E)",
                            "border-color": "#000000",
                            "color": "#000000"
                        });
                    });
                    $groupBtn.text(tr("Deselect All"));
                }

                updateSelectionCount();
                updateSummary();
            });
        }
    }

    /**
     * Attach "Select All / Deselect All" click handlers to group headers.
     * Called after `Render.renderGroupedStars` when multi-select is active.
     *
     * Each group header has:
     *   <button class="star-group-select-all" data-group="Ori">Select All</button>
     *
     * Clicking toggles every object within the group.
     *
     * @function attachGroupSelectAllHandlers
     * @param {jQuery} $c - The objects container
     * @returns {void}
     */
    function attachGroupSelectAllHandlers($c) {
        $c.find(".star-group-select-all").each(function() {
            var $groupBtn = $(this);
            var groupName = $groupBtn.data("group");
            var $groupButtons = $c.find('.star-group-buttons[data-group="' + groupName + '"] .pattern-btn');

            // Check if all are selected → set button text accordingly
            var totalInGroup = $groupButtons.length;
            var selectedInGroup = 0;
            $groupButtons.each(function() {
                if (state.selectedObjects.has($(this).attr("data-pattern-id"))) {
                    selectedInGroup++;
                }
            });
            $groupBtn.text(selectedInGroup === totalInGroup && totalInGroup > 0
                ? "Deselect All"
                : "Select All");

            // Bind click
            $groupBtn.off("click.sgGroupAll").on("click.sgGroupAll", function(e) {
                e.preventDefault();
                e.stopPropagation();

                var allSelected = true;
                $groupButtons.each(function() {
                    if (!state.selectedObjects.has($(this).attr("data-pattern-id"))) {
                        allSelected = false;
                    }
                });

                if (allSelected) {
                    // Deselect all
                    $groupButtons.each(function() {
                        var name = $(this).attr("data-pattern-id");
                        state.selectedObjects.delete(name);
                        $(this).removeClass("active").css({
                            "background": "", "border-color": "", "color": ""
                        });
                    });
                    $groupBtn.text("Select All");
                } else {
                    // Select all
                    $groupButtons.each(function() {
                        var name = $(this).attr("data-pattern-id");
                        state.selectedObjects.add(name);
                        $(this).addClass("active").css({
                            "background": "linear-gradient(#DCDBDA, #8A8C8E)",
                            "border-color": "#000000",
                            "color": "#000000"
                        });
                    });
                    $groupBtn.text("Deselect All");
                }

                updateSelectionCount();
                updateSummary();
            });
        });
    }

    /**
     * Update the "visible/total" badge.
     *
     * @function updateVisibilityCount
     * @param {Object} result - Result from Filters.applyAll
     * @returns {void}
     */
    function updateVisibilityCount(result) {
        if (!$dom.visibilityCount || !$dom.visibilityCount.length) return;
        if (!state.filterState.visible) {
            $dom.visibilityCount.text("—/—");
            return;
        }
        var v = result.visibleCount || 0;
        var t = result.totalCount   || result.patterns.length;
        $dom.visibilityCount.text(v + "/" + t);
    }

    /**
     * Update the objects counter badge.
     *
     * @function updateObjectsCount
     * @param {number} n
     * @returns {void}
     */
    function updateObjectsCount(n) {
        if ($dom.objectsCount && $dom.objectsCount.length) {
            $dom.objectsCount.text("(" + n + ")");
        }
    }

    // =====================================================================
    // OBJECT CLICK & MULTI-SELECT
    // =====================================================================

    /**
     * Navigate the Stellarium view to the given object.
     *
     * @function navigateToObject
     * @param {string} name
     * @returns {void}
     */
    function navigateToObject(name) {
        if (!name) return;
        var fov = readNum($("#sg-nav-fov"), 15);
        stelUtils.goToObject(name, fov, "object", null, null);
    }

    /**
     * Toggle an object in the multi-select set.
     *
     * @function toggleObjectSelection
     * @param {string} name
     * @param {jQuery} $btn
     * @returns {void}
     */
    function toggleObjectSelection(name, $btn) {
        if (state.selectedObjects.has(name)) {
            state.selectedObjects.delete(name);
            if ($btn && $btn.length) {
                $btn.removeClass("active").css({
                    "background": "", "border-color": "", "color": ""
                });
            }
        } else {
            state.selectedObjects.add(name);
            if ($btn && $btn.length) {
                $btn.addClass("active").css({
                    "background": "linear-gradient(#DCDBDA, #8A8C8E)",
                    "border-color": "#000000",
                    "color": "#000000"
                });
            }
        }
        updateSelectionCount();
        updateSummary();
    }

    /**
     * Enable/disable multi-select mode and re-render.
     *
     * @function toggleMultiSelect
     * @param {boolean} enabled
     * @returns {void}
     */
    function toggleMultiSelect(enabled) {
        state.multiSelectActive = !!enabled;
        toggleMultiSelectButtons(enabled);
        if (!enabled) clearObjectSelection();
        updateSelectionCount();
        updateSummary();
        // Re-render to attach/detach handlers
        if (state.patterns && state.patterns.length) {
            applyFiltersAndRender($dom.objectsContainer);
        }
    }

    /**
     * Show/hide Select All / Clear buttons.
     *
     * @function toggleMultiSelectButtons
     * @param {boolean} show
     * @returns {void}
     */
    function toggleMultiSelectButtons(show) {
        var display = show ? "inline-block" : "none";
        if ($dom.selectAllBtn && $dom.selectAllBtn.length) $dom.selectAllBtn.css("display", display);
        if ($dom.clearBtn && $dom.clearBtn.length) $dom.clearBtn.css("display", display);
    }

    /**
     * Update the "N selected" counter.
     *
     * @function updateSelectionCount
     * @returns {void}
     */
    function updateSelectionCount() {
        if (!$dom.selectionCount || !$dom.selectionCount.length) return;
        $dom.selectionCount.text(state.selectedObjects.size + " " + tr("selected"));
    }

    /**
     * Select all currently visible objects (across all groups).
     *
     * @function selectAllObjects
     * @returns {void}
     */
    function selectAllObjects() {
        if ($dom.objectsContainer && $dom.objectsContainer.length) {
            $dom.objectsContainer.find(".pattern-btn").each(function() {
                var name = $(this).attr("data-pattern-id") || $(this).attr("data-object-name");
                if (name) {
                    state.selectedObjects.add(name);
                    $(this).addClass("active").css({
                        "background": "linear-gradient(#DCDBDA, #8A8C8E)",
                        "border-color": "#000000",
                        "color": "#000000"
                    });
                }
            });
            // Update group "Select All" buttons to "Deselect All"
            $dom.objectsContainer.find(".star-group-select-all").text("Deselect All");
        }
        updateSelectionCount();
        updateSummary();
    }

    /**
     * Clear all selected objects.
     *
     * @function clearObjectSelection
     * @returns {void}
     */
    function clearObjectSelection() {
        state.selectedObjects.clear();
        if ($dom.objectsContainer && $dom.objectsContainer.length) {
            $dom.objectsContainer.find(".pattern-btn").removeClass("active").css({
                "background": "", "border-color": "", "color": ""
            });
            $dom.objectsContainer.find(".star-group-select-all").text("Select All");
        }
        updateSelectionCount();
        updateSummary();
    }

    // =====================================================================
    // SIDEBAR
    // =====================================================================

    /**
     * Render the API connection status.
     *
     * @function renderApiStatus
     * @returns {void}
     */
    function renderApiStatus() {
        if (!$dom.apiStatus || !$dom.apiStatus.length) return;
        if (state.apiReady) {
            $dom.apiStatus.html(
                '<span style="color:#A6E22E;">● ' + esc(tr("Connected")) + '</span><br>' +
                '<small style="color:#B4B7B0;">' +
                esc(state.allTypes.length + " " + tr("object types loaded")) + '</small>'
            );
        } else {
            $dom.apiStatus.html('<span style="color:#ff8888;">● ' + esc(tr("Not connected")) + '</span>');
        }
    }

    /**
     * Update the sidebar summary.
     *
     * @function updateSummary
     * @returns {void}
     */
    function updateSummary() {
        if (!$dom.summary || !$dom.summary.length) return;
        if (!state.selectedType) {
            $dom.summary.html('<div class="loading-placeholder">' + esc(tr("Nothing selected yet")) + '</div>');
            return;
        }

        var useI18n = readBool($dom.useI18nCheckbox, true);
        var typeDisplay = pickTypeName(state.selectedType, useI18n);
        var totalObjects = state.currentObjects.length;
        var selectedCount = state.selectedObjects.size;
        var visibleButtons = 0;
        if ($dom.objectsContainer && $dom.objectsContainer.length) {
            visibleButtons = $dom.objectsContainer.find(".pattern-btn").length;
        }
        var effectiveCount = state.multiSelectActive ? selectedCount : visibleButtons;

        var html = '<div style="font-size:11px;line-height:1.6;">';
        html += '<div><strong>' + esc(tr("Type:")) + '</strong> ' + esc(typeDisplay) + '</div>';
        html += '<div><strong>' + esc(tr("Total objects:")) + '</strong> ' + totalObjects + '</div>';
        html += '<div><strong>' + esc(tr("Visible (filtered):")) + '</strong> ' + visibleButtons + '</div>';
        if (state.multiSelectActive) {
            html += '<div><strong>' + esc(tr("Selected:")) + '</strong> ' + selectedCount + '</div>';
            html += '<div><strong>' + esc(tr("Will include:")) + '</strong> ' + effectiveCount + '</div>';
        } else {
            html += '<div><strong>' + esc(tr("Will include:")) + '</strong> ' + visibleButtons + '</div>';
        }
        html += '</div>';
        $dom.summary.html(html);
    }

    // =====================================================================
    // GENERATE
    // =====================================================================

    /**
     * Generate the SSC script from the current state.
     *
     * @function generateScript
     * @returns {void}
     */
    function generateScript() {
        setStatus("");
        if (!state.selectedType) {
            setStatus(tr("Please select an object type first."), true);
            return;
        }

        var objectsToUse = [];
        if (state.multiSelectActive) {
            objectsToUse = Array.from(state.selectedObjects);
            if (objectsToUse.length === 0) {
                setStatus(tr("Multi-Select is active but no objects are selected."), true);
                return;
            }
        } else {
            objectsToUse = $dom.objectsContainer.find(".pattern-btn").map(function() {
                return $(this).attr("data-pattern-id") || $(this).attr("data-object-name");
            }).get();
            if (objectsToUse.length === 0) {
                setStatus(tr("No objects to include. Please adjust filters."), true);
                return;
            }
        }

        var opts = {
            duration:     readNum($dom.duration, 4),
            zoomFov:      readNum($("#sg-nav-fov"), 15),
            overviewFov:  readNum($dom.overviewFov, 90),
            fontSize:     readNum($dom.fontSize, 16),
            showLabels:   readBool($dom.showLabels, true),
            showMarkers:  readBool($dom.showMarkers, true),
            useSaveState: readBool($dom.useSaveState, false),
            lockObserver: readBool($dom.lockObserver, false)
        };

        var proceed = function(status) {
            state.lastStatus = status || null;
            var script = Tours.generateObjectTypeTour({
                typeKey:      state.selectedType.key,
                typeName:     state.selectedType.name || state.selectedType.key,
                typeNameI18n: state.selectedType.name_i18n || state.selectedType.name || state.selectedType.key,
                objects:      objectsToUse,
                options:      opts,
                status:       state.lastStatus
            });
            if (!script) { setStatus(tr("Generation failed."), true); return; }
            state.lastGeneratedScript = script;
            showPreview(script);
            setStatus(
                tr("Generated") + ": " + (script.length / 1024).toFixed(1) +
                " KB, " + objectsToUse.length + " " + tr("objects"),
                false
            );
        };

        if (opts.lockObserver) {
            setStatus(tr("Fetching observer status..."));
            Api.fetchStatus().then(function(s) { proceed(s); }).catch(function() { proceed(null); });
        } else {
            proceed(null);
        }
    }

    /**
     * Show the generated script in the preview box.
     *
     * @function showPreview
     * @param {string} script
     * @returns {void}
     */
    function showPreview(script) {
        if (!$dom.preview || !$dom.preview.length) return;
        $dom.preview.text(script || "");
        if ($dom.preview[0]) $dom.preview[0].scrollTop = 0;
    }

    // =====================================================================
    // COPY / SEND
    // =====================================================================

    /**
     * Copy the generated script to the clipboard.
     *
     * @function copyScript
     * @returns {void}
     */
    function copyScript() {
        var script = state.lastGeneratedScript;
        if (!script) { setStatus(tr("Nothing to copy."), true); return; }
        if (window._stelScriptEditor && typeof window._stelScriptEditor.copyToClipboard === "function") {
            window._stelScriptEditor.copyToClipboard(script);
            notify(tr("Script copied"));
            return;
        }
        var $ta = $("<textarea></textarea>").val(script)
            .css({ position: "fixed", left: "-9999px" }).appendTo("body");
        $ta[0].select();
        try { document.execCommand("copy"); notify(tr("Copied")); } catch (e) {}
        $ta.remove();
    }

    /**
     * Send the generated script to the Script Editor tab.
     *
     * @function sendToEditor
     * @returns {void}
     */
    function sendToEditor() {
        var script = state.lastGeneratedScript;
        if (!script) { setStatus(tr("Nothing to send."), true); return; }
        if (Tours && typeof Tours.outputScript === "function") {
            Tours.outputScript(script, "scriptsgenerator");
            notify(tr("Sent to Script Editor"));
        } else {
            copyScript();
        }
    }

    // =====================================================================
    // INIT
    // =====================================================================

    /**
     * Initialize the Scripts Generator module.
     * Called once from mainui.js.
     *
     * @function init
     * @returns {void}
     */
    function init() {
        if (state.initialized) return;
        state.initialized = true;
        cacheDom();

        if (!$dom.typesContainer || !$dom.typesContainer.length) {
            console.warn("[ScriptsGen] Container not found");
            return;
        }

        if ($.fn.spinner) {
            $dom.duration.spinner({ min: 1, max: 30, step: 0.5 });
            $dom.zoomFov.spinner({ min: 1, max: 120, step: 1 });
            $dom.overviewFov.spinner({ min: 20, max: 180, step: 1 });
            $dom.fontSize.spinner({ min: 10, max: 40, step: 1 });
        }

        initControlPanel();
        bindUiEvents();
        renderApiStatus();

        setStatus(tr("Loading object types..."));
        fetchAllTypes()
            .then(function(types) {
                state.allTypes = types;
                state.apiReady = true;
                renderTypes();
                renderApiStatus();
                updateSummary();
                setStatus(tr("Ready. Select a type to begin."), false);
            })
            .catch(function(err) {
                state.apiReady = false;
                renderApiStatus();
                Render.renderPlaceholder($dom.typesContainer, tr("Failed to load."));
                setStatus(tr("Error: ") + err.message, true);
            });

        console.log("[ScriptsGen] Module initialized v3.0.0");
    }

    /**
     * Bind top-level UI events.
     *
     * @function bindUiEvents
     * @returns {void}
     */
    function bindUiEvents() {
        // i18n toggle
        $dom.useI18nCheckbox.off("change.sgI18n").on("change.sgI18n", function() {
            renderTypes();
            if (state.selectedType) onTypeSelected(state.selectedType.key);
        });

        // Multi-select
        $dom.multiSelectCheck.off("change.sgMulti").on("change.sgMulti", function() {
            toggleMultiSelect($(this).is(":checked"));
        });

        // Select All / Clear
        $dom.selectAllBtn.off("click.sgSelectAll").on("click.sgSelectAll", selectAllObjects);
        $dom.clearBtn.off("click.sgClear").on("click.sgClear", clearObjectSelection);

        // Lock observer info preview
        $dom.lockObserver.off("change.sgLock").on("change.sgLock", function() {
            if (!$(this).is(":checked")) { $dom.lockObserverInfo.text(""); return; }
            Api.fetchStatus().then(function(status) {
                if (status && status.location) {
                    $dom.lockObserverInfo.text(
                        "→ " + (status.location.name || "?") +
                        " @ " + (status.time.utc || "?")
                    );
                } else {
                    $dom.lockObserverInfo.text("→ " + tr("unavailable"));
                }
            });
        });

        // Generate / Copy / Send
        $dom.generateBtn.off("click.sgGen").on("click.sgGen", generateScript);
        $dom.copyBtn.off("click.sgCopy").on("click.sgCopy", copyScript);
        $dom.sendToEditorBtn.off("click.sgSend").on("click.sgSend", sendToEditor);

        // Help modals
        Help.bindHelpButtons(tr);

        // Stop timer on unload
        $(window).off("beforeunload.sgTimer").on("beforeunload.sgTimer", function() {
            stopVisibilityTimer();
        });
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        /**
         * Initialize the Scripts Generator module.
         * @public
         */
        init: init,

        /**
         * Exposed for debugging.
         * @private
         */
        _state: state
    };
});