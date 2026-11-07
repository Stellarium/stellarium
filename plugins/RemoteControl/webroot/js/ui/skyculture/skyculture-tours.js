/* ========================================================================
 * skyculture-tours.js - Tour Script Generators (All Types)
 * ========================================================================
 * 
 * This module generates Stellarium SSC scripts that tour through the
 * objects of the current sky culture. It handles all four tour types:
 * 
 * - Constellations (static array + dynamic)
 * - Asterisms (static array)
 * - Lunar Mansions (static array)
 * - Stars (static array)
 * 
 * SCRIPT STRUCTURE:
 * -----------------
 * Generated scripts follow a fixed structure:
 * 
 * 1. Header comment (culture, options, generation info)
 * 2. Translation support (i18n.inc)
 * 3. Culture verification (switch to required culture if needed)
 * 4. Initial setup (clear view, hide GUI, disable tracking)
 * 5. Save state (either save_state.inc OR manual variable save)
 * 6. Configure appearance (colors, thickness, fades)
 * 7. Welcome message (multi-line labels)
 * 8. Object list (static array or dynamic retrieval)
 * 9. Tour configuration (DURATION, ZOOM_FOV, OVERVIEW_FOV)
 * 10. Tour execution loop
 * 11. Reset and cleanup (restore state)
 * 
 * SAVE STATE MODES:
 * -----------------
 * Each script can optionally use `save_state.inc` which provides
 * saveState()/restoreState() helpers. When checked, the manual save
 * code is skipped and replaced by:
 *   include("save_state.inc");
 *   saveState("<tour_name>_state");
 *   ... (no manual save)
 *   restoreState("<tour_name>_state");
 * 
 * @module skyculture/tours
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @requires ./skyculture-multiselect
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define([
    "jquery",
		"scripteditor/scriptEditor",
    "./skyculture-config",
    "./skyculture-state",
    "./skyculture-multiselect",
    "./skyculture-api"
], function($, ScriptEditor, Config, State, MultiSelect, Api) {
    "use strict";

		/**
		 * Cached status from /api/main/status, populated by the async
		 * wrappers (generateXxxAsync) and consumed by buildHeader and
		 * buildVisibilitySnapshot.
		 * 
		 * LIFECYCLE:
		 * ----------
		 *   1. Async wrapper (withStatus) calls Api.fetchStatus()
		 *   2. Stores the result in `_lastStatus`
		 *   3. Calls the sync generator
		 *   4. Clears `_lastStatus` in a `finally` block
		 * 
		 * CAVEAT (IMPORTANT):
		 * -------------------
		 * The sync generators (`generateConstellationsTourStatic`,
		 * `generateAsterismsTour`, etc.) read `_lastStatus` **internally**
		 * via:
		 *   - buildHeader (for the "VISIBILITY FILTER ACTIVE" metadata block)
		 *   - buildVisibilitySnapshot (for the SSC location/time locking code)
		 * 
		 * If you call a sync generator DIRECTLY (not via the async wrapper),
		 * `_lastStatus` will be `null` and the generated script will NOT
		 * include location/time metadata. The script is still valid, but it
		 * will not be reproducible across machines or across time.
		 * 
		 * ALWAYS use the async wrappers from UI code.
		 * 
		 * @type {Object|null}
		 */
		var _lastStatus = null;
		
    // =====================================================================
    // HELPERS
    // =====================================================================

    /**
     * Format a number to a fixed number of decimals, returning a string.
     * 
     * @param {number} n - Number
     * @param {number} decimals - Number of decimals
     * @returns {string}
     */
    function fmt(n, decimals) {
        return parseFloat(n).toFixed(decimals === undefined ? 2 : decimals);
    }

    /**
     * Get a UI value as a number.
     * 
     * @param {string} selector - jQuery selector
     * @param {number} fallback - Default value
     * @returns {number}
     */
    function uiNum(selector, fallback) {
        var $el = $(selector);
        if ($el && $el.length) {
            var v = parseFloat($el.val());
            if (!isNaN(v)) return v;
        }
        return fallback;
    }

    /**
     * Get a UI value as a boolean (checkbox).
     * 
     * @param {string} selector - jQuery selector
     * @param {boolean} fallback - Default value
     * @returns {boolean}
     */
    function uiBool(selector, fallback) {
        var $el = $(selector);
        if ($el && $el.length) return $el.is(':checked');
        return fallback;
    }

    /**
     * Get a UI value as an integer.
     * 
     * @param {string} selector - jQuery selector
     * @param {number} fallback - Default value
     * @returns {number}
     */
    function uiInt(selector, fallback) {
        return Math.round(uiNum(selector, fallback));
    }

    /**
     * Get a color channel value (0-100 range → 0.00-1.00 string).
     * 
     * @param {string} selector - jQuery selector
     * @param {number} fallback - Default value (0-1)
     * @returns {string}
     */
    function uiColor(selector, fallback) {
        var v = uiNum(selector, fallback * 100);
        v = Math.max(0, Math.min(100, v)) / 100;
        return fmt(v, 2);
    }

		// =====================================================================
		// CONTROL INITIALIZATION HELPERS
		// =====================================================================
		// 
		// These helpers initialize the various UI controls that appear in the
		// tour toolbars: sliders, color pickers, spinners, etc. They must be
		// called AFTER the toolbar HTML has been inserted into the DOM.
		// 
		// The goal is to ensure that:
		// 1. Every color slider has a working preview swatch
		// 2. Every "spinner" input behaves as a jQuery UI spinner
		// 3. Every control's value can be read reliably at generation time
		// =====================================================================

		/**
		 * Initialize a group of RGB color sliders with a live preview swatch.
		 *   - A live preview swatch (background-color)
		 *   - A numeric value display (R, G, B in 0.00-1.00 format)
		 *   - A dynamic section header color (text + border-left)
		 * 
		 * SLIDER VALUE SEMANTICS:
		 * -----------------------
		 * Sliders operate in the range 0-100 (integers). Internally, the values
		 * must be converted to different ranges:
		 * 
		 * - CSS preview (background-color):        0-255
		 *   Formula: Math.floor((sliderValue / 100) * 255)
		 * 
		 * - Stellarium vec3f() (SSC scripts):      0.00-1.00
		 *   Formula: sliderValue / 100
		 *   Example: sliderValue=30 → 0.30
		 * 
		 * - Section header color (text + border):  0-255 (CSS rgb)
		 *   Same conversion as the preview.
		 * 
		 * ELEMENTS EXPECTED (all inside $scope):
		 *   - #{prefix}-r, -g, -b      : the range inputs
		 *   - #{prefix}-preview         : the colored preview square
		 *   - #{prefix}-value           : the numeric value display (optional)
		 * 
		 * SECTION TARGET (optional):
		 *   The parent .tour-color-group may have data-section-id="xxx" which
		 *   points to the section header to tint (text + border-left).
		 * 
		 * @param {jQuery} $scope - Container to search within
		 * @param {string} prefix - ID prefix (e.g., 'const-lines-color')
		 */
		function initColorSliders($scope, prefix) {
				var $r       = $scope.find('#' + prefix + '-r');
				var $g       = $scope.find('#' + prefix + '-g');
				var $b       = $scope.find('#' + prefix + '-b');
				var $preview = $scope.find('#' + prefix + '-preview');
				var $value   = $scope.find('#' + prefix + '-value');
				
				if (!$r.length || !$g.length || !$b.length) {
						console.warn('[SkyCulture:Tours] Color sliders not found:', prefix);
						return;
				}
				
				// -----------------------------------------------------------------
				// Detect the target section (from data-section-id on the wrapper)
				// -----------------------------------------------------------------
				var $colorGroup = $r.closest('.tour-color-group');
				var sectionId   = $colorGroup.data('section-id') || null;
				var $section    = sectionId ? $('#' + sectionId) : $();
				
				/**
				 * Read current slider values (0-100), update:
				 *   1. The preview swatch (CSS rgb 0-255)
				 *   2. The numeric display (0.00-1.00)
				 *   3. The section header color (text + border-left)
				 */
				function updateAll() {
						// Read slider values
						var sliderR = parseFloat($r.val());
						var sliderG = parseFloat($g.val());
						var sliderB = parseFloat($b.val());
						
						// Fallbacks
						if (isNaN(sliderR)) sliderR = 0;
						if (isNaN(sliderG)) sliderG = 0;
						if (isNaN(sliderB)) sliderB = 0;
						
						// Clamp to valid range (0-100)
						sliderR = Math.max(0, Math.min(100, sliderR));
						sliderG = Math.max(0, Math.min(100, sliderG));
						sliderB = Math.max(0, Math.min(100, sliderB));
						
						// -----------------------------------------------------------------
						// 1. Convert to CSS rgb range (0-255) for preview
						// -----------------------------------------------------------------
						var cssR = Math.floor((sliderR / 100) * 255);
						var cssG = Math.floor((sliderG / 100) * 255);
						var cssB = Math.floor((sliderB / 100) * 255);
						var cssColor = 'rgb(' + cssR + ',' + cssG + ',' + cssB + ')';
						
						if ($preview.length) {
								$preview.css('background-color', cssColor);
						}
						
						// -----------------------------------------------------------------
						// 2. Convert to Stellarium vec3f range (0.00-1.00) for display
						// -----------------------------------------------------------------
						var valR = (sliderR / 100).toFixed(2);
						var valG = (sliderG / 100).toFixed(2);
						var valB = (sliderB / 100).toFixed(2);
						
						if ($value.length) {
								$value.text(valR + ', ' + valG + ', ' + valB);
						}
						
						// -----------------------------------------------------------------
						// 3. Tint the section header (text + border-left)
						// -----------------------------------------------------------------
						if ($section.length) {
								$section.css('color', cssColor);
								$section.css('border-left-color', cssColor);
						}
				}
				
				// -----------------------------------------------------------------
				// Bind events (input fires on every drag; change fires on release)
				// -----------------------------------------------------------------
				$r.off('.colorSliders').on('input.colorSliders change.colorSliders', updateAll);
				$g.off('.colorSliders').on('input.colorSliders change.colorSliders', updateAll);
				$b.off('.colorSliders').on('input.colorSliders change.colorSliders', updateAll);
				
				// Initial update to sync display with the actual slider values
				updateAll();
				
				console.log('[SkyCulture:Tours] Color sliders initialized:', prefix, {
						r: $r.val(), g: $g.val(), b: $b.val(),
						sectionId: sectionId
				});
		}

		/**
		 * Initialize a set of "spinner" inputs as jQuery UI spinners.
		 * 
		 * The inputs are expected to have IDs of the form `#{prefix}` and to
		 * carry `data-min`, `data-max`, and `data-step` attributes (or use the
		 * values passed in `options`).
		 * 
		 * @param {jQuery} $scope - Container to search within
		 * @param {string} selector - jQuery selector for the spinner inputs
		 * @param {Object} [options] - {min, max, step, numberFormat}
		 */
		function initSpinners($scope, selector, options) {
				options = options || {};
				
				$scope.find(selector).each(function() {
						var $input = $(this);
						
						// Skip if already initialized
						if ($input.data('ui-spinner')) return;
						
						// Read limits from data attributes, with fallback to options
						var min  = parseFloat($input.data('min'))  || options.min  || 0;
						var max  = parseFloat($input.data('max'))  || options.max  || 100;
						var step = parseFloat($input.data('step')) || options.step || 1;
						
						$input.spinner({
								min: min,
								max: max,
								step: step
							 // numberFormat: options.numberFormat || 'n0'
						});
				});
		}

		/**
		 * Initialize the entire tour toolbar for a given type.
		 * 
		 * This is the single entry point that every tour toolbar should call
		 * after inserting its HTML. It initializes:
		 * - All color slider groups
		 * - All spinner inputs
		 * 
		 * @param {string} type - Tour type ('constellation', 'asterism', 'lunar', 'star')
		 * @param {jQuery} $toolbar - Toolbar container (already in the DOM)
		 */
		function initTourToolbar(type, $toolbar) {
				if (!$toolbar || !$toolbar.length) return;
				
				console.log('[SkyCulture:Tours] Initializing toolbar for type:', type);
				
				// -----------------------------------------------------------------
				// Define the color groups for each type
				// -----------------------------------------------------------------
				var colorGroups = {
						constellation: [
								{ prefix: 'const-lines-color',      r: 30,  g: 60,  b: 90  },
								{ prefix: 'const-labels-color',     r: 0,   g: 50,  b: 0   },
								{ prefix: 'const-boundaries-color', r: 80,  g: 30,  b: 30  },
								{ prefix: 'const-hulls-color',      r: 100, g: 70,  b: 40  }
						],
						asterism: [
								{ prefix: 'aster-lines-color',      r: 100, g: 0,   b: 20  },
								{ prefix: 'aster-labels-color',     r: 0,   g: 100, b: 100 },
								{ prefix: 'aster-rayhelpers-color', r: 50,  g: 100, b: 50  }
						],
						lunar: [
								{ prefix: 'lunar-lines-color',      r: 50,  g: 78,  b: 50  }
						],
						star: []   // stars tour has no color pickers currently
				};
				
				// -----------------------------------------------------------------
				// Initialize all color slider groups for this type
				// -----------------------------------------------------------------
				var groups = colorGroups[type] || [];
				for (var i = 0; i < groups.length; i++) {
						initColorSliders($toolbar, groups[i].prefix, groups[i]);
				}
				
				// -----------------------------------------------------------------
				// Initialize all spinner inputs
				// -----------------------------------------------------------------
				// Tour duration, FOV, font size, and thickness inputs all use the
				// .tour-input-number class. We initialize them as spinners to
				// enable arrow buttons and step-based changes.
				// -----------------------------------------------------------------
				initSpinners($toolbar, '.tour-input-number', {
						step: 1,
						numberFormat: 'n0'
				});
				
				console.log('[SkyCulture:Tours] Toolbar initialized for type:', type);
		}

    /**
     * Get the current culture name and ID from state.
     * 
     * @returns {{name: string, id: string}}
     */
    function getCultureInfo() {
        return {
            name: State.raw.culture.name || State.raw.culture.id || 'Unknown',
            id:   State.raw.culture.id   || 'unknown'
        };
    }

    /**
     * Show a temporary notification (delegates to stelUtils if available).
     * 
     * @param {string} message - Message to display
     */
    function notify(message) {
        // Try to use stelUtils.showNotification if available via global
        if (typeof window !== 'undefined' && window._stelUtils &&
            typeof window._stelUtils.showNotification === 'function') {
            window._stelUtils.showNotification(message);
            return;
        }

        // Fallback: create a temporary DOM element
        var $n = $('<div class="tour-notification"></div>').text(message);
        $('body').append($n);
        setTimeout(function() {
            $n.fadeOut(Config.ANIMATION_DURATION, function() { $n.remove(); });
        }, Config.NOTIFICATION_DURATION);
    }

    /**
     * Copy text to clipboard.
     * 
     * @param {string} text - Text to copy
     */
    function copyToClipboard(text) {
        if (window._stelScriptEditor &&
            typeof window._stelScriptEditor.copyToClipboard === 'function') {
            window._stelScriptEditor.copyToClipboard(text);
            return;
        }

        var $ta = $('<textarea></textarea>')
            .val(text)
            .css({ position: 'fixed', left: '-9999px', top: '-9999px', opacity: 0 })
            .appendTo('body');
        $ta[0].select();
        $ta[0].setSelectionRange(0, text.length);
        try {
            document.execCommand('copy');
        } catch (e) {
            console.error('[SkyCulture:Tours] Copy failed:', e);
        }
        $ta.remove();
    }

		/**
		 * Switch to the Script Editor tab in the main UI.
		 * 
		 * Navigates through two levels of tabs:
		 * 1. Main tabs → "Actions and scripts" (#tab_actions)
		 * 2. Sub-tabs  → "Script Editor" (#sub-tab-scripts-editor)
		 * 
		 * Uses defensive checks (hasClass('ui-tabs')) to ensure the tab widgets
		 * are actually initialized before calling their API. Uses try/catch to
		 * tolerate edge cases where jQuery UI is in an inconsistent state.
		 * 
		 * After switching, triggers a CodeMirror refresh to ensure the editor
		 * renders correctly (CodeMirror requires an explicit refresh when its
		 * container transitions from hidden to visible).
		 * 
		 * @function switchToScriptEditor
		 */
				function switchToScriptEditor() {
						// Step 1: Switch to the main Actions tab (#tab_actions)
						var $mainTabs = $('#tabs');
						if ($mainTabs.length && typeof $mainTabs.tabs === 'function') {
								var actionsTabIndex = -1;
								$mainTabs.find('ul li a').each(function(idx, el) {
										if ($(el).attr('href') === '#tab_actions') {
												actionsTabIndex = idx;
												return false;
										}
								});
								if (actionsTabIndex !== -1) {
										$mainTabs.tabs('option', 'active', actionsTabIndex);
								}
						}
						
						// Step 2: Switch to the Script Editor sub-tab (first tab in actions-sub-tabs)
						var $subTabs = $('#actions-sub-tabs');
						if ($subTabs.length && typeof $subTabs.tabs === 'function') {
								$subTabs.tabs('option', 'active', 0);  // Script Editor is the first tab
						}
				}

		/**
		 * Insert generated script into the Script Editor.
		 * 
		 * Attempts to insert immediately. If ScriptEditor is not yet ready
		 * (window._stelScriptEditor unavailable), waits up to 5 seconds and
		 * retries every 250ms. Falls back to clipboard if all attempts fail.
		 * 
		 * @param {string} script - Generated script
		 * @returns {boolean} True if insertion succeeded or was scheduled
		 */
		function insertIntoEditor(script) {
				// Fast path: ScriptEditor is ready
				if (window._stelScriptEditor &&
						typeof window._stelScriptEditor.setEditorContent === 'function') {
						
						window._stelScriptEditor.setEditorContent(script);
						switchToScriptEditor();
						return true;
				}
				
				// Slow path: wait for ScriptEditor to become ready
				console.warn('[SkyCulture:Tours] ScriptEditor not ready, waiting...');
				
				var attempts = 0;
				var maxAttempts = 20;   // 20 × 250ms = 5 seconds
				
				var waitForEditor = setInterval(function() {
						attempts++;
						
						if (window._stelScriptEditor &&
								typeof window._stelScriptEditor.setEditorContent === 'function') {
								
								clearInterval(waitForEditor);
								console.log('[SkyCulture:Tours] ScriptEditor ready after ' +
														attempts + ' attempt(s)');
								
								window._stelScriptEditor.setEditorContent(script);
								switchToScriptEditor();
								return;
						}
						
						if (attempts >= maxAttempts) {
								clearInterval(waitForEditor);
								console.error('[SkyCulture:Tours] ScriptEditor not available after ' +
															maxAttempts + ' attempts; falling back to clipboard');
								
								copyToClipboard(script);
								notify('Script copied to clipboard (editor not available)');
						}
				}, 250);
				
				return true;
		}

    /**
     * Output the final script: either into the editor, or copied to clipboard.
     * 
     * @param {string} script - Generated script
     * @param {string} type - Tour type for logging
     */
    function outputScript(script, type) {
        if (insertIntoEditor(script)) {
            notify(script.length + ' bytes generated and inserted into editor');
        } else {
            copyToClipboard(script);
            notify('Script copied to clipboard');
        }
    }

    // =====================================================================
    // SHARED SCRIPT SECTIONS
    // =====================================================================

    /**
     * Build the SSC code block that LOCKS the observer's location,
     * time, and sky culture to the snapshot captured at generation time.
     * 
     * This is only emitted when the visibility filter was active,
     * because that is the ONLY case where the script's content depends
     * on the observer's position and the current time. Without locking,
     * running the script later (or elsewhere) would silently produce
     * a different — and possibly mostly empty — sky.
     * 
     * The block does THREE things:
     *   1. Pin the sky culture (so cultural names match)
     *   2. Pin the observer location (so "above horizon" is reproduced)
     *   3. Pin the time (so the celestial positions match)
     * 
     * It ALSO emits a COMMENTED-OUT block that shows how to save and
     * restore the user's original state, so the user can opt in if they
     * want to return to their pre-script settings.
     * 
     * @param {Object|null} status - Result from Api.fetchStatus()
     * @returns {string} SSC code block, or empty string if no status
     */
    function buildVisibilitySnapshot(status) {
        if (!status || !status.location || !status.time) {
            return '';
        }

        var loc = status.location;
        var t   = status.time;

        // -----------------------------------------------------------------
        // Normalize values. The API returns:
        //   - latitude / longitude as doubles (degrees)
        //   - altitude as integer (meters)
        //   - planet as string ("Earth", "Mars", …)
        //   - name as string
        //   - jday as double
        //   - utc as ISO 8601 string
        // -----------------------------------------------------------------
        var lat = Number(loc.latitude).toFixed(8);
        var lon = Number(loc.longitude).toFixed(8);
        var alt = Number(loc.altitude).toFixed(1);
        var locName = (loc.name || 'Unknown').replace(/"/g, '\\"');
        var planet = (loc.planet || 'Earth').replace(/"/g, '\\"');
        var jday = Number(t.jday).toFixed(10);
        var utc = t.utc || '';
        var isNow = t.isTimeNow === true;

        //var cultureId = State.raw.culture.id || 'modern';

        var s = '';
        s += '// ============================================================\n';
        s += '// LOCKED OBSERVER POSITION AND TIME\n';
        s += '// ------------------------------------------------------------\n';
        s += '// The visibility filter was active when this script was generated.\n';
        s += '// To reproduce the same sky, the script pins the observer location\n';
        s += '// and time to the snapshot captured at generation time.\n';
        s += '//\n';
        s += '// If you want the script to use your CURRENT location/time instead,\n';
        s += '// comment out the two "core.set…" calls below.\n';
        s += '// ============================================================\n';
        s += '\n';
        s += '// ---- Observer location ----\n';
        s += '// Snapshot: ' + locName + ' (' + planet + ')\n';
        s += '// Latitude:  ' + lat + '°  Longitude: ' + lon + '°  Altitude: ' + alt + ' m\n';
        s += 'core.setObserverLocation(\n';
        s += '    ' + lon + ',   // longitude (°E)\n';
        s += '    ' + lat + ',   // latitude (°N)\n';
        s += '    ' + alt + ',         // altitude (m)\n';
        s += '    0,                 // duration (0 = instant)\n';
        s += '    "' + locName + '",\n';
        s += '    "' + planet + '"\n';
        s += ');\n';
        s += 'core.wait(0.5);\n';
        s += '\n';
        s += '// ---- Time ----\n';
        s += '// Snapshot: ' + utc + '\n';
        s += '// Julian Day: ' + jday + '\n';
        if (isNow) {
            s += '// (Stellarium was in real-time mode at generation)\n';
        }
        s += 'core.setJDay(' + jday + ');\n';
        s += '\n';
        s += '// Optional: freeze time to keep the exact sky during the tour.\n';
        s += '// Uncomment the next line if you want time NOT to advance.\n';
        s += '// core.setTimeRate(0);\n';
        s += '\n';
        s += '// ------------------------------------------------------------\n';
        s += '// SAVE ORIGINAL POSITION (optional)\n';
        s += '// ------------------------------------------------------------\n';
        s += '// To restore your ORIGINAL location, time, and culture after\n';
        s += '// the tour ends, UNCOMMENT BOTH blocks:\n';
        s += '//   1. This block (save)\n';
        s += '//   2. The matching block near the END of this script\n';
        s += '//      (search for "RESTORE ORIGINAL POSITION")\n';
        s += '// ------------------------------------------------------------\n';
        s += '// var __sc_oldLocationId = core.getObserverLocation();\n';
        s += '// var __sc_oldJDay       = core.getJDay();\n';
        s += '// var __sc_oldCulture    = core.getSkyCulture();\n';
        s += '\n';
        s += 'core.wait(1);\n';
        s += '\n';

        return s;
    }

    /**
     * Build the "VISIBILITY FILTER ACTIVE" comment block for the header.
     * 
     * This is metadata only (all lines start with "//"). The actual
     * SSC locking code is generated separately by buildVisibilitySnapshot.
     * 
     * @param {number} filteredCount - Items in the generated array
     * @param {number} totalCount - Total items in the culture
     * @param {Object|null} status - Result from Api.fetchStatus()
     * @returns {string}
     */
    function buildVisibilityMetadataLines(filteredCount, totalCount, status) {
        var lines = [];
        lines.push('// ------------------------------------------------------------');
        lines.push('// VISIBILITY FILTER ACTIVE');
        lines.push('// ------------------------------------------------------------');
        lines.push('// Objects included: ' + filteredCount + ' / ' + totalCount);
        lines.push('// This script was generated while the "Visible above horizon only"');
        lines.push('// filter was active. Only objects above the horizon at the moment');
        lines.push('// of generation are included.');
        lines.push('//');
        lines.push('// The observer location and time have been LOCKED inside the');
        lines.push('// script body (see the "LOCKED OBSERVER POSITION" section),');
        lines.push('// so the same sky is reproduced when the script is run later.');
        lines.push('//');

        if (status && status.location) {
            var loc = status.location;
            lines.push('// Observer location: ' + (loc.name || 'Unknown') +
                       ' (' + (loc.planet || 'Earth') + ')');
            lines.push('// Latitude:  ' + Number(loc.latitude).toFixed(4) + '°');
            lines.push('// Longitude: ' + Number(loc.longitude).toFixed(4) + '°');
            lines.push('// Altitude:  ' + loc.altitude + ' m');
        }

        if (status && status.time) {
            var t = status.time;
            lines.push('//');
            lines.push('// Snapshot time (UTC): ' + t.utc);
            lines.push('// Julian Day: ' + Number(t.jday).toFixed(6));
            if (t.isTimeNow) {
                lines.push('// (Stellarium was in real-time mode at generation)');
            }
        }

        lines.push('// ------------------------------------------------------------');

        var s = '';
        for (var i = 0; i < lines.length; i++) {
            s += lines[i] + '\n';
        }
        return s;
    }

    /**
     * Generate the common script header.
     * 
     * In addition to the standard metadata, this function emits a
     * "VISIBILITY FILTER ACTIVE" block whenever the filter was enabled
     * at generation time, documenting:
     *   - Filtered object count / total count
     *   - Observer location, time, and Julian Day (from `_lastStatus`)
     *   - A warning that the sky rotates
     * 
     * @param {Object} opts - Header options
     *   - cultureName
     *   - cultureId
     *   - tourType
     *   - extraHeaderLines
     *   - type:           Object type ('constellation' | 'asterism' | 'lunar' | 'star')
     *   - filteredCount:  Number of items in the generated array
     *   - totalCount:     Total items of this type in the culture
     * @returns {string}
     */
    function buildHeader(opts) {
        var s = '';
        s += '//\n';
        s += '// ============================================================\n';
        s += '// Name: ' + opts.cultureName + ' ' + opts.tourType + '\n';
        s += '// ============================================================\n';
        s += '// Version: 1.0\n';
        s += '// Author: Stellarium Remote Control\n';
        s += '// License: Public Domain\n';
        s += '// Culture: ' + opts.cultureId + '\n';

        if (opts.extraHeaderLines && opts.extraHeaderLines.length) {
            for (var i = 0; i < opts.extraHeaderLines.length; i++) {
                s += opts.extraHeaderLines[i] + '\n';
            }
        }

        // -----------------------------------------------------------------
        // Visibility-filter metadata block (only if applicable)
        // -----------------------------------------------------------------
        if (opts.type) {
            var filterKey = opts.type === 'star' ? 'star' : opts.type;
            var fs = State.raw.filters[filterKey];
            if (fs && fs.visible && typeof opts.filteredCount === 'number') {
                s += buildVisibilityMetadataLines(
                    opts.filteredCount,
                    opts.totalCount,
                    _lastStatus
                );
            }
        }

        s += '// Generated: ' + new Date().toLocaleString() + '\n';
        s += '// ============================================================\n';
        s += '//\n';
        return s;
    }
		
    /**
     * Generate the translation support section.
     * 
     * @returns {string}
     */
    function buildTranslationSection() {
        var s = '';
        s += '// ============================================================\n';
        s += '// TRANSLATION SUPPORT\n';
        s += '// ============================================================\n';
        s += 'include("i18n.inc");\n\n';
        return s;
    }

    /**
     * Generate the culture verification section.
     * 
     * @param {string} cultureId - Required culture ID
     * @returns {string}
     */
    function buildCultureVerification(cultureId) {
        var s = '';
        s += '// ============================================================\n';
        s += '// CULTURE VERIFICATION\n';
        s += '// ============================================================\n';
        s += 'var currentCulture = core.getSkyCulture();\n';
        s += 'var requiredCulture = "' + cultureId + '";\n';
        s += 'core.debug(tr("Current culture: ") + currentCulture);\n';
        s += 'core.debug(tr("Required culture: ") + requiredCulture);\n\n';
        s += 'if (currentCulture !== requiredCulture) {\n';
        s += '    core.debug(tr("Switching to required sky culture: ") + requiredCulture);\n';
        s += '    core.setSkyCulture(requiredCulture);\n';
        s += '    core.wait(1);\n';
        s += '    core.debug(tr("Culture switched."));\n';
        s += '}\n\n';
        return s;
    }

    /**
     * Generate the initial setup section.
     * 
     * @param {boolean} hideGui - Whether to hide the GUI
     * @returns {string}
     */
    function buildInitialSetup(hideGui) {
        var s = '';
        s += '// ============================================================\n';
        s += '// INITIAL SETUP\n';
        s += '// ============================================================\n';
        s += 'core.clear("starchart");\n\n';
        s += 'GridLinesMgr.setFlagGridlines(false);\n\n';
        if (hideGui) {
            s += 'core.setGuiVisible(false);\n\n';
        }
        s += 'StelMovementMgr.setFlagTracking(false);\n\n';
        return s;
    }

    /**
     * Generate the welcome message block.
     * 
     * @param {string} cultureName - Culture name
     * @param {string} msg3Content - Content of the third message
     * @param {string} msg3Raw - Raw msg3 expression (if not translatable)
     * @returns {string}
     */
    function buildWelcomeMessage(cultureName, msg3Content, msg3Raw) {
        var s = '';
        s += '// ============================================================\n';
        s += '// WELCOME MESSAGE\n';
        s += '// ============================================================\n';
        s += 'var msg1 = tr("Welcome to the ");\n';
        s += 'var msg2 = "' + cultureName + '" + tr(" tour!");\n';
        if (msg3Raw) {
            s += 'var msg3 = ' + msg3Raw + ';\n';
        } else {
            s += 'var msg3 = tr("' + msg3Content + '");\n';
        }
        s += 'var msg4 = tr("Press Ctrl+T to show toolbar at any time.");\n\n';
        s += 'var labelY1 = 100;\n';
        s += 'var labelY2 = 125;\n';
        s += 'var labelY3 = 150;\n';
        s += 'var labelX = 100;\n';
        s += 'var fontSize = 16;\n';
        s += 'var color = "#ffaa66";\n\n';
        s += 'var label1 = LabelMgr.labelScreen(msg1 + msg2, labelX, labelY1, false, fontSize, color);\n';
        s += 'var label2 = LabelMgr.labelScreen(msg3, labelX, labelY2, false, fontSize, color);\n';
        s += 'var label3 = LabelMgr.labelScreen(msg4, labelX, labelY3, false, fontSize, color);\n\n';
        s += 'LabelMgr.setLabelShow(label1, true);\n';
        s += 'LabelMgr.setLabelShow(label2, true);\n';
        s += 'LabelMgr.setLabelShow(label3, true);\n';
        s += 'core.wait(6);\n';
        s += 'LabelMgr.deleteAllLabels();\n\n';
        return s;
    }

		/**
		 * Generate the tour execution loop skeleton (common to all types).
		 * 
		 * IMPORTANT: The generated SSC script uses `tr("Visiting " + itemLabel)` 
		 * pattern where itemLabel is inserted as a literal string. This allows
		 * the translation system to work correctly.
		 * 
		 * Example generated line (for constellations):
		 *   core.debug(tr("Visiting constellation") + " " + currentNum + "/" + constellations.length + ": " + constName);
		 * 
		 * @param {Object} opts - Options with:
		 *   - arrayName: name of the array variable
		 *   - itemLabel: literal string label (e.g. "constellation")
		 *   - selectFunction: e.g. "core.selectConstellationByName"
		 *   - moveToFunction: e.g. "core.moveToObject"
		 *   - deselectFunction: optional
		 * @returns {string}
		 */
		function buildTourLoop(opts) {
				var s = '';
				s += 'for (var i = 0; i < ' + opts.arrayName + '.length; i++) {\n';
				s += '    var constName = ' + opts.arrayName + '[i];\n';
				s += '    var currentNum = i + 1;\n';
				s += '    \n';
				s += '    core.debug(tr("Visiting ' + opts.itemLabel + '") + " " + currentNum + "/" + ' + opts.arrayName + '.length + ": " + constName);\n';
				s += '    \n';
				s += '    ' + opts.selectFunction + '(constName);\n';
				s += '    \n';
				s += '    ' + opts.moveToFunction + '(constName, 2);\n';
				s += '    core.wait(1);\n';
				s += '    \n';
				s += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
				s += '    core.wait(1);\n';
				s += '    \n';
				s += '    var nameLabel = LabelMgr.labelScreen(constName, 50, 50, false, 16, "#ffaa66");\n';
				s += '    LabelMgr.setLabelShow(nameLabel, true);\n';
				s += '    core.wait(DURATION);\n';
				s += '    LabelMgr.deleteLabel(nameLabel);\n';
				s += '    \n';
				if (opts.deselectFunction) {
						s += '    ' + opts.deselectFunction + '();\n';
						s += '    \n';
				}
				s += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
				s += '    core.wait(1);\n';
				s += '}\n\n';
				return s;
		}

    /**
     * Generate the reset and cleanup footer.
     * 
     * @param {Object} opts - Options
     * @returns {string}
     */
    function buildResetAndCleanup(opts) {
        var s = '';
        s += '// ============================================================\n';
        s += '// RESET AND CLEANUP\n';
        s += '// ============================================================\n';
        s += 'core.debug(tr("Tour completed! Restoring original settings..."));\n\n';

        if (opts.useSaveState) {
            s += '// Restore state using save_state.inc\n';
            s += 'restoreState("' + opts.stateName + '");\n\n';
        } else if (opts.restoreLines && opts.restoreLines.length) {
            for (var i = 0; i < opts.restoreLines.length; i++) {
                s += opts.restoreLines[i] + '\n';
            }
            s += '\n';
        }

        if (opts.extraCleanup && opts.extraCleanup.length) {
            for (var j = 0; j < opts.extraCleanup.length; j++) {
                s += opts.extraCleanup[j] + '\n';
            }
            s += '\n';
        }

        s += '// Restore grids\n';
        s += 'GridLinesMgr.setFlagGridlines(true);\n\n';
        s += '// Show GUI again\n';
        s += 'core.setGuiVisible(true);\n\n';
        s += '// «natural» Clear the display options: azimuthal mount, atmosphere, landscape, no lines, labels or markers\n';
        s += 'core.clear("natural");\n\n';
        s += '// Reset view  position to Fov 100, Look South, Horizon 40 deg\n';
        s += 'StelMovementMgr.zoomTo(100, 3);\n';
        s += 'core.moveToAltAzi(40.0, 180.0, 3.0);\n\n';
        s += 'core.debug(tr("Tour completed successfully!"));\n';
        // -----------------------------------------------------------------
        // Optional restore block (commented out by default)
        // -----------------------------------------------------------------
        // Only emit this block when the visibility filter was active,
        // because that is the ONLY case where we emitted a save block
        // at the top. Emitting a restore block without a save block
        // would reference undefined variables (if uncommented).
        // -----------------------------------------------------------------
        if (opts.visibilityFilterActive) {
            s += '\n';
            s += buildRestoreOriginalPosition();
        }
				
        return s;
    }

    /**
     * Build the "RESTORE ORIGINAL POSITION" block for the END of the
     * script. Pairs with the optional save block emitted by
     * buildVisibilitySnapshot at the beginning.
     * 
     * The block is COMMENTED OUT by default. The user uncomments BOTH
     * the save block (at the top) and this restore block (at the end)
     * to enable automatic restoration of their original location,
     * time, and sky culture.
     * 
     * The restoration uses `duration = 1` for setObserverLocation so
     * the transition is smooth (not jarring), and restores JDay and
     * sky culture immediately.
     * 
     * @returns {string} SSC code block (with commented-out restore code)
     */
    function buildRestoreOriginalPosition() {
        var s = '';
        s += '// ============================================================\n';
        s += '// RESTORE ORIGINAL POSITION (optional)\n';
        s += '// ------------------------------------------------------------\n';
        s += '// This block pairs with the "SAVE ORIGINAL POSITION" block\n';
        s += '// near the TOP of this script.\n';
        s += '//\n';
        s += '// To enable automatic restoration of your original location,\n';
        s += '// time, and sky culture after the tour ends:\n';
        s += '//   1. Uncomment the SAVE block at the top of this script\n';
        s += '//   2. Uncomment THIS block (the RESTORE block)\n';
        s += '//\n';
        s += '// If both blocks are commented out (default), the script leaves\n';
        s += '// Stellarium at the locked snapshot (Paris, 2026-10-20 01:57 UTC).\n';
        s += '// ============================================================\n';
        s += '//\n';
        s += '// core.debug(tr("Restoring original location, time, and culture..."));\n';
        s += '//\n';
        s += '// // Restore sky culture first (so UI labels match)\n';
        s += '// core.setSkyCulture(__sc_oldCulture);\n';
        s += '// core.wait(0.5);\n';
        s += '//\n';
        s += '// // Restore observer location (smooth 1-second transition)\n';
        s += '// core.setObserverLocation(__sc_oldLocationId, 1);\n';
        s += '// core.wait(1);\n';
        s += '//\n';
        s += '// // Restore simulation time\n';
        s += '// core.setJDay(__sc_oldJDay);\n';
        s += '//\n';
        s += '// core.debug(tr("Original state restored."));\n';
        s += '\n';
        return s;
    }
		
    // =====================================================================
    // CONSTELLATIONS TOUR
    // =====================================================================

    /**
     * Generate the Constellations Tour script.
     * 
     * @returns {string|null} Generated script, or null if no data
     */
    function generateConstellationsTourStatic() {
        // Determine which constellations to use
        var isMultiSelect = MultiSelect.isActive('constellation') &&
                            MultiSelect.getSelectedSet('constellation').size > 0;

        /*var constellationsToUse = isMultiSelect ?
            MultiSelect.getSelectedPatterns('constellation') :
            State.raw.data.constellations; */
						
        // Respect the visibility filter: use `displayed` (which is
        // already filtered + sorted) when not in multi-select mode.
        var constellationsToUse;
        if (isMultiSelect) {
            constellationsToUse = MultiSelect.getSelectedPatterns('constellation');
        } else {
            var displayed = State.raw.displayed && State.raw.displayed.constellations;
            constellationsToUse = (displayed && displayed.length)
                ? displayed
                : State.raw.data.constellations;
        }

        if (!constellationsToUse || !constellationsToUse.length) {
            notify('No constellations available');
            return null;
        }

        var total         = constellationsToUse.length;
        var totalAvail    = State.raw.data.constellations.length;
        var cultureInfo   = getCultureInfo();

        // Read UI options
        var duration       = uiNum('#const-tour-duration', 4);
        var isolate        = uiBool('#const-tour-isolate', true);
        var pickLastOnly   = uiBool('#const-tour-pick-last', false);
        var zoomFov        = uiNum('#const-zoom-fov', 40);
        var overviewFov    = uiNum('#const-overview-fov', 90);
        var fontSize       = uiInt('#const-font-size', 16);
        var showLines      = uiBool('#const-show-lines', true);
        var lineThickness  = uiInt('#const-line-thickness', 2);
        var bndThickness   = uiInt('#const-boundaries-thickness', 1);
        var hullThickness  = uiInt('#const-hulls-thickness', 1);
        var showArt        = uiBool('#const-show-art', true);
        var showBoundaries = uiBool('#const-show-boundaries', false);
        var showHulls      = uiBool('#const-show-hulls', false);
        var artIntensity   = (uiNum('#const-art-intensity', 80) / 100).toFixed(2);
        var artFade        = uiNum('#const-art-fade', 2.0);
        var linesFade      = uiNum('#const-lines-fade', 1.0);
        var labelsFade     = uiNum('#const-labels-fade', 1.0);
        var bndFade        = uiNum('#const-boundaries-fade', 1.0);
        var hullFade       = uiNum('#const-hulls-fade', 1.0);

        var linesColorR = uiColor('#const-lines-color-r', 0.30);
        var linesColorG = uiColor('#const-lines-color-g', 0.60);
        var linesColorB = uiColor('#const-lines-color-b', 0.90);
        var labelsColorR = uiColor('#const-labels-color-r', 0.00);
        var labelsColorG = uiColor('#const-labels-color-g', 0.50);
        var labelsColorB = uiColor('#const-labels-color-b', 0.00);
        var bndColorR = uiColor('#const-boundaries-color-r', 0.80);
        var bndColorG = uiColor('#const-boundaries-color-g', 0.30);
        var bndColorB = uiColor('#const-boundaries-color-b', 0.30);
        var hullColorR = uiColor('#const-hulls-color-r', 1.00);
        var hullColorG = uiColor('#const-hulls-color-g', 0.70);
        var hullColorB = uiColor('#const-hulls-color-b', 0.40);

        var useSaveState = uiBool('#const-tour-use-savestate', false);

        // Build selection note for header
        var extraHeader = [];
        if (isMultiSelect) {
            extraHeader.push('// Selected Constellations: ' + total + ' out of ' + totalAvail);
            extraHeader.push('// This script includes only the constellations manually selected by the user.');
        } else {
            extraHeader.push('// Total Constellations: ' + total);
        }
        extraHeader.push('// Duration per Constellation: ' + duration + ' seconds');
        extraHeader.push('// Zoom FOV: ' + zoomFov + ' degrees');
        extraHeader.push('// Overview FOV: ' + overviewFov + ' degrees');
        extraHeader.push('// Select Last Only: ' + (pickLastOnly ? 'ON' : 'OFF'));
        extraHeader.push('// Font Size: ' + fontSize + ' pixels');
        extraHeader.push('// Show Lines: ' + (showLines ? 'ON' : 'OFF'));
        extraHeader.push('// Line Thickness: ' + lineThickness + ' pixels');
        extraHeader.push('// Boundaries Thickness: ' + bndThickness + ' pixels');
        extraHeader.push('// Hulls Thickness: ' + hullThickness + ' pixels');
        extraHeader.push('// Isolate Mode: ' + (isolate ? 'ON' : 'OFF'));
        extraHeader.push('// Show Artwork: ' + (showArt ? 'ON' : 'OFF'));
        extraHeader.push('// Show Boundaries: ' + (showBoundaries ? 'ON' : 'OFF'));
        extraHeader.push('// Show Hulls: ' + (showHulls ? 'ON' : 'OFF'));
        extraHeader.push('// Art Intensity: ' + artIntensity);
        extraHeader.push('// Art Fade Duration: ' + artFade + ' seconds');
        extraHeader.push('// Lines Fade Duration: ' + linesFade + ' seconds');
        extraHeader.push('// Labels Fade Duration: ' + labelsFade + ' seconds');
        extraHeader.push('// Boundaries Fade Duration: ' + bndFade + ' seconds');
        extraHeader.push('// Hulls Fade Duration: ' + hullFade + ' seconds');
        extraHeader.push('// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')');
        extraHeader.push('// Labels Color: RGB(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ')');
        extraHeader.push('// Boundaries Color: RGB(' + bndColorR + ', ' + bndColorG + ', ' + bndColorB + ')');
        extraHeader.push('// Hulls Color: RGB(' + hullColorR + ', ' + hullColorG + ', ' + hullColorB + ')');
        extraHeader.push('// Use save_state.inc: ' + (useSaveState ? 'YES' : 'NO'));

        // Assemble script
        /*var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Constellations Tour (Static Array)',
            extraHeaderLines: extraHeader
        });*/

        // Build header WITH type/filteredCount/totalCount so visibility
        // metadata can be emitted automatically.
        var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Constellations Tour (Static Array)',
            extraHeaderLines: extraHeader,
            type:            'constellation',
            filteredCount:   total,
            totalCount:      totalAvail
        });

        s += buildTranslationSection();
        s += buildCultureVerification(cultureInfo.id);
				
        // Lock observer position/time/culture if visibility was active
        var filterState = State.raw.filters.constellation;
        var visibilityActive = filterState && filterState.visible;
        if (visibilityActive) {
            s += buildVisibilitySnapshot(_lastStatus);
        }
				
        s += buildInitialSetup(true);

        // Save state
        if (useSaveState) {
            s += '// ============================================================\n';
            s += '// SAVE_STATE.INC - Preserve and restore display state\n';
            s += '// ============================================================\n';
            s += 'include("save_state.inc");\n\n';
            s += 'saveState("constellation_tour_static_state");\n\n';
        } else {
            s += '// ============================================================\n';
            s += '// SAVE ORIGINAL SETTINGS (MANUALLY)\n';
            s += '// ============================================================\n';
            s += 'var oldArtState = ConstellationMgr.getFlagArt();\n';
            s += 'var oldBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
            s += 'var oldHullsState = ConstellationMgr.getFlagHulls();\n';
            s += 'var oldLinesState = ConstellationMgr.getFlagLines();\n';
            s += 'var oldLabelsState = ConstellationMgr.getFlagLabels();\n';
            s += 'var oldIsolateState = ConstellationMgr.getFlagIsolateSelected();\n';
            s += 'var oldPickLastState = ConstellationMgr.getFlagConstellationPick();\n';
            s += 'var oldFontSize = ConstellationMgr.getFontSize();\n';
            s += 'var oldLineThickness = ConstellationMgr.getConstellationLineThickness();\n';
            s += 'var oldBoundariesThickness = ConstellationMgr.getBoundariesThickness();\n';
            s += 'var oldHullsThickness = ConstellationMgr.getHullsThickness();\n';
            s += 'var oldArtIntensity = ConstellationMgr.getArtIntensity();\n';
            s += 'var oldLinesColor = ConstellationMgr.getLinesColor();\n';
            s += 'var oldLabelsColor = ConstellationMgr.getLabelsColor();\n';
            s += 'var oldBoundariesColor = ConstellationMgr.getBoundariesColor();\n';
            s += 'var oldHullsColor = ConstellationMgr.getHullsColor();\n';
            s += 'var oldLinesFade = ConstellationMgr.getLinesFadeDuration();\n';
            s += 'var oldLabelsFade = ConstellationMgr.getLabelsFadeDuration();\n';
            s += 'var oldBoundariesFade = ConstellationMgr.getBoundariesFadeDuration();\n';
            s += 'var oldHullsFade = ConstellationMgr.getHullsFadeDuration();\n';
            s += 'var oldArtFade = ConstellationMgr.getArtFadeDuration();\n\n';
        }

        // Configure appearance
        s += '// ============================================================\n';
        s += '// CONFIGURE APPEARANCE\n';
        s += '// ============================================================\n';
        s += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        s += 'ConstellationMgr.setLinesColor(linesColor);\n\n';
        s += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
        s += 'ConstellationMgr.setLabelsColor(labelsColor);\n\n';
        s += 'var boundariesColor = core.vec3f(' + bndColorR + ', ' + bndColorG + ', ' + bndColorB + ');\n';
        s += 'ConstellationMgr.setBoundariesColor(boundariesColor);\n\n';
        s += 'var hullsColor = core.vec3f(' + hullColorR + ', ' + hullColorG + ', ' + hullColorB + ');\n';
        s += 'ConstellationMgr.setHullsColor(hullsColor);\n\n';
        s += 'ConstellationMgr.setFontSize(' + fontSize + ');\n';
        s += 'ConstellationMgr.setConstellationLineThickness(' + lineThickness + ');\n';
        s += 'ConstellationMgr.setBoundariesThickness(' + bndThickness + ');\n';
        s += 'ConstellationMgr.setHullsThickness(' + hullThickness + ');\n\n';
        s += 'ConstellationMgr.setArtIntensity(' + artIntensity + ');\n';
        s += 'ConstellationMgr.setArtFadeDuration(' + artFade + ');\n';
        s += 'ConstellationMgr.setLinesFadeDuration(' + linesFade + ');\n';
        s += 'ConstellationMgr.setLabelsFadeDuration(' + labelsFade + ');\n';
        s += 'ConstellationMgr.setBoundariesFadeDuration(' + bndFade + ');\n';
        s += 'ConstellationMgr.setHullsFadeDuration(' + hullFade + ');\n\n';
        s += 'ConstellationMgr.setFlagArt(' + (showArt ? 'true' : 'false') + ');\n';
        s += 'ConstellationMgr.setFlagBoundaries(' + (showBoundaries ? 'true' : 'false') + ');\n';
        s += 'ConstellationMgr.setFlagHulls(' + (showHulls ? 'true' : 'false') + ');\n';
        s += 'ConstellationMgr.setFlagLines(' + (showLines ? 'true' : 'false') + ');\n';
        s += 'ConstellationMgr.setFlagLabels(true);\n';
        s += 'ConstellationMgr.setFlagIsolateSelected(' + (isolate ? 'true' : 'false') + ');\n\n';
        s += 'ConstellationMgr.setFlagConstellationPick(' + (pickLastOnly ? 'true' : 'false') + ');\n\n';

        // Welcome message
        var msg3Raw = isMultiSelect ?
            'tr("Showing ") + ' + total + ' + tr(" of ") + ' + totalAvail + ' + tr(" selected constellations.")' :
            'tr("Exploring celestial patterns across the sky.")';
        s += buildWelcomeMessage(cultureInfo.name, null, msg3Raw);

        // Constellation array
        s += '// ============================================================\n';
        s += '// CONSTELLATION LIST (Static Array)\n';
        s += '// ============================================================\n';
        s += '// This array contains ' + total + ' constellation names\n';
        s += '// extracted from the ' + cultureInfo.name + ' sky culture data.\n';
        s += 'var constellations = [\n';
        for (var i = 0; i < constellationsToUse.length; i++) {
            var c = constellationsToUse[i];
            var name = (c.searchName || c.name || c.id).replace(/"/g, '\\"');
            var comment = '';
            if (c.englishName && c.englishName !== name) comment = ' // ' + c.englishName;
            else if (c.nativeName && c.nativeName !== name) comment = ' // ' + c.nativeName;

            s += '    "' + name + '"' + (i < constellationsToUse.length - 1 ? ',' : '') + comment + '\n';
        }
        s += '];\n\n';

        // Tour configuration
        s += '// ============================================================\n';
        s += '// TOUR CONFIGURATION\n';
        s += '// ============================================================\n';
        s += 'var DURATION = ' + duration + ';  // Seconds per constellation\n';
        s += 'var ZOOM_FOV = ' + zoomFov + ';  // Field of view when zoomed in (degrees)\n';
        s += 'var OVERVIEW_FOV = ' + overviewFov + ';  // Field of view for overview\n\n';

        // Tour execution
        s += '// ============================================================\n';
        s += '// TOUR EXECUTION\n';
        s += '// ============================================================\n';
        s += 'core.debug(tr("Starting constellations tour..."));\n';
        s += 'core.debug(tr("Total constellations in array: ") + constellations.length);\n\n';
        s += buildTourLoop({
            arrayName:        'constellations',
            itemLabel:        'constellation',
            selectFunction:   'core.selectConstellationByName',
            moveToFunction:   'core.moveToObject',
            deselectFunction: null
        });

        // Reset and cleanup
        var restoreLines = null;
        if (!useSaveState) {
            restoreLines = [
                'ConstellationMgr.deselectConstellations();',
                'ConstellationMgr.setFlagArt(oldArtState);',
                'ConstellationMgr.setFlagBoundaries(oldBoundariesState);',
                'ConstellationMgr.setFlagHulls(oldHullsState);',
                'ConstellationMgr.setFlagLines(oldLinesState);',
                'ConstellationMgr.setFlagLabels(oldLabelsState);',
                'ConstellationMgr.setFlagIsolateSelected(oldIsolateState);',
                'ConstellationMgr.setFlagConstellationPick(oldPickLastState);',
                '',
                'ConstellationMgr.setFontSize(oldFontSize);',
                'ConstellationMgr.setConstellationLineThickness(oldLineThickness);',
                'ConstellationMgr.setBoundariesThickness(oldBoundariesThickness);',
                'ConstellationMgr.setHullsThickness(oldHullsThickness);',
                'ConstellationMgr.setArtIntensity(oldArtIntensity);',
                'ConstellationMgr.setLinesColor(oldLinesColor);',
                'ConstellationMgr.setLabelsColor(oldLabelsColor);',
                'ConstellationMgr.setBoundariesColor(oldBoundariesColor);',
                'ConstellationMgr.setHullsColor(oldHullsColor);',
                'ConstellationMgr.setLinesFadeDuration(oldLinesFade);',
                'ConstellationMgr.setLabelsFadeDuration(oldLabelsFade);',
                'ConstellationMgr.setBoundariesFadeDuration(oldBoundariesFade);',
                'ConstellationMgr.setHullsFadeDuration(oldHullsFade);',
                'ConstellationMgr.setArtFadeDuration(oldArtFade);'
            ];
        }
        s += buildResetAndCleanup({
            useSaveState: useSaveState,
            stateName:    'constellation_tour_static_state',
            restoreLines: restoreLines,
            visibilityFilterActive: visibilityActive						
        });

        return s;
    }

		/**
		 * Generate a complete SSC script for constellation tour using DYNAMIC
		 * constellation retrieval.
		 * 
		 * Unlike the static version, this script fetches the constellation list
		 * at runtime from Stellarium's ConstellationMgr.getConstellationsEnglishNames().
		 * It therefore works with any sky culture without embedding names.
		 * 
		 * Features:
		 * - Dynamic constellation list (no hardcoded array)
		 * - Automatic translation support via i18n.inc
		 * - Smooth zoom transitions
		 * - Constellation art display during tour
		 * - GUI hiding for immersive experience
		 * - Proper state restoration
		 * 
		 * @returns {string|null} Generated script, or null if no data
		 * @function generateConstellationsTourDynamic
		 */
		function generateConstellationsTourDynamic() {
				// Count available constellations (for validation only)
				var totalConstellations = (State.raw.data.constellations || []).length;
				if (totalConstellations === 0) {
						notify('No constellations available for this culture');
						return null;
				}
				
				var cultureInfo = getCultureInfo();
				
				// Read UI options (identical to static version)
				var duration        = uiNum('#const-tour-duration', 4);
				var isolate         = uiBool('#const-tour-isolate', true);
				var pickLastOnly    = uiBool('#const-tour-pick-last', false);
				var zoomFov         = uiNum('#const-zoom-fov', 40);
				var overviewFov     = uiNum('#const-overview-fov', 90);
				var fontSize        = uiInt('#const-font-size', 16);
				var showLines       = uiBool('#const-show-lines', true);
				var lineThickness   = uiInt('#const-line-thickness', 2);
				var bndThickness    = uiInt('#const-boundaries-thickness', 1);
				var hullThickness   = uiInt('#const-hulls-thickness', 1);
				var showArt         = uiBool('#const-show-art', true);
				var showBoundaries  = uiBool('#const-show-boundaries', false);
				var showHulls       = uiBool('#const-show-hulls', false);
				var artIntensity    = (uiNum('#const-art-intensity', 80) / 100).toFixed(2);
				var artFade         = uiNum('#const-art-fade', 2.0);
				var linesFade       = uiNum('#const-lines-fade', 1.0);
				var labelsFade      = uiNum('#const-labels-fade', 1.0);
				var bndFade         = uiNum('#const-boundaries-fade', 1.0);
				var hullFade        = uiNum('#const-hulls-fade', 1.0);
				
				var linesColorR = uiColor('#const-lines-color-r', 0.30);
				var linesColorG = uiColor('#const-lines-color-g', 0.60);
				var linesColorB = uiColor('#const-lines-color-b', 0.90);
				var labelsColorR = uiColor('#const-labels-color-r', 0.00);
				var labelsColorG = uiColor('#const-labels-color-g', 0.50);
				var labelsColorB = uiColor('#const-labels-color-b', 0.00);
				var bndColorR = uiColor('#const-boundaries-color-r', 0.80);
				var bndColorG = uiColor('#const-boundaries-color-g', 0.30);
				var bndColorB = uiColor('#const-boundaries-color-b', 0.30);
				var hullColorR = uiColor('#const-hulls-color-r', 1.00);
				var hullColorG = uiColor('#const-hulls-color-g', 0.70);
				var hullColorB = uiColor('#const-hulls-color-b', 0.40);
				
				var useSaveState = uiBool('#const-tour-use-savestate', false);
				
				// Build header comments
				var extraHeader = [];
				extraHeader.push('// Total Constellations: ' + totalConstellations + ' (retrieved dynamically)');
				extraHeader.push('// Duration per Constellation: ' + duration + ' seconds');
				extraHeader.push('// Zoom FOV: ' + zoomFov + ' degrees');
				extraHeader.push('// Overview FOV: ' + overviewFov + ' degrees');
				extraHeader.push('// Select Last Only: ' + (pickLastOnly ? 'ON' : 'OFF'));
				extraHeader.push('// Font Size: ' + fontSize + ' pixels');
				extraHeader.push('// Show Lines: ' + (showLines ? 'ON' : 'OFF'));
				extraHeader.push('// Line Thickness: ' + lineThickness + ' pixels');
				extraHeader.push('// Boundaries Thickness: ' + bndThickness + ' pixels');
				extraHeader.push('// Hulls Thickness: ' + hullThickness + ' pixels');
				extraHeader.push('// Isolate Mode: ' + (isolate ? 'ON' : 'OFF'));
				extraHeader.push('// Show Artwork: ' + (showArt ? 'ON' : 'OFF'));
				extraHeader.push('// Show Boundaries: ' + (showBoundaries ? 'ON' : 'OFF'));
				extraHeader.push('// Show Hulls: ' + (showHulls ? 'ON' : 'OFF'));
				extraHeader.push('// Art Intensity: ' + artIntensity);
				extraHeader.push('// Art Fade Duration: ' + artFade + ' seconds');
				extraHeader.push('// Lines Fade Duration: ' + linesFade + ' seconds');
				extraHeader.push('// Labels Fade Duration: ' + labelsFade + ' seconds');
				extraHeader.push('// Boundaries Fade Duration: ' + bndFade + ' seconds');
				extraHeader.push('// Hulls Fade Duration: ' + hullFade + ' seconds');
				extraHeader.push('// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')');
				extraHeader.push('// Labels Color: RGB(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ')');
				extraHeader.push('// Boundaries Color: RGB(' + bndColorR + ', ' + bndColorG + ', ' + bndColorB + ')');
				extraHeader.push('// Hulls Color: RGB(' + hullColorR + ', ' + hullColorG + ', ' + hullColorB + ')');
				extraHeader.push('// Use save_state.inc: ' + (useSaveState ? 'YES' : 'NO'));
				
				// If the visibility filter is acttive add Note here
				if (filterState && filterState.visible) {
						extraHeader.push('// NOTE: The visibility filter is active in the UI, but this');
						extraHeader.push('// dynamic script does NOT apply it. It includes ALL');
						extraHeader.push('// constellations. Use the Static variant for filtering.');
				}
				
				// Assemble script
				var s = buildHeader({
						cultureName: cultureInfo.name,
						cultureId:   cultureInfo.id,
						tourType:    'Constellations Tour (Dynamic)',
						extraHeaderLines: extraHeader
				});
				
				s += buildTranslationSection();
				s += buildCultureVerification(cultureInfo.id);
				s += buildInitialSetup(true);
				
				// Save state
				if (useSaveState) {
						s += '// ============================================================\n';
						s += '// SAVE_STATE.INC - Preserve and restore display state\n';
						s += '// ============================================================\n';
						s += 'include("save_state.inc");\n\n';
						s += 'saveState("constellation_tour_dynamic_state");\n\n';
				} else {
						s += '// ============================================================\n';
						s += '// SAVE ORIGINAL SETTINGS (MANUALLY)\n';
						s += '// ============================================================\n';
						s += 'var oldArtState = ConstellationMgr.getFlagArt();\n';
						s += 'var oldBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
						s += 'var oldHullsState = ConstellationMgr.getFlagHulls();\n';
						s += 'var oldLinesState = ConstellationMgr.getFlagLines();\n';
						s += 'var oldLabelsState = ConstellationMgr.getFlagLabels();\n';
						s += 'var oldIsolateState = ConstellationMgr.getFlagIsolateSelected();\n';
						s += 'var oldPickLastState = ConstellationMgr.getFlagConstellationPick();\n';
						s += 'var oldFontSize = ConstellationMgr.getFontSize();\n';
						s += 'var oldLineThickness = ConstellationMgr.getConstellationLineThickness();\n';
						s += 'var oldBoundariesThickness = ConstellationMgr.getBoundariesThickness();\n';
						s += 'var oldHullsThickness = ConstellationMgr.getHullsThickness();\n';
						s += 'var oldArtIntensity = ConstellationMgr.getArtIntensity();\n';
						s += 'var oldLinesColor = ConstellationMgr.getLinesColor();\n';
						s += 'var oldLabelsColor = ConstellationMgr.getLabelsColor();\n';
						s += 'var oldBoundariesColor = ConstellationMgr.getBoundariesColor();\n';
						s += 'var oldHullsColor = ConstellationMgr.getHullsColor();\n';
						s += 'var oldLinesFade = ConstellationMgr.getLinesFadeDuration();\n';
						s += 'var oldLabelsFade = ConstellationMgr.getLabelsFadeDuration();\n';
						s += 'var oldBoundariesFade = ConstellationMgr.getBoundariesFadeDuration();\n';
						s += 'var oldHullsFade = ConstellationMgr.getHullsFadeDuration();\n';
						s += 'var oldArtFade = ConstellationMgr.getArtFadeDuration();\n\n';
				}
				
				// Configure appearance
				s += '// ============================================================\n';
				s += '// CONFIGURE APPEARANCE\n';
				s += '// ============================================================\n';
				s += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
				s += 'ConstellationMgr.setLinesColor(linesColor);\n\n';
				s += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
				s += 'ConstellationMgr.setLabelsColor(labelsColor);\n\n';
				s += 'var boundariesColor = core.vec3f(' + bndColorR + ', ' + bndColorG + ', ' + bndColorB + ');\n';
				s += 'ConstellationMgr.setBoundariesColor(boundariesColor);\n\n';
				s += 'var hullsColor = core.vec3f(' + hullColorR + ', ' + hullColorG + ', ' + hullColorB + ');\n';
				s += 'ConstellationMgr.setHullsColor(hullsColor);\n\n';
				s += 'ConstellationMgr.setFontSize(' + fontSize + ');\n';
				s += 'ConstellationMgr.setConstellationLineThickness(' + lineThickness + ');\n';
				s += 'ConstellationMgr.setBoundariesThickness(' + bndThickness + ');\n';
				s += 'ConstellationMgr.setHullsThickness(' + hullThickness + ');\n\n';
				s += 'ConstellationMgr.setArtIntensity(' + artIntensity + ');\n';
				s += 'ConstellationMgr.setArtFadeDuration(' + artFade + ');\n';
				s += 'ConstellationMgr.setLinesFadeDuration(' + linesFade + ');\n';
				s += 'ConstellationMgr.setLabelsFadeDuration(' + labelsFade + ');\n';
				s += 'ConstellationMgr.setBoundariesFadeDuration(' + bndFade + ');\n';
				s += 'ConstellationMgr.setHullsFadeDuration(' + hullFade + ');\n\n';
				s += 'ConstellationMgr.setFlagArt(' + (showArt ? 'true' : 'false') + ');\n';
				s += 'ConstellationMgr.setFlagBoundaries(' + (showBoundaries ? 'true' : 'false') + ');\n';
				s += 'ConstellationMgr.setFlagHulls(' + (showHulls ? 'true' : 'false') + ');\n';
				s += 'ConstellationMgr.setFlagLines(' + (showLines ? 'true' : 'false') + ');\n';
				s += 'ConstellationMgr.setFlagLabels(true);\n';
				s += 'ConstellationMgr.setFlagIsolateSelected(' + (isolate ? 'true' : 'false') + ');\n\n';
				s += 'ConstellationMgr.setFlagConstellationPick(' + (pickLastOnly ? 'true' : 'false') + ');\n\n';
				
				// Welcome message
				s += buildWelcomeMessage(cultureInfo.name, null,
						'tr("This script dynamically fetches constellation names.")');
				
				// Dynamic constellation retrieval
				s += '// ============================================================\n';
				s += '// GET CONSTELLATION LIST (DYNAMIC)\n';
				s += '// ============================================================\n';
				s += 'var constellations = ConstellationMgr.getConstellationsEnglishNames();\n';
				s += 'core.debug(tr("Total constellations found: ") + constellations.length);\n\n';
				
				// Tour configuration
				s += '// ============================================================\n';
				s += '// TOUR CONFIGURATION\n';
				s += '// ============================================================\n';
				s += 'var DURATION = ' + duration + ';\n';
				s += 'var ZOOM_FOV = ' + zoomFov + ';\n';
				s += 'var OVERVIEW_FOV = ' + overviewFov + ';\n\n';
				
				// Tour execution (same loop as static)
				s += '// ============================================================\n';
				s += '// TOUR EXECUTION\n';
				s += '// ============================================================\n';
				s += 'core.debug(tr("Starting constellations tour..."));\n';
				s += 'core.debug(tr("Total constellations in array: ") + constellations.length);\n\n';
				s += buildTourLoop({
						arrayName:        'constellations',
						itemLabel:        'constellation',
						selectFunction:   'core.selectConstellationByName',
						moveToFunction:   'core.moveToObject',
						deselectFunction: null
				});
				
				// Reset and cleanup
				var restoreLines = null;
				if (!useSaveState) {
						restoreLines = [
								'ConstellationMgr.deselectConstellations();',
								'ConstellationMgr.setFlagArt(oldArtState);',
								'ConstellationMgr.setFlagBoundaries(oldBoundariesState);',
								'ConstellationMgr.setFlagHulls(oldHullsState);',
								'ConstellationMgr.setFlagLines(oldLinesState);',
								'ConstellationMgr.setFlagLabels(oldLabelsState);',
								'ConstellationMgr.setFlagIsolateSelected(oldIsolateState);',
								'ConstellationMgr.setFlagConstellationPick(oldPickLastState);',
								'',
								'ConstellationMgr.setFontSize(oldFontSize);',
								'ConstellationMgr.setConstellationLineThickness(oldLineThickness);',
								'ConstellationMgr.setBoundariesThickness(oldBoundariesThickness);',
								'ConstellationMgr.setHullsThickness(oldHullsThickness);',
								'ConstellationMgr.setArtIntensity(oldArtIntensity);',
								'ConstellationMgr.setLinesColor(oldLinesColor);',
								'ConstellationMgr.setLabelsColor(oldLabelsColor);',
								'ConstellationMgr.setBoundariesColor(oldBoundariesColor);',
								'ConstellationMgr.setHullsColor(oldHullsColor);',
								'ConstellationMgr.setLinesFadeDuration(oldLinesFade);',
								'ConstellationMgr.setLabelsFadeDuration(oldLabelsFade);',
								'ConstellationMgr.setBoundariesFadeDuration(oldBoundariesFade);',
								'ConstellationMgr.setHullsFadeDuration(oldHullsFade);',
								'ConstellationMgr.setArtFadeDuration(oldArtFade);'
						];
				}
				s += buildResetAndCleanup({
						useSaveState: useSaveState,
						stateName:    'constellation_tour_dynamic_state',
						restoreLines: restoreLines,
            visibilityFilterActive: false   // not used in dynamic tour
				});
				
				return s;
		}

    // =====================================================================
    // ASTERISMS TOUR
    // =====================================================================

    /**
     * Generate the Asterisms Tour script.
     * 
     * @returns {string|null}
     */
    function generateAsterismsTour() {
        var isMultiSelect = MultiSelect.isActive('asterism') &&
                            MultiSelect.getSelectedSet('asterism').size > 0;

        /*var asterismsToUse = isMultiSelect ?
            MultiSelect.getSelectedPatterns('asterism') :
            State.raw.data.asterisms;*/
						
        // Respect the visibility filter: use `displayed` (which is
        // already filtered + sorted) when not in multi-select mode.
        var asterismsToUse;
        if (isMultiSelect) {
            asterismsToUse = MultiSelect.getSelectedPatterns('asterism');
        } else {
            var displayed = State.raw.displayed && State.raw.displayed.asterisms;
            asterismsToUse = (displayed && displayed.length)
                ? displayed
                : State.raw.data.asterisms;
        }

        if (!asterismsToUse || !asterismsToUse.length) {
            notify('No asterisms available');
            return null;
        }

        var total      = asterismsToUse.length;
        var totalAvail = State.raw.data.asterisms.length;
        var cultureInfo = getCultureInfo();

        // Read UI options
        var duration        = uiNum('#aster-tour-duration', 3);
        var isolate         = uiBool('#aster-tour-isolate', true);
        var showRayHelpers  = uiBool('#aster-show-rayhelpers', true);
        var zoomFov         = uiNum('#aster-zoom-fov', 25);
        var overviewFov     = uiNum('#aster-overview-fov', 60);
        var fontSize        = uiInt('#aster-font-size', 16);
        var lineThickness   = uiInt('#aster-line-thickness', 2);
        var rayThickness    = uiInt('#aster-rayhelper-thickness', 1);
        var linesFade       = uiNum('#aster-lines-fade', 1.0);
        var labelsFade      = uiNum('#aster-labels-fade', 1.0);
        var rayHelpersFade  = uiNum('#aster-rayhelpers-fade', 1.0);

        var linesColorR = uiColor('#aster-lines-color-r', 1.00);
        var linesColorG = uiColor('#aster-lines-color-g', 0.00);
        var linesColorB = uiColor('#aster-lines-color-b', 0.20);
        var labelsColorR = uiColor('#aster-labels-color-r', 0.00);
        var labelsColorG = uiColor('#aster-labels-color-g', 1.00);
        var labelsColorB = uiColor('#aster-labels-color-b', 1.00);
        var rayColorR = uiColor('#aster-rayhelpers-color-r', 0.50);
        var rayColorG = uiColor('#aster-rayhelpers-color-g', 1.00);
        var rayColorB = uiColor('#aster-rayhelpers-color-b', 0.50);

        var useSaveState = uiBool('#aster-tour-use-savestate', false);

        // Header extras
        var extraHeader = [];
        if (isMultiSelect) {
            extraHeader.push('// Selected Asterisms: ' + total + ' out of ' + totalAvail);
            extraHeader.push('// This script includes only the asterisms manually selected by the user.');
        }
        extraHeader.push('// Total Asterisms in this script: ' + total);
        extraHeader.push('// Duration per Asterism: ' + duration + ' seconds');
        extraHeader.push('// Zoom FOV: ' + zoomFov + ' degrees');
        extraHeader.push('// Overview FOV: ' + overviewFov + ' degrees');
        extraHeader.push('// Font Size: ' + fontSize + ' pixels');
        extraHeader.push('// Line Thickness: ' + lineThickness + ' pixels');
        extraHeader.push('// Ray Helper Thickness: ' + rayThickness + ' pixels');
        extraHeader.push('// Isolate Mode: ' + (isolate ? 'ON' : 'OFF'));
        extraHeader.push('// Show Ray Helpers: ' + (showRayHelpers ? 'ON' : 'OFF'));
        extraHeader.push('// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')');
        extraHeader.push('// Labels Color: RGB(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ')');
        extraHeader.push('// Ray Helpers Color: RGB(' + rayColorR + ', ' + rayColorG + ', ' + rayColorB + ')');
        extraHeader.push('// Use save_state.inc: ' + (useSaveState ? 'YES' : 'NO'));

        var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Asterisms Tour (Static Array)',
            extraHeaderLines: extraHeader,
            type:            'asterism',
            filteredCount:   total,
            totalCount:      totalAvail						
        });

        s += buildTranslationSection();
        s += buildCultureVerification(cultureInfo.id);
				
        // Lock observer position/time/culture if visibility was active
        var filterState = State.raw.filters.asterism;
        var visibilityActive = filterState && filterState.visible;
        if (visibilityActive) {
            s += buildVisibilitySnapshot(_lastStatus);
        }
				
        s += buildInitialSetup(true);

        // Save state
        if (useSaveState) {
            s += '// ============================================================\n';
            s += '// SAVE_STATE.INC - Preserve and restore display state\n';
            s += '// ============================================================\n';
            s += 'include("save_state.inc");\n\n';
            s += 'saveState("asterism_tour_static_state");\n\n';
        } else {
            s += '// ============================================================\n';
            s += '// SAVE ORIGINAL SETTINGS (MANUALLY)\n';
            s += '// ============================================================\n';
            s += 'var oldLinesState = AsterismMgr.getFlagLines();\n';
            s += 'var oldLabelsState = AsterismMgr.getFlagLabels();\n';
            s += 'var oldRayHelpersState = AsterismMgr.getFlagRayHelpers();\n';
            s += 'var oldIsolateState = AsterismMgr.getFlagIsolateAsterismSelected();\n';
            s += 'var oldLineThickness = AsterismMgr.getAsterismLineThickness();\n';
            s += 'var oldRayHelperThickness = AsterismMgr.getRayHelperThickness();\n';
            s += 'var oldFontSize = AsterismMgr.getFontSize();\n';
            s += 'var oldLinesColor = AsterismMgr.getLinesColor();\n';
            s += 'var oldLabelsColor = AsterismMgr.getLabelsColor();\n';
            s += 'var oldRayHelpersColor = AsterismMgr.getRayHelpersColor();\n';
            s += 'var oldLinesFade = AsterismMgr.getLinesFadeDuration();\n';
            s += 'var oldLabelsFade = AsterismMgr.getLabelsFadeDuration();\n';
            s += 'var oldRayHelpersFade = AsterismMgr.getRayHelpersFadeDuration();\n\n';
        }

        // Configure appearance
        s += '// ============================================================\n';
        s += '// CONFIGURE APPEARANCE\n';
        s += '// ============================================================\n';
        s += 'var linesColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        s += 'AsterismMgr.setLinesColor(linesColor);\n\n';
        s += 'var labelsColor = core.vec3f(' + labelsColorR + ', ' + labelsColorG + ', ' + labelsColorB + ');\n';
        s += 'AsterismMgr.setLabelsColor(labelsColor);\n\n';
        s += 'var rayHelpersColor = core.vec3f(' + rayColorR + ', ' + rayColorG + ', ' + rayColorB + ');\n';
        s += 'AsterismMgr.setRayHelpersColor(rayHelpersColor);\n\n';
        s += 'AsterismMgr.setFontSize(' + fontSize + ');\n';
        s += 'AsterismMgr.setAsterismLineThickness(' + lineThickness + ');\n';
        s += 'AsterismMgr.setRayHelperThickness(' + rayThickness + ');\n\n';
        s += 'AsterismMgr.setLinesFadeDuration(' + linesFade + ');\n';
        s += 'AsterismMgr.setLabelsFadeDuration(' + labelsFade + ');\n';
        s += 'AsterismMgr.setRayHelpersFadeDuration(' + rayHelpersFade + ');\n\n';
        s += 'AsterismMgr.setFlagLines(true);\n';
        s += 'AsterismMgr.setFlagLabels(true);\n';
        s += 'AsterismMgr.setFlagRayHelpers(' + (showRayHelpers ? 'true' : 'false') + ');\n';
        s += 'AsterismMgr.setFlagIsolateAsterismSelected(' + (isolate ? 'true' : 'false') + ');\n\n';

        // Disable constellations
        s += 'ConstellationMgr.deselectConstellations();\n';
        s += 'ConstellationMgr.setFlagArt(false);\n';
        s += 'ConstellationMgr.setFlagBoundaries(false);\n';
        s += 'ConstellationMgr.setFlagLines(false);\n';
        s += 'ConstellationMgr.setFlagLabels(false);\n\n';

        // Welcome message
        var msg3Raw = isMultiSelect ?
            'tr("Showing ") + ' + total + ' + tr(" of ") + ' + totalAvail + ' + tr(" selected asterisms.")' :
            'tr("Exploring celestial patterns across the sky.")';
        s += buildWelcomeMessage(cultureInfo.name, null, msg3Raw);

        // Asterism array
        s += '// ============================================================\n';
        s += '// ASTERISM LIST (Static Array)\n';
        s += '// ============================================================\n';
        s += '// This array contains ' + total + ' asterism names\n';
        s += '// extracted from the ' + cultureInfo.name + ' sky culture data.\n';
        s += 'var asterisms = [\n';
        for (var i = 0; i < asterismsToUse.length; i++) {
            var a = asterismsToUse[i];
            var name = (a.searchName || a.name || a.id).replace(/"/g, '\\"');
            var comment = '';
            if (a.englishName && a.englishName !== name) comment = ' // ' + a.englishName;
            else if (a.nativeName && a.nativeName !== name) comment = ' // ' + a.nativeName;

            s += '    "' + name + '"' + (i < asterismsToUse.length - 1 ? ',' : '') + comment + '\n';
        }
        s += '];\n\n';

        // Tour configuration
        s += '// ============================================================\n';
        s += '// TOUR CONFIGURATION\n';
        s += '// ============================================================\n';
        s += 'var DURATION = ' + duration + ';\n';
        s += 'var ZOOM_FOV = ' + zoomFov + ';\n';
        s += 'var OVERVIEW_FOV = ' + overviewFov + ';\n\n';

        // Tour execution
        s += '// ============================================================\n';
        s += '// TOUR EXECUTION\n';
        s += '// ============================================================\n';
        s += 'core.debug(tr("Starting asterisms tour..."));\n';
        s += 'core.debug(tr("Total asterisms in array: ") + asterisms.length);\n\n';
        s += buildTourLoop({
            arrayName:        'asterisms',
            itemLabel:        'asterism',
            selectFunction:   'core.selectObjectByName',
            moveToFunction:   'core.moveToObject',
            deselectFunction: 'AsterismMgr.deselectAsterisms'
        });

        // Reset and cleanup
        var restoreLines = null;
        if (!useSaveState) {
            restoreLines = [
                'AsterismMgr.deselectAsterisms();',
                'AsterismMgr.setFlagLines(oldLinesState);',
                'AsterismMgr.setFlagLabels(oldLabelsState);',
                'AsterismMgr.setFlagRayHelpers(oldRayHelpersState);',
                'AsterismMgr.setFlagIsolateAsterismSelected(oldIsolateState);',
                'AsterismMgr.setAsterismLineThickness(oldLineThickness);',
                'AsterismMgr.setRayHelperThickness(oldRayHelperThickness);',
                'AsterismMgr.setFontSize(oldFontSize);',
                'AsterismMgr.setLinesColor(oldLinesColor);',
                'AsterismMgr.setLabelsColor(oldLabelsColor);',
                'AsterismMgr.setRayHelpersColor(oldRayHelpersColor);',
                'AsterismMgr.setLinesFadeDuration(oldLinesFade);',
                'AsterismMgr.setLabelsFadeDuration(oldLabelsFade);',
                'AsterismMgr.setRayHelpersFadeDuration(oldRayHelpersFade);'
            ];
        }
        s += buildResetAndCleanup({
            useSaveState: useSaveState,
            stateName:    'asterism_tour_static_state',
            restoreLines: restoreLines,
            visibilityFilterActive: visibilityActive						
        });

        return s;
    }

    // =====================================================================
    // LUNAR MANSIONS TOUR
    // =====================================================================

    /**
     * Generate the Lunar Mansions Tour script.
     * 
     * @returns {string|null}
     */
    function generateLunarTour() {
        var isMultiSelect = MultiSelect.isActive('lunar') &&
                            MultiSelect.getSelectedSet('lunar').size > 0;

        /*var mansionsToUse = isMultiSelect ?
            MultiSelect.getSelectedPatterns('lunar') :
            State.raw.data.lunar;*/

        // Respect the visibility filter: use `displayed` (which is
        // already filtered + sorted) when not in multi-select mode.
        var mansionsToUse;
        if (isMultiSelect) {
            mansionsToUse = MultiSelect.getSelectedPatterns('lunar');
        } else {
            var displayed = State.raw.displayed && State.raw.displayed.lunar;
            mansionsToUse = (displayed && displayed.length)
                ? displayed
                : State.raw.data.lunar;
        }
				
        if (!mansionsToUse || !mansionsToUse.length) {
            notify('No lunar mansions available');
            return null;
        }

        var total      = mansionsToUse.length;
        var totalAvail = State.raw.data.lunar.length;
        var cultureInfo = getCultureInfo();

        var duration      = uiNum('#lunar-tour-duration', 4);
        var zoomFov       = uiNum('#lunar-zoom-fov', 30);
        var overviewFov   = uiNum('#lunar-overview-fov', 90);
        var fontSize      = uiInt('#lunar-font-size', 16);
        var showLabels    = uiBool('#lunar-show-labels', true);
        var showLines     = uiBool('#lunar-show-lines', true);
        var lineThickness = uiInt('#lunar-line-thickness', 2);

        var linesColorR = uiColor('#lunar-lines-color-r', 0.50);
        var linesColorG = uiColor('#lunar-lines-color-g', 0.78);
        var linesColorB = uiColor('#lunar-lines-color-b', 0.50);

        var extraHeader = [];
        if (isMultiSelect) {
            extraHeader.push('// Selected Lunar Mansions: ' + total + ' out of ' + totalAvail);
        }
        extraHeader.push('// Total Lunar Mansions: ' + total);
        extraHeader.push('// Duration per Mansion: ' + duration + ' seconds');
        extraHeader.push('// Zoom FOV: ' + zoomFov + ' degrees');
        extraHeader.push('// Overview FOV: ' + overviewFov + ' degrees');
        extraHeader.push('// Font Size: ' + fontSize + ' pixels');
        extraHeader.push('// Show Labels: ' + (showLabels ? 'ON' : 'OFF'));
        extraHeader.push('// Show Lines: ' + (showLines ? 'ON' : 'OFF'));
        extraHeader.push('// Lines Color: RGB(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ')');
        extraHeader.push('// Lines Thickness: ' + lineThickness);

				// Build header WITH type/filteredCount/totalCount so visibility
        // metadata can be emitted automatically.
        var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Lunar Mansions Tour',
            extraHeaderLines: extraHeader,
            type:            'lunar',
            filteredCount:   total,
            totalCount:      totalAvail
        });

        s += buildTranslationSection();
        s += buildCultureVerification(cultureInfo.id);
				
        // Lock observer position/time/culture if visibility was active
        var filterState = State.raw.filters.lunar;
        var visibilityActive = filterState && filterState.visible;
        if (visibilityActive) {
            s += buildVisibilitySnapshot(_lastStatus);
        }				
				
        s += buildInitialSetup(true);

        // Save original settings
        s += '// ============================================================\n';
        s += '// SAVE ORIGINAL SETTINGS\n';
        s += '// ============================================================\n';
        s += 'var oldLunarState = ConstellationMgr.getFlagLunarSystem();\n';
        s += 'var oldLunarColor = ConstellationMgr.getLunarSystemColor();\n';
        s += 'var oldLunarThickness = ConstellationMgr.getLunarSystemThickness();\n\n';
        s += 'var oldConstLinesState = ConstellationMgr.getFlagLines();\n';
        s += 'var oldConstBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
        s += 'var oldConstLabelsState = ConstellationMgr.getFlagLabels();\n';
        s += 'var oldConstArtState = ConstellationMgr.getFlagArt();\n\n';
        s += 'var oldAsterLinesState = AsterismMgr.getFlagLines();\n';
        s += 'var oldAsterLabelsState = AsterismMgr.getFlagLabels();\n\n';

        // Disable distractions
        s += '// ============================================================\n';
        s += '// DISABLE DISTRACTING ELEMENTS\n';
        s += '// ============================================================\n';
        s += 'ConstellationMgr.deselectConstellations();\n';
        s += 'ConstellationMgr.setFlagArt(false);\n';
        s += 'ConstellationMgr.setFlagBoundaries(false);\n';
        s += 'ConstellationMgr.setFlagLines(false);\n';
        s += 'ConstellationMgr.setFlagLabels(false);\n\n';
        s += 'AsterismMgr.setFlagLines(false);\n';
        s += 'AsterismMgr.setFlagLabels(false);\n\n';

        // Configure lunar system
        s += '// ============================================================\n';
        s += '// CONFIGURE LUNAR SYSTEM APPEARANCE\n';
        s += '// ============================================================\n';
        s += 'ConstellationMgr.setFlagLunarSystem(' + (showLines ? 'true' : 'false') + ');\n';
        s += 'var lunarColor = core.vec3f(' + linesColorR + ', ' + linesColorG + ', ' + linesColorB + ');\n';
        s += 'ConstellationMgr.setLunarSystemColor(lunarColor);\n';
        s += 'ConstellationMgr.setLunarSystemThickness(' + lineThickness + ');\n\n';

        // Welcome message
        var msg3Raw = isMultiSelect ?
            'tr("Showing ") + ' + total + ' + tr(" of ") + ' + totalAvail + ' + tr(" selected lunar mansions.")' :
            'tr("Lunar mansions are divisions of the Moon\'s path across the sky.")';
        s += buildWelcomeMessage(cultureInfo.name, null, msg3Raw);

        // Lunar mansions array
        s += '// ============================================================\n';
        s += '// LUNAR MANSIONS LIST (Static Array)\n';
        s += '// ============================================================\n';
        s += '// This array contains ' + total + ' lunar mansion names\n';
        s += '// extracted from the ' + cultureInfo.name + ' sky culture data.\n';
        s += 'var lunarMansions = [\n';
				
				// Diagnostic debugger for the stars array map
				console.log('[Diagnostic] mansionsToUse order:', mansionsToUse.map(s => ({
						id: s.id,
						vmag: s._info ? s._info.vmag : 'no-info',
						name: s.name
				})));				
				
        for (var i = 0; i < mansionsToUse.length; i++) {
            var m = mansionsToUse[i];
            var name = (m.searchName || m.name || m.id).replace(/"/g, '\\"');
            var comment = '';
            if (m.englishName && m.englishName !== name) comment = ' // ' + m.englishName;
            else if (m.nativeName && m.nativeName !== name) comment = ' // ' + m.nativeName;

            s += '    "' + name + '"' + (i < mansionsToUse.length - 1 ? ',' : '') + comment + '\n';
        }
        s += '];\n\n';

        // Tour configuration
        s += '// ============================================================\n';
        s += '// TOUR CONFIGURATION\n';
        s += '// ============================================================\n';
        s += 'var DURATION = ' + duration + ';\n';
        s += 'var ZOOM_FOV = ' + zoomFov + ';\n';
        s += 'var OVERVIEW_FOV = ' + overviewFov + ';\n';
        s += 'var SHOW_LABELS = ' + (showLabels ? 'true' : 'false') + ';\n';
        s += 'var LABEL_FONT_SIZE = ' + fontSize + ';\n\n';

        // Tour execution (custom because of SHOW_LABELS conditional)
        s += '// ============================================================\n';
        s += '// TOUR EXECUTION\n';
        s += '// ============================================================\n';
        s += 'core.debug(tr("Starting lunar mansions tour..."));\n';
        s += 'core.debug(tr("Total lunar mansions in array: ") + lunarMansions.length);\n\n';
        s += 'for (var i = 0; i < lunarMansions.length; i++) {\n';
        s += '    var mansionName = lunarMansions[i];\n';
        s += '    var currentNum = i + 1;\n';
        s += '    \n';
        s += '    core.debug(tr("Visiting lunar mansion") + " " + currentNum + "/" + lunarMansions.length + ": " + mansionName);\n';
        s += '    \n';
        s += '    core.moveToObject(mansionName, 2);\n';
        s += '    core.wait(1);\n';
        s += '    \n';
        s += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        s += '    core.wait(1);\n';
        s += '    \n';
        s += '    var nameLabel = null;\n';
        s += '    if (SHOW_LABELS) {\n';
        s += '        nameLabel = LabelMgr.labelScreen(mansionName, 50, 50, false, LABEL_FONT_SIZE, "#ffaa66");\n';
        s += '        LabelMgr.setLabelShow(nameLabel, true);\n';
        s += '    }\n';
        s += '    core.wait(DURATION);\n';
        s += '    if (nameLabel !== null) {\n';
        s += '        LabelMgr.deleteLabel(nameLabel);\n';
        s += '    }\n';
        s += '    \n';
        s += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        s += '    core.wait(1);\n';
        s += '}\n\n';

        // Reset and cleanup
        s += buildResetAndCleanup({
            useSaveState: false,
            stateName:    null,
            restoreLines: [
                'ConstellationMgr.setFlagLunarSystem(oldLunarState);',
                'ConstellationMgr.setLunarSystemColor(oldLunarColor);',
                'ConstellationMgr.setLunarSystemThickness(oldLunarThickness);',
                '',
                'ConstellationMgr.setFlagLines(oldConstLinesState);',
                'ConstellationMgr.setFlagBoundaries(oldConstBoundariesState);',
                'ConstellationMgr.setFlagLabels(oldConstLabelsState);',
                'ConstellationMgr.setFlagArt(oldConstArtState);',
                '',
                'AsterismMgr.setFlagLines(oldAsterLinesState);',
                'AsterismMgr.setFlagLabels(oldAsterLabelsState);'
            ],
            visibilityFilterActive: visibilityActive						
        });

        return s;
    }

    // =====================================================================
    // STARS TOUR
    // =====================================================================

    /**
     * Generate the Stars Tour script.
     * 
     * @returns {string|null}
     */
    function generateStarsTour() {
        var isMultiSelect = MultiSelect.isActive('star') &&
                            MultiSelect.getSelectedSet('star').size > 0;

				/*var starsToUse = isMultiSelect ?
            MultiSelect.getSelectedPatterns('star') :
            State.raw.data.stars;
						*/

        // ✅ Respect the visibility filter: use `displayed` (which is
        // already filtered + sorted) when not in multi-select mode.
        var starsToUse;
        if (isMultiSelect) {
            starsToUse = MultiSelect.getSelectedPatterns('star');
        } else {
            var displayed = State.raw.displayed && State.raw.displayed.stars;
            starsToUse = (displayed && displayed.length)
                ? displayed
                : State.raw.data.stars;
        }

        if (!starsToUse || !starsToUse.length) {
            notify('No stars available');
            return null;
        }

        var total      = starsToUse.length;
        var totalAvail = State.raw.data.stars.length;
        var cultureInfo = getCultureInfo();

        var duration        = uiNum('#stars-tour-duration', 3);
        var zoomFov         = uiNum('#stars-zoom-fov', 10);
        var overviewFov     = uiNum('#stars-overview-fov', 60);
        var fontSize        = uiInt('#stars-font-size', 16);
        var showLabels      = uiBool('#stars-show-labels', true);
        var labelsAmount    = uiNum('#stars-labels-amount', 6.0);
        var showAddNames    = uiBool('#stars-show-additional-names', true);
        var showDblStars    = uiBool('#stars-show-dbl-stars', false);
        var showVarStars    = uiBool('#stars-show-var-stars', false);
        var showHIPNumbers  = uiBool('#stars-show-hip-numbers', false);
        var absScale        = uiNum('#stars-absolute-scale', 1.0);
        var relScale        = uiNum('#stars-relative-scale', 1.0);
        var showTwinkle     = uiBool('#stars-show-twinkle', true);
        var twinkleAmount   = uiNum('#stars-twinkle-amount', 0.3);
        var showSpiky       = uiBool('#stars-show-spiky', false);
        var showHalo        = uiBool('#stars-show-halo', true);
        var useMagLimit     = uiBool('#stars-use-mag-limit', false);
        var customMagLimit  = uiNum('#stars-mag-limit', 8.0);
        var useSaveState    = uiBool('#stars-tour-use-savestate', false);

        // Build header extras
        var extraHeader = [];
        if (isMultiSelect) {
            extraHeader.push('// Selected Stars/Objects: ' + total + ' out of ' + totalAvail);
        }
        extraHeader.push('// Total Objects in this script: ' + total);
        extraHeader.push('// Duration per Object: ' + duration + ' seconds');
        extraHeader.push('// Zoom FOV: ' + zoomFov + ' degrees');
        extraHeader.push('// Overview FOV: ' + overviewFov + ' degrees');
        extraHeader.push('// Font Size: ' + fontSize + ' pixels');
        extraHeader.push('// Use save_state.inc: ' + (useSaveState ? 'YES' : 'NO'));

        /*var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Stars and Celestial Objects Tour',
            extraHeaderLines: extraHeader
        });*/
				
				// Build header WITH type/filteredCount/totalCount so visibility
        // metadata can be emitted automatically.
        var s = buildHeader({
            cultureName: cultureInfo.name,
            cultureId:   cultureInfo.id,
            tourType:    'Stars and Celestial Objects Tour',
            extraHeaderLines: extraHeader,
            type:            'star',
            filteredCount:   total,
            totalCount:      totalAvail
        });

        s += buildTranslationSection();
        s += buildCultureVerification(cultureInfo.id);
				
        // Lock observer position/time/culture if visibility was active
        var filterState = State.raw.filters.star;
        var visibilityActive = filterState && filterState.visible;

        if (visibilityActive) {
            s += buildVisibilitySnapshot(_lastStatus);
        }
				
        s += buildInitialSetup(true);

        // Save state
        if (useSaveState) {
            s += '// ============================================================\n';
            s += '// SAVE_STATE.INC - Preserve and restore display state\n';
            s += '// ============================================================\n';
            s += 'include("save_state.inc");\n\n';
            s += 'saveState("stars_tour_static_state");\n\n';
        } else {
            s += '// ============================================================\n';
            s += '// SAVE ORIGINAL SETTINGS (MANUALLY)\n';
            s += '// ============================================================\n';
            s += 'var oldStarsState = StarMgr.getFlagStars();\n';
            s += 'var oldLabelsState = StarMgr.getFlagLabels();\n';
            s += 'var oldLabelsAmount = StarMgr.getLabelsAmount();\n';
            s += 'var oldAdditionalNames = StarMgr.getFlagAdditionalNames();\n';
            s += 'var oldDblStars = StarMgr.getFlagDblStarsDesignation();\n';
            s += 'var oldVarStars = StarMgr.getFlagVarStarsDesignation();\n';
            s += 'var oldHIPDesignation = StarMgr.getFlagHIPDesignation();\n\n';
            s += 'var oldAbsoluteScale = StelSkyDrawer.getAbsoluteStarScale();\n';
            s += 'var oldRelativeScale = StelSkyDrawer.getRelativeStarScale();\n';
            s += 'var oldTwinkleFlag = StelSkyDrawer.getFlagTwinkle();\n';
            s += 'var oldTwinkleAmount = StelSkyDrawer.getTwinkleAmount();\n';
            s += 'var oldSpikyFlag = StelSkyDrawer.getFlagStarSpiky();\n';
            s += 'var oldHaloFlag = StelSkyDrawer.getFlagDrawBigStarHalo();\n';
            s += 'var oldMagLimitFlag = StelSkyDrawer.getFlagStarMagnitudeLimit();\n';
            s += 'var oldCustomMagLimit = StelSkyDrawer.getCustomStarMagnitudeLimit();\n\n';
            s += 'var oldConstLinesState = ConstellationMgr.getFlagLines();\n';
            s += 'var oldConstLabelsState = ConstellationMgr.getFlagLabels();\n';
            s += 'var oldConstArtState = ConstellationMgr.getFlagArt();\n';
            s += 'var oldConstBoundariesState = ConstellationMgr.getFlagBoundaries();\n';
            s += 'var oldAsterLinesState = AsterismMgr.getFlagLines();\n';
            s += 'var oldAsterLabelsState = AsterismMgr.getFlagLabels();\n';
            s += 'var oldLunarState = ConstellationMgr.getFlagLunarSystem();\n\n';
        }

        // Disable distractions
        s += '// ============================================================\n';
        s += '// DISABLE DISTRACTING ELEMENTS\n';
        s += '// ============================================================\n';
        s += 'ConstellationMgr.deselectConstellations();\n';
        s += 'ConstellationMgr.setFlagArt(false);\n';
        s += 'ConstellationMgr.setFlagBoundaries(false);\n';
        s += 'ConstellationMgr.setFlagLines(false);\n';
        s += 'ConstellationMgr.setFlagLabels(false);\n\n';
        s += 'AsterismMgr.setFlagLines(false);\n';
        s += 'AsterismMgr.setFlagLabels(false);\n\n';
        s += 'ConstellationMgr.setFlagLunarSystem(false);\n\n';
        s += 'GridLinesMgr.setFlagGridlines(false);\n\n';

        // Configure star display
        s += '// ============================================================\n';
        s += '// CONFIGURE STAR DISPLAY\n';
        s += '// ============================================================\n';
        s += 'StarMgr.setFlagStars(true);\n\n';
        s += 'StarMgr.setFlagLabels(' + (showLabels ? 'true' : 'false') + ');\n';
        s += 'StarMgr.setLabelsAmount(' + labelsAmount + ');\n';
        s += 'StarMgr.setFlagAdditionalNames(' + (showAddNames ? 'true' : 'false') + ');\n';
        s += 'StarMgr.setFlagDblStarsDesignation(' + (showDblStars ? 'true' : 'false') + ');\n';
        s += 'StarMgr.setFlagVarStarsDesignation(' + (showVarStars ? 'true' : 'false') + ');\n';
        s += 'StarMgr.setFlagHIPDesignation(' + (showHIPNumbers ? 'true' : 'false') + ');\n\n';
        s += 'StelSkyDrawer.setAbsoluteStarScale(' + absScale + ');\n';
        s += 'StelSkyDrawer.setRelativeStarScale(' + relScale + ');\n';
        s += 'StelSkyDrawer.setFlagTwinkle(' + (showTwinkle ? 'true' : 'false') + ');\n';
        s += 'StelSkyDrawer.setTwinkleAmount(' + twinkleAmount + ');\n';
        s += 'StelSkyDrawer.setFlagStarSpiky(' + (showSpiky ? 'true' : 'false') + ');\n';
        s += 'StelSkyDrawer.setFlagDrawBigStarHalo(' + (showHalo ? 'true' : 'false') + ');\n';
        s += 'StelSkyDrawer.setFlagStarMagnitudeLimit(' + (useMagLimit ? 'true' : 'false') + ');\n';
        if (useMagLimit) {
            s += 'StelSkyDrawer.setCustomStarMagnitudeLimit(' + customMagLimit + ');\n';
        }
        s += 'StarMgr.setFontSize(' + fontSize + ');\n\n';

        // Welcome message
        var msg3Raw = isMultiSelect ?
            'tr("Showing ") + ' + total + ' + tr(" of ") + ' + totalAvail + ' + tr(" selected stars/objects.")' :
            'tr("Exploring culturally significant stars and celestial objects.")';
        s += buildWelcomeMessage(cultureInfo.name, null, msg3Raw);

        // Stars array
        s += '// ============================================================\n';
        s += '// STARS AND CELESTIAL OBJECTS LIST\n';
        s += '// ============================================================\n';
        s += 'var celestialObjects = [\n';

				// Diagnostic debugger for the stars array map
				console.log('[Diagnostic] starsToUse order:', starsToUse.map(s => ({
						id: s.id,
						vmag: s._info ? s._info.vmag : 'no-info',
						name: s.name
				})));

        for (var i = 0; i < starsToUse.length; i++) {
            var star = starsToUse[i];
            var objectId = star.id.replace(/"/g, '\\"');
            var commentText = '';
            if (star.englishName && star.englishName !== objectId && star.englishName !== star.name) {
                commentText = star.englishName;
            } else if (star.nativeName && star.nativeName !== objectId && star.nativeName !== star.name) {
                commentText = star.nativeName;
            } else if (star.name && star.name !== objectId) {
                commentText = star.name;
            }
            if (star.isSolarSystem && commentText) commentText = 'Planet: ' + commentText;
            else if (star.isSolarSystem) commentText = 'Solar system object';
            if (commentText && commentText.length > 60) commentText = commentText.substring(0, 57) + '...';

            s += '    "' + objectId + '"' + (i < starsToUse.length - 1 ? ',' : '') +
                 (commentText ? ' // ' + commentText : '') + '\n';
        }
        s += '];\n\n';

        // Tour configuration
        s += '// ============================================================\n';
        s += '// TOUR CONFIGURATION\n';
        s += '// ============================================================\n';
        s += 'var DURATION = ' + duration + ';\n';
        s += 'var ZOOM_FOV = ' + zoomFov + ';\n';
        s += 'var OVERVIEW_FOV = ' + overviewFov + ';\n';
        s += 'var SHOW_LABELS = ' + (showLabels ? 'true' : 'false') + ';\n\n';

        // Tour execution
        s += '// ============================================================\n';
        s += '// TOUR EXECUTION\n';
        s += '// ============================================================\n';
        s += 'core.debug(tr("Starting stars tour..."));\n';
        s += 'core.debug(tr("Total objects: ") + celestialObjects.length);\n\n';
        s += 'for (var i = 0; i < celestialObjects.length; i++) {\n';
        s += '    var objId = celestialObjects[i];\n';
        s += '    var currentNum = i + 1;\n';
        s += '    \n';
        s += '    core.debug(tr("Visiting object") + " " + currentNum + "/" + celestialObjects.length + ": " + objId);\n';
        s += '    \n';
        s += '    core.selectObjectByName(objId, true);\n';
        s += '    core.moveToObject(objId, 2);\n';
        s += '    core.wait(1);\n';
        s += '    \n';
        s += '    StelMovementMgr.zoomTo(ZOOM_FOV, 2);\n';
        s += '    core.wait(1);\n';
        s += '    \n';
        s += '    var nameLabel = null;\n';
        s += '    if (SHOW_LABELS) {\n';
        s += '        nameLabel = LabelMgr.labelScreen(objId, 50, 50, false, ' + fontSize + ', "#ffaa66");\n';
        s += '        LabelMgr.setLabelShow(nameLabel, true);\n';
        s += '    }\n';
        s += '    core.wait(DURATION);\n';
        s += '    if (nameLabel !== null) {\n';
        s += '        LabelMgr.deleteLabel(nameLabel);\n';
        s += '    }\n';
        s += '    \n';
        s += '    core.selectObjectByName("", false);\n';
        s += '    \n';
        s += '    StelMovementMgr.zoomTo(OVERVIEW_FOV, 2);\n';
        s += '    core.wait(1);\n';
        s += '}\n\n';

        // Reset and cleanup
        var restoreLines = null;
        if (!useSaveState) {
            restoreLines = [
                'core.selectObjectByName("", false);',
                '',
                'StarMgr.setFlagStars(oldStarsState);',
                'StarMgr.setFlagLabels(oldLabelsState);',
                'StarMgr.setLabelsAmount(oldLabelsAmount);',
                'StarMgr.setFlagAdditionalNames(oldAdditionalNames);',
                'StarMgr.setFlagDblStarsDesignation(oldDblStars);',
                'StarMgr.setFlagVarStarsDesignation(oldVarStars);',
                'StarMgr.setFlagHIPDesignation(oldHIPDesignation);',
                '',
                'StelSkyDrawer.setAbsoluteStarScale(oldAbsoluteScale);',
                'StelSkyDrawer.setRelativeStarScale(oldRelativeScale);',
                'StelSkyDrawer.setFlagTwinkle(oldTwinkleFlag);',
                'StelSkyDrawer.setTwinkleAmount(oldTwinkleAmount);',
                'StelSkyDrawer.setFlagStarSpiky(oldSpikyFlag);',
                'StelSkyDrawer.setFlagDrawBigStarHalo(oldHaloFlag);',
                'StelSkyDrawer.setFlagStarMagnitudeLimit(oldMagLimitFlag);',
                'StelSkyDrawer.setCustomStarMagnitudeLimit(oldCustomMagLimit);',
                '',
                'ConstellationMgr.setFlagLines(oldConstLinesState);',
                'ConstellationMgr.setFlagLabels(oldConstLabelsState);',
                'ConstellationMgr.setFlagArt(oldConstArtState);',
                'ConstellationMgr.setFlagBoundaries(oldConstBoundariesState);',
                'AsterismMgr.setFlagLines(oldAsterLinesState);',
                'AsterismMgr.setFlagLabels(oldAsterLabelsState);',
                'ConstellationMgr.setFlagLunarSystem(oldLunarState);'
            ];
        }
        s += buildResetAndCleanup({
            useSaveState: useSaveState,
            stateName:    'stars_tour_static_state',
            restoreLines: restoreLines,
            extraCleanup: ['StarMgr.setFontSize(12);'],
            visibilityFilterActive: visibilityActive						
        });

        return s;
    }

    // =====================================================================
    // TOOLBAR CREATION
    // =====================================================================

    /**
     * Create a standard tour toolbar with a help button.
     * 
     * @param {Object} opts
     *   - containerId:    toolbar container ID
     *   - sectionTitle:   title text
     *   - helpTargetId:   help modal ID
     *   - helpLabel:      help button text
     *   - tabSelector:    tab selector (e.g. '#patterns-tab-constellations')
     *   - buttonsContainerSelector: where buttons live
     *   - tr:             translation function
     *   - buildContent:   function($toolbar) that fills the toolbar
     */
		function createToolbar(opts) {
				if ($('#' + opts.containerId).length > 0) return;

				var $tab = $(opts.tabSelector);
				if (!$tab.length) {
						console.warn('[SkyCulture:Tours] Tab not found:', opts.tabSelector);
						return;
				}

				var $buttonsContainer = $tab.find(opts.buttonsContainerSelector);
				if (!$buttonsContainer.length) {
						console.warn('[SkyCulture:Tours] Buttons container not found:', 
												 opts.buttonsContainerSelector);
						return;
				}

				var $toolbarContainer = $('<div id="' + opts.containerId +
						'" class="tour-controls-container"></div>');

				var $sectionTitle = $(
						'<div class="tour-section-title">' +
								'<h2>' + opts.sectionTitle + '</h2>' +
								'<button type="button" ' +
										'class="pattern-btn jquerybutton tour-help-btn" ' +
										'data-help-target="' + opts.helpTargetId + '" ' +
										'title="' + opts.tr('How to use this section') + '">' +
										'<span class="ui-icon ui-icon-help"></span> ' +
										opts.helpLabel +
								'</button>' +
						'</div>'
				);

				var $toolbar = $('<div class="tour-toolbar-grid"></div>');

				// Build content (HTML + click handlers)
				opts.buildContent($toolbar, opts.tr);

				$toolbarContainer.append($sectionTitle);
				$toolbarContainer.append($toolbar);

				// Insert into the DOM BEFORE initializing controls
				// (jQuery UI widgets require the elements to be in the document)
				$buttonsContainer.after($toolbarContainer);

				// -----------------------------------------------------------------
				// Initialize all tour controls (sliders, spinners, color previews)
				// 
				// This is called AFTER insertion into the DOM, which is required
				// for jQuery UI widgets to work correctly.
				// -----------------------------------------------------------------
				if (opts.tourType) {
						initTourToolbar(opts.tourType, $toolbar);
				}
		}

    /**
     * Async wrapper that fetches server status before delegating to the
     * synchronous generator. Used by the toolbar button handlers.
     * 
     * The status is cached in `_lastStatus` and consumed by:
     *   - buildHeader (metadata)
     *   - buildVisibilitySnapshot (SSC locking code)
     * 
     * If fetchStatus fails, generation proceeds normally (metadata
     * omits location/time, but the script is still valid).
     * 
     * @param {Function} syncGenerator - The sync generator to invoke
     * @returns {Promise<string|null>}
     */
    function withStatus(syncGenerator) {
        return Api.fetchStatus().then(function(status) {
            _lastStatus = status;
            try {
                return syncGenerator();
            } finally {
                _lastStatus = null;   // clear after use
            }
        });
    }

    // Convenience wrappers
    function generateConstellationsTourStaticAsync() {
        return withStatus(generateConstellationsTourStatic);
    }
    function generateConstellationsTourDynamicAsync() {
        return withStatus(generateConstellationsTourDynamic);
    }
    function generateAsterismsTourAsync() {
        return withStatus(generateAsterismsTour);
    }
    function generateLunarTourAsync() {
        return withStatus(generateLunarTour);
    }
    function generateStarsTourAsync() {
        return withStatus(generateStarsTour);
    }
		
		
    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        // Generation
				generateConstellationsTour:       generateConstellationsTourStatic,   // alias
				generateConstellationsTourStatic: generateConstellationsTourStatic,
				generateConstellationsTourDynamic: generateConstellationsTourDynamic,
        generateAsterismsTour:      generateAsterismsTour,
        generateLunarTour:          generateLunarTour,
        generateStarsTour:          generateStarsTour,
        
				// Async generators (preferred — enrich with status)
        generateConstellationsTourStaticAsync,
        generateConstellationsTourDynamicAsync,
        generateAsterismsTourAsync,
        generateLunarTourAsync,
        generateStarsTourAsync,
				
        // Toolbar
        createToolbar: createToolbar,
    
				// Control initialization helpers
				outputScript: outputScript,
				initTourToolbar: initTourToolbar,
				initColorSliders: initColorSliders,
				initSpinners: initSpinners,

        // Async wrappers for constellation helpers (exposed for testing)
        withStatus,
				
        // Helpers (exposed for reuse)
        uiNum:      uiNum,
        uiBool:     uiBool,
        uiInt:      uiInt,
        uiColor:    uiColor,
        notify:     notify,
        fmt:        fmt
    };
});