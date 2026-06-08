/* ========================================================================
 * skyculture.js - Sky Culture Manager with Direct Index.json Data Loading
 * ========================================================================
 * 
 * This module provides a complete UI for managing sky cultures and their
 * associated astronomical data in the Stellarium Remote Control plugin.
 * 
 * =========================================================================
 * TABLE OF CONTENTS
 * =========================================================================
 * 1. Overview and Architecture
 * 2. Key Features
 * 3. Data Flow Diagram
 * 4. API Endpoints Used
 * 5. Module Dependencies
 * 6. Constants and Configuration
 * 7. Private Variables
 * 8. Helper Functions
 * 9. Data Extraction Functions (Direct from index.json)
 * 10. Panel Rendering Functions
 * 11. Culture Management Functions
 * 12. Synchronization Listeners
 * 13. Initialization
 * 14. Public API
 * 
 * =========================================================================
 * 1. OVERVIEW AND ARCHITECTURE
 * =========================================================================
 * 
 * The Sky Culture Manager provides a multi-panel tabbed interface for
 * exploring astronomical data across different cultural traditions. It
 * replaces the traditional dropdown selector with an interactive button
 * grid and organizes cultural patterns into categorized panels.
 * 
 * DATA SOURCE PHILOSOPHY:
 * -----------------------
 * This module uses index.json as the SINGLE SOURCE OF TRUTH for all
 * culture-specific data. This architectural decision was made because:
 * 
 * 1. The Stellarium API's listobjectsbytype endpoint does not properly
 *    filter constellation and asterism names by culture/language.
 * 2. The API may return inconsistent results (mixing English and native
 *    names, missing entries, or incorrect counts).
 * 3. index.json contains the complete, authoritative data for each culture.
 * 
 * By loading directly from index.json, we guarantee:
 * - Correct constellation and asterism counts
 * - Proper display name selection (English preferred, native as fallback)
 * - Functional search using the English name
 * - Consistent behavior across all 54+ sky cultures
 * 
 * ARCHITECTURE FLOW:
 * ------------------
 * 
 *   ┌─────────────────┐
 *   │ listskyculture  │  (GET /api/view/listskyculture)
 *   │ (API)           │  → Returns translated culture names (needs new Api) right now English names only
 *   └────────┬────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐
 *   │ Culture Buttons │  (User selects a culture)
 *   │ (UI Grid)       │
 *   └────────┬────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐     ┌─────────────────┐
 *   │ setSkyCulture   │───▶│ StelProperty    │  (POST /api/stelproperty/set)
 *   │                 │     │ Update          │
 *   └────────┬────────┘     └─────────────────┘
 *            │
 *            ▼
 *   ┌─────────────────┐
 *   │ index.json      │  (GET /api/view/skyculturedescription/index.json)
 *   │ (Single Source) │  → Complete culture data
 *   └────────┬────────┘
 *            │
 *            ├──────────────────────────────────────┐
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Constellations  │                    │ Asterisms       │
 *   │ (extract from   │                    │ (extract from   │
 *   │  index.json)    │                    │  index.json)    │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Zodiac Signs    │                    │ Lunar Mansions  │
 *   │ (extract from   │                    │ (extract from   │
 *   │  index.json)    │                    │  index.json)    │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            ▼                                      ▼
 *   ┌─────────────────┐                    ┌─────────────────┐
 *   │ Notable Stars   │                    │ Description     │
 *   │ (extract from   │                    │ (iframe)        │
 *   │  index.json)    │                    │                 │
 *   └─────────────────┘                    └─────────────────┘
 *            │                                      │
 *            └──────────────┬───────────────────────┘
 *                           ▼
 *                  ┌─────────────────┐
 *                  │ RENDER PANELS   │
 *                  │ (Button Grids)  │
 *                  └─────────────────┘
 * 
 * =========================================================================
 * 2. KEY FEATURES
 * =========================================================================
 * 
 * - Sky culture selection via responsive, scrollable button grid
 * - Six-panel tabbed interface (Constellations, Asterisms, Zodiac Signs,
 *   Lunar Mansions, Notable Stars, Description)
 * - Direct data extraction from index.json (no API filtering issues)
 * - Constellation isolation with automatic state preservation
 * - Type-specific FOV zoom levels (constellations: 60°, asterisms: 30°, etc.)
 * - Real-time synchronization with server-side culture changes
 * - Automatic selection clearing to prevent view conflicts
 * - Translation support for culture names (via listskyculture API)
 * - Lazy loading with visual loading placeholders
 * - Responsive design with mobile/touch support
 * 
 * =========================================================================
 * 3. DATA FLOW DIAGRAM
 * =========================================================================
 * 
 * User Action                Module Response              Server Interaction
 * ─────────────────────────────────────────────────────────────────────────
 * 
 * Page Load
 *     │
 *     ▼
 * init() ─────────────────────────────────────────────▶ GET /listskyculture
 *     │                                                      │
 *     │                                                      ▼
 *     │                                            Translated culture names
 *     │                                                      │
 *     ▼                                                      ▼
 * renderCultureButtons() ◀─────────────────────────── availableCultures[]
 *     │
 *     ▼
 * loadAllCultureData() ──────────────────────────────▶ GET /index.json
 *     │                                                      │
 *     │                                                      ▼
 *     │                                            Complete culture data
 *     │                                                      │
 *     ├── loadConstellationsFromIndexJson() ◀────────────────┤
 *     ├── loadAsterismsFromIndexJson()    ◀──────────────────┤
 *     ├── extractZodiac()                 ◀──────────────────┤
 *     ├── extractLunarMansions()          ◀──────────────────┤
 *     └── extractStarNames()              ◀──────────────────┤
 *     │                                                      │
 *     ▼                                                      │
 * renderConstellationsPanel()                                │
 * renderAsterismsPanel()                                     │
 * renderZodiacPanel()                                        │
 * renderLunarPanel()                                         │
 * renderStarsPanel()                                         │
 *     │                                                      │
 *     ▼                                                      │
 * User clicks constellation ──────────────────────────────────┤
 *     │                                                      │
 *     ▼                                                      │
 * stelUtils.toggleConstellationHighlight() ──────────────────┼──▶ POST /main/focus
 *                                                            │    POST /stelproperty/set
 *                                                            │
 * =========================================================================
 * 4. API ENDPOINTS USED
 * =========================================================================
 * 
 * | Endpoint                                      | Method | Purpose                    |
 * |-----------------------------------------------|--------|----------------------------|
 * | /api/view/listskyculture                      | GET    | List all cultures (translated) |
 * | /api/view/skyculturedescription/index.json    | GET    | Complete culture data (source of truth) |
 * | /api/stelproperty/set                         | POST   | Change current culture     |
 * | /api/view/skyculturedescription/              | GET    | Culture HTML description (iframe) |
 * 
 * Note: /api/objects/listobjectsbytype is NOT used for constellations
 * and asterisms due to inconsistent filtering behavior across cultures.
 * 
 * =========================================================================
 * 5. MODULE DEPENDENCIES
 * =========================================================================
 * 
 * @requires jquery               - DOM manipulation and AJAX
 * @requires api/properties       - StelProperty access
 * @requires api/remotecontrol    - Server communication and translation
 * @requires ui/stellarium-utils  - Shared navigation utilities
 * 
 * =========================================================================
 * 6. AUTHOR AND VERSION
 * =========================================================================
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-18
 * @license GPLv2+
 * @version 4.0.0 - Direct index.json loading (eliminates API filtering issues)
 * 
 * ======================================================================== */

