/**
 * Scripts UI Module for Stellarium Remote Control
 * 
 * @module ui/scripts
 * @description Implements the script management panel and controls for running,
 * stopping scripts, and resetting the view to a natural sky appearance.
 * 
 * Key Features:
 * - Dynamic loading of script list from Stellarium's script manager.
 * - Run selected script and stop current script functionality.
 * - **Reset View button** to restore natural sky appearance with smooth transitions.
 * - **Visual indicator (pulsing border) on info frame when any script is running.**
 * - **Automatic highlighting of active script in the selection list.**
 * - **Button state management: Run and Reset View disabled while script is running.**
 * - Integration with `stelssc` CSS class for direct script buttons elsewhere in the UI.
 * - Synchronization with Stellarium's native script system via StelProperties.
 * 
 * Technical Architecture:
 * - Depends on `api/scripts` to fetch script list and execute commands.
 * - Uses `api/properties` to monitor `StelScriptMgr.runningScriptId`.
 * - Uses `api/actions` for StelAction execution.
 * - Uses `api/time` for time-related operations.
 * - Uses `api/viewcontrol` for FOV adjustments.
 * - Listens to `activeScriptChanged` event from `api/scripts`.
 * - Applies CSS classes for visual feedback (`script-running`, `active-script`).
 * 
 * @requires jquery
 * @requires api/scripts
 * @requires api/remotecontrol
 * @requires api/viewcontrol
 * @requires api/properties
 * @requires api/actions
 * @requires api/time
 * 
 * @license GPLv2+
 */

