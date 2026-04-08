/* ========================================================================
 * skyculture.js - Sky Culture Manager with Multiple Data Sections
 * ========================================================================
 * 
 * This module provides a complete UI for managing sky cultures and their
 * associated astronomical data including:
 * - Constellations (traditional star groupings)
 * - Asterisms (star patterns within constellations)
 * - Zodiac Signs (ecliptic constellations)
 * - Lunar Mansions (traditional lunar stations)
 * - Notable Stars (named stars with cultural significance)
 * 
 * Version: 2.2.0
 * Date: 2026-04-07
 * 
 * ======================================================================== */

define(["jquery", "api/properties", "api/remotecontrol", "api/actions", "ui/stellarium-utils"], 
    function($, propApi, rc, actions, stelUtils) {
    "use strict";

    // =====================================================================
    // SECTION 1: CONSTANTS
    // =====================================================================

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

    // Cache for parsed data
    var CULTURE_DATA_CACHE = {};

    // =====================================================================
    // SECTION 2: PRIVATE VARIABLES
    // =====================================================================

    var $cultureContainer = null;
    var $constellationsContainer = null;
    var $asterismsContainer = null;
    var $zodiacContainer = null;
    var $lunarContainer = null;
    var $starsContainer = null;
    var $infoFrame = null;

    var currentCultureId = null;
    var availableCultures = [];
    var cultureData = null;
    var selectedPattern = null;
    var isInitialized = false;
    var isUpdating = false;

    // =====================================================================
    // SECTION 3: DATA EXTRACTION FUNCTIONS
    // =====================================================================

    /**
     * Extracts constellations from index.json data
     */
    function extractConstellations(data) {
        var patterns = [];
        
        if (data.constellations && Array.isArray(data.constellations)) {
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
        }
        
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        return patterns;
    }

    /**
     * Extracts asterisms from index.json data (skip ray helpers)
     */
    function extractAsterisms(data) {
        var patterns = [];
        
        if (data.asterisms && Array.isArray(data.asterisms)) {
            for (var i = 0; i < data.asterisms.length; i++) {
                var aster = data.asterisms[i];
                
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
        }
        
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        return patterns;
    }

    /**
     * Extracts zodiac signs from index.json data
     */
    function extractZodiac(data) {
        var patterns = [];
        
        if (data.zodiac && data.zodiac.names && Array.isArray(data.zodiac.names)) {
            for (var i = 0; i < data.zodiac.names.length; i++) {
                var sign = data.zodiac.names[i];
                var signName = sign.english || sign.native || ("Sign " + (i + 1));
                
                patterns.push({
                    id: "zodiac_" + (i + 1),
                    name: signName,
                    symbol: sign.symbol || "",
                    native: sign.native || "",
                    type: "zodiac"
                });
            }
        }
        
        return patterns;
    }

    /**
     * Extracts lunar mansions from index.json data
     */
    function extractLunarMansions(data) {
        var patterns = [];
        
        if (data.lunar_system && data.lunar_system.names && Array.isArray(data.lunar_system.names)) {
            for (var i = 0; i < data.lunar_system.names.length; i++) {
                var mansion = data.lunar_system.names[i];
                var mansionName = mansion.english || mansion.native || ("Mansion " + (i + 1));
                
                patterns.push({
                    id: "lunar_" + (i + 1),
                    name: mansionName,
                    symbol: mansion.symbol || "",
                    native: mansion.native || "",
                    type: "lunar"
                });
            }
        }
        
        return patterns;
    }

    /**
     * Extracts notable star names from index.json data
     */
    function extractStarNames(data) {
        var patterns = [];
        
        if (data.common_names && typeof data.common_names === 'object') {
            var processedNames = {};
            
            for (var starId in data.common_names) {
                if (data.common_names.hasOwnProperty(starId)) {
                    var names = data.common_names[starId];
                    
                    for (var i = 0; i < names.length; i++) {
                        var nameObj = names[i];
                        var starName = nameObj.english || nameObj.native;
                        
                        if (starName && starName.length > 0 && !processedNames[starName]) {
                            processedNames[starName] = true;
                            patterns.push({
                                id: starId,
                                name: starName,
                                type: "star"
                            });
                        }
                    }
                }
            }
        }
        
        // Limit to first 100 star names to avoid overwhelming the UI
        patterns = patterns.slice(0, 7000);
        
        patterns.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        return patterns;
    }

    // =====================================================================
    // SECTION 4: RENDERING FUNCTIONS
    // =====================================================================

    function renderButtons(container, patterns, containerName, onClickHandler) {
        if (!container) return;
        
        container.empty();
        
        if (!patterns || patterns.length === 0) {
            var placeholderText = "";
            if (containerName === "constellations") placeholderText = _tr("No constellations available");
            else if (containerName === "asterisms") placeholderText = _tr("No asterisms available");
            else if (containerName === "zodiac") placeholderText = _tr("No zodiac data available");
            else if (containerName === "lunar") placeholderText = _tr("No lunar mansions data available");
            else if (containerName === "stars") placeholderText = _tr("No star name data available");
            else placeholderText = _tr("No data available");
            
            container.html('<div class="loading-placeholder">' + placeholderText + '</div>');
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
						escapeHtml(pattern.id) + '">' + escapeHtml(pattern.name) + '</button>';
        }
        
        buttonsHtml += '</div>';
        container.html(buttonsHtml);
        
        // Attach click handlers
        container.find(".pattern-btn").on("click", function() {
            var patternName = $(this).data("pattern-name");
            var patternType = $(this).data("pattern-type");
            if (patternName && onClickHandler) {
                onClickHandler(patternName, patternType);
            }
        });
    }

    function renderConstellations(patterns) {
        renderButtons($constellationsContainer, patterns, "constellations", function(patternName, patternType) {
            stelUtils.toggleConstellationHighlight(patternName);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }

    function renderAsterisms(patterns) {
        renderButtons($asterismsContainer, patterns, "asterisms", function(patternName, patternType) {
            stelUtils.centerOnObject(patternName, 2);
            selectedPattern = patternName;
            updateAllButtonStates();
            stelUtils.showNotification(_tr("Centering on: ") + patternName);
        });
    }

    function renderZodiac(patterns) {
        renderButtons($zodiacContainer, patterns, "zodiac", function(patternName, patternType) {
            stelUtils.centerOnObject(patternName, 2);
            selectedPattern = patternName;
            updateAllButtonStates();
            stelUtils.showNotification(_tr("Centering on: ") + patternName);
        });
    }

    function renderLunarMansions(patterns) {
        renderButtons($lunarContainer, patterns, "lunar", function(patternName, patternType) {
            stelUtils.centerOnObject(patternName, 2);
            selectedPattern = patternName;
            updateAllButtonStates();
            stelUtils.showNotification(_tr("Centering on: ") + patternName);
        });
    }

    function renderStarNames(patterns) {
        renderButtons($starsContainer, patterns, "stars", function(patternName, patternType) {
            stelUtils.searchAndCenter(patternName, 2);
            selectedPattern = patternName;
            updateAllButtonStates();
        });
    }

    function updateAllButtonStates() {
        // Update all containers' button states
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

    function showHidePanels(data) {
        // Show/hide panels based on available data
        var hasAsterisms = (data.asterisms && data.asterisms.length > 0);
        var hasZodiac = (data.zodiac && data.zodiac.names && data.zodiac.names.length > 0);
        var hasLunar = (data.lunar_system && data.lunar_system.names && data.lunar_system.names.length > 0);
        var hasStars = (data.common_names && Object.keys(data.common_names).length > 0);
        
        if ($asterismsContainer && $asterismsContainer.length) {
            $asterismsContainer.closest('.asterisms-panel').toggle(hasAsterisms);
        }
        if ($zodiacContainer && $zodiacContainer.length) {
            $zodiacContainer.closest('.zodiac-panel').toggle(hasZodiac);
        }
        if ($lunarContainer && $lunarContainer.length) {
            $lunarContainer.closest('.lunar-panel').toggle(hasLunar);
        }
        if ($starsContainer && $starsContainer.length) {
            $starsContainer.closest('.stars-panel').toggle(hasStars);
        }
    }

    // =====================================================================
    // SECTION 5: DATA LOADING
    // =====================================================================

    function loadCultureData(callback) {
        if (!currentCultureId) {
            if (callback) callback(false);
            return;
        }

        // Check cache
        if (CULTURE_DATA_CACHE[currentCultureId]) {
            cultureData = CULTURE_DATA_CACHE[currentCultureId];
            processCultureData();
            if (callback) callback(true);
            return;
        }
        
        var indexPath = "/api/view/skyculturedescription/index.json";
        
        $.ajax({
            url: indexPath,
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                cultureData = data;
                CULTURE_DATA_CACHE[currentCultureId] = data;
                processCultureData();
                if (callback) callback(true);
            },
            error: function(xhr, status, errorThrown) {
                console.warn("[SkyCulture] Error loading index.json: " + errorThrown);
                useDefaultData();
                if (callback) callback(false);
            }
        });
    }

    function processCultureData() {
        if (!cultureData) return;
        
        // Extract and render all data types
        var constellations = extractConstellations(cultureData);
        var asterisms = extractAsterisms(cultureData);
        var zodiac = extractZodiac(cultureData);
        var lunar = extractLunarMansions(cultureData);
        var stars = extractStarNames(cultureData);
        
        renderConstellations(constellations);
        renderAsterisms(asterisms);
        renderZodiac(zodiac);
        renderLunarMansions(lunar);
        renderStarNames(stars);
        
        // Show/hide panels based on available data
        showHidePanels(cultureData);
        
        console.log("[SkyCulture] Processed culture data:", {
            constellations: constellations.length,
            asterisms: asterisms.length,
            zodiac: zodiac.length,
            lunar: lunar.length,
            stars: stars.length
        });
    }

    function useDefaultData() {
        // Use default IAU constellations
        var defaultPatterns = [];
        for (var i = 0; i < DEFAULT_IAU_CONSTELLATIONS.length; i++) {
            defaultPatterns.push({
                id: DEFAULT_IAU_CONSTELLATIONS[i].toLowerCase(),
                name: DEFAULT_IAU_CONSTELLATIONS[i],
                type: "constellation"
            });
        }
        renderConstellations(defaultPatterns);
        
        // Clear other panels
        if ($asterismsContainer) renderAsterisms([]);
        if ($zodiacContainer) renderZodiac([]);
        if ($lunarContainer) renderLunarMansions([]);
        if ($starsContainer) renderStarNames([]);
    }

    // =====================================================================
    // SECTION 6: SKY CULTURE MANAGEMENT
    // =====================================================================

    function updateInfoFrame(cultureId) {
        if ($infoFrame && $infoFrame.length) {
            var url = "/api/view/skyculturedescription/";
            $infoFrame.attr("src", url);
        }
    }

    function updateCultureButtonStates() {
        if (!$cultureContainer) return;
        
        $cultureContainer.find(".skyculture-btn").removeClass("active");
        $cultureContainer.find(".skyculture-btn").each(function() {
            if ($(this).data("culture-id") === currentCultureId) {
                $(this).addClass("active");
            }
        });
    }

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
						escapeHtml(culture.id) + '">' + 
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
                console.log("[SkyCulture] Loaded " + availableCultures.length + " sky cultures");
                
                if (propApi) {
                    var current = propApi.getStelProp("StelSkyCultureMgr.currentSkyCultureID");
                    currentCultureId = current || (availableCultures.length > 0 ? availableCultures[0].id : null);
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
        loadCultureData();
    }

    function refresh() {
        for (var key in CULTURE_DATA_CACHE) {
            delete CULTURE_DATA_CACHE[key];
        }
        loadSkyCultures();
    }

    // =====================================================================
    // SECTION 7: EVENT HANDLERS
    // =====================================================================

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
                loadCultureData();
            }
        });
    }

    // =====================================================================
    // SECTION 8: INITIALIZATION
    // =====================================================================

    function init(cultureContainerSelector, patternsContainers, iframeSelector) {
        if (isInitialized) {
            console.warn("[SkyCulture] Module already initialized");
            return;
        }
        
        $cultureContainer = $(cultureContainerSelector || "#skyculture-buttons-container");
        $constellationsContainer = $(patternsContainers.constellations || "#constellations-buttons-container");
        $asterismsContainer = $(patternsContainers.asterisms || "#asterisms-buttons-container");
        $zodiacContainer = $(patternsContainers.zodiac || "#zodiac-buttons-container");
        $lunarContainer = $(patternsContainers.lunar || "#lunar-buttons-container");
        $starsContainer = $(patternsContainers.stars || "#stars-buttons-container");
        $infoFrame = $(iframeSelector || "#vo_skycultureinfo");
        
        if (!$cultureContainer || !$cultureContainer.length) {
            console.error("[SkyCulture] Culture container not found");
            return;
        }
        
        setupPropertyListener();
        loadSkyCultures();
        
        isInitialized = true;
        console.log("[SkyCulture] Module initialized successfully");
    }

    // =====================================================================
    // SECTION 9: UTILITIES
    // =====================================================================

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

    function _tr(text) {
        if (typeof window.tr === 'function') return window.tr.apply(window, arguments);
        if (typeof rc !== 'undefined' && rc && typeof rc.tr === 'function') return rc.tr.apply(rc, arguments);
        return text;
    }

    // =====================================================================
    // SECTION 10: PUBLIC API
    // =====================================================================

    return {
        init: init,
        refresh: refresh,
        setCulture: setSkyCulture,
        getCurrentCulture: function() { return currentCultureId; },
        getAvailableCultures: function() { return availableCultures.slice(); }
    };
});