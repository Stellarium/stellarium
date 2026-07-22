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


		// ---------------------------------------------------------------------
    // Translation Function
    // ---------------------------------------------------------------------
    
    /** @type {Function} Translation function from remotecontrol module */
    var _tr = rc.tr;
		
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
														desc: `<p>Retrieve a comprehensive snapshot of the current program state. This is the primary polling endpoint for monitoring Stellarium.</p>

																	<div style="margin-top: 10px;">
																			<strong>Returned Data Overview:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><strong>Location</strong>: Name, planet, latitude, longitude, altitude, region, state, and landscape key.</li>
																					<li><strong>Time</strong>: Julian Day (JD), UTC/Local ISO strings, time zone, and current time rate.</li>
																					<li><strong>Selection</strong>: Detailed HTML info string of the currently selected object.</li>
																					<li><strong>View</strong>: Current Field of View (FOV) in degrees.</li>
																					<li><strong>Differential Updates</strong>: Lists of recently changed actions and properties (if requested via IDs).</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Parameters (for differential polling):</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>actionId</code> (optional, number): The last known action change ID.<br>
																							<span style="color: #888;">If provided, the response will include <code>actionChanges</code> containing only actions that changed since this ID, plus a new <code>id</code> to use next time. Saves bandwidth on frequent polls.</span>
																					</li>
																					<li>
																							<code>propId</code> (optional, number): The last known property change ID.<br>
																							<span style="color: #888;">Similar to <code>actionId</code>, but for StelProperties (e.g., MilkyWay.intensity). Returns only the properties that changed since the given ID.</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>If <code>actionId</code> or <code>propId</code> is omitted or set to <code>-1</code>, the server returns the full current state of all actions/properties.</li>
																					<li>The server maintains an internal circular buffer of the last 100 changes for both actions and properties.</li>
																					<li>If the provided ID is older than the buffer (or invalid), the server automatically returns the full state to resynchronize the client.</li>
																			</ul>
																	</div>`,
														parameters: {
																actionId: { required: false, type: 'number', description: 'Last known action change ID' },
																propId: { required: false, type: 'number', description: 'Last known property change ID' }
														}
												},
												time: {
														method: 'POST',
														desc: `<p>Set the simulation time and/or the time flow rate.</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameters:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>time</code> (optional, number): <strong>Julian Day (JD)</strong> value.<br>
																							<span style="color: #d9534f;"><strong>Important:</strong> The server expects a numeric Julian Day (e.g., <code>2460050.5</code>).</span><br>
																							<span style="color: #888;">Sending an ISO 8601 date string (e.g., "2025-01-01T00:00:00") will be rejected by the server (it will log a warning and ignore the parameter).</span>
																					</li>
																					<li>
																							<code>timerate</code> (optional, number): Time flow multiplier.<br>
																							<ul style="margin-top: 3px; padding-left: 20px;">
																									<li><code>1.0</code> → Normal real-time speed (1 second per second).</li>
																									<li><code>0.0</code> → Pause the simulation completely.</li>
																									<li><code>3600.0</code> → 1 hour of simulation per real second.</li>
																									<li><code>86400.0</code> → 1 day of simulation per real second.</li>
																									<li><code>-1.0</code> → Reverse time at normal speed.</li>
																							</ul>
																							<span style="color: #888;">If omitted, the current time rate remains unchanged.</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>You can provide <code>time</code> only, <code>timerate</code> only, or both in the same request.</li>
																					<li>Invalid numeric values (NaN, Infinity) are automatically rejected to prevent crashes.</li>
																					<li>The server will respond with <code>ok</code> if at least one parameter was successfully applied; otherwise, it returns an error.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>time=2460050.5</code> → Set time to January 1, 2025 at noon (JD 2460050.5).</li>
																					<li><code>timerate=0</code> → Pause the simulation.</li>
																					<li><code>timerate=3600</code> → Speed up to 1 hour per second.</li>
																					<li><code>time=2460050.5&timerate=1</code> → Set time to JD 2460050.5 and reset speed to normal.</li>
																			</ul>
																	</div>`,
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
														desc: `<p>Set the Field of View (FOV) to zoom in or out.</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameter:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>fov</code> (required, number): Desired FOV in <strong>degrees</strong>.<br>
																							<ul style="margin-top: 3px; padding-left: 20px;">
																									<li><strong>Minimum</strong>: ~0.1° (extremely zoomed in, e.g., viewing a planet's disk).</li>
																									<li><strong>Maximum</strong>: 235° (fish-eye / wide-angle view).</li>
																									<li><strong>Default</strong>: ~60° (standard view).</li>
																							</ul>
																							<span style="color: #888;">Values outside the min/max range are automatically clamped to the nearest valid limit.</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>The zoom transition is animated smoothly over approximately <strong>0.25 seconds</strong> (250 ms).</li>
																					<li>The actual movement speed is interpolated for a smooth visual experience.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>fov=10</code> → Zoom in to a 10° field of view (good for observing constellations).</li>
																					<li><code>fov=120</code> → Zoom out to a 120° wide view.</li>
																					<li><code>fov=235</code> → Maximum fish-eye zoom out.</li>
																			</ul>
																	</div>`,
														parameters: {
																fov: { required: true, type: 'number', description: 'FOV in degrees (0.1 to 235)' }
														}
												},
												focus: {
														method: 'POST',
														desc: `<p>Point the view towards a specific celestial object, or directly to a J2000 position vector.</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameters (mutually exclusive groups):</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<strong>Object Mode:</strong>
																							<ul style="padding-left: 20px;">
																									<li><code>target</code> (string): Name of the object (supports English and localized names).<br>
																											<span style="color: #888;">Examples: "Mars", "M31", "Sirius", "Polaris".</span>
																									</li>
																							</ul>
																					</li>
																					<li>
																							<strong>Position Mode (J2000):</strong>
																							<ul style="padding-left: 20px;">
																									<li><code>position</code> (vector3): JSON array of 3 doubles <code>[x, y, z]</code> representing the J2000 direction vector.<br>
																											<span style="color: #888;">If provided, <code>target</code> is ignored. This will also clear any selected object.</span>
																									</li>
																							</ul>
																					</li>
																					<li>
																							<strong>Mode (applies only to Object Mode):</strong>
																							<ul style="padding-left: 20px;">
																									<li><code>mode</code> (optional, string): Controls the focusing behavior.<br>
																											<ul>
																													<li><code>center</code> (default): Moves the view to center the object and enables tracking.</li>
																													<li><code>zoom</code>: Moves to center the object, then automatically zooms in for a closer view.</li>
																													<li><code>mark</code>: Selects the object (highlights it) without moving the camera view.</li>
																											</ul>
																									</li>
																							</ul>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>If <code>target</code> is empty and <code>position</code> is not provided, the current selection is cleared (unselect).</li>
																					<li>If <code>target</code> is the name of the current home planet (e.g., "Earth"), the selection is cleared to avoid pointing to the ground.</li>
																					<li>When using <code>position</code>, the view moves directly to the J2000 vector without selecting any object.</li>
																					<li>The movement duration respects the global "auto move duration" setting in Stellarium.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>target=Mars&mode=zoom</code> → Find Mars and zoom in on it.</li>
																					<li><code>target=Sirius&mode=center</code> → Center the view on Sirius (tracking enabled).</li>
																					<li><code>target=Polaris&mode=mark</code> → Select Polaris without moving the view.</li>
																					<li><code>position=[0.933,0.357,0.035]</code> → Point directly to the J2000 coordinates of Polaris.</li>
																			</ul>
																	</div>`,
														parameters: {
																target: { required: true, type: 'string', description: 'Object name,  Examples: "Mars", "M31", "Sirius", "Polaris" ' },
																position: { required: false, type: 'vector3', description: 'JSON array [x,y,z] to focus on (alternative to target)'},
																mode: { required: false, type: 'string', description: 'Focus mode: center, zoom, mark' }
														}
												},
												move: {
														method: 'POST',
														desc: `<p>Continuous view movement in panning style (similar to using a joystick or arrow keys). The view will drift smoothly while speed values are active.</p>

														<div style="margin-top: 10px;">
																<strong>Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>x</code>: Horizontal speed. Range: <strong>-1.0</strong> to <strong>1.0</strong>.<br>
																				<span style="color: #888;">Positive values move the view right (East), negative values move it left (West).</span>
																		</li>
																		<li>
																				<code>y</code>: Vertical speed. Range: <strong>-1.0</strong> to <strong>1.0</strong>.<br>
																				<span style="color: #888;">Positive values move the view up (North), negative values move it down (South).</span>
																		</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Important Behavior (from MainService.cpp):</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<strong>Speed Auto-Scaling:</strong><br>
																				The actual panning speed is dynamically adjusted based on the current Field of View (FOV).<br>
																				When zoomed in (small FOV), the movement slows down for precise control.<br>
																				When zoomed out (large FOV), the movement speeds up to cover more sky quickly.
																		</li>
																		<li>
																				<strong>Automatic Timeout (Safety Feature):</strong><br>
																				If Stellarium does not receive a new <code>x</code> or <code>y</code> update for more than <strong>1 second</strong>, the movement will automatically stop (resets to zero).<br>
																				This prevents the view from continuing to drift endlessly if the connection drops or the client disconnects unexpectedly.
																		</li>
																		<li>
																				<strong>Stopping the View:</strong><br>
																				To immediately stop all motion, send <code>x=0</code> and <code>y=0</code>.
																		</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>x=0.5, y=0</code> → Slowly pan to the right (East).</li>
																		<li><code>x=-1.0, y=0</code> → Pan left (West) at maximum speed.</li>
																		<li><code>x=0, y=0.7</code> → Pan upwards (North) at 70% speed.</li>
																		<li><code>x=0.8, y=-0.5</code> → Pan diagonally down-right (South-East).</li>
																		<li><code>x=0, y=0</code> → Completely stop all movement.</li>
																</ul>
														</div>`,
														parameters: {
																x: { required: true, type: 'number', description: 'Horizontal speed (-1 to 1)' },
																y: { required: true, type: 'number', description: 'Vertical speed (-1 to 1)' }
														}
												},
												view: {
														method: 'GET',
														desc: `<p>Retrieve the current view direction vector in one or more coordinate systems.</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameter:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>coord</code> (optional, string): Specifies which coordinate system to return.<br>
																							<ul style="margin-top: 3px; padding-left: 20px;">
																									<li><code>j2000</code> → Returns the view direction in the J2000 equatorial frame (ICRS).</li>
																									<li><code>jNow</code> → Returns the view direction in the current equatorial frame (of date), including optional refraction correction.</li>
																									<li><code>altAz</code> → Returns the view direction in the horizontal (Altitude/Azimuth) frame.</li>
																									<li><strong>Default (if omitted):</strong> All three are returned.</li>
																							</ul>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Additional (automatic) parameters:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>ref</code> (optional, string): Controls refraction correction for the <code>jNow</code> response (if requested).<br>
																							<ul style="margin-top: 3px; padding-left: 20px;">
																									<li><code>auto</code> (default) → Refraction is applied automatically based on altitude.</li>
																									<li><code>on</code> → Refraction is always applied.</li>
																									<li><code>off</code> → Refraction is never applied.</li>
																							</ul>
																							<span style="color: #888;">Note: This parameter is only effective when <code>coord=jNow</code> is specified or if all three are returned.</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Response Format:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Returns a JSON object containing the requested vector(s) as strings of the form <code>"[x, y, z]"</code>.</li>
																					<li>Example response (all three): <code>{"j2000":"[0.933,0.357,0.035]", "jNow":"[0.933,0.357,0.035]", "altAz":"[0.0,1.0,0.0]"}</code></li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>/api/main/view?coord=j2000</code> → Get J2000 vector only.</li>
																					<li><code>/api/main/view?coord=jNow&ref=on</code> → Get JNow vector with refraction enabled.</li>
																					<li><code>/api/main/view</code> → Get all three vectors (default).</li>
																			</ul>
																	</div>`,
														parameters: {
																coord: { required: false, type: 'string', description: 'Coordinate system: j2000, jNow, altAz' }
														}
												},
												setview: {
														method: 'POST',
														desc: `<p>Set view direction. Supports J2000/JNow/AltAz vectors (JSON array) OR separate azimuth/altitude angles in RADIANS.</p>

														<div style="margin-top: 10px;">
																<strong>Vector Mode (use one of these):</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>j2000</code>: [x,y,z] → J2000 equatorial vector.</li>
																		<li><code>jNow</code>: [x,y,z] → Current epoch equatorial vector (use with "ref" param).</li>
																		<li><code>altAz</code>: [x,y,z] → Horizontal vector (Altitude/Azimuth).</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Angle Mode (use az & alt together or individually):</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>az</code>: Azimuth in RADIANS (0=North, π/2=East, π=South, 3π/2=West).</li>
																		<li><code>alt</code>: Altitude in RADIANS (0=Horizon, π/2=Zenith, -π/2=Nadir).</li>
																		<li><em>If you provide only one angle, the other remains unchanged.</em></li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Additional Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>ref</code>: Refraction mode for JNow (auto|on|off). Default: auto.</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>az=0, alt=0.7854</code> → Look North at 45° altitude</li>
																		<li><code>az=1.5708, alt=0</code> → Look East at horizon</li>
																		<li><code>j2000=[0.933,0.357,0.035]</code> → J2000 vector to Polaris</li>
																</ul>
														</div>`,
														fullPath: 'main/view',
														parameters: {
																j2000: { required: false, type: 'vector3', description: 'J2000 vector [x,y,z] (e.g., [0.933,0.357,0.035])' },
																jNow: { required: false, type: 'vector3', description: 'JNow vector [x,y,z] (e.g., [0.933,0.357,0.035])' },
																altAz: { required: false, type: 'vector3', description: 'Alt/Az vector [x,y,z]' },
																az: { required: false, type: 'number', description: 'Azimuth in RADIANS (0=North, 1.5708=East, 3.14159=South, 4.71239=West)' },
																alt: { required: false, type: 'number', description: 'Altitude in RADIANS (0=Horizon, 1.5708=Zenith)' },
																ref: { required: false, type: 'select', options: ['auto', 'on', 'off'], description: 'Refraction mode (only applies to jNow) mode(auto|on|off)' }
														}
												},                
												plugins: {
														method: 'GET',
														desc: `<p>List all installed Stellarium plugins with their metadata.</p>

																	<div style="margin-top: 10px;">
																			<strong>Response:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Returns a JSON object where each key is the plugin ID, and the value is an object containing:</li>
																					<ul style="margin-top: 3px; padding-left: 20px;">
																							<li><code>loadAtStartup</code> (boolean) → Whether the plugin is loaded automatically on startup.</li>
																							<li><code>loaded</code> (boolean) → Whether the plugin is currently loaded and active.</li>
																							<li><code>info</code> (object) → Detailed plugin metadata:
																									<ul style="margin-top: 3px; padding-left: 20px;">
																											<li><code>authors</code> (string) → Author(s) of the plugin.</li>
																											<li><code>contact</code> (string) → Contact information.</li>
																											<li><code>description</code> (string) → Brief description of the plugin.</li>
																											<li><code>displayedName</code> (string) → User-friendly name.</li>
																											<li><code>startByDefault</code> (boolean) → Whether the plugin is enabled by default.</li>
																											<li><code>version</code> (string) → Version number.</li>
																									</ul>
																							</li>
																					</ul>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>This endpoint does not accept any parameters.</li>
																					<li>The list includes only plugins that are properly registered with Stellarium's module manager.</li>
																					<li>Plugins that are not installed or not detected will not appear in the list.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Example Response (excerpt):</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 12px; overflow-x: auto;">
{
	"RemoteControl": {
		"info": {
			"authors": "Florian Schaukowitsch, Georg Zotti",
			"contact": "https://homepage.univie.ac.at/Georg.Zotti",
			"description": "Provides remote control functionality using a webserver interface. See manual for detailed description.",
			"displayedName": "Remote Control",
			"startByDefault": false,
			"version": "26.1.0"
		},
		"loadAtStartup": true,
		"loaded": true
	},
	"Exoplanets": {
		"info": {
			"authors": "Alexander Wolf",
			"contact": "https://github.com/Stellarium/stellarium",
			"description": "This plugin plots the position of stars with exoplanets. Exoplanets data is derived from the 'Extrasolar Planets Encyclopaedia' at exoplanet.eu",
			"displayedName": "Exoplanets",
			"startByDefault": false,
			"version": "26.1.0"
		},
		"loadAtStartup": true,
		"loaded": true
	}
}
</pre>
																	</div>`,
														parameters: {}
												},
												window: {
														method: 'POST',
														desc: `<p>Change the dimensions of the main Stellarium window.</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameters:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>w</code> (required, integer): Desired window width in <strong>pixels</strong>.<br>
																							<span style="color: #888;">Must be a positive integer (e.g., 1920).</span>
																					</li>
																					<li>
																							<code>h</code> (required, integer): Desired window height in <strong>pixels</strong>.<br>
																							<span style="color: #888;">Must be a positive integer (e.g., 1080).</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>The window is resized immediately (no animation).</li>
																					<li>The internal rendering surface and OpenGL context are automatically adjusted to the new dimensions.</li>
																					<li>If the provided values are non-numeric, zero, or negative, the server will reject the request with an error.</li>
																					<li>On success, the server responds with <code>ok</code>.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>w=1280&h=720</code> → Resize to 720p HD.</li>
																					<li><code>w=1920&h=1080</code> → Resize to Full HD.</li>
																					<li><code>w=800&h=600</code> → Resize to standard 4:3.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px; font-style: italic; color: #000000;">
																			Note: This operation is effective when Stellarium is running in both windowed mode, or full-screen mode.
																	</div>`,
														parameters: {
																w: { required: true, type: 'number', description: 'Width in pixels' },
																h: { required: true, type: 'number', description: 'Height in pixels' }
														}
												}
														}
												},
								objects: {
										name: 'Object Service',
										path: 'objects',
										operations: {
												find: {
														method: 'GET',
														desc: `<p>Search for celestial objects in Stellarium catalogs.</p>
														<div style="margin-top: 10px;">
																<strong>Parameter:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>str</code> (required, string): Search term (case-insensitive).<br>
																		<span style="color: #888;">Supports Greek letters (e.g., "α" for "Alpha").</span></li>
																</ul>
														</div>
														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Searches all object catalogs (stars, planets, deep-sky objects, etc.).</li>
																		<li>Search is case-insensitive and supports partial matches.</li>
																		<li>If the search term contains Greek letters, they are automatically substituted.</li>
																		<li>Results are sorted by name length (shorter names first) for better autocomplete.</li>
																		<li>Duplicates are automatically removed from the result list.</li>
																</ul>
														</div>
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>str=Mars</code> → Returns ["Mars"]</li>
																		<li><code>str=M31</code> → Returns ["M31", "NGC 224", "Andromeda Galaxy"]</li>
																		<li><code>str=α</code> → Returns ["Alpha Centauri", "Alpha Andromedae", ...]</li>
																</ul>
														</div>`,
														parameters: {
																str: { required: true, type: 'string', description: 'Search string' }
														}
												},
												info: {
														method: 'GET',
														desc: `<p>Retrieve detailed information about a specific celestial object.</p>
														<div style="margin-top: 10px;">
																<strong>Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>name</code> (optional): Object name. If omitted, uses currently selected object.</li>
																		<li><code>format</code> (optional): Output format - <code>html</code> (default), <code>json</code>, or <code>map</code>.</li>
																</ul>
														</div>
														<div style="margin-top: 10px;">
																<strong>JSON Response Includes:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Identification: name, type, constellation, spectral class</li>
																		<li>Magnitude & Distance: vmag, absolute-mag, distance, parallax</li>
																		<li>Coordinates: RA/Dec (J2000 & JNow), Galactic, Ecliptic</li>
																		<li>Current Position: Altitude, Azimuth, Above Horizon, Air Mass</li>
																		<li>Rise/Set/Transit times</li>
																		<li>Star properties (if applicable): bV, period, variable type</li>
																		<li>Double star data: separation, position angle</li>
																		<li>Solar System: illumination, phase, velocity, moons</li>
																		<li>Object size: angular diameter</li>
																</ul>
														</div>`,
														parameters: {
																name: { required: false, type: 'string', description: 'Object name (empty for selected)' },
																format: { required: false, type: 'select', options: ['html', 'json', 'map'], description: 'Output format: html (default), json, or map' }
														}
												},
												listobjecttypes: {
														method: 'GET',
														desc: `<p>Get a list of all available object types (catalogs) in Stellarium.</p>
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>key</code>: Internal type identifier (e.g., "StarMgr", "SolarSystem").</li>
																		<li><code>name</code>: English name.</li>
																		<li><code>name_i18n</code>: Localized name.</li>
																</ul>
														</div>
														<div style="margin-top: 10px;">
																<strong>Common Types:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>StarMgr → Stars (Hipparcos catalog)</li>
																		<li>SolarSystem → Planets, moons, asteroids, comets</li>
																		<li>NebulaMgr → Nebulae, galaxies, clusters (NGC/IC/Messier)</li>
																		<li>AsterismMgr → Asterisms (star patterns)</li>
																		<li>ConstellationMgr → Constellation boundaries</li>
																</ul>
														</div>`,
														parameters: {}
												},
								listobjectsbytype: {
										method: 'GET',
										desc: `<p>Get a list of all objects belonging to a specific type/catalog.</p>
										
										<div style="margin-top: 10px; background: rgba(0,0,0,0.15); padding: 12px; border-radius: 4px;">
												<strong style="color: #66D9EF;">Quick Tip:</strong>
												<span style="color: #B4B7B0;">Click the <strong>"Browse Types"</strong> button below to see all available types with their localized names.</span>
										</div>
										
										<div style="margin-top: 10px;">
												<strong>Parameters:</strong>
												<ul style="margin-top: 5px; padding-left: 20px;">
														<li>
																<code>type</code> (required): Object type key from <code>listobjecttypes</code>.<br>
																<span style="color: #888;">Use the "Browse Types" button to select from available types without guessing.</span>
														</li>
														<li style="margin-top: 5px;">
																<code>english</code> (optional): Name language preference.<br>
																<ul style="padding-left: 20px; margin-top: 3px;">
																		<li><code>0</code> (default): Returns localized names (translated to Stellarium's current language).</li>
																		<li><code>1</code>: Returns English names only.</li>
																</ul>
														</li>
												</ul>
										</div>
										
										<div style="margin-top: 10px;">
												<strong>Available Type Categories (from listobjecttypes):</strong>
												<ul style="margin-top: 5px; padding-left: 20px;">
														<li><strong>StarMgr</strong> → Stars (Hipparcos catalog)</li>
														<ul style="padding-left: 20px; margin-top: 3px;">
																<li><code>StarMgr</code> → All stars</li>
																<li><code>StarMgr:0</code> → Interesting double stars</li>
																<li><code>StarMgr:1</code> → Interesting variable stars</li>
																<li><code>StarMgr:2</code> → Bright double stars</li>
																<li><code>StarMgr:3</code> → Bright variable stars</li>
																<li><code>StarMgr:4</code> → Bright stars with high proper motion</li>
																<li><code>StarMgr:5</code> → Algol-type eclipsing systems</li>
																<li><code>StarMgr:6</code> → Classical cepheids</li>
																<li><code>StarMgr:7</code> → Bright carbon stars</li>
																<li><code>StarMgr:8</code> → Bright barium stars</li>
														</ul>
												</ul>
												<ul style="margin-top: 8px; padding-left: 20px;">
														<li><strong>SolarSystem</strong> → Solar System objects</li>
														<ul style="padding-left: 20px; margin-top: 3px;">
																<li><code>SolarSystem</code> → All solar system objects</li>
																<li><code>SolarSystem:planet</code> → Planets</li>
																<li><code>SolarSystem:moon</code> → Moons</li>
																<li><code>SolarSystem:asteroid</code> → Asteroids</li>
																<li><code>SolarSystem:comet</code> → Comets</li>
																<li><code>SolarSystem:dwarf planet</code> → Dwarf planets</li>
																<li><code>SolarSystem:plutino</code> → Plutinos</li>
																<li><code>SolarSystem:cubewano</code> → Cubewanos</li>
																<li><code>SolarSystem:scattered disc object</code> → Scattered disc objects</li>
																<li><code>SolarSystem:sednoid</code> → Sednoids</li>
																<li><code>SolarSystem:artificial</code> → Artificial objects</li>
														</ul>
												</ul>
												<ul style="margin-top: 8px; padding-left: 20px;">
														<li><strong>NebulaMgr</strong> → Deep-sky objects (DSO)</li>
														<ul style="padding-left: 20px; margin-top: 3px;">
																<li><code>NebulaMgr</code> → All deep-sky objects</li>
																<li><code>NebulaMgr:0</code> → Bright galaxies</li>
																<li><code>NebulaMgr:1</code> → Active galaxies</li>
																<li><code>NebulaMgr:2</code> → Radio galaxies</li>
																<li><code>NebulaMgr:3</code> → Interacting galaxies</li>
																<li><code>NebulaMgr:4</code> → Bright quasars</li>
																<li><code>NebulaMgr:5</code> → Star clusters</li>
																<li><code>NebulaMgr:6</code> → Open star clusters</li>
																<li><code>NebulaMgr:7</code> → Globular star clusters</li>
																<li><code>NebulaMgr:8</code> → Stellar associations</li>
																<li><code>NebulaMgr:10</code> → Nebulae</li>
																<li><code>NebulaMgr:11</code> → Planetary nebulae</li>
																<li><code>NebulaMgr:12</code> → Dark nebulae</li>
																<li><code>NebulaMgr:13</code> → Reflection nebulae</li>
																<li><code>NebulaMgr:14</code> → Bipolar nebulae</li>
																<li><code>NebulaMgr:15</code> → Emission nebulae</li>
																<li><code>NebulaMgr:16</code> → Clusters associated with nebulosity</li>
																<li><code>NebulaMgr:17</code> → HII regions</li>
																<li><code>NebulaMgr:18</code> → Supernova remnants</li>
																<li><code>NebulaMgr:19</code> → Interstellar matter</li>
																<li><code>NebulaMgr:20</code> → Emission objects</li>
																<li><code>NebulaMgr:21</code> → BL Lac objects</li>
																<li><code>NebulaMgr:22</code> → Blazars</li>
																<li><code>NebulaMgr:23</code> → Molecular Clouds</li>
																<li><code>NebulaMgr:24</code> → Young Stellar Objects</li>
																<li><code>NebulaMgr:25</code> → Possible Quasars</li>
																<li><code>NebulaMgr:26</code> → Possible Planetary Nebulae</li>
																<li><code>NebulaMgr:27</code> → Protoplanetary Nebulae</li>
																<li><code>NebulaMgr:29</code> → Symbiotic stars</li>
																<li><code>NebulaMgr:30</code> → Emission-line stars</li>
																<li><code>NebulaMgr:32</code> → Supernova remnant candidates</li>
																<li><code>NebulaMgr:33</code> → Clusters of galaxies</li>
																<li><code>NebulaMgr:35</code> → Regions of the sky</li>
																<li><code>NebulaMgr:100</code> → Messier Catalogue</li>
																<li><code>NebulaMgr:101</code> → Caldwell Catalogue</li>
																<li><code>NebulaMgr:102</code> → Barnard Catalogue</li>
																<li><code>NebulaMgr:103</code> → Sharpless Catalogue</li>
																<li><code>NebulaMgr:104</code> → van den Bergh Catalogue</li>
																<li><code>NebulaMgr:105</code> → RCW Catalogue</li>
																<li><code>NebulaMgr:106</code> → Collinder Catalogue</li>
																<li><code>NebulaMgr:107</code> → Melotte Catalogue</li>
																<li><code>NebulaMgr:108</code> → New General Catalogue (NGC)</li>
																<li><code>NebulaMgr:109</code> → Index Catalogue (IC)</li>
																<li><code>NebulaMgr:110</code> → LBN (Lynds' Bright Nebulae)</li>
																<li><code>NebulaMgr:111</code> → LDN (Lynds' Dark Nebulae)</li>
																<li><code>NebulaMgr:114</code> → Cederblad Catalog</li>
																<li><code>NebulaMgr:115</code> → Arp (Peculiar Galaxies)</li>
																<li><code>NebulaMgr:116</code> → VV (Interacting Galaxies)</li>
																<li><code>NebulaMgr:117</code> → PK (Galactic Planetary Nebulae)</li>
																<li><code>NebulaMgr:118</code> → PN G (Strasbourg-ESO Planetary Nebulae)</li>
																<li><code>NebulaMgr:119</code> → SNR G (Supernova Remnants)</li>
																<li><code>NebulaMgr:120</code> → Abell (Rich Clusters of Galaxies)</li>
																<li><code>NebulaMgr:121</code> → HCG (Hickson Compact Group)</li>
																<li><code>NebulaMgr:122</code> → ESO (ESO/Uppsala Survey)</li>
																<li><code>NebulaMgr:123</code> → VdBH (Southern stars in nebulosity)</li>
																<li><code>NebulaMgr:124</code> → DWB (HII Regions)</li>
																<li><code>NebulaMgr:125</code> → Trumpler Catalogue</li>
																<li><code>NebulaMgr:126</code> → Stock Catalogue</li>
																<li><code>NebulaMgr:127</code> → Ruprecht Catalogue</li>
																<li><code>NebulaMgr:128</code> → vdB-Ha (van den Bergh-Hagen)</li>
																<li><code>NebulaMgr:150</code> → Dwarf galaxies</li>
																<li><code>NebulaMgr:151</code> → Herschel 400 Catalogue</li>
																<li><code>NebulaMgr:152</code> → Bennett Catalogue</li>
																<li><code>NebulaMgr:153</code> → Dunlop Catalogue</li>
														</ul>
												</ul>
												<ul style="margin-top: 8px; padding-left: 20px;">
														<li><strong>Other Types:</strong></li>
														<ul style="padding-left: 20px; margin-top: 3px;">
																<li><code>AsterismMgr</code> → Asterisms (star patterns)</li>
																<li><code>ConstellationMgr</code> → Constellations</li>
																<li><code>MeteorShowers</code> → Meteor Showers</li>
																<li><code>Novae</code> → Bright Novae</li>
																<li><code>Supernovae</code> → Historical Supernovae</li>
														</ul>
												</ul>
										</div>
										
										<div style="margin-top: 10px;">
												<strong>Examples:</strong>
												<ul style="margin-top: 5px; padding-left: 20px;">
														<li><code>type=StarMgr&english=1</code> → List all stars with English names.</li>
														<li><code>type=SolarSystem:planet</code> → List all planets with localized names.</li>
														<li><code>type=NebulaMgr:108&english=1</code> → List all NGC objects with English names.</li>
														<li><code>type=NebulaMgr:6&english=0</code> → List all open star clusters with localized names.</li>
														<li><code>type=ConstellationMgr</code> → List all constellations.</li>
												</ul>
										</div>
										
										<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
												<strong style="color: #E6DB74;">Pro Tip:</strong>
												<span style="color: #B4B7B0;">The <code>key</code> format is <strong>"ModuleName"</strong> for the entire catalog, or <strong>"ModuleName:number"</strong> for specific sub-catalogs (e.g., <code>NebulaMgr:108</code> for NGC). Use the <strong>"Browse Types"</strong> button to explore all available options.</span>
										</div>`,
										parameters: {
												type: { 
														required: true, 
														type: 'string', 
														description: 'Object type key (e.g., StarMgr, SolarSystem:planet, NebulaMgr:108)',
														suggest: 'objecttypes'
												},
												english: { 
														required: false, 
														type: 'select', 
														options: ['', '0', '1'], 
														description: '0 = localized names (default), 1 = English names' 
												}
										}
								}
										}
								},
								location: {
										name: 'Location Service',
										path: 'location',
													operations: {
														list: {
														method: 'GET',
														desc: 'Get a list of all location names in the database',
														parameters: {}
												},
											planetimage: {
														method: 'GET',
														desc: `<p>Retrieve the texture image (map) of a specific planet or celestial body.</p>

														<div style="margin-top: 10px;">
																<strong>Parameter:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>planet</code> (required, string): Name of the planet.<br>
																				<span style="color: #888;">Use the names returned by <code>planetlist</code> (e.g., "Earth", "Mars", "Moon").</span>
																		</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Returns the actual image file (PNG/JPG) of the planet's texture map, not JSON.</li>
																		<li>The image is displayed directly in the response area with a preview.</li>
																		<li>If the planet name is invalid or the texture file is missing, returns HTTP 404.</li>
																		<li>Supported planets include all major planets and moons with texture maps.</li>
																</ul>
														</div>

														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>planet=Earth</code> → Returns Earth's surface texture map.</li>
																		<li><code>planet=Mars</code> → Returns Mars surface texture.</li>
																		<li><code>planet=Moon</code> → Returns lunar surface texture.</li>
																</ul>
														</div>

														<div style="margin-top: 10px; font-style: italic; color: #888;">
																Note: This endpoint returns binary image data, not JSON. The response is displayed as an image preview.
														</div>`,
														parameters: {
																planet: {	required: true, type: 'string', description: 'Planet name (Earth, Mars, Moon ...)'}
														},
														responseType: 'image'
												},
												setlocationfields: {
														method: 'POST',
														desc: `<p>Set the observer's location using either a predefined location ID or custom coordinates.</p>

																	<div style="margin-top: 10px;">
																			<strong>Two Methods (mutually exclusive):</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<strong>Method 1: Use Location ID (simplest)</strong>
																							<ul style="padding-left: 20px; margin-top: 3px;">
																									<li><code>id</code> (string): Predefined location name from Stellarium's database.</li>
																									<li style="color: #888; font-style: italic;">If <code>id</code> is provided, all other parameters are ignored.</li>
																							</ul>
																					</li>
																					<li style="margin-top: 8px;">
																							<strong>Method 2: Manual Coordinates</strong>
																							<ul style="padding-left: 20px; margin-top: 3px;">
																									<li><code>latitude</code> (number, optional): Latitude in degrees (positive = North, negative = South).</li>
																									<li><code>longitude</code> (number, optional): Longitude in degrees (positive = East, negative = West).</li>
																									<li><code>altitude</code> (number, optional): Altitude above sea level in meters.</li>
																									<li><code>planet</code> (string, optional): Planet name (e.g., "Earth", "Mars", "Moon").</li>
																									<li><code>name</code> (string, optional): Custom name for the location. If omitted, a name is auto-generated from coordinates.</li>
																									<li><code>region</code> (string, optional): Geographic region (e.g., "South Africa", "Western Europe").</li>
																							</ul>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>If you provide <code>id</code>, the location is loaded directly from the database with all its stored values.</li>
																					<li>If you don't provide <code>id</code>, the current location is used as a base, and only the fields you specify are updated.</li>
																					<li>Supports both comma (",") and dot (".") as decimal separators for latitude/longitude values.</li>
																					<li>The <code>planet</code> name must match an existing planet in the Solar System (case-insensitive).</li>
																					<li>If <code>planet</code> is changed, the location is moved to that planet's surface (or appropriate reference frame).</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>id=Paris, Western Europe</code> → Move to Paris using its database entry.</li>
																					<li><code>latitude=48.8566&longitude=2.3522&name=Paris</code> → Move to Paris (France) with custom name.</li>
																					<li><code>latitude=30.0444&longitude=31.2357&altitude=100&planet=Earth</code> → Set exact coordinates in Cairo with 100m altitude.</li>
																					<li><code>latitude=0&longitude=0&planet=Mars</code> → Move to the Martian equator (0°N, 0°E).</li>
																			</ul>
																	</div>`,
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
														desc: `<p>Get a list of all planets that can be used as observer locations.</p>

																	<div style="margin-top: 10px;">
																			<strong>Returned Data:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>An array of planet objects, each containing:</li>
																					<ul style="padding-left: 20px;">
																							<li><code>name</code>: English name of the planet (e.g., "Earth", "Mars").</li>
																							<li><code>name_i18n</code>: Localized name (translated to the current Stellarium language).</li>
																					</ul>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Usage Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Use the <code>name</code> field as the <code>planet</code> parameter in <code>setlocationfields</code>.</li>
																					<li>The list includes all planets and major moons that have a defined surface reference frame.</li>
																					<li>Planet names are case-sensitive in the API (use exactly as returned).</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Example Response:</strong>
																			<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 12px; margin: 5px 0;">
				[
						{"name": "Mercury", "name_i18n": "Mercure"},
						{"name": "Venus", "name_i18n": "Vénus"},
						{"name": "Earth", "name_i18n": "Terre"},
						{"name": "Mars", "name_i18n": "Mars"},
						{"name": "Moon", "name_i18n": "Lune"}
				]</pre>
																	</div>`,
														parameters: {}
												},
								regionlist: {
														method: 'GET',
														desc: `<p>Get a list of all available geographic regions used for location categorization.</p>

																	<div style="margin-top: 10px;">
																			<strong>Returned Data:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>An array of region objects, each containing:</li>
																					<ul style="padding-left: 20px;">
																							<li><code>name</code>: Internal region name (e.g., "North Africa", "Eastern Europe").</li>
																							<li><code>name_i18n</code>: Localized region name (translated to the current Stellarium language).</li>
																					</ul>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Usage Notes:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Region names are used to categorize locations in the Stellarium database.</li>
																					<li>Use the <code>name</code> field as the <code>region</code> parameter in <code>setlocationfields</code>.</li>
																					<li>Region names are not required for setting a location, but can help with organization.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Example Response:</strong>
				<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 12px; margin: 5px 0;">
				[
						{"name": "Northern Africa", "name_i18n": "Northern Africa"},
						{"name": "Northern America", "name_i18n": "Northern America"},
						{"name": "Western Asia", "name_i18n": "Western Asia"},
						{"name": "Eastern Europe", "name_i18n": "Eastern Europe"}
				]</pre>
																	</div>`,
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
														desc: `<p>Search for locations by name (supports wildcard patterns).</p>

																	<div style="margin-top: 10px;">
																			<strong>Parameter:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>term</code> (required, string): Search term to match against location names.<br>
																							<span style="color: #888;">Supports wildcards: use <code>*</code> as a placeholder for any characters.</span>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Searches the full location name, including city, and region information.</li>
																					<li>Wildcard mode is enabled by default: "new*" matches "New York" and "New Delhi".</li>
																					<li>Search is case-insensitive ("paris*" matches "Paris, Western Europe" + and other locations containing "Paris".)</li>
																					<li>Returns an array of matching location names (strings).</li>
																					<li>Results are not sorted by relevance; the list is returned in the order stored in the database.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>term=Paris*</code> → Returns "Paris, Western Europe" and other locations containing "Paris".</li>
																					<li><code>term=New*</code> → Returns all locations starting with "New" (New York, New Delhi, etc.).</li>
																					<li><code>term=*polis*</code> → Returns all locations ending with "polis" (Indianapolis, Minneapolis, etc.).</li>
																					<li><code>term=west*asia</code> → Returns locations matching "west" then "asia" (e.g., "Western Asia").</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px; font-style: italic; color: #888;">
																			Note: This endpoint is designed for autocomplete functionality and is optimized for quick searches.
																	</div>`,
														parameters: {
																term: { required: true, type: 'string', description: 'Search term' }
														}
												},
												nearby: {
														method: 'GET',
														desc: `<p>Find locations near a specific point on a planet's surface.</p>

																	<div style="margin-top: 10px;">
																			<strong>Required Parameters:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>planet</code> (string): Planet name (e.g., "Earth", "Mars").</li>
																					<li><code>latitude</code> (number): Latitude in degrees (positive = North, negative = South).</li>
																					<li><code>longitude</code> (number): Longitude in degrees (positive = East, negative = West).</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Optional Parameter:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>
																							<code>radius</code> (number, optional): Search radius in degrees.<br>
																							<span style="color: #888;">If not specified, a default radius is used.</span>
																							<ul style="padding-left: 20px; margin-top: 3px;">
																									<li>On Earth, 1° ≈ 111 km (great circle distance).</li>
																									<li>Larger radius returns more results.</li>
																							</ul>
																					</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Behavior:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li>Searches the location database for locations within the specified radius.</li>
																					<li>Returns an array of matching location names (strings).</li>
																					<li>Results are sorted by distance from the center point (nearest first).</li>
																					<li>The search is performed on a copy of the location database to avoid blocking the main Stellarium thread.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px;">
																			<strong>Examples:</strong>
																			<ul style="margin-top: 5px; padding-left: 20px;">
																					<li><code>planet=Earth&latitude=48.8566&longitude=2.3522&radius=5</code> → Find locations within 5° of Paris, France.</li>
																					<li><code>planet=Earth&latitude=30.0444&longitude=31.2357&radius=10</code> → Find locations within 10° of Cairo, Egypt.</li>
																					<li><code>planet=Mars&latitude=0&longitude=0&radius=20</code> → Find locations near the Martian equator at 0° longitude.</li>
																			</ul>
																	</div>

																	<div style="margin-top: 10px; font-style: italic; color: #888;">
																			Note: The actual location database may have limited coverage on planets other than Earth. Use <code>planetlist</code> to see available planets.
																	</div>`,
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
														desc: `<p>Get a list of all installed script files available in Stellarium.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">Note:</strong>
																<span style="color: #B4B7B0;">This endpoint returns an array of script filenames (e.g., <code>["double_stars.ssc", "triple_moon.ssc", "solar_system.ssc"]</code>).</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>An array of script IDs (filenames) available in Stellarium's script directories.</li>
																		<li>Scripts are loaded from the standard script paths (user and system directories).</li>
																		<li>Each ID can be used with the <code>info</code> and <code>run</code> operations.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Example Response:</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0;">
[
		"constellations_tour.ssc",
		"double_stars.ssc",
		"planet_orbits.ssc",
		"solar_system.ssc",
		"triple_moon.ssc"
]</pre>
														</div>`,
														parameters: {}
												},								
												info: {
														method: 'GET',
														desc: `<p>Get detailed information about a specific script.</p>
														
														<div style="margin-top: 10px;">
																<strong>Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>id</code> (required, string): Script identifier (filename).<br>
																				<span style="color: #888;">Use the <code>list</code> operation to see all available script IDs.</span>
																		</li>
																		<li style="margin-top: 5px;">
																				<code>html</code> (optional): If present, returns HTML description instead of JSON.
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data (JSON format):</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id</code>: Script filename.</li>
																		<li><code>name</code>: Script name (English).</li>
																		<li><code>name_localized</code>: Localized script name.</li>
																		<li><code>description</code>: Script description (English).</li>
																		<li><code>description_localized</code>: Localized script description.</li>
																		<li><code>author</code>: Script author name.</li>
																		<li><code>license</code>: Script license information.</li>
																		<li><code>version</code>: Script version number.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Tip:</strong>
																<span style="color: #B4B7B0;">If the script ID is invalid or not found, all fields except <code>id</code> will return empty strings.</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id=double_stars.ssc</code> → Returns JSON metadata about the double_stars script.</li>
																		<li><code>id=constellations_tour.ssc&html</code> → Returns HTML description of the script.</li>
																</ul>
														</div>`,
														parameters: {
																id: { 
																		required: true, 
																		type: 'string', 
																		description: 'Script identifier (filename)', 
																		suggest: 'scripts' 
																},
																html: { 
																		required: false, 
																		type: 'checkbox', 
																		description: 'Enable to return HTML description instead of JSON' 
																}
														}
												},
												
												run: {
														method: 'POST',
														desc: `<p>Execute an installed script file.</p>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Important:</strong>
																<span style="color: #B4B7B0;">Only one script can run at a time. If a script is already running, this operation will return an error.</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Parameter:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>id</code> (required, string): Script identifier (filename).<br>
																				<span style="color: #888;">Use the <code>list</code> operation to see all available script IDs.</span>
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>The script is prepared and validated before execution.</li>
																		<li>If the script ID is invalid, returns "error: could not prepare script, wrong id?"</li>
																		<li>If a script is already running, returns "error: a script is already running".</li>
																		<li>Script execution happens asynchronously (non-blocking).</li>
																		<li>Use the <code>status</code> operation to check if a script is running.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id=double_stars.ssc</code> → Runs the double_stars script.</li>
																		<li><code>id=constellations_tour.ssc</code> → Starts the constellations tour.</li>
																</ul>
														</div>`,
														parameters: {
																id: { 
																		required: true, 
																		type: 'string', 
																		description: 'Script identifier (filename)', 
																		suggest: 'scripts' 
																}
														}
												},
												
												direct: {
														method: 'POST',
														desc: `<p>Execute script code directly without using a script file.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">Use Case:</strong>
																<span style="color: #B4B7B0;">This is useful for testing small snippets of code or executing temporary commands without creating a permanent script file.</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>code</code> (required, string): The Stellarium Script code to execute.<br>
																				<span style="color: #888;">Supports all Stellarium Script commands (core.*, StelMovementMgr.*, etc.).</span>
																		</li>
																		<li style="margin-top: 5px;">
																				<code>useIncludes</code> (optional, boolean): Enable preprocessing with includes directory.<br>
																				<ul style="padding-left: 20px; margin-top: 3px;">
																						<li><code>0</code> (default): No preprocessing.</li>
																						<li><code>1</code> or <code>true</code>: Use includes directory for preprocessing.</li>
																				</ul>
																				<span style="color: #888;">When enabled, the code will be processed through Stellarium's include system, allowing the use of shared script libraries.</span>
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Important:</strong>
																<span style="color: #B4B7B0;">Only one script can run at a time. If a script is already running, this operation will return an error.</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>code=core.setDate("2025-01-01T00:00:00", "utc")</code> → Set time to January 1, 2025.</li>
																		<li><code>code=core.moveToObject("Mars", 2)</code> → Move to Mars.</li>
																		<li><code>code=StelMovementMgr.zoomTo(30, 3)&useIncludes=1</code> → Zoom to 30° with includes support.</li>
																</ul>
														</div>`,
														parameters: {
																code: { 
																		required: true, 
																		type: 'textarea', 
																		description: 'Stellarium Script code to execute' 
																},
																useIncludes: { 
																		required: false, 
																		type: 'checkbox', 
																		description: 'Enable includes directory for preprocessing' 
																}
														}
												},
												
												status: {
														method: 'GET',
														desc: `<p>Get the current script execution status.</p>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>scriptIsRunning</code> (boolean): Whether a script is currently executing.</li>
																		<li><code>runningScriptId</code> (string): The ID of the currently running script, or an empty string if none.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Example Response:</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0;">
{
		"scriptIsRunning": true,
		"runningScriptId": "double_stars.ssc"
}</pre>
														</div>`,
														parameters: {}
												},        
												stop: {
														method: 'POST',
														desc: `<p>Stop the currently running script.</p>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Note:</strong>
																<span style="color: #B4B7B0;">This operation immediately stops the script, even if it's in the middle of a loop or long-running operation.</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>If no script is running, the operation still returns "ok".</li>
																		<li>Use the <code>status</code> operation to verify that the script has stopped.</li>
																		<li>Stopping a script is asynchronous - the script will be terminated at the next opportunity.</li>
																</ul>
														</div>`,
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
														desc: `<p>Get a categorized list of all registered StelActions with their current states.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">What are StelActions?</strong>
																<span style="color: #B4B7B0;">
																		StelActions are the fundamental building blocks of Stellarium's interactive controls. Every toggle, button, 
																		and menu item in Stellarium is powered by a StelAction. They represent both <strong>toggleable</strong> 
																		(on/off) and <strong>trigger</strong> (one-shot) actions.
																</span>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Dynamic Categories:</strong>
																<span style="color: #B4B7B0;">
																		The categories listed below are <strong>dynamically generated</strong> from Stellarium's internal action groups. 
																		Categories appear or disappear based on which plugins are loaded and which modules are active.
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data Structure:</strong>
																<p style="color: #B4B7B0; font-size: 11px; margin: 5px 0;">
																		Returns a JSON object where each key is a <strong>category name</strong> (translated to the current Stellarium language), 
																		and each value is an array of actions belonging to that category.
																</p>
								<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0; white-space: pre-wrap;">
{
		"Display Options": [
				{"id": "actionShow_Stars", "isCheckable": true, "isChecked": true, "text": "Stars"},
				{"id": "actionShow_Atmosphere", "isCheckable": true, "isChecked": false, "text": "Atmosphere"}
		],
		"Movement and Selection": [
				{"id": "actionGo_Home_Global", "isCheckable": false, "text": "Go to home"},
				{"id": "actionGoto_Selected_Object", "isCheckable": true, "isChecked": false, "text": "Center on selected object"}
		],
		"Date and Time": [
				{"id": "actionIncrease_Time_Speed", "isCheckable": false, "text": "Increase time speed"},
				{"id": "actionSet_Real_Time_Speed", "isCheckable": false, "text": "Set normal time rate"}
		],
		"Oculars": [
				{"id": "actionShow_Oculars", "isCheckable": true, "isChecked": false, "text": "Ocular view"},
				{"id": "actionShow_Oculars_dialog", "isCheckable": true, "isChecked": false, "text": "Show settings dialog"}
		],
		"Scripts": [
				{"id": "actionScript/double_stars.ssc", "isCheckable": false, "text": "20 Fun Naked-Eye Double Stars"}
		],
		"Telescope Control": [
				{"id": "actionMove_Telescope_To_Selection_1", "isCheckable": false, "text": "Move telescope #1 to selected object"}
		],
		"ArchaeoLines": [
				{"id": "actionShow_ArchaeoLines", "isCheckable": true, "isChecked": false, "text": "ArchaeoLines"}
		]
}</pre>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Common Categories (varies by installation):</strong>
																<ul style="margin-top: 5px; padding-left: 20px; columns: 2; column-gap: 20px;">
																		<li><strong>Display Options</strong> — Toggle visibility of graphical elements</li>
																		<li><strong>Movement and Selection</strong> — Navigation and selection actions</li>
																		<li><strong>Date and Time</strong> — Time speed and jump controls</li>
																		<li><strong>Windows</strong> — Open/close dialog windows</li>
																		<li><strong>Scripts</strong> — Execute installed scripts</li>
																		<li><strong>Oculars</strong> — Telescope/eyepiece simulation</li>
																		<li><strong>Telescope Control</strong> — Telescope movement and sync</li>
																		<li><strong>Field of View</strong> — Zoom presets</li>
																		<li><strong>Satellites</strong> — Satellite visibility groups</li>
																		<li><strong>Exoplanets</strong> — Exoplanet display options</li>
																		<li><strong>ArchaeoLines</strong> — Archaeoastronomy lines</li>
																		<li><strong>Navigational Stars</strong> — Navigation star markers</li>
																		<li><strong>Calendars</strong> — Calendar display</li>
																		<li><strong>Equation of Time</strong> — EoT display</li>
																		<li><strong>Observability</strong> — Observability analysis</li>
																		<li><strong>Miscellaneous</strong> — Various utility actions</li>
																		<li><strong>Meteor Showers</strong> — Meteor shower display</li>
																		<li><strong>Remote Control</strong> — Remote control settings</li>
																		<li><strong>Online Queries</strong> — Online database lookups</li>
																		<li><strong>Angle Measure</strong> — Angle measurement tools</li>
																		<li><strong>Pulsars</strong> — Pulsar display</li>
																		<li><strong>Bright Novae</strong> — Nova display</li>
																		<li><strong>Historical Supernovae</strong> — Supernova display</li>
																		<li><strong>Scenery3d</strong> — 3D scenery controls</li>
																		<li><strong>Mosaic Camera</strong> — Mosaic camera controls</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Action Properties:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id</code> (string): Unique action identifier. Use this with the <code>do</code> operation.</li>
																		<li><code>isCheckable</code> (boolean): Whether the action can be toggled on/off.</li>
																		<li><code>isChecked</code> (boolean, only if checkable): Current state (true = on/checked).</li>
																		<li><code>text</code> (string): Human-readable action name (localized).</li>
																</ul>
																
														</div>`,
														parameters: {}
												},
												
												do: {
														method: 'POST',
														desc: `<p>Execute or toggle a specific StelAction.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">How It Works:</strong>
																<span style="color: #B4B7B0;">
																		This endpoint triggers a StelAction. If the action is <strong>checkable</strong> (toggleable), it will 
																		switch its state (on → off or off → on). If it's a <strong>trigger</strong> action, it executes once 
																		(e.g., "Go Home", "Quit", "Save Screenshot").
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Parameter:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>id</code> (required, string): Action identifier.<br>
																				<span style="color: #888;">Use the <code>list</code> operation or the <strong>"Browse Actions"</strong> button to see all available action IDs.</span>
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Return Values:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><strong>For checkable actions:</strong> Returns <code>"true"</code> or <code>"false"</code> (the new state).</li>
																		<li><strong>For trigger actions:</strong> Returns <code>"ok"</code> on success.</li>
																		<li><strong>Invalid ID:</strong> Returns <code>"error: invalid id parameter"</code>.</li>
																		<li><strong>Closing actions:</strong> Returns <code>"error: Stellarium or remote control closing!"</code> 
																				(for <code>actionQuit_Global</code> or <code>actionShow_Remote_Control</code>).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Important Notes:</strong>
																<ul style="margin-top: 5px; padding-left: 20px; margin-bottom: 0;">
																		<li>Some actions (like <code>actionQuit_Global</code> or script-related actions) are executed asynchronously 
																				to prevent deadlocks.</li>
																		<li>Triggering <code>actionQuit_Global</code> will close Stellarium entirely.</li>
																		<li>Triggering <code>actionShow_Remote_Control</code> will close the Remote Control plugin.</li>
																		<li>For checkable actions, the response reflects the <strong>new</strong> state after toggling.</li>
																		<li>Script actions (starting with <code>actionScript/</code>) execute installed scripts.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id=actionShow_Stars</code> → Toggle stars visibility.</li>
																		<li><code>id=actionShow_Equatorial_Grid</code> → Toggle equatorial grid.</li>
																		<li><code>id=actionGo_Home_Global</code> → Move to home view (trigger).</li>
																		<li><code>id=actionSwitch_Equatorial_Mount</code> → Switch between equatorial/azimuthal mount.</li>
																		<li><code>id=actionShow_Night_Mode</code> → Toggle night mode.</li>
																		<li><code>id=actionSet_Full_Screen_Global</code> → Toggle full screen.</li>
																		<li><code>id=actionScript/double_stars.ssc</code> → Run the double stars script.</li>
																		<li><code>id=actionShow_Oculars</code> → Toggle ocular view.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Pro Tip:</strong>
																<span style="color: #B4B7B0;">
																		This is one of the most powerful API endpoints! Almost everything you can click in the Stellarium GUI 
																		is available as a StelAction. This allows you to automate virtually any UI interaction through the API.
																		Use the <strong>"Browse Actions"</strong> button to discover all available actions without searching through documentation.
																</span>
														</div>
																								
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">Pro Tip:</strong>
																<span style="color: #B4B7B0;">
																		Use the <strong>"Browse Actions"</strong> button below to fetch this list directly and select an action 
																		without guessing its ID. Categories are loaded dynamically from Stellarium and reflect the current 
																		state of all loaded plugins and modules.
																</span>
														</div>`,
														parameters: {
																id: { 
																		required: true, 
																		type: 'string', 
																		description: 'Action identifier (e.g., actionShow_Stars, actionScript/double_stars.ssc)',
																		suggest: 'actions'
																}
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
														desc: `<p>Get a comprehensive list of all registered StelProperties with their current values and metadata.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">What are StelProperties?</strong>
																<span style="color: #B4B7B0;">
																		StelProperties are the fundamental configuration variables of Stellarium. Every setting, preference, 
																		and adjustable parameter in Stellarium is exposed as a StelProperty. They can be <strong>read</strong> 
																		and <strong>written</strong> through the API, enabling full programmatic control over Stellarium's 
																		configuration.
																</span>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Dynamic Properties:</strong>
																<span style="color: #B4B7B0;">
																		The list of properties is <strong>dynamically generated</strong> from Stellarium's internal property system. 
																		Properties appear or disappear based on which modules and plugins are loaded (e.g., ArchaeoLines, Oculars, 
																		Satellites, Exoplanets, etc.).
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data Structure:</strong>
																<p style="color: #B4B7B0; font-size: 11px; margin: 5px 0;">
																		Returns a JSON object where each key is a property ID, and each value contains detailed property metadata.
																</p>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0; white-space: pre-wrap;">
{
		"MilkyWay.intensity": {
				"value": 1.0,
				"variantType": "double",
				"typeString": "double",
				"typeEnum": 6,
				"canNotify": true,
				"isWritable": true
		},
		"ConstellationMgr.linesColor": {
				"value": "[0.2, 0.2, 0.6]",
				"variantType": "Vector3<float>",
				"typeString": "Vector3<float>",
				"typeEnum": 65536,
				"canNotify": true,
				"isWritable": true
		},
		"StelCore.currentProjectionTypeKey": {
				"value": "ProjectionStereographic",
				"variantType": "QString",
				"typeString": "QString",
				"typeEnum": 10,
				"canNotify": true,
				"isWritable": true
		},
		"LandscapeMgr.allLandscapeNames": {
				"value": ["Garching","Geneva","Guereins","Jupiter","Mars","Moon"],
				"variantType": "QStringList",
				"typeString": "QStringList",
				"typeEnum": 11,
				"canNotify": true,
				"isWritable": false
		}
}</pre>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Property Metadata Fields:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>value</code>: Current value of the property (type varies).</li>
																		<li><code>variantType</code>: The actual Qt data type (e.g., "double", "bool", "int", "QString", "Vector3<float>", "QStringList").</li>
																		<li><code>typeString</code>: The type as declared in the meta-object.</li>
																		<li><code>typeEnum</code>: Numeric Qt meta-type enum value.</li>
																		<li><code>canNotify</code>: Whether the property can emit change signals (true/false).</li>
																		<li><code>isWritable</code>: Whether the property can be modified (read-only if false).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Data Type Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px; columns: 2; column-gap: 20px;">
																		<li><code>bool</code> — true/false (e.g., <code>actionShow_Stars</code>)</li>
																		<li><code>double</code> — Floating point (e.g., <code>MilkyWay.intensity</code>)</li>
																		<li><code>int</code> — Integer (e.g., <code>StelApp.guiFontSize</code>)</li>
																		<li><code>QString</code> — Text string (e.g., <code>StelCore.currentProjectionTypeKey</code>)</li>
																		<li><code>QStringList</code> — String array (e.g., <code>LandscapeMgr.allLandscapeNames</code>)</li>
																		<li><code>Vector3&lt;float&gt;</code> — RGB color [r,g,b] (e.g., <code>ConstellationMgr.linesColor</code>)</li>
																		<li><code>Vector4&lt;float&gt;</code> — 4-component vector (e.g., <code>Oculars.telradFOV</code>)</li>
																		<li><code>Enum types</code> — Name or numeric value (e.g., <code>StelCore::ProjectionType</code>)</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Common Property Categories (varies by installation):</strong>
																<ul style="margin-top: 5px; padding-left: 20px; columns: 2; column-gap: 20px;">
																		<li><strong>MilkyWay.*</strong> — Milky Way display settings</li>
																		<li><strong>StelSkyDrawer.*</strong> — Star and sky rendering settings</li>
																		<li><strong>ConstellationMgr.*</strong> — Constellation display settings</li>
																		<li><strong>AsterismMgr.*</strong> — Asterism display settings</li>
																		<li><strong>SolarSystem.*</strong> — Planet and orbit settings</li>
																		<li><strong>NebulaMgr.*</strong> — Deep-sky object settings</li>
																		<li><strong>StarMgr.*</strong> — Star catalog settings</li>
																		<li><strong>StelMovementMgr.*</strong> — View and movement settings</li>
																		<li><strong>LandscapeMgr.*</strong> — Landscape and ground settings</li>
																		<li><strong>StelCore.*</strong> — Core simulation settings</li>
																		<li><strong>GridLinesMgr.*</strong> — Grid and line display settings</li>
																		<li><strong>Oculars.*</strong> — Oculars plugin settings (if loaded)</li>
																		<li><strong>Satellites.*</strong> — Satellite plugin settings (if loaded)</li>
																		<li><strong>Exoplanets.*</strong> — Exoplanet plugin settings (if loaded)</li>
																		<li><strong>ArchaeoLines.*</strong> — ArchaeoLines plugin settings (if loaded)</li>
																		<li><strong>TelescopeControl.*</strong> — Telescope control settings</li>
																		<li><strong>StelApp.*</strong> — Application-wide settings</li>
																		<li><strong>MainView.*</strong> — Main view settings (screenshot, FPS, etc.)</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">Pro Tip:</strong>
																<span style="color: #B4B7B0;">
																		Use the <strong>"Browse Properties"</strong> button below to fetch this list directly and select a property 
																		without guessing its ID. The list includes current values, data types, and writability information.
																		Properties with <code>isWritable: false</code> are read-only and cannot be modified.
																</span>
														</div>`,
														parameters: {}
												},
												
												set: {
														method: 'POST',
														desc: `<p>Set the value of a specific StelProperty.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">How It Works:</strong>
																<span style="color: #B4B7B0;">
																		This endpoint modifies a StelProperty's value. The server automatically handles type conversion 
																		based on the property's declared type (bool, int, double, string, enum, Vector3, etc.).
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Parameters:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>id</code> (required, string): Property identifier.<br>
																				<span style="color: #888;">Use the <code>list</code> operation or the <strong>"Browse Properties"</strong> button to see all available property IDs.</span>
																		</li>
																		<li style="margin-top: 5px;">
																				<code>value</code> (required, string): New value for the property.<br>
																				<span style="color: #888;">The value will be automatically converted to the property's data type.</span>
																				<ul style="padding-left: 20px; margin-top: 3px;">
																						<li><strong>Boolean:</strong> Use <code>true</code> or <code>false</code> (case-insensitive).</li>
																						<li><strong>Number (int/double):</strong> Use numeric values (e.g., <code>5.0</code>, <code>10</code>).</li>
																						<li><strong>String:</strong> Use quoted or unquoted strings (e.g., <code>ProjectionStereographic</code>).</li>
																						<li><strong>Vector3 (RGB color):</strong> Use JSON array format <code>[0.2,0.2,0.6]</code>.</li>
																						<li><strong>Vector4:</strong> Use JSON array format <code>[0.5,2,4,0]</code>.</li>
																						<li><strong>Enum:</strong> Use numeric enum value or the enum name (e.g., <code>1</code> or <code>ProjectionStereographic</code>).</li>
																						<li><strong>QStringList:</strong> Use JSON array format <code>["item1","item2"]</code> (some may be read-only).</li>
																				</ul>
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Return Values:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><strong>Success:</strong> Returns <code>"ok"</code>.</li>
																		<li><strong>Unknown property:</strong> Returns <code>"error: unknown property"</code>.</li>
																		<li><strong>Invalid data type:</strong> Returns <code>"error: could not set property, invalid data type?"</code>.</li>
																		<li><strong>Read-only property:</strong> Returns <code>"error: could not set property, invalid data type?"</code> (since isWritable is false).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Important Notes:</strong>
																<ul style="margin-top: 5px; padding-left: 20px; margin-bottom: 0;">
																		<li>Only writable properties (those with <code>isWritable: true</code>) can be modified.</li>
																		<li>Properties with <code>canNotify: false</code> will not trigger change events, but the value still updates.</li>
																		<li>Enum properties accept either the numeric value or the enum name string.</li>
																		<li>Vector3/Vector4 values must be formatted as JSON arrays: <code>[r,g,b]</code> or <code>[x,y,z,w]</code>.</li>
																		<li>When setting boolean properties, <code>"true"</code>, <code>"1"</code>, <code>"yes"</code> all evaluate to <code>true</code>.</li>
																		<li>String values with special characters should be properly URL-encoded.</li>
																		<li>Some plugin properties only exist when the plugin is loaded (e.g., ArchaeoLines, Oculars).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>id=MilkyWay.intensity&value=2.5</code> → Set Milky Way brightness to 2.5.</li>
																		<li><code>id=StelSkyDrawer.absoluteStarScale&value=1.2</code> → Set star scale to 1.2.</li>
																		<li><code>id=ConstellationMgr.linesDisplayed&value=true</code> → Turn on constellation lines.</li>
																		<li><code>id=StelMovementMgr.flagTracking&value=false</code> → Disable object tracking.</li>
																		<li><code>id=ConstellationMgr.linesColor&value=[1.0,0.5,0.0]</code> → Set constellation line color to orange.</li>
																		<li><code>id=StelCore.currentProjectionTypeKey&value=ProjectionStereographic</code> → Set projection type.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Pro Tip:</strong>
																<span style="color: #B4B7B0;">
																		The <strong>"Browse Properties"</strong> button shows you the current value and data type of each property, 
																		making it easy to know what value format to use. For Vector3 properties, the value is stored as a string 
																		like <code>"[r, g, b]"</code> — use the same format when setting new values.
																</span>
														</div>`,
														parameters: {
																id: { 
																		required: true, 
																		type: 'string', 
																		description: 'Property identifier (e.g., MilkyWay.intensity, StelSkyDrawer.absoluteStarScale)',
																		suggest: 'properties'
																},
																value: { 
																		required: true, 
																		type: 'string', 
																		description: 'New value (auto-converted to appropriate type: bool, int, double, string, enum, Vector3, etc.)' 
																}
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
													desc: `<p>Query the online SIMBAD astronomical database for celestial objects.</p>
													
													<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
															<strong style="color: #66D9EF;">About SIMBAD:</strong>
															<span style="color: #B4B7B0;">
																	SIMBAD (Set of Identifications, Measurements, and Bibliography for Astronomical Data) is a professional astronomical database 
																	maintained by the Centre de Données astronomiques de Strasbourg (CDS). It contains millions of objects with their 
																	coordinates, names, and bibliographical references.
															</span>
													</div>
													
													<div style="margin-top: 10px;">
															<strong>Parameter:</strong>
															<ul style="margin-top: 5px; padding-left: 20px;">
																	<li>
																			<code>str</code> (required, string): Search term for SIMBAD query.<br>
																			<span style="color: #888;">Supports object names, coordinates, and catalog designations.</span>
																	</li>
															</ul>
													</div>
													
													<div style="margin-top: 10px;">
															<strong>Returned Data Structure:</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0;">
{
		"status": "found",           // "found", "empty", "error", or "unknown"
		"status_i18n": "Search completed successfully.",
		"errorString": "",           // Only present on error
		"results": {
				"names": ["M31", "Andromeda Galaxy", "NGC 224"],
				"positions": [
						[0.12, 0.34, 0.56],  // J2000 coordinates (x, y, z) for each object
						[0.78, 0.91, 0.23]
				]
		}
}</pre>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Status Values:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>found</code> → Results were found and returned.</li>
																		<li><code>empty</code> → Search completed but no results found.</li>
																		<li><code>error</code> → An error occurred during the query.</li>
																		<li><code>unknown</code> → Unexpected status (should not normally occur).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Tip:</strong>
																<span style="color: #B4B7B0;">
																		SIMBAD queries are performed asynchronously in a background thread to avoid blocking the main Stellarium process. 
																		Results include the object name(s) and their J2000 position vectors (x, y, z) which can be used with the 
																		<code>focus</code> operation's <code>position</code> parameter.
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>str=M31</code> → Search for the Andromeda Galaxy.</li>
																		<li><code>str=Betelgeuse</code> → Search for the star Betelgeuse.</li>
																		<li><code>str=alpha cen</code> → Search for Alpha Centauri.</li>
																		<li><code>str=ngc 224</code> → Search by NGC catalog number.</li>
																		<li><code>str=Jupiter</code> → Search for the planet Jupiter.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(249, 38, 114, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #F92672;">Notes:</strong>
																<ul style="margin-top: 5px; padding-left: 20px; margin-bottom: 0;">
																		<li>An active internet connection is required for SIMBAD queries.</li>
																		<li>If the SIMBAD server is unavailable, the operation will return an error status.</li>
																		<li>Search terms are automatically trimmed and converted to lowercase.</li>
																		<li>Multiple results are returned as a list of names and corresponding position vectors.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Using Results with focus:</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0; color: #A6E22E;">// Example: Focus on the first SIMBAD result using its position vector
// position = [0.12, 0.34, 0.56] (J2000 coordinates from SIMBAD response)
curl -X POST -d "position=[0.12,0.34,0.56]" http://localhost:8090/api/main/focus</pre>
														</div>`,
														parameters: {
																str: { 
																		required: true, 
																		type: 'string', 
																		description: 'Search term (object name, catalog designation, or coordinates)' 
																}
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
														desc: `<p>Get a list of all installed landscapes with their names.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">What are Landscapes?</strong>
																<span style="color: #B4B7B0;">
																		Landscapes are the ground and horizon environments in Stellarium. Each landscape provides a different 
																		visual setting, from natural horizons to planetary surfaces.
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Returns a JSON object where each key is a landscape ID, and each value is the landscape name (localized).</li>
																		<li>Example: <code>{"guereins": "Guereins", "mars": "Mars", "moon": "Moon", "ocean": "Ocean"}</code></li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Usage:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Use the landscape ID with the <code>LandscapeMgr.setCurrentLandscapeID</code> property to change landscapes.</li>
																		<li><code>curl -d "id=LandscapeMgr.currentLandscapeID" -d "value=trees" http://localhost:8090/api/stelproperty/set</code></li>
																		<li><code>curl -d "id=LandscapeMgr.currentLandscapeID" -d "value=guereins" http://localhost:8090/api/stelproperty/set</code></li>
																</ul>
														</div>`,
														parameters: {}
												},
												
												landscapedescription: {
														method: 'GET',
														desc: `<p>Get the HTML description of the current landscape, or access landscape resource files.</p>
														
														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><strong>Without path:</strong> Returns the HTML description of the currently active landscape.</li>
																		<li><strong>With path:</strong> Returns a specific file from the landscape directory (e.g., images, textures).</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Note:</strong>
																<span style="color: #B4B7B0;">
																		This endpoint follows the pattern: <code>/api/view/landscapedescription/</code> (with trailing slash).
																		Adding a path after the slash will return the corresponding file from the landscape folder.
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Examples:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>GET /api/view/landscapedescription/</code> → Returns HTML description of current landscape.</li>
																		<li><code>GET /api/view/landscapedescription/image.png</code> → Returns the image file from the current landscape folder(Example: guereins1.png, guereins2.png ..).</li>
																</ul>
														</div>`,
														parameters: {},
														fullPath: 'view/landscapedescription/'
												},
												
												listskyculture: {
														method: 'GET',
														desc: `<p>Get a list of all installed sky cultures with their names.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">What are Sky Cultures?</strong>
																<span style="color: #B4B7B0;">
																		Sky cultures define how constellations, asterisms, and celestial patterns are represented in Stellarium. 
																		Each culture provides its own set of constellation lines, names, and artistic interpretations from 
																		different civilizations and time periods (e.g., Modern, Chinese, Tibetan, Egyptian (dendera), etc.).
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Returns a JSON object where each key is a sky culture ID, and each value is the culture name (localized).</li>
																		<li>Example: <code>{"modern": "Modern", "arabic_al-sufi": "Arabic (Al-Sufi)", "chinese": "Chinese", "korean": "Korean"}</code></li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Usage:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>StelProperty method:</li>
																		<li>Use the sky culture ID with the <code>StelSkyCultureMgr.currentSkyCultureID</code> property to change cultures.</li>
																		<li>curl -d "id=StelSkyCultureMgr.currentSkyCultureID" -d "value=chinese" http://localhost:8090/api/stelproperty/set</code></li>
																		<li>curl -d "id=StelSkyCultureMgr.currentSkyCultureID" -d "value=modern" http://localhost:8090/api/stelproperty/set</code></li>
																		<li>curl -d "id=StelSkyCultureMgr.currentSkyCultureID" -d "value=arabic_al-sufi" http://localhost:8090/api/stelproperty/set</code></li>
																</ul>
																<ul style="margin-top: 5px; padding-left: 20px;">																		
																		<li>script method:</li>
																		<li>Use the sky culture ID with the <code>StelSkyCultureMgr.setCurrentSkyCultureID</code> with (/api/scripts/direct) endpoint to change cultures.</li>
																		<li>curl -d "code=StelSkyCultureMgr.setCurrentSkyCultureID(%22modern%22)%3B" -d "useIncludes=false" http://localhost:8090/api/scripts/direct</code></li>
																		<li>curl -d "code=StelSkyCultureMgr.setCurrentSkyCultureID(%22arabic_al-sufi%22)%3B" -d "useIncludes=false" http://localhost:8090/api/scripts/direct</code></li>
																		
																</ul>
														</div>`,
														parameters: {}
												},								
												skyculturedescription: {
														method: 'GET',
														desc: `<p>Access files and resources from the current sky culture folder.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">📁 Available Files:</strong>
																<span style="color: #B4B7B0;">
																		<ul style="margin-top: 5px; padding-left: 20px; margin-bottom: 0;">
																				<li><code>/</code> (empty) → Returns HTML description of the current sky culture.</li>
																				<li><code>index.json</code> → Complete cultural dataset (constellations, asterisms, star names).</li>
																				<li><code>description.md</code> → Raw Markdown description of the culture.</li>
																				<li><code>illustrations/*.png</code> → Constellation and asterism images.</li>
																				<li><code>*.fab</code> → Legacy text files (if present).</li>
																		</ul>
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Parameter:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>
																				<code>file</code> (optional, string): Path to a file within the sky culture folder.<br>
																				<span style="color: #888;">Leave empty to get the HTML description. Use "index.json" for cultural data, "description.md" for raw description, or "illustrations/filename.png" for images.</span>
																		</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px; background: rgba(253, 216, 134, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #E6DB74;">Tip:</strong>
																<span style="color: #B4B7B0;">
																		Use the <strong>"Browse Files"</strong> button below to discover all available files and illustrations 
																		from the current sky culture. The button will analyze <code>index.json</code> to find all referenced images 
																		and list common files.
																</span>
														</div>`,
														parameters: {
																file: {
																		required: false,
																		type: 'string',
																		description: 'File path within the sky culture folder (e.g., "index.json", "description.md", "illustrations/andromeda.png")'
																}
														},
														fullPath: 'view/skyculturedescription/'
												},								
												listprojection: {
														method: 'GET',
														desc: `<p>Get a list of all available projection types.</p>
														
														<div style="margin-top: 10px; background: rgba(102, 217, 239, 0.1); padding: 10px; border-radius: 4px;">
																<strong style="color: #66D9EF;">What are Projections?</strong>
																<span style="color: #B4B7B0;">
																		Projections determine how the 3D celestial sphere is mapped onto the 2D screen. Different projections 
																		provide different visual perspectives, from realistic views to artistic or specialized representations.
																</span>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Returned Data:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Returns a JSON object where each key is a projection type key, and each value is the projection name (localized).</li>
																		<li>Common projections: <code>'ProjectionPerspective'</code>, <code>'ProjectionFisheye'</code>, <code>'ProjectionMercator'</code>, <code>'ProjectionHammer'</code>, <code>'ProjectionMollweide'</code>, <code>'ProjectionCylinder'</code>, <code>'ProjectionOrthographic'</code>, <code>'ProjectionStereographic'</code>, <code>'ProjectionEqualArea'</code>, <code>'ProjectionMiller'</code>, <code>'ProjectionSinusoidal'</code></li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Usage: set Projection Type</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>StelProperty method:</li>
																		<li>Use the projection key with the <code>StelCore.currentProjectionTypeKey</code> property to change projections.</li>
																		<li><code>curl -d "id=StelCore.currentProjectionTypeKey" -d "value=ProjectionFisheye" http://localhost:8090/api/stelproperty/set</code></li>
																		<li><code>curl -d "id=StelCore.currentProjectionTypeKey" -d "value=ProjectionStereographic" http://localhost:8090/api/stelproperty/set</code></li>
																</ul>
																<ul style="margin-top: 5px; padding-left: 20px;">
																<li>Script method:</li>
																<li>Use the projection key with the <code>StelCore.setProjectionMode</code> function with endpoint:(/api/scripts/direct) to change projections.</li>
																<li><code>curl -d "code=core.setProjectionMode('ProjectionStereographic')%3B" -d "useIncludes=false" http://localhost:8090/api/scripts/direct</code></li>
																<li><code>curl -d "code=core.setProjectionMode('ProjectionEqualArea')%3B" -d "useIncludes=false" http://localhost:8090/api/scripts/direct</code></li>
																<li><code>curl -d "code=core.setProjectionMode('ProjectionMercator')%3B" -d "useIncludes=false" http://localhost:8090/api/scripts/direct</code></li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Example Response:</strong>
<pre style="background: #888; padding: 10px; border-radius: 4px; font-size: 11px; margin: 5px 0;">
{
		"ProjectionStereographic": "Stereographic",
		"ProjectionOrthographic": "Orthographic",
		"ProjectionFisheye": "Fisheye",
		"ProjectionMercator": "Mercator",
		"ProjectionHammer": "Hammer-Aitoff"
}</pre>
														</div>`,
														parameters: {}
												},
												
												projectiondescription: {
														method: 'GET',
														desc: `<p>Get the HTML description of the current projection.</p>
														
														<div style="margin-top: 10px;">
																<strong>No Parameters Required</strong>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Behavior:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li>Returns the HTML description/summary of the currently active projection.</li>
																		<li>Includes information about the projection's characteristics and usage.</li>
																</ul>
														</div>
														
														<div style="margin-top: 10px;">
																<strong>Example:</strong>
																<ul style="margin-top: 5px; padding-left: 20px;">
																		<li><code>GET /api/view/projectiondescription</code> → Returns HTML description of current projection.</li>
																</ul>
														</div>`,
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
		 * Escape HTML special characters for use in HTML attributes.
		 */
		function escapeAttr(text) {
				if (!text) return '';
				return String(text).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/'/g, '&#39;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
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
		 * Handles image URLs specially (shows the URL instead of binary data).
		 */
		function toggleResponseDisplay() {
				if (currentDisplayMode === 'formatted') {
						// Switch to raw mode
						currentDisplayMode = 'raw';
						var $wrapper = $('#explorer-response-area .response-content-wrapper');
						
						// Check if it's an image response (raw contains Image: URL)
						if (currentRawResponse && currentRawResponse.startsWith('Image:')) {
								var imageUrl = currentRawResponse.replace('Image:', '').trim();
								$wrapper.html(
										'<pre class="raw-response" style="white-space: pre-wrap; word-break: break-word; margin: 0; font-family: monospace; font-size: 12px;">' + 
										escapeHtml(imageUrl) + 
										'</pre>'
								);
						} else {
								$wrapper.html(
										'<pre class="raw-response" style="white-space: pre-wrap; word-break: break-word; margin: 0; font-family: monospace; font-size: 11px;">' + 
										escapeHtml(currentRawResponse) + 
										'</pre>'
								);
						}
						$('#toggle-display-btn').html('[Show Formatted]');
				} else {
						// Switch to formatted mode
						currentDisplayMode = 'formatted';
						$('#explorer-response-area .response-content-wrapper').html(currentFormattedHtml);
						$('#toggle-display-btn').html('[Show Raw]');
				}
		}
		
		// =====================================================================
		// SKY CULTURE FILE BROWSER
		// =====================================================================

		/**
		 * Browse and display all available files in the current sky culture
		 * with proper cache-busting to prevent showing images from previous cultures.
		 */
		function browseSkyCultureFiles() {
				var $responseArea = $('#explorer-response-area');
				var $statusEl = $('#explorer-response-status');
				
				$responseArea.html('<div class="response-loading"><span class="loading-spinner"></span> Scanning sky culture files...</div>');
				$statusEl.text('');
				
				var commonFiles = [
						'index.json',
						'description.md',
						//'description.html',
						//'culture.json',
						//'stars.json',
						//'constellations.json'
				];
				
				$.ajax({
						url: '/api/stelproperty/list',
						dataType: 'json',
						timeout: 5000
				}).done(function(propData) {
						var cultureId = null;
						if (propData && propData['StelSkyCultureMgr.currentSkyCultureID']) {
								cultureId = propData['StelSkyCultureMgr.currentSkyCultureID'].value;
						}
						if (!cultureId) {
								cultureId = 'modern';
						}
						
						var cacheBuster = '?t=' + new Date().getTime();
						$.ajax({
								url: '/api/view/skyculturedescription/index.json' + cacheBuster,
								dataType: 'json',
								timeout: 5000
						}).done(function(indexData) {
								var illustrations = [];
								var seenPaths = {};
								
								if (indexData.constellations && Array.isArray(indexData.constellations)) {
										indexData.constellations.forEach(function(constel) {
												if (constel.image && constel.image.file) {
														var path = constel.image.file;
														if (!seenPaths[path]) {
																seenPaths[path] = true;
																var name = constel.common_name ? 
																		(constel.common_name.english || constel.common_name.native || constel.id) : 
																		constel.id;
																illustrations.push({
																		path: path,
																		name: name || 'Unnamed',
																		type: 'constellation'
																});
														}
												}
										});
								}
								
								if (indexData.asterisms && Array.isArray(indexData.asterisms)) {
										indexData.asterisms.forEach(function(asterism) {
												if (asterism.image && asterism.image.file) {
														var path = asterism.image.file;
														if (!seenPaths[path]) {
																seenPaths[path] = true;
																var name = asterism.common_name ? 
																		(asterism.common_name.english || asterism.common_name.native || asterism.id) : 
																		asterism.id;
																illustrations.push({
																		path: path,
																		name: name || 'Unnamed',
																		type: 'asterism'
																});
														}
												}
										});
								}
								
								showFileBrowser(commonFiles, illustrations, cultureId);
								
						}).fail(function() {
								showFileBrowser(commonFiles, [], cultureId);
						});
				}).fail(function() {
						showFileBrowser(commonFiles, [], 'modern');
				});
		}

		/**
		 * Display file browser with common files and illustrations
		 * with proper cache-busting for images.
		 * FIXED: All user input is escaped before HTML insertion.
		 */
		function showFileBrowser(commonFiles, illustrations, cultureId) {
				var $responseArea = $('#explorer-response-area');
				
				// ============================================================
				// CRITICAL: Use culture ID to prevent cross-culture contamination
				// ============================================================
				// SAFE: Use escapeAttr() for attribute values
				var cacheBuster = '?t=' + new Date().getTime();
				var cultureCacheKey = 'culture_' + cultureId + '_' + new Date().getTime();
				
				var html = '<div style="padding:10px;" data-culture="' + escapeAttr(cultureId) + '" data-cache-key="' + escapeAttr(cultureCacheKey) + '">';
				
				// Culture info with clear identification
				html += '<div style="background:rgba(102,217,239,0.1);padding:10px;border-radius:4px;margin-bottom:15px;border-left:3px solid #66D9EF;">';
				html += '<strong style="color:#66D9EF;">📁 Current Sky Culture:</strong> ';
				html += '<span style="color:#B4B7B0;font-weight:bold;">' + escapeHtml(cultureId) + '</span>';
				html += ' <span style="color:#8A8C8E;font-size:10px;">(' + commonFiles.length + ' common files, ' + illustrations.length + ' illustrations)</span>';
				html += '<span style="display:block;font-size:9px;color:#8A8C8E;margin-top:4px;">' + 
								'Loaded: ' + new Date().toLocaleString() + ' (cache-busting enabled)</span>';
				html += '</div>';
				
				// Common files section
				if (commonFiles.length > 0) {
						html += '<h4 style="color:#FD971F;margin:10px 0 8px 0;font-size:12px;">Common Files</h4>';
						html += '<div style="display:flex;flex-wrap:wrap;gap:6px;margin-bottom:15px;">';
						commonFiles.forEach(function(filename) {
								// SAFE: Escape filename
								html += '<button class="file-btn jquerybutton" data-file="' + escapeAttr(filename) + '" style="font-size:10px;padding:4px 10px;">';
								html += ' ' + escapeHtml(filename);
								html += '</button>';
						});
						html += '</div>';
				}
				
				// Illustrations section
				if (illustrations.length > 0) {
						html += '<h4 style="color:#A6E22E;margin:10px 0 8px 0;font-size:12px;">Illustrations (' + illustrations.length + ')</h4>';
						html += '<div style="display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:8px;" data-culture="' + escapeAttr(cultureId) + '">';
						
						illustrations.forEach(function(item) {
								// ============================================================
								// CRITICAL: Add cache-busting timestamp to each image URL
								// This prevents browser from showing cached images from previous culture
								// ============================================================
								var imageUrl = '/api/view/skyculturedescription/' + item.path + cacheBuster;
								
								// SAFE: Escape all user-controlled data
								html += '<div class="illustration-thumb" data-path="' + escapeAttr(item.path) + 
												'" data-culture="' + escapeAttr(cultureId) + '" style="' +
												'border:1px solid rgba(255,255,255,0.1);border-radius:4px;overflow:hidden;cursor:pointer;' +
												'transition:all 0.2s ease;background:rgba(0,0,0,0.2);' +
												'" onmouseover="this.style.borderColor=\'#FD971F\'" ' +
												'onmouseout="this.style.borderColor=\'rgba(255,255,255,0.1)\'">';
								html += '<div style="height:100px;overflow:hidden;display:flex;align-items:center;justify-content:center;background:rgba(0,0,0,0.3);position:relative;">';
								html += '<img src="' + escapeAttr(imageUrl) + '" alt="' + escapeAttr(item.name) + '" ' +
												'loading="lazy" ' +
												'style="max-width:100%;max-height:100px;object-fit:contain;" ' +
												'data-culture="' + escapeAttr(cultureId) + '" ' +
												'onerror="this.style.display=\'none\';this.parentElement.querySelector(\'.thumb-error\').style.display=\'flex\';">';
								html += '<div class="thumb-error" style="display:none;position:absolute;top:0;left:0;width:100%;height:100%;align-items:center;justify-content:center;color:#666;font-size:10px;background:rgba(0,0,0,0.2);">' + _tr("No image") + '</div>';
								html += '</div>';
								html += '<div style="padding:4px 6px;font-size:8px;color:#B4B7B0;text-overflow:ellipsis;overflow:hidden;white-space:nowrap;">';
								html += '<span style="color:#66D9EF;font-size:7px;">' + escapeHtml(item.type) + '</span> ';
								html += escapeHtml(item.name);
								html += '</div>';
								html += '</div>';
						});
						
						html += '</div>';
				}
				
				// Manual file input - this part is safe as it uses HTML elements with no user input
				html += '<div style="margin-top:15px;padding:10px;background:rgba(0,0,0,0.15);border-radius:4px;">';
				html += '<div style="display:flex;gap:8px;flex-wrap:wrap;align-items:center;">';
				html += '<label style="font-size:11px;color:#B4B7B0;">🔍 Enter file path:</label>';
				html += '<input type="text" id="file-path-input" placeholder="e.g., index.json, illustrations/andromeda.png" style="' +
								'flex:1;min-width:200px;padding:6px 10px;background:rgba(0,0,0,0.3);color:#DCDBDA;border:1px solid #3A3C3E;border-radius:4px;font-size:11px;">';
				html += '<button id="btn-load-file" class="jquerybutton" style="font-size:10px;padding:6px 14px;">Load File</button>';
				html += '</div>';
				html += '</div>';
				
				html += '</div>';
				
				currentRawResponse = 'Sky culture file browser: ' + cultureId;
				currentFormattedHtml = html;
				currentDisplayMode = 'formatted';
				
				$responseArea.html(
						'<div class="response-toolbar">' +
						'<div class="response-toolbar-left">' +
						'<span class="response-type-badge">FILE BROWSER</span>' +
						'<span style="color:#8A8C8E;font-size:10px;">' + escapeHtml(cultureId) + '</span>' +
						'<span style="color:#8A8C8E;font-size:9px;margin-left:8px;">🔒 cache-busting enabled</span>' +
						'</div>' +
						'<div class="response-toolbar-right">' +
						'<button class="copy-btn jquerybutton" id="copy-file-list">Copy File List</button>' +
						'<button class="copy-btn ui-icon-refresh jquerybutton" id="refresh-culture-files" style="margin-left:4px;">Refresh</button>' +
						'</div>' +
						'</div>' +
						'<div class="response-content-wrapper" style="direction:ltr;max-height:70vh;overflow-y:auto;">' +
						html +
						'</div>'
				);
				
				// ============================================================
				// BIND EVENTS WITH CULTURE CONTEXT
				// ============================================================
				
				// Common file buttons
				// Bind events
				$('.file-btn').off('click').on('click', function() {
						var filename = $(this).data('file');
						loadSkyCultureFile(filename, cultureId);
				});
				
				// Illustration thumbnails
				$('.illustration-thumb').off('click').on('click', function() {
						var path = $(this).data('path');
						// Pass culture ID to ensure correct context
						loadSkyCultureFile(path, cultureId);
				});
				
				// Load file button
				$('#btn-load-file').off('click').on('click', function() {
						var path = $('#file-path-input').val().trim();
						if (path) {
								loadSkyCultureFile(path, cultureId);
						}
				});
				
				// Enter key on input
				$('#file-path-input').off('keydown').on('keydown', function(e) {
						if (e.key === 'Enter') {
								var path = $(this).val().trim();
								if (path) {
										loadSkyCultureFile(path, cultureId);
								}
						}
				});
				
				// Refresh button - reload everything
				$('#refresh-culture-files').off('click').on('click', function() {
						browseSkyCultureFiles();
				});
				
				// Copy file list
				$('#copy-file-list').off('click').on('click', function() {
						var list = 'Sky Culture: ' + cultureId + '\n';
						list += 'Loaded: ' + new Date().toLocaleString() + '\n';
						list += 'Common Files: ' + commonFiles.join(', ') + '\n';
						list += 'Illustrations:\n';
						illustrations.forEach(function(item) {
								list += '  - ' + item.path + ' (' + item.name + ')\n';
						});
						navigator.clipboard.writeText(list).then(function() {
								var $btn = $('#copy-file-list');
								var originalText = $btn.html();
								$btn.html('Copied!');
								setTimeout(function() { $btn.html(originalText); }, 2000);
						});
				});
		}

		/**
		 * Display loaded file content with culture context using jQuery UI icons.
		 * FIXED: All user input is escaped using escapeHtml() before HTML insertion.
		 */
		function displayFileContent(filePath, url, type, content, cultureId) {
				var $responseArea = $('#explorer-response-area');
				var $statusEl = $('#explorer-response-status');
				
				cultureId = cultureId || 'unknown';
				
				currentRawResponse = type === 'image' ? 'Image: ' + url : content || '';
				currentDisplayMode = 'formatted';
				
				var html = '';
				
				if (type === 'image') {
						// SAFE: All user input is escaped with escapeHtml()
						html = '<div style="text-align:center;padding:20px;" data-culture="' + escapeAttr(cultureId) + '">';
						html += '<div style="margin-bottom:10px;font-size:12px;color:#FD971F;">';
						html += '<span class="ui-icon ui-icon-image" style="display:inline-block;vertical-align:middle;margin-right:6px;"></span>';
						html += escapeHtml(filePath);
						html += ' <span style="font-size:9px;color:#8A8C8E;">(' + escapeHtml(cultureId) + ')</span>';
						html += '</div>';
						html += '<img src="' + escapeAttr(url) + '" alt="' + escapeAttr(filePath) + '" ' +
										'style="max-width:100%;max-height:70vh;border:1px solid rgba(255,255,255,0.1);border-radius:4px;" ' +
										'data-culture="' + escapeAttr(cultureId) + '" ' +
										'onerror="this.style.display=\'none\';this.parentElement.innerHTML=\'<div style=\\\'color:#888;padding:20px;\\\'><span class=\\\'ui-icon ui-icon-alert\\\' style=\\\'display:inline-block;vertical-align:middle;margin-right:6px;\\\'></span><strong>Image not available</strong><br>Could not load: ' + escapeAttr(filePath) + '<br>Culture: ' + escapeAttr(cultureId) + '</div>\';">';
						html += '<div style="margin-top:10px;font-size:10px;color:#8A8C8E;">';
						html += '<span class="ui-icon ui-icon-link" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span>';
						html += 'URL: <code style="background:rgba(0,0,0,0.3);padding:2px 6px;border-radius:3px;word-break:break-all;">' + escapeHtml(url) + '</code>';
						html += '</div>';
						html += '<div style="margin-top:5px;font-size:9px;color:#8A8C8E;">';
						html += 'Culture: <strong>' + escapeHtml(cultureId) + '</strong> | Cache-busting: enabled';
						html += '</div>';
						html += '</div>';
						$statusEl.html('<span class="response-status success">Image loaded: ' + escapeHtml(filePath) + ' (' + escapeHtml(cultureId) + ')</span>');
						
				} else if (type === 'text') {
						try {
								var jsonData = JSON.parse(content);
								// SAFE: Use escapeHtml() for all user-controlled data
								html = '<div style="padding:10px;" data-culture="' + escapeAttr(cultureId) + '">';
								html += '<div style="font-size:10px;color:#8A8C8E;margin-bottom:8px;">';
								html += '<span class="ui-icon ui-icon-document" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span>';
								html += escapeHtml(filePath) + ' (' + (content ? content.length : 0) + ' chars) - Culture: ' + escapeHtml(cultureId);
								html += '</div>';
								html += '<pre style="background:rgba(0,0,0,0.3);padding:15px;border-radius:4px;font-family:monospace;font-size:10px;color:#A6E22E;white-space:pre-wrap;word-break:break-word;max-height:60vh;overflow-y:auto;margin:0;">';
								// CRITICAL: Escape JSON content before rendering
								html += escapeHtml(JSON.stringify(jsonData, null, 2));
								html += '</pre>';
								html += '</div>';
								$statusEl.html('<span class="response-status success">JSON loaded: ' + escapeHtml(filePath) + ' (' + escapeHtml(cultureId) + ')</span>');
						} catch(e) {
								// SAFE: Escape content before rendering
								html = '<div style="padding:10px;" data-culture="' + escapeAttr(cultureId) + '">';
								html += '<div style="font-size:10px;color:#8A8C8E;margin-bottom:8px;">';
								html += '<span class="ui-icon ui-icon-document" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span>';
								html += escapeHtml(filePath) + ' (' + (content ? content.length : 0) + ' chars) - Culture: ' + escapeHtml(cultureId);
								html += '</div>';
								html += '<pre style="background:rgba(0,0,0,0.3);padding:15px;border-radius:4px;font-family:monospace;font-size:10px;color:#DCDBDA;white-space:pre-wrap;word-break:break-word;max-height:60vh;overflow-y:auto;margin:0;">';
								html += escapeHtml(content);
								html += '</pre>';
								html += '</div>';
								$statusEl.html('<span class="response-status success">File loaded: ' + escapeHtml(filePath) + ' (' + escapeHtml(cultureId) + ')</span>');
						}
						
				} else {
						// SAFE: Error message is escaped
						html = '<div style="padding:20px;text-align:center;color:#F92672;" data-culture="' + escapeAttr(cultureId) + '">';
						html += '<div style="font-size:24px;margin-bottom:10px;">';
						html += '<span class="ui-icon ui-icon-alert" style="display:inline-block;vertical-align:middle;margin-right:8px;font-size:24px;"></span>';
						html += '</div>';
						html += '<div><strong>Error:</strong> ' + escapeHtml(content) + '</div>';
						html += '<div style="margin-top:10px;font-size:10px;color:#8A8C8E;">Path: ' + escapeHtml(filePath) + ' | Culture: ' + escapeHtml(cultureId) + '</div>';
						html += '</div>';
						$statusEl.html('<span class="response-status error">Error: ' + escapeHtml(filePath) + ' (' + escapeHtml(cultureId) + ')</span>');
				}
				
				currentFormattedHtml = html;
				
				$responseArea.html(
						'<div class="response-toolbar">' +
						'<div class="response-toolbar-left">' +
						'<span class="response-type-badge">' +
						'<span class="ui-icon ui-icon-' + (type === 'image' ? 'image' : (type === 'error' ? 'alert' : 'document')) + '" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span>' +
						(type === 'image' ? 'IMAGE' : (type === 'error' ? 'ERROR' : 'FILE')) +
						'</span>' +
						'<span style="color:#8A8C8E;font-size:10px;">' + escapeHtml(filePath) + '</span>' +
						'<span style="color:#8A8C8E;font-size:9px;margin-left:6px;">🌐 ' + escapeHtml(cultureId) + '</span>' +
						'</div>' +
						'<div class="response-toolbar-right">' +
						'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">' +
						'<span class="ui-icon ui-icon-document" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span> [Show Raw]' +
						'</button>' +
						'<button class="toggle-dir-btn jquerybutton" id="toggle-dir-btn" title="Toggle text direction for RTL languages">' +
						'<span class="ui-icon ui-icon-transfer-e-w" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span> [Switch to RTL]' +
						'</button>' +
						'<button class="copy-btn jquerybutton" id="copy-file-url">' +
						'<span class="ui-icon ui-icon-copy" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span> Copy URL' +
						'</button>' +
						'<button class="copy-btn jquerybutton" id="close-file-view">' +
						'<span class="ui-icon ui-icon-arrowreturn-1-w" style="display:inline-block;vertical-align:middle;margin-right:4px;"></span> Back' +
						'</button>' +
						'</div>' +
						'</div>' +
						'<div class="response-content-wrapper" style="direction:ltr;max-height:70vh;overflow-y:auto;">' +
						html +
						'</div>'
				);
				
				// Bind events (same as before)
				$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
				$('#toggle-dir-btn').off('click').on('click', function() { toggleResponseDirection(); });
				
				$('#copy-file-url').off('click').on('click', function() {
						var cleanUrl = url.split('?')[0];
						navigator.clipboard.writeText(cleanUrl).then(function() {
								var $btn = $('#copy-file-url');
								var originalText = $btn.html();
								$btn.html('Copied!');
								setTimeout(function() { $btn.html(originalText); }, 2000);
						});
				});
				
				$('#close-file-view').off('click').on('click', function() {
						browseSkyCultureFiles();
				});
		}

		/**
		 * Load and display a specific file from the sky culture
		 * with proper cache-busting and culture context.
		 */
		function loadSkyCultureFile(filePath, cultureId) {
				var $responseArea = $('#explorer-response-area');
				var $statusEl = $('#explorer-response-status');
				
				var cacheBuster = '?t=' + new Date().getTime();
				var cultureIdForCache = cultureId || 'unknown';
				
				var imageExtensions = ['.png', '.jpg', '.jpeg', '.gif', '.webp', '.svg', '.bmp', '.tiff'];
				var isImage = imageExtensions.some(function(ext) {
						return filePath.toLowerCase().endsWith(ext);
				});
				
				var url = '/api/view/skyculturedescription/' + filePath + cacheBuster;
				
				$statusEl.html('<span class="response-status">Loading: ' + escapeHtml(filePath) + ' (' + escapeHtml(cultureIdForCache) + ')</span>');
				$responseArea.html('<div class="response-loading"><span class="loading-spinner"></span> Loading ' + escapeHtml(filePath) + '...</div>');
				
				if (isImage) {
						var img = new Image();
						img.onload = function() {
								displayFileContent(filePath, url, 'image', null, cultureIdForCache);
						};
						img.onerror = function() {
								displayFileContent(filePath, url, 'error', 'Could not load image. File may not exist.', cultureIdForCache);
						};
						img.setAttribute('data-culture', cultureIdForCache);
						img.src = url;
				} else {
						$.ajax({
								url: url,
								dataType: 'text',
								timeout: 10000,
								headers: {
										'Cache-Control': 'no-cache, no-store, must-revalidate',
										'Pragma': 'no-cache',
										'Expires': '0'
								}
						}).done(function(data) {
								displayFileContent(filePath, url, 'text', data, cultureIdForCache);
						}).fail(function(xhr) {
								var errorMsg = 'Could not load file. Status: ' + (xhr.status || 'unknown');
								if (xhr.status === 404) {
										errorMsg = 'File not found: ' + filePath;
								}
								displayFileContent(filePath, url, 'error', errorMsg, cultureIdForCache);
						});
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
		 * FIXED: All string values are escaped before HTML insertion.
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
						// SAFE: Escape the string value before inserting into HTML
						var escaped = escapeHtml(value);
						// Check if the string contains HTML
						if (value.trim().startsWith('<') && value.includes('>')) {
								return '<span class="json-string html-content" title="HTML content">"' + escaped + '"</span>';
						}
						return '<span class="json-string">"' + escaped + '"</span>';
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
								
								// SAFE: Escape the key name
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
				
				// SAFE: Fallback - escape any other value
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
        // Add Browse Files button for skyculturedescription
				if (serviceKey === 'view' && operationKey === 'skyculturedescription') {
						html += '<div style="margin: 15px 0; display: flex; gap: 10px; flex-wrap: wrap;">';
						html += '<button class="jquerybutton" id="btn-browse-files" style="background:linear-gradient(#6B6E70, #4A4C4E);border-color:#FD971F;">';
						html += '📂 Browse Files';
						html += '</button>';
						html += '</div>';
				}

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
				
				// bind  "Browse Types" Button
				$('#explorer-browse-types-btn').on('click', function(e) {
						e.preventDefault();
						var $targetInput = $('#explorer-param-type');
						if ($targetInput.length) {
								createExplorerSuggestionDropdown($(this), $targetInput, 'objecttypes');
						}
				});
				
				// Bind "Browse Actions" Button
				$('#explorer-browse-actions-btn').on('click', function(e) {
						e.preventDefault();
						var $targetInput = $('#explorer-param-id');
						if ($targetInput.length) {
								createExplorerSuggestionDropdown($(this), $targetInput, 'actions');
						}
				});
				
				// Binding "Browse Properties" Button
				$('#explorer-browse-properties-btn').on('click', function(e) {
						e.preventDefault();
						var $targetInput = $('#explorer-param-id');
						if ($targetInput.length) {
								createExplorerSuggestionDropdown($(this), $targetInput, 'properties');
						}
				});
        
				// Bind Browse Files button
				$('#btn-browse-files').off('click').on('click', function() {
						browseSkyCultureFiles();
				});
				
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
		 * Supports various input types: text, number, datetime, select, checkbox.
		 * Also adds contextual "Browse" buttons for specific parameter types.
		 * 
		 * @param {string} key - Parameter key/name
		 * @param {Object} param - Parameter definition containing type, description, and options
		 * @param {boolean} required - Whether the parameter is required
		 * @returns {string} HTML string for the parameter field
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
				
				// Special handling for file parameter (sky culture file browser)
				} else if (key === 'file') {
						html += '<input type="text" id="explorer-param-' + key + '" ';
						html += 'placeholder="e.g., index.json, description.md, illustrations/andromeda.png" style="width:100%;">';
						if (param.description) {
								html += '<span class="param-hint">' + escapeHtml(param.description) + '</span>';
						}
						html += '<div style="margin-top:5px;font-size:9px;color:#8A8C8E;">';
						html += 'Tip: Use the "Browse Files" button to see available files and illustrations.';
						html += '</div>';
						html += '</div>';
						return html;
				
				// Special handling for select type
				} else if (param.type === 'select') {
						html += '<select id="explorer-param-' + key + '" style="width:100%;">';
						// Add empty option if not required
						if (!required) {
								html += '<option value="">-- Select ' + key + ' --</option>';
						}
						if (param.options && Array.isArray(param.options)) {
								param.options.forEach(function(opt) {
										var selected = (param.defaultValue !== undefined && param.defaultValue === opt) ? ' selected' : '';
										html += '<option value="' + escapeAttr(opt) + '"' + selected + '>' + escapeHtml(opt) + '</option>';
								});
						}
						html += '</select>';
						if (param.description) {
								html += '<span class="param-hint">' + escapeHtml(param.description) + '</span>';
						}
				
				// Special handling for checkbox type
				} else if (param.type === 'checkbox') {
						var checked = (param.defaultValue === true) ? ' checked' : '';
						html += '<input type="checkbox" id="explorer-param-' + key + '"' + checked + '>';
						if (param.description) {
								html += ' <span class="param-hint" style="display:inline;">' + escapeHtml(param.description) + '</span>';
						}
				
				// Standard text/number input
				} else {
						var inputType = param.type === 'number' ? 'number' : 'text';
						html += '<input type="' + inputType + '" id="explorer-param-' + key + '" ';
						html += 'placeholder="' + escapeHtml(param.description || '') + '"';
						html += (inputType === 'number' ? ' step="any"' : '') + ' style="width:100%;">';
						if (param.description) {
								html += '<span class="param-hint">' + escapeHtml(param.description) + '</span>';
						}
				}
				
				// ============================================================
				// CONTEXTUAL BROWSE BUTTONS
				// ============================================================
				
				// Browse Types button for object type parameter (listobjectsbytype)
				if (key === 'type' && param.suggest === 'objecttypes') {
						html += '<div style="margin-top:5px;">';
						html += '<button type="button" class="suggestion-btn jquerybutton" id="explorer-browse-types-btn" title="Browse available object types (catalogs)">';
						html += '<span class="suggestion-btn-icon">T</span> Browse Types';
						html += '</button>';
						html += '</div>';
				}
				
				// Browse Actions button for action ID parameter (stelaction/do)
				if (key === 'id' && param.suggest === 'actions') {
						html += '<div style="margin-top:5px;">';
						html += '<button type="button" class="suggestion-btn jquerybutton" id="explorer-browse-actions-btn" title="Browse available StelActions (categorized from Stellarium)">';
						html += '<span class="suggestion-btn-icon">A</span> Browse Actions';
						html += '</button>';
						html += '</div>';
				}
				
				// Browse Properties button for property ID parameter (stelproperty/set)
				if (key === 'id' && param.suggest === 'properties') {
						html += '<div style="margin-top:5px;">';
						html += '<button type="button" class="suggestion-btn jquerybutton" id="explorer-browse-properties-btn" title="Browse available StelProperties with current values and types">';
						html += '<span class="suggestion-btn-icon">P</span> Browse Properties';
						html += '</button>';
						html += '</div>';
				}
				
				html += '</div>';
				return html;
		}

		// =====================================================================
		// SUGGESTION DROPDOWN (for API Explorer)
		// =====================================================================

		/**
		 * Create a suggestion dropdown for a parameter field.
		 * 
		 * @param {jQuery} $triggerElement - The suggestion link/button that was clicked
		 * @param {jQuery} $targetInput - The input field to fill when user selects
		 * @param {string} type - Type of suggestions: 'objecttypes'
		 */
		function createExplorerSuggestionDropdown($triggerElement, $targetInput, type) {
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
				fetchExplorerSuggestions(type, function(items) {
						renderExplorerSuggestionList($list, items, function(selectedValue) {
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
								updateExplorerSelection($items, selectedIndex);
						} else if (e.key === 'ArrowUp') {
								e.preventDefault();
								selectedIndex = Math.max(selectedIndex - 1, 0);
								updateExplorerSelection($items, selectedIndex);
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
				$(document).on('mousedown.suggestionExplorer', function(e) {
						if (!$(e.target).closest('.suggestion-dropdown').length && 
										!$(e.target).is($triggerElement)) {
								$dropdown.remove();
								$(document).off('mousedown.suggestionExplorer');
						}
				});
		}

		/**
		 * Position the dropdown relative to the trigger element.
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
		 * Fetches suggestion data from Stellarium API endpoints for autocomplete dropdowns.
		 * 
		 * Supports multiple data types with custom transformation logic for each:
		 * - objecttypes: Fetches object type keys from /api/objects/listobjecttypes
		 * - actions: Fetches categorized StelActions from /api/stelaction/list
		 * - scripts: Fetches script filenames from /api/scripts/list
		 * - properties: Fetches property IDs from /api/stelproperty/list
		 * - locations: Fetches location names from /api/location/list
		 * 
		 * @param {string} type - The type of suggestions to fetch
		 * @param {function} callback - Called with array of suggestion items {id, text, category?, ...}
		 */
		function fetchExplorerSuggestions(type, callback) {
				var endpoints = {
						'objecttypes': '/api/objects/listobjecttypes',
						'actions': '/api/stelaction/list',
						'scripts': '/api/scripts/list',
						'properties': '/api/stelproperty/list',
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
						
						// ============================================================
						// TYPE: actions - Categorized StelActions
						// Sort by category then by text within each category
						// ============================================================
						if (type === 'actions') {
								if (typeof data === 'object' && data !== null) {
										var categories = Object.keys(data);
										// Sort categories alphabetically
										categories.sort(function(a, b) {
												return a.localeCompare(b);
										});
										
										categories.forEach(function(category) {
												var actions = data[category];
												if (Array.isArray(actions)) {
														// Sort actions within category by text
														actions.sort(function(a, b) {
																return (a.text || a.id).localeCompare(b.text || b.id);
														});
														
														actions.forEach(function(action) {
																var displayText = action.id;
																if (action.text && action.text !== action.id) {
																		displayText = action.id + ' → ' + action.text;
																}
																// Add state indicator for checkable actions
																if (action.isCheckable === true) {
																		var state = action.isChecked ? '✓' : '✗';
																		displayText += ' [' + state + ']';
																}
																items.push({
																		id: action.id,
																		text: displayText,
																		category: category,
																		isCheckable: action.isCheckable,
																		isChecked: action.isChecked,
																		actionText: action.text
																});
														});
												}
										});
								}
						}
						
						// ============================================================
						// TYPE: objecttypes - Sort by key
						// ============================================================
						else if (type === 'objecttypes') {
								if (Array.isArray(data)) {
										data.forEach(function(item) {
												items.push({
														id: item.key,
														text: item.key + ' → ' + (item.name_i18n || item.name || item.key),
														name: item.name,
														name_i18n: item.name_i18n
												});
										});
										// Sort by key
										items.sort(function(a, b) {
												return a.id.localeCompare(b.id);
										});
								}
						}
						
						// ============================================================
						// TYPE: scripts - Sort alphabetically
						// ============================================================
						else if (type === 'scripts') {
								if (Array.isArray(data)) {
										data.forEach(function(filename) {
												if (typeof filename === 'string' && filename.trim()) {
														items.push({
																id: filename,
																text: filename,
																name: filename
														});
												}
										});
										items.sort(function(a, b) {
												return a.text.localeCompare(b.text);
										});
								}
						}
						
						// ============================================================
						// TYPE: properties - Sort by property ID
						// ============================================================
						else if (type === 'properties') {
								if (typeof data === 'object' && data !== null) {
										var propIds = Object.keys(data);
										propIds.sort(function(a, b) {
												return a.localeCompare(b);
										});
										
										propIds.forEach(function(propId) {
												var propInfo = data[propId];
												var displayText = propId;
												if (propInfo && typeof propInfo === 'object' && propInfo.value !== undefined) {
														var valueStr = typeof propInfo.value === 'string' ? 
																'"' + propInfo.value + '"' : 
																String(propInfo.value);
														if (valueStr.length > 40) {
																valueStr = valueStr.substring(0, 37) + '...';
														}
														displayText = propId + ' = ' + valueStr;
														if (propInfo.variantType && propInfo.variantType !== 'QVariant') {
																displayText += ' [' + propInfo.variantType + ']';
														}
														if (propInfo.isWritable === false) {
																displayText += ' [read-only]';
														}
												}
												items.push({
														id: propId,
														text: displayText,
														type: propInfo && propInfo.variantType ? propInfo.variantType : 'unknown',
														isWritable: propInfo && propInfo.isWritable !== undefined ? propInfo.isWritable : true,
														currentValue: propInfo && propInfo.value !== undefined ? propInfo.value : null
												});
										});
								}
						}
						
						// ============================================================
						// TYPE: locations - Sort alphabetically
						// ============================================================
						else if (type === 'locations') {
								if (Array.isArray(data)) {
										data.forEach(function(locationName) {
												if (typeof locationName === 'string' && locationName.trim()) {
														items.push({
																id: locationName,
																text: locationName,
																name: locationName
														});
												}
										});
										items.sort(function(a, b) {
												return a.text.localeCompare(b.text);
										});
								}
						}
						
						callback(items);
						
				}).fail(function(jqXHR, textStatus, errorThrown) {
						console.error('[ApiExplorer] Failed to fetch suggestions for type:', type, errorThrown);
						
						var errorMessage = 'Error loading suggestions. Check connection.';
						if (jqXHR.status === 404) {
								errorMessage = 'Endpoint not found. Is Stellarium running?';
						} else if (jqXHR.status === 0) {
								errorMessage = 'Cannot connect to Stellarium. Is it running?';
						}
						
						callback([{ id: '', text: errorMessage }]);
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
		function renderExplorerSuggestionList($list, items, onSelect) {
				$list.empty();
				
				if (!items || items.length === 0) {
						$list.html('<div class="suggestion-empty">No results found</div>');
						return;
				}
				
				var currentCategory = null;
				
				items.forEach(function(item) {
						// Add category header when category changes (for actions type)
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
						
						// Check if we should show a sub-label (e.g., for properties with types)
						var label = '';
						if (item.type && item.type !== 'unknown' && !item.category) {
								label = ' <span style="color:#8A8C8E;font-size:9px;">[' + escapeHtml(item.type) + ']</span>';
						}
						
						var $item = $('<div class="suggestion-item" data-value="' + escapeAttr(item.id) + '">' + 
										escapeHtml(item.text) + 
										label +
										'</div>');
						
						$item.on('click', function() {
								onSelect($(this).data('value'));
						});
						
						$list.append($item);
				});
		}

		/**
		 * Update selection highlight in the dropdown list.
		 */
		function updateExplorerSelection($items, index) {
				$items.removeClass('suggestion-selected');
				if (index >= 0 && index < $items.length) {
						$items.eq(index).addClass('suggestion-selected');
						// Scroll into view
						var $selected = $items.eq(index);
						var containerTop = $selected.parent().scrollTop();
						var containerHeight = $selected.parent().height();
						var itemTop = $selected.position().top;
						var itemHeight = $selected.outerHeight();
						
						if (itemTop < 0 || itemTop + itemHeight > containerHeight) {
								$selected.parent().scrollTop(containerTop + itemTop - containerHeight/2 + itemHeight/2);
						}
				}
		}

		/**
		 * Create a suggestion button with consistent styling.
		 */
		function createExplorerSuggestionButton(text, symbol, onClick, title) {
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
    // REQUEST EXECUTION
    // =====================================================================

		/**
		 * Collect parameter values from the operation panel form.
		 * Converts ISO datetime to Julian Day for time parameter.
		 * Special handling for 'file' parameter (stored as-is for path building).
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
										var jd = isoToJDay(isoString);
										if (jd !== null) {
												value = jd;
												console.log('[ApiExplorer] Converted ISO', isoString, 'to JD', jd);
										} else {
												value = isoString;
										}
								}
								
						// Special handling for file parameter - keep as string for path building
						} else if (key === 'file') {
								value = $input.val();
								if (value && value.trim() !== '') {
										params[key] = value.trim();
								}
								return;
								
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
		 * Handles image responses, time rate conversion, and sky culture file requests.
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
				// Check if this is an image response (from planetimage etc.)
				var isImageResponse = op.responseType === 'image';
				
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
				
				// ============================================================
				// BUILD FULL PATH - Special handling for sky culture files
				// ============================================================
				var fullPath;
				var isFileRequest = false;
				var requestedFilePath = '';
				var isImageFileRequest = false;
				
				if (currentOperation === 'skyculturedescription' || currentOperation === 'landscapedescription') {
						// Check if 'file' parameter exists and is not empty
						if (serverParams.file && serverParams.file.trim() !== '') {
								requestedFilePath = serverParams.file;
								delete serverParams.file; // Remove from query params
								isFileRequest = true;
								fullPath = service.path + '/' + currentOperation + '/' + requestedFilePath;
								
								// Check if this is an image file
								var imageExtensions = ['.png', '.jpg', '.jpeg', '.gif', '.webp', '.svg', '.bmp', '.tiff'];
								var fileLower = requestedFilePath.toLowerCase();
								isImageFileRequest = imageExtensions.some(function(ext) {
										return fileLower.endsWith(ext);
								});
						} else {
								fullPath = service.path + '/' + currentOperation + '/';
						}
				} else if (op.fullPath) {
						fullPath = op.fullPath;
				} else {
						fullPath = service.path + '/' + currentOperation;
				}
				
				var baseUrl = '/api/';
				var url = baseUrl + fullPath;
				
				// ============================================================
				// HANDLE IMAGE FILE REQUESTS (sky culture illustrations)
				// ============================================================
				if (isImageFileRequest) {
						// Build the image URL with any remaining query params
						var imageUrl = url;
						var queryParams = [];
						Object.keys(serverParams).forEach(function(key) {
								queryParams.push(encodeURIComponent(key) + '=' + encodeURIComponent(String(serverParams[key])));
						});
						if (queryParams.length > 0) {
								imageUrl += '?' + queryParams.join('&');
						}
						
						// Display the image directly
						$statusEl.html('<span class="response-status success">Image: ' + escapeHtml(requestedFilePath) + '</span>');
						currentRawResponse = 'Image: ' + imageUrl;
						currentDisplayMode = 'formatted';
						
						var formattedContent = '<div class="image-response-container" style="text-align:center;padding:10px;">' +
								'<div style="margin-bottom:10px;font-size:12px;color:#FD971F;">' + escapeHtml(requestedFilePath) + '</div>' +
								'<img src="' + imageUrl + '" alt="' + escapeHtml(requestedFilePath) + '" style="max-width:100%;max-height:70vh;border:1px solid #ccc;border-radius:4px;" onerror="this.style.display=\'none\';this.parentElement.innerHTML=\'<div style=\\\'color:#888;padding:20px;\\\'><strong>Image not available</strong><br>Could not load: ' + escapeHtml(requestedFilePath) + '</div>\'">' +
								'<div style="margin-top:10px;color:#888;font-size:11px;">' +
								'URL: <a href="' + imageUrl + '" target="_blank" style="color:#66D9EF;">' + escapeHtml(imageUrl) + '</a>' +
								'</div>' +
								'</div>';
						
						currentFormattedHtml = formattedContent;
						
						$responseArea.html(
								'<div class="response-toolbar">' +
								'<div class="response-toolbar-left">' +
								'<span class="response-type-badge">IMAGE</span>' +
								'<span style="color:#8A8C8E;font-size:10px;">' + escapeHtml(requestedFilePath) + '</span>' +
								'</div>' +
								'<div class="response-toolbar-right">' +
								'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">[Show Raw URL]</button>' +
								'<button class="toggle-dir-btn jquerybutton" id="toggle-dir-btn" title="Toggle text direction for RTL languages">[Switch to RTL]</button>' +
								'<button class="copy-btn jquerybutton" id="copy-response-btn">Copy URL</button>' +
								'</div>' +
								'</div>' +
								'<div class="response-content-wrapper" style="direction:ltr;">' +
								formattedContent +
								'</div>'
						);
						
						$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
						$('#toggle-dir-btn').off('click').on('click', function() { toggleResponseDirection(); });
						$('#copy-response-btn').off('click').on('click', function() { copyResponse(); });
						
						return;
				}
				
				// ============================================================
				// HANDLE IMAGE RESPONSES (planetimage with responseType: 'image')
				// ============================================================
				if (isImageResponse) {
						var imageUrl = baseUrl + fullPath;
						var queryParams = [];
						Object.keys(serverParams).forEach(function(key) {
								queryParams.push(encodeURIComponent(key) + '=' + encodeURIComponent(String(serverParams[key])));
						});
						if (queryParams.length > 0) {
								imageUrl += '?' + queryParams.join('&');
						}
						
						$statusEl.html('<span class="response-status success">HTTP 200 OK (Image)</span>');
						currentRawResponse = 'Image: ' + imageUrl;
						currentDisplayMode = 'formatted';
						
						var formattedContent = '<div class="image-response-container" style="text-align:center;padding:10px;">' +
								'<img src="' + imageUrl + '" alt="Image" style="max-width:100%;max-height:500px;border:1px solid #ccc;border-radius:4px;" onerror="this.style.display=\'none\';this.parentElement.innerHTML=\'<div style=\\\'color:#888;padding:20px;\\\'><strong>Image not available</strong><br>Could not load image.</div>\'">' +
								'<div style="margin-top:10px;color:#888;font-size:12px;">' +
								'Image URL: <a href="' + imageUrl + '" target="_blank">' + imageUrl + '</a>' +
								'</div>' +
								'</div>';
						
						currentFormattedHtml = formattedContent;
						
						$responseArea.html(
								'<div class="response-toolbar">' +
								'<div class="response-toolbar-left">' +
								'<span class="response-type-badge">IMAGE</span>' +
								'<button class="toggle-display-btn jquerybutton" id="toggle-display-btn">[Show Raw URL]</button>' +
								'<button class="toggle-dir-btn jquerybutton" id="toggle-dir-btn" title="Toggle text direction for RTL languages">[Switch to RTL]</button>' +
								'</div>' +
								'<div class="response-toolbar-right">' +
								'<button class="copy-btn jquerybutton" id="copy-response-btn">Copy URL</button>' +
								'</div>' +
								'</div>' +
								'<div class="response-content-wrapper" style="direction:ltr;">' +
								formattedContent +
								'</div>'
						);
						
						$('#toggle-display-btn').off('click').on('click', function() { toggleResponseDisplay(); });
						$('#toggle-dir-btn').off('click').on('click', function() { toggleResponseDirection(); });
						$('#copy-response-btn').off('click').on('click', function() { copyResponse(); });
						
						return;
				}
				
				// ============================================================
				// NORMAL AJAX REQUEST (JSON/HTML/TEXT)
				// ============================================================
				var ajaxOptions = {
						url: url,
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