define(["jquery", "api/properties", "api/remotecontrol", "ui/stellarium-utils"], 
    function($, propApi, rc, stelUtils) {
    "use strict";

    // =====================================================================
    // 6. CONSTANTS AND CONFIGURATION
    // =====================================================================

    /**
     * Default FOV (Field of View) values for different object types.
     * These values provide optimal viewing angles for each category.
     * @constant {Object.<string, number>}
     */
    var DEFAULT_FOV = {
        constellation: 60,
        asterism: 30,
        zodiac: 40,
        lunar: 30,
        star: 15
    };

    // =====================================================================
    // 7. PRIVATE VARIABLES
    // =====================================================================

    // ---------------------------------------------------------------------
    // DOM Elements
    // ---------------------------------------------------------------------

    /** @type {jQuery} Container for culture selection buttons */
    var $cultureContainer = null;
    
    /** @type {jQuery} Container for constellation buttons */
    var $constellationsContainer = null;
    
    /** @type {jQuery} Container for asterism buttons */
    var $asterismsContainer = null;
    
    /** @type {jQuery} Container for zodiac buttons */
    var $zodiacContainer = null;
    
    /** @type {jQuery} Container for lunar mansion buttons */
    var $lunarContainer = null;
    
    /** @type {jQuery} Container for star buttons */
    var $starsContainer = null;
    
    /** @type {jQuery} iframe for culture description */
    var $infoFrame = null;
    
    /** @type {jQuery} Element displaying culture count badge */
    var $cultureCount = null;

    // ---------------------------------------------------------------------
    // State Variables
    // ---------------------------------------------------------------------

    /** @type {string} Currently active culture ID (e.g., "korean", "arabic_al-sufi") */
    var currentCultureId = null;
    
    /** @type {string} Current Stellarium language code (e.g., 'en', 'ar', 'fr') */
    var currentLanguage = 'en';
    
    /** @type {string} Name of the currently active culture (translated) */
    var currentCultureName = null;
    
    /** @type {Array<{id: string, name: string}>} List of all available cultures */
    var availableCultures = [];
    
    /** @type {string} Name of the currently selected pattern (for UI highlighting) */
    var selectedPattern = null;
    
    /** @type {string} ID of the currently selected pattern */
    var selectedPatternId = null;
    
    /** 
     * Current patterns organized by type.
     * @type {Object.<string, Array>}
     */
    var currentPatternsData = { 
        constellations: [], 
        asterisms: [], 
        zodiac: [], 
        lunar: [], 
        stars: [] 
    };
    
    /** @type {Object} Raw culture data from index.json (cached for star navigation) */
    var currentCultureRawData = null;
    
    /** @type {boolean} Module initialization state */
    var isInitialized = false;
    
    /** @type {boolean} Synchronization enabled flag (can be toggled for debugging) */
    var syncEnabled = true;


    // ---------------------------------------------------------------------
    // Translation Function
    // ---------------------------------------------------------------------
    
    /** @type {Function} Translation function from remotecontrol module */
    var _tr = rc.tr;
		
    // =====================================================================
    // 8. HELPER FUNCTIONS
    // =====================================================================

    /**
     * Escape HTML special characters to prevent XSS attacks.
     * Converts the characters & < > " ' to their HTML entity equivalents.
     * 
     * @param {string} str - The string to escape
     * @returns {string} The escaped string, safe for HTML insertion
     */
    function escapeHtml(str) {
        if (!str) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    /**
     * Update the pattern count badge displayed in tab headers.
     * 
     * @param {string} type - Pattern type (constellations, asterisms, zodiac, lunar, stars)
     * @param {number} count - Number of items to display
     */
    function updatePatternCount(type, count) {
        var $countElement = $("#" + type + "-count");
        if ($countElement && $countElement.length) {
            $countElement.text(count > 0 ? "(" + count + ")" : "");
        }
    }

    /**
     * Clear active state from all pattern buttons across all containers.
     * Called when selection is cleared or culture is changed.
     */
    function clearAllActiveButtons() {
        var containers = [
            $constellationsContainer, $asterismsContainer, 
            $zodiacContainer, $lunarContainer, $starsContainer
        ];
        for (var i = 0; i < containers.length; i++) {
            if (containers[i] && containers[i].length) {
                containers[i].find(".pattern-btn").removeClass("active");
            }
        }
        selectedPattern = null;
        selectedPatternId = null;
    }

		/**
		 * Update button active states based on current selection.
		 * Ensures only the button matching selectedPatternId is highlighted.
		 * 
		 * Enhanced matching logic:
		 * - Exact match (most reliable)
		 * - Composite ID number extraction for zodiac and lunar mansions
		 * - Name-based match as final fallback
		 * 
		 * @function updateAllButtonStates
		 */
		function updateAllButtonStates() {
				var containers = [
						$constellationsContainer, $asterismsContainer, 
						$zodiacContainer, $lunarContainer, $starsContainer
				];
				
				for (var c = 0; c < containers.length; c++) {
						var container = containers[c];
						if (container && container.length) {
								container.find(".pattern-btn").removeClass("active");
								
								if (selectedPatternId) {
										container.find(".pattern-btn").each(function() {
												var $btn = $(this);
												var btnPatternId = $btn.data("pattern-id");
												
												var match = false;
												
												// ============================================================
												// METHOD 1: EXACT MATCH (MOST RELIABLE)
												// ============================================================
												if (btnPatternId === selectedPatternId) {
														match = true;
												}
												
												// ============================================================
												// METHOD 2: ZODIAC COMPOSITE ID MATCHING
												// Prevents "zodiac_modern_0" from matching "zodiac_modern_1"
												// Extracts and compares the numeric index
												// ============================================================
												if (!match && btnPatternId && selectedPatternId) {
														// Check if both are zodiac composite IDs
														if (btnPatternId.indexOf('zodiac_') === 0 && 
																selectedPatternId.indexOf('zodiac_') === 0) {
																var btnParts = btnPatternId.split('_');
																var selectedParts = selectedPatternId.split('_');
																var btnNum = parseInt(btnParts[btnParts.length - 1], 10);
																var selectedNum = parseInt(selectedParts[selectedParts.length - 1], 10);
																if (btnNum === selectedNum) {
																		match = true;
																}
														}
														// ========================================================
														// METHOD 3: LUNAR MANSION COMPOSITE ID MATCHING
														// Prevents "lunar_1" from matching "lunar_16"
														// ========================================================
														else if (btnPatternId.indexOf('lunar_') === 0 && 
																		 selectedPatternId.indexOf('lunar_') === 0) {
																var btnNum = parseInt(btnPatternId.split('_')[1], 10);
																var selectedNum = parseInt(selectedPatternId.split('_')[1], 10);
																if (btnNum === selectedNum) {
																		match = true;
																}
														}
														// ========================================================
														// METHOD 4: CONSTELLATION ABBREVIATION MATCHING
														// For CON prefix IDs (e.g., "CON chinese_manchu Borboho")
														// ========================================================
														else if (btnPatternId.indexOf('CON ') === 0 && 
																		 selectedPatternId.indexOf('CON ') === 0) {
																var btnParts = btnPatternId.split(' ');
																var selectedParts = selectedPatternId.split(' ');
																if (btnParts.length >= 3 && selectedParts.length >= 3) {
																		if (btnParts[2] === selectedParts[2]) {
																				match = true;
																		}
																}
														}
														// ========================================================
														// METHOD 5: ASTERISM ABBREVIATION MATCHING
														// For AST prefix IDs (e.g., "AST chinese_manchu 001")
														// ========================================================
														else if (btnPatternId.indexOf('AST ') === 0 && 
																		 selectedPatternId.indexOf('AST ') === 0) {
																var btnParts = btnPatternId.split(' ');
																var selectedParts = selectedPatternId.split(' ');
																if (btnParts.length >= 3 && selectedParts.length >= 3) {
																		if (btnParts[2] === selectedParts[2]) {
																				match = true;
																		}
																}
														}
														// ========================================================
														// METHOD 6: SIMPLE ID MATCHING (for backward compatibility)
														// ========================================================
														else if (btnPatternId === selectedPatternId) {
																match = true;
														}
												}
												
												// ============================================================
												// METHOD 7: MATCH BY NAME (FINAL FALLBACK)
												// Used when ID matching fails
												// ============================================================
												if (!match && selectedPattern) {
														var btnName = $btn.data("pattern-name");
														var btnSearchName = $btn.data("search-name");
														if (btnName === selectedPattern || btnSearchName === selectedPattern) {
																match = true;
																console.log("[SkyCulture] Matched by name fallback:", btnName);
														}
												}
												
												if (match) {
														$btn.addClass("active");
														console.log("[SkyCulture] Activated button:", $btn.data("pattern-name"), "ID:", btnPatternId);
												}
										});
								}
						}
				}
		}

    /**
     * Get the current Stellarium language from StelProperty.
     * 
     * @returns {string} Language code (e.g., 'en', 'ar', 'fr')
     */
    function getCurrentLanguage() {
        if (propApi) {
            var lang = propApi.getStelProp("StelLocaleMgr.appLanguage");
            if (lang) {
                currentLanguage = lang;
                return lang;
            }
        }
        return currentLanguage;
    }

    /**
     * Render buttons in a container with click handlers.
     * This is the core UI rendering function used by all pattern panels.
     * 
     * @param {jQuery} $container - Container element to populate
     * @param {Array} patterns - Array of pattern objects with properties:
     *                           {id, name, searchName, type}
     * @param {string} containerName - Name of container type (for placeholder text)
     * @param {Function} onClickHandler - Click handler function(patternName, patternType, searchName, patternId)
     * @param {boolean} disableOriginalHandler - If true, skip attaching the original handler (for multi-select mode)
     */
    function renderButtons($container, patterns, containerName, onClickHandler, disableOriginalHandler) {
        if (!$container || !$container.length) return;
        $container.empty();
        
        // Handle empty state
        if (!patterns || patterns.length === 0) {
            var placeholderText = _tr("No data available");
            if (containerName === "constellations") {
                placeholderText = _tr("No constellations available for this culture");
            } else if (containerName === "asterisms") {
                placeholderText = _tr("No asterisms available for this culture");
            } else if (containerName === "zodiac") {
                placeholderText = _tr("No zodiac data available for this culture");
            } else if (containerName === "lunar") {
                placeholderText = _tr("No lunar mansions data available for this culture");
            } else if (containerName === "stars") {
                placeholderText = _tr("No star name data available for this culture");
            }
            
            $container.html('<div class="loading-placeholder">' + placeholderText + '</div>');
            return;
        }
        
        // Build button grid
        var buttonsHtml = '<div class="patterns-buttons-grid">';
        for (var i = 0; i < patterns.length; i++) {
            var pattern = patterns[i];
            var isActive = (selectedPatternId === pattern.id);
            var tooltip = pattern.type === "star" ? ' title="' + escapeHtml(pattern.id) + '"' : '';
            
            buttonsHtml += '<button type="button" class="pattern-btn' + (isActive ? ' active' : '') + 
                '" data-pattern-id="' + escapeHtml(pattern.id) + 
                '" data-pattern-name="' + escapeHtml(pattern.name) + 
                '" data-pattern-type="' + escapeHtml(pattern.type) + 
                '" data-search-name="' + escapeHtml(pattern.searchName || pattern.name) + '"' + 
                tooltip + '>' + escapeHtml(pattern.name) + '</button>';
        }
        buttonsHtml += '</div>';
        $container.html(buttonsHtml);
        
        // Attach click handlers - ONLY if not disabled
        if (!disableOriginalHandler) {
            $container.find(".pattern-btn").on("click", function() {
                var $btn = $(this);
                var patternId = $btn.data("pattern-id");
                var patternName = $btn.data("pattern-name");
                var patternType = $btn.data("pattern-type");
                var searchName = $btn.data("search-name");
                
                if ($btn.hasClass('active')) {
                    // Toggle OFF: Deselect and clear highlight
                    $container.find(".pattern-btn").removeClass("active");
                    selectedPattern = null;
                    selectedPatternId = null;
                    if (patternType === "constellation") {
                        stelUtils.clearConstellationHighlight();
                    }
                } else {
                    // Toggle ON: Select and execute action
                    $container.find(".pattern-btn").removeClass("active");
                    $btn.addClass("active");
                    selectedPattern = patternName;
                    selectedPatternId = patternId;
                    if (patternName && onClickHandler) {
                        onClickHandler(patternName, patternType, searchName, patternId);
                    }
                }
            });
        }
    }

    // =====================================================================
    // 9. DATA EXTRACTION FUNCTIONS (Direct from index.json)
    // =====================================================================
    // 
    // These functions extract data directly from the index.json structure.
    // This bypasses the Stellarium API's inconsistent filtering behavior
    // and guarantees correct data for all 56+ sky cultures.
    // =====================================================================

    /**
     * Load constellations directly from index.json data.
     * 
     * Display Name Priority:
     * 1. English name (if available) - preferred for consistency
     * 2. Native name (if available) - fallback when English missing
     * 3. Constellation ID - absolute fallback
     * 
     * Search Name: Always uses English name (or fallback) for reliable
     * Stellarium object lookup.
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array} Constellation pattern objects with properties:
     *                  {id, name, searchName, type, englishName, nativeName}
     */
    function loadConstellationsFromIndexJson(cultureData) {
        var patterns = [];
        
        if (!cultureData || !cultureData.constellations || !Array.isArray(cultureData.constellations)) {
            console.warn("[SkyCulture] No constellations array in culture data");
            return patterns;
        }
        
        var totalConstellations = cultureData.constellations.length;
        
        for (var i = 0; i < totalConstellations; i++) {
            var con = cultureData.constellations[i];
            var id = con.id;
            
            if (!id) {
                console.warn("[SkyCulture] Constellation missing ID at index", i);
                continue;
            }
            
            // Extract name components
            var englishName = (con.common_name && con.common_name.english) 
                ? con.common_name.english.trim() 
                : null;
            var nativeName = (con.common_name && con.common_name.native) 
                ? con.common_name.native.trim() 
                : null;
            
            // Determine display name: English preferred, then native, then ID
            var displayName = englishName || nativeName || id;
            
            // Search name: English preferred for reliable Stellarium lookup
            var searchName = englishName || displayName;
            
            patterns.push({
                id: id,
                name: displayName,
                searchName: searchName,
                type: "constellation",
                englishName: englishName,
                nativeName: nativeName
            });
        }
        
        // Sort alphabetically by display name
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        console.log("[SkyCulture] Loaded " + patterns.length + " constellations from index.json " +
            "(total in file: " + totalConstellations + ")");
        
        return patterns;
    }

    /**
     * Load asterisms directly from index.json data.
     * 
     * This function filters out "ray helpers" (is_ray_helper === true) which
     * are auxiliary lines rather than proper asterisms.
     * 
     * Display Name Priority: Same as constellations (English → Native → ID)
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array} Asterism pattern objects
     */
    function loadAsterismsFromIndexJson(cultureData) {
        var patterns = [];
        
        if (!cultureData || !cultureData.asterisms || !Array.isArray(cultureData.asterisms)) {
            console.warn("[SkyCulture] No asterisms array in culture data");
            return patterns;
        }
        
        var totalAsterisms = cultureData.asterisms.length;
        var skippedRayHelpers = 0;
        
        for (var i = 0; i < totalAsterisms; i++) {
            var ast = cultureData.asterisms[i];
            
            // Skip ray helpers - they are auxiliary lines, not proper asterisms
            if (ast.is_ray_helper === true) {
                skippedRayHelpers++;
                continue;
            }
            
            var id = ast.id;
            if (!id) {
                console.warn("[SkyCulture] Asterism missing ID at index", i);
                continue;
            }
            
            // Extract name components
            var englishName = (ast.common_name && ast.common_name.english) 
                ? ast.common_name.english.trim() 
                : null;
            var nativeName = (ast.common_name && ast.common_name.native) 
                ? ast.common_name.native.trim() 
                : null;
            
            // Determine display name: English preferred, then native, then ID
            var displayName = englishName || nativeName || id;
            
            // Search name: English preferred for reliable Stellarium lookup
            var searchName = englishName || displayName;
            
            patterns.push({
                id: id,
                name: displayName,
                searchName: searchName,
                type: "asterism",
                englishName: englishName,
                nativeName: nativeName
            });
        }
        
        // Sort alphabetically by display name
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        console.log("[SkyCulture] Loaded " + patterns.length + " asterisms from index.json " +
            "(total: " + totalAsterisms + ", skipped " + skippedRayHelpers + " ray helpers)");
        
        return patterns;
    }

		/**
		 * Extract zodiac signs from culture data.
		 * 
		 * Note: Zodiac signs are always extracted from index.json as there is
		 * no API endpoint for this data type.
		 * 
		 * IMPORTANT: This function now generates composite IDs in the format
		 * "zodiac_cultureId_index" to prevent partial matching issues.
		 * For backward compatibility, the simple ID is also stored.
		 * 
		 * Example IDs:
		 * - compositeId: "zodiac_modern_0" (for Aries)
		 * - simpleId: "aries" (for backward compatibility)
		 * 
		 * @param {Object} cultureData - The culture data from index.json
		 * @returns {Array|null} Zodiac pattern objects with properties:
		 *                       {id, compositeId, name, searchName, type, englishName, nativeName, symbol}
		 */
		function extractZodiac(cultureData) {
				if (!cultureData || !cultureData.zodiac || !cultureData.zodiac.names || 
						!cultureData.zodiac.names.length) {
						return null;
				}
				
				var patterns = [];
				var zodiacNames = cultureData.zodiac.names;
				var cultureId = currentCultureId || 'unknown';
				
				for (var i = 0; i < zodiacNames.length; i++) {
						var sign = zodiacNames[i];
						var signName = sign.english || sign.native || ("Sign " + (i + 1));
						
						// Create simple ID for backward compatibility (e.g., "aries")
						var simpleId = signName.toLowerCase().replace(/\s+/g, '_');
						
						// Create composite ID to prevent partial matching issues
						// Format: "zodiac_cultureId_index" (e.g., "zodiac_modern_0")
						var compositeId = "zodiac_" + cultureId + "_" + i;
						
						patterns.push({ 
								id: compositeId,           // Primary ID for matching (prevents partial matches)
								simpleId: simpleId,        // Legacy ID for backward compatibility
								name: signName,            // Display name
								searchName: signName,      // Name used for API lookup
								type: "zodiac",
								englishName: sign.english || null,
								nativeName: sign.native || null,
								symbol: sign.symbol || null
						});
				}
				
				console.log("[SkyCulture] Extracted " + patterns.length + " zodiac signs with composite IDs");
				return patterns;
		}

    /**
     * Extract lunar mansions from culture data.
     * 
     * Note: Lunar mansions are always extracted from index.json as there is
     * no API endpoint for this data type.
     * 
     * @param {Object} cultureData - The culture data from index.json
     * @returns {Array|null} Lunar mansion pattern objects or null if not available
     */
    function extractLunarMansions(cultureData) {
        if (!cultureData || !cultureData.lunar_system || !cultureData.lunar_system.names || 
            !cultureData.lunar_system.names.length) {
            return null;
        }
        
        var patterns = [];
        var lunarNames = cultureData.lunar_system.names;
        
        for (var i = 0; i < lunarNames.length; i++) {
            var mansion = lunarNames[i];
            var mansionName = mansion.english || mansion.native || ("Mansion " + (i + 1));
            
            patterns.push({ 
                id: "lunar_" + (i + 1), 
                name: mansionName, 
                searchName: mansionName, 
                type: "lunar" 
            });
        }
        
        console.log("[SkyCulture] Extracted " + patterns.length + " lunar mansions");
        return patterns;
    }

		/**
		 * Extract notable star names from culture data.
		 * 
		 * Handles multiple name variants for the same star (HIP ID) and combines
		 * them for display. Limits results to 3200 entries to prevent UI
		 * performance degradation with extremely large datasets.
		 * 
		 * IMPORTANT: This function also handles special prefixes like "NAME"
		 * which are used for planets and solar system objects. For these,
		 * we extract the actual object name (e.g., "NAME Venus" -> "Venus")
		 * to ensure proper navigation in Stellarium.
		 * 
		 * @param {Object} cultureData - The culture data from index.json
		 * @returns {Array|null} Star pattern objects or null if not available
		 */
		function extractStarNames(cultureData) {
				if (!cultureData || !cultureData.common_names || 
						typeof cultureData.common_names !== 'object' || 
						Object.keys(cultureData.common_names).length === 0) {
						return null;
				}
				
				var patterns = [];
				var commonNames = cultureData.common_names;
				
				for (var hipId in commonNames) {
						if (!commonNames.hasOwnProperty(hipId)) continue;
						
						var entries = commonNames[hipId];
						if (!entries || !Array.isArray(entries) || entries.length === 0) continue;
						
						var displayNames = [];
						var primaryName = null;
						var primaryNativeName = null;
						
						// Process the ID - handle "NAME" prefix for solar system objects
						var searchId = hipId;
						var originalId = hipId;
						var isSolarSystemObject = false;
						
						// Check if this is a solar system object (NAME prefix)
						if (hipId.indexOf('NAME ') === 0) {
								isSolarSystemObject = true;
								// Extract the actual object name (e.g., "NAME Mars" -> "Mars")
								searchId = hipId.substring(5).trim();
								console.log("[SkyCulture] Solar system object detected:", hipId, "-> search ID:", searchId);
						}
						
						// Collect all valid names for this star/object
						for (var i = 0; i < entries.length; i++) {
								var entry = entries[i];
								
								// Collect English names (skip codes like ARA_01)
								if (entry.english && entry.english.trim()) {
										var name = entry.english.trim();
										if (!name.match(/^[A-Z]+_[0-9]+$/)) {
												displayNames.push({ type: 'english', value: name });
												if (primaryName === null) primaryName = name;
										}
								}
								
								// Collect native names as fallback
								if (entry.native && entry.native.trim()) {
										var nativeName = entry.native.trim();
										if (primaryNativeName === null) primaryNativeName = nativeName;
										var exists = displayNames.some(function(dn) { 
												return dn.value === nativeName; 
										});
										if (!exists) {
												displayNames.push({ type: 'native', value: nativeName });
										}
								}
						}
						
						// Determine display and search names
						var displayName = '';
						var finalSearchName = '';
						
						if (displayNames.length === 0) {
								displayName = searchId;
								finalSearchName = searchId;
						} else {
								displayName = displayNames[0].value;
								finalSearchName = primaryName || primaryNativeName || displayNames[0].value;
								
								// Combine multiple names for compact display
								if (displayNames.length === 2) {
										displayName = displayNames[0].value + " - " + displayNames[1].value;
								} else if (displayNames.length >= 3) {
										displayName = displayNames[0].value + " - " + displayNames[1].value;
										var additional = displayNames.slice(2).map(function(dn) { 
												return dn.value; 
										}).join(" / ");
										displayName += " (" + additional + ")";
								}
								
								// Truncate extremely long display names
								if (displayName.length > 140) {
										displayName = displayName.substring(0, 137) + "...";
								}
						}
						
						// For solar system objects, ensure search name is the actual object name
						if (isSolarSystemObject) {
								finalSearchName = searchId;
						}
						
						patterns.push({
								id: searchId,                    // Use cleaned ID for navigation (e.g., "Mars" not "NAME Mars")
								originalId: originalId,          // Keep original ID for reference (e.g., "NAME Mars")
								name: displayName,
								searchName: finalSearchName,
								type: "star",
								isSolarSystem: isSolarSystemObject,
								englishName: primaryName,
								nativeName: primaryNativeName
						});
				}
				
				if (patterns.length === 0) return null;
				
				// Sort alphabetically and limit to prevent UI lag
				patterns.sort(function(a, b) { return a.name.localeCompare(b.name); });
				var limitedPatterns = patterns.slice(0, 3200);
				
				console.log("[SkyCulture] Extracted " + limitedPatterns.length + " star names " +
						(patterns.length > 3200 ? "(limited from " + patterns.length + ")" : ""));
				
				return limitedPatterns;
		}

    // =====================================================================
    // 10. PANEL RENDERING FUNCTIONS
    // =====================================================================

    /**
     * Render the Constellations panel.
     * 
     * @param {Array} patterns - Constellation pattern objects
     * @param {boolean} disableHandler - If true, skip attaching the original click handler
     */
    function renderConstellationsPanel(patterns, disableHandler) {
        if (!patterns || !patterns.length) {
            if ($constellationsContainer) {
                $constellationsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No constellations available for this culture") + '</div>');
            }
            updatePatternCount("constellations", 0);
            currentPatternsData.constellations = [];
            return;
        }
        
        updatePatternCount("constellations", patterns.length);
        currentPatternsData.constellations = patterns;
        
        renderButtons($constellationsContainer, patterns, "constellations", 
            function(patternName, patternType, searchName, patternId) {
                // IMPORTANT: Pass BOTH name and original ID
                stelUtils.toggleConstellationHighlight(searchName || patternName, patternId);
                updateAllButtonStates();
            },
            disableHandler === true
        );
    }

    /**
     * Render the Asterisms panel.
     * 
     * @param {Array} patterns - Asterism pattern objects
     */
    function renderAsterismsPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($asterismsContainer) {
                $asterismsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No asterisms available for this culture") + '</div>');
            }
            updatePatternCount("asterisms", 0);
            currentPatternsData.asterisms = [];
            return;
        }
        
        updatePatternCount("asterisms", patterns.length);
        currentPatternsData.asterisms = patterns;
        
        renderButtons($asterismsContainer, patterns, "asterisms", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToAsterism(searchName || patternName, patternId);
                updateAllButtonStates();
            });
    }

		/**
		 * Render the Zodiac Signs panel.
		 * 
		 * IMPORTANT: This function passes the patternId (composite ID) to the
		 * navigation handler to ensure proper bidirectional synchronization.
		 * 
		 * @param {Array} patterns - Zodiac pattern objects with composite IDs
		 */
		function renderZodiacPanel(patterns) {
				if (!patterns || !patterns.length) {
						if ($zodiacContainer) {
								$zodiacContainer.html('<div class="loading-placeholder">' + 
										_tr("No zodiac signs available for this culture") + '</div>');
						}
						updatePatternCount("zodiac", 0);
						currentPatternsData.zodiac = [];
						return;
				}
				
				updatePatternCount("zodiac", patterns.length);
				currentPatternsData.zodiac = patterns;
				
				renderButtons($zodiacContainer, patterns, "zodiac", 
						function(patternName, patternType, searchName, patternId) {
								console.log("[SkyCulture] Zodiac button clicked:", patternName, "ID:", patternId);
								stelUtils.goToZodiacSign(searchName || patternName, patternId);
								updateAllButtonStates();
						});
		}

    /**
     * Render the Lunar Mansions panel.
     * 
     * @param {Array} patterns - Lunar mansion pattern objects
     */
    function renderLunarPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($lunarContainer) {
                $lunarContainer.html('<div class="loading-placeholder">' + 
                    _tr("No lunar mansions available for this culture") + '</div>');
            }
            updatePatternCount("lunar", 0);
            currentPatternsData.lunar = [];
            return;
        }
        
        updatePatternCount("lunar", patterns.length);
        currentPatternsData.lunar = patterns;
        
        renderButtons($lunarContainer, patterns, "lunar", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToLunarMansion(searchName || patternName, patternId);
                updateAllButtonStates();
            });
    }

    /**
     * Render the Notable Stars panel.
     * 
     * @param {Array} patterns - Star pattern objects
     */
    function renderStarsPanel(patterns) {
        if (!patterns || !patterns.length) {
            if ($starsContainer) {
                $starsContainer.html('<div class="loading-placeholder">' + 
                    _tr("No star names available for this culture") + '</div>');
            }
            updatePatternCount("stars", 0);
            currentPatternsData.stars = [];
            return;
        }
        
        updatePatternCount("stars", patterns.length);
        currentPatternsData.stars = patterns;
        
        renderButtons($starsContainer, patterns, "stars", 
            function(patternName, patternType, searchName, patternId) {
                stelUtils.goToObject(patternId, DEFAULT_FOV.star, 'star', currentCultureRawData);
                updateAllButtonStates();
            });
    }

    // =====================================================================
    // 11. MAIN DATA LOADING FUNCTION
    // =====================================================================

    /**
     * Load all culture data and render all panels.
     * 
     * This is the main orchestration function that:
     * 1. Updates the current language
     * 2. Shows loading placeholders in all panels
     * 3. Fetches index.json (single source of truth)
     * 4. Extracts and renders all data types
     * 
     * @param {Function} callback - Optional callback when loading completes
     */
    function loadAllCultureData(callback) {
        if (!currentCultureId) { 
            if (callback) callback(false); 
            return; 
        }
        
        // Update current language
        getCurrentLanguage();
        
        // Show loading states in all panels
        if ($constellationsContainer) {
            $constellationsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading constellations...") + '</div>');
        }
        if ($asterismsContainer) {
            $asterismsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading asterisms...") + '</div>');
        }
        if ($zodiacContainer) {
            $zodiacContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading zodiac signs...") + '</div>');
        }
        if ($lunarContainer) {
            $lunarContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading lunar mansions...") + '</div>');
        }
        if ($starsContainer) {
            $starsContainer.html('<div class="loading-placeholder">' + 
                _tr("Loading star names...") + '</div>');
        }
        
        // Reset counters and clear active selections
        updatePatternCount("constellations", 0);
        updatePatternCount("asterisms", 0);
        updatePatternCount("zodiac", 0);
        updatePatternCount("lunar", 0);
        updatePatternCount("stars", 0);
        clearAllActiveButtons();
        
        // Fetch index.json - the SINGLE SOURCE OF TRUTH
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                currentCultureRawData = data;
                
                console.log("[SkyCulture] Successfully loaded index.json for culture:", currentCultureId);
                
                // Extract all data types directly from index.json
                var constellations = loadConstellationsFromIndexJson(data);
                var asterisms = loadAsterismsFromIndexJson(data);
                var zodiac = extractZodiac(data);
                var lunar = extractLunarMansions(data);
                var stars = extractStarNames(data);
                
                // Render all panels
                renderConstellationsPanel(constellations);
                renderAsterismsPanel(asterisms);
                renderZodiacPanel(zodiac);
                renderLunarPanel(lunar);
                renderStarsPanel(stars);
								
                console.log("[SkyCulture] All panels rendered for culture:", currentCultureId);
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.error("[SkyCulture] Failed to load index.json:", errorThrown);
                
                // Show error states in all panels
                renderConstellationsPanel([]);
                renderAsterismsPanel([]);
                renderZodiacPanel(null);
                renderLunarPanel(null);
                renderStarsPanel(null);
                
                if (callback) callback(false);
            }
        });
    }

    // =====================================================================
    // 12. SKY CULTURE MANAGEMENT FUNCTIONS
    // =====================================================================

    /**
     * Update the info iframe with the current culture description.
     */
    function updateInfoFrame() {
        if ($infoFrame && $infoFrame.length) {
            $infoFrame.attr("src", "/api/view/skyculturedescription/");
        }
    }

    /**
     * Update culture button active states based on currentCultureId.
     */
    function updateCultureButtonStates() {
        if (!$cultureContainer) return;
        $cultureContainer.find(".skyculture-btn").removeClass("active");
        $cultureContainer.find(".skyculture-btn").each(function() {
            if ($(this).data("culture-id") === currentCultureId) {
                $(this).addClass("active");
            }
        });
    }

    /**
     * Render culture selection buttons.
     * Names are automatically translated by the server.
     * 
     * @param {Array} cultures - Available cultures with translated names
     * @param {string} activeCulture - Currently active culture ID
     */
    function renderCultureButtons(cultures, activeCulture) {
        if (!$cultureContainer) return;
        $cultureContainer.empty();
        
        if (!cultures || !cultures.length) {
            $cultureContainer.html('<div class="loading-placeholder">' + 
                _tr("No sky cultures available") + '</div>');
            return;
        }
        
        if ($cultureCount && $cultureCount.length) {
            $cultureCount.text('(' + cultures.length + ')');
        }
        
        var buttonsHtml = '<div class="skyculture-buttons-grid">';
        for (var i = 0; i < cultures.length; i++) {
            var culture = cultures[i];
            var isActive = (culture.id === activeCulture);
            // culture.name is already translated by the server
            buttonsHtml += '<button type="button" class="skyculture-btn' + 
                (isActive ? ' active' : '') + 
                '" data-culture-id="' + escapeHtml(culture.id) + '">' + 
                escapeHtml(culture.name) + '</button>';
        }
        buttonsHtml += '</div>';
        $cultureContainer.html(buttonsHtml);
        
        $cultureContainer.find(".skyculture-btn").on("click", function() {
            var cultureId = $(this).data("culture-id");
            if (cultureId && cultureId !== currentCultureId) {
                setSkyCulture(cultureId);
            }
        });
    }

    /**
     * Load available sky cultures from server.
     * Names are automatically translated based on Stellarium's current language.
     * 
     * @param {Function} callback - Optional callback when loading completes
     */
    function loadSkyCultures(callback) {
        $.ajax({
            url: '/api/view/listskyculture',
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                var culturesArray = [];
                for (var id in data) {
                    if (data.hasOwnProperty(id)) {
                        // data[id] contains the translated name
                        culturesArray.push({ id: id, name: data[id] });
                    }
                }
                culturesArray.sort(function(a, b) { 
                    return a.name.localeCompare(b.name); 
                });
                availableCultures = culturesArray;
                
                console.log("[SkyCulture] Loaded " + availableCultures.length + 
                    " sky cultures with translated names");
                
                if (propApi) {
                    currentCultureId = propApi.getStelProp("StelSkyCultureMgr.currentSkyCultureID") || 
                                      (availableCultures.length > 0 ? availableCultures[0].id : null);
                								
								    var culture = availableCultures.find(function(c) { return c.id === currentCultureId; });
                    currentCultureName = culture ? culture.name : currentCultureId;
                }
        
                
                renderCultureButtons(availableCultures, currentCultureId);
                updateInfoFrame();
                loadAllCultureData();
                
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.error("[SkyCulture] Failed to load sky cultures:", errorThrown);
                if ($cultureContainer) {
                    $cultureContainer.html('<div class="loading-placeholder">' + 
                        _tr("Error loading sky cultures") + '</div>');
                }
                if (callback) callback(false);
            }
        });
    }

    /**
     * Set the current sky culture.
     * 
     * @param {string} cultureId - Culture ID to set (e.g., "korean", "arabic_al-sufi")
     */
    function setSkyCulture(cultureId) {
        if (!cultureId || cultureId === currentCultureId) return;
        
        console.log("[SkyCulture] Setting sky culture to: " + cultureId);
        
        if (propApi && typeof propApi.setStelProp === 'function') {
            propApi.setStelProp("StelSkyCultureMgr.currentSkyCultureID", cultureId);
        }
        
        currentCultureId = cultureId;
				
				var culture = availableCultures.find(function(c) { return c.id === cultureId; });
        currentCultureName = culture ? culture.name : cultureId;
        
				updateCultureButtonStates();
        updateInfoFrame();
        
        selectedPattern = null;
        selectedPatternId = null;
        
        loadAllCultureData();
    }

    /**
     * Refresh all data by reloading cultures.
     */
    function refresh() {
        loadSkyCultures();
    }

    // =====================================================================
    // 13. SYNCHRONIZATION LISTENERS
    // =====================================================================

		/**
		 * Setup synchronization listeners for culture and selection changes.
		 * Ensures the UI stays in sync with Stellarium's state.
		 * 
		 * Enhanced matching logic for zodiac signs:
		 * - Exact match by composite ID
		 * - Numeric index extraction from composite IDs
		 * - Name-based matching as fallback
		 * - Support for both new composite IDs and legacy simple IDs
		 */
		function setupSyncListeners() {
				// Listen to objectSelected events from tables and other components
				$(document).on("objectSelected", function(evt, data) {
						if (!syncEnabled) return;
						
						if (!data || !data.type || data.type === 'none' || !data.name) {
								clearAllActiveButtons();
								return;
						}
						
						var found = false;
						var patternGroups = [
								{ container: $constellationsContainer, patterns: currentPatternsData.constellations, type: "constellation" },
								{ container: $asterismsContainer, patterns: currentPatternsData.asterisms, type: "asterism" },
								{ container: $zodiacContainer, patterns: currentPatternsData.zodiac, type: "zodiac" },
								{ container: $lunarContainer, patterns: currentPatternsData.lunar, type: "lunar" },
								{ container: $starsContainer, patterns: currentPatternsData.stars, type: "star" }
						];
						
						for (var g = 0; g < patternGroups.length; g++) {
								var group = patternGroups[g];
								var $container = group.container;
								var patterns = group.patterns;
								if (!$container || !$container.length || !patterns) continue;
								
								for (var i = 0; i < patterns.length; i++) {
										var pattern = patterns[i];
										var match = false;
										
										// ============================================================
										// ZODIAC MATCHING LOGIC
										// ============================================================
										if (group.type === "zodiac") {
												// METHOD 1: Match by composite ID (most reliable)
												if (data.id && pattern.id && data.id === pattern.id) {
														match = true;
												}
												// METHOD 2: Match by numeric index from composite ID
												else if (data.id && pattern.id && 
																 data.id.indexOf('zodiac_') === 0 && 
																 pattern.id.indexOf('zodiac_') === 0) {
														var dataParts = data.id.split('_');
														var patternParts = pattern.id.split('_');
														var dataIndex = parseInt(dataParts[dataParts.length - 1], 10);
														var patternIndex = parseInt(patternParts[patternParts.length - 1], 10);
														if (dataIndex === patternIndex) {
																match = true;
														}
												}
												// METHOD 3: Match by simple ID (legacy support)
												else if (data.id && pattern.simpleId && data.id === pattern.simpleId) {
														match = true;
												}
												// METHOD 4: Match by name (fallback)
												else if (data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name ||
																		 (pattern.englishName && pattern.englishName === data.name));
												}
										}
										// ============================================================
										// LUNAR MANSION MATCHING LOGIC
										// ============================================================
										else if (group.type === "lunar") {
												// Exact match
												if (data.id && pattern.id && data.id === pattern.id) {
														match = true;
												}
												// Match by numeric index
												else if (data.id && pattern.id && 
																 data.id.indexOf('lunar_') === 0 && 
																 pattern.id.indexOf('lunar_') === 0) {
														var dataNum = parseInt(data.id.split('_')[1], 10);
														var patternNum = parseInt(pattern.id.split('_')[1], 10);
														if (dataNum === patternNum) {
																match = true;
														}
												}
												// Match by name
												else if (data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name);
												}
										}
										// ============================================================
										// CONSTELLATION MATCHING LOGIC
										// ============================================================
										else if (group.type === "constellation") {
												// Match by original ID
												if (data.id && pattern.id) {
														if (pattern.id === data.id) {
																match = true;
														}
														// Match by abbreviation (last part of CON ID)
														else if (pattern.id.indexOf('CON ') === 0 && 
																		 data.id.indexOf('CON ') === 0) {
																var patternParts = pattern.id.split(' ');
																var dataParts = data.id.split(' ');
																if (patternParts.length >= 3 && dataParts.length >= 3) {
																		if (patternParts[2] === dataParts[2]) {
																				match = true;
																		}
																}
														}
												}
												// Match by name
												if (!match && data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name ||
																		 (pattern.englishName && pattern.englishName === data.name));
												}
										}
										// ============================================================
										// ASTERISM MATCHING LOGIC
										// ============================================================
										else if (group.type === "asterism") {
												if (data.id && pattern.id) {
														if (pattern.id === data.id) {
																match = true;
														}
														// Match by abbreviation (last part of AST ID)
														else if (pattern.id.indexOf('AST ') === 0 && 
																		 data.id.indexOf('AST ') === 0) {
																var patternParts = pattern.id.split(' ');
																var dataParts = data.id.split(' ');
																if (patternParts.length >= 3 && dataParts.length >= 3) {
																		if (patternParts[2] === dataParts[2]) {
																				match = true;
																		}
																}
														}
												}
												if (!match && data.name) {
														match = (pattern.name === data.name || 
																		 pattern.searchName === data.name);
												}
										}
										// ============================================================
										// STAR MATCHING LOGIC
										// ============================================================
										else if (group.type === "star") {
												if (data.id && pattern.id && pattern.id === data.id) {
														match = true;
												}
												else if (data.name && pattern.name === data.name) {
														match = true;
												}
												else if (data.name && pattern.searchName === data.name) {
														match = true;
												}
										}
										
										if (match) {
												$container.find(".pattern-btn").removeClass("active");
												$container.find(".pattern-btn").each(function() {
														var $btn = $(this);
														var btnPatternId = $btn.data("pattern-id");
														// Exact match only for button activation
														if (btnPatternId === pattern.id || 
																(pattern.simpleId && btnPatternId === pattern.simpleId)) {
																$btn.addClass("active");
																selectedPattern = pattern.name;
																selectedPatternId = pattern.id;
																found = true;
																console.log("[SkyCulture] ObjectSelected matched:", pattern.name, "ID:", pattern.id);
														}
												});
												break;
										}
								}
								if (found) break;
						}
						
						if (!found) clearAllActiveButtons();
				});				
    
				// ============================================================
				// LISTEN TO CULTURE CHANGES FROM SERVER (NEW)
				// ============================================================
				if (propApi) {
						$(propApi).on("stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID", 
								function(evt, data) {
										var newCultureId = data.value;
										if (newCultureId && newCultureId !== currentCultureId) {
												console.log("[SkyCulture] Culture changed from server to: " + newCultureId);
												currentCultureId = newCultureId;
												// Find the culture name from availableCultures
												var culture = availableCultures.find(function(c) { return c.id === newCultureId; });
												if (culture) currentCultureName = culture.name;
												updateCultureButtonStates();
												updateInfoFrame();
												selectedPattern = null;
												selectedPatternId = null;
												loadAllCultureData();
										}
								});
						
						// Listen to language changes
						$(propApi).on("stelPropertyChanged:StelLocaleMgr.appLanguage", 
								function(evt, data) {
										var newLang = data.value;
										if (newLang && newLang !== currentLanguage) {
												console.log("[SkyCulture] Language changed to: " + newLang);
												currentLanguage = newLang;
												// Reload cultures to get newly translated names
												refresh();
										}
								});
				}
		}

    // =====================================================================
    // 14. INITIALIZATION
    // =====================================================================

    /**
     * Initialize the sky culture module.
     * 
     * @param {string} cultureContainerSelector - Selector for culture buttons container
     * @param {Object} patternsContainers - Selectors for pattern containers
     * @param {string} iframeSelector - Selector for info iframe
     * @param {Object} options - Additional options
     */
    function init(cultureContainerSelector, patternsContainers, iframeSelector, options) {
        if (isInitialized) {
            console.warn("[SkyCulture] Module already initialized");
            return;
        }
        
        $cultureContainer = $(cultureContainerSelector || "#skyculture-buttons-container");
        options = options || {};
        $cultureCount = $(options.cultureCount || "#skyculture-culture-count");
        
        if (patternsContainers) {
            $constellationsContainer = $(patternsContainers.constellations || 
                "#constellations-buttons-container");
            $asterismsContainer = $(patternsContainers.asterisms || 
                "#asterisms-buttons-container");
            $zodiacContainer = $(patternsContainers.zodiac || 
                "#zodiac-buttons-container");
            $lunarContainer = $(patternsContainers.lunar || 
                "#lunar-buttons-container");
            $starsContainer = $(patternsContainers.stars || 
                "#stars-buttons-container");
        }
        
        $infoFrame = $(iframeSelector || "#vo_skycultureinfo");
        
        if (!$cultureContainer || !$cultureContainer.length) {
            console.error("[SkyCulture] Culture container not found");
            return;
        }
        
        // Get initial language
        getCurrentLanguage();
        
        setupSyncListeners();
        
        if ($("#skyculture-patterns-tabs").length) {
            $("#skyculture-patterns-tabs").tabs({ heightStyle: "content" });
        }
        
        loadSkyCultures(function(success) { 
            if (success) {
                isInitialized = true;
                console.log("[SkyCulture] Module initialized successfully v4.0.0 - Direct index.json loading");
            }
        });
				

    }

    // =====================================================================
    // 15. PUBLIC API
    // =====================================================================

    return {
        // Initialization
        init: init,
        refresh: refresh,
        
        // Culture management
        setCulture: setSkyCulture,
        getCurrentCulture: function() { return currentCultureId; },
        getAvailableCultures: function() { return availableCultures.slice(); },
        
        // Pattern selection
        getSelectedPattern: function() { return selectedPattern; },
        getSelectedPatternId: function() { return selectedPatternId; },
        
        // Data refresh
        refreshData: function() { loadAllCultureData(); },
        
        // Synchronization control
        isSyncEnabled: function() { return syncEnabled; },
        setSyncEnabled: function(enabled) { syncEnabled = enabled; },
        
        // Language
        getCurrentLanguage: getCurrentLanguage
    };
});