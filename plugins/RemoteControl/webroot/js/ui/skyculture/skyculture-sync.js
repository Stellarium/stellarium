/* ========================================================================
 * skyculture-sync.js - Centralized Synchronization
 * ========================================================================
 * 
 * This module handles bidirectional synchronization between the Sky Culture
 * UI and Stellarium's internal selection state. It listens for the
 * `objectSelected` event and updates button highlighting accordingly.
 * 
 * WHY A CENTRAL NORMALIZER?
 * -------------------------
 * The current code base uses 3 different matching strategies duplicated
 * across multiple functions (setupSyncListeners, updateAllButtonStates).
 * This causes:
 * - Performance issues: O(n × 7) matching in the worst case
 * - Maintenance burden: adding a new type requires touching multiple places
 * - Inconsistent behavior: different match rules in different places
 * 
 * This module provides a SINGLE matching function that:
 * - Uses Map-based lookup for O(1) exact matches
 * - Falls back to partial matching (CON/AST abbreviations) only when needed
 * - Is used by both sync handlers and button-state updates
 * 
 * EVENT CONTRACT:
 * ---------------
 * Emitted by stellarium-utils:
 *   $(document).trigger('objectSelected', {
 *     name: 'Orion',
 *     id:   'CON modern Ori',
 *     type: 'constellation'
 *   });
 * 
 * Heard here, and translated into:
 * - Highlighting the corresponding button
 * - Updating State.selection
 * 
 * @module skyculture/sync
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @requires ./skyculture-render
 * @requires ./skyculture-multiselect
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define([
    "jquery",
    "./skyculture-config",
    "./skyculture-state",
    "./skyculture-render",
    "./skyculture-multiselect"
], function($, Config, State, Render, MultiSelect) {
    "use strict";

    var OBJECT_TYPES = Config.OBJECT_TYPES;

    // =====================================================================
    // BUTTON INDEX (PERFORMANCE)
    // =====================================================================

    /**
     * Map of patternId → $button for fast exact-match lookup.
     * Rebuilt whenever the UI is re-rendered.
     * @type {Map<string, jQuery>}
     */
    var buttonIndex = new Map();

    /**
     * Rebuild the button index by scanning all containers.
     * Call this after any render operation.
     */
    function rebuildIndex() {
        buttonIndex.clear();

        var containers = State.raw.ui.containers;
        var keys = ['constellations', 'asterisms', 'zodiac', 'lunar', 'stars'];

        for (var i = 0; i < keys.length; i++) {
            var $c = containers[keys[i]];
            if (!$c || !$c.length) continue;

            $c.find('.pattern-btn').each(function() {
                var $btn = $(this);
                var id = $btn.data('pattern-id');
                if (id) buttonIndex.set(String(id), $btn);
            });
        }
    }

    // =====================================================================
    // NORMALIZATION
    // =====================================================================

    /**
     * Given a raw identifier (ID or name) and type hint, find the canonical
     * pattern in State and return {type, id, name, pattern}.
     * 
     * Strategy:
     * 1. Exact ID match in State.data (fast, most reliable)
     * 2. Exact match via button index (covers dynamically-rendered buttons)
     * 3. Name-based match
     * 
     * @param {string} rawId - The ID from the objectSelected event
     * @param {string} rawName - The name from the event
     * @param {string} typeHint - The type from the event
     * @returns {Object|null} Normalized {type, id, name, pattern} or null
     */
    function normalize(rawId, rawName, typeHint) {
        var data = State.raw.data;

        // Build ordered list of (dataKey, typeKey) pairs to search
        var groups = [
            { dataKey: 'constellations', type: OBJECT_TYPES.CONSTELLATION },
            { dataKey: 'asterisms',      type: OBJECT_TYPES.ASTERISM },
            { dataKey: 'zodiac',         type: OBJECT_TYPES.ZODIAC },
            { dataKey: 'lunar',          type: OBJECT_TYPES.LUNAR },
            { dataKey: 'stars',          type: OBJECT_TYPES.STAR }
        ];

        // Try exact ID match first across all groups
        if (rawId) {
            for (var g = 0; g < groups.length; g++) {
                var group = groups[g];
                var list = data[group.dataKey] || [];
                for (var i = 0; i < list.length; i++) {
                    var p = list[i];
                    if (p.id === rawId) {
                        return { type: group.type, id: p.id, name: p.name, pattern: p };
                    }
                }
            }

            // Composite ID partial match for zodiac/lunar
            // (data.id may be "zodiac_modern_5" but pattern.id is "zodiac_modern_5")
            // This is handled by exact match above, so no additional logic needed.

            // CON/AST abbreviation fallback
            if (rawId.indexOf('CON ') === 0 || rawId.indexOf('AST ') === 0) {
                var rawParts = rawId.split(' ');
                for (var g2 = 0; g2 < groups.length; g2++) {
                    var group2 = groups[g2];
                    var list2 = data[group2.dataKey] || [];
                    for (var j = 0; j < list2.length; j++) {
                        var pj = list2[j];
                        if (pj.id.indexOf('CON ') === 0 || pj.id.indexOf('AST ') === 0) {
                            var parts = pj.id.split(' ');
                            if (parts.length >= 3 && rawParts.length >= 3 &&
                                parts[2] === rawParts[2]) {
                                return {
                                    type: group2.type,
                                    id: pj.id,
                                    name: pj.name,
                                    pattern: pj
                                };
                            }
                        }
                    }
                }
            }
        }

        // Name-based fallback
        if (rawName) {
            for (var g3 = 0; g3 < groups.length; g3++) {
                var group3 = groups[g3];
                var list3 = data[group3.dataKey] || [];
                for (var k = 0; k < list3.length; k++) {
                    var pk = list3[k];
                    if (pk.name === rawName ||
                        pk.searchName === rawName ||
                        pk.englishName === rawName ||
                        pk._displayName === rawName) {
                        return {
                            type: group3.type,
                            id: pk.id,
                            name: pk.name,
                            pattern: pk
                        };
                    }
                }
            }
        }

        return null;
    }

    // =====================================================================
    // BUTTON HIGHLIGHTING
    // =====================================================================

    /**
     * Remove active class from all buttons across all containers.
     */
    function clearAllActive() {
        Render.clearAllActiveButtons();
    }

    /**
     * Highlight the button matching a pattern ID. Uses the button index
     * for O(1) lookup, with a container-scan fallback.
     * 
     * @param {string} patternId - Pattern ID
     * @param {string} patternName - Optional name fallback
     */
    function highlightById(patternId, patternName) {
        clearAllActive();

        // Fast path: index lookup
        if (patternId && buttonIndex.has(String(patternId))) {
            buttonIndex.get(String(patternId)).addClass('active');
            return;
        }

        // Fallback: container scan
        if (!patternId && !patternName) return;

        var containers = State.raw.ui.containers;
        var keys = ['constellations', 'asterisms', 'zodiac', 'lunar', 'stars'];

        for (var i = 0; i < keys.length; i++) {
            var $c = containers[keys[i]];
            if (!$c || !$c.length) continue;

            var $match = null;

            $c.find('.pattern-btn').each(function() {
                var $btn = $(this);
                if (patternId && $btn.data('pattern-id') === patternId) {
                    $match = $btn;
                    return false;
                }
                if (patternName && $btn.data('pattern-name') === patternName) {
                    $match = $btn;
                    return false;
                }
            });

            if ($match) {
                $match.addClass('active');
                return;
            }
        }
    }

    /**
     * Highlight the artwork item matching a constellation ID.
     * 
     * @param {string} constellationId - Constellation ID
     */
    function highlightArtwork(constellationId) {
        var $artwork = State.raw.ui.containers.artwork;
        if (!$artwork || !$artwork.length) return;

        $artwork.find('.artwork-item').removeClass('selected-artwork');

        if (!constellationId) return;

        $artwork.find('.artwork-item').each(function() {
            if ($(this).data('artwork-id') === constellationId) {
                $(this).addClass('selected-artwork');
                return false;
            }
        });
    }

    // =====================================================================
    // EVENT HANDLERS
    // =====================================================================

    /**
     * Handle the `objectSelected` event.
     * 
     * @param {Event} evt - jQuery event
     * @param {Object} data - {name, id, type}
     */
    function onObjectSelected(evt, data) {
        if (!State.raw.flags.syncEnabled) return;

        // Empty selection → clear everything
        if (!data || !data.type || data.type === 'none' || !data.name) {
            State.clearSelection();
            clearAllActive();
            highlightArtwork(null);
            return;
        }

        // Normalize the incoming identifier
        var normalized = normalize(data.id, data.name, data.type);

        if (!normalized) {
            // Nothing matched → clear
            State.clearSelection();
            clearAllActive();
            highlightArtwork(null);
            return;
        }

        // Update state
        State.setSelection(normalized.name, normalized.id, normalized.type);

        // Update buttons
        highlightById(normalized.id, normalized.name);

        // Update artwork selection if constellation
        if (normalized.type === OBJECT_TYPES.CONSTELLATION) {
            highlightArtwork(normalized.id);
        } else if (data.type !== OBJECT_TYPES.CONSTELLATION) {
            highlightArtwork(null);
        }
    }

    /**
     * Handle server-side culture change.
     * 
     * @param {Function} onCultureChange - Callback to reload culture data
     * @returns {Function} Event handler
     */
    function createCultureChangeHandler(onCultureChange) {
        return function(evt, data) {
            var newCultureId = data && data.value;
            if (!newCultureId) return;
            if (newCultureId === State.raw.culture.id) return;

            State.raw.culture.id = newCultureId;

            // Update culture name from list
            var cultures = State.raw.culture.list || [];
            for (var i = 0; i < cultures.length; i++) {
                if (cultures[i].id === newCultureId) {
                    State.raw.culture.name = cultures[i].name;
                    break;
                }
            }

            if (onCultureChange) onCultureChange(newCultureId);
        };
    }

    // =====================================================================
    // SETUP
    // =====================================================================

    /**
     * Setup synchronization listeners.
     * 
     * @param {Function} onCultureChange - Callback when culture changes
     */
    function setup(onCultureChange) {
        $(document).off('objectSelected.skyculture').on(
            'objectSelected.skyculture',
            onObjectSelected
        );

        // Culture change listener is set up in the main module since it
        // requires propApi reference
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        setup:                     setup,
        rebuildIndex:              rebuildIndex,
        normalize:                 normalize,
        onObjectSelected:          onObjectSelected,
        highlightById:             highlightById,
        highlightArtwork:          highlightArtwork,
        clearAllActive:            clearAllActive,
        createCultureChangeHandler:createCultureChangeHandler
    };
});