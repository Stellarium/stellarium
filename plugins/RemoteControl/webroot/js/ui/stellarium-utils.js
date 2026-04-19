/* ========================================================================
 * stellarium-utils.js - Shared Stellarium Utilities
 * ========================================================================
 * 
 * This module provides shared utility functions for Stellarium Remote Control.
 * Used by gpcontroller.js, skyculture.js, and skyculture-stats.js to avoid
 * code duplication and ensure consistent behavior.
 * 
 * Features:
 * - FOV management with smooth transitions
 * - Object navigation (constellations, asterisms, zodiac signs, stars, lunar mansions)
 * - Constellation isolation with state preservation
 * - Automatic selection clearing to prevent view conflicts
 * - View state save/restore (FOV and display settings)
 * - Event-based synchronization for button state updates
 * - Universal object navigation with intelligent ID parsing
 * - Solar system object special handling (direct focus bypassing search)
 * 
 * Technical Implementation:
 * - Uses ConstellationMgr.isolateSelected + flagConstellationPick for isolation
 * - Preserves previous display states (lines, boundaries, labels, art, FOV)
 * - Restores states when isolation is cleared
 * - Tracks currently isolated constellation globally
 * - Clears any existing object selection before new navigation
 * - Emits 'objectSelected' event for bidirectional UI synchronization
 * - Parses object IDs (HIP, NGC, NAME, etc.) for accurate Stellarium lookup
 * - Bypasses /api/objects/find for solar system objects to avoid moon confusion
 * 
 * Events Emitted:
 * - objectSelected: Triggered when an object is selected/navigated to
 *   Data: { name: string, id: string, type: string }
 * 
 * @module stellarium-utils
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/viewcontrol
 * @requires api/actions
 * @requires api/properties
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-16
 * @license GPLv2+
 * @version 3.0.0
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/viewcontrol", "api/actions", "api/properties"],
    function($, rc, viewcontrol, actions, propApi) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    /**
     * Saved constellation display states before isolation.
     * Used to restore original view when isolation is cleared.
     * @type {Object}
     */
    var savedConstellationStates = {
        linesDisplayed: null,
        boundariesDisplayed: null,
        labelsDisplayed: null,
        artDisplayed: null,
        fov: null
    };
    
    /**
     * Track current isolated constellation.
     * Used for toggle logic and state queries.
     * @type {string|null}
     */
    var currentIsolatedConstellation = null;
    
    /**
     * Flag indicating if state has been saved.
     * Prevents saving multiple times during same isolation session.
     * @type {boolean}
     */
    var hasSavedState = false;

    /**
     * Translation function from remotecontrol module.
     * Provides consistent i18n across all utilities.
     * @type {Function}
     */
    var _tr = rc.tr;

    /**
     * Solar system object name mapping.
     * Maps 'NAME X' format from index.json to standard English names
     * used by Stellarium's search/focus API.
     * @type {Object.<string, string>}
     */
    var SOLAR_SYSTEM_DEFAULT_NAMES = {
        'NAME Sun': 'Sun',
        'NAME Moon': 'Moon',
        'NAME Mercury': 'Mercury',
        'NAME Venus': 'Venus',
        'NAME Earth': 'Earth',
        'NAME Mars': 'Mars',
        'NAME Jupiter': 'Jupiter',
        'NAME Saturn': 'Saturn',
        'NAME Uranus': 'Uranus',
        'NAME Neptune': 'Neptune',
        'NAME Pluto': 'Pluto'
    };

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    /**
     * Shows a temporary notification message that auto-dismisses after 2 seconds.
     * Used for user feedback during navigation actions.
     * 
     * @param {string} message - Message to display
     */
    function showNotification(message) {
        var $notification = $(
            '<div class="notification-message" style="position:fixed;top:20px;right:20px;' +
            'padding:10px 15px;background:linear-gradient(#6B6E70, #3A3C3E);color:#fff;' +
            'border-radius:5px;z-index:9999;box-shadow:0 2px 10px rgba(0,0,0,0.3);">' + 
            message + '</div>'
        );
        $("body").append($notification);
        setTimeout(function() {
            $notification.fadeOut(function() { $notification.remove(); });
        }, 2000);
    }

    /**
     * Clears any currently selected object to prevent view conflicts.
     * This is called before navigating to a new object to ensure
     * that the view doesn't snap back to a previously selected object.
     */
    function clearSelection() {
        rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
    }

    /**
     * Emits an event when an object is selected for bidirectional sync.
     * Allows UI components (buttons, tables) to stay synchronized with
     * the current Stellarium selection state.
     * 
     * @param {string} name - Display name of the object
     * @param {string} id - Unique identifier (HIP ID, constellation ID, etc.)
     * @param {string} type - Object type: "constellation", "asterism", "zodiac", "star", "lunar"
     */
    function emitObjectSelected(name, id, type) {
        $(document).trigger("objectSelected", { name: name, id: id, type: type });
        console.log("[StelUtils] Emitted objectSelected event:", { name, id, type });
    }

    // =====================================================================
    // FOV MANAGEMENT
    // =====================================================================

    /**
     * Gets the current FOV from Stellarium.
     * 
     * @returns {number} Current field of view in degrees
     */
    function getCurrentFov() {
        if (viewcontrol && typeof viewcontrol.getFOV === 'function') {
            return viewcontrol.getFOV();
        }
        return propApi.getStelProp("StelCore.fov") || 60;
    }

    /**
     * Sets the FOV with smooth transition.
     * 
     * @param {number} fov - Target field of view in degrees (0.001389 to 360)
     * @param {number} duration - Transition duration in seconds (default: 2)
     */
    function setFov(fov, duration) {
        duration = duration || 2;
        var targetFov = Math.max(0.001389, Math.min(360, fov));
        
        rc.postCmd("/api/scripts/direct", {
            code: "StelMovementMgr.zoomTo(" + targetFov + ", " + duration + ")",
            useIncludes: false
        });
    }

    /**
     * Resets zoom to default 60°.
     * 
     * @param {number} duration - Transition duration (default: 2)
     */
    function resetZoom(duration) {
        setFov(60, duration || 2);
    }

    // =====================================================================
    // OBJECT NAVIGATION
    // =====================================================================

    /**
     * Centers view on a specific object.
     * Clears any existing selection first to prevent view conflicts.
     * 
     * @param {string} objectName - Name of the object to center on
     * @param {number} duration - Transition duration in seconds (default: 2)
     * @param {string} objectId - Unique identifier for the object (optional)
     * @param {string} objectType - Type of object (optional)
     * @param {function} callback - Optional callback after centering
     */
    function centerOnObject(objectName, duration, objectId, objectType, callback) {
        duration = duration || 2;
        clearSelection();
        
        setTimeout(function() {
            rc.postCmd("/api/scripts/direct", {
                code: "core.moveToObject(\"" + objectName + "\", " + duration + ")",
                useIncludes: false
            }, null, function() {
                if (objectId || objectName) {
                    emitObjectSelected(objectName, objectId || objectName, objectType || "object");
                }
                if (callback) callback();
            });
        }, 50);
    }

    /**
     * Centers on object and zooms to specified FOV.
     * Clears any existing selection first.
     * 
     * @param {string} objectName - Name of the object
     * @param {number} zoomFov - Target zoom level (default: 60)
     * @param {number} duration - Transition duration (default: 2)
     * @param {string} objectId - Unique identifier for the object (optional)
     * @param {string} objectType - Type of object (optional)
     * @param {function} callback - Optional callback after centering and zooming
     */
    function centerAndZoom(objectName, zoomFov, duration, objectId, objectType, callback) {
        duration = duration || 2;
        zoomFov = zoomFov || 60;
        clearSelection();
        
        setTimeout(function() {
            rc.postCmd("/api/scripts/direct", {
                code: "core.moveToObject(\"" + objectName + "\", " + duration + "); StelMovementMgr.zoomTo(" + zoomFov + ", " + duration + ")",
                useIncludes: false
            }, null, function() {
                if (objectId || objectName) {
                    emitObjectSelected(objectName, objectId || objectName, objectType || "object");
                }
                if (callback) callback();
            });
        }, 50);
    }

    /**
     * Parse and normalize an object ID for search.
     * Handles various ID formats from index.json common_names section:
     * - NAME Sun, NAME Jupiter (solar system objects)
     * - HIP 12345 (Hipparcos catalog)
     * - M 31 (Messier objects)
     * - NGC 1234, NGC1234 (New General Catalogue)
     * - IC 1234, IC1234 (Index Catalogue)
     * - DSO:xxx (Deep Sky Object)
     * - Bare numbers (interpreted as HIP IDs)
     * 
     * Also extracts native names from cultureData when available for
     * better user feedback in notifications.
     * 
     * @param {string} id - Raw ID from culture file
     * @param {Object} cultureData - Culture data from index.json (optional)
     * @returns {Object} Normalized search term and display info with properties:
     *                   {searchTerm, type, display, nativeName?, defaultName?}
     */
    function parseObjectId(id, cultureData) {
        if (typeof id !== 'string') {
            return { searchTerm: String(id), type: 'other', display: String(id) };
        }
        
        // Handle NAME prefix (solar system objects)
        if (id.startsWith('NAME ')) {
            var defaultName = SOLAR_SYSTEM_DEFAULT_NAMES[id] || id.substring(5).trim();
            var searchTerm = defaultName;
            var nativeName = null;
            
            if (cultureData && cultureData.common_names && cultureData.common_names[id]) {
                var entries = cultureData.common_names[id];
                if (Array.isArray(entries) && entries.length > 0) {
                    nativeName = entries[0].native || null;
                }
            }
            
            return { 
                searchTerm: searchTerm,
                type: 'solar', 
                display: id,
                nativeName: nativeName,
                defaultName: defaultName
            };
        }
        
        // Handle HIP prefix
        if (id.startsWith('HIP ')) {
            var hipNum = id.substring(4).trim();
            var searchTerm = 'HIP ' + hipNum;
            var nativeName = null;
            
            if (cultureData && cultureData.common_names && cultureData.common_names[id]) {
                var entries = cultureData.common_names[id];
                if (Array.isArray(entries) && entries.length > 0) {
                    nativeName = entries[0].native || null;
                }
            }
            
            return { searchTerm: searchTerm, type: 'hip', display: id, nativeName: nativeName };
        }
        
        // Handle Messier objects
        if (id.startsWith('M ')) return { searchTerm: id, type: 'messier', display: id };
        
        // Handle NGC objects (with and without space)
        if (id.startsWith('NGC ')) return { searchTerm: id, type: 'ngc', display: id };
        if (id.startsWith('NGC') && /^NGC\d+/.test(id)) {
            var num = id.substring(3);
            return { searchTerm: 'NGC ' + num, type: 'ngc', display: id };
        }
        
        // Handle IC objects (with and without space)
        if (id.startsWith('IC ')) return { searchTerm: id, type: 'ic', display: id };
        if (id.startsWith('IC') && /^IC\d+/.test(id)) {
            var num = id.substring(2);
            return { searchTerm: 'IC ' + num, type: 'ic', display: id };
        }
        
        // Handle DSO prefix
        if (id.startsWith('DSO:')) {
            var dsoId = id.substring(4);
            return { searchTerm: dsoId, type: 'dso', display: dsoId };
        }
        
        // Handle bare numbers (interpret as HIP IDs)
        if (/^\d+$/.test(id)) {
            var searchTerm = 'HIP ' + id;
            var nativeName = null;
            var hipKey = 'HIP ' + id;
            
            if (cultureData && cultureData.common_names && cultureData.common_names[hipKey]) {
                var entries = cultureData.common_names[hipKey];
                if (Array.isArray(entries) && entries.length > 0) {
                    nativeName = entries[0].native || null;
                }
            }
            
            return { searchTerm: searchTerm, type: 'hip', display: hipKey, nativeName: nativeName };
        }
        
        return { searchTerm: id, type: 'other', display: id };
    }

    /**
     * Universal object navigation function.
     * Handles navigation to any celestial object by its ID or name.
     * 
     * Features:
     * - Intelligent ID parsing for various catalog formats
     * - Special handling for solar system objects (direct focus bypasses search)
     * - Two-step search-then-focus for reliable targeting of stars/DSOs
     * - Smooth FOV transition after centering
     * - Emits objectSelected event for UI synchronization
     * - Displays user-friendly notifications with native names when available
     * 
     * @param {string} objectId - Object ID (HIP, M, NGC, NAME, etc.) or name
     * @param {number} zoomFov - Zoom level in degrees (default: 15°)
     * @param {string} objectType - Type of object (star, constellation, etc.)
     * @param {Object} cultureData - Culture data from index.json for native name lookup
     * @param {function} callback - Optional callback after navigation, receives (success, actualName)
     */
    function goToObject(objectId, zoomFov, objectType, cultureData, callback) {
        // Handle optional parameters with flexible argument order
        if (typeof zoomFov === 'function') {
            callback = zoomFov;
            zoomFov = 15;
            objectType = 'object';
            cultureData = null;
        } else if (typeof objectType === 'function') {
            callback = objectType;
            objectType = 'object';
            cultureData = null;
        } else if (typeof cultureData === 'function') {
            callback = cultureData;
            cultureData = null;
        }
        
        zoomFov = zoomFov || 15;
        objectType = objectType || 'object';
        
        // Parse the ID to get proper search term
        var parsed = parseObjectId(objectId, cultureData);
        var searchTerm = parsed.searchTerm;
        
        console.log("[StelUtils] Navigating to object:", objectId, "-> parsed:", parsed);
        
        // Clear any existing selection
        clearSelection();
        
        // ===== SPECIAL HANDLING FOR SOLAR SYSTEM OBJECTS =====
        // For solar system objects (NAME Sun, NAME Jupiter, etc.), 
        // use direct focus with the default English name.
        // This bypasses the /api/objects/find search which may return 
        // moons instead of planets (e.g., "Jupiter" search returns Galilean moons).
        if (parsed.type === 'solar') {
            var directName = parsed.defaultName || searchTerm;
            console.log("[StelUtils] Solar system object, using direct focus:", directName);
            
            rc.postCmd("/api/main/focus", { target: directName, mode: "center" }, null, function() {
                setTimeout(function() {
                    setFov(zoomFov, 2);
                    var displayName = parsed.nativeName || directName;
                    showNotification(_tr("Centering on: ") + displayName);
                    emitObjectSelected(directName, objectId, objectType);
                    if (callback) callback(true, directName);
                }, 300);
            });
            return;
        }
        
        // ===== NORMAL SEARCH FOR OTHER OBJECTS =====
        // Step 1: Search to get the correct object name (resolves aliases)
        $.ajax({
            url: "/api/objects/find",
            data: { str: searchTerm },
            dataType: "json",
            success: function(data) {
                if (data && data.length > 0) {
                    var correctName = data[0];
                    console.log("[StelUtils] Selected object:", correctName);
                    
                    // Step 2: Focus on the correct name
                    rc.postCmd("/api/main/focus", { target: correctName, mode: "center" }, null, function() {
                        setTimeout(function() {
                            setFov(zoomFov, 2);
                            var displayName = parsed.nativeName || correctName;
                            showNotification(_tr("Centering on: ") + displayName);
                            emitObjectSelected(correctName, objectId, objectType);
                            if (callback) callback(true, correctName);
                        }, 300);
                    });
                } else {
                    // Try direct focus without search (for objects like Sun, Moon, planets)
                    console.log("[StelUtils] Search failed, trying direct focus:", searchTerm);
                    
                    rc.postCmd("/api/main/focus", { target: searchTerm, mode: "center" }, null, function() {
                        setTimeout(function() {
                            setFov(zoomFov, 2);
                            var displayName = parsed.nativeName || searchTerm;
                            showNotification(_tr("Centering on: ") + displayName);
                            emitObjectSelected(searchTerm, objectId, objectType);
                            if (callback) callback(true, searchTerm);
                        }, 300);
                    });
                }
            },
            error: function(xhr, status, errorThrown) {
                console.error("[StelUtils] Error searching for object:", errorThrown);
                
                // Try direct focus as fallback
                rc.postCmd("/api/main/focus", { target: searchTerm, mode: "center" }, null, function() {
                    setTimeout(function() {
                        setFov(zoomFov, 2);
                        var displayName = parsed.nativeName || searchTerm;
                        showNotification(_tr("Centering on: ") + displayName);
                        emitObjectSelected(searchTerm, objectId, objectType);
                        if (callback) callback(true, searchTerm);
                    }, 300);
                });
            }
        });
    }

    /**
     * Navigate to a star by its HIP ID or name.
     * Convenience wrapper around goToObject with star-specific defaults.
     * 
     * @param {string} starId - HIP ID or star name
     * @param {number} zoomFov - Zoom level (default: 15°)
     * @param {Object} cultureData - Culture data for native name lookup
     * @param {function} callback - Optional callback after navigation
     */
    function goToStar(starId, zoomFov, cultureData, callback) {
        if (typeof zoomFov === 'function') {
            callback = zoomFov;
            zoomFov = 15;
            cultureData = null;
        } else if (typeof cultureData === 'function') {
            callback = cultureData;
            cultureData = null;
        }
        goToObject(starId, zoomFov, 'star', cultureData, callback);
    }

    /**
     * Navigates to a zodiac sign (constellation).
     * Clears any existing selection first.
     * Emits objectSelected event for bidirectional sync.
     * 
     * @param {string} signName - Name of the zodiac sign
     * @param {string} signId - Unique identifier for the sign (optional)
     * @param {function} callback - Optional callback
     */
    function goToZodiacSign(signName, signId, callback) {
        clearSelection();
        setTimeout(function() {
            centerAndZoom(signName, 40, 2, signId || signName, "zodiac", callback);
            showNotification(_tr("Centering on zodiac sign: ") + signName);
        }, 50);
    }

    /**
     * Navigates to an asterism.
     * Clears any existing selection first.
     * Emits objectSelected event for bidirectional sync.
     * 
     * @param {string} asterismName - Name of the asterism
     * @param {string} asterismId - Unique identifier for the asterism (optional)
     * @param {function} callback - Optional callback
     */
    function goToAsterism(asterismName, asterismId, callback) {
        clearSelection();
        setTimeout(function() {
            centerAndZoom(asterismName, 30, 2, asterismId || asterismName, "asterism", callback);
            showNotification(_tr("Centering on asterism: ") + asterismName);
        }, 50);
    }

    /**
     * Navigates to a lunar mansion.
     * Clears any existing selection first.
     * Emits objectSelected event for bidirectional sync.
     * 
     * @param {string} mansionName - Name of the lunar mansion
     * @param {string} mansionId - Unique identifier for the mansion (optional)
     * @param {function} callback - Optional callback
     */
    function goToLunarMansion(mansionName, mansionId, callback) {
        clearSelection();
        setTimeout(function() {
            centerAndZoom(mansionName, 30, 2, mansionId || mansionName, "lunar", callback);
            showNotification(_tr("Centering on lunar mansion: ") + mansionName);
        }, 50);
    }

    // =====================================================================
    // CONSTELLATION DISPLAY STATE MANAGEMENT
    // =====================================================================

    /**
     * Saves the current constellation display settings.
     * Called before isolating a constellation to enable restoration later.
     */
    function saveConstellationDisplayStates() {
        savedConstellationStates.linesDisplayed = propApi.getStelProp("ConstellationMgr.linesDisplayed");
        savedConstellationStates.boundariesDisplayed = propApi.getStelProp("ConstellationMgr.boundariesDisplayed");
        savedConstellationStates.labelsDisplayed = propApi.getStelProp("ConstellationMgr.namesDisplayed");
        savedConstellationStates.artDisplayed = propApi.getStelProp("ConstellationMgr.artDisplayed");
        savedConstellationStates.fov = getCurrentFov();
        hasSavedState = true;
        console.log("[StelUtils] Saved constellation states");
    }

    /**
     * Restores the previous constellation display settings.
     * Called after isolation is cleared.
     */
    function restoreConstellationDisplayStates() {
        if (!hasSavedState) return;
        
        var currentLines = propApi.getStelProp("ConstellationMgr.linesDisplayed");
        if (currentLines !== savedConstellationStates.linesDisplayed) {
            if (savedConstellationStates.linesDisplayed) {
                actions.execute("actionShow_Constellation_Lines");
            } else if (currentLines === true) {
                actions.execute("actionShow_Constellation_Lines");
            }
        }
        
        var currentBoundaries = propApi.getStelProp("ConstellationMgr.boundariesDisplayed");
        if (currentBoundaries !== savedConstellationStates.boundariesDisplayed) {
            if (savedConstellationStates.boundariesDisplayed) {
                actions.execute("actionShow_Constellation_Boundaries");
            } else if (currentBoundaries === true) {
                actions.execute("actionShow_Constellation_Boundaries");
            }
        }
        
        var currentLabels = propApi.getStelProp("ConstellationMgr.namesDisplayed");
        if (currentLabels !== savedConstellationStates.labelsDisplayed) {
            if (savedConstellationStates.labelsDisplayed) {
                actions.execute("actionShow_Constellation_Labels");
            } else if (currentLabels === true) {
                actions.execute("actionShow_Constellation_Labels");
            }
        }
        
        var currentArt = propApi.getStelProp("ConstellationMgr.artDisplayed");
        if (currentArt !== savedConstellationStates.artDisplayed) {
            if (savedConstellationStates.artDisplayed) {
                actions.execute("actionShow_Constellation_Art");
            } else if (currentArt === true) {
                actions.execute("actionShow_Constellation_Art");
            }
        }
        
        if (savedConstellationStates.fov !== null && savedConstellationStates.fov !== getCurrentFov()) {
            setFov(savedConstellationStates.fov, 2);
        }
        
        hasSavedState = false;
        console.log("[StelUtils] Restored constellation states");
    }

    /**
     * Enables all constellation display elements.
     * Called before isolating a constellation to ensure visibility.
     */
    function enableAllConstellationDisplays() {
        if (propApi.getStelProp("ConstellationMgr.linesDisplayed") === false) {
            actions.execute("actionShow_Constellation_Lines");
        }
        if (propApi.getStelProp("ConstellationMgr.boundariesDisplayed") === false) {
            actions.execute("actionShow_Constellation_Boundaries");
        }
        if (propApi.getStelProp("ConstellationMgr.namesDisplayed") === false) {
            actions.execute("actionShow_Constellation_Labels");
        }
        if (propApi.getStelProp("ConstellationMgr.artDisplayed") === false) {
            actions.execute("actionShow_Constellation_Art");
        }
    }

    // =====================================================================
    // CONSTELLATION HIGHLIGHT/ISOLATION (Core Logic)
    // =====================================================================

    /**
     * Toggles constellation highlighting/isolation.
     * 
     * First press: clears any existing selection, saves current state,
     *              enables all constellation displays, isolates and centers
     *              on the constellation with 60° FOV.
     * Second press: restores previous state and clears isolation.
     * Emits objectSelected event for bidirectional sync.
     * 
     * @param {string} constellationName - Name of the constellation
     * @returns {boolean} True if constellation was highlighted, false if cleared
     */
    function toggleConstellationHighlight(constellationName) {
        var searchTerm = constellationName.replace(/_/g, ' ');
        searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
        
        var isCurrentlyHighlighted = (currentIsolatedConstellation === searchTerm);
        
        if (isCurrentlyHighlighted) {
            console.log("[StelUtils] Toggle OFF - Clearing highlight:", searchTerm);
            
            propApi.setStelProp("ConstellationMgr.isolateSelected", false);
            propApi.setStelProp("ConstellationMgr.flagConstellationPick", false);
            clearSelection();
            restoreConstellationDisplayStates();
            currentIsolatedConstellation = null;
            
            showNotification(_tr("Cleared highlight: ") + searchTerm);
            emitObjectSelected("", "", "none");
            return false;
        } else {
            console.log("[StelUtils] Toggle ON - Isolating constellation:", searchTerm);
            
            clearSelection();
            
            if (!hasSavedState) {
                saveConstellationDisplayStates();
            }
            
            enableAllConstellationDisplays();
            
            setTimeout(function() {
                propApi.setStelProp("ConstellationMgr.flagConstellationPick", true);
                propApi.setStelProp("ConstellationMgr.isolateSelected", true);
                rc.postCmd("/api/main/focus", { target: searchTerm, mode: "mark" }, null, function() {});
                currentIsolatedConstellation = searchTerm;
                
                setTimeout(function() {
                    rc.postCmd("/api/scripts/direct", {
                        code: "core.moveToObject(\"" + searchTerm + "\", 2); StelMovementMgr.zoomTo(60, 2);",
                        useIncludes: false
                    });
                    emitObjectSelected(searchTerm, searchTerm, "constellation");
                    showNotification(_tr("Isolating constellation: ") + searchTerm);
                }, 150);
            }, 100);
            
            return true;
        }
    }

    /**
     * Clears all constellation highlighting and restores previous state.
     * Emits event to clear button states.
     */
    function clearConstellationHighlight() {
        console.log("[StelUtils] Clearing all constellation highlights");
        
        propApi.setStelProp("ConstellationMgr.isolateSelected", false);
        propApi.setStelProp("ConstellationMgr.flagConstellationPick", false);
        clearSelection();
        restoreConstellationDisplayStates();
        currentIsolatedConstellation = null;
        
        emitObjectSelected("", "", "none");
        showNotification(_tr("All constellations visible"));
    }

    /**
     * Gets the currently highlighted constellation name.
     * 
     * @returns {string|null} Name of highlighted constellation or null
     */
    function getHighlightedConstellation() {
        return currentIsolatedConstellation;
    }

    /**
     * Checks if a specific constellation is currently highlighted.
     * 
     * @param {string} constellationName - Name to check
     * @returns {boolean} True if highlighted
     */
    function isConstellationHighlighted(constellationName) {
        var searchTerm = constellationName.replace(/_/g, ' ');
        searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
        return currentIsolatedConstellation === searchTerm;
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initializes the utilities module.
     */
    function init() {
        console.log("[StelUtils] Initialized shared utilities v3.0.0");
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    var publ = {
        init: init,
        
        // FOV Management
        getCurrentFov: getCurrentFov,
        setFov: setFov,
        resetZoom: resetZoom,
        
        // Object Navigation
        centerOnObject: centerOnObject,
        centerAndZoom: centerAndZoom,
        clearSelection: clearSelection,
        
        // Universal object navigation
        goToObject: goToObject,
        goToStar: goToStar,
        goToZodiacSign: goToZodiacSign,
        goToAsterism: goToAsterism,
        goToLunarMansion: goToLunarMansion,
        parseObjectId: parseObjectId,
        
        // Constellation Display State Management
        saveConstellationDisplayStates: saveConstellationDisplayStates,
        restoreConstellationDisplayStates: restoreConstellationDisplayStates,
        enableAllConstellationDisplays: enableAllConstellationDisplays,
        
        // Constellation Highlight/Isolation
        toggleConstellationHighlight: toggleConstellationHighlight,
        clearConstellationHighlight: clearConstellationHighlight,
        getHighlightedConstellation: getHighlightedConstellation,
        isConstellationHighlighted: isConstellationHighlighted,
        
        // Event emission
        emitObjectSelected: emitObjectSelected,
        
        // Utilities
        showNotification: showNotification,
        
        // Event handling support for other modules
        on: function(event, callback) { $(publ).on(event, callback); },
        off: function(event, callback) { $(publ).off(event, callback); }
    };

    return publ;
});