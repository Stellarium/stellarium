/* ========================================================================
 * skyculture-multiselect.js - Unified Multi-Select Mode
 * ========================================================================
 * 
 * This module provides a unified multi-select implementation for all
 * pattern types in the Sky Culture subsystem. It replaces the previous
 * per-type implementations with a single, parameterized engine.
 * 
 * RESPONSIBILITIES:
 * -----------------
 * - Track multi-select mode per object type (constellation, asterism,
 *   lunar, star)
 * - Maintain the Set of selected IDs for each type
 * - Update selection count badges in real time
 * - Apply/remove visual styling to selected buttons
 * - Preserve selection state across re-renders
 * - Enable/disable the tour-generation button based on selection
 * 
 * DESIGN PRINCIPLES:
 * ------------------
 * 1. Single source of truth: State.raw.multiSelect holds all state.
 * 2. Pure helpers: toggle/select/deselect operate on the Set and DOM.
 * 3. ID contract: every type maps to a canonical prefix that MUST match
 *    the IDs produced by the control panel builder in skyculture-help.js.
 * 4. Non-destructive: visual updates never trigger re-renders.
 * 
 * ID CONTRACT (must match skyculture-help.js CONTROL_PANEL_CONFIG):
 * -----------------------------------------------------------------
 * | Type          | filterIdPrefix | Multi-Select Checkbox ID          |
 * |---------------|----------------|-----------------------------------|
 * | constellation | constellation  | #constellation-multi-select       |
 * | asterism      | asterisms      | #asterisms-multi-select           |
 * | lunar         | lunar          | #lunar-multi-select               |
 * | star          | stars          | #stars-multi-select               |
 * 
 * Corresponding selection counters:
 * | #constellation-selection-count |
 * | #asterisms-selection-count     |
 * | #lunar-selection-count         |
 * | #stars-selection-count         |
 * 
 * Corresponding tour-generation buttons:
 * | #btn-generate-const-static  |
 * | #btn-generate-aster-static  |
 * | #btn-generate-lunar-static  |
 * | #btn-generate-stars-static  |
 * 
 * @module skyculture/multiselect
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @requires ./skyculture-filters
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 2.0.0 - Unified single-engine implementation
 * ======================================================================== */

