/* jshint expr: true */
/**
 * Gamepad Controller Module for Stellarium Remote Control
 * 
 * This module provides comprehensive gamepad support using the W3C Gamepad API.
 * It supports multiple controllers simultaneously with independent profiles,
 * full button customization, calibration, vibration feedback, live input monitoring,
 * and educational presentation features.
 * 
 * Key Features:
 * - W3C Gamepad API compliance (no static database dependencies)
 * - Multi-controller support with independent profiles per device
 * - Persistent device profiles stored by unique device identifier
 * - Live Input Monitor for real-time raw button/axis value display with visual meters
 * - Profile import/export functionality for backup and sharing
 * - Enhanced Scan button with visual feedback animation
 * - Real-time joystick preview with calibration visualization and zoom inversion
 * - Continuous zoom operation with adjustable speed and Y-axis inversion
 * - Vibration/rumble control with intensity slider and test function
 * - Full button remapping with categorized action groups
 * - Device detection via vendor/product ID extraction from browser
 * - Responsive UI with multi-device switching and compact layouts
 * - Server connection awareness with auto-resume on reconnect
 * - Dynamic plugin detection and action integration
 *   (ArchaeoLines, Scenery3d, ExoPlanets, NavStars, MeteorShowers, Quasars, Pulsars)
 * - Educational presentation features:
 *   - View directions (North, South, East, West, Zenith, Nadir)
 *   - Zoom levels (0.1° to 235°)
 *   - Notable stars and deep sky objects
 *   - Constellation isolation and highlighting
 *   - Time jumps and season demonstrations
 * - Vibration feedback on button press with adjustable intensity
 * - Joystick calibration to compensate for hardware drift
 * - Sensitivity, deadzone, and axis inversion controls (including zoom stick inversion)
 * - Live Input Monitor with 4-column compact button grid and visual meter bars for axes
 * - Real-time direction display for left stick movement (North, South, East, West, etc.)
 * 
 * Technical Architecture:
 * - Uses requestAnimationFrame for smooth 60fps polling
 * - localStorage for per-device settings persistence
 * - jQuery UI sliders for intuitive configuration
 * - Canvas-based joystick preview with deadzone visualization and zoom inversion support
 * - Live Input panel with real-time raw value updates and visual feedback
 * - Responsive grid layouts for button customization and live input panels
 * 
 * @module gpcontroller
 * @requires jquery
 * @requires settings
 * @requires api/remotecontrol
 * @requires api/viewcontrol
 * @requires api/actions
 * @requires api/time
 * @requires api/properties
 * @requires api/search
 * @requires jquery-ui
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-03-30
 * @license GPLv2+
 * @version 2.6.0
 */
 
