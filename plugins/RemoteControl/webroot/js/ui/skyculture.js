/* ========================================================================
 * skyculture.js - Sky Culture Manager with Multi-Panel Data Visualization
 * ========================================================================
 * 
 * This module provides a complete UI for managing sky cultures and their
 * associated astronomical data in Stellarium Remote Control.
 * 
 * Features:
 * 
 * 1. Sky Culture Buttons - Replaces the traditional select dropdown with a
 *    responsive button grid for selecting different sky cultures.
 *    - Uses /api/view/listskyculture endpoint (official Stellarium API)
 *    - Real-time synchronization with server-side changes
 *    - Visual feedback for active culture
 * 
 * 2. Dynamic Data Panels - Automatically displays available data types:
 *    - Constellations: Traditional star groupings with isolation/highlight
 *    - Asterisms: Star patterns within constellations (non-IAU)
 *    - Zodiac Signs: The 12 ecliptic signs (when defined in culture)
 *    - Lunar Mansions: Traditional lunar stations (when defined)
 *    - Notable Stars: Named stars with cultural significance
 * 
 * 3. Smart Panel Management:
 *    - Panels appear only when data is available in index.json
 *    - Count badges show number of items per category
 *    - Empty panels are automatically hidden
 * 
 * Technical Architecture:
 * - Uses Stellarium's official REST API for all data access
 * - Property system for real-time synchronization
 * - Event-driven architecture with jQuery custom events
 * - Responsive grid layout for all screen sizes
 * - Shared utilities module for navigation and isolation
 * 
 * API Endpoints Used:
 * - GET  /api/view/listskyculture                    - List all sky cultures
 * - GET  /api/view/skyculturedescription/            - Get culture description (iframe)
 * - GET  /api/view/skyculturedescription/index.json  - Get culture-specific data
 * - POST /api/stelproperty/set                       - Set current sky culture
 * 
 * Data Extraction:
 * - Constellations: from data.constellations array
 * - Asterisms: from data.asterisms array (filters out is_ray_helper)
 * - Zodiac: from data.zodiac.names array
 * - Lunar Mansions: from data.lunar_system.names array
 * - Star Names: from data.common_names object
 * 
 * Dependencies:
 * - jQuery 3.x
 * - Stellarium API modules (properties, remotecontrol, actions)
 * - stellarium-utils (shared utilities for navigation and isolation)
 * 
 * @module skyculture
 * @requires jquery
 * @requires api/properties
 * @requires api/remotecontrol
 * @requires api/actions
 * @requires stellarium-utils
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-08
 * @license GPLv2+
 * @version 2.2.0
 * 
 * ======================================================================== */

