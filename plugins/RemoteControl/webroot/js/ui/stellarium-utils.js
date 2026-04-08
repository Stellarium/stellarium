/* ========================================================================
 * stellarium-utils.js - Shared Stellarium Utilities
 * ========================================================================
 * 
 * This module provides shared utility functions for Stellarium Remote Control.
 * Used by both gpcontroller.js and skyculture.js to avoid code duplication.
 * 
 * Features:
 * - FOV management with smooth transitions
 * - Object navigation (constellations, asterisms, zodiac signs, stars)
 * - Constellation isolation with state preservation
 * - Automatic selection clearing to prevent view conflicts
 * - View state save/restore (FOV and display settings)
 * 
 * Technical Implementation:
 * - Uses ConstellationMgr.isolateSelected + flagConstellationPick for isolation
 * - Preserves previous display states (lines, boundaries, labels, art, FOV)
 * - Restores states when isolation is cleared
 * - Tracks currently isolated constellation globally
 * - Clears any existing object selection before new navigation
 * 
 * @module stellarium-utils
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/viewcontrol
 * @requires api/actions
 * @requires api/properties
 * @requires api/search
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-04-08
 * @license GPLv2+
 * @version 2.0.0
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/viewcontrol", "api/actions", "api/properties", "api/search"],
    function($, rc, viewcontrol, actions, propApi, searchApi) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    // Saved constellation display states before isolation
    var savedConstellationStates = {
        linesDisplayed: null,
        boundariesDisplayed: null,
        labelsDisplayed: null,
        artDisplayed: null,
        fov: null
    };
    
    // Track current isolated constellation
    var currentIsolatedConstellation = null;
    
    // Flag indicating if state has been saved
    var hasSavedState = false;

    // Default FOV for different object types
    var DEFAULT_FOV = {
        constellation: 60,
        asterism: 30,
        zodiac: 40,
        star: 15
    };

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    /**
     * Simple translation helper with fallback
     * @param {string} text - Text to translate
     * @returns {string} Translated text
     */
    function _tr(text) {
        if (typeof window.tr === 'function') {
            return window.tr.apply(window, arguments);
        }
        if (typeof rc !== 'undefined' && rc && typeof rc.tr === 'function') {
            return rc.tr.apply(rc, arguments);
        }
        return text;
    }

    /**
     * Shows a temporary notification message
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
            $notification.fadeOut(function() {
                $notification.remove();
            });
        }, 2000);
    }

    /**
     * Formats constellation/object name for display
     * @param {string} name - Raw name (e.g., "ursa_major")
     * @returns {string} Formatted name (e.g., "Ursa Major")
     */
    function formatConstellationName(name) {
        if (!name) return "";
        
        var specialNames = {
            "ursa_major": "Ursa Major",
            "ursa_minor": "Ursa Minor",
            "canis_major": "Canis Major",
            "canis_minor": "Canis Minor",
            "corona_borealis": "Corona Borealis",
            "corona_australis": "Corona Australis",
            "coma_berenices": "Coma Berenices",
            "piscis_austrinus": "Piscis Austrinus",
            "triangulum_australe": "Triangulum Australe",
            "canes_venatici": "Canes Venatici",
            "boötes": "Boötes"
        };
        
        if (specialNames[name.toLowerCase()]) return specialNames[name.toLowerCase()];
        
        var formatted = name.replace(/_/g, ' ');
        formatted = formatted.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
        
        return formatted;
    }

    /**
     * Clears any currently selected object to prevent view conflicts
     * This is called before navigating to a new object to ensure
     * that the view doesn't snap back to a previously selected object
     */
    function clearSelection() {
        rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
    }

    // =====================================================================
    // FOV MANAGEMENT
    // =====================================================================

    /**
     * Gets the current FOV from Stellarium
     * @returns {number} Current field of view in degrees
     */
    function getCurrentFov() {
        if (viewcontrol && typeof viewcontrol.getFOV === 'function') {
            return viewcontrol.getFOV();
        }
        return propApi.getStelProp("StelCore.fov") || 60;
    }

    /**
     * Sets the FOV with smooth transition
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
     * Resets zoom to default 60°
     * @param {number} duration - Transition duration (default: 2)
     */
    function resetZoom(duration) {
        setFov(60, duration || 2);
    }

    // =====================================================================
    // OBJECT NAVIGATION (with automatic selection clearing)
    // =====================================================================

    /**
     * Centers view on a specific object
     * Clears any existing selection first to prevent view conflicts
     * 
     * @param {string} objectName - Name of the object to center on
     * @param {number} duration - Transition duration in seconds (default: 2)
     * @param {function} callback - Optional callback after centering
     */
    function centerOnObject(objectName, duration, callback) {
        duration = duration || 2;
        
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        // Small delay to ensure selection is cleared
        setTimeout(function() {
            rc.postCmd("/api/scripts/direct", {
                code: "core.moveToObject(\"" + objectName + "\", " + duration + ")",
                useIncludes: false
            }, null, callback);
        }, 50);
    }

    /**
     * Centers on object and zooms to specified FOV
     * Clears any existing selection first
     * 
     * @param {string} objectName - Name of the object
     * @param {number} zoomFov - Target zoom level (default: 60)
     * @param {number} duration - Transition duration (default: 2)
     * @param {function} callback - Optional callback after centering and zooming
     */
    function centerAndZoom(objectName, zoomFov, duration, callback) {
        duration = duration || 2;
        zoomFov = zoomFov || 60;
        
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            rc.postCmd("/api/scripts/direct", {
                code: "core.moveToObject(\"" + objectName + "\", " + duration + "); StelMovementMgr.zoomTo(" + zoomFov + ", " + duration + ")",
                useIncludes: false
            }, null, callback);
        }, 50);
    }

    /**
     * Searches for a star by its common name and centers on it
     * Uses the search API for better star name resolution
     * Clears any existing selection first
     * 
     * @param {string} starName - The common name of the star
     * @param {number} duration - Transition duration (default: 2)
     * @param {function} callback - Optional callback after centering
     */
    function searchAndCenter(starName, duration, callback) {
        duration = duration || 2;
        
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            if (searchApi && typeof searchApi.selectObjectByName === 'function') {
                var mode = $("#select_SelectionMode").val() || "center";
                searchApi.selectObjectByName(starName, mode);
            } else {
                rc.postCmd("/api/main/focus", { target: starName, mode: "center" }, null, callback);
            }
        }, 50);
    }

    /**
     * Navigates to an object with appropriate FOV based on type
     * 
     * @param {string} objectName - Name of the object
     * @param {string} objectType - Type: "constellation", "asterism", "zodiac", "star"
     * @param {function} callback - Optional callback after navigation
     */
    function navigateToObject(objectName, objectType, callback) {
        var zoomFov = DEFAULT_FOV[objectType] || 60;
        var duration = 2;
        
        if (objectType === "constellation") {
            // For constellations, use the highlight/isolation system
            var result = toggleConstellationHighlight(objectName);
            if (callback) callback(result);
        } else {
            // For other types, just center and zoom
            centerAndZoom(objectName, zoomFov, duration, callback);
        }
    }

    // =====================================================================
    // CONSTELLATION DISPLAY STATE MANAGEMENT
    // =====================================================================

    /**
     * Saves the current constellation display settings
     * Called before isolating a constellation
     */
    function saveConstellationDisplayStates() {
        savedConstellationStates.linesDisplayed = propApi.getStelProp("ConstellationMgr.linesDisplayed");
        savedConstellationStates.boundariesDisplayed = propApi.getStelProp("ConstellationMgr.boundariesDisplayed");
        savedConstellationStates.labelsDisplayed = propApi.getStelProp("ConstellationMgr.namesDisplayed");
        savedConstellationStates.artDisplayed = propApi.getStelProp("ConstellationMgr.artDisplayed");
        savedConstellationStates.fov = getCurrentFov();
        hasSavedState = true;
        
        console.log("[StelUtils] Saved constellation states:", savedConstellationStates);
    }

    /**
     * Restores the previous constellation display settings
     * Called after isolation is cleared
     */
    function restoreConstellationDisplayStates() {
        if (!hasSavedState) {
            console.log("[StelUtils] No saved state to restore");
            return;
        }
        
        // Restore lines
        var currentLines = propApi.getStelProp("ConstellationMgr.linesDisplayed");
        if (currentLines !== savedConstellationStates.linesDisplayed) {
            if (savedConstellationStates.linesDisplayed) {
                actions.execute("actionShow_Constellation_Lines");
            } else {
                var current = propApi.getStelProp("ConstellationMgr.linesDisplayed");
                if (current === true) {
                    actions.execute("actionShow_Constellation_Lines");
                }
            }
        }
        
        // Restore boundaries
        var currentBoundaries = propApi.getStelProp("ConstellationMgr.boundariesDisplayed");
        if (currentBoundaries !== savedConstellationStates.boundariesDisplayed) {
            if (savedConstellationStates.boundariesDisplayed) {
                actions.execute("actionShow_Constellation_Boundaries");
            } else {
                var current = propApi.getStelProp("ConstellationMgr.boundariesDisplayed");
                if (current === true) {
                    actions.execute("actionShow_Constellation_Boundaries");
                }
            }
        }
        
        // Restore labels
        var currentLabels = propApi.getStelProp("ConstellationMgr.namesDisplayed");
        if (currentLabels !== savedConstellationStates.labelsDisplayed) {
            if (savedConstellationStates.labelsDisplayed) {
                actions.execute("actionShow_Constellation_Labels");
            } else {
                var current = propApi.getStelProp("ConstellationMgr.namesDisplayed");
                if (current === true) {
                    actions.execute("actionShow_Constellation_Labels");
                }
            }
        }
        
        // Restore art
        var currentArt = propApi.getStelProp("ConstellationMgr.artDisplayed");
        if (currentArt !== savedConstellationStates.artDisplayed) {
            if (savedConstellationStates.artDisplayed) {
                actions.execute("actionShow_Constellation_Art");
            } else {
                var current = propApi.getStelProp("ConstellationMgr.artDisplayed");
                if (current === true) {
                    actions.execute("actionShow_Constellation_Art");
                }
            }
        }
        
        // Restore FOV
        if (savedConstellationStates.fov !== null && savedConstellationStates.fov !== getCurrentFov()) {
            setFov(savedConstellationStates.fov, 2);
        }
        
        hasSavedState = false;
        console.log("[StelUtils] Restored constellation states");
    }

    /**
     * Enables all constellation display elements
     * Called before isolating a constellation to ensure visibility
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
     * 
     * @param {string} constellationName - Name of the constellation
     * @returns {boolean} True if constellation was highlighted, false if cleared
     */
    function toggleConstellationHighlight(constellationName) {
        var searchTerm = constellationName.replace(/_/g, ' ');
        searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
        
        var isCurrentlyHighlighted = (currentIsolatedConstellation === searchTerm);
        
        if (isCurrentlyHighlighted) {
            // ===== TOGGLE OFF: Clear isolation and restore previous state =====
            console.log("[StelUtils] Toggle OFF - Clearing highlight:", searchTerm);
            
            // Disable isolation modes
            propApi.setStelProp("ConstellationMgr.isolateSelected", false);
            propApi.setStelProp("ConstellationMgr.flagConstellationPick", false);
            
            // Clear selection
            clearSelection();
            
            // Restore previous display states
            restoreConstellationDisplayStates();
            
            // Clear tracker
            currentIsolatedConstellation = null;
            
            showNotification(_tr("Cleared highlight: ") + searchTerm);
            return false;
            
        } else {
            // ===== TOGGLE ON: Isolate and highlight constellation =====
            console.log("[StelUtils] Toggle ON - Isolating constellation:", searchTerm);
            
            // Clear any existing selection first (important for stars)
            clearSelection();
            
            // Save current display states (if not already saved)
            if (!hasSavedState) {
                saveConstellationDisplayStates();
            }
            
            // Enable all constellation display elements
            enableAllConstellationDisplays();
            
            setTimeout(function() {
                // Enable isolation modes
                propApi.setStelProp("ConstellationMgr.flagConstellationPick", true);
                propApi.setStelProp("ConstellationMgr.isolateSelected", true);
                
                // Select the constellation
                rc.postCmd("/api/main/focus", { target: searchTerm, mode: "mark" }, null, function() {});
                
                // Update tracker
                currentIsolatedConstellation = searchTerm;
                
                // Center and zoom to 60° FOV
                setTimeout(function() {
                    rc.postCmd("/api/scripts/direct", {
                        code: "core.moveToObject(\"" + searchTerm + "\", 2); StelMovementMgr.zoomTo(60, 2);",
                        useIncludes: false
                    });
                    
                    showNotification(_tr("Isolating constellation: ") + searchTerm);
                }, 150);
            }, 100);
            
            return true;
        }
    }

    /**
     * Clears all constellation highlighting and restores previous state
     */
    function clearConstellationHighlight() {
        console.log("[StelUtils] Clearing all constellation highlights and restoring state");
        
        propApi.setStelProp("ConstellationMgr.isolateSelected", false);
        propApi.setStelProp("ConstellationMgr.flagConstellationPick", false);
        clearSelection();
        restoreConstellationDisplayStates();
        currentIsolatedConstellation = null;
        
        showNotification(_tr("All constellations visible"));
    }

    /**
     * Gets the currently highlighted constellation name
     * @returns {string|null} Name of highlighted constellation or null
     */
    function getHighlightedConstellation() {
        return currentIsolatedConstellation;
    }

    /**
     * Checks if a specific constellation is currently highlighted
     * @param {string} constellationName - Name to check
     * @returns {boolean} True if highlighted
     */
    function isConstellationHighlighted(constellationName) {
        var searchTerm = constellationName.replace(/_/g, ' ');
        searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
        return currentIsolatedConstellation === searchTerm;
    }

    // =====================================================================
    // STAR NAVIGATION (with selection clearing)
    // =====================================================================

    /**
     * Navigates to a star by its name
     * Clears any existing selection before navigation
     * 
     * @param {string} starName - Name of the star
     * @param {number} zoomFov - Zoom level (default: 15°)
     * @param {function} callback - Optional callback
     */
    function goToStar(starName, zoomFov, callback) {
        zoomFov = zoomFov || 15;
        
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            if (searchApi && typeof searchApi.selectObjectByName === 'function') {
                var mode = $("#select_SelectionMode").val() || "center";
                searchApi.selectObjectByName(starName, mode);
            } else {
                rc.postCmd("/api/main/focus", { target: starName, mode: "center" }, null, callback);
            }
            
            // Zoom in after centering
            setTimeout(function() {
                setFov(zoomFov, 2);
                showNotification(_tr("Centering on star: ") + starName);
            }, 300);
        }, 50);
    }

    /**
     * Navigates to a zodiac sign (constellation)
     * Clears any existing selection first
     * 
     * @param {string} signName - Name of the zodiac sign
     * @param {function} callback - Optional callback
     */
    function goToZodiacSign(signName, callback) {
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            centerAndZoom(signName, 40, 2, callback);
            showNotification(_tr("Centering on zodiac sign: ") + signName);
        }, 50);
    }

    /**
     * Navigates to an asterism
     * Clears any existing selection first
     * 
     * @param {string} asterismName - Name of the asterism
     * @param {function} callback - Optional callback
     */
    function goToAsterism(asterismName, callback) {
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            centerAndZoom(asterismName, 30, 2, callback);
            showNotification(_tr("Centering on asterism: ") + asterismName);
        }, 50);
    }

    /**
     * Navigates to a lunar mansion (typically a star or region)
     * Clears any existing selection first
     * 
     * @param {string} mansionName - Name of the lunar mansion
     * @param {function} callback - Optional callback
     */
    function goToLunarMansion(mansionName, callback) {
        // Clear any existing selection to prevent view conflicts
        clearSelection();
        
        setTimeout(function() {
            centerAndZoom(mansionName, 30, 2, callback);
            showNotification(_tr("Centering on lunar mansion: ") + mansionName);
        }, 50);
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initializes the utilities module
     */
    function init() {
        console.log("[StelUtils] Initialized shared utilities v2.0.0");
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        init: init,
        
        // FOV Management
        getCurrentFov: getCurrentFov,
        setFov: setFov,
        resetZoom: resetZoom,
        
        // Object Navigation (with selection clearing)
        centerOnObject: centerOnObject,
        centerAndZoom: centerAndZoom,
        searchAndCenter: searchAndCenter,
        navigateToObject: navigateToObject,
        clearSelection: clearSelection,
        
        // Type-specific Navigation
        goToStar: goToStar,
        goToZodiacSign: goToZodiacSign,
        goToAsterism: goToAsterism,
        goToLunarMansion: goToLunarMansion,
        
        // Constellation Display State Management
        saveConstellationDisplayStates: saveConstellationDisplayStates,
        restoreConstellationDisplayStates: restoreConstellationDisplayStates,
        enableAllConstellationDisplays: enableAllConstellationDisplays,
        
        // Constellation Highlight/Isolation
        toggleConstellationHighlight: toggleConstellationHighlight,
        clearConstellationHighlight: clearConstellationHighlight,
        getHighlightedConstellation: getHighlightedConstellation,
        isConstellationHighlighted: isConstellationHighlighted,
        
        // Utilities
        formatConstellationName: formatConstellationName,
        showNotification: showNotification
    };
});