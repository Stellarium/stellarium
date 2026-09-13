/* ========================================================================
 * skyculture-stats.js - Sky Culture Statistics and Data Tables Module
 * ========================================================================
 * 
 * This module provides a comprehensive data analysis and statistics view
 * for Stellarium sky cultures. It displays all culture data in organized
 * tables rather than buttons, allowing detailed inspection and export.
 * 
 * Features:
 * - Load cultures from /api/view/listskyculture with translated names
 * - Display culture-specific data from index.json
 * - Tables for: Constellations, Asterisms, Zodiac Signs, Lunar Mansions, Stars
 * - Each table row is clickable with toggle selection
 * - Search functionality within each table
 * - Export to CSV and JSON formats
 * - Statistics counters for all data types
 * - Synchronization with server for culture and language changes
 * - Translation column showing localized names using _tr()
 * 
 * Translation Support:
 * - Culture names are displayed translated (from API)
 * - English names are shown alongside native names
 * - Translation column uses _tr() for localized display
 * - Language changes trigger UI refresh
 * 
 * @module skyculture-stats
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/properties
 * @requires ui/stellarium-utils
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-20
 * @license GPLv2+
 * @version 3.2.0
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/properties", "ui/stellarium-utils"], 
    function($, rc, propApi, stelUtils) {
    "use strict";

    var _tr = rc.tr;
    
    // DOM Elements
    var $cultureContainer = null;
    var $cultureCount = null;
    var $totalCultures = null;
    var $totalConstellations = null;
    var $totalAsterisms = null;
		var $totalRayHelpers = null;
    var $totalZodiac = null;
    var $totalLunar = null;
    var $totalStars = null;
		
		var currentCultureRawData = null;
    
    var $constellationsBody = null;
    var $asterismsBody = null;
    var $zodiacBody = null;
    var $lunarBody = null;
    var $starsBody = null;
    
    var $constellationsCount = null;
    var $asterismsCount = null;
    var $zodiacCount = null;
    var $lunarCount = null;
    var $starsCount = null;
    
    var $descriptionFrame = null;
    
    // State
    var currentCultureId = null;
    var currentCultureName = null;
    var currentLanguage = 'en';
    var availableCultures = [];
    var currentCultureData = null;
    var isInitialized = false;
    
    var CULTURE_DATA_CACHE = {};
    var selectedRow = null;

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    /**
     * Parse ID type from string identifier with improved pattern matching.
     * Supports various catalog formats including:
     * - HIP, HIP with space
     * - Messier (M, M with space, M without space)
     * - NGC (with/without space, uppercase/lowercase)
     * - IC (with/without space, uppercase/lowercase)
     * - Gaia DR3 (long numeric strings, with/without prefix)
     * - DSO prefixes
     * - Solar system objects (NAME prefix)
     * - Constellation IDs (CON prefix)
     * - Asterism IDs (AST prefix)
     * 
     * @param {string} id - The ID string to parse
     * @returns {Object} Object with type, value, and display properties
     */
    function parseIdType(id) {
        if (typeof id !== 'string') {
            return { type: 'other', value: String(id), display: 'Other' };
        }
        
        var trimmedId = id.trim();
        
        // ============================================================
        // 1. MESSIER OBJECTS (M45, M 45, M 31, M31)
        // ============================================================
        // Match "M45" or "M 45" (with or without space)
        var messierPattern = /^M\s*(\d+)$/i;
        var messierMatch = trimmedId.match(messierPattern);
        if (messierMatch) {
            var messierNum = messierMatch[1];
            return { 
                type: 'messier', 
                value: messierNum, 
                display: 'M',
                fullId: 'M ' + messierNum
            };
        }
        
        // ============================================================
        // 2. NGC OBJECTS (NGC1750, NGC 1750, ngc1750, ngc 1750)
        // ============================================================
        var ngcPattern = /^NGC\s*(\d+)$/i;
        var ngcMatch = trimmedId.match(ngcPattern);
        if (ngcMatch) {
            var ngcNum = ngcMatch[1];
            return { 
                type: 'ngc', 
                value: ngcNum, 
                display: 'NGC',
                fullId: 'NGC ' + ngcNum
            };
        }
        
        // ============================================================
        // 3. IC OBJECTS (IC1750, IC 1750, ic1750, ic 1750)
        // ============================================================
        var icPattern = /^IC\s*(\d+)$/i;
        var icMatch = trimmedId.match(icPattern);
        if (icMatch) {
            var icNum = icMatch[1];
            return { 
                type: 'ic', 
                value: icNum, 
                display: 'IC',
                fullId: 'IC ' + icNum
            };
        }
        
        // ============================================================
        // 4. HIPPARCOS OBJECTS (HIP 12345, HIP12345, 12345)
        // ============================================================
        // Check for HIP prefix (with or without space)
        var hipPattern = /^HIP\s*(\d+)$/i;
        var hipMatch = trimmedId.match(hipPattern);
        if (hipMatch) {
            var hipNum = hipMatch[1];
            return { 
                type: 'hip', 
                value: hipNum, 
                display: 'HIP',
                fullId: 'HIP ' + hipNum
            };
        }
        
        // Check for bare numbers (interpret as HIP IDs, but only if not too long for Gaia)
        if (/^\d+$/.test(trimmedId)) {
            var numValue = parseInt(trimmedId, 10);
            // Gaia DR3 IDs are typically 16-19 digits, HIP IDs are 1-6 digits
            if (trimmedId.length >= 16) {
                // This is likely a Gaia DR3 ID
                return { 
                    type: 'gaia', 
                    value: trimmedId, 
                    display: 'Gaia DR3'
                };
            } else {
                // This is likely a HIP ID
                return { 
                    type: 'hip', 
                    value: trimmedId, 
                    display: 'HIP',
                    fullId: 'HIP ' + trimmedId
                };
            }
        }
        
        // ============================================================
        // 5. GAIA DR3 OBJECTS (with prefix)
        // ============================================================
        var gaiaPattern = /^Gaia\s*DR3\s*(\d+)$/i;
        var gaiaMatch = trimmedId.match(gaiaPattern);
        if (gaiaMatch) {
            var gaiaNum = gaiaMatch[1];
            return { 
                type: 'gaia', 
                value: gaiaNum, 
                display: 'Gaia DR3'
            };
        }
        
        // ============================================================
        // 6. DSO PREFIX (Deep Sky Objects)
        // ============================================================
        if (trimmedId.startsWith('DSO:')) {
            var dsoId = trimmedId.substring(4);
            return { 
                type: 'dso', 
                value: dsoId, 
                display: 'DSO'
            };
        }
        
        // ============================================================
        // 7. SOLAR SYSTEM OBJECTS (NAME prefix)
        // ============================================================
        if (trimmedId.startsWith('NAME ')) {
            var solarName = trimmedId.substring(5);
            return { 
                type: 'solar', 
                value: solarName, 
                display: 'Solar'
            };
        }
        
        // ============================================================
        // 8. CONSTELLATION IDs (CON prefix)
        // ============================================================
        if (trimmedId.startsWith('CON ')) {
            var conName = trimmedId.substring(4);
            return { 
                type: 'constellation', 
                value: conName, 
                display: 'CON'
            };
        }
        
        // ============================================================
        // 9. ASTERISM IDs (AST prefix)
        // ============================================================
        if (trimmedId.startsWith('AST ')) {
            var astName = trimmedId.substring(4);
            return { 
                type: 'asterism', 
                value: astName, 
                display: 'AST'
            };
        }
        
        // ============================================================
        // 10. OTHER CATALOGS (PGC, UGC, ESO, ACO, SH2, PK, PNG, CR, B, etc.)
        // ============================================================
        var otherCatalogs = [
            { pattern: /^PGC\s*(\d+)$/i, type: 'pgc', display: 'PGC' },
            { pattern: /^UGC\s*(\d+)$/i, type: 'ugc', display: 'UGC' },
            { pattern: /^ESO\s*(\d+[-]\d+)$/i, type: 'eso', display: 'ESO' },
            { pattern: /^ACO\s*(\d+)$/i, type: 'aco', display: 'ACO' },
            { pattern: /^SH2-(\d+)$/i, type: 'sh2', display: 'SH2' },
            { pattern: /^PK\s*(\d+[-]\d+\.\d)$/i, type: 'pk', display: 'PK' },
            { pattern: /^PN\s*G\s*(\d+\.\d+[-]\d+\.\d+)$/i, type: 'png', display: 'PNG' },
            { pattern: /^CR\s*(\d+)$/i, type: 'cr', display: 'CR' },
            { pattern: /^B\s*(\d+)$/i, type: 'b', display: 'B' },
            { pattern: /^C\s*(\d+)$/i, type: 'caldwell', display: 'C' },
            { pattern: /^Melotte\s*(\d+)$/i, type: 'melotte', display: 'Mel' },
            { pattern: /^Collinder\s*(\d+)$/i, type: 'collinder', display: 'Cr' }
        ];
        
        for (var i = 0; i < otherCatalogs.length; i++) {
            var catalog = otherCatalogs[i];
            var match = trimmedId.match(catalog.pattern);
            if (match) {
                return { 
                    type: catalog.type, 
                    value: match[1], 
                    display: catalog.display
                };
            }
        }
        
        // ============================================================
        // 11. FALLBACK: Unknown type
        // ============================================================
        return { type: 'other', value: trimmedId, display: 'Other' };
    }

    /**
     * Extract all unique IDs from lines array.
     * @param {Array} lines - Array of lines
     * @returns {Array} Array of unique ID strings
     */
    function extractIdsFromLines(lines) {
        var ids = [];
        if (!Array.isArray(lines)) return ids;
        var idSet = {};
        lines.forEach(function(line) {
            if (Array.isArray(line)) {
                line.forEach(function(id) { 
                    if (id && !idSet[id]) { 
                        idSet[id] = true; 
                        ids.push(String(id)); 
                    } 
                });
            } else if (line && !idSet[line]) { 
                idSet[line] = true; 
                ids.push(String(line)); 
            }
        });
        return ids;
    }

    /**
     * Escape HTML special characters.
     * @param {string} str - String to escape
     * @returns {string} Escaped string
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
     * Format IDs for display with type badges.
     * Now uses the enhanced parseIdType function for accurate classification.
     * 
     * @param {Array} ids - Array of ID strings
     * @returns {string} HTML string with formatted badges
     */
    function formatIdsDisplay(ids) {
        if (!ids || ids.length === 0) return '-';
        
        return ids.map(function(id) {
            var parsed = parseIdType(id);
            // Use fullId if available for better display (e.g., "M 45" instead of "M45")
            var displayValue = parsed.fullId || parsed.value;
            return '<span class="id-type-badge ' + parsed.type + '" title="' + escapeHtml(parsed.display) + '">' + 
                   escapeHtml(parsed.display) + '</span>' + escapeHtml(displayValue);
        }).join(', ');
    }

    /**
     * Update a counter display.
     * @param {jQuery} $element - jQuery element for counter
     * @param {number} count - Count to display
     */
    function updateCounter($element, count) {
        if ($element && $element.length) $element.text('(' + count + ')');
    }

    /**
     * Clear all row highlights across all tables.
     */
    function clearAllRowHighlights() {
        if (selectedRow) { 
            selectedRow.removeClass('selected-row'); 
            selectedRow = null; 
        }
        var tables = [$constellationsBody, $asterismsBody, $zodiacBody, $lunarBody, $starsBody];
        for (var i = 0; i < tables.length; i++) {
            if (tables[i] && tables[i].length) {
                tables[i].find('.selected-row').removeClass('selected-row');
            }
        }
    }

    /**
     * Get current Stellarium language.
     * @returns {string} Language code
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

    // =====================================================================
    // ROW TOGGLE LOGIC
    // =====================================================================

		/**
		 * Handle row toggle selection with proper bidirectional sync.
		 * 
		 * @param {jQuery} $row - The row element
		 * @param {string} patternType - Type of pattern (constellation, asterism, zodiac, lunar, star)
		 * @param {string} patternName - Display name (native name)
		 * @param {string} patternId - Pattern ID (composite ID for lunar like "lunar_16")
		 * @param {string} searchName - Search name (English name for API lookup)
		 */
		function handleRowToggle($row, patternType, patternName, patternId, searchName) {
				var isCurrentlySelected = $row.hasClass('selected-row');
				
				if (isCurrentlySelected) {
						// Deselect: Clear highlight and emit empty event
						$row.removeClass('selected-row');
						selectedRow = null;
						if (patternType === 'constellation') {
								stelUtils.clearConstellationHighlight();
						} else {
								stelUtils.clearSelection();
						}
						stelUtils.emitObjectSelected('', '', 'none');
						console.log("[SkyCultureStats] Deselected row:", patternType, patternName);
				} else {
						// Select: Clear previous selection and highlight new row
						clearAllRowHighlights();
						$row.addClass('selected-row');
						selectedRow = $row;
						
						console.log("[SkyCultureStats] Selected row:", patternType, patternName, "ID:", patternId);
						
						// ============================================================
						// FORWARD SYNC: Navigate in Stellarium based on pattern type
						// ============================================================
						if (patternType === 'constellation') {
								// patternId is like "CON chinese_manchu Borboho"
								stelUtils.toggleConstellationHighlight(searchName || patternName, patternId);
						} 
						else if (patternType === 'asterism') {
								// patternId is like "AST chinese_manchu 001"
								stelUtils.goToAsterism(searchName || patternName, patternId);
						} 
						else if (patternType === 'zodiac') {
								stelUtils.goToZodiacSign(searchName || patternName, patternId);
						} 
						else if (patternType === 'lunar') {
								// CRITICAL: For lunar mansions, pass BOTH searchName AND patternId
								// patternId is the composite ID (e.g., "lunar_16") which matches skyculture.js
								// This ensures proper bidirectional sync
								if (patternId && patternId.indexOf('lunar_') === 0) {
										console.log("[SkyCultureStats] Navigating to lunar mansion with ID:", patternId, "Name:", searchName);
										stelUtils.goToLunarMansion(searchName || patternName, patternId);
								} else {
										console.log("[SkyCultureStats] Navigating to lunar mansion with name only:", searchName);
										stelUtils.goToLunarMansion(searchName || patternName);
								}
						} 
						else if (patternType === 'star') {
								stelUtils.goToObject(patternId, 15, 'star', currentCultureRawData);
						}
				}
		}

    // =====================================================================
    // DATA LOADING
    // =====================================================================

    /**
     * Load available sky cultures from server.
     * Names are automatically translated.
     * @param {Function} callback - Optional callback
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
                        culturesArray.push({ id: id, name: data[id] });
                    }
                }
                culturesArray.sort(function(a, b) { 
                    return a.name.localeCompare(b.name); 
                });
                availableCultures = culturesArray;
                
                console.log("[SkyCultureStats] Loaded " + availableCultures.length + 
                    " sky cultures with translated names");
                
                if ($totalCultures) $totalCultures.text(availableCultures.length);
                
                if (propApi) {
                    currentCultureId = propApi.getStelProp("StelSkyCultureMgr.currentSkyCultureID") || 
                                      (availableCultures.length > 0 ? availableCultures[0].id : null);
                    if (currentCultureId) {
                        var culture = availableCultures.find(function(c) { 
                            return c.id === currentCultureId; 
                        });
                        currentCultureName = culture ? culture.name : currentCultureId;
                    }
                }
                
                renderCultureButtons();
                if (currentCultureId) loadCultureData(currentCultureId);
                if (callback) callback(true);
            },
            error: function() {
                if ($cultureContainer) {
                    $cultureContainer.html('<div class="loading-placeholder">' + 
                        _tr("Error loading sky cultures") + '</div>');
                }
                if (callback) callback(false);
            }
        });
    }

    /**
     * Load culture data from index.json.
     * @param {string} cultureId - Culture ID to load
     * @param {Function} callback - Optional callback
     */
    function loadCultureData(cultureId, callback) {
        if (!cultureId) {
            if (callback) callback();
            return;
        }
        
        if (CULTURE_DATA_CACHE[cultureId]) {
            currentCultureData = CULTURE_DATA_CACHE[cultureId];
						currentCultureRawData = currentCultureData;					
            displayCultureData(currentCultureData);
            updateTotalStats(currentCultureData);
            if (callback) callback();
            return;
        }
        
        showLoadingStates();
        
        $.ajax({
            url: '/api/view/skyculturedescription/index.json',
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                CULTURE_DATA_CACHE[cultureId] = data;
                currentCultureData = data;
								currentCultureRawData = data;
                updateTotalStats(data);
                displayCultureData(data);
                if (callback) callback();
            },
            error: function() {
                showErrorStates();
                if (callback) callback();
            }
        });
    }

    /**
     * Show loading placeholders in all tables.
     */
    function showLoadingStates() {
        var loadingHtml = '<tr><td colspan="10" class="loading-placeholder">' + 
                          _tr("Loading...") + '</td></tr>';
        if ($constellationsBody) $constellationsBody.html(loadingHtml);
        if ($asterismsBody) $asterismsBody.html(loadingHtml);
        if ($zodiacBody) $zodiacBody.html(loadingHtml);
        if ($lunarBody) $lunarBody.html(loadingHtml);
        if ($starsBody) $starsBody.html(loadingHtml);
        updateCounter($constellationsCount, 0);
        updateCounter($asterismsCount, 0);
        updateCounter($zodiacCount, 0);
        updateCounter($lunarCount, 0);
        updateCounter($starsCount, 0);
    }

    /**
     * Show error states in all tables.
     */
    function showErrorStates() {
        var errorHtml = '<tr><td colspan="10" class="loading-placeholder">' + 
                        _tr("Error loading data") + '</td></tr>';
        if ($constellationsBody) $constellationsBody.html(errorHtml);
        if ($asterismsBody) $asterismsBody.html(errorHtml);
        if ($zodiacBody) $zodiacBody.html(errorHtml);
        if ($lunarBody) $lunarBody.html(errorHtml);
        if ($starsBody) $starsBody.html(errorHtml);
    }
		
    /**
     * Update total statistics counters with comprehensive data.
     * @param {Object} data - Culture data from index.json
     */
    function updateTotalStats(data) {
        // Constellations
        var constellations = data.constellations || [];
        if ($totalConstellations) $totalConstellations.text(constellations.length);
        
        // Asterisms (all, including ray helpers)
        var allAsterisms = data.asterisms || [];
        var realAsterisms = allAsterisms.filter(function(a) { return !a.is_ray_helper; });
        var rayHelpers = allAsterisms.filter(function(a) { return a.is_ray_helper === true; });
        
        if ($totalAsterisms) $totalAsterisms.text(realAsterisms.length);
        if ($totalRayHelpers) $totalRayHelpers.text(rayHelpers.length);
        
        // Zodiac signs
        var zodiac = data.zodiac;
        var zodiacCount = (zodiac && zodiac.names) ? zodiac.names.length : 0;
        if ($totalZodiac) $totalZodiac.text(zodiacCount);
        
        // Lunar mansions
        var lunar = data.lunar_system;
        var lunarCount = (lunar && lunar.names) ? lunar.names.length : 0;
        if ($totalLunar) $totalLunar.text(lunarCount);
        
        // Named stars/objects
        var stars = data.common_names ? Object.keys(data.common_names).length : 0;
        if ($totalStars) $totalStars.text(stars);
        
        console.log("[SkyCultureStats] Statistics updated:", {
            constellations: constellations.length,
            asterisms: realAsterisms.length,
            rayHelpers: rayHelpers.length,
            zodiac: zodiacCount,
            lunar: lunarCount,
            stars: stars
        });
    }

    // =====================================================================
    // SYNCHRONIZATION
    // =====================================================================

    /**
     * Setup property listener to sync culture and language changes from server.
     */
    function setupSyncListener() {
        if (!propApi) return;
        
        // Listen to culture changes
        $(propApi).on("stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID", 
            function(evt, data) {
                var newCultureId = data.value;
                if (newCultureId && newCultureId !== currentCultureId) {
                    console.log("[SkyCultureStats] Culture changed from server to: " + newCultureId);
                    
                    currentCultureId = newCultureId;
                    var culture = availableCultures.find(function(c) { 
                        return c.id === newCultureId; 
                    });
                    if (culture) currentCultureName = culture.name;
                    
                    updateCultureButtonStates();
                    
                    loadCultureData(newCultureId, function() {
                        if (currentCultureData) {
                            updateTotalStats(currentCultureData);
                        }
                    });
                    
                    if ($descriptionFrame && $descriptionFrame.length) {
                        $descriptionFrame.attr('src', '/api/view/skyculturedescription/');
                    }
                }
            });
        
        // Listen to language changes
        $(propApi).on("stelPropertyChanged:StelLocaleMgr.appLanguage", 
            function(evt, data) {
                var newLang = data.value;
                if (newLang && newLang !== currentLanguage) {
                    console.log("[SkyCultureStats] Language changed to: " + newLang);
                    currentLanguage = newLang;
                    // Reload cultures to get newly translated names
                    refresh();
                }
            });
        
        // Listen to objectSelected events from tables and other components
        $(document).on("objectSelected", function(evt, data) {
            console.log("[SkyCultureStats] objectSelected event received:", data);
            
            if (!data || !data.type) {
                clearAllRowHighlights();
                return;
            }
            
            if (data.type === 'none') {
                clearAllRowHighlights();
                return;
            }
            
            // Clear existing highlights before applying new ones
            clearAllRowHighlights();
            
            // Build search terms for matching
            var searchIds = [];
            var searchNames = [];
            
            // Collect all possible identifiers from the event data
            if (data.id) searchIds.push(String(data.id));
            if (data.originalId) searchIds.push(String(data.originalId));
            if (data.name) searchNames.push(String(data.name));
            if (data.originalName) searchNames.push(String(data.originalName));
            
            // Also add the pattern-id if available
            if (data.patternId) searchIds.push(String(data.patternId));
            
            console.log("[SkyCultureStats] Searching for:", { type: data.type, ids: searchIds, names: searchNames });
            
            var matched = false;
            
            // ============================================================
            // CONSTELLATION TABLE MATCHING
            // ============================================================
            if (data.type === "constellation") {
                if ($constellationsBody && $constellationsBody.length) {
                    $constellationsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowId = $row.data('pattern-id');
                        var rowName = $row.data('pattern-name');
                        var searchName = $row.data('search-name');
                        
                        // Convert to strings for comparison
                        var rowIdStr = rowId ? String(rowId) : '';
                        var rowNameStr = rowName ? String(rowName) : '';
                        var searchNameStr = searchName ? String(searchName) : '';
                        
                        var rowMatched = false;
                        
                        // 1. Match by exact ID
                        for (var s = 0; s < searchIds.length; s++) {
                            var searchId = String(searchIds[s]);
                            if (!searchId) continue;
                            
                            if (rowIdStr === searchId) {
                                rowMatched = true;
                                console.log("[SkyCultureStats] Constellation matched by exact ID:", rowIdStr);
                                break;
                            }
                        }
                        
                        // 2. Match by partial ID (e.g., "Ara" inside "CON modern Ara")
                        if (!rowMatched) {
                            for (var s = 0; s < searchIds.length; s++) {
                                var searchId = String(searchIds[s]);
                                if (!searchId || searchId.length < 2) continue;
                                
                                if (rowIdStr.indexOf(searchId) !== -1) {
                                    rowMatched = true;
                                    console.log("[SkyCultureStats] Constellation matched by partial ID:", rowIdStr, "contains", searchId);
                                    break;
                                }
                                if (searchId.indexOf(rowIdStr) !== -1 && rowIdStr.length > 2) {
                                    rowMatched = true;
                                    console.log("[SkyCultureStats] Constellation matched by reverse partial ID:", searchId, "contains", rowIdStr);
                                    break;
                                }
                            }
                        }
                        
                        // 3. Match by name
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]);
                                if (!searchTerm) continue;
                                
                                if (rowNameStr === searchTerm || searchNameStr === searchTerm) {
                                    rowMatched = true;
                                    console.log("[SkyCultureStats] Constellation matched by name:", rowNameStr);
                                    break;
                                }
                            }
                        }
                        
                        // 4. Match by case-insensitive name (fallback)
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]).toLowerCase();
                                if (!searchTerm) continue;
                                
                                if (rowNameStr.toLowerCase() === searchTerm || 
                                    searchNameStr.toLowerCase() === searchTerm) {
                                    rowMatched = true;
                                    console.log("[SkyCultureStats] Constellation matched by case-insensitive name:", rowNameStr);
                                    break;
                                }
                            }
                        }
                        
                        if (rowMatched) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            matched = true;
                            console.log("[SkyCultureStats] ✓ Constellation row selected:", rowNameStr);
                            return false; // break each loop
                        }
                    });
                }
            }
            
            // ============================================================
            // ASTERISM TABLE MATCHING
            // ============================================================
            else if (data.type === "asterism") {
                if ($asterismsBody && $asterismsBody.length) {
                    $asterismsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowId = $row.data('pattern-id');
                        var rowName = $row.data('pattern-name');
                        var searchName = $row.data('search-name');
                        
                        var rowIdStr = rowId ? String(rowId) : '';
                        var rowNameStr = rowName ? String(rowName) : '';
                        var searchNameStr = searchName ? String(searchName) : '';
                        
                        var rowMatched = false;
                        
                        // Match by ID
                        for (var s = 0; s < searchIds.length; s++) {
                            var searchId = String(searchIds[s]);
                            if (!searchId) continue;
                            if (rowIdStr === searchId) {
                                rowMatched = true;
                                break;
                            }
                            if (rowIdStr.indexOf(searchId) !== -1 && searchId.length > 2) {
                                rowMatched = true;
                                break;
                            }
                        }
                        
                        // Match by name
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]);
                                if (!searchTerm) continue;
                                if (rowNameStr === searchTerm || searchNameStr === searchTerm) {
                                    rowMatched = true;
                                    break;
                                }
                            }
                        }
                        
                        if (rowMatched) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            matched = true;
                            console.log("[SkyCultureStats] ✓ Asterism row selected:", rowNameStr);
                            return false;
                        }
                    });
                }
            }
            
            // ============================================================
            // ZODIAC TABLE MATCHING
            // ============================================================
            else if (data.type === "zodiac") {
                if ($zodiacBody && $zodiacBody.length) {
                    $zodiacBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowName = $row.data('pattern-name');
                        var searchName = $row.data('search-name');
                        
                        var rowNameStr = rowName ? String(rowName) : '';
                        var searchNameStr = searchName ? String(searchName) : '';
                        
                        var rowMatched = false;
                        
                        for (var n = 0; n < searchNames.length; n++) {
                            var searchTerm = String(searchNames[n]);
                            if (!searchTerm) continue;
                            if (rowNameStr === searchTerm || searchNameStr === searchTerm) {
                                rowMatched = true;
                                break;
                            }
                        }
                        
                        // Case-insensitive fallback
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]).toLowerCase();
                                if (!searchTerm) continue;
                                if (rowNameStr.toLowerCase() === searchTerm || 
                                    searchNameStr.toLowerCase() === searchTerm) {
                                    rowMatched = true;
                                    break;
                                }
                            }
                        }
                        
                        if (rowMatched) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            matched = true;
                            console.log("[SkyCultureStats] ✓ Zodiac row selected:", rowNameStr);
                            return false;
                        }
                    });
                }
            }
            
						// ============================================================
						// LUNAR TABLE MATCHING - Enhanced for bidirectional sync
						// ============================================================
						else if (data.type === "lunar") {
								if ($lunarBody && $lunarBody.length) {
										var lunarMatched = false;
										
										// Debug logging
										console.log("[SkyCultureStats] Lunar matching - received data:", {
												id: data.id,
												name: data.name,
												originalId: data.originalId
										});
										
										$lunarBody.find('.clickable-row').each(function() {
												var $row = $(this);
												var rowLunarId = $row.data('lunar-id');        // "lunar_16"
												var rowName = $row.data('pattern-name');       // Native name (娄)
												var searchName = $row.data('search-name');     // English name (Bond)
												var rowIndex = $row.data('lunar-index');       // 16
												var englishName = $row.data('english-name');   // "Bond"
												
												var rowMatched = false;
												
												// ============================================================
												// METHOD 1: Match by composite ID (most reliable)
												// This matches "lunar_16" from skyculture.js buttons
												// ============================================================
												if (data.id && rowLunarId && data.id === rowLunarId) {
														rowMatched = true;
														console.log("[SkyCultureStats] ✓ Lunar matched by composite ID:", rowLunarId);
												}
												
												// ============================================================
												// METHOD 2: Match by extracted number from composite ID
												// If data.id is "lunar_16", extract "16" and compare with rowIndex
												// ============================================================
												if (!rowMatched && data.id && typeof data.id === 'string' && data.id.indexOf('lunar_') === 0) {
														var lunarNumber = parseInt(data.id.split('_')[1], 10);
														if (!isNaN(lunarNumber) && lunarNumber === rowIndex) {
																rowMatched = true;
																console.log("[SkyCultureStats] ✓ Lunar matched by extracted number:", lunarNumber);
														}
												}
												
												// ============================================================
												// METHOD 3: Match by exact name (English or Native)
												// ============================================================
												if (!rowMatched && data.name) {
														var searchTerm = String(data.name);
														if (searchName === searchTerm || englishName === searchTerm || rowName === searchTerm) {
																rowMatched = true;
																console.log("[SkyCultureStats] ✓ Lunar matched by exact name:", rowName);
														}
												}
												
												// ============================================================
												// METHOD 4: Match by data.originalId if available
												// ============================================================
												if (!rowMatched && data.originalId) {
														var originalId = String(data.originalId);
														if (rowLunarId === originalId || searchName === originalId || englishName === originalId) {
																rowMatched = true;
																console.log("[SkyCultureStats] ✓ Lunar matched by originalId:", originalId);
														}
												}
												
												// ============================================================
												// METHOD 5: Case-insensitive name match (final fallback)
												// ============================================================
												if (!rowMatched && data.name) {
														var searchTermLower = String(data.name).toLowerCase();
														if (searchName.toLowerCase() === searchTermLower || 
																englishName.toLowerCase() === searchTermLower ||
																rowName.toLowerCase() === searchTermLower) {
																rowMatched = true;
																console.log("[SkyCultureStats] ✓ Lunar matched by case-insensitive name:", rowName);
														}
												}
												
												if (rowMatched) {
														$row.addClass('selected-row');
														selectedRow = $row;
														lunarMatched = true;
														console.log("[SkyCultureStats] ✓ Lunar row selected and highlighted:", rowName);
														return false; // break the each loop
												}
										});
										
										if (lunarMatched) {
												matched = true;
										} else {
												console.log("[SkyCultureStats] No lunar match found for:", data.id, data.name);
										}
								}
						}
            
            // ============================================================
            // STARS TABLE MATCHING
            // ============================================================
            else if (data.type === "star") {
                if ($starsBody && $starsBody.length) {
                    $starsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowId = $row.data('pattern-id');
                        var rowName = $row.data('pattern-name');
                        
                        var rowIdStr = rowId ? String(rowId) : '';
                        var rowNameStr = rowName ? String(rowName) : '';
                        
                        var rowMatched = false;
                        
                        // Match by ID (HIP number)
                        for (var s = 0; s < searchIds.length; s++) {
                            var searchId = String(searchIds[s]);
                            if (!searchId) continue;
                            if (rowIdStr === searchId) {
                                rowMatched = true;
                                break;
                            }
                        }
                        
                        // Match by name
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]);
                                if (!searchTerm) continue;
                                if (rowNameStr === searchTerm) {
                                    rowMatched = true;
                                    break;
                                }
                            }
                        }
                        
                        // Case-insensitive fallback
                        if (!rowMatched) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]).toLowerCase();
                                if (!searchTerm) continue;
                                if (rowNameStr.toLowerCase() === searchTerm) {
                                    rowMatched = true;
                                    break;
                                }
                            }
                        }
                        
                        if (rowMatched) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            matched = true;
                            console.log("[SkyCultureStats] ✓ Star row selected:", rowNameStr);
                            return false;
                        }
                    });
                }
            }
            
            // ============================================================
            // FALLBACK: If no match found, try partial matching in all tables
            // ============================================================
            if (!matched && searchIds.length > 0) {
                console.log("[SkyCultureStats] No direct match, trying fallback matching...");
                
                var allTables = [
                    { body: $constellationsBody, name: 'constellations', idAttr: 'pattern-id', nameAttr: 'pattern-name' },
                    { body: $asterismsBody, name: 'asterisms', idAttr: 'pattern-id', nameAttr: 'pattern-name' },
                    { body: $zodiacBody, name: 'zodiac', idAttr: 'pattern-id', nameAttr: 'pattern-name' },
                    { body: $lunarBody, name: 'lunar', idAttr: 'pattern-id', nameAttr: 'pattern-name' },
                    { body: $starsBody, name: 'stars', idAttr: 'pattern-id', nameAttr: 'pattern-name' }
                ];
                
                for (var t = 0; t < allTables.length; t++) {
                    var table = allTables[t];
                    if (!table.body || !table.body.length) continue;
                    
                    var fallbackMatched = false;
                    table.body.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowId = $row.data(table.idAttr);
                        var rowName = $row.data(table.nameAttr);
                        
                        var rowIdStr = rowId ? String(rowId) : '';
                        var rowNameStr = rowName ? String(rowName) : '';
                        
                        for (var s = 0; s < searchIds.length; s++) {
                            var searchId = String(searchIds[s]);
                            if (!searchId) continue;
                            
                            // Check if searchId is contained in rowId
                            if (rowIdStr.indexOf(searchId) !== -1) {
                                $row.addClass('selected-row');
                                selectedRow = $row;
                                fallbackMatched = true;
                                console.log("[SkyCultureStats] Fallback matched in", table.name, "by ID partial:", rowIdStr);
                                return false;
                            }
                            
                            // Check if rowId is contained in searchId
                            if (searchId.indexOf(rowIdStr) !== -1 && rowIdStr.length > 2) {
                                $row.addClass('selected-row');
                                selectedRow = $row;
                                fallbackMatched = true;
                                console.log("[SkyCultureStats] Fallback matched in", table.name, "by reverse ID partial:", searchId);
                                return false;
                            }
                        }
                        
                        // Match by name (case-insensitive)
                        if (!fallbackMatched && searchNames.length > 0) {
                            for (var n = 0; n < searchNames.length; n++) {
                                var searchTerm = String(searchNames[n]).toLowerCase();
                                if (!searchTerm) continue;
                                if (rowNameStr.toLowerCase() === searchTerm) {
                                    $row.addClass('selected-row');
                                    selectedRow = $row;
                                    fallbackMatched = true;
                                    console.log("[SkyCultureStats] Fallback matched in", table.name, "by name:", rowNameStr);
                                    return false;
                                }
                            }
                        }
                    });
                    
                    if (fallbackMatched) {
                        matched = true;
                        break;
                    }
                }
            }
            
            if (!matched) {
                console.log("[SkyCultureStats] No match found for:", { type: data.type, ids: searchIds, names: searchNames });
            }
        });
		}

    /**
     * Update culture button active states.
     */
    function updateCultureButtonStates() {
        if (!$cultureContainer) return;
        $cultureContainer.find('.skyculture-btn').removeClass('active');
        $cultureContainer.find('[data-culture-id="' + currentCultureId + '"]').addClass('active');
        if (currentCultureName) {
            var $activeBtn = $cultureContainer.find('[data-culture-id="' + currentCultureId + '"]');
            if ($activeBtn.length && $activeBtn.text() !== currentCultureName) {
                $activeBtn.text(currentCultureName);
            }
        }
    }

    // =====================================================================
    // RENDERING FUNCTIONS
    // =====================================================================

    /**
     * Render culture selection buttons.
     */
    function renderCultureButtons() {
        if (!$cultureContainer) return;
        $cultureContainer.empty();
        
        if (!availableCultures || !availableCultures.length) {
            $cultureContainer.html('<div class="loading-placeholder">' + 
                _tr("No sky cultures available") + '</div>');
            return;
        }
        
        var buttonsHtml = '<div class="skyculture-buttons-grid">';
        availableCultures.forEach(function(culture) {
            var isActive = (culture.id === currentCultureId);
            var displayName = culture.name;
            if (isActive && currentCultureName) displayName = currentCultureName;
            buttonsHtml += '<button type="button" class="skyculture-btn' + 
                (isActive ? ' active' : '') + 
                '" data-culture-id="' + escapeHtml(culture.id) + '">' + 
                escapeHtml(displayName) + '</button>';
        });
        buttonsHtml += '</div>';
        $cultureContainer.html(buttonsHtml);
        
        if ($cultureCount && $cultureCount.length) {
            $cultureCount.text('(' + availableCultures.length + ')');
        }
        
        $cultureContainer.find('.skyculture-btn').on('click', function() {
            var cultureId = $(this).data('culture-id');
            if (cultureId && cultureId !== currentCultureId) selectCulture(cultureId);
        });
    }

    /**
     * Select a culture and load its data.
     * @param {string} cultureId - Culture ID to select
     */
    function selectCulture(cultureId) {
        if (cultureId === currentCultureId) return;
        currentCultureId = cultureId;
        var culture = availableCultures.find(function(c) { return c.id === cultureId; });
        currentCultureName = culture ? culture.name : cultureId;
        updateCultureButtonStates();
        if (propApi && typeof propApi.setStelProp === 'function') {
            propApi.setStelProp("StelSkyCultureMgr.currentSkyCultureID", cultureId);
        }
        delete CULTURE_DATA_CACHE[cultureId];
        loadCultureData(cultureId, function() {
            if (currentCultureData) {
                updateTotalStats(currentCultureData);
            }
        });
        
        if ($descriptionFrame && $descriptionFrame.length) {
            $descriptionFrame.attr('src', '/api/view/skyculturedescription/');
        }
    }

    /**
     * Display all culture data in tables.
     * @param {Object} data - Culture data from index.json
     */
    function displayCultureData(data) {
        renderConstellationsTable(data.constellations || []);
        renderAsterismsTable(data.asterisms || []);
        renderZodiacTable(data.zodiac);
        renderLunarTable(data.lunar_system);
        renderStarsTable(data.common_names || {});
    }

    /**
     * Render constellations table with alphabetical sorting by ID.
     * 
     * @param {Array} constellations - Array of constellation objects
     */
    function renderConstellationsTable(constellations) {
        if (!$constellationsBody) return;
        
        // Sort alphabetically by ID (e.g., "CON modern Ara", "CON modern Com", etc.)
        var sorted = constellations.slice().sort(function(a, b) {
            var idA = a.id || '';
            var idB = b.id || '';
            return idA.localeCompare(idB);
        });
        
        var count = sorted.length;
        updateCounter($constellationsCount, count);
        
        if (count === 0) {
            $constellationsBody.html('<tr><td colspan="9" class="no-data">' + 
                _tr("No constellations available") + '</td></tr>');
            return;
        }
        
        var html = '';
        for (var i = 0; i < sorted.length; i++) {
            var con = sorted[i];
            var ids = extractIdsFromLines(con.lines);
            var idsDisplay = formatIdsDisplay(ids);
            var nativeName = (con.common_name && con.common_name.native) || '-';
            var englishName = (con.common_name && con.common_name.english) || '-';
            var pronounce = (con.common_name && con.common_name.pronounce) || '-';
            var conId = con.id || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="constellation" ' +
                    'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
                    'data-pattern-id="' + escapeHtml(conId) + '" ' +
                    'data-search-name="' + escapeHtml(englishName || nativeName) + '">' +
                    '<td class="row-number">' + (i + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(conId) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '<td>' + ids.length + '</td>' +
                    '<td class="ids-display">' + idsDisplay + '</td>' +
                    '</tr>';
        }
        
        $constellationsBody.html(html);
        
        // Attach click handlers
        $constellationsBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'constellation', $row.data('pattern-name'), 
                $row.data('pattern-id'), $row.data('search-name'));
        });
    }

    /**
     * Render asterisms table with alphabetical sorting by ID.
     * 
     * @param {Array} asterisms - Array of asterism objects
     */
    function renderAsterismsTable(asterisms) {
        if (!$asterismsBody) return;
        
        // Filter out ray helpers first
        var filtered = asterisms.filter(function(a) { return !a.is_ray_helper; });
        
        // Sort alphabetically by ID
        var sorted = filtered.slice().sort(function(a, b) {
            var idA = a.id || '';
            var idB = b.id || '';
            return idA.localeCompare(idB);
        });
        
        updateCounter($asterismsCount, sorted.length);
        
        if (sorted.length === 0) {
            $asterismsBody.html('<tr><td colspan="9" class="no-data">' + 
                _tr("No asterisms available") + '</td></tr>');
            return;
        }
        
        var html = '';
        for (var i = 0; i < sorted.length; i++) {
            var ast = sorted[i];
            var ids = extractIdsFromLines(ast.lines);
            var idsDisplay = formatIdsDisplay(ids);
            var nativeName = (ast.common_name && ast.common_name.native) || '-';
            var englishName = (ast.common_name && ast.common_name.english) || '-';
            var pronounce = (ast.common_name && ast.common_name.pronounce) || '-';
            var astId = ast.id || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="asterism" ' +
                    'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
                    'data-pattern-id="' + escapeHtml(astId) + '" ' +
                    'data-search-name="' + escapeHtml(englishName || nativeName) + '">' +
                    '<td class="row-number">' + (i + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(astId) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '<td>' + ids.length + '</td>' +
                    '<td class="ids-display">' + idsDisplay + '</td>' +
                    '</tr>';
        }
        
        $asterismsBody.html(html);
        
        // Attach click handlers
        $asterismsBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'asterism', $row.data('pattern-name'), 
                $row.data('pattern-id'), $row.data('search-name'));
        });
    }

		/**
		 * Render zodiac signs table with alphabetical sorting by English name.
		 * 
		 * IMPORTANT: This function adds hidden data attributes for synchronization
		 * with the button interface (skyculture.js):
		 * - data-zodiac-id: Composite ID like "zodiac_modern_0" matching enhanced skyculture.js format
		 * - data-zodiac-simple-id: Simple ID like "aries" for backward compatibility
		 * 
		 * The visible content remains unchanged - only the original data from
		 * index.json is displayed. Hidden attributes are for internal sync only.
		 * 
		 * @param {Object} zodiac - Zodiac data object
		 */
		function renderZodiacTable(zodiac) {
				if (!$zodiacBody) return;
				
				var names = (zodiac && zodiac.names) ? zodiac.names.slice() : [];
				
				// Sort alphabetically by English name for display
				names.sort(function(a, b) {
						var nameA = a.english || a.native || '';
						var nameB = b.english || b.native || '';
						return nameA.localeCompare(nameB);
				});
				
				updateCounter($zodiacCount, names.length);
				
				if (names.length === 0) {
						$zodiacBody.html('<tr><td colspan="6" class="no-data">' + 
								_tr("No zodiac data available") + 'NonNull79');
						return;
				}
				
				var html = '';
				var cultureId = currentCultureId || 'unknown';
				
				for (var i = 0; i < names.length; i++) {
						var sign = names[i];
						var nativeName = sign.native || '-';
						var englishName = sign.english || '-';
						var pronounce = sign.pronounce || '-';
						var symbol = sign.symbol || '-';
						var translation = englishName !== '-' ? _tr(englishName) : '-';
						
						// Create composite ID matching enhanced skyculture.js format
						// Format: "zodiac_cultureId_index" (e.g., "zodiac_modern_0")
						var zodiacCompositeId = "zodiac_" + cultureId + "_" + i;
						
						// Create simple ID for backward compatibility
						var simpleId = englishName.toLowerCase().replace(/\s+/g, '_');
						
						html += '<tr class="clickable-row" data-pattern-type="zodiac" ' +
										'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
										'data-search-name="' + escapeHtml(englishName || nativeName) + '" ' +
										'data-zodiac-id="' + escapeHtml(zodiacCompositeId) + '" ' +
										'data-zodiac-simple-id="' + escapeHtml(simpleId) + '" ' +
										'>' +
										'<td class="row-number">' + (i + 1) + '</td>' +
										'<td>' + escapeHtml(symbol) + '</td>' +
										'<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
										'<td>' + escapeHtml(englishName) + '</td>' +
										'<td>' + escapeHtml(translation) + '</td>' +
										'<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
										'</tr>';
				}
				
				$zodiacBody.html(html);
				
				// Attach click handlers - use composite ID for navigation
				$zodiacBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
						var $row = $(this);
						var zodiacId = $row.data('zodiac-id');      // Composite ID for primary matching
						var patternName = $row.data('pattern-name');
						var searchName = $row.data('search-name');
						
						handleRowToggle($row, 'zodiac', patternName, zodiacId, searchName);
				});
		}
		
		/**
		 * Render lunar mansions table with alphabetical sorting by English name.
		 * 
		 * IMPORTANT: This function adds hidden data attributes for proper
		 * bidirectional synchronization with skyculture.js buttons:
		 * - data-lunar-id: Composite ID like "lunar_16" matching skyculture.js format
		 * - data-lunar-index: Numeric index (1-based) for reliable matching
		 * - data-pattern-name: Native name for display
		 * - data-search-name: English name for navigation
		 * 
		 * The visible content remains unchanged - only the original data from
		 * index.json is displayed. Hidden attributes are for internal sync only.
		 * 
		 * @param {Object} lunar - Lunar system data object
		 */
		function renderLunarTable(lunar) {
				if (!$lunarBody) return;
				
				var names = (lunar && lunar.names) ? lunar.names.slice() : [];
				
			/*	// Sort alphabetically by English name
				names.sort(function(a, b) {
						var nameA = a.english || a.native || '';
						var nameB = b.english || b.native || '';
						return nameA.localeCompare(nameB);
				});*/
				
				updateCounter($lunarCount, names.length);
				
				if (names.length === 0) {
						$lunarBody.html('<tr><td colspan="6" class="no-data">' + 
								_tr("No lunar mansions data available") + '</td></tr>');
						return;
				}
				
				var html = '';
				for (var i = 0; i < names.length; i++) {
						var mansion = names[i];
						var nativeName = mansion.native || '-';
						var englishName = mansion.english || '-';
						var pronounce = mansion.pronounce || '-';
						var symbol = mansion.symbol || '-';
						var translation = englishName !== '-' ? _tr(englishName) : '-';
						
						// Create composite ID matching skyculture.js format (lunar_1, lunar_2, ...)
						var lunarCompositeId = "lunar_" + (i + 1);
						var lunarIndex = i + 1;
						
						html += '<tr class="clickable-row" data-pattern-type="lunar" ' +
										'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
										'data-search-name="' + escapeHtml(englishName || nativeName) + '" ' +
										'data-lunar-id="' + escapeHtml(lunarCompositeId) + '" ' +
										'data-lunar-index="' + lunarIndex + '" ' +
										'data-english-name="' + escapeHtml(englishName) + '" ' +
										'>' +
										'<td class="row-number">' + (i + 1) + '</td>' +
										'<td>' + escapeHtml(symbol) + '</td>' +
										'<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
										'<td>' + escapeHtml(englishName) + '</td>' +
										'<td>' + escapeHtml(translation) + '</td>' +
										'<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
										'</tr>';
				}
				
				$lunarBody.html(html);
				
				// Attach click handlers for forward sync (Table → Stellarium → Buttons)
				$lunarBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
						var $row = $(this);
						var lunarId = $row.data('lunar-id');           // "lunar_16"
						var patternName = $row.data('pattern-name');   // Native name (娄)
						var searchName = $row.data('search-name');     // English name (Bond)
						
						// IMPORTANT: Call handleRowToggle with all three identifiers
						handleRowToggle($row, 'lunar', patternName, lunarId, searchName);
				});
		}

    /**
     * Render notable stars table with alphabetical sorting by ID.
     * 
     * @param {Object} commonNames - Common names object from index.json
     */
    function renderStarsTable(commonNames) {
        if (!$starsBody) return;
        
        // First, collect all star entries into a flat array
        var starEntries = [];
        
        for (var hipId in commonNames) {
            if (commonNames.hasOwnProperty(hipId)) {
                var entries = commonNames[hipId];
                if (Array.isArray(entries)) {
                    entries.forEach(function(entry) {
                        starEntries.push({
                            id: hipId,
                            native: entry.native || '',
                            english: entry.english || '',
                            pronounce: entry.pronounce || '',
                            ipa: entry.IPA || ''
                        });
                    });
                }
            }
        }
        
        // Sort alphabetically by ID (HIP number)
        starEntries.sort(function(a, b) {
            // Convert HIP IDs to numbers for numeric sorting, fallback to string comparison
            var aNum = parseInt(a.id, 10);
            var bNum = parseInt(b.id, 10);
            
            if (!isNaN(aNum) && !isNaN(bNum)) {
                return aNum - bNum;
            }
            return String(a.id).localeCompare(String(b.id));
        });
        
        updateCounter($starsCount, starEntries.length);
        
        if (starEntries.length === 0) {
            $starsBody.html('<tr><td colspan="8" class="no-data">' + 
                _tr("No star name data available") + '</td></tr>');
            return;
        }
        
        var html = '';
        for (var i = 0; i < starEntries.length; i++) {
            var star = starEntries[i];
            var parsed = parseIdType(star.id);
            var englishName = star.english || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="star" ' +
                    'data-pattern-id="' + escapeHtml(star.id) + '" ' +
                    'data-pattern-name="' + escapeHtml(star.native || star.english) + '">' +
                    '<td class="row-number">' + (i + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(star.id) + '</td>' +
                    '<td><span class="id-type-badge ' + parsed.type + '">' + 
                    escapeHtml(parsed.display) + '</span></td>' +
                    '<td><span class="native-name">' + escapeHtml(star.native || '-') + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(star.pronounce || '-') + '</td>' +
                    '<td class="ipa-value">' + escapeHtml(star.ipa || '-') + '</td>' +
                    '</tr>';
        }
        
        $starsBody.html(html);
        
        // Attach click handlers
        $starsBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'star', $row.data('pattern-name'), 
                $row.data('pattern-id'), $row.data('pattern-name'));
        });
    }

    // =====================================================================
    // SEARCH FUNCTIONALITY
    // =====================================================================

    /**
     * Filter table by search term.
     * @param {string} tbodySelector - Selector for tbody element
     * @param {string} term - Search term
     */
    function filterTable(tbodySelector, term) {
        var $tbody = $(tbodySelector);
        if (!$tbody || !$tbody.length) return;
        var rows = $tbody.find('tr');
        var searchTerm = term.toLowerCase().trim();
        if (searchTerm === '') { rows.show(); return; }
        rows.each(function() {
            var $row = $(this);
            if ($row.find('.loading-placeholder').length || $row.find('.no-data').length) return;
            $row.toggle($row.text().toLowerCase().indexOf(searchTerm) !== -1);
        });
    }

    /**
     * Clear all search inputs.
     */
    function clearAllSearchInputs() {
        $('#search-constellations-stats, #search-asterisms-stats, #search-stars-stats').each(function() {
            $(this).val('').trigger('keyup');
        });
    }

    /**
     * Setup search handlers.
     */
    function setupSearchHandlers() {
        $(document).on('keyup', '#search-constellations-stats', function(e) {
            if (e.key === 'Escape' || e.keyCode === 27) $(this).val('');
            filterTable('#constellations-stats-body', $(this).val());
        });
        $(document).on('keyup', '#search-asterisms-stats', function(e) {
            if (e.key === 'Escape' || e.keyCode === 27) $(this).val('');
            filterTable('#asterisms-stats-body', $(this).val());
        });
        $(document).on('keyup', '#search-stars-stats', function(e) {
            if (e.key === 'Escape' || e.keyCode === 27) $(this).val('');
            filterTable('#stars-stats-body', $(this).val());
        });
        $(document).on('keydown', function(e) {
            if ((e.key === 'Escape' || e.keyCode === 27) && 
                !$(document.activeElement).hasClass('search-box')) {
                clearAllSearchInputs();
            }
        });
    }

    // =====================================================================
    // EXPORT FUNCTIONS
    // =====================================================================

		/**
		 * Export table data to CSV format with semicolon delimiter.
		 * 
		 * WHY SEMICOLON INSTEAD OF COMMA?
		 * ===============================
		 * Standard CSV uses comma (,) as delimiter, but Arabic/European regional 
		 * settings in Microsoft Excel expect semicolon (;) as the default list 
		 * separator. When opening comma-delimited CSV files in Arabic Excel, all 
		 * data appears in a single column because Excel fails to recognize commas
		 * as cell separators.
		 * 
		 * Using semicolon ensures:
		 * - Each value appears in its own cell (horizontal layout)
		 * - Automatic column detection when double-clicking the file
		 * - Compatibility with Arabic, French, German, and other European locales
		 * - No need for manual Text Import Wizard configuration
		 * 
		 * ADDITIONAL IMPROVEMENT:
		 * ======================
		 * Previously, all IDs were concatenated into a single cell (e.g., "86263, 84012").
		 * Now each ID occupies its own dedicated column (ID 1, ID 2, ID 3, ...) horizontally,
		 * allowing for proper data analysis, filtering, and lookup operations in Excel.
		 * 
		 * @param {string} type - Data type to export ('constellations', 'asterisms', 'stars')
		 */
		function exportToCSV(type) {
				if (!currentCultureData) return;
				
				// Use semicolon delimiter for Arabic Excel compatibility
				var DELIMITER = ';';
				var filename = (currentCultureId || 'culture') + '_' + type + '.csv';
				var headers = [];
				var data = [];
				
				/**
				 * Sanitize a cell value for CSV format.
				 * Escapes double quotes and wraps values containing delimiter or special
				 * characters in double quotes per CSV standard (RFC 4180).
				 * 
				 * @param {*} value - The cell value to sanitize
				 * @returns {string} Sanitized cell value ready for CSV output
				 */
				function sanitizeCell(value) {
						var str = String(value || '');
						// Check if the value contains the delimiter, quotes, or newlines
						if (str.indexOf(DELIMITER) !== -1 || str.indexOf('"') !== -1 || str.indexOf('\n') !== -1) {
								// Escape internal double quotes by doubling them
								return '"' + str.replace(/"/g, '""') + '"';
						}
						return str;
				}
				
				if (type === 'constellations' || type === 'asterisms') {
						// Determine the source dataset based on type
						var items;
						if (type === 'constellations') {
								items = (currentCultureData.constellations || []).slice();
						} else {
								items = (currentCultureData.asterisms || []).filter(function(a) { 
										return !a.is_ray_helper; 
								});
						}
						
						// Sort alphabetically by native name (fallback to English, then ID)
						items.sort(function(a, b) {
								var nameA = (a.common_name && a.common_name.native) || 
														(a.common_name && a.common_name.english) || a.id || '';
								var nameB = (b.common_name && b.common_name.native) || 
														(b.common_name && b.common_name.english) || b.id || '';
								return nameA.localeCompare(nameB);
						});
						
						// Calculate maximum number of IDs across all rows to create enough columns
						var maxIdCount = 0;
						var allRowsData = [];
						
						items.forEach(function(item, index) {
								var ids = extractIdsFromLines(item.lines);
								if (ids.length > maxIdCount) maxIdCount = ids.length;
								
								var englishName = (item.common_name && item.common_name.english) || '';
								allRowsData.push({
										index: index + 1,
										id: item.id || '',
										native: (item.common_name && item.common_name.native) || '',
										english: englishName,
										translation: englishName ? _tr(englishName) : '',
										pronounce: (item.common_name && item.common_name.pronounce) || '',
										starCount: ids.length,
										ids: ids
								});
						});
						
						// Build column headers with dynamic ID columns
						headers = ['#', 'ID', 'Native Name', 'English Name', 'Translation', 'Pronounce', 'Stars'];
						for (var i = 1; i <= maxIdCount; i++) {
								headers.push('ID ' + i);
						}
						
						// Build data rows with each ID in its own cell
						allRowsData.forEach(function(row) {
								var rowData = [
										row.index,
										row.id,
										row.native,
										row.english,
										row.translation,
										row.pronounce,
										row.starCount
								];
								// Add each ID as a separate cell
								for (var j = 0; j < maxIdCount; j++) {
										rowData.push(j < row.ids.length ? row.ids[j] : '');
								}
								data.push(rowData);
						});
						
				} else if (type === 'stars') {
						// Build star entries from common_names data
						var starEntries = [];
						var commonNames = currentCultureData.common_names || {};
						for (var hipId in commonNames) {
								if (commonNames.hasOwnProperty(hipId)) {
										var entries = commonNames[hipId];
										if (Array.isArray(entries)) {
												entries.forEach(function(entry) {
														starEntries.push({ 
																id: hipId, 
																native: entry.native || '', 
																english: entry.english || '',
																pronounce: entry.pronounce || '', 
																ipa: entry.IPA || '' 
														});
												});
										}
								}
						}
						
						starEntries.sort(function(a, b) { 
								return (a.native || a.english).localeCompare(b.native || b.english); 
						});
						
						headers = ['#', 'Original ID', 'Type', 'Native Name', 'English Name', 'Translation', 'Pronounce', 'IPA'];
						
						starEntries.forEach(function(star, index) {
								var parsed = parseIdType(star.id);
								data.push([
										index + 1, 
										star.id, 
										parsed.display, 
										star.native, 
										star.english,
										star.english ? _tr(star.english) : '',
										star.pronounce, 
										star.ipa
								]);
						});
				}
				
				// Guard against empty datasets
				if (!data.length) return;
				
				// Build CSV content with BOM for UTF-8 support in Excel
				// BOM (\ufeff) ensures Excel recognizes the file as UTF-8 encoded
				var csvContent = '\ufeff';
				
				// Add sanitized headers row
				csvContent += headers.map(function(h) { return sanitizeCell(h); }).join(DELIMITER) + '\n';
				
				// Add sanitized data rows
				csvContent += data.map(function(row) {
						return row.map(function(cell) { 
								return sanitizeCell(cell); 
						}).join(DELIMITER);
				}).join('\n');
				
				// Create and trigger download
				var blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
				var url = URL.createObjectURL(blob);
				var a = document.createElement('a');
				a.href = url; 
				a.download = filename;
				// Append to DOM for Firefox compatibility
				document.body.appendChild(a);
				a.click();
				document.body.removeChild(a);
				// Delay URL revocation to ensure download completes
				setTimeout(function() { URL.revokeObjectURL(url); }, 100);
		}


		/**
		 * Export table data to JSON format.
		 * 
		 * @param {string} type - Data type to export ('constellations', 'asterisms', 'zodiac', 'lunar', 'stars')
		 */
		function exportToJSON(type) {
				if (!currentCultureData) return;
				
				var exportData = { 
						culture: currentCultureId, 
						cultureName: currentCultureName, 
						type: type, 
						timestamp: new Date().toISOString(),
						totalRecords: 0
				};
				
				switch(type) {
						case 'constellations': 
								exportData.data = currentCultureData.constellations || [];
								exportData.totalRecords = exportData.data.length;
								break;
						case 'asterisms': 
								exportData.data = (currentCultureData.asterisms || []).filter(function(a) { 
										return !a.is_ray_helper; 
								}); 
								exportData.totalRecords = exportData.data.length;
								break;
						case 'zodiac': 
								exportData.data = currentCultureData.zodiac || {};
								exportData.totalRecords = (exportData.data.names) ? exportData.data.names.length : 0;
								break;
						case 'lunar': 
								exportData.data = currentCultureData.lunar_system || {};
								exportData.totalRecords = (exportData.data.names) ? exportData.data.names.length : 0;
								break;
						case 'stars': 
								exportData.data = currentCultureData.common_names || {};
								exportData.totalRecords = Object.keys(exportData.data).length;
								break;
				}
				
				var jsonContent = JSON.stringify(exportData, null, 2);
				var blob = new Blob(['\ufeff' + jsonContent], { type: 'application/json;charset=utf-8;' });
				var url = URL.createObjectURL(blob);
				var a = document.createElement('a');
				a.href = url; 
				a.download = (currentCultureId || 'culture') + '_' + type + '.json';
				document.body.appendChild(a);
				a.click();
				document.body.removeChild(a);
				setTimeout(function() { URL.revokeObjectURL(url); }, 100);
		}

    /**
     * Setup export button handlers.
     */
    function setupExportHandlers() {
        $(document).on('click', '#export-constellations-csv', function() { exportToCSV('constellations'); });
        $(document).on('click', '#export-constellations-json', function() { exportToJSON('constellations'); });
        $(document).on('click', '#export-asterisms-csv', function() { exportToCSV('asterisms'); });
        $(document).on('click', '#export-asterisms-json', function() { exportToJSON('asterisms'); });
        $(document).on('click', '#export-stars-csv', function() { exportToCSV('stars'); });
        $(document).on('click', '#export-stars-json', function() { exportToJSON('stars'); });
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the sky culture statistics module.
     * @param {Object} options - Configuration options
     */
    function init(options) {
        if (isInitialized) return;
        options = options || {};
        
        $cultureContainer = $(options.cultureContainer || '#skyculture-stats-buttons');
        $cultureCount = $(options.cultureCount || '#skyculture-stats-count');
        $totalCultures = $(options.totalCultures || '#stats-total-cultures');
        $totalConstellations = $(options.totalConstellations || '#stats-total-constellations');
        $totalAsterisms = $(options.totalAsterisms || '#stats-total-asterisms');
				$totalRayHelpers = $(options.totalRayHelpers || '#stats-total-ray-helpers');
        $totalZodiac = $(options.totalZodiac || '#stats-total-zodiac');
        $totalLunar = $(options.totalLunar || '#stats-total-lunar');
        $totalStars = $(options.totalStars || '#stats-total-stars');
        
        $constellationsBody = $(options.constellationsBody || '#constellations-stats-body');
        $asterismsBody = $(options.asterismsBody || '#asterisms-stats-body');
        $zodiacBody = $(options.zodiacBody || '#zodiac-stats-body');
        $lunarBody = $(options.lunarBody || '#lunar-stats-body');
        $starsBody = $(options.starsBody || '#stars-stats-body');
        
        $constellationsCount = $(options.constellationsCount || '#constellations-stats-count');
        $asterismsCount = $(options.asterismsCount || '#asterisms-stats-count');
        $zodiacCount = $(options.zodiacCount || '#zodiac-stats-count');
        $lunarCount = $(options.lunarCount || '#lunar-stats-count');
        $starsCount = $(options.starsCount || '#stars-stats-count');
        
        $descriptionFrame = $(options.descriptionFrame || '#stats-vo-skycultureinfo');
        
        if (!$cultureContainer || !$cultureContainer.length) {
            console.error("[SkyCultureStats] Culture container not found");
            return;
        }
        
        getCurrentLanguage();
        setupSearchHandlers();
        setupExportHandlers();
        setupSyncListener();
        
        if ($("#skyculture-stats-tabs").length) {
            $("#skyculture-stats-tabs").tabs({ heightStyle: "content" });
        }
        
        loadSkyCultures(function(success) { 
            if (success) {
                isInitialized = true;
                console.log("[SkyCultureStats] Module initialized successfully v3.1.0");
            }
        });
    }

    /**
     * Refresh all data.
     */
    function refresh() { 
        CULTURE_DATA_CACHE = {}; 
        loadSkyCultures(); 
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        init: init,
        refresh: refresh,
        selectCulture: selectCulture,
        getCurrentCulture: function() { return currentCultureId; },
        getAvailableCultures: function() { return availableCultures.slice(); },
        exportToCSV: exportToCSV,
        exportToJSON: exportToJSON,
        getCurrentLanguage: getCurrentLanguage
    };
});