define(["jquery", "api/scripts", "api/remotecontrol", "api/viewcontrol", "api/properties", "api/actions", "api/time"], 
    function($, scriptApi, rc, viewcontrol, propApi, actionApi, timeApi) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    /** @type {jQuery} Span displaying active script name */
    var $activescript;
    
    /** @type {jQuery} Run selected script button */
    var $bt_runscript;
    
    /** @type {jQuery} Stop current script button */
    var $bt_stopscript;
    
    /** @type {jQuery} Reset View button - restores natural sky appearance */
    var $bt_resetview;
    
    /** @type {jQuery} Select element containing script list */
    var $scriptlist;
    
    /** @type {jQuery} Iframe displaying script description */
    var $scriptinfo;

    // =====================================================================
    // SCRIPT LIST POPULATION
    // =====================================================================

    /**
     * Fills the script selection list with data from the server.
     * Called automatically when the script list is loaded from the API.
     * 
     * @param {Array} data - Array of script names from the server
     */
    function fillScriptList(data) {
        //document must be ready
        $(function() {
            $scriptlist.empty();

            //sort it and insert
            $.each(data.sort(), function(idx, elem) {
                $("<option/>").text(elem).val(elem).appendTo($scriptlist);
            });
        });
    }

    // =====================================================================
    // RESET TO NATURAL SKY - FULL RESET FUNCTIONALITY
    // =====================================================================
    
    /**
     * Resets the view to a natural sky appearance with smooth transitions.
     * 
     * This function performs a comprehensive reset of the Stellarium view,
     * restoring all display settings to their ideal state for naked-eye observing.
     * It uses sequential script execution with JavaScript delays to ensure
     * reliable movement and zoom operations without script conflicts.
     * 
     * Actions performed (in order):
     * 1. Time reset: Set to current time and normal rate (1.0x)
     * 2. Cleanup: Clear environment, delete all labels and markers, clear selection
     * 3. Movement: Disable tracking and equatorial mount
     * 4. Landscape: Enable landscape, atmosphere, and stars
     * 5. Solar System: Enable planets and planet labels
     * 6. Deep Sky: Enable nebulae and Milky Way
     * 7. Constellations: Lines ON, Labels ON, Boundaries OFF, Art OFF, Isolation OFF
     * 8. Grids: All grids OFF, Cardinal Points ON
     * 9. Interface: Night mode OFF, GUI visible, disk viewport OFF
     * 10. Projection: Reset to stereographic (default)
     * 11. View: South (azimuth 180°), altitude 40°, FOV 90°
     * 
     * The function uses a sequential approach with 200ms delays between
     * script parts to avoid overwhelming the server and prevent the
     * "script already running" error.
     * 
     * @returns {void}
     */
    function resetToNaturalSky() {
        console.log("[Scripts] Resetting to natural sky with full time/GUI/display reset...");
        
        // Split the script into parts and send sequentially with JavaScript delays
        var parts = [
            // Part 1: Reset time to now and normal rate
            "core.setDate(\"now\"); core.setTimeRate(1);",
            
            // Part 2: Basic cleanup
            "core.clear(\"natural\"); LabelMgr.deleteAllLabels(); MarkerMgr.deleteAllMarkers(); core.clearSelection();",
            
            // Part 3: Movement settings
            "StelMovementMgr.setFlagTracking(false); StelMovementMgr.setEquatorialMount(false); core.goHome();",
            
            // Part 4: Landscape and stars
            "LandscapeMgr.setFlagLandscape(true); LandscapeMgr.setFlagAtmosphere(true); StarMgr.setStars(true);",
            
            // Part 5: Solar system
            "SolarSystem.setPlanets(true); SolarSystem.setPlanetsLabels(true);",
            
            // Part 6: Nebulae and Milky Way
            "NebulaMgr.setNebulas(true); MilkyWay.setMilkyWay(true);",
            
            // Part 7: Constellations
            "ConstellationMgr.setLines(true); ConstellationMgr.setLabels(true); ConstellationMgr.setBoundaries(false); ConstellationMgr.setArt(false); ConstellationMgr.setIsolateSelected(false); ConstellationMgr.setFlagConstellationPick(false);",
            
            // Part 8: Grids
            "GridLinesMgr.setEquatorialGrid(false); GridLinesMgr.setAzimuthalGrid(false); GridLinesMgr.setCardinalPoints(true);",
            
            // Part 9: Interface and night mode
            "core.setNightMode(false); core.setGuiVisible(true); core.setDiskViewport(false);",
            
            // Part 10: Projection mode (reset to default stereographic)
            "core.setProjectionMode(\"stereographic\");",
            
            // Part 11: View position (South, altitude 40 degrees)
            "core.moveToAltAzi(40.0, 180.0, 3.0);"
        ];
        
        /**
         * Recursively sends script parts with delays to avoid conflicts.
         * 
         * @param {number} index - Current index in the parts array
         */
        function sendPart(index) {
            if (index < parts.length) {
                scriptApi.runDirectScript(parts[index], false);
                // Delay 200ms between each part
                setTimeout(function() { sendPart(index + 1); }, 200);
            } else {
                // After all parts are sent, wait 3.5 seconds then send zoom
                setTimeout(function() {
                    scriptApi.runDirectScript("StelMovementMgr.zoomTo(90.0, 2.0);", false);
                    console.log("[Scripts] ✓ Full reset completed successfully.");
                }, 3500);
            }
        }
        
        // Start sending parts
        sendPart(0);
    }

    // =====================================================================
    // VISUAL FEEDBACK FOR ACTIVE SCRIPT
    // =====================================================================

    /**
     * Updates the visual indicators and button states when a script 
     * becomes active or stops.
     * 
     * Button State Rules:
     * - When script is running: Run and Reset View are DISABLED, Stop is ENABLED
     * - When no script: Run and Reset View are ENABLED, Stop is DISABLED
     * 
     * Visual Indicators:
     * - Pulsing silver border on info iframe when script is running
     * - Active script highlighted in the selection list
     * - Active script name displayed below the list
     * 
     * @param {string} scriptName - Name of the active script, or empty string if none
     */
    function updateActiveScriptIndicators(scriptName) {
        var hasActiveScript = !!scriptName;
        
        // 1. Pulsing border effect on info frame
        if ($scriptinfo && $scriptinfo.length) {
            if (hasActiveScript) {
                $scriptinfo.addClass('script-running');
            } else {
                $scriptinfo.removeClass('script-running');
            }
        }
        
        // 2. Highlight active script in the selection list
        if ($scriptlist && $scriptlist.length) {
            // Remove existing highlight from all options
            $scriptlist.find('option').removeClass('active-script');
            
            // Add highlight to matching option
            if (hasActiveScript) {
                $scriptlist.find('option').each(function() {
                    if ($(this).val() === scriptName) {
                        $(this).addClass('active-script');
                        return false; // break loop
                    }
                });
            }
        }
        
        // 3. Update the active script text display
        if ($activescript && $activescript.length) {
            $activescript.text(scriptName || rc.tr("-none-"));
        }
        
        // 4. Manage button states based on script running status
        // Run button: disabled when script is running
        if ($bt_runscript && $bt_runscript.length) {
            $bt_runscript.prop('disabled', hasActiveScript);
        }
        
        // Stop button: enabled ONLY when script is running
        if ($bt_stopscript && $bt_stopscript.length) {
            $bt_stopscript.prop('disabled', !hasActiveScript);
        }
        
        // Reset View button: disabled when script is running (avoid view conflicts)
        if ($bt_resetview && $bt_resetview.length) {
            $bt_resetview.prop('disabled', hasActiveScript);
        }
    }

    // =====================================================================
    // INITIALIZATION OF SCRIPT BUTTONS
    // =====================================================================

    /**
     * Initializes automatic script buttons with the 'stelssc' class.
     * These buttons execute script code directly without using the script list.
     * 
     * Buttons with the 'stelssc' class can have their script code in the 'value'
     * attribute, and optionally set data-useIncludes="true" to enable include
     * processing.
     */
    function initScriptButtons() {
        $("button.stelssc, input[type='button'].stelssc").click(function() {
            var code = this.value;
            if (!code) {
                console.error("No code defined for this script button");
                return;
            }

            var useIncludes = false;
            if ($(this).data("useIncludes"))
                useIncludes = true;

            scriptApi.runDirectScript(code, useIncludes);
        });
    }

    /**
     * Initializes all UI controls and event handlers for the scripts panel.
     * Sets up the script list, run/stop/reset buttons, and info iframe.
     */
    function initControls() {
        $scriptlist = $("#scriptlist");
        $bt_runscript = $("#bt_runscript");
        $bt_stopscript = $("#bt_stopscript");
        $bt_resetview = $("#bt_script_resetview");
        $activescript = $("#activescript");
        $scriptinfo = $("#scriptinfo");

        /**
         * Runs the currently selected script from the list.
         */
        var runscriptfn = function() {
            var selection = $scriptlist.val();
            if (selection) {
                scriptApi.runScript(selection);
            }
        };

        // Update info iframe when selection changes
        $scriptlist.change(function() {
            var selection = $scriptlist.children("option").filter(":selected").val();

            $scriptinfo.attr('src', "/api/scripts/info?html=true&id=" + encodeURIComponent(selection));

            // Enable run button only if no script is currently running
            var hasActiveScript = !!($activescript.text() && $activescript.text() !== rc.tr("-none-"));
            $bt_runscript.prop('disabled', hasActiveScript);
            
            console.log("selected: " + selection);
        }).dblclick(runscriptfn);

        $bt_runscript.click(runscriptfn);
        $bt_stopscript.click(scriptApi.stopScript);
        
        // Reset View button - restores natural sky appearance
        if ($bt_resetview && $bt_resetview.length) {
            $bt_resetview.click(resetToNaturalSky);
        }

        initScriptButtons();
        
        // Set initial button states
        $bt_stopscript.prop('disabled', true);
        
        console.log("[Scripts] Controls initialized with working reset function");
    }

    // =====================================================================
    // EVENT HANDLERS
    // =====================================================================

    /**
     * Handles activeScriptChanged event from scriptApi.
     * Updates UI to reflect the currently running script.
     */
    $(scriptApi).on("activeScriptChanged", function(evt, script) {
        updateActiveScriptIndicators(script || "");
    });

    /**
     * Listens for changes to the running script ID via StelProperties.
     * This catches scripts started from anywhere (Actions menu, main GUI, etc.)
     */
    $(propApi).on("stelPropertyChanged:StelScriptMgr.runningScriptId", function(evt, data) {
        var scriptName = data.value || "";
        updateActiveScriptIndicators(scriptName);
        
        // Also notify scriptApi to keep it in sync
        $(scriptApi).trigger("activeScriptChanged", scriptName);
    });

    // =====================================================================
    // INITIALIZATION ON DOM READY
    // =====================================================================

    $(function() {
        initControls();
    });

    // Queue script list loading as soon as this script is loaded instead of ready event
    scriptApi.loadScriptList(fillScriptList);

    // =====================================================================
    // EXPOSE FUNCTIONS FOR MANUAL TESTING (OPTIONAL)
    // =====================================================================
    
    /**
     * Expose key functions to the browser console for manual testing.
     * Can be accessed via window.stellariumScripts.resetToNaturalSky()
     */
    window.stellariumScripts = {
        /**
         * Manually trigger the natural sky reset function.
         * @function
         */
        resetToNaturalSky: resetToNaturalSky
    };

    // =====================================================================
    // RETURN PUBLIC API
    // =====================================================================
    
    return {
        /**
         * Public API: Reset the view to natural sky appearance.
         * @function
         */
        resetToNaturalSky: resetToNaturalSky
    };

});