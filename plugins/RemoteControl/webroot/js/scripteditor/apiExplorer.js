/* ========================================================================
 * apiExplorer.js - Interactive API Explorer for Stellarium Remote Control
 * ========================================================================
 * 
 * This module provides an interactive API explorer that displays available
 * services and allows users to experiment with API operations.
 * 
 * Features:
 * - Service tree with expandable operation lists
 * - Operation details panel with parameter forms
 * - Request execution with response display (formatted/raw toggle)
 * - cURL command generation with dynamic base URL resolution
 * - Service filtering by name
 * - Text direction toggle for RTL languages (Arabic, Hebrew)
 * - Response formatting with syntax highlighting (JSON, HTML, plain text)
 * - Copy response to clipboard
 * - Real-time response status display
 * - Parameter validation and type-aware input fields
 * - Support for all 9 API services (Main, Objects, Location, Scripts, etc.)
 * 
 * Technical Implementation:
 * - Dynamically builds service tree from service definitions
 * - Supports GET and POST methods with parameter encoding
 * - Automatic response type detection (JSON/HTML/TEXT)
 * - JSON syntax highlighting with recursion
 * - Table formatting for array responses
 * - Dynamic URL resolution for generated cURL commands
 * - Works with any host/port configuration
 * 
 * Supported Services:
 * - Main Service (time, fov, focus, move, view, status, plugins, window)
 * - Object Service (find, info, listobjecttypes, listobjectsbytype)
 * - Location Service (setlocationfields, planetlist, regionlist)
 * - Location Search Service (search, nearby)
 * - Script Service (list, info, run, direct, status, stop)
 * - StelAction Service (list, do)
 * - StelProperty Service (list, set)
 * - SIMBAD Service (lookup)
 * - View Service (listlandscape, landscapedescription, listskyculture, skyculturedescription, listprojection, projectiondescription)
 * 
 * @module apiExplorer
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

    var currentService = null;
    var currentOperation = null;
    var currentOpData = null;
    var currentRawResponse = null;      // Store raw response for toggling
    var currentFormattedHtml = null;    // Store formatted HTML for toggling
    var currentDisplayMode = 'formatted'; // 'formatted' or 'raw'		
		var currentTextDirection = 'ltr'; // 'ltr' or 'rtl'


    // =====================================================================
    // CONFIGURATION
    // =====================================================================

    /**
     * Configuration object for the API Explorer.
     * Users can override these values if needed for custom deployments.
     * 
     * @type {Object}
     * @property {boolean} forceHttpsForCurl - Force HTTPS protocol for cURL commands
     * @property {string} customHostname - Custom hostname for cURL (empty = use current)
     * @property {string} customPort - Custom port for cURL (empty = use current)
     * @property {boolean} debugUrls - Log resolved URLs for debugging
     */
    var ApiExplorerConfig = {
        forceHttpsForCurl: false,
        customHostname: '',
        customPort: '',
        debugUrls: true
    };
		
		// =====================================================================
		// TIME CONVERSION UTILITIES
		// =====================================================================

		// JD_SECOND: One second expressed in Julian Days (1/86400)
		var JD_SECOND = 0.000011574074074074074;

		// Convert user time rate to server value
		function convertUserTimeRateToServer(userRate) {
				if (userRate === 0) return 0;
				if (userRate === undefined || userRate === null) return JD_SECOND;
				return parseFloat(userRate) * JD_SECOND;
		}

		/**
		 * Convert ISO 8601 date string to Julian Day.
		 * @param {string} isoString - ISO 8601 date string (e.g., "2023-06-01T03:51:40")
		 * @returns {number} Julian Day number
		 */
		function isoToJDay(isoString) {
				var date = new Date(isoString);
				if (isNaN(date.getTime())) return null;
				
				var year = date.getUTCFullYear();
				var month = date.getUTCMonth() + 1;
				var day = date.getUTCDate();
				var hour = date.getUTCHours();
				var minute = date.getUTCMinutes();
				var second = date.getUTCSeconds();
				
				// Calculate Julian Day
				if (month <= 2) {
						year -= 1;
						month += 12;
				}
				
				var A = Math.floor(year / 100);
				var B = 2 - A + Math.floor(A / 4);
				
				var jd = Math.floor(365.25 * (year + 4716)) +
								 Math.floor(30.6001 * (month + 1)) +
								 day + B - 1524.5;
				
				// Add time fraction
				var timeFraction = (hour + minute / 60 + second / 3600) / 24;
				jd += timeFraction;
				
				return jd;
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
        
        if (ApiExplorerConfig.debugUrls) {
            console.log('[ApiExplorer] Base URL resolved:', baseUrl);
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
        var hostname = ApiExplorerConfig.customHostname || window.location.hostname;
        var port = ApiExplorerConfig.customPort || window.location.port;
        var protocol = ApiExplorerConfig.forceHttpsForCurl ? 'https' : 'http';
        
        var baseUrl = protocol + '://' + hostname;
        if (port && port !== '80' && port !== '443') {
            baseUrl += ':' + port;
        }
        
        if (ApiExplorerConfig.debugUrls) {
            console.log('[ApiExplorer] cURL Base URL resolved:', baseUrl);
        }
        
        return baseUrl;
    }

    /**
     * Set custom configuration for the API Explorer.
     * Allows users to override hostname, port, or protocol settings.
     * 
     * @param {Object} config - Configuration options
     * @param {boolean} [config.forceHttpsForCurl] - Force HTTPS for cURL
     * @param {string} [config.customHostname] - Custom hostname for cURL
     * @param {string} [config.customPort] - Custom port for cURL
     * @param {boolean} [config.debugUrls] - Enable URL debugging
     */
    function setApiExplorerConfig(config) {
        if (config.forceHttpsForCurl !== undefined) {
            ApiExplorerConfig.forceHttpsForCurl = config.forceHttpsForCurl;
        }
        if (config.customHostname !== undefined) {
            ApiExplorerConfig.customHostname = config.customHostname;
        }
        if (config.customPort !== undefined) {
            ApiExplorerConfig.customPort = config.customPort;
        }
        if (config.debugUrls !== undefined) {
            ApiExplorerConfig.debugUrls = config.debugUrls;
        }
        
        console.log('[ApiExplorer] Configuration updated:', ApiExplorerConfig);
    }

    // =====================================================================
    // SERVICE DEFINITIONS
    // =====================================================================

    var services = {
        main: {
            name: 'Main Service',
            path: 'main',
            operations: {
                status: {
                    method: 'GET',
                    desc: 'Get full program state snapshot',
                    parameters: {
                        actionId: { required: false, type: 'number', description: 'Last known action change ID' },
                        propId: { required: false, type: 'number', description: 'Last known property change ID' }
                    }
                },
								time: {
										method: 'POST',
										desc: 'Set simulation time and/or time rate',
										parameters: {
												time: { 
														required: false, 
														type: 'datetime',
														desc: 'Date and time (ISO 8601) - will be converted to Julian Day' 
												},
												timerate: { 
														required: false, 
														type: 'number', 
														desc: 'Time rate (1 = normal, 0 = pause)' 
												}
										}
								},
                fov: {
                    method: 'POST',
                    desc: 'Set field of view in degrees',
                    parameters: {
                        fov: { required: true, type: 'number', description: 'FOV in degrees (0.1 to 235)' }
                    }
                },
                focus: {
                    method: 'POST',
                    desc: 'Focus on a celestial object',
                    parameters: {
                        target: { required: true, type: 'string', description: 'Object name' },
                        mode: { required: false, type: 'string', description: 'Focus mode: center, zoom, mark' }
                    }
                },
                move: {
                    method: 'POST',
                    desc: 'Continuous view movement (joystick style)',
                    parameters: {
                        x: { required: true, type: 'number', description: 'Horizontal speed (-1 to 1)' },
                        y: { required: true, type: 'number', description: 'Vertical speed (-1 to 1)' }
                    }
                },
                view: {
                    method: 'GET',
                    desc: 'Get current view direction',
                    parameters: {
                        coord: { required: false, type: 'string', description: 'Coordinate system: j2000, jNow, altAz' }
                    }
                },
                plugins: {
                    method: 'GET',
                    desc: 'List installed plugins',
                    parameters: {}
                }
            }
        },
        objects: {
            name: 'Object Service',
            path: 'objects',
            operations: {
                find: {
                    method: 'GET',
                    desc: 'Search for objects in catalogs',
                    parameters: {
                        str: { required: true, type: 'string', description: 'Search string' }
                    }
                },
                info: {
                    method: 'GET',
                    desc: 'Get object information',
                    parameters: {
                        name: { required: false, type: 'string', description: 'Object name (empty for selected)' },
                        format: { required: false, type: 'string', description: 'Output format: html, json, map' }
                    }
                },
                listobjecttypes: {
                    method: 'GET',
                    desc: 'List available object types',
                    parameters: {}
                },
                listobjectsbytype: {
                    method: 'GET',
                    desc: 'List objects of a specific type',
                    parameters: {
                        type: { required: true, type: 'string', description: 'Object type key (AsterismMgr, ConstellationMgr, StarMgr, StarMgr: 0, NebulaMgr: 108 ... )' },
                        english: { required: false, type: 'string', description: 'Use English names (0 or 1)' }
                    }
                }
            }
        },
        location: {
            name: 'Location Service',
            path: 'location',
            operations: {
                setlocationfields: {
                    method: 'POST',
                    desc: 'Set observer location fields',
                    parameters: {
                        id: { required: false, type: 'string', description: 'Location ID from database' },
                        latitude: { required: false, type: 'number', description: 'Latitude in degrees' },
                        longitude: { required: false, type: 'number', description: 'Longitude in degrees' },
                        altitude: { required: false, type: 'number', description: 'Altitude in meters' },
                        planet: { required: false, type: 'string', description: 'Planet name' }
                    }
                },
                planetlist: {
                    method: 'GET',
                    desc: 'List available planets',
                    parameters: {}
                },
                regionlist: {
                    method: 'GET',
                    desc: 'List available regions',
                    parameters: {}
                }
            }
        },
        locationsearch: {
            name: 'Location Search Service',
            path: 'locationsearch',
            operations: {
                search: {
                    method: 'GET',
                    desc: 'Search locations by name',
                    parameters: {
                        term: { required: true, type: 'string', description: 'Search term' }
                    }
                },
                nearby: {
                    method: 'GET',
                    desc: 'Search locations near coordinates',
                    parameters: {
                        planet: { required: true, type: 'string', description: 'Planet name' },
                        latitude: { required: true, type: 'number', description: 'Latitude' },
                        longitude: { required: true, type: 'number', description: 'Longitude' },
                        radius: { required: false, type: 'number', description: 'Search radius in degrees' }
                    }
                }
            }
        },
        scripts: {
            name: 'Script Service',
            path: 'scripts',
            operations: {
                list: {
                    method: 'GET',
                    desc: 'List available scripts',
                    parameters: {}
                },
                info: {
                    method: 'GET',
                    desc: 'Get script information',
                    parameters: {
                        id: { required: true, type: 'string', description: 'Script filename' }
                    }
                },
                run: {
                    method: 'POST',
                    desc: 'Run an installed script',
                    parameters: {
                        id: { required: true, type: 'string', description: 'Script filename' }
                    }
                },
                direct: {
                    method: 'POST',
                    desc: 'Run script code directly',
                    parameters: {
                        code: { required: true, type: 'string', description: 'Script code' },
                        useIncludes: { required: false, type: 'string', description: 'Use includes directory' }
                    }
                },
                status: {
                    method: 'GET',
                    desc: 'Get script execution status',
                    parameters: {}
                },
                stop: {
                    method: 'POST',
                    desc: 'Stop running script',
                    parameters: {}
                }
            }
        },
        stelaction: {
            name: 'StelAction Service',
            path: 'stelaction',
            operations: {
                list: {
                    method: 'GET',
                    desc: 'List all registered actions',
                    parameters: {}
                },
                do: {
                    method: 'POST',
                    desc: 'Execute a StelAction',
                    parameters: {
                        id: { required: true, type: 'string', description: 'Action identifier' }
                    }
                }
            }
        },
        stelproperty: {
            name: 'StelProperty Service',
            path: 'stelproperty',
            operations: {
                list: {
                    method: 'GET',
                    desc: 'List all registered properties',
                    parameters: {}
                },
                set: {
                    method: 'POST',
                    desc: 'Set a property value',
                    parameters: {
                        id: { required: true, type: 'string', description: 'Property identifier' },
                        value: { required: true, type: 'string', description: 'New value' }
                    }
                }
            }
        },
        simbad: {
            name: 'SIMBAD Service',
            path: 'simbad',
            operations: {
                lookup: {
                    method: 'GET',
                    desc: 'Search SIMBAD database',
                    parameters: {
                        str: { required: true, type: 'string', description: 'Search query' }
                    }
                }
            }
        },
        view: {
            name: 'View Service',
            path: 'view',
            operations: {
                listlandscape: {
                    method: 'GET',
                    desc: 'List available landscapes',
                    parameters: {}
                },
                landscapedescription: {
                    method: 'GET',
                    desc: 'Get current landscape description (HTML)',
                    parameters: {},
                    fullPath: 'view/landscapedescription/'
                },
                listskyculture: {
                    method: 'GET',
                    desc: 'List available sky cultures',
                    parameters: {}
                },
                skyculturedescription: {
                    method: 'GET',
                    desc: 'Get current sky culture description (HTML)',
                    parameters: {},
                    fullPath: 'view/skyculturedescription/'
                },
                listprojection: {
                    method: 'GET',
                    desc: 'List available projections',
                    parameters: {}
                },
                projectiondescription: {
                    method: 'GET',
                    desc: 'Get current projection description',
                    parameters: {}
                }
            }
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
        var div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    /**
     * Format an operation name for display (convert underscores to spaces and capitalize).
     * 
     * @param {string} name - The operation name
     * @returns {string} Formatted operation name
     */
    function formatOperationName(name) {
        return name.replace(/_/g, ' ').replace(/\b\w/g, function(l) { return l.toUpperCase(); });
    }

    /**
     * Get the icon character for a service key.
     * 
     * @param {string} key - The service key
     * @returns {string} Icon character
     */
    function getServiceIcon(key) {
        var icons = {
            'main': 'M',
            'objects': 'O',
            'location': 'L',
            'locationsearch': 'S',
            'scripts': 'R',
            'simbad': 'W',
            'stelaction': 'A',
            'stelproperty': 'P',
            'view': 'V'
        };
        return icons[key] || '?';
    }

		// =====================================================================
		// RESPONSE DIRECTION TOGGLE
		// =====================================================================

		/**
		 * Toggle the text direction of the HTML response area between LTR and RTL.
		 * This is specifically useful for languages like Arabic and Hebrew.
		 */
		function toggleResponseDirection() {
				var $responseWrapper = $('#explorer-response-area .response-content-wrapper');
				if (!$responseWrapper.length) return;

				if (currentTextDirection === 'ltr') {
						currentTextDirection = 'rtl';
						$('#toggle-dir-btn').html('[Switch to LTR]');
				} else {
						currentTextDirection = 'ltr';
						$('#toggle-dir-btn').html('[Switch to RTL]');
				}

				// Apply direction to the main container
				$responseWrapper.css('direction', currentTextDirection);
				
				// For HTML responses, also apply dir="auto" to specific elements
				// to ensure mixed content (numbers, English) is displayed correctly.
				if (currentFormattedHtml && currentFormattedHtml.indexOf('html-response') !== -1) {
						$responseWrapper.find('.html-response').css('direction', currentTextDirection);
						// Ensure tables and paragraphs inherit the direction
						$responseWrapper.find('.html-response table, .html-response p, .html-response div').css('direction', currentTextDirection);
				}
				
				// If it's a table response (like planetlist), adjust the table container
				if ($responseWrapper.find('.response-table').length) {
						$responseWrapper.find('.response-table').css('direction', currentTextDirection);
						// Align table header text appropriately
						$responseWrapper.find('th').css('text-align', currentTextDirection === 'rtl' ? 'right' : 'left');
						$responseWrapper.find('td').css('text-align', currentTextDirection === 'rtl' ? 'right' : 'left');
				}

				console.log('[ApiExplorer] Response direction toggled to:', currentTextDirection);
		}
		
		// =====================================================================
		// RESPONSE DISPLAY MODES
		// =====================================================================

    /**
     * Toggle between formatted and raw response display.
     */
    function toggleResponseDisplay() {
        if (currentDisplayMode === 'formatted') {
            // Switch to raw mode
            currentDisplayMode = 'raw';
            $('#explorer-response-area .response-content-wrapper').html(
                '<pre class="raw-response" style="white-space: pre-wrap; word-break: break-word; margin: 0; font-family: monospace; font-size: 11px;">' + 
                escapeHtml(currentRawResponse) + 
                '</pre>'
            );
            $('#toggle-display-btn').html('[Show Formatted]');
        } else {
            // Switch to formatted mode
            currentDisplayMode = 'formatted';
            $('#explorer-response-area .response-content-wrapper').html(currentFormattedHtml);
            $('#toggle-display-btn').html('[Show Raw]');
        }
    }

    /**
     * Extract object key from name for display (for listobjecttypes response).
     * 
     * @param {Object} item - The object item
     * @returns {string} Display key
     */
    function getDisplayKey(item) {
        if (item.key !== undefined) {
            return item.key;
        }
        if (item.id !== undefined) {
            return item.id;
        }
        return '';
    }
		
		// =====================================================================
    // RESPONSE FORMATTING UTILITIES
    // =====================================================================

    /**
     * Format a JSON response for display with syntax highlighting.
     * Handles both objects and arrays, with proper indentation.
     * 
     * @param {string} responseText - The raw response text from the server
     * @returns {string} Formatted HTML string with syntax highlighting
     */
    function formatResponse(responseText) {
        if (!responseText || responseText.trim() === '') {
            return '<span style="color: rgb(150,150,150);">(empty response)</span>';
        }
        
        var trimmed = responseText.trim();
        var formattedHtml = '';
        
        // Try to parse as JSON
        try {
            var parsed = JSON.parse(trimmed);
            formattedHtml = formatJsonValue(parsed, 0);
            return '<div class="json-viewer">' + formattedHtml + '</div>';
        } catch(e) {
            // Not valid JSON, check if it's an HTML response
            if (trimmed.startsWith('<') && trimmed.includes('>')) {
                return '<div class="html-response">' + trimmed + '</div>';
            }
            // Plain text response
            return '<pre style="white-space: pre-wrap; margin: 0;">' + escapeHtml(trimmed) + '</pre>';
        }
    }

    /**
     * Recursively format a JSON value with syntax highlighting.
     * 
     * @param {*} value - The JSON value to format
     * @param {number} depth - Current nesting depth (for indentation)
     * @returns {string} Formatted HTML string
     */
    function formatJsonValue(value, depth) {
        var indent = '  '.repeat(depth);
        var nextIndent = '  '.repeat(depth + 1);
        
        if (value === null) {
            return '<span class="json-null">null</span>';
        }
        
        if (typeof value === 'boolean') {
            return '<span class="json-boolean">' + value + '</span>';
        }
        
        if (typeof value === 'number') {
            return '<span class="json-number">' + value + '</span>';
        }
        
        if (typeof value === 'string') {
            // Check if the string contains HTML
            if (value.trim().startsWith('<') && value.includes('>')) {
                return '<span class="json-string html-content" title="HTML content">"' + escapeHtml(value) + '"</span>';
            }
            return '<span class="json-string">"' + escapeHtml(value) + '"</span>';
        }
        
        if (Array.isArray(value)) {
            if (value.length === 0) {
                return '<span class="json-array-empty">[]</span>';
            }
            
            var arrayHtml = '<span class="json-array-bracket">[</span>\n';
            for (var i = 0; i < value.length; i++) {
                arrayHtml += nextIndent + formatJsonValue(value[i], depth + 1);
                if (i < value.length - 1) {
                    arrayHtml += '<span class="json-comma">,</span>';
                }
                arrayHtml += '\n';
            }
            arrayHtml += indent + '<span class="json-array-bracket">]</span>';
            
            // For small arrays (less than 5 items), display inline for better readability
            if (value.length <= 5 && depth === 0) {
                var inlineArray = '<span class="json-array-bracket">[</span> ';
                for (var j = 0; j < value.length; j++) {
                    inlineArray += formatJsonValue(value[j], depth);
                    if (j < value.length - 1) {
                        inlineArray += '<span class="json-comma">, </span>';
                    }
                }
                inlineArray += ' <span class="json-array-bracket">]</span>';
                return inlineArray;
            }
            
            return arrayHtml;
        }
        
        if (typeof value === 'object') {
            var keys = Object.keys(value);
            if (keys.length === 0) {
                return '<span class="json-object-empty">{}</span>';
            }
            
            var objectHtml = '<span class="json-object-brace">{</span>\n';
            for (var k = 0; k < keys.length; k++) {
                var key = keys[k];
                var keyValue = value[key];
                
                objectHtml += nextIndent + '<span class="json-key">"' + escapeHtml(key) + '"</span>';
                objectHtml += '<span class="json-colon">: </span>';
                objectHtml += formatJsonValue(keyValue, depth + 1);
                if (k < keys.length - 1) {
                    objectHtml += '<span class="json-comma">,</span>';
                }
                objectHtml += '\n';
            }
            objectHtml += indent + '<span class="json-object-brace">}</span>';
            return objectHtml;
        }
        
        return escapeHtml(String(value));
    }

    /**
     * Format an array response in a table format for better readability.
     * Used for script lists, planet lists, and other array responses.
     * 
     * @param {Array} array - The array to format
     * @returns {string} Formatted HTML table
     */
    function formatArrayAsTable(array) {
        if (!array || array.length === 0) {
            return '<div class="array-empty">No items found</div>';
        }
        
        // Detect if array contains objects with key property (like listobjecttypes)
        var hasKeyProperty = array.length > 0 && 
                             typeof array[0] === 'object' && 
                             array[0].key !== undefined;
        
        // Detect if array contains objects with name/name_i18n properties
        var hasNamedObjects = array.length > 0 && 
                              typeof array[0] === 'object' && 
                              (array[0].name !== undefined || array[0].name_i18n !== undefined);
        
        // For listobjecttypes response (has key, name, name_i18n)
        if (hasKeyProperty) {
            var tableHtml = '<table class="response-table">';
            tableHtml += '<thead><tr>';
            tableHtml += '<th>#</th>';
            tableHtml += '<th>Key</th>';
            tableHtml += '<th>Name</th>';
            if (array[0].name_i18n !== undefined) {
                tableHtml += '<th>Name (Localized)</th>';
            }
            tableHtml += '</tr></thead><tbody>';
            
            for (var i = 0; i < array.length; i++) {
                var item = array[i];
                tableHtml += '<tr>';
                tableHtml += '<td class="row-number">' + (i + 1) + '</td>';
                tableHtml += '<td class="item-key">' + escapeHtml(item.key || '-') + '</td>';
                tableHtml += '<td class="item-name">' + escapeHtml(item.name || '-') + '</td>';
                if (item.name_i18n !== undefined) {
                    tableHtml += '<td class="item-name-i18n">' + escapeHtml(item.name_i18n || '-') + '</td>';
                }
                tableHtml += '</tr>';
            }
            tableHtml += '</tbody></table>';
            return tableHtml;
        }
        
        // For planet list and similar (has name, name_i18n)
        if (hasNamedObjects) {
            var tableHtml2 = '<table class="response-table">';
            tableHtml2 += '<thead><tr>';
            tableHtml2 += '<th>#</th>';
            tableHtml2 += '<th>Name</th>';
            if (array[0].name_i18n !== undefined) {
                tableHtml2 += '<th>Name (Localized)</th>';
            }
            tableHtml2 += '</tr></thead><tbody>';
            
            for (var j = 0; j < array.length; j++) {
                var item2 = array[j];
                tableHtml2 += '<tr>';
                tableHtml2 += '<td class="row-number">' + (j + 1) + '</td>';
                tableHtml2 += '<td class="item-name">' + escapeHtml(item2.name || '-') + '</td>';
                if (item2.name_i18n !== undefined) {
                    tableHtml2 += '<td class="item-name-i18n">' + escapeHtml(item2.name_i18n || '-') + '</td>';
                }
                tableHtml2 += '</tr>';
            }
            tableHtml2 += '</tbody></table>';
            return tableHtml2;
        }
        
        // Simple array - format as numbered list
        var listHtml = '<div class="array-list">';
        for (var k = 0; k < array.length; k++) {
            var item3 = array[k];
            if (typeof item3 === 'object') {
                listHtml += '<div class="array-item">';
                listHtml += '<span class="array-index">' + (k + 1) + '.</span> ';
                listHtml += '<pre class="array-item-json">' + escapeHtml(JSON.stringify(item3, null, 2)) + '</pre>';
                listHtml += '</div>';
            } else {
                listHtml += '<div class="array-item">';
                listHtml += '<span class="array-index">' + (k + 1) + '.</span> ';
                listHtml += '<span class="array-value">' + escapeHtml(String(item3)) + '</span>';
                listHtml += '</div>';
            }
        }
        listHtml += '</div>';
        return listHtml;
    }

    /**
     * Detect response type and format appropriately.
     * 
     * @param {string} responseText - The raw response text
     * @param {string} responseType - Optional type hint ('json', 'html', 'text')
     * @returns {string} Formatted HTML
     */
    function formatResponseByType(responseText, responseType) {
        if (!responseText || responseText.trim() === '') {
            return '<div class="response-empty">(empty response)</div>';
        }
        
        var trimmed = responseText.trim();
        
        // Try to detect array of objects (like planetlist response)
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            try {
                var parsed = JSON.parse(trimmed);
                if (Array.isArray(parsed)) {
                    // Use table format for arrays (will handle key property automatically)
                    return formatArrayAsTable(parsed);
                }
                // Fallback to JSON viewer
                return '<div class="json-viewer">' + formatJsonValue(parsed, 0) + '</div>';
            } catch(e) {
                // Not valid JSON, continue
            }
        }
        
        // Try to parse as JSON object (not array)
        try {
            var parsed = JSON.parse(trimmed);
            if (typeof parsed === 'object' && !Array.isArray(parsed)) {
                return '<div class="json-viewer">' + formatJsonValue(parsed, 0) + '</div>';
            }
        } catch(e) {
            // Not valid JSON
        }
        
        // Check if it's HTML
        if (trimmed.startsWith('<') && trimmed.includes('>')) {
            return '<div class="html-response">' + trimmed + '</div>';
        }
        
        // Plain text - show as is
        return '<pre class="plain-text-response">' + escapeHtml(trimmed) + '</pre>';
    }

    /**
     * Get a human-readable response type label.
     * 
     * @param {string} responseText - The raw response text
     * @returns {string} Type label
     */
    function getResponseTypeLabel(responseText) {
        if (!responseText || responseText.trim() === '') {
            return 'EMPTY';
        }
        
        var trimmed = responseText.trim();
        
        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            try {
                var parsed = JSON.parse(trimmed);
                if (Array.isArray(parsed)) {
                    if (parsed.length > 0 && typeof parsed[0] === 'string') {
                        return 'STRING ARRAY (' + parsed.length + ' items)';
                    }
                    if (parsed.length > 0 && typeof parsed[0] === 'object') {
                        return 'OBJECT ARRAY (' + parsed.length + ' items)';
                    }
                    return 'ARRAY (' + parsed.length + ' items)';
                }
            } catch(e) {}
            return 'JSON ARRAY';
        }
        
        if (trimmed.startsWith('{') && trimmed.endsWith('}')) {
            try {
                var parsed = JSON.parse(trimmed);
                var keyCount = Object.keys(parsed).length;
                return 'JSON OBJECT (' + keyCount + ' keys)';
            } catch(e) {}
            return 'JSON OBJECT';
        }
        
        if (trimmed.startsWith('<') && trimmed.includes('>')) {
            return 'HTML';
        }
        
        return 'TEXT';
    }

    // =====================================================================
    // SERVICE TREE
    // =====================================================================

    /**
     * Build the service tree UI with expandable operation lists.
     */
    function buildServiceTree() {
        var $tree = $('#api-service-tree');
        if (!$tree.length) return;
        $tree.empty();

        var serviceKeys = Object.keys(services).sort();

        serviceKeys.forEach(function(key) {
            var service = services[key];
            var $node = $('<div class="service-node"></div>');
            
            var $header = $(
                '<div class="service-header" data-service="' + key + '">' +
                '<span class="icon">' + getServiceIcon(key) + '</span>' +
                '<span class="name">' + escapeHtml(service.name) + '</span>' +
                '<span class="arrow">&#9654;</span>' +
                '</div>'
            );
            
            var $opList = $('<div class="operation-list" data-service="' + key + '"></div>');
            
            var opKeys = Object.keys(service.operations).sort();
            opKeys.forEach(function(opKey) {
                var op = service.operations[opKey];
                var method = (op.method || 'GET').toLowerCase();
                var $opItem = $(
                    '<div class="operation-item" data-service="' + key + '" data-operation="' + opKey + '">' +
                    '<span class="method-badge ' + method + '">' + op.method + '</span>' +
                    '<span>' + formatOperationName(opKey) + '</span>' +
                    '</div>'
                );
                $opList.append($opItem);
            });
            
            $node.append($header);
            $node.append($opList);
            $tree.append($node);
        });

        bindTreeEvents();
    }

    /**
     * Bind events for service tree interaction.
     */
    function bindTreeEvents() {
        // Expand/collapse service
        $('#api-service-tree').off('click', '.service-header').on('click', '.service-header', function() {
            var $header = $(this);
            var serviceKey = $header.data('service');
            var $opList = $('.operation-list[data-service="' + serviceKey + '"]');
            
            $header.toggleClass('expanded');
            $opList.toggleClass('expanded');
        });

        // Select operation
        $('#api-service-tree').off('click', '.operation-item').on('click', '.operation-item', function() {
            var $this = $(this);
            var serviceKey = $this.data('service');
            var operationKey = $this.data('operation');
            
            $('.operation-item.active').removeClass('active');
            $this.addClass('active');
            
            currentService = serviceKey;
            currentOperation = operationKey;
            currentOpData = services[serviceKey].operations[operationKey];
            
            showOperationPanel(serviceKey, operationKey);
        });

        // Filter services
        $('#api-service-filter').off('input').on('input', function() {
            var query = $(this).val().toLowerCase();
            
            if (query.length === 0) {
                $('.service-node').show();
                return;
            }
            
            $('.service-node').each(function() {
                var $node = $(this);
                var headerText = $node.find('.service-header .name').text().toLowerCase();
                var hasMatch = headerText.indexOf(query) >= 0;
                
                if (!hasMatch) {
                    $node.find('.operation-item').each(function() {
                        if ($(this).text().toLowerCase().indexOf(query) >= 0) {
                            hasMatch = true;
                            return false;
                        }
                    });
                }
                
                $node.toggle(hasMatch);
            });
        });
    }

    // =====================================================================
    // OPERATION PANEL
    // =====================================================================

    /**
     * Display the operation panel for a selected operation.
     * 
     * @param {string} serviceKey - The service key
     * @param {string} operationKey - The operation key
     */
    function showOperationPanel(serviceKey, operationKey) {
        var service = services[serviceKey];
        var op = currentOpData;
        
        if (!op) return;
        
        $('#api-welcome-message').hide();
        var $panel = $('#api-operation-panel');
        $panel.show();
        
        var method = (op.method || 'GET').toLowerCase();
        var methodClass = method === 'get' ? 'get' : 'post';
        
        // Build full path
        var fullPath;
        if (op.fullPath) {
            fullPath = op.fullPath;
        } else {
            fullPath = service.path + '/' + operationKey;
        }
        
        var html = '<div class="operation-details">';
        
        // Title
        html += '<h2>';
        html += '<span class="method-badge ' + methodClass + '">' + op.method + '</span> ';
        html += formatOperationName(operationKey);
        html += '</h2>';
        
        // Path
        html += '<div class="operation-path">';
        html += '/api/' + fullPath;
        html += '</div>';
        
        // Description
        html += '<div class="operation-desc">' + op.desc + '</div>';
        
        // Parameters
        var paramKeys = Object.keys(op.parameters || {});
        var requiredParams = paramKeys.filter(function(k) { return op.parameters[k].required; });
        var optionalParams = paramKeys.filter(function(k) { return !op.parameters[k].required; });
        
        html += '<div class="params-section">';
        html += '<h3>Parameters</h3>';
        
        if (paramKeys.length === 0) {
            html += '<p style="color: rgb(150,150,150);">No parameters required</p>';
        } else {
            if (requiredParams.length > 0) {
                html += '<h4 style="color: #F92672;">Required:</h4>';
                requiredParams.forEach(function(key) {
                    html += renderParamField(key, op.parameters[key], true);
                });
            }
            
            if (optionalParams.length > 0) {
                html += '<h4 style="color: rgb(150,150,150); margin-top: 15px;">Optional:</h4>';
                optionalParams.forEach(function(key) {
                    html += renderParamField(key, op.parameters[key], false);
                });
            }
        }
        
        html += '</div>';
        
        // Action buttons
        html += '<div style="margin: 15px 0; display: flex; gap: 10px; flex-wrap: wrap;">';
        html += '<button class="jquerybutton" id="btn-explorer-execute">Execute Request</button>';
        html += '<button class="jquerybutton" id="btn-explorer-curl">Generate cURL</button>';
        html += '</div>';
        
        // Response area
        html += '<div class="response-section">';
        html += '<h3>Response <span id="explorer-response-status"></span></h3>';
        html += '<div class="response-area" id="explorer-response-area">';
        html += '<span style="color: rgb(150,150,150);">Response will appear here after execution...</span>';
        html += '</div>';
        html += '</div>';
        
        html += '</div>';
        
        $panel.html(html);
        
        // Bind new buttons
        $('#btn-explorer-execute').on('click', function() {
            executeRequest();
        });
        
        $('#btn-explorer-curl').on('click', function() {
            generateCurlCommand();
        });
    }

		/**
		 * Render a parameter field in the operation panel.
		 * FIXED: Now properly renders datetime picker for time parameter.
		 * 
		 * @param {string} key - Parameter key
		 * @param {Object} param - Parameter definition
		 * @param {boolean} required - Whether the parameter is required
		 * @returns {string} HTML string for the parameter field
		 * Render a parameter field in the operation panel.
		 * FIXED: Now properly renders datetime picker with date and time inputs.
		 */
		function renderParamField(key, param, required) {
				var html = '<div class="param-group">';
				html += '<label>' + key;
				html += required ? ' <span class="param-required">*</span>' : ' <span class="param-optional">(optional)</span>';
				html += '</label>';
				
				// Special handling for datetime type
				if (param.type === 'datetime') {
						var now = new Date();
						var defaultDate = now.toISOString().split('T')[0];
						var defaultTime = now.toTimeString().split(' ')[0];
						
						html += '<div style="display:flex;gap:8px;flex-wrap:wrap;">';
						html += '<input type="date" id="explorer-param-' + key + '-date" style="flex:1;min-width:150px;" value="' + defaultDate + '">';
						html += '<input type="time" id="explorer-param-' + key + '-time" style="flex:1;min-width:120px;" value="' + defaultTime + '" step="1">';
						html += '</div>';
						html += '<input type="hidden" id="explorer-param-' + key + '">';
				} else {
						var inputType = param.type === 'number' ? 'number' : 'text';
						html += '<input type="' + inputType + '" id="explorer-param-' + key + '" ';
						html += 'placeholder="' + escapeHtml(param.description || '') + '"';
						html += (inputType === 'number' ? ' step="any"' : '') + ' style="width:100%;">';
				}
				
				if (param.description) {
						html += '<span class="param-hint">' + escapeHtml(param.description) + '</span>';
				}
				
				html += '</div>';
				return html;
		}

    // =====================================================================
    // REQUEST EXECUTION
    // =====================================================================

		/**
		 * Collect parameter values from the operation panel form.
		 * FIXED: Converts ISO datetime to Julian Day for time parameter.
		 * 
		 * @returns {Object} Collected parameters
		 */
		function collectExplorerParams() {
				var params = {};
				
				if (!currentOpData || !currentOpData.parameters) return params;
				
				Object.keys(currentOpData.parameters).forEach(function(key) {
						var paramDef = currentOpData.parameters[key];
						var $input = $('#explorer-param-' + key);
						
						if (!$input.length) return;
						
						var value;
						
						// Special handling for time parameter with datetime picker
						if (key === 'time' && paramDef.type === 'datetime') {
								var dateVal = $('#explorer-param-time-date').val();
								var timeVal = $('#explorer-param-time-time').val() || '12:00:00';
								if (dateVal && dateVal !== '') {
										var isoString = dateVal + 'T' + timeVal;
										// Convert ISO to Julian Day
										var jd = isoToJDay(isoString);
										if (jd !== null) {
												value = jd;
												console.log('[ApiExplorer] Converted ISO', isoString, 'to JD', jd);
										} else {
												value = isoString;
										}
								}
						} else if ($input.is('select')) {
								value = $input.val();
						} else if ($input.is(':checkbox')) {
								value = $input.prop('checked');
						} else {
								value = $input.val();
						}
						
						if (value !== undefined && value !== null && value !== '') {
								params[key] = value;
						}
				});
				
				// For time operation, ensure timerate is included if time is present
				if (currentOperation === 'time' && params.time !== undefined && params.timerate === undefined) {
						params.timerate = 1;
						console.log('[ApiExplorer] Added default timerate=1 because time was specified');
				}
				
				return params;
		}

		/**
		 * Execute the current API request and display the response.
		 * FIXED: Converts timerate and ISO datetime to proper server values.
		 */
		function executeRequest() {
				var $responseArea = $('#explorer-response-area');
				var $statusEl = $('#explorer-response-status');
				
				$responseArea.html('<div class="response-loading"><span class="loading-spinner"></span> Executing request...</div>');
				$statusEl.text('');
				
				var params = collectExplorerParams();
				var service = services[currentService];
				var op = currentOpData;
				
				// Check if this is a time operation that needs timerate conversion
				var isTimeOperation = (currentOperation === 'time');
				
				// Convert parameters for server
				var serverParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeOperation && params[k] !== undefined && params[k] !== null && params[k] !== '') {
								serverParams[k] = convertUserTimeRateToServer(params[k]);
								console.log('[ApiExplorer] timerate converted from', params[k], 'to', serverParams[k]);
						} else {
								serverParams[k] = params[k];
						}
				});
				
				// Build full path
				var fullPath;
				if (op.fullPath) {
						fullPath = op.fullPath;
				} else {
						fullPath = service.path + '/' + currentOperation;
				}
				
				// Use relative path
				var baseUrl = '/api/';
				
				var ajaxOptions = {
						url: baseUrl + fullPath,
						dataType: 'text',
						success: function(data, textStatus, xhr) {
								var statusCode = xhr.status;
								var statusText = statusCode === 200 ? 'OK' : (xhr.statusText || '');
								$statusEl.html('<span class="response-status ' + (statusCode === 200 ? 'success' : 'error') + '">HTTP ' + statusCode + ' ' + statusText + '</span>');
								
								currentRawResponse = data;
								currentDisplayMode = 'formatted';
								
								var formattedContent = formatResponseByType(data);
								currentFormattedHtml = formattedContent;
								
								var responseType = getResponseTypeLabel(data);
								var responseSize = data ? data.length : 0;
								var sizeInfo = responseSize > 0 ? ' (' + formatBytes(responseSize) + ')' : '';
								
								$responseArea.html(
										'<div class="response-toolbar">' +
										'<div class="response-toolbar-left">' +
										'<span class="response-type-badge">' + escapeHtml(responseType) + sizeInfo + '</span>' +
										'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">[Show Raw]</button>' +
										'<button class="toggle-dir-btn jquerybutton" id="toggle-dir-btn" title="Toggle text direction for RTL languages (e.g., Arabic, Hebrew)">[Switch to RTL]</button>' +
										'</div>' +
										'<div class="response-toolbar-right">' +
										'<button class="copy-btn jquerybutton" id="copy-response-btn">Copy</button>' +
										'</div>' +
										'</div>' +
										'<div class="response-content-wrapper" style="direction: ltr;">' +
										formattedContent +
										'</div>'
								);
								
								$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
								$('#toggle-dir-btn').off('click').on('click', function() { toggleResponseDirection(); });
								$('#copy-response-btn').off('click').on('click', function() { copyResponse(); });
						},
						error: function(xhr, status, errorThrown) {
								var statusCode = xhr.status || 0;
								$statusEl.html('<span class="response-status error">HTTP ' + statusCode + ' ' + (xhr.statusText || 'Error') + '</span>');
								
								var errorContent = '<div class="response-error">';
								errorContent += '<div class="error-icon">!</div>';
								errorContent += '<div class="error-message"><strong>Error:</strong> ' + escapeHtml(errorThrown || 'Request failed') + '</div>';
								if (xhr.responseText) {
										errorContent += '<div class="error-details"><strong>Response:</strong><pre>' + escapeHtml(xhr.responseText) + '</pre></div>';
								}
								errorContent += '</div>';
								
								currentRawResponse = errorThrown + '\n' + (xhr.responseText || '');
								currentFormattedHtml = errorContent;
								currentDisplayMode = 'formatted';
								
								$responseArea.html(
										'<div class="response-toolbar">' +
										'<div class="response-toolbar-left">' +
										'<span class="response-type-badge error">ERROR</span>' +
										'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">[Show Raw]</button>' +
										'</div>' +
										'<div class="response-toolbar-right">' +
										'<button class="copy-btn jquerybutton" id="copy-response-btn">Copy</button>' +
										'</div>' +
										'</div>' +
										'<div class="response-content-wrapper">' +
										errorContent +
										'</div>'
								);
								
								$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
								$('#copy-response-btn').off('click').on('click', function() { copyResponse(); });
						}
				};
				
				if (op.method === 'GET') {
						ajaxOptions.method = 'GET';
						if (Object.keys(serverParams).length > 0) {
								ajaxOptions.data = serverParams;
						}
				} else {
						ajaxOptions.method = 'POST';
						ajaxOptions.data = serverParams;
				}
				
				$.ajax(ajaxOptions);
		}

    /**
     * Format bytes to human-readable string.
     * 
     * @param {number} bytes - Number of bytes
     * @returns {string} Human-readable size string
     */
    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + ' B';
        if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
        return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
    }

		/**
		 * Generate a cURL command for the current operation.
		 * Uses dynamic base URL resolution to work with any Stellarium configuration.
		 * FIXED: Converts timerate from user value (1 = normal speed) to server JD_SECOND value.
		 * FIXED: Properly includes time parameter with ISO format.
		 * Generate a cURL command for the current operation.
		 * FIXED: Converts timerate and ISO datetime to proper server values.
		 */
		function generateCurlCommand() {
				var params = collectExplorerParams();
				var service = services[currentService];
				var op = currentOpData;
				
				console.log('[ApiExplorer] Collected params for cURL:', params);
				
				// Check if this is a time operation that needs conversion
				var isTimeOperation = (currentOperation === 'time');
				
				// Convert parameters for server
				var convertedParams = {};
				Object.keys(params).forEach(function(k) {
						if (k === 'timerate' && isTimeOperation && params[k] !== undefined && params[k] !== null && params[k] !== '') {
								convertedParams[k] = convertUserTimeRateToServer(params[k]);
								console.log('[ApiExplorer] timerate converted from', params[k], 'to', convertedParams[k]);
						} else {
								convertedParams[k] = params[k];
						}
				});
				
				var fullPath;
				if (op.fullPath) {
						fullPath = op.fullPath;
				} else {
						fullPath = service.path + '/' + currentOperation;
				}
				
				var baseUrl = getCurlBaseUrl();
				var cmd = 'curl';
				
				if (op.method === 'GET') {
						if (Object.keys(convertedParams).length > 0) {
								cmd += ' -G';
								Object.keys(convertedParams).forEach(function(key) {
										var value = convertedParams[key];
										cmd += ' -d "' + key + '=' + encodeURIComponent(String(value)) + '"';
								});
						}
				} else {
						if (Object.keys(convertedParams).length > 0) {
								Object.keys(convertedParams).forEach(function(key) {
										var value = convertedParams[key];
										cmd += ' -d "' + key + '=' + encodeURIComponent(String(value)) + '"';
								});
						}
				}
				
				cmd += ' ' + baseUrl + '/api/' + fullPath;
				
				console.log('[ApiExplorer] Generated cURL:', cmd);
				
				currentRawResponse = cmd;
				currentFormattedHtml = '<pre class="curl-command">' + escapeHtml(cmd) + '</pre>';
				currentDisplayMode = 'formatted';
				
				var $responseArea = $('#explorer-response-area');
				var $statusEl = $('#explorer-response-status');
				
				$statusEl.text('');
				$responseArea.html(
						'<div class="response-toolbar">' +
						'<div class="response-toolbar-left">' +
						'<span class="response-type-badge">CURL COMMAND</span>' +
						'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">[Show Raw]</button>' +
						'</div>' +
						'<div class="response-toolbar-right">' +
						'<button class="copy-btn jquerybutton" id="copy-response-btn">Copy</button>' +
						'</div>' +
						'</div>' +
						'<div class="response-content-wrapper">' +
						'<pre class="curl-command">' + escapeHtml(cmd) + '</pre>' +
						'</div>'
				);
				
				$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
				$('#copy-response-btn').off('click').on('click', function() { copyResponse(); });
		}

    /**
     * Copy the response content to clipboard.
     * Always copies the RAW response text, not the formatted HTML.
     */
    function copyResponse() {
        var textToCopy = currentRawResponse || '';
        
        if (!textToCopy) {
            textToCopy = 'No response data to copy.';
        }
        
        navigator.clipboard.writeText(textToCopy).then(function() {
            var $btn = $('#copy-response-btn');
            var originalText = $btn.html();
            $btn.html('Copied!');
            setTimeout(function() { $btn.html(originalText); }, 2000);
        }).catch(function() {
            // Fallback for older browsers
            var textarea = document.createElement('textarea');
            textarea.value = textToCopy;
            textarea.style.position = 'fixed';
            textarea.style.left = '-9999px';
            document.body.appendChild(textarea);
            textarea.select();
            try {
                document.execCommand('copy');
                var $btn = $('#copy-response-btn');
                var originalText = $btn.html();
                $btn.html('Copied!');
                setTimeout(function() { $btn.html(originalText); }, 2000);
            } catch(e) {
                alert('Copy failed. Please select and copy manually.');
            }
            document.body.removeChild(textarea);
        });
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the API Explorer module.
     * Builds the service tree and sets up the initial view.
     */
    function init() {
        console.log('[ApiExplorer] Initializing...');
        
        // Log the resolved base URL for debugging
        var baseUrl = getCurrentBaseUrl();
        console.log('[ApiExplorer] Base URL resolved:', baseUrl);
        
        buildServiceTree();
        
        // Set page to show welcome by default
        $('#api-welcome-message').show();
        $('#api-operation-panel').hide();
        
        console.log('[ApiExplorer] Initialized');
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    // Expose helpers globally for other modules
    window._stelApiExplorer = {
        copyResponse: copyResponse,
        setConfig: setApiExplorerConfig,
        getBaseUrl: getCurrentBaseUrl
    };

    return {
        init: init,
        executeRequest: executeRequest,
        generateCurlCommand: generateCurlCommand,
        setConfig: setApiExplorerConfig,
        getBaseUrl: getCurrentBaseUrl
    };
});