/* ========================================================================
 * skyculture-data.js - Data Extraction from index.json
 * ========================================================================
 * 
 * This module extracts structured pattern data from the raw index.json
 * structure. It is the SINGLE SOURCE OF TRUTH for parsing culture data.
 * 
 * WHY index.json AND NOT THE API?
 * -------------------------------
 * The Stellarium API's listobjectsbytype endpoint returns flat arrays of
 * names without keys, IDs, or association with their parent objects. It
 * does not properly filter constellation/asterism names by culture and
 * may mix English with native names inconsistently.
 * 
 * index.json, in contrast, contains:
 * - Complete, authoritative data for each culture
 * - Stable IDs and structure
 * - Both English and native names
 * - All required fields (lines, images, pronunciations, etc.)
 * 
 * EXTRACTED DATA TYPES:
 * ---------------------
 * 
 * | Type          | Source Field       | ID Format                            |
 * |---------------|-------------------------------------|---------------------|
 * | Constellations| cultureData.constellations          | "CON culture XX"    |
 * | Asterisms     | cultureData.asterisms               | "AST culture NNN"   |
 * | Zodiac        | cultureData.zodiac.names            | "zodiac_culture_i"  |
 * | Lunar         | cultureData.lunar_system.names      | "lunar_i"           |
 * | Stars         | cultureData.common_names            | "HIP 12345" etc.    |
 * | Artwork       | cultureData.constellations[i].image | (const ID)          |
 * 
 * @module skyculture/data
 * @requires ./skyculture-config
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define(["./skyculture-config"], function(Config) {
    "use strict";

    // =====================================================================
    // CONSTANTS
    // =====================================================================

    /**
     * Names of solar system objects in the "NAME X" format, mapped to
     * their canonical Stellarium names.
     * @constant {Object.<string, string>}
     */
    var SOLAR_SYSTEM_DEFAULT_NAMES = {
        'NAME Sun':     'Sun',
        'NAME Moon':    'Moon',
        'NAME Mercury': 'Mercury',
        'NAME Venus':   'Venus',
        'NAME Earth':   'Earth',
        'NAME Mars':    'Mars',
        'NAME Jupiter': 'Jupiter',
        'NAME Saturn':  'Saturn',
        'NAME Uranus':  'Uranus',
        'NAME Neptune': 'Neptune',
        'NAME Pluto':   'Pluto'
    };

    // =====================================================================
    // HELPERS
    // =====================================================================

    /**
     * Extract English and native names from a common_name object.
     * 
     * @param {Object} commonName - {english, native, ...} object
     * @returns {{english: string|null, native: string|null, display: string|null}}
     */
    function extractNames(commonName) {
        if (!commonName) {
            return { english: null, native: null, display: null };
        }

        var english = (commonName.english && String(commonName.english).trim()) || null;
        var native  = (commonName.native  && String(commonName.native).trim())  || null;
        var display = english || native || null;

        return { english: english, native: native, display: display };
    }

    /**
     * Check if an English name looks like a code (e.g., "ARA_01").
     * These are not real names and should be skipped.
     * 
     * @param {string} name - Name to check
     * @returns {boolean}
     */
    function isCodeLikeName(name) {
        return /^[A-Z]+_[0-9]+$/.test(name);
    }

    // =====================================================================
    // CONSTELLATIONS
    // =====================================================================

    /**
     * Extract constellations from culture data.
     * 
     * Display name priority: English → Native → ID
     * Search name: always English (or fallback) for reliable API lookup
     * 
     * @param {Object} cultureData - Raw index.json data
     * @returns {Array} Array of constellation pattern objects
     */
    function extractConstellations(cultureData) {
        if (!cultureData || !cultureData.constellations ||
            !Array.isArray(cultureData.constellations)) {
            return [];
        }

        var patterns = [];
        var list = cultureData.constellations;

        for (var i = 0; i < list.length; i++) {
            var con = list[i];
            if (!con.id) continue;

            var names = extractNames(con.common_name);
            var displayName = names.display || con.id;
            var searchName  = names.english || displayName;

            patterns.push({
                id:          con.id,
                name:        displayName,
                searchName:  searchName,
                type:        Config.OBJECT_TYPES.CONSTELLATION,
                englishName: names.english,
                nativeName:  names.native,
                _displayName: displayName,
                _aboveHorizon: null
            });
        }

        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });

        return patterns;
    }

    // =====================================================================
    // ASTERISMS
    // =====================================================================

    /**
     * Extract asterisms from culture data.
     * Ray helpers (is_ray_helper === true) are skipped.
     * 
     * @param {Object} cultureData - Raw index.json data
     * @returns {Array} Array of asterism pattern objects
     */
    function extractAsterisms(cultureData) {
        if (!cultureData || !cultureData.asterisms ||
            !Array.isArray(cultureData.asterisms)) {
            return [];
        }

        var patterns = [];
        var list = cultureData.asterisms;

        for (var i = 0; i < list.length; i++) {
            var ast = list[i];

            // Skip ray helpers
            if (ast.is_ray_helper === true) continue;
            if (!ast.id) continue;

            var names = extractNames(ast.common_name);
            var displayName = names.display || ast.id;
            var searchName  = names.english || displayName;

            patterns.push({
                id:          ast.id,
                name:        displayName,
                searchName:  searchName,
                type:        Config.OBJECT_TYPES.ASTERISM,
                englishName: names.english,
                nativeName:  names.native,
                _displayName: displayName,
                _aboveHorizon: null
            });
        }

        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });

        return patterns;
    }

    // =====================================================================
    // ZODIAC
    // =====================================================================

    /**
     * Extract zodiac signs from culture data.
     * 
     * Composite IDs are generated as "zodiac_<cultureId>_<index>" to
     * prevent partial matching issues (e.g., "zodiac_modern_0" should
     * not match "zodiac_modern_10").
     * 
     * @param {Object} cultureData - Raw index.json data
     * @param {string} cultureId - Current culture ID
     * @returns {Array|null} Zodiac pattern objects or null
     */
    function extractZodiac(cultureData, cultureId) {
        if (!cultureData || !cultureData.zodiac ||
            !cultureData.zodiac.names ||
            !cultureData.zodiac.names.length) {
            return null;
        }

        var patterns  = [];
        var signs     = cultureData.zodiac.names;
        var culture   = cultureId || 'unknown';

        for (var i = 0; i < signs.length; i++) {
            var sign     = signs[i];
            var signName = sign.english || sign.native || ('Sign ' + (i + 1));

            // Simple ID for backward compatibility
            var simpleId = signName.toLowerCase().replace(/\s+/g, '_');

            // Composite ID prevents "zodiac_modern_0" matching "zodiac_modern_10"
            var compositeId = 'zodiac_' + culture + '_' + i;

            patterns.push({
                id:          compositeId,
                simpleId:    simpleId,
                name:        signName,
                searchName:  signName,
                type:        Config.OBJECT_TYPES.ZODIAC,
                englishName: sign.english || null,
                nativeName:  sign.native  || null,
                symbol:      sign.symbol  || null,
                _displayName: signName
            });
        }

        return patterns;
    }

    // =====================================================================
    // LUNAR MANSIONS
    // =====================================================================

    /**
     * Extract lunar mansions from culture data.
     * 
     * @param {Object} cultureData - Raw index.json data
     * @returns {Array|null} Lunar mansion pattern objects or null
     */
    function extractLunarMansions(cultureData) {
        if (!cultureData || !cultureData.lunar_system ||
            !cultureData.lunar_system.names ||
            !cultureData.lunar_system.names.length) {
            return null;
        }

        var patterns = [];
        var list     = cultureData.lunar_system.names;

        for (var i = 0; i < list.length; i++) {
            var mansion     = list[i];
            var mansionName = mansion.english || mansion.native || ('Mansion ' + (i + 1));

            patterns.push({
                id:          'lunar_' + (i + 1),
                name:        mansionName,
                searchName:  mansionName,
                type:        Config.OBJECT_TYPES.LUNAR,
                englishName: mansion.english || null,
                nativeName:  mansion.native  || null,
                symbol:      mansion.symbol  || null,
                _displayName: mansionName,
                _aboveHorizon: null
            });
        }

        return patterns;
    }

    // =====================================================================
    // STARS & CELESTIAL OBJECTS
    // =====================================================================

    /**
     * Extract notable star and celestial object names from culture data.
     * 
     * Handles multiple name variants for the same HIP ID and combines
     * them for display. Limits results to Config.MAX_STAR_NAMES entries
     * to prevent UI performance degradation.
     * 
     * Special handling for solar system objects (NAME prefix):
     * - "NAME Venus" → id: "Venus", isSolarSystem: true
     * 
     * @param {Object} cultureData - Raw index.json data
     * @returns {Array|null} Star pattern objects or null
     */
    function extractStars(cultureData) {
        if (!cultureData || !cultureData.common_names ||
            typeof cultureData.common_names !== 'object' ||
            Object.keys(cultureData.common_names).length === 0) {
            return null;
        }

        var patterns     = [];
        var commonNames  = cultureData.common_names;

        for (var rawId in commonNames) {
            if (!commonNames.hasOwnProperty(rawId)) continue;

            var entries = commonNames[rawId];
            if (!entries || !Array.isArray(entries) || entries.length === 0) continue;

            var displayNames      = [];
            var primaryName       = null;
            var primaryNativeName = null;

            // Handle NAME prefix (solar system objects)
            var searchId            = rawId;
            var isSolarSystemObject = false;

            if (rawId.indexOf('NAME ') === 0) {
                isSolarSystemObject = true;
                searchId = rawId.substring(5).trim();
            }

            // Collect all valid names
            for (var i = 0; i < entries.length; i++) {
                var entry = entries[i];

                if (entry.english && String(entry.english).trim()) {
                    var en = String(entry.english).trim();
                    if (!isCodeLikeName(en)) {
                        displayNames.push({ type: 'english', value: en });
                        if (primaryName === null) primaryName = en;
                    }
                }

                if (entry.native && String(entry.native).trim()) {
                    var na = String(entry.native).trim();
                    if (primaryNativeName === null) primaryNativeName = na;

                    var exists = displayNames.some(function(dn) {
                        return dn.value === na;
                    });
                    if (!exists) {
                        displayNames.push({ type: 'native', value: na });
                    }
                }
            }

            // Determine display and search names
            var displayName    = '';
            var finalSearchName = '';

            if (displayNames.length === 0) {
                displayName     = searchId;
                finalSearchName = searchId;
            } else {
                displayName     = displayNames[0].value;
                finalSearchName = primaryName || primaryNativeName || displayNames[0].value;

                // Combine multiple names for compact display
                if (displayNames.length === 2) {
                    displayName = displayNames[0].value + ' - ' + displayNames[1].value;
                } else if (displayNames.length >= 3) {
                    displayName = displayNames[0].value + ' - ' + displayNames[1].value;
                    var additional = displayNames.slice(2).map(function(dn) {
                        return dn.value;
                    }).join(' / ');
                    displayName += ' (' + additional + ')';
                }

                // Truncate extremely long display names
                if (displayName.length > Config.MAX_DISPLAY_NAME_LENGTH) {
                    displayName = displayName.substring(0,
                        Config.MAX_DISPLAY_NAME_LENGTH - 3) + '...';
                }
            }

            // For solar system objects, search name is the object name
            if (isSolarSystemObject) {
                finalSearchName = searchId;
            }

            patterns.push({
                id:            searchId,
                originalId:    rawId,
                name:          displayName,
                searchName:    finalSearchName,
                type:          Config.OBJECT_TYPES.STAR,
                isSolarSystem: isSolarSystemObject,
                englishName:   primaryName,
                nativeName:    primaryNativeName,
                _displayName:  displayName,
                _aboveHorizon: null
            });
        }

        if (patterns.length === 0) return null;

        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });

        // Limit to prevent UI lag with extremely large cultures
        if (patterns.length > Config.MAX_STAR_NAMES) {
            patterns = patterns.slice(0, Config.MAX_STAR_NAMES);
        }

        return patterns;
    }

    // =====================================================================
    // ARTWORK
    // =====================================================================

    /**
     * Extract constellation artwork from culture data.
     * 
     * @param {Object} cultureData - Raw index.json data
     * @param {Array} constellations - Pre-loaded constellation patterns
     * @returns {Array} Array of artwork objects
     */
    function extractArtwork(cultureData, constellations) {
        var artworks = [];

        if (!cultureData || !constellations || !constellations.length) {
            return artworks;
        }

        if (!cultureData.constellations || !Array.isArray(cultureData.constellations)) {
            return artworks;
        }

        var list = cultureData.constellations;

        for (var i = 0; i < list.length; i++) {
            var con = list[i];
            if (!con.image || !con.image.file) continue;

            var names       = extractNames(con.common_name);
            var displayName = names.display || con.id;
            var searchName  = names.english || displayName;

            artworks.push({
                id:          con.id,
                name:        displayName,
                searchName:  searchName,
                imageFile:   con.image.file,
                imagePath:   '/api/view/skyculturedescription/' + con.image.file,
                nativeName:  names.native,
                englishName: names.english,
                originalData: con
            });
        }

        artworks.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });

        return artworks;
    }

    // =====================================================================
    // SOLAR SYSTEM NAME MAPPING (exposed for use elsewhere)
    // =====================================================================

    /**
     * Get the canonical Stellarium name for a solar system object.
     * 
     * @param {string} nameWithPrefix - e.g. "NAME Venus"
     * @returns {string} Canonical name, e.g. "Venus"
     */
    function getSolarSystemDefaultName(nameWithPrefix) {
        return SOLAR_SYSTEM_DEFAULT_NAMES[nameWithPrefix] ||
               nameWithPrefix.substring(5).trim();
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        extractConstellations:     extractConstellations,
        extractAsterisms:          extractAsterisms,
        extractZodiac:             extractZodiac,
        extractLunarMansions:      extractLunarMansions,
        extractStars:              extractStars,
        extractArtwork:            extractArtwork,
        getSolarSystemDefaultName: getSolarSystemDefaultName,
        SOLAR_SYSTEM_DEFAULT_NAMES: SOLAR_SYSTEM_DEFAULT_NAMES
    };
});