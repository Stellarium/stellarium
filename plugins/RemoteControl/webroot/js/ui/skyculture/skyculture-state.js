/* ========================================================================
 * skyculture-state.js - Centralized State Management
 * ========================================================================
 * 
 * This module provides a centralized state container for the Sky Culture
 * subsystem. All mutable state is kept here, avoiding scattered variables
 * and ensuring a single source of truth.
 * 
 * State is organized into logical groups:
 * - selection:    current single-selection (pattern, id, type)
 * - multiSelect:  multi-select mode state per object type
 * - filters:      filter/sort state per object type
 * - data:         current loaded data arrays
 * - cache:        API response caches
 * - ui:           UI element references and transient flags
 * 
 * @module skyculture/state
 * @requires jquery
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define(["jquery", "./skyculture-config"], function($, Config) {
    "use strict";

    var OBJECT_TYPES = Config.OBJECT_TYPES;

    // =====================================================================
    // STATE CONTAINER
    // =====================================================================

    /**
     * Central state container. All module state is stored here.
     * @type {Object}
     */
    var State = {

        // -----------------------------------------------------------------
        // Selection (single-selection mode)
        // -----------------------------------------------------------------
        selection: {
            /** @type {string|null} Display name of selected pattern */
            pattern: null,
            /** @type {string|null} Unique ID of selected pattern */
            id: null,
            /** @type {string|null} Type of selected pattern */
            type: null
        },

        // -----------------------------------------------------------------
        // Multi-Select mode
        // -----------------------------------------------------------------
        multiSelect: {
            /** @type {boolean} Whether multi-select mode is active */
            active: false,
            /** @type {string|null} Current multi-select type */
            mode: null,
            /** @type {Object.<string, Set>} Selected IDs per type */
            selected: {
                constellation: new Set(),
                asterism:      new Set(),
                lunar:         new Set(),
                star:          new Set()
            }
        },

        // -----------------------------------------------------------------
        // Filters per object type
        // -----------------------------------------------------------------
        filters: {
            constellation: {
                visible:   false,       // "above horizon only"
                nameMode:  'default',   // 'default' | 'localized'
                sortBy:    'name',
                sortDir:   'asc'
            },
            asterism: {
                visible:   false,
                nameMode:  'default',
                sortBy:    'name',
                sortDir:   'asc'
            },
            lunar: {
                visible:   false,
                nameMode:  'default',
                sortBy:    'name',
                sortDir:   'asc'
            },
            star: {
                visible:   false,
                nameMode:  'default',
                sortBy:    'name',
                sortDir:   'asc',
                starType:  'all'        // 'all' | 'star' | 'double-star' | 'variable'
            }
        },

        // -----------------------------------------------------------------
        // Field of View per type
        // -----------------------------------------------------------------
        fov: {
            constellation: Config.DEFAULT_FOV.constellation,
            asterism:      Config.DEFAULT_FOV.asterism,
            zodiac:        Config.DEFAULT_FOV.zodiac,
            lunar:         Config.DEFAULT_FOV.lunar,
            star:          Config.DEFAULT_FOV.star
        },

        // -----------------------------------------------------------------
        // Loaded data
        // -----------------------------------------------------------------
        data: {
            constellations: [],
            asterisms:      [],
            zodiac:         [],
            lunar:          [],
            stars:          [],
            artwork:        [],
            raw:            null        // Raw index.json data
        },

        // -----------------------------------------------------------------
        // Original (unfiltered) data for restoration
        // -----------------------------------------------------------------
        original: {
            constellations: [],
            asterisms:      [],
            lunar:          [],
            stars:          []
        },

        // -----------------------------------------------------------------
        // Caches
        // -----------------------------------------------------------------
        cache: {
            constellation: {},          // cultureId|constId -> {info, timestamp, ...}
            star:          {},          // cultureId|starId -> info
            displayName:   {}           // cultureId|id|mode -> displayName
        },

        // -----------------------------------------------------------------
        // Culture info
        // -----------------------------------------------------------------
        culture: {
            id:   null,
            name: null,
            list: []
        },

        // -----------------------------------------------------------------
        // UI element references (jQuery objects)
        // -----------------------------------------------------------------
        ui: {
            containers: {
                cultures:       null,
                constellations: null,
                asterisms:      null,
                zodiac:         null,
                lunar:          null,
                stars:          null,
                artwork:        null
            },
            infoFrame:      null,
            cultureCount:   null
        },

        // -----------------------------------------------------------------
        // Transient flags (loading, syncing, etc.)
        // -----------------------------------------------------------------
        flags: {
            isLoadingConstellationData: false,
            isSortingStars:             false,
            isArtworkClickLocked:       false,
            isInitialized:              false,
            syncEnabled:                true
        },

        // -----------------------------------------------------------------
        // Displayed order per type (set after each render)
        // -----------------------------------------------------------------
        // Holds the last rendered (filtered + sorted) pattern array for each
        // type. This is the EXACT sequence the user sees on screen. Used by
        // MultiSelect.getSelectedPatterns to preserve that order in the
        // generated script, so the script reflects what the user picked.
        // -----------------------------------------------------------------
        displayed: {
            constellations: [],
            asterisms:      [],
            lunar:          [], // lunar
            stars:          []
        },
				
        // -----------------------------------------------------------------
        // Timestamps
        // -----------------------------------------------------------------
        timestamps: {
            lastConstellationFetch: 0
        }
    };

    // =====================================================================
    // SELECTION HELPERS
    // =====================================================================

    /**
     * Set the current single selection.
     * 
     * @param {string|null} pattern - Display name
     * @param {string|null} id - Unique ID
     * @param {string|null} type - Object type
     */
    function setSelection(pattern, id, type) {
        State.selection.pattern = pattern;
        State.selection.id      = id;
        State.selection.type    = type;
    }

    /**
     * Clear the current single selection.
     */
    function clearSelection() {
        State.selection.pattern = null;
        State.selection.id      = null;
        State.selection.type    = null;
    }

    // =====================================================================
    // MULTI-SELECT HELPERS
    // =====================================================================

    /**
     * Get the Set of selected IDs for a given object type.
     * 
     * @param {string} type - Object type (constellation, asterism, lunar, star)
     * @returns {Set}
     */
    function getSelectedSet(type) {
        return State.multiSelect.selected[type] || null;
    }

    /**
     * Toggle selection of an ID for a given type.
     * 
     * @param {string} type - Object type
     * @param {string} id - ID to toggle
     * @returns {boolean} True if now selected, false if deselected
     */
    function toggleSelected(type, id) {
        var set = getSelectedSet(type);
        if (!set) return false;

        if (set.has(id)) {
            set.delete(id);
            return false;
        } else {
            set.add(id);
            return true;
        }
    }

    /**
     * Clear all multi-select selections for a given type.
     * 
     * @param {string} type - Object type
     */
    function clearSelected(type) {
        var set = getSelectedSet(type);
        if (set) set.clear();
    }

    /**
     * Clear all multi-select selections across all types.
     */
    function clearAllSelected() {
        Object.keys(State.multiSelect.selected).forEach(function(type) {
            State.multiSelect.selected[type].clear();
        });
    }

    // =====================================================================
    // FILTER HELPERS
    // =====================================================================

    /**
     * Get the filter state for a given type.
     * 
     * @param {string} type - Object type ('constellation', 'asterism', 'lunar', 'star')
     * @returns {Object}
     */
    function getFilter(type) {
        return State.filters[type] || null;
    }

    /**
     * Reset all filters to defaults.
     */
    function resetFilters() {
        State.filters.constellation = {
            visible: false, nameMode: 'default', sortBy: 'name', sortDir: 'asc'
        };
        State.filters.asterism = {
            visible: false, nameMode: 'default', sortBy: 'name', sortDir: 'asc'
        };
        State.filters.lunar = {
            visible: false, nameMode: 'default', sortBy: 'name', sortDir: 'asc'
        };
        State.filters.star = {
            visible: false, nameMode: 'default', sortBy: 'name', sortDir: 'asc',
            starType: 'all'
        };
    }

    // =====================================================================
    // DATA HELPERS
    // =====================================================================

    /**
     * Store the current data for a given type.
     * 
     * @param {string} type - Data type key in State.data
     * @param {Array} items - Array of pattern objects
     */
    function setData(type, items) {
        State.data[type] = items || [];
    }

    /**
     * Get the current data for a given type.
     * 
     * @param {string} type - Data type key
     * @returns {Array}
     */
    function getData(type) {
        return State.data[type] || [];
    }

    /**
     * Reset all loaded data when culture changes.
     * 
     * IMPORTANT: This also clears the `_info` cache stored on each pattern.
     * When the culture changes, all `_info` data becomes invalid because
     * it was fetched for the previous culture's objects. The next fetch
     * will populate `_info` with fresh data.
     */
    function resetData() {
        State.data.constellations = [];
        State.data.asterisms      = [];
        State.data.zodiac         = [];
        State.data.lunar          = [];
        State.data.stars          = [];
        State.data.artwork        = [];
        State.data.raw            = null;

        State.original.constellations = [];
        State.original.asterisms      = [];
        State.original.lunar          = [];
        State.original.stars          = [];

        // Clear _info cache from previous culture
        // (the arrays are now empty, but we clear for safety)
        // The actual _info on pattern objects is discarded automatically
        // because the pattern objects themselves are replaced.
        
        console.log('[SkyCulture] Data reset — _info cache will be rebuilt on next fetch');
    }	

    // =====================================================================
    // CACHE HELPERS
    // =====================================================================

    /**
     * Clear all caches.
     */
    function clearCaches() {
        State.cache.constellation = {};
        State.cache.star          = {};
        State.cache.displayName   = {};
    }

    /**
     * Clear constellation cache for a specific culture.
     * 
     * @param {string} cultureId - Culture ID
     */
    function clearConstellationCache(cultureId) {
        if (!cultureId) {
            State.cache.constellation = {};
            return;
        }
        var prefix = cultureId + '|';
        var keys = Object.keys(State.cache.constellation);
        for (var i = 0; i < keys.length; i++) {
            if (keys[i].indexOf(prefix) === 0) {
                delete State.cache.constellation[keys[i]];
            }
        }
    }

    /**
     * Explicitly clear the `_info` cache on an array of patterns.
     * 
     * This is used when the culture changes to ensure that no stale
     * `_info` survives. While the current implementation replaces the
     * entire pattern array (making this a no-op), keeping this function
     * makes the intent explicit and provides a hook for future
     * optimizations where patterns might be reused.
     * 
     * @param {Array} patterns - Array of pattern objects
     */
    function clearPatternsInfoCache(patterns) {
        if (!patterns || !patterns.length) return;
        for (var i = 0; i < patterns.length; i++) {
            delete patterns[i]._info;
        }
    }	

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        // Raw state access (for advanced use)
        raw: State,

        // Selection
        setSelection: setSelection,
        clearSelection: clearSelection,

        // Multi-select
        getSelectedSet: getSelectedSet,
        toggleSelected: toggleSelected,
        clearSelected: clearSelected,
        clearAllSelected: clearAllSelected,

        // Filters
        getFilter: getFilter,
        resetFilters: resetFilters,

        // Data
        setData: setData,
        getData: getData,
        resetData: resetData,

        // Cache
        clearCaches: clearCaches,
        clearPatternsInfoCache: clearPatternsInfoCache,				
        clearConstellationCache: clearConstellationCache
    };
});