define(["jquery", "api/properties", "api/remotecontrol", "api/actions", "ui/stellarium-utils"], 
    function($, propApi, rc, actions, stelUtils) {
    "use strict";

    // =====================================================================
    // SECTION 1: CONSTANTS AND CONFIGURATION
    // =====================================================================

    /**
     * Default IAU Constellations (88 standard constellations)
     * Used as fallback when a sky culture doesn't have an index.json file
     * Sorted alphabetically for consistent display
     * @constant {Array<string>}
     */
    var DEFAULT_IAU_CONSTELLATIONS = [
        "Andromeda", "Antlia", "Apus", "Aquarius", "Aquila", "Ara", "Aries", "Auriga",
        "Bootes", "Caelum", "Camelopardalis", "Cancer", "Canes Venatici", "Canis Major",
        "Canis Minor", "Capricornus", "Carina", "Cassiopeia", "Centaurus", "Cepheus",
        "Cetus", "Chamaeleon", "Circinus", "Columba", "Coma Berenices", "Corona Australis",
        "Corona Borealis", "Corvus", "Crater", "Crux", "Cygnus", "Delphinus", "Dorado",
        "Draco", "Equuleus", "Eridanus", "Fornax", "Gemini", "Grus", "Hercules",
        "Horologium", "Hydra", "Hydrus", "Indus", "Lacerta", "Leo", "Leo Minor", "Lepus",
        "Libra", "Lupus", "Lynx", "Lyra", "Mensa", "Microscopium", "Monoceros", "Musca",
        "Norma", "Octans", "Ophiuchus", "Orion", "Pavo", "Pegasus", "Perseus", "Phoenix",
        "Pictor", "Pisces", "Piscis Austrinus", "Puppis", "Pyxis", "Reticulum", "Sagitta",
        "Sagittarius", "Scorpius", "Sculptor", "Scutum", "Serpens", "Sextans", "Taurus",
        "Telescopium", "Triangulum", "Triangulum Australe", "Tucana", "Ursa Major",
        "Ursa Minor", "Vela", "Virgo", "Volans", "Vulpecula"
    ];

    /**
     * Cache for parsed culture data to avoid redundant API calls
     * Stores the full extracted data object for each culture
     * @constant {Object.<string, Object>}
     */
    var CULTURE_DATA_CACHE = {};

    // =====================================================================
    // SECTION 2: PRIVATE VARIABLES
    // =====================================================================

    // DOM Elements
    var $cultureContainer = null;           // Container for sky culture buttons
    var $constellationsContainer = null;    // Container for constellation buttons
    var $asterismsContainer = null;         // Container for asterism buttons
    var $zodiacContainer = null;            // Container for zodiac sign buttons
    var $lunarContainer = null;             // Container for lunar mansion buttons
    var $starsContainer = null;             // Container for star name buttons
    var $infoFrame = null;                  // Iframe for culture description

    // State Variables
    var currentCultureId = null;            // Currently selected culture ID
    var availableCultures = [];              // Array of {id, name} objects
    var selectedPattern = null;              // Currently selected pattern name
    var isInitialized = false;               // Module initialization flag
    var isUpdating = false;                  // Prevents concurrent updates

    // =====================================================================
    // SECTION 3: HELPER FUNCTIONS
    // =====================================================================

    /**
     * Translation helper with fallback support
     * @param {string} text - Text to translate
     * @param {...*} args - Optional arguments for string replacement
     * @returns {string} Translated text
     */
    function _tr(text) {
        if (typeof window.tr === 'function') {
            return window.tr.apply(window, arguments);
        }
        if (typeof rc !== 'undefined' && rc && typeof rc.tr === 'function') {
            return rc.tr.apply(rc, arguments);
        }
        if (arguments.length > 1) {
            var args = Array.prototype.slice.call(arguments, 1);
            return text.replace(/%(\d+)/g, function(match, num) {
                return typeof args[num] != 'undefined' ? args[num] : match;
            });
        }
        return text;
    }

    /**
     * Escape HTML special characters to prevent XSS
     * Handles &, <, >, ", and ' characters
     * @param {string} str - String to escape
     * @returns {string} Escaped string
     */
    function escapeHtml(str) {
        if (!str) return '';
        return str.replace(/[&<>"']/g, function(m) {
            if (m === '&') return '&amp;';
            if (m === '<') return '&lt;';
            if (m === '>') return '&gt;';
            if (m === '"') return '&quot;';
            if (m === "'") return '&#39;';
            return m;
        });
    }

    /**
     * Updates the count badge for a specific pattern type
     * @param {string} type - Pattern type: "constellations", "asterisms", "zodiac", "lunar", "stars"
     * @param {number} count - Number of items
     */
    function updatePatternCount(type, count) {
        var $countElement = $("#" + type + "-count");
        if ($countElement && $countElement.length) {
            if (count > 0) {
                $countElement.text("(" + count + ")");
            } else {
                $countElement.text("");
            }
        }
    }

    /**
     * Updates the visual state of all pattern buttons
     * Removes active class from all buttons and adds it to the currently selected pattern
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
                if (selectedPattern) {
                    container.find(".pattern-btn").each(function() {
                        if ($(this).data("pattern-name") === selectedPattern) {
                            $(this).addClass("active");
                        }
                    });
                }
            }
        }
    }

    /**
     * Renders a button grid for a pattern container
     * @param {jQuery} $container - jQuery object of the container element
     * @param {Array} patterns - Array of pattern objects {id, name, type}
     * @param {string} containerName - Name of the container for logging
     * @param {Function} onClickHandler - Callback function when button is clicked
     */
    function renderButtons($container, patterns, containerName, onClickHandler) {
        if (!$container || !$container.length) return;
        
        $container.empty();
        
        if (!patterns || patterns.length === 0) {
            var placeholderText = "";
            if (containerName === "constellations") placeholderText = _tr("No constellations available");
            else if (containerName === "asterisms") placeholderText = _tr("No asterisms available");
            else if (containerName === "zodiac") placeholderText = _tr("No zodiac data available");
            else if (containerName === "lunar") placeholderText = _tr("No lunar mansions data available");
            else if (containerName === "stars") placeholderText = _tr("No star name data available");
            else placeholderText = _tr("No data available");
            
            $container.html('<div class="loading-placeholder">' + placeholderText + '</div>');
            return;
        }
        
        var buttonsHtml = '<div class="patterns-buttons-grid">';
        
        for (var i = 0; i < patterns.length; i++) {
            var pattern = patterns[i];
            var displayName = pattern.name;
            var isActive = (selectedPattern === pattern.name);
            
            buttonsHtml += '<button type="button" class="pattern-btn' + 
                (isActive ? ' active' : '') + '" data-pattern-name="' + 
                escapeHtml(pattern.name) + '" data-pattern-type="' + 
                escapeHtml(pattern.type) + '" data-pattern-id="' + 
                escapeHtml(pattern.id) + '">' + escapeHtml(displayName) + '</button>';
        }
        
        buttonsHtml += '</div>';
        $container.html(buttonsHtml);
        
        // Attach click handlers
        $container.find(".pattern-btn").on("click", function() {
            var patternName = $(this).data("pattern-name");
            var patternType = $(this).data("pattern-type");
            if (patternName && onClickHandler) {
                onClickHandler(patternName, patternType);
            }
        });
    }

    // =====================================================================
    // SECTION 4: DATA EXTRACTION FUNCTIONS
    // =====================================================================

    /**
     * Extracts constellations from index.json data
     * @param {Object} data - Parsed index.json content
     * @returns {Array|null} Array of constellation pattern objects or null if not present
     */
    function extractConstellations(data) {
        if (!data.constellations || !Array.isArray(data.constellations) || data.constellations.length === 0) {
            return null;
        }
        
        var patterns = [];
        
        for (var i = 0; i < data.constellations.length; i++) {
            var cons = data.constellations[i];
            var consName = null;
            
            if (cons.common_name) {
                if (cons.common_name.english) {
                    consName = cons.common_name.english;
                } else if (cons.common_name.native) {
                    consName = cons.common_name.native;
                }
            }
            
            if (!consName && cons.id) {
                consName = cons.id;
            }
            
            if (consName) {
                patterns.push({
                    id: cons.id,
                    name: consName,
                    type: "constellation"
                });
            }
        }
        
        if (patterns.length === 0) return null;
        
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        return patterns;
    }

    /**
     * Extracts asterisms from index.json data (skip ray helpers)
     * @param {Object} data - Parsed index.json content
     * @returns {Array|null} Array of asterism pattern objects or null if not present
     */
    function extractAsterisms(data) {
        if (!data.asterisms || !Array.isArray(data.asterisms) || data.asterisms.length === 0) {
            return null;
        }
        
        var patterns = [];
        
        for (var i = 0; i < data.asterisms.length; i++) {
            var aster = data.asterisms[i];
            
            // Skip ray helpers (these are just visual guides, not real patterns)
            if (aster.is_ray_helper === true) continue;
            
            var asterName = null;
            
            if (aster.common_name) {
                if (aster.common_name.english) {
                    asterName = aster.common_name.english;
                } else if (aster.common_name.native) {
                    asterName = aster.common_name.native;
                }
            }
            
            if (asterName && asterName.length > 0) {
                patterns.push({
                    id: aster.id,
                    name: asterName,
                    type: "asterism"
                });
            }
        }
        
        if (patterns.length === 0) return null;
        
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        return patterns;
    }

    /**
     * Extracts zodiac signs from index.json data
     * @param {Object} data - Parsed index.json content
     * @returns {Array|null} Array of zodiac pattern objects or null if not present
     */
    function extractZodiac(data) {
        if (!data.zodiac || !data.zodiac.names || !Array.isArray(data.zodiac.names) || data.zodiac.names.length === 0) {
            return null;
        }
        
        var patterns = [];
        
        for (var i = 0; i < data.zodiac.names.length; i++) {
            var sign = data.zodiac.names[i];
            var signName = sign.english || sign.native || ("Sign " + (i + 1));
            
            patterns.push({
                id: "zodiac_" + (i + 1),
                name: signName,
                type: "zodiac"
            });
        }
        
        if (patterns.length === 0) return null;
        
        return patterns;
    }

    /**
     * Extracts lunar mansions from index.json data
     * @param {Object} data - Parsed index.json content
     * @returns {Array|null} Array of lunar mansion pattern objects or null if not present
     */
    function extractLunarMansions(data) {
        if (!data.lunar_system || !data.lunar_system.names || !Array.isArray(data.lunar_system.names) || data.lunar_system.names.length === 0) {
            return null;
        }
        
        var patterns = [];
        
        for (var i = 0; i < data.lunar_system.names.length; i++) {
            var mansion = data.lunar_system.names[i];
            var mansionName = mansion.english || mansion.native || ("Mansion " + (i + 1));
            
            patterns.push({
                id: "lunar_" + (i + 1),
                name: mansionName,
                type: "lunar"
            });
        }
        
        if (patterns.length === 0) return null;
        
        return patterns;
    }

		/**
		 * Extracts notable star names from index.json data
		 * Combines multiple English names for the same star into a single entry for display
		 * Uses only the first English name for search/centering
		 * 
		 * @param {Object} data - Parsed index.json content
		 * @returns {Array|null} Array of star pattern objects or null if not present
		 */
		function extractStarNames(data) {
				if (!data.common_names || typeof data.common_names !== 'object' || Object.keys(data.common_names).length === 0) {
						return null;
				}
				
				var patterns = [];
				
				for (var starId in data.common_names) {
						if (data.common_names.hasOwnProperty(starId)) {
								var entries = data.common_names[starId];
								
								if (!entries || !Array.isArray(entries) || entries.length === 0) continue;
								
								// Collect all English names for this star
								var englishNames = [];
								var primaryName = null;
								
								for (var i = 0; i < entries.length; i++) {
										var entry = entries[i];
										
										// Only use "english" field, ignore "native"
										if (entry.english && entry.english.trim()) {
												var name = entry.english.trim();
												englishNames.push(name);
												
												// First English name becomes the primary name (used for search)
												if (primaryName === null) {
														primaryName = name;
												}
										}
								}
								
								if (englishNames.length === 0) continue;
								
								// Build display name by combining all English names
								var displayName = englishNames[0];
								
								if (englishNames.length === 2) {
										// Two names: "Name1 - Name2"
										displayName = englishNames[0] + " - " + englishNames[1];
								} else if (englishNames.length >= 3) {
										// Three or more names: "Name1 - Name2 (Name3)"
										displayName = englishNames[0] + " - " + englishNames[1];
										for (var j = 2; j < englishNames.length; j++) {
												if (j === 2) {
														displayName += " (" + englishNames[j];
												} else {
														displayName += " / " + englishNames[j];
												}
										}
										displayName += ")";
								}
								
								// Limit display name length to avoid huge buttons
								if (displayName.length > 85) {
										displayName = displayName.substring(0, 80) + "...";
								}
								
								patterns.push({
										id: starId,
										name: displayName,           // For display on button
										searchName: primaryName,     // For actual search/centering
										type: "star"
								});
						}
				}
				
				if (patterns.length === 0) return null;
				
				// Sort by display name
				patterns.sort(function(a, b) {
						return a.name.localeCompare(b.name);
				});
				
				// Limit to first 200 star names
				patterns = patterns.slice(0, 4000);
				
				return patterns;
		}

    // =====================================================================
    // SECTION 5: PANEL RENDERING FUNCTIONS
    // =====================================================================

    /**
     * Renders the constellations panel
     * @param {Array|null} patterns - Array of patterns or null if not available
     */
    function renderConstellationsPanel(patterns) {
        var $panel = $constellationsContainer ? $constellationsContainer.closest('.skyculture-panel') : null;
        
        if (!patterns || patterns.length === 0) {
            if ($panel) $panel.hide();
            updatePatternCount("constellations", 0);
            return;
        }
        
        if ($panel) $panel.show();
        updatePatternCount("constellations", patterns.length);
        renderButtons($constellationsContainer, patterns, "constellations", function(patternName, patternType) {
            stelUtils.toggleConstellationHighlight(patternName);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }

    /**
     * Renders the asterisms panel
     * @param {Array|null} patterns - Array of patterns or null if not available
     */
    function renderAsterismsPanel(patterns) {
        var $panel = $asterismsContainer ? $asterismsContainer.closest('.asterisms-panel') : null;
        
        if (!patterns || patterns.length === 0) {
            if ($panel) $panel.hide();
            updatePatternCount("asterisms", 0);
            return;
        }
        
        if ($panel) $panel.show();
        updatePatternCount("asterisms", patterns.length);
        renderButtons($asterismsContainer, patterns, "asterisms", function(patternName, patternType) {
            stelUtils.goToAsterism(patternName);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }

    /**
     * Renders the zodiac panel
     * @param {Array|null} patterns - Array of patterns or null if not available
     */
    function renderZodiacPanel(patterns) {
        var $panel = $zodiacContainer ? $zodiacContainer.closest('.zodiac-panel') : null;
        
        if (!patterns || patterns.length === 0) {
            if ($panel) $panel.hide();
            updatePatternCount("zodiac", 0);
            return;
        }
        
        if ($panel) $panel.show();
        updatePatternCount("zodiac", patterns.length);
        renderButtons($zodiacContainer, patterns, "zodiac", function(patternName, patternType) {
            stelUtils.goToZodiacSign(patternName);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }

    /**
     * Renders the lunar mansions panel
     * @param {Array|null} patterns - Array of patterns or null if not available
     */
    function renderLunarPanel(patterns) {
        var $panel = $lunarContainer ? $lunarContainer.closest('.lunar-panel') : null;
        
        if (!patterns || patterns.length === 0) {
            if ($panel) $panel.hide();
            updatePatternCount("lunar", 0);
            return;
        }
        
        if ($panel) $panel.show();
        updatePatternCount("lunar", patterns.length);
        renderButtons($lunarContainer, patterns, "lunar", function(patternName, patternType) {
            stelUtils.goToLunarMansion(patternName);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }
	
		/**
		 * Renders the stars panel
		 * @param {Array|null} patterns - Array of patterns or null if not available
		 */
		function renderStarsPanel(patterns) {
				var $panel = $starsContainer ? $starsContainer.closest('.stars-panel') : null;
				
				if (!patterns || patterns.length === 0) {
						if ($panel) $panel.hide();
						updatePatternCount("stars", 0);
						return;
				}
				
				if ($panel) $panel.show();
				updatePatternCount("stars", patterns.length);
				renderButtons($starsContainer, patterns, "stars", function(patternName, patternType) {
						// Find the pattern object to get the searchName
						var pattern = null;
						for (var i = 0; i < patterns.length; i++) {
								if (patterns[i].name === patternName) {
										pattern = patterns[i];
										break;
								}
						}
						
						// Use searchName for centering, not the display name
						var searchTerm = pattern ? (pattern.searchName || patternName) : patternName;
						stelUtils.goToStar(searchTerm, 15);
						selectedPattern = patternName;
						updateAllButtonStates();
				});
		}

    /**
     * Processes culture data and renders all panels
     * @param {Object} data - Parsed index.json data
     */
    function processCultureData(data) {
        if (!data) return;
        
        // Extract all pattern types (returns null if not present)
        var constellations = extractConstellations(data);
        var asterisms = extractAsterisms(data);
        var zodiac = extractZodiac(data);
        var lunar = extractLunarMansions(data);
        var stars = extractStarNames(data);
        
        // Render each panel (null values will hide the panel)
        renderConstellationsPanel(constellations);
        renderAsterismsPanel(asterisms);
        renderZodiacPanel(zodiac);
        renderLunarPanel(lunar);
        renderStarsPanel(stars);
        
        console.log("[SkyCulture] Processed culture data:", {
            constellations: constellations ? constellations.length : 0,
            asterisms: asterisms ? asterisms.length : 0,
            zodiac: zodiac ? zodiac.length : 0,
            lunar: lunar ? lunar.length : 0,
            stars: stars ? stars.length : 0
        });
    }

    /**
     * Loads constellation/asterism data for the current sky culture
     * Uses the official API endpoint /api/view/skyculturedescription/index.json
     * Falls back to default IAU list if index.json doesn't exist or has no data
     * 
     * @param {function} callback - Optional callback function
     */
    function loadCultureData(callback) {
        if (!currentCultureId) {
            console.warn("[SkyCulture] No sky culture selected");
            if (callback) callback(false);
            return;
        }

        console.log("[SkyCulture] Loading data for culture: " + currentCultureId);
        
        // Check cache first
        if (CULTURE_DATA_CACHE[currentCultureId]) {
            console.log("[SkyCulture] Using cached data for " + currentCultureId);
            processCultureData(CULTURE_DATA_CACHE[currentCultureId]);
            if (callback) callback(true);
            return;
        }
        
        var indexPath = "/api/view/skyculturedescription/index.json";
        
        $.ajax({
            url: indexPath,
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                CULTURE_DATA_CACHE[currentCultureId] = data;
                processCultureData(data);
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.warn("[SkyCulture] Error loading index.json: " + errorThrown);
                useDefaultData();
                if (callback) callback(false);
            }
        });
    }
    
    /**
     * Uses the default IAU constellation list as fallback
     */
    function useDefaultData() {
        var defaultPatterns = [];
        
        for (var i = 0; i < DEFAULT_IAU_CONSTELLATIONS.length; i++) {
            defaultPatterns.push({
                id: DEFAULT_IAU_CONSTELLATIONS[i].toLowerCase(),
                name: DEFAULT_IAU_CONSTELLATIONS[i],
                type: "constellation"
            });
        }
        
        console.log("[SkyCulture] Using " + defaultPatterns.length + " default IAU constellations");
        
        // Render only constellations panel with default data
        renderConstellationsPanel(defaultPatterns);
        
        // Hide all other panels
        renderAsterismsPanel(null);
        renderZodiacPanel(null);
        renderLunarPanel(null);
        renderStarsPanel(null);
    }

    /**
     * Refreshes the culture data (called when sky culture changes)
     */
    function refreshCultureData() {
        if (isUpdating) return;
        isUpdating = true;
        
        selectedPattern = null;
        loadCultureData(function() {
            isUpdating = false;
        });
    }

    // =====================================================================
    // SECTION 6: SKY CULTURE MANAGEMENT
    // =====================================================================

    /**
     * Updates the iframe content with culture description
     * @param {string} cultureId - Culture identifier
     */
    function updateInfoFrame(cultureId) {
        if ($infoFrame && $infoFrame.length) {
            var url = "/api/view/skyculturedescription/";
            $infoFrame.attr("src", url);
        }
    }

    /**
     * Updates the active state of culture buttons
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
     * Renders the sky culture button grid
     * @param {Array} cultures - Array of {id, name} objects
     * @param {string} activeCulture - Currently active culture identifier
     */
    function renderCultureButtons(cultures, activeCulture) {
        if (!$cultureContainer) return;
        
        $cultureContainer.empty();
        
        if (!cultures || cultures.length === 0) {
            $cultureContainer.html('<div class="loading-placeholder">' + _tr("No sky cultures available") + '</div>');
            return;
        }
        
        var buttonsHtml = '<div class="skyculture-buttons-grid">';
        
        for (var i = 0; i < cultures.length; i++) {
            var culture = cultures[i];
            var isActive = (culture.id === activeCulture);
            
            buttonsHtml += '<button type="button" class="skyculture-btn' + 
                (isActive ? ' active' : '') + '" data-culture-id="' + 
                escapeHtml(culture.id) + '">' + escapeHtml(culture.name) + '</button>';
        }
        
        buttonsHtml += '</div>';
        $cultureContainer.html(buttonsHtml);
        
        // Attach click handlers
        $cultureContainer.find(".skyculture-btn").on("click", function() {
            var cultureId = $(this).data("culture-id");
            if (cultureId && cultureId !== currentCultureId) {
                setSkyCulture(cultureId);
            }
        });
    }

    /**
     * Loads available sky cultures from Stellarium
     * Uses official API endpoint /api/view/listskyculture
     * @param {function} callback - Optional callback function
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
                        culturesArray.push({
                            id: id,
                            name: data[id]
                        });
                    }
                }
                
                culturesArray.sort(function(a, b) {
                    return a.name.localeCompare(b.name);
                });
                
                availableCultures = culturesArray;
                console.log("[SkyCulture] Loaded " + availableCultures.length + " sky cultures");
                
                if (propApi) {
                    var current = propApi.getStelProp("StelSkyCultureMgr.currentSkyCultureID");
                    if (current && current !== currentCultureId) {
                        currentCultureId = current;
                    } else if (!currentCultureId && availableCultures.length > 0) {
                        currentCultureId = availableCultures[0].id;
                    }
                }
                
                renderCultureButtons(availableCultures, currentCultureId);
                updateInfoFrame(currentCultureId);
                loadCultureData();
                
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.error("[SkyCulture] Error loading sky cultures: " + errorThrown);
                if ($cultureContainer) {
                    $cultureContainer.html('<div class="loading-placeholder">' + _tr("Error loading sky cultures") + '</div>');
                }
                if (callback) callback(false);
            }
        });
    }

    /**
     * Sets the current sky culture
     * Uses Stellarium's property system for synchronization
     * @param {string} cultureId - Culture identifier to activate
     */
    function setSkyCulture(cultureId) {
        if (!cultureId || cultureId === currentCultureId) return;
        
        console.log("[SkyCulture] Setting sky culture to: " + cultureId);
        
        if (propApi && typeof propApi.setStelProp === 'function') {
            propApi.setStelProp("StelSkyCultureMgr.currentSkyCultureID", cultureId);
        }
        
        currentCultureId = cultureId;
        updateCultureButtonStates();
        updateInfoFrame(cultureId);
        
        // Clear selection and reload data
        selectedPattern = null;
        refreshCultureData();
    }

    /**
     * Refreshes the entire module (reloads from server)
     */
    function refresh() {
        // Clear cache to force fresh data
        for (var key in CULTURE_DATA_CACHE) {
            delete CULTURE_DATA_CACHE[key];
        }
        loadSkyCultures();
    }

    // =====================================================================
    // SECTION 7: EVENT HANDLERS
    // =====================================================================

    /**
     * Sets up property change listener for real-time synchronization
     * Listens for sky culture changes from Stellarium server
     */
    function setupPropertyListener() {
        if (!propApi) return;
        
        $(propApi).on("stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID", function(evt, data) {
            var newCultureId = data.value;
            if (newCultureId && newCultureId !== currentCultureId) {
                console.log("[SkyCulture] Property changed from server: " + newCultureId);
                currentCultureId = newCultureId;
                updateCultureButtonStates();
                updateInfoFrame(currentCultureId);
                selectedPattern = null;
                refreshCultureData();
            }
        });
    }

    // =====================================================================
    // SECTION 8: INITIALIZATION
    // =====================================================================

    /**
     * Initializes the Sky Culture Manager module
     * 
     * @param {string|jQuery} cultureContainerSelector - CSS selector for culture buttons container
     * @param {Object} patternsContainers - Object with selectors for all pattern containers
     *        Example: {
     *            constellations: "#constellations-buttons-container",
     *            asterisms: "#asterisms-buttons-container",
     *            zodiac: "#zodiac-buttons-container",
     *            lunar: "#lunar-buttons-container",
     *            stars: "#stars-buttons-container"
     *        }
     * @param {string|jQuery} iframeSelector - CSS selector for info iframe
     */
    function init(cultureContainerSelector, patternsContainers, iframeSelector) {
        if (isInitialized) {
            console.warn("[SkyCulture] Module already initialized");
            return;
        }
        
        // Initialize DOM references
        $cultureContainer = $(cultureContainerSelector || "#skyculture-buttons-container");
        
        if (patternsContainers) {
            $constellationsContainer = $(patternsContainers.constellations || "#constellations-buttons-container");
            $asterismsContainer = $(patternsContainers.asterisms || "#asterisms-buttons-container");
            $zodiacContainer = $(patternsContainers.zodiac || "#zodiac-buttons-container");
            $lunarContainer = $(patternsContainers.lunar || "#lunar-buttons-container");
            $starsContainer = $(patternsContainers.stars || "#stars-buttons-container");
        }
        
        $infoFrame = $(iframeSelector || "#vo_skycultureinfo");
        
        // Validate required containers
        if (!$cultureContainer || !$cultureContainer.length) {
            console.error("[SkyCulture] Culture container not found");
            return;
        }
        
        // Set up event listeners
        setupPropertyListener();
        
        // Load data and initialize
        loadSkyCultures(function(success) {
            if (success) {
                isInitialized = true;
                console.log("[SkyCulture] Module initialized successfully version 2.2.0");
                $(document).trigger("skyculture:ready", [currentCultureId, availableCultures]);
            } else {
                console.error("[SkyCulture] Failed to initialize");
            }
        });
    }

    // =====================================================================
    // SECTION 9: PUBLIC API
    // =====================================================================

    return {
        // Initialization
        init: init,
        refresh: refresh,
        
        // Culture management
        setCulture: setSkyCulture,
        getCurrentCulture: function() { return currentCultureId; },
        getAvailableCultures: function() { return availableCultures.slice(); },
        
        // Pattern management
        getSelectedPattern: function() { return selectedPattern; },
        refreshData: refreshCultureData
    };
});