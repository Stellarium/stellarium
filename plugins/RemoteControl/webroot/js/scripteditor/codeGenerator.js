/* ========================================================================
 * codeGenerator.js - Interactive Code Generator for Stellarium Remote Control
 * ========================================================================
 * 
 * This module provides an interactive code generator that produces cURL,
 * Stellarium Script (.ssc), Python, and JavaScript code for all Remote
 * Control API operations.
 * 
 * Features:
 * - Step-by-step wizard (Category -> Action -> Parameters -> Generate)
 * - Multi-format output: cURL, SSC, Python, JavaScript
 * - Dynamic base URL resolution (works with any host/port)
 * - Quick examples for common operations
 * - Recent codes history with localStorage persistence
 * - Parameter validation and type-aware input fields
 * - Suggestion buttons for locations, actions, properties, scripts
 * - Real-time code generation with syntax highlighting
 * - Direct execution of generated code
 * - Insert generated SSC code into Script Editor
 * - Copy to clipboard support
 * 
 * Technical Implementation:
 * - All 9 API service categories with full operation coverage
 * - Dynamic URL resolution for generated code (localhost or remote)
 * - Parameter filtering (skips empty/optional parameters)
 * - Custom generators for complex operations (vector3, datetime)
 * - Time rate conversion between user value (1 = normal) and server JD_SECOND
 * - Suggestion dropdown with API data fetching
 * - Recent codes saved to localStorage (10 items max)
 * 
 * Data Sources (Stellarium source files):
 * - MainService.cpp, MainService.hpp
 * - ObjectService.cpp, ObjectService.hpp
 * - LocationService.cpp, LocationService.hpp
 * - LocationSearchService.cpp, LocationSearchService.hpp
 * - ScriptService.cpp, ScriptService.hpp
 * - SimbadService.cpp, SimbadService.hpp
 * - StelActionService.cpp, StelActionService.hpp
 * - StelPropertyService.cpp, StelPropertyService.hpp
 * - ViewService.cpp, ViewService.hpp
 * 
 * @module codeGenerator
 * @requires jquery
 * @requires api/remotecontrol
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-06-06
 * @license GPLv2+
 * @version 1.0.0
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol"], 
    function($, rc) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    var currentStep = 1;
    var selectedCategory = null;
    var selectedAction = null;
    var generatedCodes = {};
    var recentCodes = [];
    var maxRecentCodes = 10;

    // =====================================================================
    // CONFIGURATION
    // =====================================================================

    /**
     * Configuration object for the Code Generator.
     * Users can override these values if needed for custom deployments.
     * 
     * @type {Object}
     * @property {boolean} forceHttpsForCurl - Force HTTPS protocol for cURL commands
     * @property {string} customHostname - Custom hostname for cURL (empty = use current)
     * @property {string} customPort - Custom port for cURL (empty = use current)
     * @property {boolean} debugUrls - Log resolved URLs for debugging
     */
    var CodeGeneratorConfig = {
        forceHttpsForCurl: false,
        customHostname: '',
        customPort: '',
        debugUrls: true
    };
		
		// =====================================================================
		// TIME RATE CONVERSION CONSTANTS
		// =====================================================================

		// JD_SECOND: One second expressed in Julian Days (1/86400)
		// This is the value that Stellarium expects for normal real-time speed
		var JD_SECOND = 0.000011574074074074074;

		// User-friendly time rates mapped to server values
		// User value (real seconds per simulation second) -> Server value (JD per real second)
		function convertUserTimeRateToServer(userRate) {
				if (userRate === 0) return 0;
				// Normal speed: 1 real second = 1 simulation second = JD_SECOND JD
				return userRate * JD_SECOND;
		}

		// Convert server timerate back to user-friendly value
		function convertServerTimeRateToUser(serverRate) {
				if (serverRate === 0) return 0;
				return serverRate / JD_SECOND;
		}

    // =====================================================================
    // BASE URL RESOLUTION FUNCTIONS
    // =====================================================================

    /**
     * Get the current base URL for Stellarium Remote Control API.
     * Dynamically resolves the protocol, hostname, and port from the current page.
     * This ensures generated code works regardless of custom port configurations
     * or remote access setups.
     * 
     * @returns {string} The base URL (e.g., "http://localhost:8090")
     */
    function getCurrentBaseUrl() {
        var protocol = window.location.protocol;   // "http:" or "https:"
        var hostname = window.location.hostname;   // "localhost", "192.168.1.100", etc.
        var port = window.location.port;           // "8090" or custom port
        
        // Build the base URL
        var baseUrl = protocol + '//' + hostname;
        if (port) {
            baseUrl += ':' + port;
        }
        
        if (CodeGeneratorConfig.debugUrls) {
            console.log('[CodeGenerator] Base URL resolved:', baseUrl);
        }
        
        return baseUrl;
    }

    /**
     * Get the current base URL for cURL commands.
     * For cURL commands, we use HTTP by default (as Stellarium Remote Control
     * typically uses HTTP, not HTTPS). Users can override via configuration.
     * 
     * If running from a remote machine, localhost won't work, so we use the
     * actual hostname from the current page.
     * 
     * @returns {string} The base URL for cURL commands
     */
    function getCurlBaseUrl() {
        var hostname = CodeGeneratorConfig.customHostname || window.location.hostname;
        var port = CodeGeneratorConfig.customPort || window.location.port;
        var protocol = CodeGeneratorConfig.forceHttpsForCurl ? 'https' : 'http';
        
        var baseUrl = protocol + '://' + hostname;
        if (port && port !== '80' && port !== '443') {
            baseUrl += ':' + port;
        }
        
        if (CodeGeneratorConfig.debugUrls) {
            console.log('[CodeGenerator] cURL Base URL resolved:', baseUrl);
        }
        
        return baseUrl;
    }

    /**
     * Set custom configuration for the Code Generator.
     * Allows users to override hostname, port, or protocol settings.
     * 
     * @param {Object} config - Configuration options
     * @param {boolean} [config.forceHttpsForCurl] - Force HTTPS for cURL
     * @param {string} [config.customHostname] - Custom hostname for cURL
     * @param {string} [config.customPort] - Custom port for cURL
     * @param {boolean} [config.debugUrls] - Enable URL debugging
     */
    function setCodeGeneratorConfig(config) {
        if (config.forceHttpsForCurl !== undefined) {
            CodeGeneratorConfig.forceHttpsForCurl = config.forceHttpsForCurl;
        }
        if (config.customHostname !== undefined) {
            CodeGeneratorConfig.customHostname = config.customHostname;
        }
        if (config.customPort !== undefined) {
            CodeGeneratorConfig.customPort = config.customPort;
        }
        if (config.debugUrls !== undefined) {
            CodeGeneratorConfig.debugUrls = config.debugUrls;
        }
        
        console.log('[CodeGenerator] Configuration updated:', CodeGeneratorConfig);
    }

    // =====================================================================
    // CATEGORIES AND ACTIONS DEFINITIONS
    // Based on Stellarium Remote Control API source files
    // =====================================================================

    var categories = [
        {
            id: 'time',
            name: 'Time & Simulation',
            name_en: 'Time & Simulation',
            icon: 'T',
            desc: 'Control simulation time, date, and speed via MainService',
            source: 'MainService.cpp - POST: time operation',
            actions: [
                {
                    id: 'time-set-jd',
                    name: 'Set Specific Time (Julian Day)',
                    name_en: 'Set Julian Day',
                    desc: 'Change simulation date and time using Julian Day',
                    method: 'POST',
                    path: 'main/time',
                    source: 'MainService.cpp line ~200: time operation',
                    params: {
                        time: {
                            type: 'jday',
                            required: true,
                            desc: 'Julian Day (JD)',
                            hint: 'Example: 2460050.5 (January 1, 2025 noon)',
                            suggest: 'current'
                        },
                        timerate: {
                            type: 'number',
                            required: false,
                            desc: 'Time rate (real seconds per simulation second)',
                            hint: '1 = normal speed, 0 = pause, 3600 = 1 hour/second'
                        }
                    },
                    sscTemplate: 'core.setJD({time});\ncore.setTimeRate({timerate});'
                },
                {
                    id: 'time-rate',
                    name: 'Change Time Speed Only',
                    name_en: 'Set Time Rate',
                    desc: 'Speed up or slow down time flow without changing current time',
                    method: 'POST',
                    path: 'main/time',
                    source: 'MainService.cpp: timerate parameter',
                    params: {
                        timerate: {
                            type: 'number',
                            required: true,
                            desc: 'Time rate (real seconds per simulation second)',
                            hint: '1 = normal, -1 = reverse, 3600 = 1 hour/second, 0 = pause'
                        }
                    },
                    sscTemplate: 'core.setTimeRate({timerate});'
                }
            ]
        },
        {
            id: 'location',
            name: 'Location & Observer',
            name_en: 'Location & Observer',
            icon: 'L',
            desc: 'Change observer location and planet via LocationService and MainService',
            source: 'LocationService.cpp, LocationSearchService.cpp',
            actions: [
                {
                    id: 'location-set-by-id',
                    name: 'Set Location by Name',
                    name_en: 'Set Location by Name',
                    desc: 'Move to a known city or location from Stellarium database',
                    method: 'POST',
                    path: 'location/setlocationfields',
                    source: 'LocationService.cpp: setlocationfields with id parameter',
                    params: {
                        id: {
                            type: 'string',
                            required: true,
                            desc: 'Location name as stored in Stellarium database',
                            hint: 'Example: Cairo, Northern Africa | Paris, Western Europe | Tokyo, Eastern Asia',
                            suggest: 'search'
                        }
                    },
                    sscTemplate: 'core.setObserverLocation("{id}", "Earth");'
                },
								{
										id: 'location-set-by-coords',
										name: 'Set Location by Coordinates',
										name_en: 'Set Location by Coordinates',
										desc: 'Set latitude, longitude, altitude, and planet manually',
										method: 'POST',
										path: 'location/setlocationfields',
										source: 'LocationService.cpp: setlocationfields with coordinate parameters',
										params: {
												latitude: {
														type: 'number',
														required: true,
														desc: 'Latitude in degrees (positive = North, negative = South)',
														hint: 'Example: 30.0444 (Cairo), 48.8566 (Paris)'
												},
												longitude: {
														type: 'number',
														required: true,
														desc: 'Longitude in degrees (positive = East, negative = West)',
														hint: 'Example: 31.2357 (Cairo), 2.3522 (Paris)'
												},
												altitude: {
														type: 'number',
														required: false,
														desc: 'Altitude above sea level in meters',
														hint: 'Example: 100'
												},
												planet: {
														type: 'string',
														required: false,
														desc: 'Planet name (default: Earth)',
														hint: 'Example: Earth, Mars, Moon'
												},
												name: {
														type: 'string',
														required: false,
														desc: 'Custom location name (optional)',
														hint: 'Example: My Favorite Spot'
												}
										},
										_generateSSC: function(params) {
												var code = 'core.setObserverLocation(';
												if (params.name) {
														code += '"' + params.name + '", ';
												} else {
														code += '"Custom Location", ';
												}
												if (params.planet) {
														code += '"' + params.planet + '", ';
												} else {
														code += '"Earth", ';
												}
												code += params.latitude + ', ' + params.longitude;
												if (params.altitude) {
														code += ', ' + params.altitude;
												} else {
														code += ', 0';
												}
												code += ', 1);';
												return code;
										}
								},
                {
                    id: 'location-list',
                    name: 'List All Locations',
                    name_en: 'List All Locations',
                    desc: 'Get list of all locations stored in the database',
                    method: 'GET',
                    path: 'location/list',
                    source: 'LocationService.cpp: list operation',
                    params: {},
                    sscTemplate: '// No direct script equivalent\n// Use core.setObserverLocation() to set a specific location'
                },
                {
                    id: 'location-planetlist',
                    name: 'List Available Planets',
                    name_en: 'List Available Planets',
                    desc: 'Get list of planets that can be used as observer location',
                    method: 'GET',
                    path: 'location/planetlist',
                    source: 'LocationService.cpp: planetlist operation',
                    params: {},
                    sscTemplate: '// No direct script equivalent\n// Use core.setObserverLocation() with planet name'
                },
                {
                    id: 'location-search',
                    name: 'Search Location',
                    name_en: 'Search Location',
                    desc: 'Search the location database using partial text',
                    method: 'GET',
                    path: 'locationsearch/search',
                    source: 'LocationSearchService.cpp: search operation',
                    params: {
                        term: {
                            type: 'string',
                            required: true,
                            desc: 'Search text (supports wildcards)',
                            hint: 'Example: Cairo, western asia | Pari, western europe .. '
                        }
                    },
                    sscTemplate: '// Use core.setObserverLocation() after finding the location name'
                }
            ]
        },
        {
            id: 'view',
            name: 'View & Movement',
            name_en: 'View & Movement',
            icon: 'V',
            desc: 'Control view direction, zoom, and landscapes via MainService and ViewService',
            source: 'MainService.cpp (view, move, fov), ViewService.cpp',
            actions: [
                {
                    id: 'view-fov',
                    name: 'Set Field of View (FOV)',
                    name_en: 'Set Field of View',
                    desc: 'Change the zoom level of the display',
                    method: 'POST',
                    path: 'main/fov',
                    source: 'MainService.cpp: fov operation',
                    params: {
                        fov: {
                            type: 'number',
                            required: true,
                            desc: 'Field of view in degrees',
                            hint: '0.1 = very zoomed in, 60 = default, 180 = wide, 235 = maximum'
                        }
                    },
                    sscTemplate: 'StelMovementMgr.zoomTo({fov}, 2);'
                },
								{
										id: 'view-direction-altaz',
										name: 'Set View Direction (Alt/Az)',
										name_en: 'Set View Direction (Alt/Az)',
										desc: 'Point the view to specific horizontal coordinates using radians',
										method: 'POST',
										path: 'main/view',
										source: 'MainService.cpp: view operation with az/alt parameters',
										params: {
												az: {
														type: 'number',
														required: true,
														desc: 'Azimuth angle in RADIANS (0 = North, π/2 = East, π = South, 3π/2 = West)',
														hint: 'Example: 0 (North), 1.5708 (East), 3.14159 (South), 4.71239 (West)'
												},
												alt: {
														type: 'number',
														required: true,
														desc: 'Altitude angle in RADIANS (0 = horizon, π/2 = zenith)',
														hint: 'Example: 0 (horizon), 0.7854 (45°), 1.5708 (zenith)'
												}
										},
										/**
										 * Generates SSC code for moving to Alt/Az coordinates.
										 * Note: SSC uses degrees, not radians!
										 */
										_generateSSC: function(params) {
												var azRad = parseFloat(params.az);
												var altRad = parseFloat(params.alt);
												
												if (isNaN(azRad) || isNaN(altRad)) {
														return '// Invalid Alt/Az values\ncore.debug("Error: Invalid Alt/Az values");';
												}
												
												// Convert radians to degrees for SSC (since core.moveToAltAzi expects degrees)
												var azDeg = (azRad * 180 / Math.PI).toFixed(4);
												var altDeg = (altRad * 180 / Math.PI).toFixed(4);
												
												return '// Move view to Alt/Az coordinates\n' +
															 '// Note: Converting from radians (' + azRad + ', ' + altRad + ') to degrees (' + azDeg + '°, ' + altDeg + '°)\n' +
															 'core.moveToAltAzi(' + altDeg + ', ' + azDeg + ', 2);\n' +
															 'core.debug("Moved to Az: ' + azDeg + '°, Alt: ' + altDeg + '°");';
										}
								},
								{
										id: 'view-direction-j2000',
										name: 'Set View Direction (J2000 Vector)',
										name_en: 'Set View Direction (J2000)',
										desc: 'Point the view using J2000 Cartesian vector',
										method: 'POST',
										path: 'main/view',
										source: 'MainService.cpp: view operation with j2000 parameter',
										params: {
												j2000_x: {
														type: 'number',
														required: true,
														desc: 'X component (cos(dec)*cos(ra))',
														hint: 'Example: 0.5'
												},
												j2000_y: {
														type: 'number',
														required: true,
														desc: 'Y component (cos(dec)*sin(ra))',
														hint: 'Example: 0.5'
												},
												j2000_z: {
														type: 'number',
														required: true,
														desc: 'Z component (sin(dec))',
														hint: 'Example: 0.707'
												}
										},
										/**
										 * Generates cURL command with JSON array.
										 * Overrides default generation to properly format j2000 as JSON.
										 */
										_generateCurl: function(params) {
												var baseUrl = getCurlBaseUrl();
												var x = params.j2000_x;
												var y = params.j2000_y;
												var z = params.j2000_z;
												var jsonArray = '[' + x + ',' + y + ',' + z + ']';
												return 'curl -X POST -d "j2000=' + encodeURIComponent(jsonArray) + '" "' + baseUrl + '/api/main/view"';
										},
										_generateSSC: function(params) {
												var x = params.j2000_x;
												var y = params.j2000_y;
												var z = params.j2000_z;
												return '// Move view using J2000 vector\n' +
															 'var vec = core.vec3d(' + x + ', ' + y + ', ' + z + ');\n' +
															 'StelMovementMgr.setViewDirectionJ2000(vec.toVec3d());\n' +
															 'core.debug("View direction set to J2000 vector: (' + x + ', ' + y + ', ' + z + ')");';
										}
								},
                {
                    id: 'view-direction-j2000',
                    name: 'Set View Direction (J2000)',
                    name_en: 'Set View Direction (J2000)',
                    desc: 'Point the view using J2000 equatorial coordinates',
                    method: 'POST',
                    path: 'main/view',
                    source: 'MainService.cpp: view operation with j2000 parameter',
                    params: {
                        j2000_x: {
                            type: 'number',
                            required: true,
                            desc: 'X component of J2000 vector',
                            hint: 'cos(dec)*cos(ra)'
                        },
                        j2000_y: {
                            type: 'number',
                            required: true,
                            desc: 'Y component of J2000 vector',
                            hint: 'cos(dec)*sin(ra)'
                        },
                        j2000_z: {
                            type: 'number',
                            required: true,
                            desc: 'Z component of J2000 vector',
                            hint: 'sin(dec)'
                        }
                    },
                    sscTemplate: '// Use core.moveToRaDec() instead\ncore.moveToRaDec({ra}, {dec}, 2);'
                },
								{
										id: 'view-direction-jnow',
										name: 'Set View Direction (JNow)',
										name_en: 'Set View Direction (JNow)',
										desc: 'Point the view using current date equatorial coordinates',
										method: 'POST',
										path: 'main/view',
										source: 'MainService.cpp: view operation with jNow parameter',
										params: {
												jNow_x: {
														type: 'number',
														required: true,
														desc: 'X component of JNow vector',
														hint: 'cos(dec)*cos(ra)'
												},
												jNow_y: {
														type: 'number',
														required: true,
														desc: 'Y component of JNow vector',
														hint: 'cos(dec)*sin(ra)'
												},
												jNow_z: {
														type: 'number',
														required: true,
														desc: 'Z component of JNow vector',
														hint: 'sin(dec)'
												},
												ref: {
														type: 'select',
														options: ['', 'auto', 'on', 'off'],
														required: false,
														desc: 'Atmospheric refraction mode (default: auto)'
												}
										},
										sscTemplate: '// Use core.moveToRaDec() for equatorial coordinates'
								},
                {
                    id: 'view-move',
                    name: 'Continuous View Movement',
                    name_en: 'Continuous View Movement',
                    desc: 'Move view like a joystick (continues until zero values sent)',
                    method: 'POST',
                    path: 'main/move',
                    source: 'MainService.cpp: move operation',
                    params: {
                        x: {
                            type: 'number',
                            required: true,
                            desc: 'Horizontal speed (positive = right, negative = left)',
                            hint: '-1 to 1, where 1 = arrow key speed'
                        },
                        y: {
                            type: 'number',
                            required: true,
                            desc: 'Vertical speed (positive = up, negative = down)',
                            hint: '-1 to 1, where 1 = arrow key speed'
                        }
                    },
                    sscTemplate: '// Continuous movement not directly available in scripts\n// Use core.moveToAltAzi() or core.moveToRaDec()'
                },
								{
										id: 'view-get-direction',
										name: 'Get Current View Direction',
										name_en: 'Get Current View Direction',
										desc: 'Get current view direction vector in requested coordinate system via HTTP API.\n\n' +
													'For scripting, use the following functions:\n' +
													'• core.getViewAltitudeAngle() - Altitude angle (degrees)\n' +
													'• core.getViewAzimuthAngle() - Azimuth angle (degrees, 0=N, 90=E, 180=S, 270=W)\n' +
													'• core.getViewRaAngle() - Right Ascension (degrees, current epoch)\n' +
													'• core.getViewDecAngle() - Declination (degrees, current epoch)\n' +
													'• core.getViewRaJ2000Angle() - Right Ascension (degrees, J2000)\n' +
													'• core.getViewDecJ2000Angle() - Declination (degrees, J2000)',
										method: 'GET',
										path: 'main/view',
										source: 'MainService.cpp: GET view operation',
										params: {
												coord: {
														type: 'select',
														options: ['', 'j2000', 'jNow', 'altAz'],
														required: false,
														desc: 'Coordinate system (empty = all)'
												},
												ref: {
														type: 'select',
														options: ['', 'on', 'off', 'auto'],
														required: false,
														desc: 'Refraction mode for JNow'
												}
										},
										/**
										 * Generates correct SSC code for getting view direction.
										 * Uses the correct StelMainScriptAPI functions.
										 */
										_generateSSC: function(params) {
												var coord = params.coord || '';
												var ref = params.ref || '';
												
												var code = '';
												var showAll = (coord === '');
												
												// ============================================================
												// ALTITUDE AND AZIMUTH (always available)
												// ============================================================
												code += '// ============================================================\n';
												code += '// CURRENT VIEW DIRECTION\n';
												code += '// ============================================================\n\n';
												
												code += '// Get altitude and azimuth\n';
												code += 'var alt = core.getViewAltitudeAngle();\n';
												code += 'var azi = core.getViewAzimuthAngle();\n';
												code += 'core.debug("Altitude: " + alt.toFixed(2) + "°");\n';
												code += 'core.debug("Azimuth: " + azi.toFixed(2) + "°");\n\n';
												
												// Determine cardinal direction from azimuth
												code += '// Determine cardinal direction\n';
												code += 'var direction = "";\n';
												code += 'if(azi >= 337.5 || azi < 22.5) direction = "North";\n';
												code += 'else if(azi >= 22.5 && azi < 67.5) direction = "Northeast";\n';
												code += 'else if(azi >= 67.5 && azi < 112.5) direction = "East";\n';
												code += 'else if(azi >= 112.5 && azi < 157.5) direction = "Southeast";\n';
												code += 'else if(azi >= 157.5 && azi < 202.5) direction = "South";\n';
												code += 'else if(azi >= 202.5 && azi < 247.5) direction = "Southwest";\n';
												code += 'else if(azi >= 247.5 && azi < 292.5) direction = "West";\n';
												code += 'else if(azi >= 292.5 && azi < 337.5) direction = "Northwest";\n';
												code += 'core.debug("Direction: " + direction + " (" + azi.toFixed(1) + "°)");\n\n';
												
												// ============================================================
												// EQUATORIAL COORDINATES (current epoch)
												// ============================================================
												if (showAll || coord === 'jNow') {
														code += '// Get equatorial coordinates (current epoch)\n';
														code += 'var ra = core.getViewRaAngle();\n';
														code += 'var dec = core.getViewDecAngle();\n';
														code += 'core.debug("RA (JNow): " + ra.toFixed(4) + "°");\n';
														code += 'core.debug("Dec (JNow): " + dec.toFixed(4) + "°");\n\n';
														
														// Convert RA to hours
														code += '// RA in hours (15° = 1 hour)\n';
														code += 'var raHours = ra / 15;\n';
														code += 'core.debug("RA (hours): " + raHours.toFixed(2) + "h");\n\n';
														
														// Determine hemisphere from declination
														code += '// Declination hemisphere\n';
														code += 'var hemisphere = dec >= 0 ? "North" : "South";\n';
														code += 'core.debug("Dec hemisphere: " + hemisphere);\n\n';
												}
												
												// ============================================================
												// EQUATORIAL COORDINATES (J2000)
												// ============================================================
												if (showAll || coord === 'j2000') {
														code += '// Get equatorial coordinates (J2000 frame)\n';
														code += 'var raJ2000 = core.getViewRaJ2000Angle();\n';
														code += 'var decJ2000 = core.getViewDecJ2000Angle();\n';
														code += 'core.debug("RA (J2000): " + raJ2000.toFixed(4) + "°");\n';
														code += 'core.debug("Dec (J2000): " + decJ2000.toFixed(4) + "°");\n\n';
														
														// Compare J2000 vs current epoch
														if (showAll) {
																code += '// Difference between J2000 and current epoch (precession)\n';
																code += 'var raCurrent = core.getViewRaAngle();\n';
																code += 'var decCurrent = core.getViewDecAngle();\n';
																code += 'core.debug("RA difference (J2000 - JNow): " + (raJ2000 - raCurrent).toFixed(4) + "°");\n';
																code += 'core.debug("Dec difference (J2000 - JNow): " + (decJ2000 - decCurrent).toFixed(4) + "°");\n\n';
														}
												}
												
												// ============================================================
												// ALTITUDE/AZIMUTH ONLY (coord = 'altAz')
												// ============================================================
												if (coord === 'altAz') {
														// Only show altitude and azimuth (already included above)
														// No additional code needed
												}
												
												// ============================================================
												// REFRACTION MODE (if specified)
												// ============================================================
												if (ref && ref !== '') {
														code += '// Note: Refraction mode "' + ref + '" was requested.\n';
														code += '// Refraction is automatically applied to view calculations.\n\n';
												}
												
												// ============================================================
												// SUMMARY
												// ============================================================
												code += '// ============================================================\n';
												code += '// SUMMARY\n';
												code += '// ============================================================\n';
												code += 'core.debug("");\n';
												code += 'core.debug("=== VIEW DIRECTION SUMMARY ===");\n';
												code += 'core.debug("Altitude: " + alt.toFixed(2) + "°");\n';
												code += 'core.debug("Azimuth: " + azi.toFixed(2) + "° (" + direction + ")");\n';
												
												if (showAll || coord === 'jNow') {
														code += 'core.debug("RA (JNow): " + ra.toFixed(4) + "° (" + (ra/15).toFixed(2) + "h)");\n';
														code += 'core.debug("Dec (JNow): " + dec.toFixed(4) + "°");\n';
												}
												
												if (showAll || coord === 'j2000') {
														code += 'core.debug("RA (J2000): " + raJ2000.toFixed(4) + "°");\n';
														code += 'core.debug("Dec (J2000): " + decJ2000.toFixed(4) + "°");\n';
												}
												
												code += 'core.debug("================================");\n';
												
												return code;
										}
								},
                {
                    id: 'view-listlandscape',
                    name: 'List Landscapes',
                    name_en: 'List Landscapes',
                    desc: 'Get list of all installed landscapes',
                    method: 'GET',
                    path: 'view/listlandscape',
                    source: 'ViewService.cpp: listlandscape operation',
                    params: {},
                    sscTemplate: '// Use LandscapeMgr to access landscapes\n// LandscapeMgr.getAllLandscapeIDs();\nvar ids = LandscapeMgr.getAllLandscapeIDs();\ncore.debug("Landscape IDs: " + ids.join(", "));'
                },
                {
                    id: 'view-landscapedescription',
                    name: 'Current Landscape Description',
                    name_en: 'Current Landscape Description',
                    desc: 'Get HTML description of the current landscape',
                    method: 'GET',
                    path: 'view/landscapedescription/',
                    source: 'ViewService.cpp: landscapedescription/ operation',
                    params: {},
                    sscTemplate: '// Current landscape description\n// LandscapeMgr.getCurrentLandscapeHtmlDescription()'
                },
                {
                    id: 'view-listskyculture',
                    name: 'List Sky Cultures',
                    name_en: 'List Sky Cultures',
                    desc: 'Get list of all installed sky cultures',
                    method: 'GET',
                    path: 'view/listskyculture',
                    source: 'ViewService.cpp: listskyculture operation',
                    params: {},
                    sscTemplate: 'var cultures = core.getAllSkyCultureIDs();\ncore.debug("Cultures: " + cultures.join(", "));'
                },
                {
                    id: 'view-skyculturedescription',
                    name: 'Current Sky Culture Description',
                    name_en: 'Current Sky Culture Description',
                    desc: 'Get HTML description of the current sky culture',
                    method: 'GET',
                    path: 'view/skyculturedescription/',
                    source: 'ViewService.cpp: skyculturedescription/ operation',
                    params: {},
                    sscTemplate: '// Current culture description\n// var fulldescription = StelSkyCultureMgr.getCurrentSkyCultureHtmlDescription();\ncore.debug("Sky culture description : " + fulldescription);'
                },
                {
                    id: 'view-listprojection',
                    name: 'List Projections',
                    name_en: 'List Projections',
                    desc: 'Get list of available projection types',
                    method: 'GET',
                    path: 'view/listprojection',
                    source: 'ViewService.cpp: listprojection operation',
                    params: {},
                    sscTemplate: 'var projection = core.getProjectionMode();\ncore.debug("Current map projection: " + projection);\n// Use core.setProjectionMode() to change projection'
                },
                {
                    id: 'view-projectiondescription',
                    name: 'Current Projection Description',
                    name_en: 'Current Projection Description',
                    desc: 'Get HTML description of the current projection',
                    method: 'GET',
                    path: 'view/projectiondescription',
                    source: 'ViewService.cpp: projectiondescription operation',
                    params: {},
                    sscTemplate: '// No direct script equivalent to retrive projection description'
                }
            ]
        },
        {
            id: 'objects',
            name: 'Celestial Objects',
            name_en: 'Celestial Objects',
            icon: 'O',
            desc: 'Search, target, and get information about celestial objects via ObjectService and MainService',
            source: 'ObjectService.cpp, MainService.cpp',
            actions: [
								{
										id: 'objects-find',
										name: 'Find and Select Object',
										name_en: 'Find and Select Object',
										desc: 'Search for and select a celestial object by name',
										method: 'POST',
										path: 'main/focus',
										source: 'MainService.cpp: focus operation',
										params: {
												target: {
														type: 'string',
														required: true,
														desc: 'Object name to search for (supports English and local names)',
														hint: 'Example: Mars, Moon, Jupiter, M31, Sirius, Polaris'
												},
												mode: {
														type: 'select',
														options: ['center', 'zoom', 'mark'],
														required: false,
														desc: 'Focus mode: center = point view, zoom = auto zoom, mark = select only',
														hint: 'Select focus mode (optional)',
														defaultValue: 'center'
												}
										},
										/**
										 * Generates SSC code for finding/selecting an object.
										 * Uses the correct Stellarium API: selectObjectByName and getObjectInfo.
										 * 
										 * @param {Object} params - The collected parameters
										 * @returns {string} SSC script
										 */
										_generateSSC: function(params) {
												var target = (params.target || '').replace(/"/g, '\\"');
												var mode = params.mode || 'center';
												
												if (!target) {
														return '// Please specify an object name\ncore.debug("Error: No object name provided");';
												}
												
												var code = '// Find and select object: ' + target + '\n';
												code += 'var objectName = "' + target + '";\n\n';
												
												// First, get object info to verify it exists
												code += '// Get object information first\n';
												code += 'var objInfo = core.getObjectInfo(objectName);\n';
												code += 'if (!objInfo) {\n';
												code += '    core.debug("Object not found: " + objectName);\n';
												code += '} else {\n';
												code += '    core.debug("Found: " + objectName);\n';
												code += '    core.debug("  Type: " + (objInfo["object-type"] || objInfo["type"] || "Unknown"));\n';
												code += '    core.debug("  Magnitude: " + (objInfo["vmag"] !== undefined ? objInfo["vmag"].toFixed(2) : "N/A"));\n';
												code += '    core.debug("  Constellation: " + (objInfo["iauConstellation"] || "N/A"));\n\n';
												
												// Then select and move to the object
												code += '    // Select and move to the object\n';
												code += '    core.selectObjectByName(objectName, true);\n';
												
												if (mode === 'zoom') {
														code += '    // Auto zoom for better view\n';
														code += '    StelMovementMgr.autoZoomIn(3);\n';
												} else if (mode === 'mark') {
														code += '    // Just select without moving camera\n';
														code += '    // (camera already moves with selectObjectByName when second param is true)\n';
												}
												
												// Display current position info
												code += '\n';
												code += '    // Current position information\n';
												code += '    core.debug("");\n';
												code += '    core.debug("Current position of " + objectName + ":");\n';
												code += '    core.debug("  Altitude: " + (objInfo["altitude"] !== undefined ? objInfo["altitude"].toFixed(2) + "°" : "N/A"));\n';
												code += '    core.debug("  Azimuth: " + (objInfo["azimuth"] !== undefined ? objInfo["azimuth"].toFixed(2) + "°" : "N/A"));\n';
												code += '    core.debug("  Above Horizon: " + (objInfo["above-horizon"] ? "Yes" : "No"));\n';
												code += '    core.debug("  Distance: " + (objInfo["distance-ly"] !== undefined ? objInfo["distance-ly"].toFixed(2) + " ly" : (objInfo["distance"] ? objInfo["distance"] + " AU" : "N/A")));\n';
												code += '}\n';
												
												return code;
										}
								},
								{
										id: 'objects-focus',
										name: 'Focus on Object',
										name_en: 'Focus Object',
										desc: 'Point view towards a specific object with zoom and selection options',
										method: 'POST',
										path: 'main/focus',
										source: 'MainService.cpp: focus operation',
										params: {
												target: {
														type: 'string',
														required: true,
														desc: 'Object name (supports English and local names)',
														hint: 'Example: Mars, Moon, Jupiter, M31, Sirius'
												},
												mode: {
														type: 'select',
														options: ['center', 'zoom', 'mark'],
														required: false,
														desc: 'Focus mode: center = point view, zoom = auto zoom, mark = select only',
														hint: 'Select focus mode (optional)'
												}
										},
										sscTemplate: 'core.moveToObject("{target}", 2);\n// To select only:\n// core.selectObjectByName("{target}");'
								},
                {
                    id: 'objects-focus-position',
                    name: 'Focus on J2000 Position',
                    name_en: 'Focus J2000 Position',
                    desc: 'Point view towards specific J2000 coordinates',
                    method: 'POST',
                    path: 'main/focus',
                    source: 'MainService.cpp: focus operation with position parameter',
                    params: {
                        position_x: {
                            type: 'number',
                            required: true,
                            desc: 'X component',
                            hint: 'cos(dec)*cos(ra)'
                        },
                        position_y: {
                            type: 'number',
                            required: true,
                            desc: 'Y component',
                            hint: 'cos(dec)*sin(ra)'
                        },
                        position_z: {
                            type: 'number',
                            required: true,
                            desc: 'Z component',
                            hint: 'sin(dec)'
                        }
                    },
                    sscTemplate: 'core.moveToRaDec({ra}, {dec}, 2);'
                },
								{
										id: 'objects-info',
										name: 'Object Information',
										name_en: 'Object Information',
										desc: 'Get detailed information about a specific object',
										method: 'GET',
										path: 'objects/info',
										source: 'ObjectService.cpp: info operation',
										params: {
												name: {
														type: 'string',
														required: true,
														desc: 'Object name (supports English and local names)',
														hint: 'Example: Sun, Moon, Jupiter, Polaris, Sirius, HIP 11767'
												},
												format: {
														type: 'select',
														options: ['html', 'json', 'map'],
														required: false,
														desc: 'Output format: html (default), json, map',
														hint: 'Select output format',
														defaultValue: 'html'
												}
										},
										/**
										 * Generates comprehensive SSC code for displaying object information.
										 * Properly handles all available properties from the Stellarium API.
										 * 
										 * @param {Object} params - The collected parameters
										 * @returns {string} SSC script
										 */
										_generateSSC: function(params) {
												var objectName = (params.name || 'Sun').replace(/"/g, '\\"');
												var format = params.format || 'html';
												
												if (format === 'json') {
														return '// Get complete object information in JSON format\n' +
																	 'var info = core.getObjectInfo("' + objectName + '");\n' +
																	 'if (info) {\n' +
																	 '    core.debug("=== ' + objectName + ' Complete Data === ");\n' +
																	 '    core.debug(JSON.stringify(info, null, 2));\n' +
																	 '} else {\n' +
																	 '    core.debug("Object not found: ' + objectName + '");\n' +
																	 '}\n\n' +
																	 '// Access specific properties:\n' +
																	 '// core.debug("Name: " + info["name"]);\n' +
																	 '// core.debug("Type: " + info["object-type"]);\n' +
																	 '// core.debug("Magnitude: " + info["vmag"]);\n' +
																	 '// core.debug("Distance: " + info["distance-ly"] + " light years");\n' +
																	 '// core.debug("RA: " + info["ra"] + "°");\n' +
																	 '// core.debug("Dec: " + info["dec"] + "°");\n' +
																	 '// core.debug("Altitude: " + info["altitude"] + "°");\n' +
																	 '// core.debug("Azimuth: " + info["azimuth"] + "°");';
												} else {
														return '// Get detailed information about ' + objectName + '\n' +
																	 'var info = core.getObjectInfo("' + objectName + '");\n\n' +
																	 'if (!info) {\n' +
																	 '    core.debug("Object not found: ' + objectName + '");\n' +
																	 '} else {\n' +
																	 '    core.debug("╔══════════════════════════════════════════════════════════════╗");\n' +
																	 '    core.debug("║                    ' + objectName.toUpperCase() + ' INFORMATION                    ║");\n' +
																	 '    core.debug("╚══════════════════════════════════════════════════════════════╝");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === BASIC IDENTIFICATION ===\n' +
																	 '    core.debug("┌────────────── BASIC IDENTIFICATION ──────────────┐");\n' +
																	 '    core.debug("│ Name:          " + (info["name"] || "N/A"));\n' +
																	 '    core.debug("│ Localized Name:" + (info["localized-name"] || "N/A"));\n' +
																	 '    core.debug("│ Object Type:   " + (info["object-type"] || info["type"] || "N/A"));\n' +
																	 '    core.debug("│ IAU Const.:    " + (info["iauConstellation"] || "N/A"));\n' +
																	 '    core.debug("│ Spectral Class:" + (info["spectral-class"] || "N/A"));\n' +
																	 '    core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === MAGNITUDE AND DISTANCE ===\n' +
																	 '    core.debug("┌────────────── MAGNITUDE & DISTANCE ──────────────┐");\n' +
																	 '    core.debug("│ Visual Mag:    " + (info["vmag"] !== undefined ? info["vmag"].toFixed(2) : "N/A"));\n' +
																	 '    core.debug("│ Absolute Mag:  " + (info["absolute-mag"] !== undefined ? info["absolute-mag"].toFixed(2) : "N/A"));\n' +
																	 '    core.debug("│ Distance (ly): " + (info["distance-ly"] !== undefined ? info["distance-ly"].toFixed(2) : "N/A"));\n' +
																	 '    core.debug("│ Distance (AU): " + (info["distance"] !== undefined ? info["distance"].toFixed(2) : "N/A"));\n' +
																	 '    core.debug("│ Parallax:      " + (info["parallax"] !== undefined ? info["parallax"] + " mas" : "N/A"));\n' +
																	 '    core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === COORDINATES ===\n' +
																	 '    core.debug("┌────────────────── COORDINATES ──────────────────┐");\n' +
																	 '    core.debug("│ RA (J2000):     " + (info["raJ2000"] !== undefined ? info["raJ2000"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Dec (J2000):    " + (info["decJ2000"] !== undefined ? info["decJ2000"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ RA (JNow):      " + (info["ra"] !== undefined ? info["ra"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Dec (JNow):     " + (info["dec"] !== undefined ? info["dec"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Galactic L:     " + (info["glong"] !== undefined ? info["glong"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Galactic B:     " + (info["glat"] !== undefined ? info["glat"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Ecliptic Lon:   " + (info["elong"] !== undefined ? info["elong"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Ecliptic Lat:   " + (info["elat"] !== undefined ? info["elat"].toFixed(4) + "°" : "N/A"));\n' +
																	 '    core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === CURRENT POSITION (Alt/Az) ===\n' +
																	 '    core.debug("┌────────────── CURRENT POSITION (Alt/Az) ──────────┐");\n' +
																	 '    core.debug("│ Altitude:      " + (info["altitude"] !== undefined ? info["altitude"].toFixed(2) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Azimuth:       " + (info["azimuth"] !== undefined ? info["azimuth"].toFixed(2) + "°" : "N/A"));\n' +
																	 '    core.debug("│ Above Horizon: " + (info["above-horizon"] ? "YES ✓" : "NO ✗"));\n' +
																	 '    core.debug("│ Air Mass:      " + (info["airmass"] !== undefined ? info["airmass"].toFixed(2) : "N/A"));\n' +
																	 '    core.debug("│ Hour Angle:    " + (info["hourAngle-hms"] || info["hourAngle-dd"] || "N/A"));\n' +
																	 '    core.debug("│ App Sidereal:  " + (info["appSidTm"] || "N/A"));\n' +
																	 '    core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === RISE/SET/TRANSIT ===\n' +
																	 '    core.debug("┌────────────── RISE / SET / TRANSIT ──────────────┐");\n' +
																	 '    core.debug("│ Rise Time:     " + (info["rise"] || "Circumpolar"));\n' +
																	 '    core.debug("│ Transit Time:  " + (info["transit"] || "N/A"));\n' +
																	 '    core.debug("│ Set Time:      " + (info["set"] || "Circumpolar"));\n' +
																	 '    core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '    core.debug("");\n' +
																	 '    \n' +
																	 '    // === STAR-SPECIFIC PROPERTIES ===\n' +
																	 '    if (info["star-type"] || info["spectral-class"] || info["period"] || info["bV"]) {\n' +
																	 '        core.debug("┌─────────────── STAR PROPERTIES ───────────────┐");\n' +
																	 '        core.debug("│ Star Type:     " + (info["star-type"] || "N/A"));\n' +
																	 '        core.debug("│ Spectral:      " + (info["spectral-class"] || "N/A"));\n' +
																	 '        core.debug("│ B-V Index:     " + (info["bV"] !== undefined ? info["bV"].toFixed(3) : "N/A"));\n' +
																	 '        core.debug("│ Period:        " + (info["period"] !== undefined ? info["period"] + " days" : "N/A"));\n' +
																	 '        core.debug("│ Variable Type: " + (info["variable-star"] || "N/A"));\n' +
																	 '        core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '        core.debug("");\n' +
																	 '    }\n' +
																	 '    \n' +
																	 '    // === DOUBLE STAR PROPERTIES ===\n' +
																	 '    if (info["wds-separation"] !== undefined || info["wds-position-angle"] !== undefined) {\n' +
																	 '        core.debug("┌────────────── DOUBLE STAR DATA ───────────────┐");\n' +
																	 '        core.debug("│ Separation:    " + (info["wds-separation"] !== undefined ? info["wds-separation"] + " arcsec" : "N/A"));\n' +
																	 '        core.debug("│ Position Angle:" + (info["wds-position-angle"] !== undefined ? info["wds-position-angle"] + "°" : "N/A"));\n' +
																	 '        core.debug("│ WDS Year:      " + (info["wds-year"] || "N/A"));\n' +
																	 '        core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '        core.debug("");\n' +
																	 '    }\n' +
																	 '    \n' +
																	 '    // === SOLAR SYSTEM PROPERTIES ===\n' +
																	 '    if (info["illumination"] !== undefined || info["phase"] !== undefined || info["age"] !== undefined) {\n' +
																	 '        core.debug("┌──────────── SOLAR SYSTEM DATA ───────────────┐");\n' +
																	 '        if (info["illumination"] !== undefined) core.debug("│ Illumination:  " + info["illumination"] + "%");\n' +
																	 '        if (info["phase"] !== undefined) core.debug("│ Phase:         " + (info["phase"] * 100).toFixed(1) + "%");\n' +
																	 '        if (info["age"] !== undefined) core.debug("│ Age:           " + info["age"] + " days");\n' +
																	 '        if (info["velocity-kms"] !== undefined) core.debug("│ Velocity:      " + info["velocity-kms"] + " km/s");\n' +
																	 '        if (info["radial-velocity"] !== undefined) core.debug("│ Radial Vel:    " + info["radial-velocity"] + " km/s");\n' +
																	 '        if (info["moons"] !== undefined) core.debug("│ Moons:         " + info["moons"]);\n' +
																	 '        core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '        core.debug("");\n' +
																	 '    }\n' +
																	 '    \n' +
																	 '    // === OBJECT SIZE ===\n' +
																	 '    if (info["size-deg"] !== undefined && info["size-deg"] !== "0.00000°") {\n' +
																	 '        core.debug("┌──────────────── OBJECT SIZE ─────────────────┐");\n' +
																	 '        core.debug("│ Angular Size:  " + (info["size-deg"] || "N/A"));\n' +
																	 '        core.debug("│ Angular Size:  " + (info["size-dms"] || "N/A"));\n' +
																	 '        core.debug("└────────────────────────────────────────────────────┘");\n' +
																	 '        core.debug("");\n' +
																	 '    }\n' +
																	 '    \n' +
																	 '    core.debug("╔══════════════════════════════════════════════════════════════╗");\n' +
																	 '    core.debug("║  Tip: Use format=\\"json\\" to get complete raw JSON data      ║");\n' +
																	 '    core.debug("╚══════════════════════════════════════════════════════════════╝");\n' +
																	 '}\n\n' +
																	 '// To get all available property names (for scripting):\n' +
																	 '// var props = Object.keys(info);\n' +
																	 '// core.debug("Properties: " + props.join(", "));';
												}
										}
								},
                {
                    id: 'objects-listobjecttypes',
                    name: 'List Object Types',
                    name_en: 'List Object Types',
                    desc: 'Get list of all available object types',
                    method: 'GET',
                    path: 'objects/listobjecttypes',
                    source: 'ObjectService.cpp: listobjecttypes operation',
                    params: {},
                    sscTemplate: '// Use ObjectMgr to get list of object types'
                },
                {
                    id: 'objects-listbytype',
                    name: 'List Objects by Type',
                    name_en: 'List Objects by Type',
                    desc: 'Get list of all objects of a specific type',
                    method: 'GET',
                    path: 'objects/listobjectsbytype',
                    source: 'ObjectService.cpp: listobjectsbytype operation',
                    params: {
                        type: {
                            type: 'string',
                            required: true,
                            desc: 'Type key (from listobjecttypes)',
                            hint: 'Example: StarMgr, SolarSystem, NebulaMgr, AsterismMgr, ConstellationMgr ...'
                        },
                        english: {
                            type: 'select',
                            options: ['', '0', '1'],
                            required: false,
                            desc: '"1" for English names, "0" for localized names'
                        }
                    },
                    sscTemplate: '// Use ObjectMgr to get list of objects by type'
                }
            ]
        },
				{
						id: 'actions',
						name: 'StelActions',
						name_en: 'StelActions',
						icon: 'A',
						desc: 'Execute and toggle Stellarium actions via StelActionService',
						source: 'StelActionService.cpp, StelActionMgr.cpp',
						actions: [
								{
										id: 'action-list',
										name: 'List All Actions',
										name_en: 'List All Actions',
										desc: 'Get list of all registered actions with their current states',
										method: 'GET',
										path: 'stelaction/list',
										source: 'StelActionService.cpp: list operation',
										params: {},
										sscTemplate: `// List All Actions - Use API Explorer or cURL for this.
		// StelActionMgr is NOT available in Stellarium Scripts.
		// For script use, directly call the module functions:
		// - SolarSystem.setFlagPlanets(true/false)
		// - ConstellationMgr.setFlagLines(true/false)
		// - StelMovementMgr.zoomTo(fov, duration)
		// etc.`
								},
								{
										id: 'action-do',
										name: 'Execute Action',
										name_en: 'Execute Action',
										desc: 'Run or toggle a specific action (constellation lines, grids, etc.)',
										method: 'POST',
										path: 'stelaction/do',
										source: 'StelActionService.cpp: do operation',
										params: {
												id: {
														type: 'string',
														required: true,
														desc: 'Action identifier (e.g., actionShow_Constellation_Lines)',
														hint: 'Example: actionShow_Constellation_Lines, actionShow_Atmosphere',
														suggest: 'actions'
												}
										},
										/**
										 * Generates correct SSC code for StelAction operations.
										 * StelActionMgr is NOT available in scripts - use direct module calls.
										 */
										_generateSSC: function(params) {
												var actionId = params.id || 'actionShow_Stars';
												
												// Map action IDs to their equivalent script commands
												var actionMap = {
														'actionShow_Stars': {
																module: 'StarMgr',
																method: 'setFlagStars',
																args: 'true/false'
														},
														'actionShow_Planets': {
																module: 'SolarSystem',
																method: 'setFlagPlanets',
																args: 'true/false'
														},
														'actionShow_Planets_Labels': {
																module: 'SolarSystem',
																method: 'setFlagLabels',
																args: 'true/false'
														},
														'actionShow_Planets_Orbits': {
																module: 'SolarSystem',
																method: 'setFlagOrbits',
																args: 'true/false'
														},
														'actionShow_Planets_Hints': {
																module: 'SolarSystem',
																method: 'setFlagHints',
																args: 'true/false'
														},
														'actionShow_Atmosphere': {
																module: 'LandscapeMgr',
																method: 'setFlagAtmosphere',
																args: 'true/false'
														},
														'actionShow_Ground': {
																module: 'LandscapeMgr',
																method: 'setFlagLandscape',
																args: 'true/false'
														},
														'actionShow_Fog': {
																module: 'LandscapeMgr',
																method: 'setFlagFog',
																args: 'true/false'
														},
														'actionShow_Night_Mode': {
																module: 'core',
																method: 'setNightMode',
																args: 'true/false'
														},
														'actionShow_Equatorial_Grid': {
																module: 'GridLinesMgr',
																method: 'setFlagEquatorGrid',
																args: 'true/false'
														},
														'actionShow_Azimuthal_Grid': {
																module: 'GridLinesMgr',
																method: 'setFlagAzimuthalGrid',
																args: 'true/false'
														},
														'actionShow_Constellation_Lines': {
																module: 'ConstellationMgr',
																method: 'setFlagLines',
																args: 'true/false'
														},
														'actionShow_Constellation_Labels': {
																module: 'ConstellationMgr',
																method: 'setFlagLabels',
																args: 'true/false'
														},
														'actionShow_Constellation_Boundaries': {
																module: 'ConstellationMgr',
																method: 'setFlagBoundaries',
																args: 'true/false'
														},
														'actionShow_Constellation_Art': {
																module: 'ConstellationMgr',
																method: 'setFlagArt',
																args: 'true/false'
														},
														'actionShow_Nebulas': {
																module: 'NebulaMgr',
																method: 'setFlagHints',
																args: 'true/false'
														},
														'actionShow_MilkyWay': {
																module: 'MilkyWay',
																method: 'setFlagShow',
																args: 'true/false'
														},
														'actionGo_Home_Global': {
																module: 'core',
																method: 'goHome',
																args: 'duration'
														},
														'actionSwitch_Equatorial_Mount': {
																module: 'StelMovementMgr',
																method: 'setEquatorialMount',
																args: 'true/false'
														}
												};
												
												// Try to find a mapping for this action
												var mapping = actionMap[actionId];
												if (mapping) {
														var code = '// ⚡ ' + actionId + ' - Use direct module call:\n';
														code += '// ' + mapping.module + '.' + mapping.method + '(' + mapping.args + ')\n\n';
														code += '// Example: Toggle ' + actionId + '\n';
														if (mapping.args === 'true/false') {
																code += '// Get current value (if available)\n';
																code += '// var current = core.getStelProperty("' + actionId + '");\n';
																code += '// core.debug("Current value: " + current);\n\n';
																code += '// Toggle the setting\n';
																code += '// ' + mapping.module + '.' + mapping.method + '(true);  // Turn ON\n';
																code += '// ' + mapping.module + '.' + mapping.method + '(false); // Turn OFF\n';
																code += '// core.debug("' + actionId + ' toggled");\n';
														} else if (mapping.args === 'duration') {
																code += '// ' + mapping.module + '.' + mapping.method + '(3); // Duration in seconds\n';
																code += '// core.debug("Navigating to home view");\n';
														}
														return code;
												}
												
												// For unmapped actions, provide a generic solution using core.getStelProperty/setStelProperty
												var code = '// Action: ' + actionId + '\n';
												code += '// StelActionMgr is NOT available in Stellarium Scripts.\n';
												code += '// Use the following alternative approaches:\n\n';
												code += '// Option 1: Use core.getStelProperty() and core.setStelProperty()\n';
												code += '// var currentValue = core.getStelProperty("' + actionId + '");\n';
												code += '// core.debug("Current value: " + currentValue);\n';
												code += '// core.setStelProperty("' + actionId + '", !currentValue);\n\n';
												code += '// Option 2: Use the appropriate module\'s public slot directly\n';
												code += '// Example: SolarSystem.setFlagPlanets(true);\n';
												code += '// Example: ConstellationMgr.setFlagLines(true);\n\n';
												code += '// Option 3: For API calls, use cURL or the API Explorer\n';
												code += '// curl -X POST -d "id=' + actionId + '" /api/stelaction/do\n';
												
												return code;
										}
								}
						]
				},
				{
						id: 'properties',
						name: 'StelProperties',
						name_en: 'StelProperties',
						icon: 'P',
						desc: 'Read and modify system properties via StelPropertyService',
						source: 'StelPropertyService.cpp, StelMainScriptAPI',
						actions: [
								{
										id: 'property-list',
										name: 'List All Properties',
										name_en: 'List All Properties',
										desc: 'Get list of all registered properties with values and types via HTTP API.\n\n' +
													'For scripting, use: var allProps = core.getPropertyList();',
										method: 'GET',
										path: 'stelproperty/list',
										source: 'StelPropertyService.cpp: list operation',
										params: {},
										sscTemplate: '// List All Properties - Use core.getPropertyList()\n' +
																 'var allProps = core.getPropertyList();\n' +
																 'var sorted = allProps.sort();\n' +
																 'core.debug("Total properties: " + sorted.length);\n\n' +
																 '// Display first 20 properties\n' +
																 'core.debug("First 20 properties:");\n' +
																 'for (var i = 0; i < Math.min(20, sorted.length); i++) {\n' +
																 '    core.debug((i+1) + ". " + sorted[i]);\n' +
																 '}\n\n' +
																 '// Save to output as comma-separated list\n' +
																 'core.output(sorted.join(","));\n' +
																 'core.debug("Saved to output");\n\n' +
																 '// Save as script array\n' +
																 'var arr = "const allProperties = [\\n";\n' +
																 'for (var i = 0; i < sorted.length; i++) {\n' +
																 '    arr += "    \'" + sorted[i] + "\'" + (i < sorted.length-1 ? "," : "") + "\\n";\n' +
																 '}\n' +
																 'arr += "];\\n// Total: " + sorted.length;\n' +
																 'core.output(arr);\n' +
																 'core.debug("Array saved to output");'
								},
								{
										id: 'property-set',
										name: 'Set Property Value',
										name_en: 'Set Property Value',
										desc: 'Set a new value for a specific StelProperty using the Stellarium Script API.\n\n' +
													'Example: core.setStelProperty("MilkyWay.intensity", 2.5);',
										method: 'POST',
										path: 'stelproperty/set',
										source: 'StelPropertyService.cpp: set operation',
										params: {
												id: {
														type: 'string',
														required: true,
														desc: 'Property identifier (e.g., MilkyWay.intensity, ConstellationMgr.flagLines)',
														hint: 'Example: MilkyWay.intensity, StelSkyDrawer.absoluteStarScale',
														suggest: 'properties'
												},
												value: {
														type: 'string',
														required: true,
														desc: 'New value (auto-converted to appropriate type: bool, int, double, string)',
														hint: 'Example: 5.0, true, false, "western"'
												}
										},
										/**
										 * Generates correct SSC code for setting a StelProperty.
										 * Uses core.setStelProperty() and core.getStelProperty() which are
										 * the correct public APIs exposed via StelMainScriptAPI.
										 *
										 * @param {Object} params - The collected parameters (must include 'id' and 'value')
										 * @returns {string} Generated SSC code
										 */
										_generateSSC: function(params) {
												var propId = params.id || 'MilkyWay.intensity';
												var propValue = params.value || '1.0';
												
												// Determine if the value looks like a string or number/boolean
												var isStringValue = isNaN(propValue) && 
																						propValue !== 'true' && 
																						propValue !== 'false';
												
												var formattedValue;
												if (propValue === 'true' || propValue === 'false') {
														formattedValue = propValue; // boolean
												} else if (!isNaN(propValue)) {
														formattedValue = propValue; // number
												} else {
														formattedValue = '"' + propValue + '"'; // string
												}
												
												return '// Read current value\n' +
															 'var currentValue = core.getStelProperty("' + propId + '");\n' +
															 'core.debug("Current value of ' + propId + ': " + currentValue);\n\n' +
															 '// Set the new value\n' +
															 'core.setStelProperty("' + propId + '", ' + formattedValue + ');\n' +
															 'core.debug("Property ' + propId + ' set to ' + formattedValue + '");\n\n' +
															 '// Verify the change\n' +
															 'var newValue = core.getStelProperty("' + propId + '");\n' +
															 'core.debug("New value of ' + propId + ': " + newValue);';
										}
								}
						]
				},
        {
            id: 'scripts',
            name: 'Scripts',
            name_en: 'Scripts',
            icon: 'S',
            desc: 'Run and manage Stellarium scripts via ScriptService (requires ENABLE_SCRIPTING)',
            source: 'ScriptService.cpp',
            requiresScripting: true,
            actions: [
                {
                    id: 'script-list',
                    name: 'List Available Scripts',
                    name_en: 'List Available Scripts',
                    desc: 'Get list of all installed script files',
                    method: 'GET',
                    path: 'scripts/list',
                    source: 'ScriptService.cpp: list operation',
                    params: {},
                    sscTemplate: '// No direct equivalent'
                },
								{
										id: 'script-info',
										name: 'Script Information',
										name_en: 'Script Information',
										desc: 'Get detailed information about a specific script (name, description, author...)',
										method: 'GET',
										path: 'scripts/info',
										source: 'ScriptService.cpp: info operation',
										params: {
												id: {
														type: 'string',
														required: true,
														desc: 'Script identifier (filename)',
														hint: 'Example: double_stars.ssc',
														suggest: 'scripts'
												},
												html: {
														type: 'checkbox',
														required: false,
														desc: 'Enable to return HTML description instead of JSON'
												}
										},
										sscTemplate: '// No direct equivalent'
								},
                {
                    id: 'script-run',
                    name: 'Run Installed Script',
                    name_en: 'Run Installed Script',
                    desc: 'Execute an existing script file (cannot run two scripts simultaneously)',
                    method: 'POST',
                    path: 'scripts/run',
                    source: 'ScriptService.cpp: run operation',
                    params: {
                        id: {
                            type: 'string',
                            required: true,
                            desc: 'Script identifier (filename)',
                            hint: 'Example: double_stars.ssc, triple_moon.ssc',
                            suggest: 'scripts'
                        }
                    },
                    sscTemplate: '// No direct equivalent to run ("{id}");'
                },
								{
										id: 'script-direct',
										name: 'Run Direct Code',
										name_en: 'Run Direct Code',
										desc: 'Execute script code sent directly as text',
										method: 'POST',
										path: 'scripts/direct',
										source: 'ScriptService.cpp: direct operation',
										params: {
												code: {
														type: 'textarea',
														required: true,
														desc: 'Stellarium Script code',
														hint: 'Example:\ncore.setDate(\"2025-01-01T00:00:00", \"utc\");\n\ncore.moveToObject(\"Mars\", 2);'
												},
												useIncludes: {
														type: 'checkbox',
														required: false,
														desc: 'Enable to use includes directory for preprocessing'
												}
										},
										sscTemplate: '{code}',
										_generateSSC: function(params) {
												return params.code || '// No code provided';
										}
								},
                {
                    id: 'script-status',
                    name: 'Script Status',
                    name_en: 'Script Status',
                    desc: 'Query current script execution status',
                    method: 'GET',
                    path: 'scripts/status',
                    source: 'ScriptService.cpp: status operation',
                    params: {},
                    sscTemplate: '// No direct equivalent in scripts;'
                },
                {
                    id: 'script-stop',
                    name: 'Stop Running Script',
                    name_en: 'Stop Running Script',
                    desc: 'Stop the currently executing script',
                    method: 'POST',
                    path: 'scripts/stop',
                    source: 'ScriptService.cpp: stop operation',
                    params: {},
                    sscTemplate: '//Stops the current script completely and exits.\ncore.exit();'
                }
            ]
								},
								{
										id: 'simbad',
										name: 'SIMBAD Search',
										name_en: 'SIMBAD Search',
										icon: 'W',
										desc: 'Search the online SIMBAD astronomical database',
										source: 'SimbadService.cpp',
										actions: [
												{
														id: 'simbad-lookup',
														name: 'SIMBAD Lookup',
														name_en: 'SIMBAD Lookup',
														desc: 'Send query to SIMBAD service and get results (names and coordinates)',
														method: 'GET',
														path: 'simbad/lookup',
														source: 'SimbadService.cpp: lookup operation',
														params: {
																str: {
																		type: 'string',
																		required: true,
																		desc: 'Search text (sent to SIMBAD server)',
																		hint: 'Example: Betelgeuse, M42, Andromeda'
																}
														},
														sscTemplate: '// No direct equivalent in scripts\n// Use core.getObjectInfo("string"); for local search'
												}
										]
								},
        {
            id: 'main',
            name: 'General Status',
            name_en: 'Main Status',
            icon: 'M',
            desc: 'General program status queries via MainService',
            source: 'MainService.cpp',
            actions: [
								{
										id: 'main-status',
										name: 'Full Program Status',
										name_en: 'Full Status',
										desc: 'Get complete snapshot of program state (location, time, view, selected object)',
										method: 'GET',
										path: 'main/status',
										source: 'MainService.cpp: status operation',
										params: {
												actionId: {
														type: 'number',
														required: false,
														desc: 'Last known action change ID (for differential updates)'
												},
												propId: {
														type: 'number',
														required: false,
														desc: 'Last known property change ID (for differential updates)'
												}
										},
										sscTemplate: '// No direct equivalent\n// Use individual functions: core.getJD(), core.getObserverLocation()...'
								},
                {
                    id: 'main-plugins',
                    name: 'List Plugins',
                    name_en: 'List Plugins',
                    desc: 'Get list of all installed plugins with their information',
                    method: 'GET',
                    path: 'main/plugins',
                    source: 'MainService.cpp: plugins operation',
                    params: {},
                    sscTemplate: '// Check if Satellites plugin is available\nif(core.isModuleLoaded("Satellites")) {\ncore.setStelProperty("Satellites.flagLabels", true);\ncore.debug("Satellites plugin active");\n} else {\ncore.debug("Satellites plugin not loaded - skipping");\n}\n// Check Exoplanets plugin\nif(core.isModuleLoaded("Exoplanets")) {\ncore.debug("Exoplanets plugin available");\n}\n// Check operating system platform\nvar platform = core.getPlatformName();\ncore.debug("Operating system: " + platform);'
                },
                {
                    id: 'main-window',
                    name: 'Set Window Size',
                    name_en: 'Set Window Size',
                    desc: 'Change Stellarium main window dimensions',
                    method: 'POST',
                    path: 'main/window',
                    source: 'MainService.cpp: window operation',
                    params: {
                        w: {
                            type: 'number',
                            required: true,
                            desc: 'Width in pixels',
                            hint: 'Example: 1920'
                        },
                        h: {
                            type: 'number',
                            required: true,
                            desc: 'Height in pixels',
                            hint: 'Example: 1080'
                        }
                    },
                    sscTemplate: 'core.setWindowSize({w}, {h});'
                }
            ]
        }
    ];

    // Quick examples
    /**
     * Quick Examples for the Code Generator.
     * Each example has a descriptive name that matches what the action actually does.
     * All examples use valid, tested Stellarium API calls.
     */
    var quickExamples = {
        'moon-tonight': {
            name: 'View the Moon',
            category: 'objects',
            action: 'objects-focus',
            params: { target: 'Moon', mode: 'zoom' }
        },
        'observe-from-cairo': {
            name: 'Observe from Cairo, Egypt',
            category: 'location',
            action: 'location-set-by-id',
            params: { id: 'Cairo, Northern Africa' }
        },
        'view-jupiter': {
            name: 'Center View on Jupiter',
            category: 'objects',
            action: 'objects-focus',
            params: { target: 'Jupiter', mode: 'center' }
        },
        'speed-up-time': {
            name: 'Speed Up Time (1 hour per second)',
            category: 'time',
            action: 'time-rate',
            params: { timerate: '3600' }
        },
        'toggle-equatorial-grid': {
            name: 'Toggle Equatorial Grid',
            category: 'actions',
            action: 'action-do',
            params: { id: 'actionShow_Equatorial_Grid' }
        },
        'capture-screenshot': {
            name: 'Capture Screenshot',
            category: 'scripts',
            action: 'script-direct',
            params: { 
                code: '// Take a screenshot and save to disk\ncore.screenshot("stellarium_screenshot", false, "", false, "png");\ncore.debug("Screenshot saved successfully.");'
            }
        },
        'toggle-atmosphere': {
            name: 'Toggle Atmosphere Visibility',
            category: 'actions',
            action: 'action-do',
            params: { id: 'actionShow_Atmosphere' }
        },
        'toggle-constellation-lines': {
            name: 'Toggle Constellation Lines',
            category: 'actions',
            action: 'action-do',
            params: { id: 'actionShow_Constellation_Lines' }
        },
        'zoom-to-andromeda': {
            name: 'Zoom to Andromeda Galaxy (M31)',
            category: 'objects',
            action: 'objects-focus',
            params: { target: 'M31', mode: 'zoom' }
        }
    };

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    /**
     * Escape HTML special characters to prevent XSS attacks.
     * 
     * @param {string} text - The text to escape
     * @returns {string} Escaped text safe for HTML insertion
     */
    function escapeHtml(text) {
        if (!text) return '';
        return String(text).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    /**
     * Escape HTML special characters for use in HTML attributes.
     * 
     * @param {string} text - The text to escape
     * @returns {string} Escaped text safe for HTML attributes
     */
    function escapeAttr(text) {
        if (!text) return '';
        return String(text).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/'/g, '&#39;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }
		
		/**
		 * Escape a string for safe inclusion in a double-quoted cURL argument.
		 * Escapes backslash and double quote.
		 */
		function escapeCurlString(str) {
				return String(str).replace(/\\/g, '\\\\').replace(/"/g, '\\"');
		}

		/**
		 * Escape a string for safe inclusion in SSC script (inside double quotes).
		 */
		function escapeSSCString(str) {
				return String(str).replace(/\\/g, '\\\\').replace(/"/g, '\\"');
		}

    /**
     * Convert radians to degrees for display.
     * 
     * @param {number} rad - Value in radians
     * @returns {number} Value in degrees
     */
    function radToDeg(rad) {
        return rad * 180 / Math.PI;
    }

    // =====================================================================
    // STEP NAVIGATION
    // =====================================================================

    /**
     * Navigate to a specific step in the code generation wizard.
     * 
     * @param {number} step - Step number (1-4)
     */
    function goToStep(step) {
        currentStep = step;
        
        $('#gen-step-1, #gen-step-2, #gen-step-3, #gen-step-4').hide();
        $('#gen-step-' + step).show();
        
        $('.generator-progress-steps .progress-step').removeClass('active done');
        for (var i = 1; i <= 4; i++) {
            if (i < step) {
                $('.generator-progress-steps .progress-step[data-step="' + i + '"]').addClass('done');
            } else if (i === step) {
                $('.generator-progress-steps .progress-step[data-step="' + i + '"]').addClass('active');
            }
        }
    }

    // =====================================================================
    // CATEGORY GRID
    // =====================================================================

    /**
     * Build the category selection grid with all available API categories.
     */
    function buildCategoryGrid() {
        var $grid = $('#category-grid');
        if (!$grid.length) return;
        $grid.empty();
        
        var totalActions = 0;
        categories.forEach(function(cat) { totalActions += cat.actions.length; });
        
        categories.forEach(function(cat) {
            var card = $(
                '<div class="category-card" data-category="' + cat.id + '">' +
                '<span class="category-count">' + cat.actions.length + ' actions</span>' +
                '<span class="category-icon">' + (cat.icon || '?') + '</span>' +
                '<span class="category-name">' + escapeHtml(cat.name) + '</span>' +
                '<span class="category-desc">' + escapeHtml(cat.desc) + '</span>' +
                '</div>'
            );
            
            card.on('click', function() {
                var catId = $(this).data('category');
                selectCategory(catId);
            });
            
            $grid.append(card);
        });
        
        console.log('[CodeGenerator] ' + categories.length + ' categories, ' + totalActions + ' total actions');
    }

    /**
     * Select a category and display its actions.
     * 
     * @param {string} categoryId - The category identifier
     */
    function selectCategory(categoryId) {
        selectedCategory = categoryId;
        
        $('.category-card').removeClass('selected');
        $('.category-card[data-category="' + categoryId + '"]').addClass('selected');
        
        goToStep(2);
        buildActionsList(categoryId);
    }

    // =====================================================================
    // ACTIONS LIST
    // =====================================================================

    /**
     * Build the actions list for a selected category.
     * 
     * @param {string} categoryId - The category identifier
     */
    function buildActionsList(categoryId) {
        var cat = categories.find(function(c) { return c.id === categoryId; });
        if (!cat) return;
        
        var $list = $('#gen-actions-list');
        var $title = $('#gen-step-2-title');
        if (!$list.length) return;
        $list.empty();
        
        if ($title.length) {
            $title.text(cat.name + ' - ' + cat.actions.length + ' actions available');
        }
        
        cat.actions.forEach(function(action) {
            var methodClass = (action.method || 'GET').toLowerCase();
            var card = $(
                '<div class="action-card" data-action="' + action.id + '">' +
                '<span class="method-badge ' + methodClass + '">' + action.method + '</span>' +
                '<div class="action-info">' +
                '<span class="action-name">' + escapeHtml(action.name) + '</span>' +
                '<span class="action-desc">' + escapeHtml(action.desc) + '</span>' +
                '</div>' +
                '<span class="action-arrow">&#8592;</span>' +
                '</div>'
            );
            
            card.on('click', function() {
                selectAction(categoryId, action);
            });
            
            $list.append(card);
        });
    }

    /**
     * Select an action and display its parameter form.
     * 
     * @param {string} categoryId - The category identifier
     * @param {Object} action - The action object
     */
    function selectAction(categoryId, action) {
        selectedAction = action;
        
        $('.action-card').removeClass('selected');
        $('.action-card[data-action="' + action.id + '"]').addClass('selected');
        
        goToStep(3);
        buildParamsForm(action);
    }

    // =====================================================================
    // SUGGESTION DROPDOWN
    // =====================================================================

    /**
     * Create a suggestion dropdown for a parameter field.
     * 
     * @param {jQuery} $triggerElement - The suggestion link/button that was clicked
     * @param {jQuery} $targetInput - The input field to fill when user selects
     * @param {string} type - Type of suggestions: 'actions', 'properties', 'scripts', 'locations'
     */
    function createSuggestionDropdown($triggerElement, $targetInput, type) {
        // Remove any existing dropdown
        $('.suggestion-dropdown').remove();
        
        // Create dropdown container
        var $dropdown = $('<div class="suggestion-dropdown"></div>');
        var $searchInput = $('<input type="text" class="suggestion-search" placeholder="Type to filter...">');
        var $list = $('<div class="suggestion-list"></div>');
        
        // Show loading state
        $list.html('<div class="suggestion-loading">Loading...</div>');
        
        // Assemble dropdown
        $dropdown.append($searchInput);
        $dropdown.append($list);
        $('body').append($dropdown);
        
        // Calculate position relative to trigger element
        positionDropdown($dropdown, $triggerElement);
        
        // Fetch data based on type
        fetchSuggestions(type, function(items) {
            renderSuggestionList($list, items, function(selectedValue) {
                $targetInput.val(selectedValue);
                $dropdown.remove();
                $targetInput.focus();
            });
        });
        
        // Bind search filter
        $searchInput.on('input', function() {
            var query = $(this).val().toLowerCase();
            $list.find('.suggestion-item').each(function() {
                var text = $(this).text().toLowerCase();
                $(this).toggle(text.indexOf(query) >= 0);
            });
        });
        
        // Keyboard navigation
        var selectedIndex = -1;
        $searchInput.on('keydown', function(e) {
            var $items = $list.find('.suggestion-item:visible');
            
            if (e.key === 'ArrowDown') {
                e.preventDefault();
                selectedIndex = Math.min(selectedIndex + 1, $items.length - 1);
                updateSelection($items, selectedIndex);
            } else if (e.key === 'ArrowUp') {
                e.preventDefault();
                selectedIndex = Math.max(selectedIndex - 1, 0);
                updateSelection($items, selectedIndex);
            } else if (e.key === 'Enter') {
                e.preventDefault();
                if (selectedIndex >= 0 && selectedIndex < $items.length) {
                    $items.eq(selectedIndex).click();
                }
            } else if (e.key === 'Escape') {
                $dropdown.remove();
                $triggerElement.focus();
            }
        });
        
        // Close dropdown when clicking outside
        $(document).on('mousedown.suggestionDropdown', function(e) {
            if (!$(e.target).closest('.suggestion-dropdown').length && 
                    !$(e.target).is($triggerElement)) {
                $dropdown.remove();
                $(document).off('mousedown.suggestionDropdown');
            }
        });
    }

    /**
     * Position the dropdown relative to the trigger element.
     * 
     * @param {jQuery} $dropdown - The dropdown element
     * @param {jQuery} $triggerElement - The trigger element
     */
    function positionDropdown($dropdown, $triggerElement) {
        var offset = $triggerElement.offset();
        var triggerHeight = $triggerElement.outerHeight();
        
        $dropdown.css({
            position: 'absolute',
            top: offset.top + triggerHeight + 4,
            left: offset.left,
            'z-index': 10000
        });
    }

		/**
		 * Fetch suggestion data from the appropriate API endpoint.
		 * 
		 * @param {string} type - 'actions', 'properties', 'scripts', or 'locations'
		 * @param {function} callback - Called with array of {id, text} objects
		 */
		function fetchSuggestions(type, callback) {
				var endpoints = {
						'actions': '/api/stelaction/list',
						'properties': '/api/stelproperty/list',
						'scripts': '/api/scripts/list',
						'locations': '/api/location/list'
				};
				
				var url = endpoints[type];
				if (!url) {
						callback([{ id: '', text: 'Unknown suggestion type: ' + type }]);
						return;
				}
				
				$.ajax({
						url: url,
						dataType: 'json',
						timeout: 5000
				}).done(function(data) {
						var items = [];
						
						if (type === 'actions') {
								// Flatten categorized action list with category sorting
								var categories = Object.keys(data);
								// Sort categories alphabetically
								categories.sort(function(a, b) {
										return a.localeCompare(b);
								});
								
								categories.forEach(function(category) {
										if (data.hasOwnProperty(category) && Array.isArray(data[category])) {
												var actions = data[category];
												// Sort actions within category by text
												actions.sort(function(a, b) {
														return (a.text || a.id).localeCompare(b.text || b.id);
												});
												
												actions.forEach(function(action) {
														items.push({
																id: action.id,
																text: action.id + (action.text ? ' (' + action.text + ')' : ''),
																category: category,
																isCheckable: action.isCheckable,
																isChecked: action.isChecked
														});
												});
										}
								});
						} else if (type === 'properties') {
								// Properties come as flat object with id as key
								var propIds = Object.keys(data);
								propIds.sort(function(a, b) {
										return a.localeCompare(b);
								});
								
								propIds.forEach(function(propId) {
										var propInfo = data[propId];
										var displayText = propId;
										if (propInfo && propInfo.value !== undefined) {
												var valueStr = typeof propInfo.value === 'string' ? 
														'"' + propInfo.value + '"' : 
														String(propInfo.value);
												if (valueStr.length > 30) {
														valueStr = valueStr.substring(0, 27) + '...';
												}
												displayText = propId + ' = ' + valueStr;
										}
										items.push({
												id: propId,
												text: displayText,
												category: null // No category for properties
										});
								});
						} else if (type === 'scripts' || type === 'locations') {
								// Simple string arrays
								if (Array.isArray(data)) {
										data.forEach(function(item) {
												var name = typeof item === 'string' ? item : (item.name || item.id || String(item));
												items.push({
														id: name,
														text: name,
														category: null
												});
										});
										items.sort(function(a, b) {
												return a.text.localeCompare(b.text);
										});
								}
						}
						
						callback(items);
				}).fail(function() {
						callback([{ id: '', text: 'Error loading suggestions. Check connection.' }]);
				});
		}

		/**
		 * Render the suggestion list with clickable items.
		 * Categories are displayed as sticky headers with proper z-index.
		 * 
		 * @param {jQuery} $list - The list container
		 * @param {Array} items - Array of {id, text, category, ...}
		 * @param {function} onSelect - Called with selected id when user clicks
		 */
		function renderSuggestionList($list, items, onSelect) {
				$list.empty();
				
				if (!items || items.length === 0) {
						$list.html('<div class="suggestion-empty">No results found</div>');
						return;
				}
				
				var currentCategory = null;
				
				items.forEach(function(item) {
						// Add category header if applicable (for actions)
						if (item.category && item.category !== currentCategory) {
								currentCategory = item.category;
								var countInCategory = items.filter(function(i) { 
										return i.category === currentCategory; 
								}).length;
								
								// Category header with proper styling to prevent overlap
								var $category = $('<div class="suggestion-category" data-category="' + escapeAttr(currentCategory) + '">' + 
										escapeHtml(currentCategory) + 
										' (' + countInCategory + ')' +
										'</div>');
								
								// Apply sticky positioning with proper z-index and background
								$category.css({
										'position': 'sticky',
										'top': '0',
										'z-index': '10',
										'background': '#4A4C4E', // Match the dropdown background
										'padding': '5px 12px 3px 12px',
										'font-size': '9px',
										'font-weight': 'bold',
										'color': '#FD971F',
										'text-transform': 'uppercase',
										'letter-spacing': '0.5px',
										'border-bottom': '1px solid rgba(0,0,0,0.15)',
										'border-top': '1px solid rgba(0,0,0,0.1)',
										'margin': '0'
								});
								
								$list.append($category);
						}
						
						var $item = $('<div class="suggestion-item" data-value="' + escapeAttr(item.id) + '">' + 
										escapeHtml(item.text) + 
										'</div>');
						
						$item.on('click', function() {
								onSelect($(this).data('value'));
						});
						
						$list.append($item);
				});
		}

		/**
		 * Update selection highlight in the dropdown list.
		 * Scrolls the selected item into view properly.
		 * 
		 * @param {jQuery} $items - The list items
		 * @param {number} index - Index to select
		 */
		function updateSelection($items, index) {
				$items.removeClass('suggestion-selected');
				if (index >= 0 && index < $items.length) {
						var $selected = $items.eq(index);
						$selected.addClass('suggestion-selected');
						
						// Scroll into view with proper offset for sticky headers
						var container = $selected.parent();
						var containerTop = container.scrollTop();
						var containerHeight = container.height();
						var itemTop = $selected.position().top;
						var itemHeight = $selected.outerHeight();
						
						// Get the height of the sticky category header if visible
						var $category = $selected.prevAll('.suggestion-category:first');
						var headerHeight = $category.length && $category.position().top < 10 ? $category.outerHeight() : 0;
						
						// Adjust scroll position to account for sticky header
						var adjustedItemTop = itemTop - headerHeight;
						
						if (adjustedItemTop < 0 || adjustedItemTop + itemHeight > containerHeight) {
								container.scrollTop(containerTop + adjustedItemTop - containerHeight/2 + itemHeight/2 + headerHeight);
						}
				}
		}

    /**
     * Create a suggestion button with consistent Stellarium styling.
     * Uses simple Unicode symbols that match the editor's monochrome design.
     * 
     * @param {string} text - Button label text
     * @param {string} symbol - Unicode symbol for the button
     * @param {function} onClick - Click handler
     * @param {string} title - Tooltip text
     * @returns {jQuery} The button element
     */
    function createSuggestionButton(text, symbol, onClick, title) {
        var $btn = $('<button type="button" class="suggestion-btn jquerybutton"></button>');
        $btn.html('<span class="suggestion-btn-icon">' + (symbol || '') + '</span> ' + text);
        if (title) $btn.attr('title', title);
        $btn.on('click', function(e) {
            e.preventDefault();
            onClick();
        });
        return $btn;
    }

    // =====================================================================
    // PARAMS FORM
    // =====================================================================

    /**
     * Build the parameters form for a selected action.
     * 
     * @param {Object} action - The action object
     */
    function buildParamsForm(action) {
        var $form = $('#gen-params-form');
        var $title = $('#gen-step-3-title');
        if (!$form.length) return;
        $form.empty();
        
        if ($title.length) {
            $title.text(action.name + ' | Path: /api/' + action.path);
        }
        
        var paramKeys = Object.keys(action.params || {});
        
        if (paramKeys.length === 0) {
            $form.html(
                '<div style="padding:20px;text-align:center;color:rgb(150,150,150);">' +
                '<p>No parameters required for this action.</p>' +
                '<p>Click "Generate Code" to continue directly.</p>' +
                '</div>'
            );
            return;
        }
        
        paramKeys.forEach(function(key) {
            var param = action.params[key];
            var field = $('<div class="param-field"></div>');
            
            var label = $(
                '<div class="param-label">' +
                '<label>' + key + '</label>' +
                '<span class="param-badge ' + (param.required ? 'required' : 'optional') + '">' +
                (param.required ? 'required' : 'optional') + '</span>' +
                '</div>'
            );
            field.append(label);
            
            if (param.desc) {
                field.append('<div class="param-desc">' + escapeHtml(param.desc) + '</div>');
            }
            
            var input;
            if (param.type === 'textarea') {
                input = $('<textarea rows="4" style="width:100%;" placeholder="' + (param.hint || '') + '" data-param-key="' + key + '" data-param-type="textarea"></textarea>');
            } else if (param.type === 'select' && param.options) {
                // FIX: Add data-param-key and data-param-type for select inputs
                input = $('<select style="width:100%;" data-param-key="' + key + '" data-param-type="select"></select>');
                // Add empty option as default when no default value
                var hasDefault = false;
                for (var optIdx = 0; optIdx < param.options.length; optIdx++) {
                    var opt = param.options[optIdx];
                    var selected = (param.defaultValue !== undefined && param.defaultValue === opt);
                    if (selected) hasDefault = true;
                    input.append('<option value="' + opt + '"' + (selected ? ' selected' : '') + '>' + opt + '</option>');
                }
                // Add empty option only if no default and not required
                if (!hasDefault && !param.required) {
                    input.prepend('<option value="">-- Select ' + key + ' --</option>');
                }
            } else if (param.type === 'checkbox') {
                input = $('<input type="checkbox" data-param-key="' + key + '" data-param-type="checkbox">');
                if (param.defaultValue === true) input.prop('checked', true);
            } else if (param.type === 'jday') {
                input = $(
                    '<div style="display:flex;gap:8px;flex-wrap:wrap;">' +
                    '<input type="date" id="gen-date-input" style="flex:1;min-width:150px;" onchange="window._stelCodeGen.updateJDay()">' +
                    '<input type="time" id="gen-time-input" style="flex:1;min-width:120px;" onchange="window._stelCodeGen.updateJDay()" value="12:00">' +
                    '</div>' +
                    '<input type="hidden" id="gen-jday-hidden" value="">' +
                    '<small style="color:rgb(150,150,150);display:block;margin-top:5px;">Or enter Julian Day directly:</small>' +
                    '<input type="number" id="gen-jday-direct" placeholder="2460050.5" step="any" style="margin-top:5px;width:100%;" oninput="window._stelCodeGen.updateJDayFromDirect()">'
                );
            } else if (param.type === 'datetime') {
                input = $(
                    '<div style="display:flex;gap:8px;flex-wrap:wrap;">' +
                    '<input type="date" id="gen-datetime-date" style="flex:1;min-width:150px;">' +
                    '<input type="time" id="gen-datetime-time" style="flex:1;min-width:120px;" value="12:00:00" step="1">' +
                    '</div>'
                );
            } else {
                input = $('<input type="' + (param.type === 'number' ? 'number' : 'text') + 
                    '" placeholder="' + (param.hint || '') + '"' +
                    (param.type === 'number' ? ' step="any"' : '') + 
                    ' data-param-key="' + key + '" data-param-type="' + param.type + '" style="width:100%;">');
            }
            
            field.append(input);
            
            // Add suggestion buttons based on param.suggest attribute
            if (param.suggest === 'search') {
                var $btnLoc = createSuggestionButton(
                    'Browse Locations', '\u2302',  // ⌂ (house/location symbol)
                    function() {
                        var $targetInput = field.find('input[data-param-key="id"]');
                        if ($targetInput.length) {
                            createSuggestionDropdown($btnLoc, $targetInput, 'locations');
                        }
                    },
                    'Search and select from available locations'
                );
                field.append($btnLoc);
                
            } else if (param.suggest === 'actions') {
                var $btnActions = createSuggestionButton(
                    'Browse Actions', '\u25A7',  // ▧ (square with fill - toggle/action symbol)
                    function() {
                        var $targetInput = field.find('input[data-param-key="id"]');
                        if ($targetInput.length) {
                            createSuggestionDropdown($btnActions, $targetInput, 'actions');
                        }
                    },
                    'Search and select from available StelActions'
                );
                field.append($btnActions);
                
            } else if (param.suggest === 'properties') {
                var $btnProps = createSuggestionButton(
                    'Browse Properties', '\u2699',  // ⚙ (gear - properties/settings symbol)
                    function() {
                        var $targetInput = field.find('input[data-param-key="id"]');
                        if ($targetInput.length) {
                            createSuggestionDropdown($btnProps, $targetInput, 'properties');
                        }
                    },
                    'Search and select from available StelProperties'
                );
                field.append($btnProps);
                
            } else if (param.suggest === 'scripts') {
                var $btnScripts = createSuggestionButton(
                    'Browse Scripts', '\u2750',  // ❐ (lower right shadowed white square - script/file)
                    function() {
                        var $targetInput = field.find('input[data-param-key="id"]');
                        if ($targetInput.length) {
                            createSuggestionDropdown($btnScripts, $targetInput, 'scripts');
                        }
                    },
                    'Search and select from installed scripts'
                );
                field.append($btnScripts);
                
            } else if (param.suggest === 'current') {
                var $btnCurrent = createSuggestionButton(
                    'Use Current Time', '\u25F1',  // ◱ (white square with lower left quadrant - clock/time)
                    function() {
                        fillCurrentTime();
                    },
                    'Fill with current date and time'
                );
                field.append($btnCurrent);
            }
            
            $form.append(field);
        });
    }

    // =====================================================================
    // PARAMETER COLLECTION
    // =====================================================================

		/**
		 * Collect parameter values from the form.
		 * FIXED: Now handles vector3 parameters by converting separate x,y,z to JSON array.
		 * FIXED: Properly handles zero values for az/alt angles.
		 * 
		 * @param {Object} action - The action object
		 * @returns {Object} Collected parameters
		 */
		function collectParams(action) {
				var params = {};
				var paramKeys = Object.keys(action.params || {});
				
				paramKeys.forEach(function(key) {
						var param = action.params[key];
						var value;
						
						// ============================================================
						// SPECIAL HANDLING: Vector3 parameters (j2000, jNow, altAz)
						// Convert separate x,y,z fields to JSON array
						// ============================================================
						if (key === 'j2000' && action.params.j2000_x) {
								var x = $('#gen-params-form [data-param-key="j2000_x"]').val();
								var y = $('#gen-params-form [data-param-key="j2000_y"]').val();
								var z = $('#gen-params-form [data-param-key="j2000_z"]').val();
								if (x !== '' && y !== '' && z !== '') {
										value = '[' + parseFloat(x) + ',' + parseFloat(y) + ',' + parseFloat(z) + ']';
										params[key] = value;
								}
								return;
						}
						
						if (key === 'jNow' && action.params.jNow_x) {
								var x = $('#gen-params-form [data-param-key="jNow_x"]').val();
								var y = $('#gen-params-form [data-param-key="jNow_y"]').val();
								var z = $('#gen-params-form [data-param-key="jNow_z"]').val();
								if (x !== '' && y !== '' && z !== '') {
										value = '[' + parseFloat(x) + ',' + parseFloat(y) + ',' + parseFloat(z) + ']';
										params[key] = value;
								}
								return;
						}
						
						if (key === 'altAz' && action.params.altAz_x) {
								var x = $('#gen-params-form [data-param-key="altAz_x"]').val();
								var y = $('#gen-params-form [data-param-key="altAz_y"]').val();
								var z = $('#gen-params-form [data-param-key="altAz_z"]').val();
								if (x !== '' && y !== '' && z !== '') {
										value = '[' + parseFloat(x) + ',' + parseFloat(y) + ',' + parseFloat(z) + ']';
										params[key] = value;
								}
								return;
						}
						
						// ============================================================
						// SPECIAL HANDLING: Position vector for focus
						// ============================================================
						if (key === 'position' && action.params.position_x) {
								var x = $('#gen-params-form [data-param-key="position_x"]').val();
								var y = $('#gen-params-form [data-param-key="position_y"]').val();
								var z = $('#gen-params-form [data-param-key="position_z"]').val();
								if (x !== '' && y !== '' && z !== '') {
										value = '[' + parseFloat(x) + ',' + parseFloat(y) + ',' + parseFloat(z) + ']';
										params[key] = value;
								}
								return;
						}
						
						// ============================================================
						// Standard parameter collection
						// ============================================================
						if (param.type === 'jday') {
								var directVal = $('#gen-jday-direct').val();
								if (directVal && directVal !== '') {
										value = parseFloat(directVal);
								} else {
										var hiddenVal = $('#gen-jday-hidden').val();
										if (hiddenVal && hiddenVal !== '') {
												value = parseFloat(hiddenVal);
										}
								}
								if (value !== undefined && value !== null && !isNaN(value)) {
										params[key] = value;
								}
						} else if (param.type === 'datetime') {
								var dateVal = $('#gen-datetime-date').val();
								var timeVal = $('#gen-datetime-time').val() || '12:00:00';
								if (dateVal && dateVal !== '') {
										value = dateVal + 'T' + timeVal;
										params[key] = value;
								}
						} else if (param.type === 'textarea') {
								value = $('#gen-params-form textarea[data-param-key="' + key + '"]').val();
								if (value !== undefined && value !== null && value !== '') {
										params[key] = value;
								}
						} else if (param.type === 'select') {
								var $select = $('#gen-params-form select[data-param-key="' + key + '"]');
								if ($select.length) {
										value = $select.val();
										// Include non-empty values (empty string is the "-- Select --" option)
										if (value !== undefined && value !== null && value !== '') {
												params[key] = value;
										}
								}
						} else if (param.type === 'checkbox') {
								var $checkbox = $('#gen-params-form input[type="checkbox"][data-param-key="' + key + '"]');
								if ($checkbox.length) {
										value = $checkbox.prop('checked');
										params[key] = value;
								}
						} else if (param.type === 'number' || param.type === 'float' || param.type === 'double') {
								var $numInput = $('#gen-params-form input[data-param-key="' + key + '"]');
								if ($numInput.length) {
										var rawValue = $numInput.val();
										if (rawValue !== undefined && rawValue !== null && rawValue !== '') {
												var numValue = parseFloat(rawValue);
												if (!isNaN(numValue)) {
														// IMPORTANT: Keep zero values (0 is valid for angles)
														params[key] = numValue;
												}
										}
								}
						} else {
								// String and other types
								var $input = $('#gen-params-form input[data-param-key="' + key + '"]');
								if ($input.length) {
										value = $input.val();
										if (value !== undefined && value !== null && value !== '') {
												params[key] = value;
										}
								}
						}
				});
				
				// Special handling for radian to degree conversion if needed
				if (action.needsRadToDeg) {
						if (params.az !== undefined && !isNaN(parseFloat(params.az))) {
								var azDeg = parseFloat(params.az) * 180 / Math.PI;
								params.az_deg = azDeg.toFixed(2);
								delete params.az;
						}
						if (params.alt !== undefined && !isNaN(parseFloat(params.alt))) {
								var altDeg = parseFloat(params.alt) * 180 / Math.PI;
								params.alt_deg = altDeg.toFixed(2);
								delete params.alt;
						}
				}
				
				console.log('[CodeGenerator] Collected params (preserving zeros):', params);
				return params;
		}

    // =====================================================================
    // CODE GENERATION
    // =====================================================================

		/**
		 * Filter out empty or default parameters.
		 * FIXED: Now properly preserves zero values (0) which are valid for angles, FOV, timerate, etc.
		 * FIXED: Properly preserves select values (including "off", "auto", "on")
		 * 
		 * @param {Object} action - The action object
		 * @param {Object} params - Raw collected parameters
		 * @returns {Object} Filtered parameters
		 */
		function filterNonEmptyParams(action, params) {
				var filtered = {};
				var paramDefs = action.params || {};
				
				Object.keys(params).forEach(function(key) {
						var value = params[key];
						var paramDef = paramDefs[key];
						
						// Skip undefined or null
						if (value === undefined || value === null) return;
						
						// ============================================================
						// FIX 1: For numeric parameters, 0 is a VALID value!
						// Examples: az=0 (North), alt=0 (horizon), fov=0.1, timerate=0 (pause)
						// ============================================================
						if (paramDef && (paramDef.type === 'number' || paramDef.type === 'float' || paramDef.type === 'double')) {
								// Check if it's a valid number
								var numValue = parseFloat(value);
								if (!isNaN(numValue)) {
										// Include ALL numbers, including 0, negative numbers, etc.
										filtered[key] = numValue;
								}
								return;
						}
						
						// ============================================================
						// FIX 2: For select inputs, include ALL non-empty values
						// "off", "auto", "on" are all valid meaningful values
						// ============================================================
						if (paramDef && paramDef.type === 'select') {
								if (value !== undefined && value !== null && value !== '') {
										filtered[key] = value;
								}
								return;
						}
						
						// ============================================================
						// For checkbox inputs, false is a VALID value
						// ============================================================
						if (paramDef && paramDef.type === 'checkbox') {
								// Include boolean values (true OR false)
								filtered[key] = value === true || value === false ? value : false;
								return;
						}
						
						// ============================================================
						// For string inputs, skip ONLY empty strings
						// ============================================================
						if (typeof value === 'string') {
								if (value !== '') {
										filtered[key] = value;
								}
								return;
						}
						
						// ============================================================
						// For vector3 or JSON inputs
						// ============================================================
						if (paramDef && paramDef.type === 'vector3') {
								if (value !== undefined && value !== null && value !== '') {
										filtered[key] = value;
								}
								return;
						}
						
						// ============================================================
						// For all other types (jday, datetime, textarea)
						// ============================================================
						if (value !== undefined && value !== null && value !== '') {
								filtered[key] = value;
						}
				});
				
				console.log('[CodeGenerator] Filtered params (preserving zeros):', filtered);
				return filtered;
		}

		/**
		 * Generate code for all formats based on selected action and parameters.
		 */
		function generateCode() {
				if (!selectedAction) return;
				
				var params = collectParams(selectedAction);
				
				// Debug: Check timerate before conversion
				if (selectedAction.id === 'time-rate' || selectedAction.id === 'time-set-jd') {
						console.log('[CodeGenerator] BEFORE conversion - timerate:', params.timerate);
						if (params.timerate !== undefined) {
								var converted = convertUserTimeRateToServer(parseFloat(params.timerate));
								console.log('[CodeGenerator] AFTER conversion - timerate should be:', converted);
						}
				}				

				console.log('[CodeGenerator] Generating code:', selectedAction.id, params);
				
				generatedCodes = {};
				generatedCodes.curl = generateCurl(selectedAction, params);
				generatedCodes.ssc = generateSSC(selectedAction, params);
				generatedCodes.python = generatePython(selectedAction, params);
				generatedCodes.javascript = generateJavaScript(selectedAction, params);
				
				// Create a display version of params with converted timerate values
				var displayParams = {};
				var isTimeAction = (selectedAction.id === 'time-rate' || selectedAction.id === 'time-set-jd');
				
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeAction) {
								// Show user-friendly value in explanation, not the converted server value
								displayParams[k] = params[k];
						} else {
								displayParams[k] = params[k];
						}
				});
				
				addToRecentCodes(selectedAction, generatedCodes);
				goToStep(4);
				displayGeneratedCode('curl');
				generateExplanation(selectedAction, params, displayParams);
		}

		/**
		 * Generate cURL command for the selected action.
		 * FIXED: Properly converts timerate from user value (1 = normal speed) to server JD_SECOND value.
		 * 
		 * @param {Object} action - The action object
		 * @param {Object} params - Collected parameters
		 * @returns {string} cURL command
		 */
		function generateCurl(action, params) {
				var baseUrl = getCurlBaseUrl();
				var url = baseUrl + '/api/' + action.path;
				var cmd = 'curl';
				
				// Check if this action involves timerate parameter
				var isTimeAction = (action.id === 'time-rate' || action.id === 'time-set-jd');
				
				// Create a copy of params with converted timerate values
				var convertedParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeAction && params[k] !== undefined && params[k] !== null) {
								var userRate = parseFloat(params[k]);
								if (!isNaN(userRate)) {
										convertedParams[k] = convertUserTimeRateToServer(userRate);
								} else {
										convertedParams[k] = params[k];
								}
						} else {
								convertedParams[k] = params[k];
						}
				});
				
				// Use the fixed filter function on converted params
				var nonEmptyParams = filterNonEmptyParams(action, convertedParams);
				var paramKeys = Object.keys(nonEmptyParams);
				
				if (action.method === 'GET') {
						if (paramKeys.length > 0) {
								var queryParts = [];
								paramKeys.forEach(function(k) {
										var value = nonEmptyParams[k];
										if (value !== undefined && value !== null) {
												var strValue = String(value);
												queryParts.push(encodeURIComponent(k) + '=' + encodeURIComponent(strValue));
										}
								});
								if (queryParts.length > 0) {
										url += '?' + queryParts.join('&');
								}
						}
						cmd += ' "' + url + '"';
				} else {
						// POST method
						if (paramKeys.length > 0) {
								var dataParts = [];
								paramKeys.forEach(function(k) {
										var value = nonEmptyParams[k];
										if (value !== undefined && value !== null) {
												var strValue = String(value);
												var escapedValue = strValue.replace(/\\/g, '\\\\').replace(/"/g, '\\"');
												dataParts.push('-d "' + k + '=' + escapedValue + '"');
										}
								});
								if (dataParts.length > 0) {
										cmd += ' ' + dataParts.join(' ');
								}
						}
						cmd += ' "' + url + '"';
				}
				
				console.log('[CodeGenerator] cURL generated with converted timerate:', 
										isTimeAction ? 'timerate=' + params.timerate + ' -> ' + convertedParams.timerate : 'no timerate');
				
				return cmd;
		}
		
    /**
     * Generate Stellarium Script (.ssc) for the selected action.
     * 
     * @param {Object} action - The action object
     * @param {Object} params - Collected parameters
     * @returns {string} SSC script
     */
		function generateSSC(action, params) {
				// Check for custom SSC generator
				if (typeof action._generateSSC === 'function') {
						return action._generateSSC(params);
				}
				
				if (!action.sscTemplate) {
						return '// No SSC template available for this action';
				}
				
				var code = action.sscTemplate;
				
				Object.keys(params).forEach(function(k) {
						var regex = new RegExp('\\{' + k + '\\}', 'g');
						var escapedValue = escapeSSCString(params[k]);
						code = code.replace(regex, escapedValue);
				});
				
				return code;
		}

		/**
		 * Generate Python code for the selected action.
		 * FIXED: Now includes ALL parameters including select values.
		 * FIXED: Converts user-friendly timerate (1 = normal speed) to server JD_SECOND value.
		 * 
		 * @param {Object} action - The action object
		 * @param {Object} params - Collected parameters
		 * @returns {string} Python code
		 */
		function generatePython(action, params) {
				var baseUrl = getCurrentBaseUrl();
				var lines = [];
				lines.push('import requests');
				lines.push('import json');
				lines.push('');
				lines.push('# ' + action.name);
				lines.push('# ' + action.desc);
				lines.push('url = "' + baseUrl + '/api/' + action.path + '"');
				lines.push('');
				
				var isTimeAction = (action.id === 'time-rate' || action.id === 'time-set-jd');
				
				// Create converted params for timerate
				var convertedParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeAction && params[k] !== undefined && params[k] !== null) {
								var userRate = parseFloat(params[k]);
								if (!isNaN(userRate)) {
										convertedParams[k] = convertUserTimeRateToServer(userRate);
								} else {
										convertedParams[k] = params[k];
								}
						} else {
								convertedParams[k] = params[k];
						}
				});
				
				// Special handling for vector3 parameters
				var hasVector3 = false;
				var vector3Data = null;
				
				if (action.id === 'view-direction-j2000' && params.j2000_x !== undefined) {
						hasVector3 = true;
						vector3Data = {
								key: 'j2000',
								value: [parseFloat(params.j2000_x), parseFloat(params.j2000_y), parseFloat(params.j2000_z)]
						};
				} else if (action.id === 'view-direction-jnow' && params.jNow_x !== undefined) {
						hasVector3 = true;
						vector3Data = {
								key: 'jNow',
								value: [parseFloat(params.jNow_x), parseFloat(params.jNow_y), parseFloat(params.jNow_z)]
						};
				} else if (action.id === 'view-direction-altaz-vector' && params.altAz_x !== undefined) {
						hasVector3 = true;
						vector3Data = {
								key: 'altAz',
								value: [parseFloat(params.altAz_x), parseFloat(params.altAz_y), parseFloat(params.altAz_z)]
						};
				}
				
				if (hasVector3 && vector3Data) {
						lines.push('data = {');
						lines.push('    "' + vector3Data.key + '": json.dumps(' + JSON.stringify(vector3Data.value) + ')');
						if (params.ref) lines.push('    "ref": "' + params.ref + '"');
						lines.push('}');
						lines.push('');
						lines.push('response = requests.post(url, data=data)');
				} else {
						var nonEmptyParams = filterNonEmptyParams(action, convertedParams);
						var paramKeys = Object.keys(nonEmptyParams);
						
						if (action.method === 'GET') {
								if (paramKeys.length > 0) {
										lines.push('params = {');
										paramKeys.forEach(function(k) {
												var value = nonEmptyParams[k];
												lines.push('    "' + k + '": ' + (typeof value === 'number' ? value : '"' + value + '"') + ',');
										});
										lines.push('}');
										lines.push('response = requests.get(url, params=params)');
								} else {
										lines.push('response = requests.get(url)');
								}
						} else {
								if (paramKeys.length > 0) {
										lines.push('data = {');
										paramKeys.forEach(function(k) {
												var value = nonEmptyParams[k];
												lines.push('    "' + k + '": ' + (typeof value === 'number' ? value : '"' + value + '"') + ',');
										});
										lines.push('}');
										lines.push('response = requests.post(url, data=data)');
								} else {
										lines.push('response = requests.post(url)');
								}
						}
				}
				
				lines.push('');
				lines.push('if response.status_code == 200:');
				lines.push('    print("Success:", response.text)');
				lines.push('else:');
				lines.push('    print("Error:", response.status_code, response.text)');
				
				return lines.join('\n');
		}

		/**
		 * Generate JavaScript code for the selected action.
		 * FIXED: Now includes ALL parameters including select values.
		 * FIXED: Converts user-friendly timerate (1 = normal speed) to server JD_SECOND value.
		 * 
		 * @param {Object} action - The action object
		 * @param {Object} params - Collected parameters
		 * @returns {string} JavaScript code
		 */
		function generateJavaScript(action, params) {
				var baseUrl = getCurrentBaseUrl();
				var lines = [];
				lines.push('// ' + action.name);
				lines.push('// ' + action.desc);
				lines.push('const url = "' + baseUrl + '/api/' + action.path + '";');
				lines.push('');
				
				var isTimeAction = (action.id === 'time-rate' || action.id === 'time-set-jd');
				
				// Create converted params for timerate
				var convertedParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeAction && params[k] !== undefined && params[k] !== null) {
								var userRate = parseFloat(params[k]);
								if (!isNaN(userRate)) {
										convertedParams[k] = convertUserTimeRateToServer(userRate);
								} else {
										convertedParams[k] = params[k];
								}
						} else {
								convertedParams[k] = params[k];
						}
				});
				
				var nonEmptyParams = filterNonEmptyParams(action, convertedParams);
				var paramKeys = Object.keys(nonEmptyParams);
				
				if (action.method === 'GET') {
						if (paramKeys.length > 0) {
								var queryParts = [];
								paramKeys.forEach(function(k) {
										var value = nonEmptyParams[k];
										if (value !== undefined && value !== null && value !== '') {
												queryParts.push(encodeURIComponent(k) + '=' + encodeURIComponent(String(value)));
										}
								});
								if (queryParts.length > 0) {
										lines.push('fetch(url + "?" + ' + JSON.stringify(queryParts.join('&')) + ')');
								} else {
										lines.push('fetch(url)');
								}
						} else {
								lines.push('fetch(url)');
						}
				} else {
						if (paramKeys.length > 0) {
								var bodyParts = [];
								paramKeys.forEach(function(k) {
										var value = nonEmptyParams[k];
										if (value !== undefined && value !== null && value !== '') {
												bodyParts.push(encodeURIComponent(k) + '=' + encodeURIComponent(String(value)));
										}
								});
								var bodyString = bodyParts.join('&');
								lines.push('fetch(url, {');
								lines.push('    method: "POST",');
								lines.push('    headers: { "Content-Type": "application/x-www-form-urlencoded" },');
								lines.push('    body: "' + bodyString + '"');
								lines.push('})');
						} else {
								lines.push('fetch(url, { method: "POST" })');
						}
				}
				
				lines.push('    .then(response => response.text())');
				lines.push('    .then(data => console.log("Success:", data))');
				lines.push('    .catch(error => console.error("Error:", error));');
				
				return lines.join('\n');
		}

    /**
     * Display generated code in the specified format.
     * 
     * @param {string} format - The format to display ('curl', 'ssc', 'python', 'javascript')
     */
    function displayGeneratedCode(format) {
        $('.gen-tab').removeClass('active');
        $('.gen-tab[data-format="' + format + '"]').addClass('active');
        
        var code = generatedCodes[format] || '// No code available';
        $('#generated-code-display').text(code);
    }

		/**
		 * Generate explanation text for the generated code.
		 * FIXED: Now displays converted timerate values correctly.
		 * 
		 * @param {Object} action - The action object
		 * @param {Object} params - Collected parameters (original user values)
		 * @param {Object} convertedParams - Converted parameters for display (optional)
		 */
		function generateExplanation(action, params, convertedParams) {
				var html = '';
				html += '<div class="explain-title">Generated Code Explanation</div>';
				html += '<p>This code performs: <strong>' + escapeHtml(action.name) + '</strong></p>';
				html += '<p>' + escapeHtml(action.desc) + '</p>';
				html += '<p><strong>Path:</strong> <code>/api/' + escapeHtml(action.path) + '</code></p>';
				html += '<p><strong>Method:</strong> <code>' + escapeHtml(action.method) + '</code></p>';
				
				if (action.source) {
						html += '<p><strong>Source:</strong> <code>' + escapeHtml(action.source) + '</code></p>';
				}
				
				// Determine which params to display
				var displayParams = convertedParams || params;
				var paramKeys = Object.keys(displayParams);
				
				if (paramKeys.length > 0) {
						html += '<p><strong>Parameters:</strong></p><ul>';
						paramKeys.forEach(function(k) {
								var rawValue = displayParams[k];
								var displayValue;
								
								// For timerate parameter, show user-friendly explanation
								if (k === 'timerate' && (action.id === 'time-rate' || action.id === 'time-set-jd')) {
										var userRate = parseFloat(params.timerate);
										if (!isNaN(userRate)) {
												if (userRate === 0) {
														displayValue = escapeHtml(String(userRate)) + ' (paused)';
												} else if (Math.abs(userRate - 1) < 0.0001) {
														displayValue = '1 (normal speed - 1 second per second)';
												} else if (userRate === 3600) {
														displayValue = '3600 (1 hour per second)';
												} else if (userRate === 86400) {
														displayValue = '86400 (1 day per second)';
												} else {
														displayValue = escapeHtml(String(userRate)) + 'x normal speed';
												}
										} else {
												displayValue = escapeHtml(String(rawValue));
										}
								} else {
										displayValue = escapeHtml(String(rawValue));
								}
								
								// Escape parameter name as well
								var safeParamName = escapeHtml(String(k));
								
								html += '<li><code>' + safeParamName + '</code> = <strong>' + displayValue + '</strong></li>';
						});
						html += '</ul>';
				}
				
				html += '<p style="margin-top:10px;"><strong>Tip:</strong> You can copy this code and use it directly in the terminal or in your own application.</p>';
				
				$('#generated-explanation').html(html);
		}

    // =====================================================================
    // RECENT CODES
    // =====================================================================

    /**
     * Add generated code to recent history.
     * 
     * @param {Object} action - The action object
     * @param {Object} codes - Generated codes object
     */
    function addToRecentCodes(action, codes) {
        recentCodes.unshift({
            time: new Date(),
            actionName: action.name,
            actionDesc: action.desc,
            codePreview: codes.curl.substring(0, 80),
            codes: codes
        });
        
        if (recentCodes.length > maxRecentCodes) {
            recentCodes.pop();
        }
        
        saveRecentCodes();
        renderRecentCodes();
    }

    /**
     * Render recent codes history in the sidebar.
     */
    function renderRecentCodes() {
        var $container = $('#recent-codes');
        if (!$container.length) return;
        
        if (recentCodes.length === 0) {
            $container.html('<span class="output-placeholder">No codes generated yet</span>');
            return;
        }
        
        var html = '';
        recentCodes.forEach(function(item, index) {
            html += '<div class="recent-code-item" data-index="' + index + '">';
            html += '<div class="recent-title">' + escapeHtml(item.actionName) + '</div>';
            html += '<div class="recent-desc">' + escapeHtml(item.actionDesc || '') + '</div>';
            html += '<div class="recent-code-preview">' + escapeHtml(item.codePreview || '') + '...</div>';
            html += '</div>';
        });
        
        $container.html(html);
        
        $container.find('.recent-code-item').on('click', function() {
            var index = parseInt($(this).data('index'));
            if (recentCodes[index]) {
                generatedCodes = recentCodes[index].codes;
                goToStep(4);
                displayGeneratedCode('curl');
                $('#generated-explanation').html(
                    '<div class="explain-title">Code Retrieved from History</div>' +
                    '<p>Retrieved from: <strong>' + recentCodes[index].time.toLocaleString() + '</strong></p>'
                );
            }
        });
    }

    /**
     * Save recent codes to localStorage.
     */
    function saveRecentCodes() {
        try {
            var toSave = recentCodes.map(function(item) {
                return {
                    time: item.time.getTime(),
                    actionName: item.actionName,
                    actionDesc: item.actionDesc,
                    codePreview: item.codePreview,
                    codes: item.codes
                };
            });
            localStorage.setItem('stellarium-recent-codes', JSON.stringify(toSave));
        } catch(e) {}
    }

    /**
     * Load recent codes from localStorage.
     */
    function loadRecentCodes() {
        try {
            var saved = localStorage.getItem('stellarium-recent-codes');
            if (saved) {
                var parsed = JSON.parse(saved);
                recentCodes = parsed.map(function(item) {
                    item.time = new Date(item.time);
                    return item;
                });
                renderRecentCodes();
            }
        } catch(e) {}
    }

    // =====================================================================
    // DATE/JDAY HELPERS
    // =====================================================================

    /**
     * Convert JavaScript Date to Julian Day.
     * 
     * @param {Date} date - JavaScript Date object
     * @returns {number} Julian Day
     */
    function dateToJDay(date) {
        var year = date.getUTCFullYear();
        var month = date.getUTCMonth() + 1;
        var day = date.getUTCDate();
        var hour = date.getUTCHours() + date.getUTCMinutes()/60 + date.getUTCSeconds()/3600;
        
        if (month <= 2) { year--; month += 12; }
        var A = Math.floor(year / 100);
        var B = 2 - A + Math.floor(A / 4);
        
        return Math.floor(365.25 * (year + 4716)) +
               Math.floor(30.6001 * (month + 1)) +
               day + hour/24 + B - 1524.5;
    }

    /**
     * Fill the current time in all time-related input fields.
     */
    function fillCurrentTime() {
        var now = new Date();
        var jday = dateToJDay(now);
        $('#gen-jday-direct').val(jday.toFixed(4));
        $('#gen-jday-hidden').val(jday.toFixed(4));
        
        var dateStr = now.toISOString().split('T')[0];
        var timeStr = now.toTimeString().split(' ')[0];
        $('#gen-datetime-date').val(dateStr);
        $('#gen-datetime-time').val(timeStr);
    }

    /**
     * Update Julian Day from date/time inputs.
     */
    function updateJDay() {
        var dateVal = $('#gen-date-input').val();
        var timeVal = $('#gen-time-input').val() || '12:00';
        
        if (dateVal) {
            var dateTimeStr = dateVal + 'T' + timeVal + ':00';
            var date = new Date(dateTimeStr + 'Z');
            if (!isNaN(date.getTime())) {
                var jday = dateToJDay(date);
                $('#gen-jday-hidden').val(jday.toFixed(4));
                $('#gen-jday-direct').val(jday.toFixed(4));
            }
        }
    }

    /**
     * Update date/time inputs from direct Julian Day input.
     */
    function updateJDayFromDirect() {
        var directVal = $('#gen-jday-direct').val();
        if (directVal) {
            $('#gen-jday-hidden').val(directVal);
        }
    }

    // =====================================================================
    // BUTTON ACTIONS
    // =====================================================================

    /**
     * Copy generated code to clipboard.
     */
    function copyGeneratedCode() {
        var activeFormat = $('.gen-tab.active').data('format') || 'curl';
        var code = generatedCodes[activeFormat] || '';
        
        if (!code) {
            alert('No code to copy. Generate code first.');
            return;
        }
        
        navigator.clipboard.writeText(code).then(function() {
            var $btn = $('#btn-copy-generated');
            var originalText = $btn.text();
            $btn.text('Copied!');
            setTimeout(function() { $btn.text(originalText); }, 3000);
        }).catch(function() {
            var textarea = document.createElement('textarea');
            textarea.value = code;
            textarea.style.position = 'fixed';
            textarea.style.opacity = '0';
            document.body.appendChild(textarea);
            textarea.select();
            try { document.execCommand('copy'); } catch(err) {}
            document.body.removeChild(textarea);
        });
    }

		/**
		 * Send generated code directly to Stellarium for execution.
		 * FIXED: Now converts timerate from user value (1 = normal speed) to server JD_SECOND value.
		 * Uses relative paths to ensure compatibility with any Stellarium configuration.
		 */
		function sendGeneratedCode() {
				if (!selectedAction) {
						alert('No action selected.');
						return;
				}
				
				var params = collectParams(selectedAction);
				var fullPath = selectedAction.path;
				
				// Check if this action involves timerate parameter
				var isTimeAction = (selectedAction.id === 'time-rate' || selectedAction.id === 'time-set-jd');
				
				// Create a copy of params with converted timerate values for server
				var serverParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeAction && params[k] !== undefined && params[k] !== null) {
								var userRate = parseFloat(params[k]);
								if (!isNaN(userRate)) {
										serverParams[k] = convertUserTimeRateToServer(userRate);
										console.log('[CodeGenerator] Send: timerate converted from', userRate, 'to', serverParams[k]);
								} else {
										serverParams[k] = params[k];
								}
						} else {
								serverParams[k] = params[k];
						}
				});
				
				// Ensure path starts with /api/
				if (fullPath.indexOf('/api/') !== 0) {
						fullPath = '/api/' + fullPath;
				}
				
				if (typeof rc !== 'undefined' && rc.postCmd) {
						if (selectedAction.method === 'POST') {
								rc.postCmd(fullPath, serverParams, function() {
										console.log('[CodeGenerator] Request completed');
								}, function(data) {
										if (data && data.trim() && data.trim() !== 'ok') {
												alert('Response:\n' + data);
										} else {
												alert('Command sent successfully.');
										}
								});
						} else {
								$.ajax({
										url: fullPath,
										method: 'GET',
										data: serverParams,
										dataType: 'text',
										success: function(data) {
												alert('Response:\n' + (data || '(empty)'));
										},
										error: function(xhr, status, error) {
												alert('Error: ' + (error || 'Unknown error'));
										}
								});
						}
				} else {
						var method = selectedAction.method || 'POST';
						var ajaxOptions = {
								url: fullPath,
								method: method,
								dataType: 'text',
								success: function(data) {
										alert('Response:\n' + (data || '(empty)'));
								},
								error: function(xhr, status, error) {
										alert('Error: ' + (error || xhr.statusText || 'Unknown error'));
								}
						};
						
						if (method === 'GET') {
								ajaxOptions.data = serverParams;
						} else {
								var formData = [];
								Object.keys(serverParams).forEach(function(k) {
										formData.push(encodeURIComponent(k) + '=' + encodeURIComponent(String(serverParams[k])));
								});
								ajaxOptions.data = formData.join('&');
								ajaxOptions.contentType = 'application/x-www-form-urlencoded';
						}
						
						$.ajax(ajaxOptions);
				}
		}

    /**
     * Insert generated SSC code into the Script Editor.
     */
    function insertInEditor() {
        var sscCode = generatedCodes.ssc || '';
        
        if (!sscCode) {
            alert('No SSC code available. Generate code first.');
            return;
        }
        
        var $tabs = $('#actions-sub-tabs');
        if ($tabs.length && typeof $tabs.tabs === 'function') {
            $tabs.tabs('option', 'active', 0);
        }
        
        setTimeout(function() {
            if (window._stelScriptEditor && typeof window._stelScriptEditor.insertAtCursor === 'function') {
                window._stelScriptEditor.insertAtCursor('\n' + sscCode + '\n');
                console.log('[CodeGenerator] Code inserted into editor');
            } else {
                var editorTextarea = document.getElementById('script-editor');
                if (editorTextarea) {
                    if (typeof CodeMirror !== 'undefined') {
                        var cmElements = document.querySelectorAll('.CodeMirror');
                        for (var i = 0; i < cmElements.length; i++) {
                            if (cmElements[i].CodeMirror) {
                                var cm = cmElements[i].CodeMirror;
                                cm.focus();
                                cm.replaceSelection('\n' + sscCode + '\n');
                                cm.refresh();
                                return;
                            }
                        }
                    }
                    editorTextarea.value = (editorTextarea.value || '') + '\n' + sscCode + '\n';
                    editorTextarea.focus();
                }
            }
        }, 300);
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the Code Generator module.
     * Builds the category grid, loads history, and binds events.
     */
    function init() {
        console.log('[CodeGenerator] Initializing...');
        console.log('[CodeGenerator] Loaded ' + categories.length + ' categories with ' + 
            categories.reduce(function(sum, cat) { return sum + cat.actions.length; }, 0) + ' actions');
        
        // Log the resolved base URL for debugging
        var baseUrl = getCurrentBaseUrl();
        console.log('[CodeGenerator] Base URL resolved:', baseUrl);
        
        buildCategoryGrid();
        loadRecentCodes();
        bindEvents();
        
        console.log('[CodeGenerator] Initialized');
    }

    /**
     * Bind all UI event handlers.
     */
    function bindEvents() {
        $('#code-generator-main').on('click', '.back-btn', function() {
            var backStep = parseInt($(this).data('back'));
            goToStep(backStep);
        });
        
        $('#btn-generate-code').on('click', function() {
            generateCode();
        });
        
        $('#code-generator-main').on('click', '.gen-tab', function() {
            var format = $(this).data('format');
            displayGeneratedCode(format);
        });
        
        $('#btn-copy-generated').on('click', function() {
            copyGeneratedCode();
        });
        
        $('#btn-send-generated').on('click', function() {
            sendGeneratedCode();
        });
        
        $('#btn-insert-editor').on('click', function() {
            insertInEditor();
        });
        
        $('#quick-examples').on('click', '.quick-example', function() {
            var exampleId = $(this).data('example');
            var example = quickExamples[exampleId];
            
            if (example) {
                selectCategory(example.category);
                
                setTimeout(function() {
                    var cat = categories.find(function(c) { return c.id === example.category; });
                    if (cat) {
                        var action = cat.actions.find(function(a) { return a.id === example.action; });
                        if (action) {
                            selectAction(example.category, action);
                            
                            setTimeout(function() {
                                Object.keys(example.params).forEach(function(key) {
                                    var input = $('#gen-params-form [data-param-key="' + key + '"]');
                                    if (input.length) {
                                        input.val(example.params[key]);
                                    }
                                });
                                generateCode();
                            }, 300);
                        }
                    }
                }, 200);
            }
        });
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    // Expose helpers globally for other modules
    window._stelCodeGen = {
        updateJDay: updateJDay,
        updateJDayFromDirect: updateJDayFromDirect,
        fillCurrentTime: fillCurrentTime,
        setConfig: setCodeGeneratorConfig,
        getBaseUrl: getCurrentBaseUrl
    };

    return {
        init: init,
        generateCode: generateCode,
        selectCategory: selectCategory,
        copyGeneratedCode: copyGeneratedCode,
        insertInEditor: insertInEditor,
        sendGeneratedCode: sendGeneratedCode,
        setConfig: setCodeGeneratorConfig,
        getBaseUrl: getCurrentBaseUrl
    };
});