define(["jquery", "settings", "api/remotecontrol", "api/viewcontrol", "api/actions", "api/time", "api/properties", "api/search", "jquery-ui"],
    function($, settings, rc, viewcontrol, actions, timeApi, propApi, searchApi, jqui) {
        "use strict";

        // =====================================================================
        // SECTION 1: CONSTANTS AND DEFAULT MAPPINGS
        // =====================================================================

        /**
         * Simple translation function that provides fallback when tr() is not available.
         * Uses Stellarium's translation system when possible.
         * @param {string} text - The text to translate
         * @param {...*} args - Optional arguments for string replacement
         * @returns {string} The translated or original text
         */
        var _tr = function(text) {
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
        };

        /**
         * Generic button mapping based on the W3C "standard" layout.
         * @constant {Object}
         */
        var GENERIC_MAPPING = {
            name: 'Standard Gamepad',
            vendorId: null,
            productId: null,
            buttons: {
                0: 'A / Cross',
                1: 'B / Circle',
                2: 'X / Square',
                3: 'Y / Triangle',
                4: 'Left Bumper (L1)',
                5: 'Right Bumper (R1)',
                6: 'Left Trigger (L2)',
                7: 'Right Trigger (R2)',
                8: 'Select / Back / Share',
                9: 'Start / Options',
                10: 'Left Stick (L3)',
                11: 'Right Stick (R3)',
                12: 'D-Pad Up',
                13: 'D-Pad Down',
                14: 'D-Pad Left',
                15: 'D-Pad Right',
                16: 'Center / Guide / PS',
                17: 'Touchpad'
            }
        };

        /**
         * Device-specific button name overrides for common controllers.
         * @constant {Object}
         */
        var DEVICE_MAPPINGS = {
            '054c': {
                '05c4': { name: 'Sony PlayStation 4 Controller', vendorName: 'Sony', model: 'CUH-ZCT1E',
                    buttons: { 0: '✕ Cross', 1: '◯ Circle', 2: '□ Square', 3: '△ Triangle', 8: 'Share', 9: 'Options', 16: 'PS', 17: 'Touchpad' } },
                '09cc': { name: 'Sony PlayStation 4 Controller (v2)', vendorName: 'Sony', model: 'CUH-ZCT2',
                    buttons: { 0: '✕ Cross', 1: '◯ Circle', 2: '□ Square', 3: '△ Triangle', 8: 'Share', 9: 'Options', 16: 'PS', 17: 'Touchpad' } },
                '0ce6': { name: 'Sony PlayStation 5 DualSense Controller', vendorName: 'Sony', model: 'DualSense',
                    buttons: { 0: '✕ Cross', 1: '◯ Circle', 2: '□ Square', 3: '△ Triangle', 4: 'L1', 5: 'R1', 6: 'L2', 7: 'R2', 8: 'Create', 9: 'Options', 10: 'L3', 11: 'R3', 12: 'D-Pad Up', 13: 'D-Pad Down', 14: 'D-Pad Left', 15: 'D-Pad Right', 16: 'PS', 17: 'Touchpad' } }
            },
            '045e': {
                '028e': { name: 'Microsoft Xbox 360 Controller', vendorName: 'Microsoft', model: 'Xbox 360',
                    buttons: { 0: 'A', 1: 'B', 2: 'X', 3: 'Y', 4: 'Left Bumper', 5: 'Right Bumper', 6: 'Left Trigger', 7: 'Right Trigger', 8: 'Back', 9: 'Start', 10: 'Left Stick', 11: 'Right Stick', 12: 'D-Pad Up', 13: 'D-Pad Down', 14: 'D-Pad Left', 15: 'D-Pad Right' } },
                '02d1': { name: 'Microsoft Xbox One Controller', vendorName: 'Microsoft', model: 'Xbox One',
                    buttons: { 0: 'A', 1: 'B', 2: 'X', 3: 'Y', 4: 'Left Bumper', 5: 'Right Bumper', 6: 'Left Trigger', 7: 'Right Trigger', 8: 'View', 9: 'Menu', 10: 'Left Stick', 11: 'Right Stick', 12: 'D-Pad Up', 13: 'D-Pad Down', 14: 'D-Pad Left', 15: 'D-Pad Right', 16: 'Xbox' } },
                '0b12': { name: 'Microsoft Xbox Series X|S Controller', vendorName: 'Microsoft', model: 'Xbox Series X|S',
                    buttons: { 0: 'A', 1: 'B', 2: 'X', 3: 'Y', 4: 'Left Bumper', 5: 'Right Bumper', 6: 'Left Trigger', 7: 'Right Trigger', 8: 'Share', 9: 'Menu', 10: 'Left Stick', 11: 'Right Stick', 12: 'D-Pad Up', 13: 'D-Pad Down', 14: 'D-Pad Left', 15: 'D-Pad Right', 16: 'Xbox' } }
            },
            '057e': {
                '2009': { name: 'Nintendo Switch Pro Controller', vendorName: 'Nintendo', model: 'Switch Pro',
                    buttons: { 0: 'B', 1: 'A', 2: 'Y', 3: 'X', 4: 'L', 5: 'R', 6: 'ZL', 7: 'ZR', 8: '-', 9: '+', 10: 'Left Stick', 11: 'Right Stick', 12: 'Home', 13: 'Capture' } },
                '2006': { name: 'Nintendo Joy-Con (Left)', vendorName: 'Nintendo', model: 'Joy-Con L',
                    buttons: { 0: 'SL', 1: 'SR', 2: 'Stick Click', 3: 'Capture', 4: 'Up', 5: 'Down', 6: 'Left', 7: 'Right', 8: 'ZL', 9: 'L' } },
                '2007': { name: 'Nintendo Joy-Con (Right)', vendorName: 'Nintendo', model: 'Joy-Con R',
                    buttons: { 0: 'A', 1: 'X', 2: 'B', 3: 'Y', 4: 'SR', 5: 'SL', 6: 'Stick Click', 7: 'Home', 8: 'ZR', 9: 'R' } }
            }
        };

        /**
         * Human-readable button names for display.
         * @constant {Object}
         */
        var BUTTON_NAMES = $.extend(true, {}, GENERIC_MAPPING.buttons);

        /**
         * Available Stellarium actions with correct display names matching Stellarium's translation system.
         * Version 2.4.0 includes educational presentation features.
         * @constant {Object}
         */
				var AVAILABLE_ACTIONS = {
						// === VIEW & NAVIGATION ===
						"actionGoto_Selected_Object": "Center on selected object",
						"actionSet_Tracking": "Track object",
						"actionSelect_Current_Object": "Select object at center",
						"reset_view": "Reset view direction",
						"reset_zoom": "Reset zoom to 60°",
						"stop_movement": "Stop movement",
						"actionGo_Home_Global": "Go to home view",

						// === EDUCATIONAL VIEW DIRECTIONS ===
						"view_north": "Look North",
						"view_south": "Look South",
						"view_east": "Look East",
						"view_west": "Look West",
						"view_zenith": "Look Zenith",
						"view_nadir": "Look Nadir",

						// === EDUCATIONAL ZOOM LEVELS ===
						"zoom_0_1": "Zoom to 0.1°",
						"zoom_1": "Zoom to 1°",
						"zoom_5": "Zoom to 5°",
						"zoom_10": "Zoom to 10°",
						"zoom_20": "Zoom to 20°",
						"zoom_30": "Zoom to 30°",
						"zoom_40": "Zoom to 40°",
						"zoom_50": "Zoom to 50°",
						"zoom_60": "Zoom to 60°",
						"zoom_90": "Zoom to 90°",
						"zoom_120": "Zoom to 120°",
						"zoom_150": "Zoom to 150°",
						"zoom_180": "Zoom to 180°",

						// === EDUCATIONAL VIEWPORT OFFSET ===
						"viewport_offset_m50": "Viewport offset -50%",
						"viewport_offset_m40": "Viewport offset -40%",
						"viewport_offset_m30": "Viewport offset -30%",
						"viewport_offset_m20": "Viewport offset -20%",
						"viewport_offset_m10": "Viewport offset -10%",
						"viewport_offset_0": "Viewport offset 0%",
						"viewport_offset_20": "Viewport offset 20%",
						"viewport_offset_40": "Viewport offset 40%",
						"viewport_offset_60": "Viewport offset 60%",
						"viewport_offset_80": "Viewport offset 80%",
						"viewport_offset_100": "Viewport offset 100%",

						// === NOTABLE STARS ===
						"search_polaris": "Center on Polaris (North Star)",
						"search_sirius": "Center on Sirius (Brightest Star)",
						"search_vega": "Center on Vega",
						"search_altair": "Center on Altair",
						"search_antares": "Center on Antares",
						"search_betelgeuse": "Center on Betelgeuse",
						"search_rigel": "Center on Rigel",
						"search_deneb": "Center on Deneb",
						
						// === MILKY WAY ===
						"milky_way_toggle": "Toggle Milky Way display",
						"milky_way_show": "Show Milky Way",
						"milky_way_hide": "Hide Milky Way",
						"milky_way_brightness_up": "Increase Milky Way brightness",
						"milky_way_brightness_down": "Decrease Milky Way brightness",

						// === DEEP SKY OBJECTS ===
						"search_andromeda_galaxy": "Center on Andromeda Galaxy",
						"search_orion_nebula": "Center on Orion Nebula (M42)",
						"search_pleiades": "Center on Pleiades (M45)",
						"search_ring_nebula": "Center on Ring Nebula (M57)",
						"search_hercules_cluster": "Center on Star Cluster in Hercules (M13)",

						// === SOLAR SYSTEM OBJECTS ===
						"search_sun": "Center on Sun",
						"search_moon": "Center on Moon",
						"search_mercury": "Center on Mercury",
						"search_venus": "Center on Venus",
						"search_mars": "Center on Mars",
						"search_jupiter": "Center on Jupiter",
						"search_saturn": "Center on Saturn",
						"search_uranus": "Center on Uranus",
						"search_neptune": "Center on Neptune",
						"search_pluto": "Center on Pluto",

						// === CONSTELLATION HIGHLIGHTS ===
						// These actions isolate and highlight ONLY the selected constellation
						"highlight_orion": "Highlight Orion (Toggle isolate)",
						"highlight_ursa_major": "Highlight Ursa Major (Toggle isolate)",
						"highlight_cassiopeia": "Highlight Cassiopeia (Toggle isolate)",
						"highlight_cygnus": "Highlight Cygnus (Toggle isolate)",
						"highlight_lyra": "Highlight Lyra (Toggle isolate)",
						"highlight_aquila": "Highlight Aquila (Toggle isolate)",
						"highlight_scorpius": "Highlight Scorpius (Toggle isolate)",
						"highlight_sagittarius": "Highlight Sagittarius (Toggle isolate)",
						"highlight_taurus": "Highlight Taurus (Toggle isolate)",
						"highlight_gemini": "Highlight Gemini (Toggle isolate)",

						// === CONSTELLATION DISPLAY ===
						"clear_constellation_highlight": "Show all constellations (clear isolation)",
						"toggle_constellation_lines": "Toggle all constellation lines",
						"toggle_constellation_labels": "Toggle all constellation labels",

						// === CELESTIAL LINES ===
						"show_celestial_sphere": "Show celestial sphere grid",
						"show_ecliptic_line": "Show ecliptic line",
						"show_equator_line": "Show celestial equator",
						"show_meridian_line": "Show meridian",
						"show_horizon_line": "Show horizon",
						"show_galactic_equator": "Show galactic equator",

						// === TIME CONTROL ===
						"actionPause_Time": "Pause time",
						"actionSet_Time_Now": "Set time to now",
						"actionIncrease_Time_Speed": "Increase time speed",
						"actionDecrease_Time_Speed": "Decrease time speed",
						"time_speed_normal": "Normal time speed (1x)",
						"time_speed_fast": "Fast forward (10x)",
						"time_speed_very_fast": "Very fast forward (100x)",
						"time_speed_slow": "Slow motion (0.1x)",
						"time_speed_reverse": "Reverse time (-1x)",

						// === TIME JUMPS (Educational) ===
						"actionSubtract_Solar_Hour": "Subtract solar hour",
						"actionAdd_Solar_Hour": "Add solar hour",
						"actionSubtract_Solar_Day": "Subtract solar day",
						"actionAdd_Solar_Day": "Add solar day",
						"actionSubtract_Solar_Week": "Subtract solar week",
						"actionAdd_Solar_Week": "Add solar week",
						"actionSubtract_Calendar_Year": "Subtract calendar year",
						"actionAdd_Calendar_Year": "Add calendar year",
						"actionSubtract_Sidereal_Day": "Subtract sidereal day",
						"actionAdd_Sidereal_Day": "Add sidereal day",
						"actionSubtract_Synodic_Month": "Subtract synodic month",
						"actionAdd_Synodic_Month": "Add synodic month",
						"actionSubtract_Draconic_Month": "Subtract draconic month",
						"actionAdd_Draconic_Month": "Add draconic month",

						// === SEASONS DEMONSTRATION ===
						"goto_summer_solstice": "Summer Solstice (June 21)",
						"goto_winter_solstice": "Winter Solstice (December 21)",
						"goto_spring_equinox": "Spring Equinox (March 20)",
						"goto_autumn_equinox": "Autumn Equinox (September 22)",
						"show_seasons_circles": "Show solstice/equinox circles",

						// === SKY CULTURE ACTIONS ===
						"skyculture_constellation_lines": "Toggle constellation lines",
						"skyculture_constellation_labels": "Toggle constellation labels",
						"skyculture_constellation_art": "Toggle constellation art",
						"skyculture_constellation_boundaries": "Toggle constellation boundaries",
						"skyculture_asterism_lines": "Toggle asterism lines",
						"skyculture_asterism_labels": "Toggle asterism labels",
						"skyculture_zodiac": "Toggle zodiac",
						"skyculture_lunar_mansions": "Toggle lunar mansions",
						"skyculture_abbreviated_names": "Use abbreviated names",

						// === CONSTELLATIONS (Standard) ===
						"actionShow_Constellation_Lines": "Constellation lines",
						"actionShow_Constellation_Labels": "Constellation labels",
						"actionShow_Constellation_Art": "Constellation art",
						"actionShow_Constellation_Boundaries": "Constellation boundaries",

						// === GRIDS ===
						"actionShow_Equatorial_Grid": "Equatorial grid",
						"actionShow_Equatorial_J2000_Grid": "Equatorial grid (J2000)",
						"actionShow_Azimuthal_Grid": "Azimuthal grid",
						"actionShow_Galactic_Grid": "Galactic grid",
						"actionShow_Supergalactic_Grid": "Supergalactic grid",

						// === LINES ===
						"actionShow_Equator_Line": "Equator (of date)",
						"actionShow_Equator_J2000_Line": "Equator (J2000)",
						"actionShow_Ecliptic_Line": "Ecliptic (of date)",
						"actionShow_Ecliptic_J2000_Line": "Ecliptic (J2000)",
						"actionShow_Horizon_Line": "Horizon",
						"actionShow_Galactic_Equator_Line": "Galactic equator",
						"actionShow_Meridian_Line": "Meridian",
						"actionShow_Prime_Vertical_Line": "Prime vertical",

						// === MARKERS ===
						"actionShow_Cardinal_Points": "Cardinal points",
						"actionShow_Intercardinal_Points": "Intercardinal points",
						"actionShow_Planets_Hints": "Planet markers",
						"actionShow_Planets_Orbits": "Planet orbits",
						"actionShow_Zenith_Nadir": "Zenith and Nadir",
						"actionShow_Celestial_Poles": "Celestial poles (of date)",
						"actionShow_Celestial_J2000_Poles": "Celestial poles (J2000)",
						"actionShow_Galactic_Poles": "Galactic poles",
						"actionShow_Equinox_J2000_Points": "Equinoxes (J2000)",
						"actionShow_Equinox_Points": "Equinoxes (of date)",
						"actionShow_Solstice_J2000_Points": "Solstices (J2000)",
						"actionShow_Solstice_Points": "Solstices (of date)",
						"actionShow_Apex_Points": "Apex points",
						"actionShow_Galactic_Center": "Galactic center",
						"actionShow_Compass_Marks": "Compass marks",
						"actionShow_Antisolar_Point": "Antisolar point",
						"actionShow_FOV_Center_Marker": "Center of FOV",
						"actionShow_FOV_Circular_Marker": "Circular FOV",
						"actionShow_FOV_Rectangular_Marker": "Rectangular FOV",
						"actionShow_Umbra_Circle": "Earth umbra",
						"actionShow_Penumbra_Circle": "Earth penumbra",

						// === DISPLAY ===
						"actionShow_Atmosphere": "Atmosphere",
						"actionShow_Ground": "Ground",
						"actionShow_Fog": "Fog",
						"actionShow_Nebulas": "Deep-sky objects",
						"actionShow_Planets": "Planets",
						"actionShow_Planets_Labels": "Planet labels",
						"actionShow_Stars": "Stars",
						"actionShow_Star_Labels": "Star labels",
						"actionShow_Night_Mode": "Night mode",
						"actionSet_Full_Screen_Global": "Full-screen mode",

						// === LANDSCAPE ===
						"actionShow_Landscape_Labels": "Landscape labels",
						"actionShow_Landscape_Illumination": "Landscape illumination",

						// === MOUNT ===
						"actionSwitch_Equatorial_Mount": "Equatorial mount",

						// === TAB NAVIGATION ===
						"tab_previous": "Previous tab",
						"tab_next": "Next tab",
						
						// === DEFAULT ===
						"none": "No action"
				};

        /**
         * Plugin-specific action definitions.
         * @constant {Object}
         */
        var PLUGIN_ACTIONS = {
            // StelActions plugins
            "ExoPlanets": { "actionShow_Exoplanets": "Toggle Exoplanets" },
            "NavStars": { "actionShow_NavStars": "Toggle Navigational Stars" },
            "MeteorShowers": { "actionShow_MeteorShowers": "Toggle Meteor Showers" },
            "Quasars": { "actionShow_Quasars": "Toggle Quasars" },
            "Pulsars": { "actionShow_Pulsars": "Toggle Pulsars" },
            
            // StelProperties plugins
            "ArchaeoLines": {
								"ArchaeoLines.enabled": "Toggle ArchaeoLines",
                "ArchaeoLines.flagShowEquinox": "Show Equinox Line",
                "ArchaeoLines.flagShowSolstices": "Show Solstice Lines",
                "ArchaeoLines.flagShowCrossquarters": "Show Crossquarter Lines",
                "ArchaeoLines.flagShowMajorStandstills": "Show Major Standstill",
                "ArchaeoLines.flagShowMinorStandstills": "Show Minor Standstill",
                "ArchaeoLines.flagShowZenithPassage": "Show Zenith Passage",
                "ArchaeoLines.flagShowNadirPassage": "Show Nadir Passage",
                "ArchaeoLines.flagShowCurrentSun": "Show Current Sun Line",
                "ArchaeoLines.flagShowCurrentMoon": "Show Current Moon Line",
                "ArchaeoLines.flagShowSelectedObject": "Show Selected Object Line"
            },
            "Scenery3d": {
                "scenery3d_enableScene": "Toggle 3D Landscape",
                "scenery3d_enableLocationInfo": "Toggle location text",
                "scenery3d_enableShadows": "Toggle shadows",
                "scenery3d_enableTorchLight": "Toggle torchlight"
            }
        };

        /**
         * Action categories for UI organization.
         * @constant {Object}
         */
				var ACTION_CATEGORIES_BASE = {
						"View & Navigation": [
								"actionGoto_Selected_Object", "actionSet_Tracking", "actionSelect_Current_Object",
								"reset_view", "reset_zoom", "stop_movement", "actionGo_Home_Global",
								"tab_previous", "tab_next"
						],
						
						"Educational: View Directions": [
								"view_north", "view_south", "view_east", "view_west", "view_zenith", "view_nadir"
						],
						
						"Educational: Zoom Levels": [
								"zoom_0_1", "zoom_1", "zoom_5", "zoom_10", "zoom_20", "zoom_30",
								"zoom_40", "zoom_50", "zoom_60", "zoom_90", "zoom_120", "zoom_150", "zoom_180"
						],
						
						"Educational: Viewport Offset": [
								"viewport_offset_m50", "viewport_offset_m40", "viewport_offset_m30",
								"viewport_offset_m20", "viewport_offset_m10", "viewport_offset_0",
								"viewport_offset_20", "viewport_offset_40", "viewport_offset_60",
								"viewport_offset_80", "viewport_offset_100"
						],
						
						"Educational: Notable Stars": [
								"search_polaris", "search_sirius", "search_vega", "search_altair",
								"search_antares", "search_betelgeuse", "search_rigel", "search_deneb"
						],
						
						"Educational: Deep Sky Objects": [
								"search_andromeda_galaxy", "search_orion_nebula", "search_pleiades",
								"search_ring_nebula", "search_hercules_cluster"
						],
						
						"Educational: Solar System": [
								"search_sun", "search_moon", "search_mercury", "search_venus",
								"search_mars", "search_jupiter", "search_saturn",
								"search_uranus", "search_neptune", "search_pluto"
						],
						
						"Educational: Constellation Highlights": [
								"highlight_orion", "highlight_ursa_major", "highlight_cassiopeia",
								"highlight_cygnus", "highlight_lyra", "highlight_aquila",
								"highlight_scorpius", "highlight_sagittarius", "highlight_taurus", "highlight_gemini",
								"clear_constellation_highlight"
						],
						
						"Educational: Celestial Lines": [
								"show_celestial_sphere", "show_ecliptic_line", "show_equator_line",
								"show_meridian_line", "show_horizon_line", "show_galactic_equator"
						],
						
						"Educational: Time Speed": [
								"actionIncrease_Time_Speed", "actionDecrease_Time_Speed", "actionPause_Time",
								"actionSet_Time_Now", "time_speed_normal", "time_speed_fast",
								"time_speed_very_fast", "time_speed_slow", "time_speed_reverse"
						],
						
						"Educational: Time Jumps": [
								"actionSubtract_Solar_Hour", "actionAdd_Solar_Hour",
								"actionSubtract_Solar_Day", "actionAdd_Solar_Day",
								"actionSubtract_Solar_Week", "actionAdd_Solar_Week",
								"actionSubtract_Calendar_Year", "actionAdd_Calendar_Year",
								"actionSubtract_Sidereal_Day", "actionAdd_Sidereal_Day",
								"actionSubtract_Synodic_Month", "actionAdd_Synodic_Month",
								"actionSubtract_Draconic_Month", "actionAdd_Draconic_Month"
						],
						
						"Educational: Seasons": [
								"goto_summer_solstice", "goto_winter_solstice",
								"goto_spring_equinox", "goto_autumn_equinox", "show_seasons_circles"
						],
						
						"Constellations": [
								"actionShow_Constellation_Lines", "actionShow_Constellation_Labels",
								"actionShow_Constellation_Art", "actionShow_Constellation_Boundaries",
								"toggle_constellation_lines", "toggle_constellation_labels"
						],
						
						"Sky Culture": [
								"skyculture_constellation_lines", "skyculture_constellation_labels",
								"skyculture_constellation_art", "skyculture_constellation_boundaries",
								"skyculture_asterism_lines", "skyculture_asterism_labels",
								"skyculture_zodiac", "skyculture_lunar_mansions", "skyculture_abbreviated_names"
						],
						
						"Grids": [
								"actionShow_Equatorial_Grid", "actionShow_Equatorial_J2000_Grid",
								"actionShow_Azimuthal_Grid", "actionShow_Galactic_Grid", "actionShow_Supergalactic_Grid"
						],
						
						"Lines": [
								"actionShow_Equator_Line", "actionShow_Equator_J2000_Line",
								"actionShow_Ecliptic_Line", "actionShow_Ecliptic_J2000_Line",
								"actionShow_Horizon_Line", "actionShow_Galactic_Equator_Line",
								"actionShow_Meridian_Line", "actionShow_Prime_Vertical_Line"
						],
						
						"Markers": [
								"actionShow_Cardinal_Points", "actionShow_Intercardinal_Points",
								"actionShow_Planets_Hints", "actionShow_Planets_Orbits",
								"actionShow_Zenith_Nadir", "actionShow_Celestial_Poles",
								"actionShow_Celestial_J2000_Poles", "actionShow_Galactic_Poles",
								"actionShow_Equinox_J2000_Points", "actionShow_Equinox_Points",
								"actionShow_Solstice_J2000_Points", "actionShow_Solstice_Points",
								"actionShow_Apex_Points", "actionShow_Galactic_Center",
								"actionShow_Compass_Marks", "actionShow_Antisolar_Point",
								"actionShow_FOV_Center_Marker", "actionShow_FOV_Circular_Marker",
								"actionShow_FOV_Rectangular_Marker", "actionShow_Umbra_Circle",
								"actionShow_Penumbra_Circle"
						],
						
						"Sky Display": [
								"actionShow_Atmosphere", "actionShow_Ground", "actionShow_Fog",
								"actionShow_Nebulas", "actionShow_Planets", "actionShow_Planets_Labels",
								"actionShow_Stars", "actionShow_Star_Labels", "actionShow_Night_Mode"
						],
						
						"Milky Way": [
								"milky_way_toggle",
								"milky_way_show",
								"milky_way_hide",
								"milky_way_brightness_up",
								"milky_way_brightness_down"
						],
						
						"Landscape": [
								"actionShow_Landscape_Labels", "actionShow_Landscape_Illumination"
						],
						
						"System": [
								"actionSwitch_Equatorial_Mount", "actionSet_Full_Screen_Global"
						]
				};

        var ACTION_CATEGORIES = $.extend(true, {}, ACTION_CATEGORIES_BASE);

        // =====================================================================
        // SECTION 2: PRIVATE VARIABLES AND STATE
        // =====================================================================

        var connectedGamepads = new Map();
        var animationFrame = null;
        var isPolling = false;
        var isServerConnected = true;
        var activeDeviceIndex = null;
        var loadedPlugins = {};

        // UI Element References
        var $deviceName, $deviceModel, $deviceVendor, $deviceId, $connectionStatus, $currentFov;
        var $leftStickCanvas, $rightStickCanvas;
        var $sensitivitySlider, $deadzoneSlider, $zoomSpeedSlider, $movementSpeedSlider;
        var $invertX, $invertY;
				var $zoomInvertY;
				var $currentDirection;  // For displaying current view direction
        var $calibrateBtn, $testVibrationBtn, $vibrationFeedback;
        var $buttonCustomizationContainer;
        var $deviceSelector;
				var $viewFovSlider;

        // Global Settings
        var controllerSettings = {
            sensitivity: 1.0,
            deadzone: 0.15,
            invertX: false,
            invertY: false,
						zoomInvertY: false,
            zoomSpeed: 0.05,
            movementSpeed: 5.0,
            vibrationFeedback: false,
						vibrationIntensity: 0.5
        };

        // Calibration Data
        var calibrationData = {
            leftX: 0, leftY: 0, rightX: 0, rightY: 0,
            isCalibrated: false,
            calibrationMode: false,
            samples: [],
            sampleCount: 0,
            activeDeviceIndex: null
        };

        var currentDeviceInfo = null;
        var currentFov = 60;
        var gamepadManager = null;

        // =====================================================================
        // SECTION 3: PLUGIN MANAGEMENT
        // =====================================================================

        function loadPluginActions() {
            $.ajax({
                url: '/api/main/plugins',
                type: 'GET',
                dataType: 'json',
                success: function(plugins) {
                    loadedPlugins = plugins;
                    
                    for (var pluginName in PLUGIN_ACTIONS) {
                        if (plugins[pluginName] && plugins[pluginName].loaded) {
                            extendActionsWithPlugin(pluginName);
                            console.log("[Gamepad] Loaded actions for plugin: " + pluginName);
                        } else if (ACTION_CATEGORIES[pluginName]) {
                            delete ACTION_CATEGORIES[pluginName];
                            var pluginActions = PLUGIN_ACTIONS[pluginName];
                            for (var actionId in pluginActions) {
                                delete AVAILABLE_ACTIONS[actionId];
                            }
                        }
                    }
                    
                    if (currentDeviceInfo) {
                        populateButtonCustomization();
                    }
                },
                error: function(xhr, status, errorThrown) {
                    console.log("[Gamepad] Error loading plugin information: " + errorThrown);
                }
            });
        }

        function extendActionsWithPlugin(pluginName) {
            var pluginActions = PLUGIN_ACTIONS[pluginName];
            if (!pluginActions) return;
            
            if (!ACTION_CATEGORIES[pluginName]) {
                ACTION_CATEGORIES[pluginName] = [];
            }
            
            for (var actionId in pluginActions) {
                if (!AVAILABLE_ACTIONS[actionId]) {
                    AVAILABLE_ACTIONS[actionId] = pluginActions[actionId];
                    ACTION_CATEGORIES[pluginName].push(actionId);
                }
            }
        }

        function isPluginLoaded(pluginName) {
            return loadedPlugins[pluginName] && loadedPlugins[pluginName].loaded;
        }

        // =====================================================================
        // SECTION 4: VIBRATION FUNCTIONS
        // =====================================================================

				/**
				 * Triggers vibration on the active gamepad device.
				 * Works with Chrome's Gamepad API implementation.
				 * @param {number} duration - Duration in milliseconds
				 * @param {number} customIntensity - Optional custom intensity (overrides settings)
				 * @returns {boolean} True if vibration was triggered
				 */
				function triggerVibration(duration, customIntensity) {
						var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
						if (!device) return false;
						
						// Use custom intensity if provided, otherwise use settings
						var intensity = (customIntensity !== undefined) ? customIntensity : controllerSettings.vibrationIntensity;
						
						// Don't vibrate if intensity is zero or vibration is disabled
						if (!controllerSettings.vibrationFeedback || intensity <= 0) return false;
						
						var gp = navigator.getGamepads()[device.index];
						if (!gp) return false;
						
						if (gp.vibrationActuator) {
								try {
										gp.vibrationActuator.playEffect("dual-rumble", {
												startDelay: 0,
												duration: duration,
												weakMagnitude: intensity * 0.6,  // less intensity for weak vibration
												strongMagnitude: intensity       // strong vibration depends on chosen intensity
										}).catch(function(err) {
												console.warn("Vibration effect failed:", err);
										});
										return true;
								} catch(e) {
										console.warn("Vibration not supported:", e);
								}
						}
						
						// Fallback: page vibration (intensity not supported)
						if (window.navigator && window.navigator.vibrate) {
								window.navigator.vibrate(duration);
								return true;
						}
						
						return false;
				}

				/**
				 * Manual vibration test function with full intensity
				 */
				function testVibrationManual() {
						var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
						if (!device) {
								showNotification(_tr("No controller connected"));
								return;
						}
						
						// Use full intensity for test (0.8) regardless of settings
						var success = triggerVibration(400, 0.8);
						if (success) {
								showNotification(_tr("Vibration test sent to ") + device.getDisplayInfo().name + 
										" (Intensity: " + Math.round(controllerSettings.vibrationIntensity * 100) + "%)");
						} else {
								showNotification(_tr("Vibration not supported on this device/browser"));
						}
				}

        // =====================================================================
        // SECTION 5: GAMEPAD DEVICE CLASS
        // =====================================================================

        var GamepadDevice = function(index, gamepad) {
            this.index = index;
            this.id = gamepad.id;
            this.gamepad = gamepad;
            this.buttonStates = new Map();
            this.previousButtonStates = new Map();
            this.axisStates = { leftX: 0, leftY: 0, rightX: 0, rightY: 0 };
            this.lastLeftX = 0;
            this.lastLeftY = 0;
            this.lastRightX = 0;
            this.lastRightY = 0;
            this.lastZoomValue = 0;
            this.isMoving = false;
            this.zoomActive = false;
            this.mappings = $.extend(true, {}, GENERIC_MAPPING);
            this.buttonMappings = new Map();
            this.vendorId = null;
            this.productId = null;
            this.uniqueId = null;
            this.calibrationData = {
                leftX: 0, leftY: 0, rightX: 0, rightY: 0,
                isCalibrated: false
            };

            this.detectDeviceType();
            this.generateUniqueId();
            this.loadButtonMappings();
            this.loadCalibration();
            console.log("[Gamepad] Device " + this.index + " initialized: " + this.mappings.name + " (" + this.id + ")");
        };

        GamepadDevice.prototype.detectDeviceType = function() {
            var idLower = this.id.toLowerCase();

            var vidMatch = idLower.match(/vid[_-]?([0-9a-f]{4})/i);
            var pidMatch = idLower.match(/pid[_-]?([0-9a-f]{4})/i);
            
            if (vidMatch && pidMatch) {
                this.vendorId = vidMatch[1];
                this.productId = pidMatch[1];
            } else {
                var idsMatch = idLower.match(/([0-9a-f]{4})-([0-9a-f]{4})/i);
                if (idsMatch) {
                    this.vendorId = idsMatch[1];
                    this.productId = idsMatch[2];
                } else {
                    var vendorMatch = idLower.match(/vendor:\s*([0-9a-f]{4})/i);
                    var productMatch = idLower.match(/product:\s*([0-9a-f]{4})/i);
                    if (vendorMatch && productMatch) {
                        this.vendorId = vendorMatch[1];
                        this.productId = productMatch[1];
                    }
                }
            }

            console.log("[Gamepad] Device " + this.index + ": Extracted VID=" + this.vendorId + ", PID=" + this.productId);

            if (this.vendorId && DEVICE_MAPPINGS[this.vendorId]) {
                if (this.productId && DEVICE_MAPPINGS[this.vendorId][this.productId]) {
                    this.mappings = $.extend(true, {}, DEVICE_MAPPINGS[this.vendorId][this.productId]);
                    console.log("[Gamepad] Device " + this.index + ": Identified as " + this.mappings.name);
                } else {
                    var vendorMappings = DEVICE_MAPPINGS[this.vendorId];
                    if (vendorMappings && Object.keys(vendorMappings).length > 0) {
                        var firstMapping = vendorMappings[Object.keys(vendorMappings)[0]];
                        this.mappings.name = firstMapping.name;
                        this.mappings.vendorName = firstMapping.vendorName;
                        console.log("[Gamepad] Device " + this.index + ": Vendor known, using generic vendor mapping.");
                    }
                }
            }

            if (this.mappings.buttons) {
                for (var idx in this.mappings.buttons) {
                    BUTTON_NAMES[parseInt(idx)] = this.mappings.buttons[idx];
                }
            }
        };

        GamepadDevice.prototype.generateUniqueId = function() {
            var components = [];
            if (this.vendorId) components.push("vid_" + this.vendorId);
            if (this.productId) components.push("pid_" + this.productId);
            
            var sanitizedName = this.id.replace(/[^a-zA-Z0-9]/g, '_').substring(0, 50);
            if (sanitizedName) components.push(sanitizedName);
            
            this.uniqueId = components.join("_") || "device_" + this.index;
            console.log("[Gamepad] Device " + this.index + ": Unique ID = " + this.uniqueId);
        };

        GamepadDevice.prototype.loadButtonMappings = function() {
            try {
                var storageKey = "gamepad_mappings_" + this.uniqueId;
                var saved = localStorage.getItem(storageKey);
                if (saved) {
                    var parsed = JSON.parse(saved);
                    for (var idx in parsed) {
                        this.buttonMappings.set(parseInt(idx), parsed[idx]);
                    }
                    console.log("[Gamepad] Device " + this.index + ": Loaded " + this.buttonMappings.size + " saved button mappings");
                }
            } catch(e) {
                console.warn("[Gamepad] Could not load button mappings:", e);
            }
        };

        GamepadDevice.prototype.saveButtonMappings = function() {
            try {
                var toSave = {};
                for (var entry of this.buttonMappings.entries()) {
                    toSave[entry[0]] = entry[1];
                }
                var storageKey = "gamepad_mappings_" + this.uniqueId;
                localStorage.setItem(storageKey, JSON.stringify(toSave));
                console.log("[Gamepad] Device " + this.index + ": Saved " + this.buttonMappings.size + " button mappings");
            } catch(e) {
                console.warn("[Gamepad] Could not save button mappings:", e);
            }
        };

        GamepadDevice.prototype.loadCalibration = function() {
            try {
                var storageKey = "gamepad_calibration_" + this.uniqueId;
                var saved = localStorage.getItem(storageKey);
                if (saved) {
                    var data = JSON.parse(saved);
                    this.calibrationData = {
                        leftX: data.leftX || 0,
                        leftY: data.leftY || 0,
                        rightX: data.rightX || 0,
                        rightY: data.rightY || 0,
                        isCalibrated: true
                    };
                    console.log("[Gamepad] Device " + this.index + ": Loaded calibration data");
                }
            } catch(e) {
                console.warn("[Gamepad] Could not load calibration:", e);
            }
        };

        GamepadDevice.prototype.saveCalibration = function() {
            try {
                var storageKey = "gamepad_calibration_" + this.uniqueId;
                localStorage.setItem(storageKey, JSON.stringify({
                    leftX: this.calibrationData.leftX,
                    leftY: this.calibrationData.leftY,
                    rightX: this.calibrationData.rightX,
                    rightY: this.calibrationData.rightY
                }));
                console.log("[Gamepad] Device " + this.index + ": Saved calibration data");
            } catch(e) {
                console.warn("[Gamepad] Could not save calibration:", e);
            }
        };

        GamepadDevice.prototype.exportProfile = function() {
            var mappings = {};
            for (var entry of this.buttonMappings.entries()) {
                mappings[entry[0]] = entry[1];
            }
            
            return {
                version: "2.4.0",
                deviceInfo: {
                    uniqueId: this.uniqueId,
                    vendorId: this.vendorId,
                    productId: this.productId,
                    name: this.mappings.name,
                    vendorName: this.mappings.vendorName,
                    model: this.mappings.model,
                    rawId: this.id
                },
                mappings: mappings,
                calibration: {
                    leftX: this.calibrationData.leftX,
                    leftY: this.calibrationData.leftY,
                    rightX: this.calibrationData.rightX,
                    rightY: this.calibrationData.rightY,
                    isCalibrated: this.calibrationData.isCalibrated
                }
            };
        };

        GamepadDevice.prototype.importProfile = function(profile) {
            try {
                if (profile.deviceInfo && profile.deviceInfo.uniqueId !== this.uniqueId) {
                    console.warn("[Gamepad] Importing profile from different device type.");
                    showNotification(_tr("Warning: This profile was created for a different controller type."));
                }
                
                if (profile.mappings) {
                    this.buttonMappings.clear();
                    for (var idx in profile.mappings) {
                        this.buttonMappings.set(parseInt(idx), profile.mappings[idx]);
                    }
                    this.saveButtonMappings();
                }
                
                if (profile.calibration) {
                    this.calibrationData = {
                        leftX: profile.calibration.leftX || 0,
                        leftY: profile.calibration.leftY || 0,
                        rightX: profile.calibration.rightX || 0,
                        rightY: profile.calibration.rightY || 0,
                        isCalibrated: profile.calibration.isCalibrated || false
                    };
                    this.saveCalibration();
                }
                
                console.log("[Gamepad] Device " + this.index + ": Successfully imported profile");
                return true;
            } catch(e) {
                console.error("[Gamepad] Failed to import profile:", e);
                return false;
            }
        };

        GamepadDevice.prototype.getButtonAction = function(buttonIndex) {
            return this.buttonMappings.get(buttonIndex) || "none";
        };

        GamepadDevice.prototype.setButtonAction = function(buttonIndex, action) {
            this.buttonMappings.set(buttonIndex, action);
            this.saveButtonMappings();
        };

        /**
         * Handles educational actions (view directions, zoom, etc.)
         */
        GamepadDevice.prototype.handleEducationalAction = function(action) {
						 // View directions with smooth transition using core.moveToAltAzi
						// Duration set to 0.5 seconds for smooth but responsive movement
						if (action === "view_north") {
								// North = azimuth 180° (π radians), altitude 0°
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(0, 0, 3)", 
										useIncludes: false 
								});
								return true;
						}
						if (action === "view_south") {
								// South = azimuth 0°, altitude 0°
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(0, 180, 3)", 
										useIncludes: false 
								});
								return true;
						}
						if (action === "view_east") {
								// East = azimuth 90°, altitude 0°
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(0, 90, 3)", 
										useIncludes: false 
								});
								return true;
						}
						if (action === "view_west") {
								// West = azimuth 270°, altitude 0°
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(0, 270, 3)", 
										useIncludes: false 
								});
								return true;
						}
						if (action === "view_zenith") {
								// Zenith = altitude 90°, azimuth unchanged
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(90, 0, 3)", 
										useIncludes: false 
								});
								return true;
						}
						if (action === "view_nadir") {
								// Nadir = altitude -90°, azimuth unchanged
								rc.postCmd("/api/scripts/direct", { 
										code: "core.moveToAltAzi(-90, 0, 3)", 
										useIncludes: false 
								});
								return true;
						}

           // Zoom levels with smooth transition (3 seconds duration)
					var zoomMap = {
							"zoom_0_1": 0.1, "zoom_1": 1, "zoom_5": 5, "zoom_10": 10,
							"zoom_20": 20, "zoom_30": 30, "zoom_40": 40, "zoom_50": 50,
							"zoom_60": 60, "zoom_90": 90, "zoom_120": 120, "zoom_150": 150, "zoom_180": 180
					};
					if (zoomMap[action] !== undefined) {
							var zoomValue = zoomMap[action];
							// Use smooth zoom transition with (3 seconds duration)
							rc.postCmd("/api/scripts/direct", {
									code: "StelMovementMgr.zoomTo(" + zoomValue + ", 3)",
									useIncludes: false
							});
							return true;
					}
            
						// Viewport offset with smooth transition (3 seconds)
						var offsetMap = {
								"viewport_offset_m50": -50, "viewport_offset_m40": -40, "viewport_offset_m30": -30,
								"viewport_offset_m20": -20, "viewport_offset_m10": -10, "viewport_offset_0": 0,
								"viewport_offset_20": 20, "viewport_offset_40": 40, "viewport_offset_60": 60,
								"viewport_offset_80": 80, "viewport_offset_100": 100
						};
						if (offsetMap[action] !== undefined) {
								var offsetValue = offsetMap[action];
								rc.postCmd("/api/scripts/direct", {
										code: "StelMovementMgr.moveViewport(0, " + offsetValue + ", 3)",
										useIncludes: false
								});
								return true;
						}
            
            // Time speed
            if (action === "time_speed_normal") {
                if (timeApi) timeApi.setTimeRate(0.000011574);
                return true;
            }
            if (action === "time_speed_fast") {
                if (timeApi) timeApi.setTimeRate(0.00011574);
                return true;
            }
            if (action === "time_speed_very_fast") {
                if (timeApi) timeApi.setTimeRate(0.0011574);
                return true;
            }
            if (action === "time_speed_slow") {
                if (timeApi) timeApi.setTimeRate(0.0000011574);
                return true;
            }
            if (action === "time_speed_reverse") {
                if (timeApi) timeApi.setTimeRate(-0.000011574);
                return true;
            }
            
            // Seasons
            if (action === "goto_summer_solstice") {
                timeApi.setJDay(2451545.0 + 92.75);
                return true;
            }
            if (action === "goto_winter_solstice") {
                timeApi.setJDay(2451545.0 + 275.25);
                return true;
            }
            if (action === "goto_spring_equinox") {
                timeApi.setJDay(2451545.0 + 0.0);
                return true;
            }
            if (action === "goto_autumn_equinox") {
                timeApi.setJDay(2451545.0 + 183.0);
                return true;
            }
            
            return false;
        };

        /**
         * Handles search actions for celestial objects
         */
        GamepadDevice.prototype.handleSearchAction = function(searchTerm) {
					 // Special case for M13/Hercules Cluster
						if (searchTerm === "hercules_cluster") {
								searchTerm = "M13";
						} else {
								searchTerm = searchTerm.replace(/_/g, ' ');
								searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase(); });
						}
            
            if (searchApi && typeof searchApi.selectObjectByName === 'function') {
                var mode = $("#select_SelectionMode").val() || "center";
                searchApi.selectObjectByName(searchTerm, mode);
            } else {
                rc.postCmd("/api/main/focus", { target: searchTerm, mode: "center" });
            }
        };

					/**
					 * Toggles constellation highlighting
					 * First press: isolates and highlights the specified constellation
					 * Second press: clears isolation and shows all constellations
					 * @param {string} constellationName - Name of the constellation (e.g., "orion", "ursa_major")
					 */
					GamepadDevice.prototype.handleConstellationHighlight = function(constellationName) {
							// Convert to proper case (e.g., "orion" -> "Orion")
							var searchTerm = constellationName.replace(/_/g, ' ');
							searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase() });
							
							// Get current state
							var isIsolated = propApi.getStelProp("ConstellationMgr.isolateSelected");
							var currentSelection = propApi.getStelProp("StelCore.selectedObject");
							
							// Check if this constellation is currently highlighted
							var isCurrentlyHighlighted = isIsolated === true && 
									currentSelection && currentSelection.toLowerCase() === searchTerm.toLowerCase();
							
							if (isCurrentlyHighlighted) {
									// Toggle OFF: Clear highlighting
									console.log("[Gamepad] Clearing constellation highlight:", searchTerm);
									
									// Disable isolation mode
									propApi.setStelProp("ConstellationMgr.isolateSelected", false);
									
									// Clear selected object
									rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
									
									// Ensure constellation lines remain visible
									var linesVisible = propApi.getStelProp("ConstellationMgr.linesDisplayed");
									if (linesVisible === false) {
											actions.execute("actionShow_Constellation_Lines");
									}
									
									showNotification(_tr("Cleared highlight: ") + searchTerm);
							} else {
									// Toggle ON: Highlight the constellation
									console.log("[Gamepad] Highlighting constellation:", searchTerm);
									
									// Step 1: Ensure constellation lines are visible
									var linesVisible = propApi.getStelProp("ConstellationMgr.linesDisplayed");
									if (linesVisible === false) {
											actions.execute("actionShow_Constellation_Lines");
									}
									
									// Step 2: Clear previous selection
									rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
									
									// Step 3: Delay to allow Stellarium to process the clear
									setTimeout(function() {
											// Step 4: Enable isolation mode
											propApi.setStelProp("ConstellationMgr.isolateSelected", true);
											
											// Step 5: Select the constellation
											rc.postCmd("/api/main/focus", { target: searchTerm, mode: "mark" }, null, function() {});
											
											// Step 6: Show feedback
											showNotification(_tr("Highlighting constellation: ") + searchTerm);
									}, 100);
							}
					};
									
									// Add a property to track last highlighted constellation
					GamepadDevice.prototype.lastHighlightedConstellation = null;

					GamepadDevice.prototype.handleConstellationHighlight = function(constellationName) {
							var searchTerm = constellationName.replace(/_/g, ' ');
							searchTerm = searchTerm.replace(/\b\w/g, function(l) { return l.toUpperCase() });
							
							// Check if this constellation is currently highlighted
							var isCurrentlyHighlighted = (this.lastHighlightedConstellation === searchTerm);
							
							if (isCurrentlyHighlighted) {
									// Toggle OFF
									console.log("[Gamepad] Clearing constellation highlight:", searchTerm);
									
									propApi.setStelProp("ConstellationMgr.isolateSelected", false);
									rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
									
									var linesVisible = propApi.getStelProp("ConstellationMgr.linesDisplayed");
									if (linesVisible === false) {
											actions.execute("actionShow_Constellation_Lines");
									}
									
									this.lastHighlightedConstellation = null;
									showNotification(_tr("Cleared highlight: ") + searchTerm);
							} else {
									// Toggle ON
									console.log("[Gamepad] Highlighting constellation:", searchTerm);
									
									var linesVisible = propApi.getStelProp("ConstellationMgr.linesDisplayed");
									if (linesVisible === false) {
											actions.execute("actionShow_Constellation_Lines");
									}
									
									rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
									
									setTimeout(function() {
											propApi.setStelProp("ConstellationMgr.isolateSelected", true);
											rc.postCmd("/api/main/focus", { target: searchTerm, mode: "mark" }, null, function() {});
											showNotification(_tr("Highlighting constellation: ") + searchTerm);
									}, 100);
									
									this.lastHighlightedConstellation = searchTerm;
							}
					};

				/**
				 * Clears all constellation highlighting
				 */
				GamepadDevice.prototype.clearConstellationHighlight = function() {
						console.log("[Gamepad] Clearing all constellation highlights");
						
						// Step 1: Disable isolation mode
						propApi.setStelProp("ConstellationMgr.isolateSelected", false);
						
						// Step 2: Clear any selected object (with empty success function to prevent alert)
						rc.postCmd("/api/main/focus", { target: "", mode: "mark" }, null, function() {});
						
						// Step 3: Ensure constellation lines are still visible
						var linesVisible = propApi.getStelProp("ConstellationMgr.linesDisplayed");
						if (linesVisible === false) {
								actions.execute("actionShow_Constellation_Lines");
						}
						
						// Use console log instead of notification to avoid translation error
						console.log(_tr("All constellations visible"));
						showNotification(_tr("All constellations visible"));
				};

				 /**
         * Handles button actions using Stellarium APIs.
         */
        GamepadDevice.prototype.handleButtonAction = function(action) {
            // Trigger vibration feedback if enabled
						if (controllerSettings.vibrationFeedback && action !== "none") {
								// Use settings intensity (no custom intensity = use settings)
								triggerVibration(60, controllerSettings.vibrationIntensity);
						}
            
            if (action === "none") return;
            
            // Tab navigation
            if (action === "tab_previous") { navigateTab(-1); return; }
            if (action === "tab_next") { navigateTab(1); return; }
            
            // View controls
            if (action === "reset_view") { this.sendMovement(0, 0); return; }
            if (action === "reset_zoom") { 
								viewcontrol.setFOV(60);
								// Force immediate update of all FOV displays
								currentFov = 60;
								updateFovDisplay(60);
								return;
						}
            if (action === "stop_movement") { this.sendMovement(0, 0); return; }
            
            // Educational actions (view directions, zoom, time speed, seasons)
            if (this.handleEducationalAction(action)) return;
            
            // Constellation highlights
            if (action.indexOf("highlight_") === 0) {
                this.handleConstellationHighlight(action.substring(10));
                return;
            }
            
            // Celestial lines
            if (action === "show_celestial_sphere") {
                actions.execute("actionShow_Equatorial_Grid");
                return;
            }
            if (action === "show_ecliptic_line") {
                actions.execute("actionShow_Ecliptic_Line");
                return;
            }
            if (action === "show_equator_line") {
                actions.execute("actionShow_Equator_Line");
                return;
            }
            if (action === "show_meridian_line") {
                actions.execute("actionShow_Meridian_Line");
                return;
            }
            if (action === "show_horizon_line") {
                actions.execute("actionShow_Horizon_Line");
                return;
            }
            if (action === "show_galactic_equator") {
                actions.execute("actionShow_Galactic_Equator_Line");
                return;
            }
            
            // Search actions
            if (action.indexOf("search_") === 0) {
                this.handleSearchAction(action.substring(7));
                return;
            }
            
            // Sky culture actions
            if (action.indexOf("skyculture_") === 0) {
                var propMap = {
                    "skyculture_constellation_lines": "ConstellationMgr.linesDisplayed",
                    "skyculture_constellation_labels": "ConstellationMgr.namesDisplayed",
                    "skyculture_constellation_art": "ConstellationMgr.artDisplayed",
                    "skyculture_constellation_boundaries": "ConstellationMgr.boundariesDisplayed",
                    "skyculture_asterism_lines": "AsterismMgr.linesDisplayed",
                    "skyculture_asterism_labels": "AsterismMgr.namesDisplayed",
                    "skyculture_zodiac": "ConstellationMgr.zodiacDisplayed",
                    "skyculture_lunar_mansions": "ConstellationMgr.lunarSystemDisplayed",
                    "skyculture_abbreviated_names": "StelSkyCultureMgr.flagUseAbbreviatedNames"
                };
                var propName = propMap[action];
                if (propName) {
                    var currentValue = propApi.getStelProp(propName);
                    if (currentValue !== undefined) {
                        propApi.setStelPropQueued(propName, !currentValue);
                    }
                }
                return;
            }
            
            // Time controls
            if (action === "actionIncrease_Time_Speed") {
                if (timeApi && typeof timeApi.increaseTimeRate === 'function') {
                    timeApi.increaseTimeRate();
                } else { rc.postCmd("/api/main/time", { timerate: "increase" }); }
                return;
            }
            if (action === "actionDecrease_Time_Speed") {
                if (timeApi && typeof timeApi.decreaseTimeRate === 'function') {
                    timeApi.decreaseTimeRate();
                } else { rc.postCmd("/api/main/time", { timerate: "decrease" }); }
                return;
            }
            if (action === "actionPause_Time") {
                if (timeApi && typeof timeApi.togglePlayPause === 'function') {
                    timeApi.togglePlayPause();
                } else { rc.postCmd("/api/main/time", { timerate: 0 }); }
                return;
            }
            if (action === "actionSet_Time_Now") {
                if (timeApi && typeof timeApi.setDateNow === 'function') {
                    timeApi.setDateNow();
                } else { rc.postCmd("/api/main/time", { time: "now" }); }
                return;
            }
            
            // Time jump actions
            if (action.indexOf("actionSubtract_") === 0 || action.indexOf("actionAdd_") === 0) {
                if (actions && typeof actions.execute === 'function') {
                    actions.execute(action);
                }
                return;
            }
            
            // Plugin handling
            if (action === "actionShow_Exoplanets" || action === "actionShow_NavStars" ||
                action === "actionShow_MeteorShowers" || action === "actionShow_Quasars" ||
                action === "actionShow_Pulsars") {
                if (actions && typeof actions.execute === 'function') {
                    actions.execute(action);
                }
                return;
            }
            
            if (action.indexOf("scenery3d_") === 0) {
                var propName = "Scenery3d." + action.substring(10);
                var currentValue = propApi.getStelProp(propName);
                if (currentValue !== undefined) {
                    propApi.setStelPropQueued(propName, !currentValue);
                }
                return;
            }
            
            if (action.indexOf("ArchaeoLines.") === 0) {
                var currentValue = propApi.getStelProp(action);
                if (currentValue !== undefined) {
                    propApi.setStelPropQueued(action, !currentValue);
                }
                return;
            }
            
            // Default: use actions API for StelActions
            if (action && action.indexOf("action") === 0 && actions && typeof actions.execute === 'function') {
                actions.execute(action);
            }

						// Clear all constellation highlighting
						if (action === "clear_constellation_highlight") {
								this.clearConstellationHighlight();
								return;
						}

						// Toggle all constellation lines (not just isolated ones)
						if (action === "toggle_constellation_lines") {
								actions.execute("actionShow_Constellation_Lines");
								return;
						}

						// Toggle all constellation labels
						if (action === "toggle_constellation_labels") {
								actions.execute("actionShow_Constellation_Labels");
								return;
						}
						
						// Milky Way display toggle
						if (action === "milky_way_toggle") {
								var isDisplayed = propApi.getStelProp("MilkyWay.flagMilkyWayDisplayed");
								if (isDisplayed !== undefined) {
										propApi.setStelPropQueued("MilkyWay.flagMilkyWayDisplayed", !isDisplayed);
								}
								return;
						}
						if (action === "milky_way_show") {
								propApi.setStelPropQueued("MilkyWay.flagMilkyWayDisplayed", true);
								return;
						}
						if (action === "milky_way_hide") {
								propApi.setStelPropQueued("MilkyWay.flagMilkyWayDisplayed", false);
								return;
						}
						if (action === "milky_way_brightness_up") {
								var current = propApi.getStelProp("MilkyWay.intensity");
								if (current !== undefined) {
										propApi.setStelPropQueued("MilkyWay.intensity", Math.min(10, current + 0.5));
								}
								return;
						}
						if (action === "milky_way_brightness_down") {
								var current = propApi.getStelProp("MilkyWay.intensity");
								if (current !== undefined) {
										propApi.setStelPropQueued("MilkyWay.intensity", Math.max(0, current - 0.5));
								}
								return;
						}
        };

        GamepadDevice.prototype.processInput = function() {
            var gp = navigator.getGamepads()[this.index];
            if (!gp) return;
            this.gamepad = gp;

            var leftX = gp.axes[0] || 0;
            var leftY = gp.axes[1] || 0;
            var rightX = gp.axes[2] || 0;
            var rightY = gp.axes[3] || 0;

            if (this.calibrationData && this.calibrationData.isCalibrated) {
                leftX = Math.max(-1, Math.min(1, leftX - this.calibrationData.leftX));
                leftY = Math.max(-1, Math.min(1, leftY - this.calibrationData.leftY));
                rightX = Math.max(-1, Math.min(1, rightX - this.calibrationData.rightX));
                rightY = Math.max(-1, Math.min(1, rightY - this.calibrationData.rightY));
            }

            leftX = Math.abs(leftX) > controllerSettings.deadzone ? leftX : 0;
            leftY = Math.abs(leftY) > controllerSettings.deadzone ? leftY : 0;
            rightY = Math.abs(rightY) > controllerSettings.deadzone ? rightY : 0;

            if (controllerSettings.invertX) leftX = -leftX;
            if (controllerSettings.invertY) leftY = -leftY;

            leftX *= controllerSettings.sensitivity;
            leftY *= controllerSettings.sensitivity;

            if (leftX !== this.lastLeftX || leftY !== this.lastLeftY) {
                this.lastLeftX = leftX;
                this.lastLeftY = leftY;

                if (leftX !== 0 || leftY !== 0) {
                    var moveX = controllerSettings.movementSpeed * leftX * leftX * leftX;
                    var moveY = controllerSettings.movementSpeed * leftY * leftY * leftY;
                    this.sendMovement(moveX, -moveY);
                    this.isMoving = true;
                } else if (this.isMoving) {
                    this.sendMovement(0, 0);
                    this.isMoving = false;
                }
            }
						
						// update direction display
						if (this.index === activeDeviceIndex && $currentDirection && $currentDirection.length) {
								var direction = this.updateCurrentDirection(leftX, leftY);
								if (direction) {
										$currentDirection.text(direction);
								} else if (this.isMoving === false) {
										$currentDirection.text("centered");
								}
						}

            // Zoom with corrected direction: pull UP = zoom IN, DOWN = zoom OUT
						// Zoom control with right stick - using same range as viewcontrol.js
						if (Math.abs(rightY) > 0.0005) {
								var zoomInput = rightY;
								if (controllerSettings.zoomInvertY) {
										zoomInput = -zoomInput;
								}
								
								// Calculate new FOV using cubic curve for smooth response
								var zoomFactor = 1 - (Math.pow(-zoomInput, 3) * controllerSettings.zoomSpeed * 2);
								var newFov = currentFov * zoomFactor;
								
								// Use the same range as viewcontrol.js (0.001389° to 360°)
								var minFovAllowed = 0.001389;
								var maxFovAllowed = 360;
								newFov = Math.max(minFovAllowed, Math.min(maxFovAllowed, newFov));
								
								// Prevent micro-updates that cause judder
								if (Math.abs(newFov - currentFov) > 0.00005) {
										currentFov = newFov;
										
										// Update via viewcontrol API - this automatically updates the slider
										if (viewcontrol && typeof viewcontrol.setFOV === 'function') {
												viewcontrol.setFOV(newFov);
										}
										
										// Update local display
										updateFovDisplay(currentFov);
										
										// Update the main FOV text in view tab directly
										var $viewFovText = $("#view_fov_text");
										if ($viewFovText && $viewFovText.length) {
												$viewFovText.text(currentFov.toPrecision(5));
										}
								}
						}

            for (var i = 0; i < gp.buttons.length; i++) {
                var button = gp.buttons[i];
                var isPressedNow = button.pressed;
                var wasPressed = this.previousButtonStates.get(i) || false;
                var finalPressed = isPressedNow || (button.value !== undefined && button.value > 0.1);

                if (finalPressed && !wasPressed) {
                    var action = this.getButtonAction(i);
                    if (action && action !== "none") {
                        this.handleButtonAction(action);
                    }
                }

                this.previousButtonStates.set(i, finalPressed);
            }
						
						// Update live input display if this is the active device
						if (this.index === activeDeviceIndex) {
								updateStickPreview("left", leftX, leftY);
								updateStickPreview("right", rightX, rightY);
								updateLiveInputDisplay();
						}
        };
				
				// updat current direction 
				GamepadDevice.prototype.updateCurrentDirection = function(leftX, leftY) {
						if (Math.abs(leftX) < 0.05 && Math.abs(leftY) < 0.05) {
								return;
						}
						
						var angle = Math.atan2(-leftY, leftX) * (180 / Math.PI);
						var direction = "";
						
						if (angle >= -22.5 && angle < 22.5) direction = "East →";
						else if (angle >= 22.5 && angle < 67.5) direction = "North-East ↗";
						else if (angle >= 67.5 && angle < 112.5) direction = "North ↑";
						else if (angle >= 112.5 && angle < 157.5) direction = "North-West ↖";
						else if (angle >= 157.5 || angle < -157.5) direction = "West ←";
						else if (angle >= -157.5 && angle < -112.5) direction = "South-West ↙";
						else if (angle >= -112.5 && angle < -67.5) direction = "South ↓";
						else if (angle >= -67.5 && angle < -22.5) direction = "South-East ↘";
						
						var intensity = Math.sqrt(leftX * leftX + leftY * leftY);
						var speedText = intensity > 0.8 ? "fast" : (intensity > 0.3 ? "medium" : "slow");
						
						return direction + " (" + speedText + ")";
				};

        GamepadDevice.prototype.sendMovement = function(x, y) {
            if (viewcontrol && typeof viewcontrol.move === 'function') {
                viewcontrol.move(x, y);
            } else {
                $.ajax({ url: "/api/main/move", type: "POST", data: { x: x, y: y } });
            }
        };

        GamepadDevice.prototype.getButtonName = function(index) {
            return BUTTON_NAMES[index] || ("Button " + index);
        };

        GamepadDevice.prototype.getButtonCount = function() {
            return this.gamepad ? this.gamepad.buttons.length : 0;
        };

        GamepadDevice.prototype.getDisplayInfo = function() {
            return {
                name: this.mappings.name || "Unknown Device",
                model: this.mappings.model || "Unknown Model",
                vendor: this.mappings.vendorName || (this.vendorId ? "Vendor: " + this.vendorId : "Generic"),
                id: this.id,
                uniqueId: this.uniqueId,
                index: this.index
            };
        };

        // =====================================================================
        // SECTION 6: GAMEPAD MANAGER
        // =====================================================================

        var GamepadManager = function() {
            this.devices = new Map();
            this.deviceList = [];
            this.init();
        };
				
				// Add this to the GamepadManager constructor or init method
				GamepadManager.prototype.startBackgroundScan = function(intervalMs) {
						var self = this;
						intervalMs = intervalMs || 3000; // Default 3 seconds
						
						if (this.scanInterval) {
								clearInterval(this.scanInterval);
						}
						
						this.scanInterval = setInterval(function() {
								if (document.hasFocus()) { // Only scan when tab is active
										self.scanForGamepads();
								}
						}, intervalMs);
				};

				GamepadManager.prototype.stopBackgroundScan = function() {
						if (this.scanInterval) {
								clearInterval(this.scanInterval);
								this.scanInterval = null;
						}
				};

        GamepadManager.prototype.init = function() {
            this.setupEventListeners();
            this.scanForGamepads();
            this.startPolling();
        };

        GamepadManager.prototype.setupEventListeners = function() {
            var self = this;
            
            window.addEventListener("gamepadconnected", function(e) {
                console.log("[Gamepad] Connected: " + e.gamepad.id);
                if (!self.devices.has(e.gamepad.index)) {
                    var device = new GamepadDevice(e.gamepad.index, e.gamepad);
                    self.devices.set(e.gamepad.index, device);
                    self.updateDeviceList();
                    
                    if (self.devices.size === 1) {
                        setActiveDevice(e.gamepad.index);
                    }
                    
                    updateDeviceSelector();
                    updateConnectionStatus(true, device);
                    populateButtonCustomization();
                }
            });

            window.addEventListener("gamepaddisconnected", function(e) {
                console.log("[Gamepad] Disconnected: " + e.gamepad.id);
                var device = self.devices.get(e.gamepad.index);
                if (device) {
                    self.devices.delete(e.gamepad.index);
                    self.updateDeviceList();
                    
                    if (activeDeviceIndex === e.gamepad.index) {
                        var nextDevice = self.devices.size > 0 ? self.devices.values().next().value : null;
                        if (nextDevice) {
                            setActiveDevice(nextDevice.index);
                        } else {
                            setActiveDevice(null);
                            updateConnectionStatus(false);
														updateLiveInputDisplay();
                        }
                    }
                    
                    updateDeviceSelector();
                    populateButtonCustomization();
                }
            });
        };

        GamepadManager.prototype.updateDeviceList = function() {
            this.deviceList = Array.from(this.devices.values());
        };

        GamepadManager.prototype.scanForGamepads = function() {
            var gamepads = navigator.getGamepads ? navigator.getGamepads() : [];
            for (var i = 0; i < gamepads.length; i++) {
                var gp = gamepads[i];
                if (gp && !this.devices.has(i)) {
                    console.log("[Gamepad] Found existing: " + gp.id);
                    var device = new GamepadDevice(i, gp);
                    this.devices.set(i, device);
                }
            }
            this.updateDeviceList();
            
            if (this.devices.size > 0 && activeDeviceIndex === null) {
                var firstDevice = this.devices.values().next().value;
                if (firstDevice) {
                    setActiveDevice(firstDevice.index);
                }
            } else if (this.devices.size === 0) {
                setActiveDevice(null);
                updateConnectionStatus(false);
            }
            
            updateDeviceSelector();
        };

        GamepadManager.prototype.startPolling = function() {
            if (animationFrame) cancelAnimationFrame(animationFrame);
            isPolling = true;

            var self = this;
            var poll = function() {
                if (isPolling && !rc.isConnectionLost()) {
                    for (var device of self.devices.values()) {
                        device.processInput();
                    }
                }
                animationFrame = requestAnimationFrame(poll);
            };
            poll();
            console.log("[Gamepad] Polling started.");
        };

        GamepadManager.prototype.stopPolling = function() {
            isPolling = false;
            if (animationFrame) {
                cancelAnimationFrame(animationFrame);
                animationFrame = null;
            }
            console.log("[Gamepad] Polling stopped.");
        };

        GamepadManager.prototype.getActiveDevice = function() {
            if (activeDeviceIndex !== null && this.devices.has(activeDeviceIndex)) {
                return this.devices.get(activeDeviceIndex);
            }
            return this.devices.size > 0 ? this.devices.values().next().value : null;
        };

        GamepadManager.prototype.getDevice = function(index) {
            return this.devices.get(index) || null;
        };

        GamepadManager.prototype.getAllDevices = function() {
            return Array.from(this.devices.values());
        };

        // =====================================================================
        // SECTION 7: UI UPDATE FUNCTIONS
        // =====================================================================

        function updateConnectionStatus(connected, device) {
            if ($connectionStatus && $connectionStatus.length) {
                $connectionStatus.html(connected ? '<span style="color:#4CAF50;">● Connected</span>' : '<span style="color:#f44336;">● Disconnected</span>');
            }
            
            if (connected && device && $deviceName && $deviceName.length) {
                var info = device.getDisplayInfo();
                $deviceName.text(info.name);
                if ($deviceModel && $deviceModel.length) $deviceModel.text(info.model || "-");
                if ($deviceVendor && $deviceVendor.length) $deviceVendor.text(info.vendor || "-");
                if ($deviceId && $deviceId.length) $deviceId.text(info.id || "-");
            } else if (!connected) {
                if ($deviceName && $deviceName.length) $deviceName.text("-");
                if ($deviceModel && $deviceModel.length) $deviceModel.text("-");
                if ($deviceVendor && $deviceVendor.length) $deviceVendor.text("-");
                if ($deviceId && $deviceId.length) $deviceId.text("-");
            }
        }

        function updateDeviceSelector() {
            if (!$deviceSelector || !$deviceSelector.length) return;
            
            var devices = gamepadManager ? gamepadManager.getAllDevices() : [];
            $deviceSelector.empty();
            
            if (devices.length === 0) {
                $deviceSelector.append('<option value="">' + _tr("No devices connected") + '</option>');
                $deviceSelector.prop('disabled', true);
                return;
            }
            
            $deviceSelector.prop('disabled', false);
            
            for (var i = 0; i < devices.length; i++) {
                var device = devices[i];
                var info = device.getDisplayInfo();
                var selected = (device.index === activeDeviceIndex);
                $deviceSelector.append('<option value="' + device.index + '"' + (selected ? ' selected' : '') + '>' + 
                    info.name + ' (' + (info.model || "Generic") + ')</option>');
            }
        }

				function setActiveDevice(index) {
						if (index !== null && index !== undefined && gamepadManager && gamepadManager.getDevice(index)) {
								activeDeviceIndex = index;
								currentDeviceInfo = gamepadManager.getDevice(index);
								updateConnectionStatus(true, currentDeviceInfo);
								updateDeviceSelector();
								populateButtonCustomization();
								updateCalibrationDisplayForDevice(currentDeviceInfo);
								updateLiveInputDisplay();
								
								// Refresh FOV display when switching devices - use viewcontrol range
								if (viewcontrol && typeof viewcontrol.getFOV === 'function') {
										currentFov = Math.max(0.001389, Math.min(360, viewcontrol.getFOV()));
										updateFovDisplay(currentFov);
								}
						} else {
								activeDeviceIndex = null;
								currentDeviceInfo = null;
								updateConnectionStatus(false);
								updateDeviceSelector();
								populateButtonCustomization();
								updateLiveInputDisplay();
						}
				}

				/**
				 * Updates the FOV display in the gamepad panel and synchronizes with the main FOV slider.
				 * Uses the same mathematical transformation as viewcontrol.js to ensure perfect synchronization.
				 * 
				 * @param {number} fov - Field of view in degrees (range: 0.001389 to 360)
				 */
				function updateFovDisplay(fov) {
						// Update text display in gamepad panel
						if ($currentFov && $currentFov.length) {
								var displayFov = isNaN(fov) ? 60 : Math.max(0.001389, Math.min(360, fov));
								$currentFov.text(displayFov.toFixed(2) + "°");
						}
						
						// Update the main FOV text in view tab (for immediate feedback)
								var $viewFovText = $("#view_fov_text");
								if ($viewFovText && $viewFovText.length) {
										$viewFovText.text(fov.toPrecision(3));
								}
						
						// Synchronize with main FOV slider in view tab using identical transformation
						if ($viewFovSlider && $viewFovSlider.length && $viewFovSlider.slider) {
								var minVal = $viewFovSlider.slider("option", "min") || 0;
								var maxVal = $viewFovSlider.slider("option", "max") || 1000;
								var fovSteps = maxVal - minVal;
								
								// Constants matching viewcontrol.js
								var minFovActual = 0.001389;
								var maxFovActual = 360;
								
								// Clamp fov to valid range
								var clampedFov = Math.max(minFovActual, Math.min(maxFovActual, fov));
								
								// Apply the same formula as viewcontrol.js: 
								// t = (fov - min) / (max - min)
								// val = t^(1/4)
								// sliderValue = fovSteps - (val * fovSteps)
								var t = (clampedFov - minFovActual) / (maxFovActual - minFovActual);
								t = Math.max(0, Math.min(1, t));
								
								var val = Math.pow(t, 0.25);
								var sliderValue = Math.round(val * fovSteps);
								var finalValue = fovSteps - sliderValue;
								finalValue = Math.max(minVal, Math.min(maxVal, finalValue));
								
								var currentSliderValue = $viewFovSlider.slider("value");
								if (Math.abs(currentSliderValue - finalValue) > 1) {
										$viewFovSlider.slider("value", finalValue);
								}
						}
				}

				/**
				 * Updates the joystick preview canvas with current stick position.
				 * Draws crosshairs, deadzone circle, and a gradient-filled cursor.
				 * For the right stick, applies zoom inversion if enabled (visual only).
				 * 
				 * @param {string} stick - Which stick to update ("left" or "right")
				 * @param {number} x - X axis value (-1 to 1)
				 * @param {number} y - Y axis value (-1 to 1)
				 */
				function updateStickPreview(stick, x, y) {
						var canvas = stick === "left" ? $leftStickCanvas : $rightStickCanvas;
						if (!canvas || !canvas.length) return;
						
						// Apply zoom inversion to right stick Y-axis for visual display only
						var displayX = x;
						var displayY = y;
						
						if (stick === "right" && controllerSettings.zoomInvertY) {
								displayY = -displayY;
						}

						var ctx = canvas[0].getContext("2d");
						var w = canvas.width();
						var h = canvas.height();

						ctx.clearRect(0, 0, w, h);
						ctx.fillStyle = "#222";
						ctx.fillRect(0, 0, w, h);
						ctx.strokeStyle = "#666";
						ctx.lineWidth = 1;
						ctx.beginPath();
						ctx.moveTo(w/2, 0);
						ctx.lineTo(w/2, h);
						ctx.stroke();
						ctx.beginPath();
						ctx.moveTo(0, h/2);
						ctx.lineTo(w, h/2);
						ctx.stroke();

						ctx.strokeStyle = "#888";
						ctx.setLineDash([2, 2]);
						ctx.beginPath();
						ctx.arc(w/2, h/2, (w/2) * controllerSettings.deadzone, 0, Math.PI * 2);
						ctx.stroke();
						ctx.setLineDash([]);

						ctx.fillStyle = "#666";
						ctx.beginPath();
						ctx.arc(w/2, h/2, 3, 0, Math.PI * 2);
						ctx.fill();

						var stickX = w/2 + (displayX * (w/2 - 8));
						var stickY = h/2 + (displayY * (h/2 - 8));
						var gradient = ctx.createRadialGradient(stickX, stickY, 2, stickX, stickY, 8);
						gradient.addColorStop(0, "#6B6E70");
						gradient.addColorStop(1, "#3A3C3E");
						ctx.fillStyle = gradient;
						ctx.beginPath();
						ctx.arc(stickX, stickY, 6, 0, Math.PI * 2);
						ctx.fill();
				}

        function populateButtonCustomization() {
            if (!$buttonCustomizationContainer || !$buttonCustomizationContainer.length) return;

            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            $buttonCustomizationContainer.empty();

            if (!device) {
                $buttonCustomizationContainer.html('<div class="no-device-message"><p>🔴 ' + _tr("Connect a controller to customize buttons") + '</p></div>');
                return;
            }

            var buttonCount = device.getButtonCount();
            console.log("[UI] Populating customization for device: " + device.getDisplayInfo().name + " with " + buttonCount + " buttons");

            var buttonGroups = {
                "Face Buttons": [0, 1, 2, 3],
                "Shoulder & Triggers": [4, 5, 6, 7],
                "System Buttons": [8, 9, 16, 17],
                "Stick Buttons": [10, 11],
                "D-Pad": [12, 13, 14, 15]
            };

            var groupsContainer = $("<div class='button-groups-container'></div>");

            for (var groupName in buttonGroups) {
                var buttonIndices = buttonGroups[groupName];
                var groupDiv = $("<div class='button-group'></div>");
                groupDiv.append("<div class='button-group-header'>" + groupName + "</div>");

                for (var j = 0; j < buttonIndices.length; j++) {
                    var buttonId = buttonIndices[j];
                    if (buttonId >= buttonCount) continue;

                    var buttonName = device.getButtonName(buttonId);
                    var currentAction = device.getButtonAction(buttonId);

                    var itemDiv = $("<div class='button-item'></div>");
                    itemDiv.append("<span class='button-name'>" + buttonName + "</span>");

                    var select = $("<select class='button-select'></select>");
                    select.data("button-id", buttonId);
                    select.data("device-index", device.index);

                    select.append("<option value='none'>" + _tr("No action") + "</option>");

                    for (var category in ACTION_CATEGORIES) {
                        var actionsList = ACTION_CATEGORIES[category];
                        if (!actionsList || actionsList.length === 0) continue;
                        
                        var optgroup = $("<optgroup>").attr("label", category);
                        for (var k = 0; k < actionsList.length; k++) {
                            var action = actionsList[k];
                            if (AVAILABLE_ACTIONS[action]) {
                                var option = $("<option>").val(action).text(AVAILABLE_ACTIONS[action]);
                                if (currentAction === action) {
                                    option.attr("selected", "selected");
                                }
                                optgroup.append(option);
                            }
                        }
                        if (optgroup.children().length > 0) {
                            select.append(optgroup);
                        }
                    }

                    select.on("change", (function(btnId, btnName) {
                        return function() {
                            var newAction = $(this).val();
                            var dev = gamepadManager ? gamepadManager.getActiveDevice() : null;
                            if (dev) {
                                dev.setButtonAction(btnId, newAction);
                                showNotification(_tr("Button ") + btnName + _tr(" mapped to ") + (AVAILABLE_ACTIONS[newAction] || _tr("No action")));
                            }
                        };
                    })(buttonId, buttonName));

                    itemDiv.append(select);
                    groupDiv.append(itemDiv);
                }
                groupsContainer.append(groupDiv);
            }
            $buttonCustomizationContainer.append(groupsContainer);
        }

        function showNotification(message) {
            var $notification = $('<div class="notification-message" style="position:fixed;top:20px;right:20px;padding:10px 15px;background:linear-gradient(#6B6E70, #3A3C3E);color:#fff;border-radius:5px;z-index:9999;box-shadow:0 2px 10px rgba(0,0,0,0.3);">' + message + '</div>');
            $("body").append($notification);
            setTimeout(function() {
                $notification.fadeOut(function() {
                    $notification.remove();
                });
            }, 2000);
        }

        // =====================================================================
        // SECTION 8: CALIBRATION
        // =====================================================================

        function startCalibration() {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (!device) {
                showNotification(_tr("Please connect a controller first"));
                return;
            }

            calibrationData.calibrationMode = true;
            calibrationData.samples = [];
            calibrationData.sampleCount = 0;
            calibrationData.activeDeviceIndex = device.index;
            
            device.calibrationData = {
                leftX: 0, leftY: 0, rightX: 0, rightY: 0,
                isCalibrated: false
            };

            if ($calibrateBtn) $calibrateBtn.text(_tr("Calibrating...")).addClass("calibrating");
            showNotification(_tr("Move both joysticks in circles, then press any button to finish"));
            updateCalibrationDisplayForDevice(device, true);

            var collecting = true;
            var startTime = Date.now();
            var timeout = 10000;
            
            var collectSamples = function() {
                if (!calibrationData.calibrationMode) {
                    collecting = false;
                    return;
                }
                
                if (Date.now() - startTime > timeout) {
                    finishCalibration();
                    collecting = false;
                    return;
                }
                
                var gp = navigator.getGamepads()[device.index];
                if (gp) {
                    calibrationData.samples.push({
                        leftX: gp.axes[0] || 0,
                        leftY: gp.axes[1] || 0,
                        rightX: gp.axes[2] || 0,
                        rightY: gp.axes[3] || 0
                    });
                    calibrationData.sampleCount++;
                    updateCalibrationDisplayForDevice(device, true);

                    for (var i = 0; i < gp.buttons.length; i++) {
                        if (gp.buttons[i].pressed) {
                            finishCalibration();
                            collecting = false;
                            return;
                        }
                    }
                }
                if (collecting) {
                    requestAnimationFrame(collectSamples);
                }
            };
            requestAnimationFrame(collectSamples);
        }

        function finishCalibration() {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (!device) {
                calibrationData.calibrationMode = false;
                if ($calibrateBtn) $calibrateBtn.text(_tr("Calibrate")).removeClass("calibrating");
                return;
            }
            
            if (!calibrationData.calibrationMode || calibrationData.samples.length === 0) {
                calibrationData.calibrationMode = false;
                if ($calibrateBtn) $calibrateBtn.text(_tr("Calibrate")).removeClass("calibrating");
                return;
            }

            var sumLeftX = 0, sumLeftY = 0, sumRightX = 0, sumRightY = 0;
            for (var i = 0; i < calibrationData.samples.length; i++) {
                var sample = calibrationData.samples[i];
                sumLeftX += sample.leftX;
                sumLeftY += sample.leftY;
                sumRightX += sample.rightX;
                sumRightY += sample.rightY;
            }

            var calibration = {
                leftX: sumLeftX / calibrationData.samples.length,
                leftY: sumLeftY / calibrationData.samples.length,
                rightX: sumRightX / calibrationData.samples.length,
                rightY: sumRightY / calibrationData.samples.length,
                isCalibrated: true
            };
            
            device.calibrationData = calibration;
            device.saveCalibration();
            
            calibrationData.calibrationMode = false;

            if ($calibrateBtn) $calibrateBtn.text(_tr("Calibrate")).removeClass("calibrating");
            showNotification(_tr("Calibration complete! ") + calibrationData.samples.length + _tr(" samples collected."));
            updateCalibrationDisplayForDevice(device);
        }

        function updateCalibrationDisplayForDevice(device, isCalibrating) {
            var container = $("#gp-calibration-values");
            if (!container.length) return;

            if (isCalibrating) {
                container.html("<div class='calibration-status'>" + _tr("Calibrating... Collected ") + calibrationData.sampleCount + _tr(" samples<br><span style='font-size:10px;'>Move joysticks in circles, press any button to finish</span></div>"));
            } else if (device && device.calibrationData && device.calibrationData.isCalibrated) {
                var data = device.calibrationData;
                container.html(
                    "<div class='calibration-values'>" +
                        "<div class='calibration-value-item'><span class='calibration-value-label'>" + _tr("Left X offset:") + "</span> <span class='calibration-value-number'>" + data.leftX.toFixed(4) + "</span></div>" +
                        "<div class='calibration-value-item'><span class='calibration-value-label'>" + _tr("Left Y offset:") + "</span> <span class='calibration-value-number'>" + data.leftY.toFixed(4) + "</span></div>" +
                        "<div class='calibration-value-item'><span class='calibration-value-label'>" + _tr("Right X offset:") + "</span> <span class='calibration-value-number'>" + data.rightX.toFixed(4) + "</span></div>" +
                        "<div class='calibration-value-item'><span class='calibration-value-label'>" + _tr("Right Y offset:") + "</span> <span class='calibration-value-number'>" + data.rightY.toFixed(4) + "</span></div>" +
                    "</div>"
                );
            } else {
                container.html("<div class='calibration-status'>" + _tr("Not calibrated") + "</div>");
            }
        }

        function resetCalibration() {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (device) {
                device.calibrationData = {
                    leftX: 0, leftY: 0, rightX: 0, rightY: 0,
                    isCalibrated: false
                };
                device.saveCalibration();
                updateCalibrationDisplayForDevice(device);
                showNotification(_tr("Calibration reset for ") + device.getDisplayInfo().name);
            }
        }

        // =====================================================================
        // SECTION 9: SETTINGS MANAGEMENT
        // =====================================================================

				function loadSettings() {
						try {
								var saved = localStorage.getItem("gamepad_controller_settings");
								if (saved) {
										var parsed = JSON.parse(saved);
										for (var key in parsed) {
												if (controllerSettings.hasOwnProperty(key)) {
														controllerSettings[key] = parsed[key];
												}
										}
								}
								
								// Default values for new settings
								if (controllerSettings.vibrationFeedback === undefined) {
										controllerSettings.vibrationFeedback = false;
								}
								if (controllerSettings.vibrationIntensity === undefined) {
										controllerSettings.vibrationIntensity = 0.5;
								}
								if (controllerSettings.zoomInvertY === undefined) {
										controllerSettings.zoomInvertY = false;
								}
						} catch(e) {
								console.warn("Could not load settings", e);
						}
				}

				function saveSettings() {
						try {
								localStorage.setItem("gamepad_controller_settings", JSON.stringify(controllerSettings));
								} catch(e) {
								console.warn("Could not save settings", e);
						}
				}

        function resetButtonMappings() {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (device) {
                device.buttonMappings.clear();
                device.saveButtonMappings();
                populateButtonCustomization();
                showNotification(_tr("Button mappings reset to defaults for ") + device.getDisplayInfo().name);
            } else {
                showNotification(_tr("No controller connected"));
            }
        }

        function exportProfile() {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (!device) {
                showNotification(_tr("No controller connected"));
                return;
            }
            
            var profile = device.exportProfile();
            var profileName = device.uniqueId + "_profile.json";
            var dataStr = JSON.stringify(profile, null, 2);
            var dataUri = 'data:application/json;charset=utf-8,'+ encodeURIComponent(dataStr);
            
            var linkElement = document.createElement('a');
            linkElement.setAttribute('href', dataUri);
            linkElement.setAttribute('download', profileName);
            linkElement.click();
            
            showNotification(_tr("Profile exported: ") + profileName);
        }

        function importProfile(file) {
            var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
            if (!device) {
                showNotification(_tr("No controller connected"));
                return;
            }
            
            var reader = new FileReader();
            reader.onload = function(e) {
                try {
                    var profile = JSON.parse(e.target.result);
                    if (device.importProfile(profile)) {
                        populateButtonCustomization();
                        updateCalibrationDisplayForDevice(device);
                        showNotification(_tr("Profile imported successfully for ") + device.getDisplayInfo().name);
                    } else {
                        showNotification(_tr("Failed to import profile"));
                    }
                } catch(err) {
                    console.error("Import error:", err);
                    showNotification(_tr("Invalid profile file"));
                }
            };
            reader.readAsText(file);
        }

        // =====================================================================
        // SECTION 10: UI SETUP
        // =====================================================================

        function setupUI() {
            $deviceName = $("#gp-device-name");
            $deviceModel = $("#gp-device-model");
            $deviceVendor = $("#gp-device-vendor");
            $deviceId = $("#gp-device-id");
            $deviceSelector = $("#gp-device-selector");
            $connectionStatus = $("#gp-connection-status");
            $currentFov = $("#gp-current-fov");
            $leftStickCanvas = $("#gp-left-stick-canvas");
            $rightStickCanvas = $("#gp-right-stick-canvas");
            $sensitivitySlider = $("#gp-sensitivity");
            $deadzoneSlider = $("#gp-deadzone");
            $zoomSpeedSlider = $("#gp-zoom-speed");
            $movementSpeedSlider = $("#gp-movement-speed");
            $invertX = $("#gp-invert-x");
            $invertY = $("#gp-invert-y");
						$zoomInvertY = $("#zoom-invert-y");
						$currentDirection = $("#gp-current-direction");
            $calibrateBtn = $("#gp-calibrate");
            $testVibrationBtn = $("#gp-test-vibration");
						 // $vibrationFeedback = $("#gp-vibration-feedback");
            $buttonCustomizationContainer = $("#gp-button-customization");
						$viewFovSlider = $("#view_fov");
						
						// New vibration intensity slider
						var $vibrationIntensityRow = $("#gp-vibration-intensity-row");
						var $vibrationIntensity = $("#gp-vibration-intensity");
						var $vibrationIntensityValue = $("#gp-vibration-intensity-value");
						
						if ($vibrationIntensity.length) {
								$vibrationIntensity.slider({
										min: 0.0,
										max: 1.0,
										step: 0.05,
										value: controllerSettings.vibrationIntensity,
										slide: function(e, ui) {
												controllerSettings.vibrationIntensity = ui.value;
												$vibrationIntensityValue.text(ui.value.toFixed(2));
												saveSettings();
												
												// Optional: quick test vibration to demonstrate intensity
												if (controllerSettings.vibrationFeedback && ui.value > 0) {
														triggerVibration(50, ui.value);
												}
										}
								});
						}
						
						// Show/hide intensity slider based on vibration feedback checkbox
						var $vibrationFeedback = $("#gp-vibration-feedback");
						if ($vibrationFeedback.length) {
								$vibrationFeedback.prop("checked", controllerSettings.vibrationFeedback).on("change", function() {
										controllerSettings.vibrationFeedback = $(this).is(":checked");
										saveSettings();
										
										// Show/hide intensity slider
										if ($vibrationIntensityRow.length) {
												if (controllerSettings.vibrationFeedback) {
														$vibrationIntensityRow.show();
												} else {
														$vibrationIntensityRow.hide();
												}
										}
										
										showNotification(controllerSettings.vibrationFeedback ? 
												_tr("Vibration feedback enabled") : _tr("Vibration feedback disabled"));
								});
								
								// Set initial visibility
								if ($vibrationIntensityRow.length) {
										if (controllerSettings.vibrationFeedback) {
												$vibrationIntensityRow.show();
										} else {
												$vibrationIntensityRow.hide();
										}
								}
						}

            if ($deviceSelector && $deviceSelector.length) {
                $deviceSelector.on("change", function() {
                    var index = parseInt($(this).val());
                    if (!isNaN(index)) {
                        setActiveDevice(index);
                    }
                });
            }

            if ($sensitivitySlider.length) {
                $sensitivitySlider.slider({
                    min: 0.1, max: 3.0, step: 0.1, value: controllerSettings.sensitivity,
                    slide: function(e, ui) {
                        controllerSettings.sensitivity = ui.value;
                        $("#gp-sensitivity-value").text(ui.value.toFixed(1));
                        saveSettings();
                    }
                });
            }
            
            if ($deadzoneSlider.length) {
                $deadzoneSlider.slider({
                    min: 0, max: 0.5, step: 0.01, value: controllerSettings.deadzone,
                    slide: function(e, ui) {
                        controllerSettings.deadzone = ui.value;
                        $("#gp-deadzone-value").text(ui.value.toFixed(2));
                        updateStickPreview("left", 0, 0);
                        updateStickPreview("right", 0, 0);
                        saveSettings();
                    }
                });
            }
            
            if ($zoomSpeedSlider.length) {
                $zoomSpeedSlider.slider({
                    min: 0.005, max: 0.1, step: 0.001, value: controllerSettings.zoomSpeed,
                    slide: function(e, ui) {
                        controllerSettings.zoomSpeed = ui.value;
                        $("#gp-zoom-speed-value").text(ui.value.toFixed(2));
                        saveSettings();
                    }
                });
            }
            
            if ($movementSpeedSlider.length) {
                $movementSpeedSlider.slider({
                    min: 0.5, max: 5, step: 0.5, value: controllerSettings.movementSpeed,
                    slide: function(e, ui) {
                        controllerSettings.movementSpeed = ui.value;
                        $("#gp-movement-speed-value").text(ui.value.toFixed(1));
                        saveSettings();
                    }
                });
            }
            
            if ($invertX.length) {
                $invertX.prop("checked", controllerSettings.invertX).on("change", function() {
                    controllerSettings.invertX = $(this).is(":checked");
                    saveSettings();
                });
            }
            if ($invertY.length) {
                $invertY.prop("checked", controllerSettings.invertY).on("change", function() {
                    controllerSettings.invertY = $(this).is(":checked");
                    saveSettings();
                });
            }
						
						// Zoom Invert Y checkbox
						if ($zoomInvertY && $zoomInvertY.length) {
								$zoomInvertY.prop("checked", controllerSettings.zoomInvertY || false).on("change", function() {
										controllerSettings.zoomInvertY = $(this).is(":checked");
										saveSettings();
								});
						}
            
            if ($vibrationFeedback && $vibrationFeedback.length) {
                $vibrationFeedback.prop("checked", controllerSettings.vibrationFeedback).on("change", function() {
                    controllerSettings.vibrationFeedback = $(this).is(":checked");
                    saveSettings();
                    showNotification(controllerSettings.vibrationFeedback ? 
                        _tr("Vibration feedback enabled") : _tr("Vibration feedback disabled"));
                });
            }
            
            if ($calibrateBtn.length) {
                $calibrateBtn.on("click", startCalibration);
            }
            
            $("#gp-reset-calibration").on("click", resetCalibration);
            
            if ($testVibrationBtn && $testVibrationBtn.length) {
                $testVibrationBtn.on("click", testVibrationManual);
            }

            $("#gp-reset-mappings").on("click", resetButtonMappings);
            
            $("#gp-save-mappings").on("click", function() {
                var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
                if (device) {
                    device.saveButtonMappings();
                    showNotification(_tr("Button mappings saved for ") + device.getDisplayInfo().name);
                } else {
                    showNotification(_tr("No controller connected"));
                }
            });
            
            $("#gp-export-profile").on("click", exportProfile);
            
            $("#gp-import-profile").on("click", function() {
                var input = $('<input type="file" accept=".json">');
                input.on('change', function(e) {
                    if (e.target.files.length > 0) {
                        importProfile(e.target.files[0]);
                    }
                });
                input.click();
            });
            
						// ========================================================================
						// ENHANCED SCAN BUTTON - with visual feedback and force refresh
						// ========================================================================
						$("#gp-connect").on("click", function() {
								var $btn = $(this);
								var originalText = $btn.text();
								
								// Show scanning feedback
								$btn.text(_tr("Scanning...")).prop("disabled", true);
								
								// Add scanning animation class
								$btn.addClass("scanning");
								
								// Force scan for gamepads
								if (gamepadManager) {
										// First, manually trigger the gamepadconnected event simulation
										// This ensures we catch any recently connected devices
										var gamepads = navigator.getGamepads();
										var foundNewDevices = false;
										
										for (var i = 0; i < gamepads.length; i++) {
												var gp = gamepads[i];
												if (gp && !gamepadManager.devices.has(i)) {
														console.log("[Gamepad] Manual scan found: " + gp.id);
														var device = new GamepadDevice(i, gp);
														gamepadManager.devices.set(i, device);
														foundNewDevices = true;
												}
										}
										
										if (foundNewDevices) {
												gamepadManager.updateDeviceList();
												if (activeDeviceIndex === null && gamepadManager.devices.size > 0) {
														var firstDevice = gamepadManager.devices.values().next().value;
														if (firstDevice) {
																setActiveDevice(firstDevice.index);
														}
												}
												updateDeviceSelector();
												populateButtonCustomization();
												showNotification(_tr("Found ") + gamepadManager.devices.size + _tr(" controller(s)"));
										} else {
												// Run the standard scan
												gamepadManager.scanForGamepads();
												
												// Check again after scan
												if (gamepadManager.devices.size > 0) {
														showNotification(_tr("Found ") + gamepadManager.devices.size + _tr(" controller(s)"));
												} else {
														showNotification(_tr("No controllers found. Please connect a controller and try again."));
												}
										}
								}
								
								// Reset button after delay
								setTimeout(function() {
										$btn.text(originalText).prop("disabled", false);
										$btn.removeClass("scanning");
								}, 500);
						});

            $("#gp-sensitivity-value").text(controllerSettings.sensitivity.toFixed(1));
            $("#gp-deadzone-value").text(controllerSettings.deadzone.toFixed(2));
            $("#gp-zoom-speed-value").text(controllerSettings.zoomSpeed.toFixed(2));
            $("#gp-movement-speed-value").text(controllerSettings.movementSpeed.toFixed(1));
						
						// Initialize live input tabs
						if ($("#gp-live-input").length) {
								$("#gp-live-input .live-input-tabs").tabs({
										heightStyle: "content"
								});
						}
        }

        function navigateTab(direction) {
            var $tabs = $("#tabs");
            var current = $tabs.tabs("option", "active");
            var maxTabs = $tabs.find(".ui-tabs-nav li").length - 1;
            var newTab = current + direction;
            if (newTab >= 0 && newTab <= maxTabs) {
                $tabs.tabs("option", "active", newTab);
            }
        }
				
				// =====================================================================
				// SECTION: LIVE INPUT PANEL
				// =====================================================================

				/**
				 * Updates the live input display with current controller state
				 * Shows raw button and axis values as reported by the browser
				 */
				function updateLiveInputDisplay() {
						var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
						
						// If no device is connected or active device is null, clear all displays
						if (!device || activeDeviceIndex === null) {
								$('#gp-buttons-container').html('<div class="loading-placeholder">' + _tr("No controller connected") + '</div>');
								$('#gp-axes-container').html('<div class="loading-placeholder">' + _tr("No controller connected") + '</div>');
								$('#gp-device-info-live').html('<div class="loading-placeholder">' + _tr("No controller connected") + '</div>');
								return;
						}
						
						var gp = navigator.getGamepads()[device.index];
						if (!gp) {
								$('#gp-buttons-container').html('<div class="loading-placeholder">' + _tr("Controller data not available") + '</div>');
								$('#gp-axes-container').html('<div class="loading-placeholder">' + _tr("Controller data not available") + '</div>');
								$('#gp-device-info-live').html('<div class="loading-placeholder">' + _tr("Controller data not available") + '</div>');
								return;
						}
						
						// Update displays with valid data
						updateButtonsDisplay(gp);
						updateAxesDisplay(gp);
						updateDeviceInfoLive(gp, device);
				}

				/**
				 * Renders the buttons panel with raw values
				 */
				function updateButtonsDisplay(gamepad) {
						var $container = $('#gp-buttons-container');
						$container.empty();
						
						if (!gamepad || !gamepad.buttons || gamepad.buttons.length === 0) {
								$container.html('<div class="loading-placeholder">' + _tr("No button data available") + '</div>');
								return;
						}
						
						var html = '';
						
						for (var i = 0; i < gamepad.buttons.length; i++) {
								var button = gamepad.buttons[i];
								var value = button.value !== undefined ? button.value : (button.pressed ? 1 : 0);
								var isPressed = button.pressed || value > 0.1;
								var buttonName = currentDeviceInfo ? currentDeviceInfo.getButtonName(i) : ('Btn ' + i);
								var valueClass = isPressed ? 'pressed' : '';
								var displayValue = value.toFixed(3);
								
								// Shorten button name for compact display
								if (buttonName.length > 12) {
										buttonName = buttonName.substring(0, 10) + '..';
								}
								
								html += '<div class="live-input-button">';
								html += '<span class="label">' + escapeHtml(buttonName) + '<span class="raw-id">#' + i + '</span></span>';
								html += '<span class="value ' + valueClass + '">' + displayValue + '</span>';
								html += '</div>';
						}
						
						$container.html(html);
				}
				
				/**
				 * Renders the axes panel with raw values
				 */
				 
				function updateAxesDisplay(gamepad) {
						var $container = $('#gp-axes-container');
						$container.empty();
						
						if (!gamepad || !gamepad.axes || gamepad.axes.length === 0) {
								$container.html('<div class="loading-placeholder">' + _tr("No axis data available") + '</div>');
								return;
						}
						
						// Axis names
						var axisNames = {
								0: 'Left X',
								1: 'Left Y',
								2: 'Right X',
								3: 'Right Y',
								4: 'L2 Trigger',
								5: 'R2 Trigger'
						};
						
						var html = '';
						
						for (var i = 0; i < gamepad.axes.length; i++) {
								var value = gamepad.axes[i];
								var displayValue = value.toFixed(3);
								var axisName = axisNames[i] || ('Axis ' + i);
								
								// Calculate meter fill percentage (0-100%)
								var fillPercent = Math.abs(value) * 100;
								var fillClass = value >= 0 ? 'positive' : 'negative';
								var meterStyle = 'width: ' + fillPercent + '%;';
								
								html += '<div class="live-input-axis">';
								html += '<span class="label">' + escapeHtml(axisName) + '<span class="raw-id">#' + i + '</span></span>';
								html += '<div class="meter-container">';
								html += '<div class="meter-fill ' + fillClass + '" style="' + meterStyle + '"></div>';
								html += '</div>';
								html += '<span class="value">' + displayValue + '</span>';
								html += '</div>';
						}
						
						$container.html(html);
				}

				/**
				 * Updates the device information tab with detailed raw data
				 */
				function updateDeviceInfoLive(gamepad, device) {
						var $container = $('#gp-device-info-live');
						if (!$container.length) return;
						
						var info = device.getDisplayInfo();
						var osInfo = getOperatingSystem();
						var browserInfo = getBrowserInfo();
						
						var html = '';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Device Name") + ':</span><span class="value">' + escapeHtml(info.name) + '</span></div>';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Vendor/Product ID") + ':</span><span class="value">' + (device.vendorId ? device.vendorId + ':' + device.productId : 'Unknown') + '</span></div>';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Button Count") + ':</span><span class="value">' + gamepad.buttons.length + '</span></div>';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Axis Count") + ':</span><span class="value">' + (gamepad.axes ? gamepad.axes.length : 0) + '</span></div>';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Operating System") + ':</span><span class="value">' + osInfo + '</span></div>';
						html += '<div class="device-info-live-row"><span class="label">' + _tr("Browser") + ':</span><span class="value">' + browserInfo + '</span></div>';
						
						// Add browser-specific notes without icons
						if (browserInfo.includes('Firefox') && info.name.includes('DualSense')) {
								html += '<div class="device-info-live-row"><span class="label">' + _tr("Note") + ':</span><span class="value">Firefox may report PS5 DualSense axes differently. Verify using the Axes panel.</span></div>';
						}
						if (browserInfo.includes('Chrome') && info.name.includes('DualSense')) {
								html += '<div class="device-info-live-row"><span class="label">' + _tr("Note") + ':</span><span class="value">Chrome provides full PS5 DualSense support including vibration.</span></div>';
						}
						
						$container.html(html);
				}

				/**
				 * Helper: Get operating system information
				 */
				function getOperatingSystem() {
						var userAgent = navigator.userAgent.toLowerCase();
						if (userAgent.indexOf('win') !== -1) return 'Windows';
						if (userAgent.indexOf('mac') !== -1) return 'macOS';
						if (userAgent.indexOf('linux') !== -1) return 'Linux';
						if (userAgent.indexOf('android') !== -1) return 'Android';
						return 'Unknown';
				}

				/**
				 * Helper: Get browser information
				 */
				function getBrowserInfo() {
						var userAgent = navigator.userAgent;
						if (userAgent.indexOf('Firefox') !== -1) return 'Mozilla Firefox';
						if (userAgent.indexOf('Chrome') !== -1 && userAgent.indexOf('Edg') === -1) return 'Google Chrome';
						if (userAgent.indexOf('Edg') !== -1) return 'Microsoft Edge';
						if (userAgent.indexOf('Safari') !== -1 && userAgent.indexOf('Chrome') === -1) return 'Apple Safari';
						return 'Other';
				}

				/**
				 * Helper: Escape HTML to prevent injection
				 */
				function escapeHtml(str) {
						if (!str) return '';
						return str.replace(/[&<>]/g, function(m) {
								if (m === '&') return '&amp;';
								if (m === '<') return '&lt;';
								if (m === '>') return '&gt;';
								return m;
						});
				}

        // =====================================================================
        // SECTION 11: INITIALIZATION
        // =====================================================================

				/**
				 * Initializes the Gamepad Controller module.
				 * Sets up UI components, event listeners, and starts the polling loop.
				 * Synchronizes FOV values with viewcontrol and properties API.
				 * 
				 * @returns {void}
				 */
				function init() {
						console.log("[Gamepad Controller] Initializing with W3C Gamepad API v2.6.0");
						loadSettings();
						setupUI();

						gamepadManager = new GamepadManager();
						gamepadManager.startBackgroundScan(3000);
						
						loadPluginActions();

						// Get initial FOV value - use the same range as viewcontrol.js
						if (viewcontrol && typeof viewcontrol.getFOV === 'function') {
								var initialFov = viewcontrol.getFOV();
								currentFov = (initialFov !== undefined && initialFov !== null) ? initialFov : 60;
						} else if (propApi) {
								var initialFov = propApi.getStelProp("StelCore.fov");
								currentFov = (initialFov !== undefined && initialFov !== null) ? initialFov : 60;
						}
						// Ensure currentFov is within valid range (0.001389° to 360°)
						currentFov = Math.max(0.001389, Math.min(360, currentFov));
						updateFovDisplay(currentFov);

						// Listen for FOV changes from viewcontrol (triggered when server confirms FOV change)
						if (viewcontrol) {
								$(viewcontrol).on("fovChanged", function(evt, fov) {
										if (Math.abs(fov - currentFov) > 0.0001) {
												currentFov = Math.max(0.001389, Math.min(360, fov));
												updateFovDisplay(currentFov);
										}
								});
						}
						
						// Listen for FOV changes from properties API as fallback
						if (propApi) {
								$(propApi).on("stelPropertyChanged:StelCore.fov", function(evt, data) {
										var newFov = data.value;
										if (Math.abs(newFov - currentFov) > 0.00001) {
												currentFov = Math.max(0.001389, Math.min(360, newFov));
												updateFovDisplay(currentFov);
										}
								});
						}

						// Monitor server connection status and pause/resume polling accordingly
						setInterval(function() {
								var connected = rc && !rc.isConnectionLost();
								if (connected !== isServerConnected) {
										isServerConnected = connected;
										if (!connected) {
												console.warn("[Gamepad] Server connection lost. Stopping polling.");
												if (gamepadManager) gamepadManager.stopPolling();
										} else {
												console.log("[Gamepad] Server connection restored. Resuming polling.");
												if (gamepadManager) gamepadManager.startPolling();
										}
								}
						}, 2000);

						console.log("[Gamepad Controller] Initialization complete.");
				}

        // =====================================================================
        // SECTION 12: PUBLIC API
        // =====================================================================

        return {
            init: init,
            
            getSettings: function() {
                return $.extend(true, {}, controllerSettings);
            },
            
						resetSettings: function() {
								controllerSettings = {
										sensitivity: 1.0,
										deadzone: 0.15,
										invertX: false,
										invertY: false,
										zoomInvertY: false,
										zoomSpeed: 0.05,
										movementSpeed: 5.0,
										vibrationFeedback: false,
										vibrationIntensity: 0.5
								};
								saveSettings();
								// Reset FOV to default value within viewcontrol range
								currentFov = 60;
								if (viewcontrol && typeof viewcontrol.setFOV === 'function') {
										viewcontrol.setFOV(60);
								}
								updateFovDisplay(currentFov);
								location.reload();
						},
            
            startCalibration: startCalibration,
            
            getDeviceCount: function() {
                return gamepadManager ? gamepadManager.getAllDevices().length : 0;
            },
            
            getActiveDevice: function() {
                var device = gamepadManager ? gamepadManager.getActiveDevice() : null;
                return device ? device.getDisplayInfo() : null;
            },
            
            exportProfile: exportProfile,
            
            testVibration: testVibrationManual,
            
            isPluginLoaded: isPluginLoaded,
            
            getLoadedPlugins: function() {
                return $.extend(true, {}, loadedPlugins);
            },
						
						/**
						 * Sets vibration intensity
						 * @param {number} intensity - Value between 0 and 1
						 */
						setVibrationIntensity: function(intensity) {
								controllerSettings.vibrationIntensity = Math.max(0, Math.min(1, intensity));
								saveSettings();
								if ($("#gp-vibration-intensity").length) {
										$("#gp-vibration-intensity").slider("value", controllerSettings.vibrationIntensity);
										$("#gp-vibration-intensity-value").text(controllerSettings.vibrationIntensity.toFixed(2));
								}
						},
						
						/**
						 * Gets current vibration intensity
						 * @returns {number} Current intensity (0-1)
						 */
						getVibrationIntensity: function() {
								return controllerSettings.vibrationIntensity;
						}
        };
    }
);