/* ========================================================================
 * skyculture-config.js - Centralized Configuration for Sky Culture Module
 * ========================================================================
 * 
 * This module provides all configuration constants and default values
 * for the Sky Culture subsystem. Centralizing configuration here:
 * 
 * - Makes tuning values easy to find and modify
 * - Avoids magic numbers scattered across the codebase
 * - Provides a single source of truth for defaults
 * - Documents the meaning and range of each value
 * 
 * @module skyculture/config
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define([], function() {
    "use strict";

    // =====================================================================
    // FIELD OF VIEW DEFAULTS
    // =====================================================================

    /**
     * Default Field of View (in degrees) used when navigating to each
     * type of celestial object. These values provide optimal viewing
     * angles for the typical extent of each object category.
     * 
     * @constant {Object.<string, number>}
     */
    var DEFAULT_FOV = {
        constellation: 60,
        asterism: 30,
        zodiac: 40,
        lunar: 30,
        star: 15
    };

    /**
     * Valid ranges for each FOV control. Used by spinners and validation.
     * @constant {Object.<string, {min: number, max: number, step: number}>}
     */
    var FOV_RANGES = {
        constellation: { min: 5, max: 120, step: 1 },
        asterism:      { min: 5, max: 90,  step: 1 },
        lunar:         { min: 5, max: 90,  step: 1 },
        star:          { min: 5, max: 120, step: 1 }
    };

    // =====================================================================
    // CACHE CONFIGURATION
    // =====================================================================

    /**
     * Time-to-live for constellation data cache in milliseconds.
     * After this period, cached data is considered stale and will be
     * re-fetched from the server.
     * @constant {number}
     */
    var CONSTELLATION_CACHE_TTL = 5 * 60 * 1000; // 5 minutes

    // =====================================================================
    // VISIBILITY CACHE CONFIGURATION
    // =====================================================================
    // 
    // Visibility (above-horizon) data is the ONLY API field that changes
    // continuously with time, as objects rise and set. All other fields
    // (localized-name, iauConstellation, vmag, object-type, star-type,
    // variable-star, cultural-names) are IMMUTABLE for a given
    // (cultureId, patternId) pair.
    // 
    // This means we can cache the full `_info` object permanently and
    // only refresh the `aboveHorizon` field periodically when the user
    // has enabled the "Visible above horizon only" filter.
    // 
    // The user can choose the refresh interval via a radio group:
    //   - 30 seconds (default — balanced)
    //   - 60 seconds (less server load)
    //   - 180 seconds / 3 minutes (minimal server load)
    //   - manual (no automatic refresh; user clicks a button)
    // 
    // A value of 0 means "manual mode" — no automatic fetch.
    // =====================================================================

    /**
     * Default visibility cache TTL in milliseconds.
     * Used when no radio selection is available.
     * @constant {number}
     */
    var VISIBILITY_CACHE_TTL_DEFAULT = 180 * 1000; // 180 seconds = 3 minute

    /**
     * Available visibility refresh modes.
     * Maps the radio button value to the TTL in milliseconds.
     * A value of 0 means "manual mode" — no automatic fetch.
     * @constant {Object.<string, number>}
     */
    var VISIBILITY_CACHE_MODES = {
        '60s':    60 * 1000,       // 60 seconds
        '180s':    180 * 1000,       // 3 minute
        '420s':   420 * 1000,      // 7 minutes
        'manual': 0                // manual only
    };

    /**
     * Maximum concurrent API requests when fetching batch data.
     * Higher values = faster but more server load.
     * 
     * NOTE: For large cultures (3000+ stars), the initial fetch can be
     * slow. Increasing this value speeds up the fetch but may overload
     * the server. A value of 16 is a good balance.
     * @constant {number}
     */
    var FETCH_CONCURRENCY = 10;
		
    /**
     * Timeout for individual API requests in milliseconds.
     * @constant {number}
     */
    var FETCH_TIMEOUT = 5000;

    /**
     * Timeout for object info requests in milliseconds.
     * @constant {number}
     */
    var OBJECT_INFO_TIMEOUT = 3000;

    // =====================================================================
    // DATA LIMITS
    // =====================================================================

    /**
     * Maximum number of star names to display to prevent UI performance
     * degradation with extremely large datasets.
     * @constant {number}
     */
    var MAX_STAR_NAMES = 3200;

    /**
     * Maximum display name length before truncation.
     * @constant {number}
     */
    var MAX_DISPLAY_NAME_LENGTH = 140;

    /**
     * Maximum size (in bytes) of a generated script before it's saved as
     * a file instead of being inserted into the editor.
     * @constant {number}
     */
    var MAX_SCRIPT_SIZE = 50 * 1024; // 50 KB

    // =====================================================================
    // UI TIMING
    // =====================================================================

    /**
     * Delay before fetching data when user input changes (debounce).
     * @constant {number}
     */
    var DEBOUNCE_DELAY = 300;

    /**
     * Duration of UI animations in milliseconds.
     * @constant {number}
     */
    var ANIMATION_DURATION = 200;

    /**
     * Duration of notification messages in milliseconds.
     * @constant {number}
     */
    var NOTIFICATION_DURATION = 2000;

    /**
     * Delay before clearing status messages.
     * @constant {number}
     */
    var STATUS_CLEAR_DELAY = 10000;

    // =====================================================================
    // API ENDPOINTS
    // =====================================================================

    /**
     * Centralized API endpoint definitions.
     * @constant {Object.<string, string>}
     */
    var API_ENDPOINTS = {
        listSkyCultures:      "/api/view/listskyculture",
        skyCultureIndex:      "/api/view/skyculturedescription/index.json",
        skyCultureDescription:"/api/view/skyculturedescription/",
        objectInfo:           "/api/objects/info",
        objectFind:           "/api/objects/find",
        focus:                "/api/main/focus",
        scriptDirect:         "/api/scripts/direct",
        stelPropertySet:      "/api/stelproperty/set",
        status:               "/api/main/status"
    };

    // =====================================================================
    // OBJECT TYPES
    // =====================================================================

    /**
     * Canonical object type identifiers used throughout the module.
     * @constant {Object.<string, string>}
     */
    var OBJECT_TYPES = {
        CONSTELLATION: 'constellation',
        ASTERISM:      'asterism',
        ZODIAC:        'zodiac',
        LUNAR:         'lunar',
        STAR:          'star',
        ARTWORK:       'artwork',
        NONE:          'none'
    };

    /**
     * Pattern type identifiers used in UI containers.
     * @constant {Array<string>}
     */
    var PATTERN_TYPES = ['constellations', 'asterisms', 'zodiac', 'lunar', 'stars'];

    // =====================================================================
    // NAME DISPLAY MODES
    // =====================================================================

    /**
     * Available name display modes.
     * @constant {Object.<string, string>}
     */
    var NAME_DISPLAY_MODES = {
        DEFAULT:   'default',
        CULTURAL:  'cultural',
        LOCALIZED: 'localized'
    };

    // =====================================================================
    // SORT OPTIONS
    // =====================================================================

    /**
     * Available sort criteria for stars.
     * @constant {Array<string>}
     */
    var STAR_SORT_OPTIONS = ['name', 'constellation', 'magnitude', 'type'];

    /**
     * Available sort directions.
     * @constant {Object.<string, string>}
     */
    var SORT_DIRECTIONS = {
        ASC:  'asc',
        DESC: 'desc'
    };

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        DEFAULT_FOV:              DEFAULT_FOV,
        FOV_RANGES:               FOV_RANGES,
        CONSTELLATION_CACHE_TTL:  CONSTELLATION_CACHE_TTL,
        VISIBILITY_CACHE_TTL_DEFAULT: VISIBILITY_CACHE_TTL_DEFAULT,
        VISIBILITY_CACHE_MODES:   VISIBILITY_CACHE_MODES,				
        FETCH_CONCURRENCY:        FETCH_CONCURRENCY,
        FETCH_TIMEOUT:            FETCH_TIMEOUT,
        OBJECT_INFO_TIMEOUT:      OBJECT_INFO_TIMEOUT,
        MAX_STAR_NAMES:           MAX_STAR_NAMES,
        MAX_DISPLAY_NAME_LENGTH:  MAX_DISPLAY_NAME_LENGTH,
        MAX_SCRIPT_SIZE:          MAX_SCRIPT_SIZE,
        DEBOUNCE_DELAY:           DEBOUNCE_DELAY,
        ANIMATION_DURATION:       ANIMATION_DURATION,
        NOTIFICATION_DURATION:    NOTIFICATION_DURATION,
        STATUS_CLEAR_DELAY:       STATUS_CLEAR_DELAY,
        API_ENDPOINTS:            API_ENDPOINTS,
        OBJECT_TYPES:             OBJECT_TYPES,
        PATTERN_TYPES:            PATTERN_TYPES,
        NAME_DISPLAY_MODES:       NAME_DISPLAY_MODES,
        STAR_SORT_OPTIONS:        STAR_SORT_OPTIONS,
        SORT_DIRECTIONS:          SORT_DIRECTIONS
    };
});