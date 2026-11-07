/* ========================================================================
 * skyculture-filters.js - Unified Filtering, Sorting, and Grouping
 * ========================================================================
 * 
 * This module provides a unified set of filter/sort/group operations that
 * are shared across all pattern types. Each function is pure (no side
 * effects) and operates on arrays of pattern objects plus optional info
 * maps (from the /api/objects/info endpoint).
 * 
 * UNIFIED OPERATIONS:
 * -------------------
 * 
 * | Operation      | Constellations | Asterisms | Zodiac | Lunar | Stars |
 * |----------------|----------------|-----------|--------|-------|-------|
 * | Visibility     | ✅             | ✅        | ❌     | ✅    | ✅    |
 * | Name Mode      | ✅ (localized) | ✅        | ✅     | ✅    | ✅    |
 * | Sort By Name   | ✅             | ✅        | ✅     | ✅    | ✅    |
 * | Sort By Const. | ✅ (parent)    | ✅(parent)| ❌     | ❌    | ✅    |
 * | Sort By Mag    | ❌             | ❌        | ❌     | ❌    | ✅    |
 * | Sort By Type   | ❌             | ❌        | ❌     | ❌    | ✅    |
 * | Grouping       | ✅ (parent)    | ✅(parent)| ❌     | ❌    | ✅    |
 * 
 * NAME MODES:
 * -----------
 * - default:    Use the pattern's name from index.json
 * - cultural:   Use info['cultural-names'] (stars only)
 * - localized:  Use info['localized-name'] (all types)
 * 
 * @module skyculture/filters
 * @requires ./skyculture-config
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define(["./skyculture-config"], function(Config) {
    "use strict";

    // =====================================================================
    // STRING CLEANING
    // =====================================================================

    /**
     * Remove invisible Unicode control characters from a name string.
     * These are present in some API responses and would break display.
     * 
     * Removes:
     * - LTR/RTL marks (U+200E, U+200F)
     * - Zero-width spaces (U+200B–U+200D)
     * - Directional formatting (U+202A–U+202E)
     * - Isolates (U+2066–U+2069)
     * - BOM (U+FEFF)
     * 
     * @param {string} str - Input string
     * @returns {string} Cleaned string
     */
    function cleanName(str) {
        if (!str) return '';
        return String(str)
            .replace(/[\u200B-\u200F\u202A-\u202E\u2066-\u2069\uFEFF]/g, '')
            .trim();
    }

    // =====================================================================
    // INFO MAP
    // =====================================================================

    /**
     * Build a lookup map from pattern ID to its API info.
     * 
     * @param {Array<{pattern: Object, info: Object|null}>} enrichedData
     * @returns {Object.<string, Object>} Map of patternId → info
     */
    function buildInfoMap(enrichedData) {
        var map = {};
        if (!enrichedData || !enrichedData.length) return map;

        for (var i = 0; i < enrichedData.length; i++) {
            var entry = enrichedData[i];
            var id = entry.pattern ? entry.pattern.id : entry.item ? entry.item.id : null;
            if (id) map[id] = entry.info;
        }
        return map;
    }

    // =====================================================================
    // DISPLAY NAME
    // =====================================================================

    /**
     * Compute the display name for a pattern based on the name mode.
     * 
     * @param {Object} pattern - Pattern object with {name, englishName, nativeName}
     * @param {Object|null} info - API info (may be null)
     * @param {string} mode - 'default' | 'cultural' | 'localized'
     * @returns {string} Display name
     */
    function getDisplayName(pattern, info, mode) {
        mode = mode || Config.NAME_DISPLAY_MODES.DEFAULT;

        // Fallback: pattern's name
        var fallback = pattern.name || pattern.id;

        if (!info) return fallback;

        if (mode === Config.NAME_DISPLAY_MODES.LOCALIZED) {
            var localized = cleanName(info.localizedName);
            if (localized) return localized;
            return fallback;
        }

        if (mode === Config.NAME_DISPLAY_MODES.CULTURAL) {
            // cultural-names is only provided for stars and some objects
            // The API returns it as info['cultural-names'] when available
            var cultural = info.culturalNames;
            if (cultural && Array.isArray(cultural) && cultural.length > 0) {
                // Cultural names come in a format like "Matariki (ماتاريكي)"
                // We extract the first part before the parenthesis
                var raw = cleanName(cultural[0]);
                var parts = raw.split('(');
                var name = parts[0].trim();
                if (name) return name;
            }
            return fallback;
        }

        // Default mode
        return fallback;
    }

    /**
     * Apply the display name mode to a list of patterns (in place).
     * 
     * @param {Array} patterns - Pattern list to modify
     * @param {Object} infoMap - Map of patternId → info
     * @param {string} mode - Name display mode
     */
    function applyNameMode(patterns, infoMap, mode) {
        for (var i = 0; i < patterns.length; i++) {
            var p = patterns[i];
            var info = infoMap[p.id] || null;
            p._displayName = getDisplayName(p, info, mode);
        }
    }

    // =====================================================================
    // VISIBILITY FILTER
    // =====================================================================

    /**
     * Filter a list of patterns to keep only those above the horizon.
     * Patterns without info are KEPT (safe default).
     * 
     * @param {Array} patterns - Pattern list
     * @param {Object} infoMap - Map of patternId → info
     * @returns {{visible: Array, visibleCount: number, totalCount: number}}
     */
    function applyVisibility(patterns, infoMap) {
        var visible = [];
        var total   = patterns.length;

        for (var i = 0; i < total; i++) {
            var p = patterns[i];
            var info = infoMap[p.id];

            // If we have no info, keep the pattern (safe default)
            if (!info) {
                visible.push(p);
                p._aboveHorizon = null;
                continue;
            }

            var isVisible = info.aboveHorizon === true;
            p._aboveHorizon = isVisible;

            if (isVisible) visible.push(p);
        }

        return {
            visible:      visible,
            visibleCount: visible.length,
            totalCount:   total
        };
    }

    // =====================================================================
    // SORTING
    // =====================================================================

    /**
     * Compare function for stars by magnitude.
     * Missing magnitudes are sorted to the end.
     */
    function compareByMagnitude(a, b, direction) {
        var ma = a._info && typeof a._info.vmag === 'number' ? a._info.vmag : 999;
        var mb = b._info && typeof b._info.vmag === 'number' ? b._info.vmag : 999;
        var result = ma - mb;
        return direction === Config.SORT_DIRECTIONS.DESC ? -result : result;
    }

    /**
     * Compare function for objects by name.
     */
    function compareByName(a, b, direction) {
        var na = (a._displayName || a.name || '').toLowerCase();
        var nb = (b._displayName || b.name || '').toLowerCase();
        var result = na.localeCompare(nb);
        return direction === Config.SORT_DIRECTIONS.DESC ? -result : result;
    }

    /**
     * Compare function for objects by parent constellation.
     */
    function compareByConstellation(a, b, direction) {
        var ca = (a._info && a._info.iauConstellation) || 'Unknown';
        var cb = (b._info && b._info.iauConstellation) || 'Unknown';
        // "Unknown" always goes last regardless of direction
        if (ca === 'Unknown' && cb !== 'Unknown') return 1;
        if (cb === 'Unknown' && ca !== 'Unknown') return -1;
        var result = ca.localeCompare(cb);
        return direction === Config.SORT_DIRECTIONS.DESC ? -result : result;
    }

    /**
     * Compare function for stars by type.
     */
    function compareByType(a, b, direction) {
        var ta = (a._info && a._info.objectType) || '';
        var tb = (b._info && b._info.objectType) || '';
        var result = ta.localeCompare(tb);
        return direction === Config.SORT_DIRECTIONS.DESC ? -result : result;
    }

    /**
     * Sort patterns by the given criterion.
     * Attaches `_info` to each pattern temporarily for comparison.
     * 
     * @param {Array} patterns - Pattern list (will be copied)
     * @param {string} sortBy - 'name' | 'constellation' | 'magnitude' | 'type'
     * @param {string} direction - 'asc' | 'desc'
     * @param {Object} infoMap - Map of patternId → info
     * @returns {Array} New sorted array
     */
    function applySort(patterns, sortBy, direction, infoMap) {
        sortBy    = sortBy    || 'name';
        direction = direction || Config.SORT_DIRECTIONS.ASC;

				// Copy array and attach _info for comparison
				var sorted = patterns.map(function(p) {
						// Shallow clone to avoid polluting original with _info
						var clone = Object.create(Object.getPrototypeOf(p));
						for (var k in p) if (p.hasOwnProperty(k)) clone[k] = p[k];
						
						// IMPORTANT: Preserve the original _info cache.
						// - If infoMap has an entry, use it (fresh from API).
						// - Otherwise, fall back to the pattern's own cached _info.
						// - Only if BOTH are missing, set to null.
						// 
						// This ensures that sorting never destroys the _info cache
						// acquired by a previous fetch.
						// 
						// CONSEQUENCE:
						// The returned clones carry _info, so downstream consumers
						// (e.g. MultiSelect.getSelectedPatterns) can reuse it without
						// refetching. The original `patterns` array is unaffected
						// (it still has its own _info).
						// 
						// See SKYCULTURE-ARCHITECTURE.md §12.2 for the full rationale.
						clone._info = infoMap[p.id] || p._info || null;
						
						return clone;
				});

        sorted.sort(function(a, b) {
            switch (sortBy) {
                case 'constellation':
                    return compareByConstellation(a, b, direction);
                case 'magnitude':
                    return compareByMagnitude(a, b, direction);
                case 'type':
                    return compareByType(a, b, direction);
                case 'name':
                default:
                    return compareByName(a, b, direction);
            }
        });

        // =====================================================================
        // DETACH _info FROM THE CLONES? NO — KEEP IT!
        // =====================================================================
        // Previous versions deleted _info after sorting. This was wrong
        // because it destroyed the cache. We now KEEP _info on the sorted
        // clones so that the rendering layer can use it directly without
        // needing to re-fetch.
        // 
        // The sorted clones are returned to the caller, which will render
        // them. The original `patterns` array is unaffected (it still has
        // its own _info).
        // =====================================================================

        return sorted;
    }

    // =====================================================================
    // GROUPING
    // =====================================================================

    /**
     * Group patterns by parent constellation or object type.
     * 
     * @param {Array} patterns - Pattern list
     * @param {string} groupBy - 'constellation' | 'type'
     * @param {Object} infoMap - Map of patternId → info
     * @returns {Object.<string, {name: string, items: Array}>}
     */
    function applyGrouping(patterns, groupBy, infoMap) {
        var groups = {};

        for (var i = 0; i < patterns.length; i++) {
            var p    = patterns[i];
            var info = infoMap[p.id];
            var key;

            if (groupBy === 'constellation') {
                key = (info && info.iauConstellation) || 'Unknown';
            } else if (groupBy === 'type') {
                key = (info && info.objectType) || 'Unknown';
            } else {
                key = 'All';
            }

            if (!groups[key]) {
                groups[key] = { name: key, items: [] };
            }
            groups[key].items.push(p);
        }

        // Sort group keys alphabetically, "Unknown" last
        var sortedKeys = Object.keys(groups).sort(function(a, b) {
            if (a === 'Unknown') return 1;
            if (b === 'Unknown') return -1;
            return a.localeCompare(b);
        });

        var result = {};
        for (var j = 0; j < sortedKeys.length; j++) {
            result[sortedKeys[j]] = groups[sortedKeys[j]];
        }

        return result;
    }

    // =====================================================================
    // STAR TYPE FILTER
    // =====================================================================

    /**
     * Filter stars by their star-type (from API info).
     * 
     * Available star types:
     * - 'all':          no filter
     * - 'star':         regular stars
     * - 'double-star':  double/multiple stars
     * - 'variable':     variable stars (any kind)
     * 
     * @param {Array} patterns - Star pattern list
     * @param {Object} infoMap - Map of patternId → info
     * @param {string} starType - Filter type
     * @returns {Array} Filtered patterns
     */
    function applyStarTypeFilter(patterns, infoMap, starType) {
        if (!starType || starType === 'all') return patterns;

        var result = [];
        for (var i = 0; i < patterns.length; i++) {
            var p    = patterns[i];
            var info = infoMap[p.id];

            // If we have no info, keep (safe default)
            if (!info) {
                result.push(p);
                continue;
            }

            var matches = false;
            var st = info.starType || '';
            var vs = info.variableStar || '';

            switch (starType) {
                case 'star':
                    matches = (st === 'star') && (vs === 'no' || !vs);
                    break;
                case 'double-star':
                    matches = (st === 'double-star');
                    break;
                case 'variable':
                    matches = (vs && vs !== 'no') || (st && st.indexOf('variable') !== -1);
                    break;
                default:
                    matches = true;
            }

            if (matches) result.push(p);
        }

        return result;
    }

    // =====================================================================
    // HIGH-LEVEL PIPELINE
    // =====================================================================

    /**
     * Apply the complete filter pipeline for a given type.
     * 
     * @param {Array} patterns - Original patterns
     * @param {Object} filterState - Filter state for this type
     * @param {Object} infoMap - Map of patternId → info
     * @param {string} type - Object type ('constellation', 'asterism', 'lunar', 'star')
     * @returns {{
     *     patterns: Array,
     *     visibleCount: number,
     *     totalCount: number,
     *     grouped: Object|null
     * }}
     */
    function applyAll(patterns, filterState, infoMap, type) {
        filterState = filterState || {};
        var result = {
            patterns:     patterns.slice(),
            visibleCount: patterns.length,
            totalCount:   patterns.length,
            grouped:      null
        };

        // 1. Visibility filter
        if (filterState.visible) {
            var vis = applyVisibility(result.patterns, infoMap);
            result.patterns     = vis.visible;
            result.visibleCount = vis.visibleCount;
            result.totalCount   = vis.totalCount;
        } else {
            // Still compute visibility flags for display
            applyVisibility(result.patterns, infoMap);
        }

        // 2. Star-type filter (stars only)
        if (type === 'star' && filterState.starType && filterState.starType !== 'all') {
            result.patterns = applyStarTypeFilter(result.patterns, infoMap, filterState.starType);
        }

        // 3. Display name mode
        applyNameMode(result.patterns, infoMap, filterState.nameMode);

        // 4. Sort
        if (filterState.sortBy) {
            result.patterns = applySort(
                result.patterns,
                filterState.sortBy,
                filterState.sortDir,
                infoMap
            );
        }

        // 5. Grouping (stars only, for constellation/type sorts)
        if (type === 'star' &&
            (filterState.sortBy === 'constellation' || filterState.sortBy === 'type')) {
            result.grouped = applyGrouping(result.patterns, filterState.sortBy, infoMap);
        }

        return result;
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        cleanName:            cleanName,
        buildInfoMap:         buildInfoMap,
        getDisplayName:       getDisplayName,
        applyNameMode:        applyNameMode,
        applyVisibility:      applyVisibility,
        applySort:            applySort,
        applyGrouping:        applyGrouping,
        applyStarTypeFilter:  applyStarTypeFilter,
        applyAll:             applyAll
    };
});