/* jshint expr: true */

define(["jquery", "settings", "globalize", "api/remotecontrol", "api/actions",
    "api/properties", "./time", "./joystickqueue", "./actions", "./viewoptions",
    "./scripts", "./viewcontrol", "./location", "./search", "./skyculture/skyculture", 
    "./skyculture-stats", "./gpcontroller", 
    "scripteditor/scriptEditor", "scripteditor/codeGenerator",
		"scripteditor/apiExplorer", "scripteditor/actionsCategorized", "scripteditor/btnGenerator", "scripteditor/unifiedButtons", "jquery-ui"
], function($, settings, globalize, rc, actionApi, propApi, timeui,
    JoystickQueue, actionsui, viewoptionsui, scriptsui, viewcontrolui, locationui,
    searchui, skyculture, skycultureStats, gpcontroller,
    scriptEditor, codeGenerator, apiExplorer, actionsCategorized, btnGenerator, unifiedButtons) {
    "use strict";

	var animationSupported = (window.requestAnimationFrame !== undefined);
	//controls
	var $noresponse;
	var $noresponsetime;

	var activeTab = 0;
	//keep preloaded images to prevent browser from releasing them
	var preloadedImgs = [];


	if (!animationSupported) {
		console.log("animation frame not supported");
	} else {
		console.log("animation frame supported");
	}

	function animate() {

		if (activeTab === 0)
			timeui.updateTimeDisplay();

		if (rc.isConnectionLost()) {
			var elapsed = (Date.now() - rc.getLastDataTime()) / 1000;
			var text = Math.floor(elapsed).toString();
			$noresponsetime[0].textContent = text;
		}

		if (settings.useAnimationFrame && animationSupported) {
			window.requestAnimationFrame(animate);
		} else {
			setTimeout(animate, settings.animationDelay);
		}
	}

	// create
	function createAutomaticGUIElements() {
		console.log("setting up GUI elements");

		//automatically setup spinners
		$("input.spinner").each(function() {
			var self = $(this),
				min = self.data("min"),
				max = self.data("max"),
				step = self.data("step"),
				format = self.data("numberformat");
			self.spinner({
				min: min,
				max: max,
				step: step,
				numberFormat: format
			});
		});

		//setup sliders
		$("div.slider").each(function() {
			var self = $(this),
				min = self.data("min"),
				max = self.data("max"),
				step = self.data("step");
			self.slider({
				min: min,
				max: max,
				step: step
			});
			// ADD MOUSE WHEEL SUPPORT FOR ALL SLIDERS
			self.on('wheel', function(e) {
					e.preventDefault();
					var $slider = $(this);
					var currentValue = $slider.slider('value');
					var min = $slider.slider('option', 'min') || 0;
					var max = $slider.slider('option', 'max') || 100;
					var step = $slider.slider('option', 'step') || 1;
					var delta = e.originalEvent.deltaY;
					var direction = delta < 0 ? 1 : -1;
					var newValue = Math.min(max, Math.max(min, currentValue + (direction * step)));
					$slider.slider('value', newValue);
					$slider.trigger('slide', { value: newValue });
					return false;
			});
		});

		//create jquery ui buttons + selectmenu
		$("button.jquerybutton").button();
		$("select.selectmenu").selectmenu({
			width: 'auto'
		});

		//setup joysticks
		$(".joystickcontainer > .joystick").each(function() {
			var joy = $(this);
			new JoystickQueue(joy, joy.data("joyurl"), joy.data("joymax"));
		});
	}

	function connectStelProperties() {
		//hook up automatic stelproperty spinners
		$("input.spinner.stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");
			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				if (!self.data("updatePaused"))
					self.spinner("value", prop.value);
			});
			self.spinner("value", propApi.getStelProp(prop));

			self.on("focus", function(evt) {
				self.data("updatePaused", true);
			});

			self.on("blur", function(evt) {
				self.data("updatePaused", false);
			});

			self.on("spinuserinput", function(evt, ui) {
				propApi.setStelPropQueued(prop, ui.value);
			});
		});

		//hook up stelproperty checkboxes
		$("input[type='checkbox'].stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");
			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self[0].checked = prop.value;
			});
			self[0].checked = propApi.getStelProp(prop);
			self.click(function() {
				propApi.setStelProp(prop, this.checked);
			});
		});

		$("div.slider.stelproperty").each(function() {
			var self = $(this);
			var prop = self.data("prop");
			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self.slider("value", prop.value);
			});
			self.slider("value", propApi.getStelProp(prop));
			self.on("slide", function(evt, ui) {
				propApi.setStelPropQueued(prop, ui.value);
			});
		});

		//hook up span stelproperty display
		$("span.stelproperty").each(function() {
			var elem = this;
			var self = $(this);
			var prop = self.data("prop");
			var numberformat = self.data("numberformat");

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				var val = prop.value;
				if (numberformat)
					val = globalize.format(val, numberformat);
				elem.textContent = val;
			});
			var val = propApi.getStelProp(prop);
			if (numberformat)
				val = globalize.format(val, numberformat);
			elem.textContent = val;
		});

		$("select.stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}
			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self.val(prop.value);
				self.data("currentSelection", prop.value);
				//if this is a jquery UI selectmenu, we have to refresh
				if (self.hasClass('selectmenu')) {
					self.selectmenu("refresh");
				}
			});
			var curVal = propApi.getStelProp(prop);
			self.val(curVal);
			//store the selection value also in the element itself
			self.data("currentSelection", curVal);
			// lgtm[js/xss] - value is sent via AJAX, not inserted into HTML
			self.on("change selectmenuchange", function(evt) {
				propApi.setStelProp(prop, self.val());
			});
		});

		//stelproperty direct value change
		$("button.stelproperty, input[type='button'].stelproperty").click(function() {
			var prop = this.name;
			var val = this.value;

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
				return;
			}

			if (!val) {
				console.error(
					'Error: no value defined for an "stelproperty" button, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no value defined for an "stelproperty" button, see log for details'
				);
				return;
			}

			propApi.setStelProp(prop, val);
		});
		
		// STELPROPERTY TOGGLE BUTTONS (unified style)
		$("button." + unifiedButtons.CLASSES.PROPERTY_TOGGLE).each(function() {
				var $btn = $(this);
				var prop = $btn.attr("name");
				
				if (!prop) {
						console.error('Error: no StelProperty name defined on stelproperty-toggle button');
						console.dir(this);
						return;
				}
				
				// Ensure action-state-icon exists
				var $iconSpan = $btn.find('.action-state-icon');
				if ($iconSpan.length === 0) {
						$iconSpan = $('<span class="action-state-icon"></span>');
						$btn.prepend($iconSpan);
				}
				
				// Function to update icon based on state
				function updateButtonIcon($button, isChecked) {
						var $icon = $button.find('.action-state-icon');
						if ($icon.length === 0) return;
						
						// Always update the icon regardless of current content
						if (isChecked) {
								$icon.text('✓').removeClass('icon-unchecked icon-trigger').addClass('icon-checked');
						} else {
								$icon.text('✗').removeClass('icon-checked icon-trigger').addClass('icon-unchecked');
						}
				}
				
				// Listen to server changes
				$(propApi).on("stelPropertyChanged:" + prop, function(evt, propData) {
						var isChecked = (propData.value === true || propData.value === 'true' || propData.value === 1 || propData.value === '1');
						
						// Update unified button state
						unifiedButtons.updateState($btn, isChecked);
						
						// Update icon - ALWAYS update regardless of existing content
						updateButtonIcon($btn, isChecked);
				});
				
				// Set initial state from server
				var initialValue = propApi.getStelProp(prop);
				if (initialValue !== undefined) {
						var isChecked = (initialValue === true || initialValue === 'true' || initialValue === 1 || initialValue === '1');
						unifiedButtons.updateState($btn, isChecked);
						
						// Set initial icon
						updateButtonIcon($btn, isChecked);
				}
				
				// Click handler - send toggle to server
				$btn.off('click.stelpropToggle').on('click.stelpropToggle', function(e) {
						e.preventDefault();
						e.stopPropagation();
						
						var currentValue = propApi.getStelProp(prop);
						var newValue = !(currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');
						
						// Optimistic UI update
						unifiedButtons.updateState($btn, newValue);
						updateButtonIcon($btn, newValue);
						
						// Send to server
						propApi.setStelProp(prop, newValue);
				});
		});

		// STELPROPERTY TEXT INPUTS (unified style)
		$("input." + unifiedButtons.CLASSES.PROPERTY_TEXT).each(function() {
				var $input = $(this);
				var prop = $input.attr("name");
				
				if (!prop) {
						console.error('Error: no StelProperty name defined on stelproperty-text input');
						console.dir(this);
						return;
				}
				
				// Listen to server changes
				$(propApi).on("stelPropertyChanged:" + prop, function(evt, propData) {
						if (propData.value !== undefined && propData.value !== null) {
								$input.val(String(propData.value));
						}
				});
				
				// Set initial value from server
				var initialValue = propApi.getStelProp(prop);
				if (initialValue !== undefined && initialValue !== null) {
						$input.val(String(initialValue));
				}
				
				// Send on Enter key or Apply button click
				$input.off('keydown.stelpropText').on('keydown.stelpropText', function(e) {
						if (e.key === 'Enter') {
								e.preventDefault();
								var value = $input.val();
								if (value !== undefined && value !== null) {
										propApi.setStelProp(prop, value);
								}
						}
				});
		});

		// ============================================================
		// RADIO GROUP STELPROPERTY SUPPORT (robust version)
		// ============================================================
		// Handles radio groups sharing the same name = StelProperty id.
		// Server -> UI: checks the radio matching server value.
		// UI -> Server: on 'change' (capture phase), sends radio value.
		//
		// Works with:
		//   - Enum string properties (e.g. Scenery3d.locationInfoStyle)
		//   - Enum numeric properties
		//   - Boolean properties (two radios with values "true"/"false")
		// Example markup:
		//   <input type="radio" class="stelproperty" name="Scenery3d.locationInfoStyle" value="LocationInfoTopRight" />
		//   <input type="radio" class="stelproperty" name="Scenery3d.locationInfoStyle" value="LocationInfoBottomCenter" />
		// ============================================================

		// Collect all radio groups by property name (deduplicated)
		var radioGroups = {};
		$("input[type='radio'].stelproperty").each(function() {
				var $r = $(this);
				var prop = $r.attr("name");

				if (!prop) {
						console.error(
								'[stelproperty radio] Missing name (StelProperty id) on element:'
						);
						console.dir(this);
						return;
				}

				if ($r.attr("value") === undefined) {
						console.error(
								'[stelproperty radio] Missing value attribute on radio for property "' +
								prop + '":'
						);
						console.dir(this);
						return;
				}

				if (!radioGroups[prop]) {
						radioGroups[prop] = [];
				}
				radioGroups[prop].push(this);
		});

		// Wire up each unique group exactly once
		$.each(radioGroups, function(prop, radios) {
				var $group = $(radios);

				// ------------------------------------------------------------
				// Server -> UI
				// ------------------------------------------------------------
				$(propApi).on("stelPropertyChanged:" + prop, function(evt, propData) {
						var serverVal = propData.value;
						var matched = false;

						$group.each(function() {
								if (String(this.value) === String(serverVal)) {
										this.checked = true;
										matched = true;
										return false; // break
								}
						});

						if (!matched) {
								$group.prop("checked", false);
						}
				});

				// ------------------------------------------------------------
				// Initial sync from server (in case property already known)
				// ------------------------------------------------------------
				var initialVal = propApi.getStelProp(prop);
				if (initialVal !== undefined && initialVal !== null) {
						$group.each(function() {
								if (String(this.value) === String(initialVal)) {
										this.checked = true;
										return false;
								}
						});
				}

				// ------------------------------------------------------------
				// UI -> Server
				// ------------------------------------------------------------
				// Use native 'change' event with capture on each radio in group.
				// We use .off() first to avoid duplicate bindings if function runs
				// more than once (e.g. after re-connecting to server).
				// ------------------------------------------------------------
				$group.off("change.stelpropRadio").on("change.stelpropRadio", function(e) {
						// `this` is the radio that triggered change
						if (!this.checked) {
								return; // ignore the radio that got unchecked
						}

						var newVal = $(this).attr("value");
						if (newVal === undefined) {
								return;
						}

						// Optional: optimistic UI is not needed here because the
						// browser already checked the radio. Just send to server.
						propApi.setStelProp(prop, newVal);

						// Debug
						console.log('[stelproperty radio]', prop, '->', newVal);
				});

				// Fallback: some browsers / themes do not fire 'change' on radio
				// when clicked via label. Bind 'click' as well (idempotent).
				$group.off("click.stelpropRadio").on("click.stelpropRadio", function(e) {
						if (!this.checked) {
								return;
						}
						var newVal = $(this).attr("value");
						if (newVal === undefined) {
								return;
						}
						// Only send if different from last known server value to
						// avoid double-send with 'change'.
						var lastKnown = propApi.getStelProp(prop);
						if (String(lastKnown) !== String(newVal)) {
								propApi.setStelProp(prop, newVal);
						}
				});
		});
	}

		// ============================================================
		// CHECKBOX inside .btn-toggle label - Unified handler
		// ============================================================
		// Applies to:
		//   <label class="btn-toggle">
		//       <input type="checkbox" class="stelproperty" name="X" />
		//   </label>
		//   <label class="btn-toggle">
		//       <input type="checkbox" class="stelaction" name="X" />
		//   </label>
		//
		// Reads the source of truth from either propApi or actionApi depending on
		// the checkbox class, and keeps 'active' on the label in sync.
		// ============================================================
		function bindLabelToggleCheckboxes() {
				function toBool(value) {
						return value === true || value === 'true' || value === 1 || value === '1';
				}

				// ---- StelProperty checkboxes ----
				$("label.btn-toggle > input[type='checkbox'].stelproperty").each(function() {
						var $cb = $(this);
						var $lbl = $cb.closest("label.btn-toggle");
						var prop = $cb.attr("name");
						if (!$lbl.length || !prop) return;

						// Initial
						var initVal = propApi.getStelProp(prop);
						if (initVal !== undefined) $lbl.toggleClass("active", toBool(initVal));

						// Server -> UI
						$(propApi).on("stelPropertyChanged:" + prop, function(evt, data) {
								$lbl.toggleClass("active", toBool(data.value));
						});

						// UI -> Server (visual only; server-side change will re-sync)
						$cb.on("change", function() {
								$lbl.toggleClass("active", this.checked === true);
						});
				});

				// ---- StelAction checkboxes ----
				$("label.btn-toggle > input[type='checkbox'].stelaction").each(function() {
						var $cb = $(this);
						var $lbl = $cb.closest("label.btn-toggle");
						var id = $cb.attr("name");
						if (!$lbl.length || !id) return;

						// Initial (uses actionApi's current known state)
						$lbl.toggleClass("active", $cb[0].checked === true);

						// Server -> UI (actions.js already updates the checkbox state;
						// we just mirror it to the label's 'active' class)
						$(actionApi).on("stelActionChanged", function(evt, changedId, data) {
								if (changedId !== id) return;
								$lbl.toggleClass("active", data.isChecked === true);
						});

						// UI -> Server (visual only; the actions.js global click handler
						// still calls actionApi.execute())
						$cb.on("change", function() {
								$lbl.toggleClass("active", this.checked === true);
						});
				});
		}

		// ============================================================
		// COLOR SPINNER CONTROLS WITH DEBOUNCING
		// ============================================================
		/**
		 * Connect color spinner controls with StelProperty synchronization.
		 * Features:
		 * - Debounced server updates (300ms delay after user stops changing)
		 * - Prevents value jumping during spinner interaction
		 * - Smooth user experience with requestAnimationFrame
		 * - Bidirectional synchronization with server
		 * 
		 * @function connectColorSpinners
		 * @global
		 */
		function connectColorSpinners() {
				// Find all color picker wrappers
				$('.color-picker-wrapper').each(function() {
						var $wrapper = $(this);
						var prop = $wrapper.data('prop');
						
						if (!prop) {
								console.error('[ColorSpinner] Missing data-prop attribute');
								return;
						}

						// Get all three color inputs
						var $rInput = $wrapper.find('.color-r');
						var $gInput = $wrapper.find('.color-g');
						var $bInput = $wrapper.find('.color-b');
						var $swatch = $wrapper.find('.color-swatch');

						// ============================================================
						// 1. STATE MANAGEMENT
						// ============================================================
						var updatePending = false;
						var updateScheduled = false;
						var debounceTimer = null;
						var isUserInteracting = false;
						var ignoreServerUpdates = false;
						var serverUpdateTimeout = null;

						// ============================================================
						// 2. INITIALIZE SPINNERS
						// ============================================================
						function initSpinner($input) {
								if (!$input || !$input.length || !$input.hasClass('spinner')) {
										return;
								}

								if ($input.data('ui-spinner')) {
										return;
								}

								var min = parseFloat($input.data('min')) || 0;
								var max = parseFloat($input.data('max')) || 1;
								var step = parseFloat($input.data('step')) || 0.01;

								$input.spinner({
										min: min,
										max: max,
										step: step,
										numberFormat: 'n2',
										// Prevent server updates during spinner animation
										start: function() {
												isUserInteracting = true;
												ignoreServerUpdates = true;
										},
										stop: function() {
												isUserInteracting = false;
												// Re-enable server updates after a short delay
												setTimeout(function() {
														ignoreServerUpdates = false;
												}, 100);
										}
								});
						}

						// Initialize all three spinners
						initSpinner($rInput);
						initSpinner($gInput);
						initSpinner($bInput);

						// ============================================================
						// 3. HELPER FUNCTIONS
						// ============================================================

						/**
						 * Get current RGB values.
						 * 
						 * Sources (in priority order):
						 *   1. color inputs (.color-r, .color-g, .color-b) if present and valid
						 *   2. inline background-color of the swatch (e.g. "rgb(0,255,240)")
						 *   3. jQuery .css() fallback (computed style)
						 *   4. [0, 0, 0] as last resort
						 * 
						 * This allows swatch-only markup (without color-inputs) to work:
						 *   <div class="color-picker-wrapper" data-prop="...">
						 *       <div class="color-swatch" style="background-color: rgb(0,255,240);"></div>
						 *   </div>
						 * 
						 * @returns {Array} [r, g, b] values (0-1)
						 */
						function getCurrentColor() {
								// ---- SOURCE 1: color inputs (full mode) ----
								if ($rInput.length && $gInput.length && $bInput.length) {
										var rIn = parseFloat($rInput.val());
										var gIn = parseFloat($gInput.val());
										var bIn = parseFloat($bInput.val());
										
										// If all three are valid numbers, use them
										if (!isNaN(rIn) && !isNaN(gIn) && !isNaN(bIn)) {
												return [
														Math.max(0, Math.min(1, rIn)),
														Math.max(0, Math.min(1, gIn)),
														Math.max(0, Math.min(1, bIn))
												];
										}
								}
								
								// ---- SOURCE 2: swatch inline style (swatch-only mode) ----
								var rgb = parseRgbFromSwatch($swatch);
								if (rgb) {
										return rgb;
								}
								
								// ---- SOURCE 3: last resort ----
								return [0, 0, 0];
						}

						/**
						 * Parse "rgb(r,g,b)" from a swatch's inline style or computed style.
						 * @param {jQuery} $sw - the swatch element
						 * @returns {Array|null} [r, g, b] in 0-1 range, or null
						 */
						function parseRgbFromSwatch($sw) {
								if (!$sw || !$sw.length) {
										return null;
								}
								
								// Try inline style first (works even if element is hidden)
								var inlineStyle = $sw.attr('style') || '';
								var match = inlineStyle.match(/rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/i);
								
								if (!match) {
										// Fallback: read computed style (jQuery normalizes to "rgb(r, g, b)")
										try {
												var computed = $sw.css('background-color') || '';
												match = computed.match(/rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)/i);
										} catch (e) {
												match = null;
										}
								}
								
								if (!match) {
										return null;
								}
								
								return [
										parseInt(match[1], 10) / 255,
										parseInt(match[2], 10) / 255,
										parseInt(match[3], 10) / 255
								];
						}

						/**
						 * Update the color swatch from current RGB values.
						 * @param {Array} rgb - Optional RGB values, uses current if not provided
						 * @returns {Array} [r, g, b] values
						 */
						function updateSwatch(rgb) {
								if (!rgb) {
										rgb = getCurrentColor();
								}
								
								var r = rgb[0], g = rgb[1], b = rgb[2];
								
								var color = 'rgb(' + 
										Math.round(r * 255) + ',' + 
										Math.round(g * 255) + ',' + 
										Math.round(b * 255) + ')';
								
								$swatch.css('background-color', color);
								return rgb;
						}

						/**
						 * Send color to server with DEBOUNCING.
						 * Waits for user to stop changing values before sending.
						 * @param {Array} rgb - [r, g, b] values
						 */
						function sendColorWithDebounce(rgb) {
								// Clear any existing debounce timer
								if (debounceTimer !== null) {
										clearTimeout(debounceTimer);
										debounceTimer = null;
								}
								
								// Set new debounce timer (300ms delay)
								debounceTimer = setTimeout(function() {
										// Don't send if user is still interacting
										if (isUserInteracting) {
												// Reschedule if still interacting
												sendColorWithDebounce(rgb);
												return;
										}
										
										// Send the final color to server
										var r = Math.max(0, Math.min(1, rgb[0]));
										var g = Math.max(0, Math.min(1, rgb[1]));
										var b = Math.max(0, Math.min(1, rgb[2]));
										
										var value = '[' + 
												r.toFixed(2) + ', ' + 
												g.toFixed(2) + ', ' + 
												b.toFixed(2) + ']';
										
										propApi.setStelProp(prop, value);
										debounceTimer = null;
										
										console.log('[ColorSpinner] Sent to server:', prop, value);
								}, 300); // 300ms delay - adjust as needed
						}

						/**
						 * Handle color change - called from user events.
						 */
						function handleColorChange() {
								// Get current color
								var rgb = getCurrentColor();
								
								// Update swatch immediately for visual feedback
								updateSwatch(rgb);
								
								// Send to server with debouncing (300ms delay)
								sendColorWithDebounce(rgb);
						}

						/**
						 * Set color value on inputs (from server).
						 * IGNORES server updates if user is interacting or if we're in debounce period.
						 * @param {Array} rgb - [r, g, b] values
						 */
						function setColorValue(rgb) {
								// IGNORE server updates if user is interacting
								if (isUserInteracting || ignoreServerUpdates) {
										console.log('[ColorSpinner] Ignoring server update (user interacting)');
										return;
								}
								
								var r = Math.max(0, Math.min(1, rgb[0]));
								var g = Math.max(0, Math.min(1, rgb[1]));
								var b = Math.max(0, Math.min(1, rgb[2]));
								
								// Store current values to detect changes
								var currentR = parseFloat($rInput.val()) || 0;
								var currentG = parseFloat($gInput.val()) || 0;
								var currentB = parseFloat($bInput.val()) || 0;
								
								// Only update if values are different
								if (Math.abs(currentR - r) < 0.001 && 
										Math.abs(currentG - g) < 0.001 && 
										Math.abs(currentB - b) < 0.001) {
										return; // Values are the same
								}
								
								// Temporarily disable events to prevent loops
								$rInput.off('.colorSpinner');
								$gInput.off('.colorSpinner');
								$bInput.off('.colorSpinner');
								
								// Set values using spinner API if available
								try {
										if ($rInput.data('ui-spinner')) {
												$rInput.spinner('value', r);
												$gInput.spinner('value', g);
												$bInput.spinner('value', b);
										} else {
												$rInput.val(r.toFixed(2));
												$gInput.val(g.toFixed(2));
												$bInput.val(b.toFixed(2));
										}
								} catch(e) {
										$rInput.val(r.toFixed(2));
										$gInput.val(g.toFixed(2));
										$bInput.val(b.toFixed(2));
								}
								
								// Update swatch
								updateSwatch([r, g, b]);
								
								// Re-enable events after a short delay
								setTimeout(function() {
										bindEvents();
								}, 50);
						}

						// ============================================================
						// 4. EVENT HANDLING
						// ============================================================

						/**
						 * Bind all events to inputs.
						 */
						function bindEvents() {
								// For each color input, bind events
								$rInput.off('.colorSpinner').on({
										'input.colorSpinner': handleColorChange,
										'spin.colorSpinner': handleColorChange,
										'spinchange.colorSpinner': handleColorChange
								});

								$gInput.off('.colorSpinner').on({
										'input.colorSpinner': handleColorChange,
										'spin.colorSpinner': handleColorChange,
										'spinchange.colorSpinner': handleColorChange
								});

								$bInput.off('.colorSpinner').on({
										'input.colorSpinner': handleColorChange,
										'spin.colorSpinner': handleColorChange,
										'spinchange.colorSpinner': handleColorChange
								});
						}

						// ============================================================
						// 5. SERVER SYNC (WITH IGNORE FLAG)
						// ============================================================

						/**
						 * Handle StelProperty changes from server.
						 * IGNORES server updates if user is interacting.
						 */
						function onServerChange(evt, propData) {
								// IGNORE server updates if user is interacting
								if (isUserInteracting || ignoreServerUpdates) {
										console.log('[ColorSpinner] Ignoring server update during interaction');
										return;
								}
								
								var value = propData.value;
								var colorArray = null;
								
								// Parse color from various formats
								if (typeof value === 'string') {
										try {
												colorArray = JSON.parse(value);
										} catch(e) {
												var cleaned = value.replace(/[\[\]]/g, '').trim().split(',');
												if (cleaned.length === 3) {
														var parsed = cleaned.map(function(v) { 
																return parseFloat(v.trim()); 
														});
														if (!parsed.some(isNaN)) {
																colorArray = parsed;
														}
												}
										}
								} else if (Array.isArray(value) && value.length === 3) {
										colorArray = value;
								}
								
								if (colorArray && colorArray.length === 3) {
										// Update the UI from server
										setColorValue(colorArray);
								}
						}

						// Listen for server changes
						$(propApi).on('stelPropertyChanged:' + prop, onServerChange);

						// ============================================================
						// 6. NATIVE COLOR PICKER (WITH DEBOUNCING)
						// ============================================================

						/**
						 * Open native color picker on swatch click.
						 */
						function onSwatchClick() {
								var rgb = getCurrentColor();
								var r = rgb[0], g = rgb[1], b = rgb[2];
								
								var hex = '#' + 
										Math.round(r * 255).toString(16).padStart(2, '0') +
										Math.round(g * 255).toString(16).padStart(2, '0') +
										Math.round(b * 255).toString(16).padStart(2, '0');
								
								var input = document.createElement('input');
								input.type = 'color';
								input.value = hex;
								
								input.addEventListener('input', function() {
										var hexVal = this.value;
										var r2 = parseInt(hexVal.substring(1,3), 16) / 255;
										var g2 = parseInt(hexVal.substring(3,5), 16) / 255;
										var b2 = parseInt(hexVal.substring(5,7), 16) / 255;
										
										var newRgb = [r2, g2, b2];
										
										// Update swatch immediately
										updateSwatch(newRgb);
										
										// Update spinners
										setColorValue(newRgb);
										
										// Send to server with debouncing
										sendColorWithDebounce(newRgb);
								});
								
								input.click();
						}

						$swatch.off('.colorSpinner').on('click.colorSpinner', onSwatchClick);

						// ============================================================
						// 7. CLEANUP
						// ============================================================
						
						/**
						 * Clean up resources when the element is removed.
						 */
						function cleanup() {
								if (debounceTimer !== null) {
										clearTimeout(debounceTimer);
										debounceTimer = null;
								}
								$(propApi).off('stelPropertyChanged:' + prop, onServerChange);
								$rInput.off('.colorSpinner');
								$gInput.off('.colorSpinner');
								$bInput.off('.colorSpinner');
								$swatch.off('.colorSpinner');
						}

						$wrapper.data('cleanupColorSpinner', cleanup);

						// ============================================================
						// 8. INITIALIZATION
						// ============================================================

						// Get initial value from server
						var initialValue = propApi.getStelProp(prop);
						if (initialValue !== undefined) {
								var colorArray = null;
								
								if (typeof initialValue === 'string') {
										try {
												colorArray = JSON.parse(initialValue);
										} catch(e) {
												var cleaned = initialValue.replace(/[\[\]]/g, '').trim().split(',');
												if (cleaned.length === 3) {
														var parsed = cleaned.map(function(v) { 
																return parseFloat(v.trim()); 
														});
														if (!parsed.some(isNaN)) {
																colorArray = parsed;
														}
												}
										}
								} else if (Array.isArray(initialValue) && initialValue.length === 3) {
										colorArray = initialValue;
								}
								
								if (colorArray && colorArray.length === 3) {
										setColorValue(colorArray);
								}
						}

						// Bind events for user interaction
						bindEvents();

						console.log('[ColorSpinner] Initialized (with debouncing) for property:', prop);
				});
		}

	//DOM-ready
	$(function() {
		//preload the error images, otherwise they may be loaded when the connection is lost, which of course wont work

		var preLoadImages = [
			"/external/images/ui-icons_fbc856_256x240.png",
			"/external/images/ui-bg_glass_35_dddddd_1x400.png"
		];

		preLoadImages.forEach(function(val) {
			var img = new Image();
			img.src = val;
			preloadedImgs.push(img);
		});

		//find and setup some controls
		$noresponse = $("#noresponse");
		$noresponsetime = $("#noresponsetime");

		$noresponse.dialog({
			autoOpen: false,
			modal: true,
			draggable: false,
			resizable: false,
			dialogClass: "fixed-dialog ui-state-error"
		});

		$(window).resize(function() {
			$noresponse.dialog("option", "position", {
				my: "center",
				at: "center",
				of: window
			});
		});

		//create and connect automatic GUI elements defined in the DOM
		createAutomaticGUIElements();
		connectStelProperties();
		bindLabelToggleCheckboxes();
		
		// CONNECT COLOR PICKER CONTROLS
		connectColorSpinners();
		
		// Initialize unfiedButtons module
		unifiedButtons.init();
		
		//main tabs
		//remember which tab was active after refresh by storing id in sessionstore
		var oldTabId = 0;
		var tabDataKey = "activeMainTab";

		var webStorageSupported = typeof(Storage) !== "undefined";

		if (webStorageSupported) {
			oldTabId = parseInt(sessionStorage.getItem(tabDataKey), 10);
			if (isNaN(oldTabId))
				oldTabId = 0;
		} else {
			console.log("webstorage API unsupported");
		}

		activeTab = oldTabId;

		var $tabs = $("#tabs");
		$tabs.tabs({
			active: oldTabId,
			activate: function(evt, ui) {
				var idx = ui.newTab.index();
				activeTab = idx;
				if (webStorageSupported) {
					sessionStorage.setItem(tabDataKey, idx);
				}
			}
		});

		var $loading = $("#loadindicator").hide(),
			timer;
		$(document).ajaxStart(function() {
			timer && clearTimeout(timer);
			timer = setTimeout(function() {
				$loading.show();
			}, settings.spinnerDelay);
		});
		$(document).ajaxStop(function() {
			clearTimeout(timer);
			$loading.hide();
		});

		//start animation & update loop
		animate();
		rc.startUpdateLoop();

		$("#loadoverlay").fadeOut();
		$(rc).trigger("uiReady"); //signal other components that the main UI init is done (some may need the jQueryUI stuff set up)
		
		// =====================================================================
		// INITIALIZE SKY CULTURE MODULE (UPDATED FOR MODULAR ARCHITECTURE)
		// =====================================================================
		// The skyculture module has been refactored into a modular
		// architecture (13 files in js/skyculture/). The public API is
		// backward-compatible with the previous version:
		// 
		//   skyculture.init(cultureContainer, patternsContainers, iframe, options)
		// 
		// The internal implementations of:
		// - fetch (unified fetchObjectInfo for all types)
		// - filters (unified applyAll pipeline)
		// - multi-select (unified single implementation)
		// - sync (central normalizer + button index)
		// 
		// all work transparently behind the same public API.
		// =====================================================================
		if (typeof skyculture !== 'undefined' && skyculture && typeof skyculture.init === 'function') {
				// Short delay to ensure DOM is fully ready and other modules are initialized
				setTimeout(function() {
						if ($("#skyculture-buttons-container").length) {
								// Define all container selectors for the skyculture module
								var containers = {
										constellations: "#constellations-buttons-container",
										asterisms: "#asterisms-buttons-container",
										zodiac: "#zodiac-buttons-container",
										lunar: "#lunar-buttons-container",
										stars: "#stars-buttons-container",
										artwork: "#artwork-container"
								};

								// Initialize the skyculture module with:
								// - Culture selector container
								// - Pattern containers (all 6 tabs)
								// - Info iframe for culture description
								// - Culture count badge
								skyculture.init(
										"#skyculture-buttons-container",
										containers,
										"#vo_skycultureinfo",
										{ cultureCount: "#skyculture-culture-count" }
								);

								console.log("[MainUI] Sky culture module initialized (modular v5.0.0)");
						} else {
								console.warn("[MainUI] Sky culture container not found, module not initialized");
						}
				}, 100);
		} else {
				console.warn("[MainUI] Sky culture module not available");
		}
		
		// =====================================================================
		// INITIALIZE SKY CULTURE STATISTICS MODULE
		// =====================================================================
		if (typeof skycultureStats !== 'undefined' && skycultureStats && typeof skycultureStats.init === 'function') {
				setTimeout(function() {
						skycultureStats.init({
								cultureContainer: "#skyculture-stats-buttons",
								cultureCount: "#skyculture-stats-count",
								totalCultures: "#stats-total-cultures",
								totalConstellations: "#stats-total-constellations",
								totalAsterisms: "#stats-total-asterisms",
								totalRayHelpers: "#stats-total-ray-helpers",
								totalZodiac: "#stats-total-zodiac",
								totalLunar: "#stats-total-lunar",
								totalStars: "#stats-total-stars",
								constellationsBody: "#constellations-stats-body",
								asterismsBody: "#asterisms-stats-body",
								zodiacBody: "#zodiac-stats-body",
								lunarBody: "#lunar-stats-body",
								starsBody: "#stars-stats-body",
								constellationsCount: "#constellations-stats-count",
								asterismsCount: "#asterisms-stats-count",
								zodiacCount: "#zodiac-stats-count",
								lunarCount: "#lunar-stats-count",
								starsCount: "#stars-stats-count",
								descriptionContent: "#stats-description-content"
						});
						console.log("[MainUI] Sky culture statistics module initialized");
				}, 200);
		}
    
    // =====================================================================
    // INITIALIZE GAMEPAD CONTROLLER
    // =====================================================================
    // Initialize Gamepad controller after UI is ready
    if (typeof gpcontroller !== 'undefined' && gpcontroller && typeof gpcontroller.init === 'function') {
        gpcontroller.init();
        console.log("[MainUI] Gamepad controller initialized");
    } else {
        console.warn("[MainUI] Gamepad controller module not available");
    }
		
		// =====================================================================
		// INITIALIZE SUB-TABS IN ACTIONS PANEL
		// =====================================================================
		if ($("#actions-sub-tabs").length) {
				$("#actions-sub-tabs").tabs({
						active: 0,
						heightStyle: "content",
						activate: function(evt, ui) {
								// Refresh CodeMirror when switching to editor tab
								var newPanel = ui.newPanel;
								if (newPanel && newPanel.attr('id') === 'sub-tab-scripts-editor') {
										// Trigger CodeMirror refresh after tab switch
										setTimeout(function() {
												if (window._stelCodeMirrorInstance) {
														window._stelCodeMirrorInstance.refresh();
														console.log('[MainUI] CodeMirror refreshed after tab switch');
												} else if (window._cm) {
														window._cm.refresh();
												}
										}, 150);
								}
						}
				});
				console.log("[MainUI] Actions sub-tabs initialized");
		}

		// =====================================================================
		// INITIALIZE SCRIPT EDITOR MODULE
		// =====================================================================
		if (typeof require !== 'undefined') {
				require(["scripteditor/scriptEditor"], function(scriptEditor) {
						if (scriptEditor && typeof scriptEditor.init === 'function') {
								scriptEditor.init();
								console.log("[MainUI] Script Editor module initialized");
						} else {
								console.warn("[MainUI] Script Editor module not available");
						}
				});
		}

		// =====================================================================
		// INITIALIZE CODE GENERATOR MODULE
		// =====================================================================
		if (typeof require !== 'undefined') {
				require(["scripteditor/codeGenerator"], function(codeGenerator) {
						if (codeGenerator && typeof codeGenerator.init === 'function') {
								codeGenerator.init();
								console.log("[MainUI] Code Generator module initialized");
						} else {
								console.warn("[MainUI] Code Generator module not available");
						}
				});
		}

		// =====================================================================
		// INITIALIZE API EXPLORER MODULE
		// =====================================================================
		if (typeof require !== 'undefined') {
				require(["scripteditor/apiExplorer"], function(apiExplorer) {
						if (apiExplorer && typeof apiExplorer.init === 'function') {
								apiExplorer.init();
								console.log("[MainUI] API Explorer module initialized");
						} else {
								console.warn("[MainUI] API Explorer module not available");
						}
				});
		}
		
		// =====================================================================
		// INITIALIZE BUTTON GENERATOR MODULE
		// =====================================================================
		if (typeof require !== 'undefined') {
				require(["scripteditor/btnGenerator"], function(btnGenerator) {
						if (btnGenerator && typeof btnGenerator.init === 'function') {
								setTimeout(function() {
										btnGenerator.init();
										console.log("[MainUI] Button Generator module initialized");
								}, 200);
						} else {
								console.warn("[MainUI] Button Generator module not available or missing init()");
						}
				});
		}

		// =====================================================================
		// INITIALIZE ACTIONS CATEGORIZED BUTTONS MODULE
		// =====================================================================
		if (typeof actionsCategorized !== 'undefined' && actionsCategorized && 
				typeof actionsCategorized.init === 'function') {
				
				setTimeout(function() {
						if ($('#stelaction-categorized').length) {
								actionsCategorized.init({
										containerSelector: '#stelaction-categorized',
										categoryTabsId: 'actions-category-tabs',
										categoryTabListId: 'actions-category-tab-list',
										categoryPanelsId: 'actions-category-panels',
										searchInputId: 'categorized-action-search',
										countDisplayId: 'categorized-action-count',
										
										// UI Behavior
										doubleClickToExecute: true,
										animateChanges: true,
										maxButtonsPerCategory: 0,
										
										// Search behavior
										searchMinChars: 1,
										
										// API endpoints
										apiEndpoint: '/api/stelaction/list',
										apiActionEndpoint: '/api/stelaction/do',
										
										// UI Text
										loadingText: 'Loading actions...',
										errorText: 'Failed to load actions. Check Stellarium connection.',
										
										// Callbacks
										onActionExecuted: function(actionId, success) {
												console.log('[MainUI] Action executed:', actionId, success ? '✓' : '✗');
										},
										onActionsLoaded: function(totalActions, totalCategories) {
												console.log('[MainUI] Actions loaded:', totalActions, 'in', totalCategories, 'categories');
										},
										onCategoryChanged: function(categoryName) {
												console.log('[MainUI] Category changed to:', categoryName);
										},
										onError: function(error) {
												console.error('[MainUI] Actions error:', error);
										}
								});
								console.log("[MainUI] Actions Categorized module initialized (v3.0 - unified)");
						} else {
								console.warn("[MainUI] Actions container #stelaction-categorized not found");
						}
				}, 100);
		} else {
				console.warn("[MainUI] Actions Categorized module not available");
		}

		// =====================================================================
		// INITIALIZE PLUGINS SUB-TABS (inside #plugins_tab)
		// =====================================================================
		$(document).on('pluginsProcessed', function() {
				var $pluginTabs = $("tab_plugins .plugins-tabs");
				if ($pluginTabs.length && !$pluginTabs.hasClass("ui-tabs")) {
						$pluginTabs.tabs({
								active: 0,
								heightStyle: "content"
						});
						console.log("[MainUI] Plugins sub-tabs initialized");
				}
		});

		// Fallback: also try after a delay in case pluginsProcessed was already fired
		setTimeout(function() {
				var $pluginTabs = $("#tab_plugins .plugins-tabs");
				if ($pluginTabs.length && !$pluginTabs.hasClass("ui-tabs")) {
						$pluginTabs.tabs({
								active: 0,
								heightStyle: "content"
						});
						console.log("[MainUI] Plugins sub-tabs initialized (fallback)");
				}
		}, 1000);
		
	});

	//new server data
	$(rc).on('serverDataReceived', function(event, data) {
		//this will get reset after the event is processed
		//this means the connection WAS lost before, but now is not anymore
		if (rc.isConnectionLost()) {
			$noresponse.dialog("close");
		}
	});

	$(rc).on("serverDataError", function(evt) {
		if (!rc.isConnectionLost()) {
			$noresponse.dialog("open");
		}
	});

});
