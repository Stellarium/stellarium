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
     * Parse ID type from string identifier.
     * @param {string} id - The ID string to parse
     * @returns {Object} Object with type, value, and display properties
     */
    function parseIdType(id) {
        if (typeof id !== 'string') return { type: 'other', value: String(id), display: 'Other' };
        
        if (id.startsWith('HIP ')) return { type: 'hip', value: id.replace('HIP ', ''), display: 'HIP' };
        if (id.startsWith('M ')) return { type: 'messier', value: id.replace('M ', ''), display: 'M' };
        if (id.startsWith('NGC ')) return { type: 'ngc', value: id.replace('NGC ', ''), display: 'NGC' };
        if (id.startsWith('IC ')) return { type: 'ic', value: id.replace('IC ', ''), display: 'IC' };
        if (id.startsWith('DSO:')) return { type: 'dso', value: id.replace('DSO:', ''), display: 'DSO' };
        if (id.startsWith('Gaia DR3 ')) return { type: 'gaia', value: id.replace('Gaia DR3 ', ''), display: 'Gaia' };
        if (id.startsWith('NAME ')) return { type: 'solar', value: id.replace('NAME ', ''), display: 'Solar' };
        
        if (id.startsWith('NGC')) { 
            var num = id.substring(3); 
            if (/^\d+$/.test(num) || /^[A-Z]/.test(num)) return { type: 'ngc', value: id, display: 'NGC' }; 
        }
        if (id.startsWith('IC')) { 
            var num = id.substring(2); 
            if (/^\d+$/.test(num) || /^[A-Z]/.test(num)) return { type: 'ic', value: id, display: 'IC' }; 
        }
        if (id.startsWith('PGC')) return { type: 'pgc', value: id, display: 'PGC' };
        if (id.startsWith('UGC')) return { type: 'ugc', value: id, display: 'UGC' };
        if (id.startsWith('ESO')) return { type: 'eso', value: id, display: 'ESO' };
        if (id.startsWith('ACO')) return { type: 'aco', value: id, display: 'ACO' };
        if (id.startsWith('SH2-')) return { type: 'sh2', value: id, display: 'SH2' };
        if (id.startsWith('PK')) return { type: 'pk', value: id, display: 'PK' };
        if (id.startsWith('PNG')) return { type: 'png', value: id, display: 'PNG' };
        if (id.startsWith('CR')) { 
            var num = id.substring(2); 
            if (/^\d+$/.test(num)) return { type: 'cr', value: id, display: 'CR' }; 
        }
        if (id.startsWith('B')) { 
            var num = id.substring(1); 
            if (/^\d+$/.test(num)) return { type: 'b', value: id, display: 'B' }; 
        }
        
        if (/^\d+$/.test(id)) return { type: 'hip', value: id, display: 'HIP' };
        if (id.startsWith('CON ')) return { type: 'constellation', value: id.replace('CON ', ''), display: 'CON' };
        if (id.startsWith('AST ')) return { type: 'asterism', value: id.replace('AST ', ''), display: 'AST' };
        
        return { type: 'other', value: id, display: 'Other' };
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
     * @param {Array} ids - Array of ID strings
     * @returns {string} HTML string with formatted badges
     */
    function formatIdsDisplay(ids) {
        if (!ids || ids.length === 0) return '-';
        return ids.map(function(id) {
            var parsed = parseIdType(id);
            return '<span class="id-type-badge ' + parsed.type + '">' + 
                   escapeHtml(parsed.display) + '</span>' + escapeHtml(parsed.value);
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
        tables.forEach(function($tbody) { 
            if ($tbody) $tbody.find('.selected-row').removeClass('selected-row'); 
        });
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
     * Handle row toggle selection.
     * @param {jQuery} $row - The row element
     * @param {string} patternType - Type of pattern
     * @param {string} patternName - Pattern name
     * @param {string} patternId - Pattern ID
     * @param {string} searchName - Search name
     */
    function handleRowToggle($row, patternType, patternName, patternId, searchName) {
        var isCurrentlySelected = $row.hasClass('selected-row');
        
        if (isCurrentlySelected) {
            $row.removeClass('selected-row');
            selectedRow = null;
            if (patternType === 'constellation') stelUtils.clearConstellationHighlight();
            else stelUtils.clearSelection();
            stelUtils.emitObjectSelected('', '', 'none');
        } else {
            clearAllRowHighlights();
            $row.addClass('selected-row');
            selectedRow = $row;
            
            if (patternType === 'constellation') {
                stelUtils.toggleConstellationHighlight(searchName || patternName);
            } else if (patternType === 'asterism') {
                stelUtils.goToAsterism(searchName || patternName, patternId);
            } else if (patternType === 'zodiac') {
                stelUtils.goToZodiacSign(searchName || patternName, patternId);
            } else if (patternType === 'lunar') {
                stelUtils.goToLunarMansion(searchName || patternName, patternId);
            } else if (patternType === 'star') {
                stelUtils.goToObject(patternId, 15, 'star', currentCultureData);
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
        
        // Listen to objectSelected events
        $(document).on("objectSelected", function(evt, data) {
            if (!data || !data.type) return;
            clearAllRowHighlights();
            
            if (data.type === 'none') return;
            
            if (data.type === "constellation" && data.name) {
                if ($constellationsBody) {
                    $constellationsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowName = $row.data('pattern-name');
                        var searchName = $row.data('search-name');
                        if (rowName === data.name || searchName === data.name) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            return false;
                        }
                    });
                }
            } else if (data.type === "asterism" && data.name) {
                if ($asterismsBody) {
                    $asterismsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        var rowName = $row.data('pattern-name');
                        var searchName = $row.data('search-name');
                        if (rowName === data.name || searchName === data.name || 
                            $row.data('pattern-id') === data.id) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            return false;
                        }
                    });
                }
            } else if (data.type === "zodiac" && data.name) {
                if ($zodiacBody) {
                    $zodiacBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        if ($row.data('pattern-name') === data.name || 
                            $row.data('search-name') === data.name) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            return false;
                        }
                    });
                }
            } else if (data.type === "lunar" && data.name) {
                if ($lunarBody) {
                    $lunarBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        if ($row.data('pattern-name') === data.name || 
                            $row.data('search-name') === data.name) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            return false;
                        }
                    });
                }
            } else if (data.type === "star" && data.id) {
                if ($starsBody) {
                    $starsBody.find('.clickable-row').each(function() {
                        var $row = $(this);
                        if ($row.data('pattern-id') === data.id) {
                            $row.addClass('selected-row');
                            selectedRow = $row;
                            return false;
                        }
                    });
                }
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
     * Render constellations table.
     * @param {Array} constellations - Array of constellation objects
     */
    function renderConstellationsTable(constellations) {
        if (!$constellationsBody) return;
        var count = constellations.length;
        updateCounter($constellationsCount, count);
        
        if (count === 0) {
            $constellationsBody.html('<tr><td colspan="9" class="no-data">' + 
                _tr("No constellations available") + '</td></tr>');
            return;
        }
        
        var sorted = constellations.slice().sort(function(a, b) {
            var nameA = (a.common_name && a.common_name.native) || 
                        (a.common_name && a.common_name.english) || a.id || '';
            var nameB = (b.common_name && b.common_name.native) || 
                        (b.common_name && b.common_name.english) || b.id || '';
            return nameA.localeCompare(nameB);
        });
        
        var html = '';
        sorted.forEach(function(con, index) {
            var ids = extractIdsFromLines(con.lines);
            var idsDisplay = formatIdsDisplay(ids);
            var nativeName = (con.common_name && con.common_name.native) || '-';
            var englishName = (con.common_name && con.common_name.english) || '-';
            var pronounce = (con.common_name && con.common_name.pronounce) || '-';
            var conId = con.id || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="constellation" ' +
                    'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
                    'data-search-name="' + escapeHtml(englishName || nativeName) + '">' +
                    '<td class="row-number">' + (index + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(conId) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '<td>' + ids.length + '</td>' +
                    '<td class="ids-display">' + idsDisplay + '</td>' +
                    '</tr>';
        });
        
        $constellationsBody.html(html);
        $constellationsBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'constellation', $row.data('pattern-name'), null, $row.data('search-name'));
        });
    }

    /**
     * Render asterisms table.
     * @param {Array} asterisms - Array of asterism objects
     */
    function renderAsterismsTable(asterisms) {
        if (!$asterismsBody) return;
        var filtered = asterisms.filter(function(a) { return !a.is_ray_helper; });
        updateCounter($asterismsCount, filtered.length);
        
        if (filtered.length === 0) {
            $asterismsBody.html('<tr><td colspan="9" class="no-data">' + 
                _tr("No asterisms available") + '</td></tr>');
            return;
        }
        
        var sorted = filtered.slice().sort(function(a, b) {
            var nameA = (a.common_name && a.common_name.native) || 
                        (a.common_name && a.common_name.english) || a.id || '';
            var nameB = (b.common_name && b.common_name.native) || 
                        (b.common_name && b.common_name.english) || b.id || '';
            return nameA.localeCompare(nameB);
        });
        
        var html = '';
        sorted.forEach(function(ast, index) {
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
                    '<td class="row-number">' + (index + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(astId) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '<td>' + ids.length + '</td>' +
                    '<td class="ids-display">' + idsDisplay + '</td>' +
                    '</tr>';
        });
        
        $asterismsBody.html(html);
        $asterismsBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'asterism', $row.data('pattern-name'), 
                $row.data('pattern-id'), $row.data('search-name'));
        });
    }

    /**
     * Render zodiac signs table.
     * @param {Object} zodiac - Zodiac data object
     */
    function renderZodiacTable(zodiac) {
        if (!$zodiacBody) return;
        var names = (zodiac && zodiac.names) ? zodiac.names : [];
        updateCounter($zodiacCount, names.length);
        
        if (names.length === 0) {
            $zodiacBody.html('<tr><td colspan="6" class="no-data">' + 
                _tr("No zodiac data available") + '</td></tr>');
            return;
        }
        
        var html = '';
        names.forEach(function(sign, index) {
            var nativeName = sign.native || '-';
            var englishName = sign.english || '-';
            var pronounce = sign.pronounce || '-';
            var symbol = sign.symbol || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="zodiac" ' +
                    'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
                    'data-search-name="' + escapeHtml(englishName || nativeName) + '">' +
                    '<td class="row-number">' + (index + 1) + '</td>' +
                    '<td>' + escapeHtml(symbol) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '</tr>';
        });
        
        $zodiacBody.html(html);
        $zodiacBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'zodiac', $row.data('pattern-name'), null, $row.data('search-name'));
        });
    }

    /**
     * Render lunar mansions table.
     * @param {Object} lunar - Lunar system data object
     */
    function renderLunarTable(lunar) {
        if (!$lunarBody) return;
        var names = (lunar && lunar.names) ? lunar.names : [];
        updateCounter($lunarCount, names.length);
        
        if (names.length === 0) {
            $lunarBody.html('<tr><td colspan="6" class="no-data">' + 
                _tr("No lunar mansions data available") + '</td></tr>');
            return;
        }
        
        var html = '';
        names.forEach(function(mansion, index) {
            var nativeName = mansion.native || '-';
            var englishName = mansion.english || '-';
            var pronounce = mansion.pronounce || '-';
            var symbol = mansion.symbol || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="lunar" ' +
                    'data-pattern-name="' + escapeHtml(nativeName) + '" ' +
                    'data-search-name="' + escapeHtml(englishName || nativeName) + '">' +
                    '<td class="row-number">' + (index + 1) + '</td>' +
                    '<td>' + escapeHtml(symbol) + '</td>' +
                    '<td><span class="native-name">' + escapeHtml(nativeName) + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(pronounce) + '</td>' +
                    '</tr>';
        });
        
        $lunarBody.html(html);
        $lunarBody.off('click', '.clickable-row').on('click', '.clickable-row', function() {
            var $row = $(this);
            handleRowToggle($row, 'lunar', $row.data('pattern-name'), null, $row.data('search-name'));
        });
    }

    /**
     * Render notable stars table.
     * @param {Object} commonNames - Common names object
     */
    function renderStarsTable(commonNames) {
        if (!$starsBody) return;
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
        
        updateCounter($starsCount, starEntries.length);
        
        if (starEntries.length === 0) {
            $starsBody.html('<tr><td colspan="8" class="no-data">' + 
                _tr("No star name data available") + '</td></tr>');
            return;
        }
        
        starEntries.sort(function(a, b) { 
            return (a.native || a.english).localeCompare(b.native || b.english); 
        });
        
        var html = '';
        starEntries.forEach(function(star, index) {
            var parsed = parseIdType(star.id);
            var englishName = star.english || '-';
            var translation = englishName !== '-' ? _tr(englishName) : '-';
            
            html += '<tr class="clickable-row" data-pattern-type="star" ' +
                    'data-pattern-id="' + escapeHtml(star.id) + '" ' +
                    'data-pattern-name="' + escapeHtml(star.native || star.english) + '">' +
                    '<td class="row-number">' + (index + 1) + '</td>' +
                    '<td class="id-cell">' + escapeHtml(star.id) + '</td>' +
                    '<td><span class="id-type-badge ' + parsed.type + '">' + 
                    escapeHtml(parsed.display) + '</span></td>' +
                    '<td><span class="native-name">' + escapeHtml(star.native || '-') + '</span></td>' +
                    '<td>' + escapeHtml(englishName) + '</td>' +
                    '<td>' + escapeHtml(translation) + '</td>' +
                    '<td class="pronounce">' + escapeHtml(star.pronounce || '-') + '</td>' +
                    '<td class="ipa-value">' + escapeHtml(star.ipa || '-') + '</td>' +
                    '</tr>';
        });
        
        $starsBody.html(html);
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
     * Export table data to CSV.
     * @param {string} type - Data type to export
     */
    function exportToCSV(type) {
        if (!currentCultureData) return;
        var filename = (currentCultureId || 'culture') + '_' + type + '.csv';
        var headers = [], data = [];
        
        if (type === 'constellations') {
            headers = ['#', 'ID', 'Native Name', 'English Name', 'Translation', 'Pronounce', 'Stars', 'IDs'];
            var constellations = currentCultureData.constellations || [];
            constellations.sort(function(a, b) {
                var nameA = (a.common_name && a.common_name.native) || 
                            (a.common_name && a.common_name.english) || a.id || '';
                var nameB = (b.common_name && b.common_name.native) || 
                            (b.common_name && b.common_name.english) || b.id || '';
                return nameA.localeCompare(nameB);
            });
            constellations.forEach(function(con, index) {
                var ids = extractIdsFromLines(con.lines);
                var englishName = (con.common_name && con.common_name.english) || '';
                data.push([
                    index + 1, 
                    con.id || '', 
                    (con.common_name && con.common_name.native) || '',
                    englishName,
                    englishName ? _tr(englishName) : '',
                    (con.common_name && con.common_name.pronounce) || '',
                    ids.length, 
                    ids.join(', ')
                ]);
            });
        } else if (type === 'asterisms') {
            headers = ['#', 'ID', 'Native Name', 'English Name', 'Translation', 'Pronounce', 'Stars', 'IDs'];
            var asterisms = (currentCultureData.asterisms || []).filter(function(a) { 
                return !a.is_ray_helper; 
            });
            asterisms.sort(function(a, b) {
                var nameA = (a.common_name && a.common_name.native) || 
                            (a.common_name && a.common_name.english) || a.id || '';
                var nameB = (b.common_name && b.common_name.native) || 
                            (b.common_name && b.common_name.english) || b.id || '';
                return nameA.localeCompare(nameB);
            });
            asterisms.forEach(function(ast, index) {
                var ids = extractIdsFromLines(ast.lines);
                var englishName = (ast.common_name && ast.common_name.english) || '';
                data.push([
                    index + 1, 
                    ast.id || '', 
                    (ast.common_name && ast.common_name.native) || '',
                    englishName,
                    englishName ? _tr(englishName) : '',
                    (ast.common_name && ast.common_name.pronounce) || '',
                    ids.length, 
                    ids.join(', ')
                ]);
            });
        } else if (type === 'stars') {
            headers = ['#', 'Original ID', 'Type', 'Native Name', 'English Name', 'Translation', 'Pronounce', 'IPA'];
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
        
        if (!data.length) return;
        var csvContent = '\ufeff' + headers.join(',') + '\n' + data.map(function(row) {
            return row.map(function(cell) { 
                return '"' + String(cell).replace(/"/g, '""') + '"'; 
            }).join(',');
        }).join('\n');
        
        var blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url; 
        a.download = filename; 
        a.click();
        URL.revokeObjectURL(url);
    }

    /**
     * Export table data to JSON.
     * @param {string} type - Data type to export
     */
    function exportToJSON(type) {
        if (!currentCultureData) return;
        var exportData = { 
            culture: currentCultureId, 
            cultureName: currentCultureName, 
            type: type, 
            timestamp: new Date().toISOString() 
        };
        switch(type) {
            case 'constellations': 
                exportData.data = currentCultureData.constellations || []; 
                break;
            case 'asterisms': 
                exportData.data = (currentCultureData.asterisms || []).filter(function(a) { 
                    return !a.is_ray_helper; 
                }); 
                break;
            case 'zodiac': 
                exportData.data = currentCultureData.zodiac || {}; 
                break;
            case 'lunar': 
                exportData.data = currentCultureData.lunar_system || {}; 
                break;
            case 'stars': 
                exportData.data = currentCultureData.common_names || {}; 
                break;
        }
        var jsonContent = JSON.stringify(exportData, null, 2);
        var blob = new Blob(['\ufeff' + jsonContent], { type: 'application/json' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url; 
        a.download = (currentCultureId || 'culture') + '_' + type + '.json'; 
        a.click();
        URL.revokeObjectURL(url);
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