define(["jquery", "./skyculture-config", "./skyculture-state", "./skyculture-filters"],
		function($, Config, State, Filters) {
    "use strict";

    // =====================================================================
    // TYPE CONFIGURATION
    // =====================================================================

    /**
     * Per-type configuration for multi-select.
     * 
     * Fields:
     * - containerKey:    key in State.raw.ui.containers
     * - dataKey:         key in State.raw.data
     * - cssClass:        CSS class applied to selected buttons
     * - idPrefix:        prefix used by control panel builder
     * - generateBtnId:   ID of the tour-generation button
     * 
     * The idPrefix MUST match the filterIdPrefix used in
     * skyculture-help.js CONTROL_PANEL_CONFIG.
     * 
     * @constant {Object.<string, Object>}
     */
    var TYPE_CONFIG = {
        constellation: {
            containerKey:  'constellations',
            dataKey:       'constellations',
						filterKey:     'constellation',
            cssClass:      'selected-constellation',
            idPrefix:      'constellation',
            generateBtnId: 'btn-generate-const-static'
        },
        asterism: {
            containerKey:  'asterisms',
            dataKey:       'asterisms',
						filterKey:     'asterism',
            cssClass:      'selected-asterism',
            idPrefix:      'asterisms',
            generateBtnId: 'btn-generate-aster-static'
        },
        lunar: {
            containerKey:  'lunar',
            dataKey:       'lunar',
						filterKey:     'lunar',
            cssClass:      'selected-lunar',
            idPrefix:      'lunar',
            generateBtnId: 'btn-generate-lunar-static'
        },
        star: {
            containerKey:  'stars',
            dataKey:       'stars',
						filterKey:     'star',
            cssClass:      'selected-star',
            idPrefix:      'stars',
            generateBtnId: 'btn-generate-stars-static'
        }
    };

    /**
     * Canonical list of supported type keys.
     * @constant {Array<string>}
     */
    var TYPE_KEYS = Object.keys(TYPE_CONFIG);

    // =====================================================================
    // DYNAMIC BUTTON STATE (CONSTELLATIONS ONLY)
    // =====================================================================
    // 
    // The Constellations tab has TWO generate buttons:
    //   - #btn-generate-const-dynamic  → dynamic script (all constellations)
    //   - #btn-generate-const-static   → static array (respects multi-select)
    // 
    // Multi-Select ONLY applies to the static variant. When Multi-Select
    // is active, the dynamic button would still generate a script with
    // ALL constellations, ignoring the user's selection — which is
    // confusing. We therefore DISABLE the dynamic button while
    // Multi-Select is active, and restore it when Multi-Select is off.
    // 
    // The original `title` attribute is captured once (lazily) so that
    // we can restore it verbatim when re-enabling the button.
    // =====================================================================

    /**
     * Cached original title of the dynamic button, captured on first use.
     * @type {string|null}
     */
    var dynamicButtonOriginalTitle = null;

    /**
     * Explanation shown in the tooltip while the dynamic button is disabled.
     */
    var DYNAMIC_DISABLED_TITLE =
        'Dynamic script is disabled while Multi-Select is active. ' +
        'Multi-Select only affects the Static Array script. ' +
        'Disable Multi-Select to use the dynamic variant.';
				
    // =====================================================================
    // CONFIG ACCESSORS
    // =====================================================================

    /**
     * Get the config for a type key. Falls back to constellation config
     * if the type is unknown, ensuring no crash on typos.
     * 
     * @param {string} type - Object type key
     * @returns {Object}
     */
    function getTypeConfig(type) {
        return TYPE_CONFIG[type] || TYPE_CONFIG.constellation;
    }

    /**
     * Get the jQuery container for a type.
     * 
     * @param {string} type - Object type key
     * @returns {jQuery}
     */
    function getContainer(type) {
        var cfg = getTypeConfig(type);
        var $c = State.raw.ui.containers[cfg.containerKey];
        return ($c && $c.length) ? $c : $();
    }

    /**
     * Get the selection count display element for a type.
     * 
     * @param {string} type - Object type key
     * @returns {jQuery}
     */
    function getSelectionCountEl(type) {
        var cfg = getTypeConfig(type);
        return $('#' + cfg.idPrefix + '-selection-count');
    }

    /**
     * Get the tour-generation button for a type.
     * 
     * @param {string} type - Object type key
     * @returns {jQuery}
     */
    function getGenerateButton(type) {
        var cfg = getTypeConfig(type);
        return $('#' + cfg.generateBtnId);
    }
		
    /**
     * Synchronize the state of the Constellations dynamic-generate button
     * with the current Multi-Select state.
     * 
     * Behavior:
     *  - When Multi-Select is active for 'constellation':
     *      → dynamic button is DISABLED
     *      → its title is replaced with an explanatory message
     *  - Otherwise:
     *      → dynamic button is ENABLED
     *      → its original title is restored (captured on first use)
     * 
     * The static button (`#btn-generate-const-static`) is NOT touched:
     * its enabled/disabled state is already managed by
     * `updateSelectionCount()` (disabled when 0 selected in multi-select).
     * 
     * This function is idempotent and safe to call repeatedly.
     */
    function syncDynamicButtonState() {
        var $btn = $('#btn-generate-const-dynamic');
        if (!$btn.length) return;

        // Capture the original title once, before we ever modify it.
        if (dynamicButtonOriginalTitle === null) {
            dynamicButtonOriginalTitle = $btn.attr('title') || '';
        }

        var multiSelectOnConstellations = isActive('constellation');

        if (multiSelectOnConstellations) {
            // Disable and explain why
            $btn.prop('disabled', true);
            $btn.attr('title', DYNAMIC_DISABLED_TITLE);
            $btn.addClass('multi-select-disabled');
        } else {
            // Restore
            $btn.prop('disabled', false);
            $btn.attr('title', dynamicButtonOriginalTitle);
            $btn.removeClass('multi-select-disabled');
        }
    }
		
    // =====================================================================
    // MODE MANAGEMENT
    // =====================================================================

    /**
     * Check if multi-select mode is currently active.
     * 
     * @param {string} [type] - If provided, checks that specific type
     * @returns {boolean}
     */
    function isActive(type) {
        if (!State.raw.multiSelect.active) return false;
        if (!type) return true;
        return State.raw.multiSelect.mode === type;
    }

    /**
     * Enter multi-select mode for a type. Clears any previous mode.
     * 
     * @param {string} type - Object type key
     */
    function enter(type) {
        if (!TYPE_CONFIG[type]) {
            console.warn('[MultiSelect] Unknown type:', type);
            return;
        }

        // If switching from another type, clear the previous one
        var previous = State.raw.multiSelect.mode;
        if (previous && previous !== type) {
            clear(previous);
        }

        State.raw.multiSelect.active = true;
        State.raw.multiSelect.mode   = type;

        updateSelectionCount(type);
    }

    /**
     * Exit multi-select mode and clear all selections for the type.
     * 
     * @param {string} type - Object type key
     */
    function exit(type) {
        State.raw.multiSelect.active = false;
        State.raw.multiSelect.mode   = null;
        clear(type);
    }

    // =====================================================================
    // SELECTION SET ACCESS
    // =====================================================================

    /**
     * Get the Set of selected IDs for a type. Returns an empty Set if
     * the type is unknown, to keep callers safe.
     * 
     * @param {string} type - Object type key
     * @returns {Set<string>}
     */
    function getSelectedSet(type) {
        var set = State.raw.multiSelect.selected[type];
        return set || new Set();
    }

    // =====================================================================
    // VISUAL STYLING
    // =====================================================================

    /**
     * Apply the "selected" style to a button.
     * 
     * @param {jQuery} $btn - Button element
     * @param {string} cssClass - Type-specific CSS class
     */
    function applySelectedStyle($btn, cssClass) {
        if (!$btn || !$btn.length) return;
        $btn.addClass(cssClass);
        $btn.css('background', 'linear-gradient(#DCDBDA, #8A8C8E)');
        $btn.css('border-color', '#000000');
        $btn.css('color', '#000000');
    }

    /**
     * Remove the "selected" style from a button.
     * 
     * @param {jQuery} $btn - Button element
     * @param {string} cssClass - Type-specific CSS class
     */
    function removeSelectedStyle($btn, cssClass) {
        if (!$btn || !$btn.length) return;
        $btn.removeClass(cssClass);
        $btn.css('background', '');
        $btn.css('border-color', '');
        $btn.css('color', '');
        $btn.removeClass('active');
    }

    // =====================================================================
    // SELECTION OPERATIONS
    // =====================================================================

    /**
     * Toggle the selected state of a button.
     * 
     * @param {string} type - Object type key
     * @param {string} id - Pattern ID
     * @param {jQuery} [$btn] - The button element (for immediate visual update)
     * @returns {boolean} True if now selected, false if deselected
     */
    function toggle(type, id, $btn) {
        if (!id) return false;

        var set = getSelectedSet(type);
        var cfg = getTypeConfig(type);
        var isNowSelected;

        if (set.has(id)) {
            set.delete(id);
            isNowSelected = false;
            removeSelectedStyle($btn, cfg.cssClass);
        } else {
            set.add(id);
            isNowSelected = true;
            applySelectedStyle($btn, cfg.cssClass);
        }

        updateSelectionCount(type);
        return isNowSelected;
    }

    /**
     * Add multiple IDs to the selection.
     * 
     * @param {string} type - Object type key
     * @param {Array<string>} ids - IDs to add
     * @param {jQuery} [$buttons] - The buttons to update visually
     */
    function selectAll(type, ids, $buttons) {
        var set = getSelectedSet(type);
        var cfg = getTypeConfig(type);

        for (var i = 0; i < ids.length; i++) {
            set.add(ids[i]);
        }

        if ($buttons && $buttons.length) {
            applySelectedStyle($buttons, cfg.cssClass);
        }

        updateSelectionCount(type);
    }

    /**
     * Remove multiple IDs from the selection.
     * 
     * @param {string} type - Object type key
     * @param {Array<string>} ids - IDs to remove
     * @param {jQuery} [$buttons] - The buttons to update visually
     */
    function deselectAll(type, ids, $buttons) {
        var set = getSelectedSet(type);
        var cfg = getTypeConfig(type);

        for (var i = 0; i < ids.length; i++) {
            set.delete(ids[i]);
        }

        if ($buttons && $buttons.length) {
            removeSelectedStyle($buttons, cfg.cssClass);
        }

        updateSelectionCount(type);
    }

    /**
     * Clear all selected IDs for a type and reset button styles.
     * 
     * @param {string} type - Object type key
     */
    function clear(type) {
        var set = getSelectedSet(type);
        set.clear();

        var cfg = getTypeConfig(type);
        var $container = getContainer(type);

        if ($container.length) {
            var $btns = $container.find('.pattern-btn');
            $btns.removeClass(cfg.cssClass);
            $btns.css('background', '');
            $btns.css('border-color', '');
            $btns.css('color', '');
            $btns.removeClass('active');
        }

        updateSelectionCount(type);
    }

    /**
     * Clear all selections across all types and reset mode.
     */
    function clearAll() {
        for (var i = 0; i < TYPE_KEYS.length; i++) {
            clear(TYPE_KEYS[i]);
        }
        State.raw.multiSelect.active = false;
        State.raw.multiSelect.mode   = null;
    }

    // =====================================================================
    // SELECTION COUNT BADGE
    // =====================================================================

    /**
     * Update the selection count badge and enable/disable the generate
     * button based on the current selection size.
     * 
     * Also reconciles the Constellations dynamic-generate button so that
     * it is disabled while Multi-Select is active (see
     * `syncDynamicButtonState` for the full rationale).
     * 
     * @param {string} type - Object type key
     */
    function updateSelectionCount(type) {
        var count = getSelectedSet(type).size;
        var $el = getSelectionCountEl(type);

        if ($el.length) {
            $el.text(count + ' ' + (count === 1 ? 'selected' : 'selected'));
        }

        // Generate button is disabled when multi-select is active and no
        // items are selected; enabled otherwise.
        var $btn = getGenerateButton(type);
        if ($btn.length) {
            var shouldDisable = isActive(type) && count === 0;
            $btn.prop('disabled', shouldDisable);
        }

        // Reconcile the Constellations dynamic-generate button. This is a
        // no-op for all other types (the button doesn't exist in their tabs).
        syncDynamicButtonState();
    }

    // =====================================================================
    // STATE PRESERVATION (FOR RE-RENDERS)
    // =====================================================================

    /**
     * Snapshot the current multi-select state for a type.
     * Used before re-rendering to preserve selections.
     * 
     * @param {string} type - Object type key
     * @returns {{ids: Set<string>, active: boolean, mode: string|null}}
     */
    function saveState(type) {
        return {
            ids:    new Set(getSelectedSet(type)),
            active: State.raw.multiSelect.active,
            mode:   State.raw.multiSelect.mode
        };
    }

    /**
     * Restore a multi-select snapshot after re-rendering.
     * Re-applies visual styling to matching buttons.
     * 
     * @param {string} type - Object type key
     * @param {Object} savedState - Snapshot from saveState()
     */
    function restoreState(type, savedState) {
        if (!savedState) return;
        // NOTE: The saved IDs refer to pattern.id, which is stable across
        // re-renders. Even if the panel is re-rendered with new clones
        // (from applySort), the IDs remain the same, so the selection is
        // correctly restored.
				
        // Restore global mode
        State.raw.multiSelect.active = savedState.active;
        State.raw.multiSelect.mode   = savedState.mode;

        // Restore selection set
        var set = getSelectedSet(type);
        set.clear();
        savedState.ids.forEach(function(id) { set.add(id); });

        // Apply visual styling only if this type is the active mode
        if (!savedState.active || savedState.mode !== type) {
            updateSelectionCount(type);
            return;
        }

        var cfg = getTypeConfig(type);
        var $container = getContainer(type);

        if ($container.length) {
            $container.find('.pattern-btn').each(function() {
                var $btn = $(this);
                var id = $btn.data('pattern-id');
                if (set.has(id)) {
                    applySelectedStyle($btn, cfg.cssClass);
                }
            });
        }

        updateSelectionCount(type);
    }
		
		/**
		 * Map a type key to the `displayed` state key.
		 * 
		 * The `displayed` object in State.raw holds the last rendered order
		 * (filtered + sorted) for each type. Because the type keys
		 * ('constellation', 'asterism', 'lunar', 'star') differ from the
		 * `displayed` keys ('constellations', 'asterisms', 'lunar', 'stars'),
		 * we need this small translation table.
		 * 
		 * USED BY:
		 *   MultiSelect.getSelectedPatterns() — to read the on-screen order
		 *   of selected items and preserve it in generated tour scripts.
		 * 
		 * @constant {Object.<string, string>}
		 */
    var DISPLAYED_KEY = {
        'constellation': 'constellations',
        'asterism':      'asterisms',
        'lunar':         'lunar',
        'star':          'stars'
    };

    /**
     * Resolve the full pattern objects for all selected IDs of a type,
     * IN THE EXACT ORDER the user currently sees them on screen.
     * 
     * STRATEGY:
     * ---------
     * Read `State.raw.displayed[type]` — the array that was saved at the
     * last render (filtered + sorted). Walk it and keep only the selected
     * IDs. This is guaranteed to match the on-screen order because it IS
     * the on-screen order.
     * 
     * FALLBACKS:
     * ----------
     *   1. If `displayed[type]` is empty (user selected before any render
     *      completed), fall back to `State.raw.data[dataKey]` and sort by
     *      the current `filterState` via Filters.applyAll.
     *   2. If that also fails, fall back to raw data order.
     * 
     * @param {string} type - Object type key
     * @returns {Array|null} Selected patterns in display order, or null
     */
    function getSelectedPatterns(type) {
        if (!isActive(type)) return null;

        var ids = getSelectedSet(type);
        if (ids.size === 0) return null;

        var cfg = getTypeConfig(type);
        var displayedKey = DISPLAYED_KEY[type] || cfg.dataKey;
        var ordered = State.raw.displayed && State.raw.displayed[displayedKey];

        // -----------------------------------------------------------------
        // Fast path: use the last displayed order (exact match)
        // -----------------------------------------------------------------
        if (ordered && ordered.length) {
            var selected = [];
            for (var i = 0; i < ordered.length; i++) {
                if (ids.has(ordered[i].id)) {
                    selected.push(ordered[i]);
                }
            }
            if (selected.length > 0) {
                return selected;
            }
            // If nothing matched, fall through to slow path (e.g. the
            // displayed array may be stale after a culture change).
        }

        // -----------------------------------------------------------------
        // Slow path: re-apply filters on raw data
        // -----------------------------------------------------------------
        var source = State.raw.data[cfg.dataKey] || [];
        if (!source.length) return null;

        var infoMap = {};
        for (var j = 0; j < source.length; j++) {
            infoMap[source[j].id] = source[j]._info || null;
        }

        var reordered;
        try {
            var filterState = State.raw.filters[cfg.filterKey] || {};
            var result = Filters.applyAll(source, filterState, infoMap, type);
            reordered = result.patterns || source;
        } catch (e) {
            console.warn('[MultiSelect] applyAll failed, using raw order:', e);
            reordered = source;
        }

        var selectedFallback = [];
        for (var k = 0; k < reordered.length; k++) {
            if (ids.has(reordered[k].id)) {
                selectedFallback.push(reordered[k]);
            }
        }

        return selectedFallback.length > 0 ? selectedFallback : null;
    }
		
    // =====================================================================
    // CLICK HANDLER ATTACHMENT
    // =====================================================================

    /**
     * Attach multi-select click handlers to all buttons in a container.
     * 
     * This is called after every render when multi-select mode is active.
     * It replaces any previous handler bound by the render layer.
     * 
     * @param {string} type - Object type key
     * @param {jQuery} [$container] - Optional container override
     */
    function attachHandlers(type, $container) {
        $container = ($container && $container.length) ? $container : getContainer(type);
        if (!$container.length) return;

        // Individual button toggle
        $container.find('.pattern-btn')
            .off('click.multiselect')
            .on('click.multiselect', function(e) {
                e.preventDefault();
                e.stopPropagation();

                var $btn = $(this);
                var id = $btn.data('pattern-id');
                if (!id) return false;

                toggle(type, id, $btn);
                return false;
            });

        // Group-level "Select All" / "Deselect All"
        $container.find('.star-group-select-all')
            .off('click.multiselect')
            .on('click.multiselect', function(e) {
                e.preventDefault();
                e.stopPropagation();

                var groupName = $(this).data('group');
                var $groupButtons = $container.find(
                    '.star-group-buttons[data-group="' + groupName + '"] .pattern-btn'
                );

                var ids = [];
                var allSelected = true;
                var set = getSelectedSet(type);

                $groupButtons.each(function() {
                    var gid = $(this).data('pattern-id');
                    if (gid) {
                        ids.push(gid);
                        if (!set.has(gid)) allSelected = false;
                    }
                });

                var $btn = $(this);
                if (allSelected) {
                    deselectAll(type, ids, $groupButtons);
                    $btn.text('Select All');
                } else {
                    selectAll(type, ids, $groupButtons);
                    $btn.text('Deselect All');
                }

                return false;
            });
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        // Type metadata
        TYPE_CONFIG: TYPE_CONFIG,
        TYPE_KEYS:   TYPE_KEYS,

        // Mode
        isActive: isActive,
        enter:    enter,
        exit:     exit,

        // Selection
        getSelectedSet:      getSelectedSet,
        getSelectedPatterns: getSelectedPatterns,
        toggle:              toggle,
        selectAll:           selectAll,
        deselectAll:         deselectAll,
        clear:               clear,
        clearAll:            clearAll,
        updateSelectionCount:updateSelectionCount,

        // State preservation
        saveState:    saveState,
        restoreState: restoreState,

        // Handlers
        attachHandlers: attachHandlers,
				
        // Dynamic button state (constellations only)
        syncDynamicButtonState: syncDynamicButtonState
				
    };
});