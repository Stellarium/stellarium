/* jshint expr: true */

define(["jquery", "settings", "globalize", "api/remotecontrol", "api/actions",
    "api/properties", "./time", "./joystickqueue", "./actions", "./viewoptions",
    "./scripts", "./viewcontrol", "./location", "./search", "ui/skyculture", 
    "ui/skyculture-stats", "./gpcontroller", 
    "scripteditor/scriptEditor", "scripteditor/codeGenerator",
		"scripteditor/apiExplorer", "scripteditor/actionsCategorized", "scripteditor/btnGenerator", "jquery-ui"
], function($, settings, globalize, rc, actionApi, propApi, timeui,
    JoystickQueue, actionsui, viewoptionsui, scriptsui, viewcontrolui, locationui,
    searchui, skyculture, skycultureStats, gpcontroller,
    scriptEditor, codeGenerator, apiExplorer, actionsCategorized, btnGenerator) {
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
			var elapsed = ($.now() - rc.getLastDataTime()) / 1000;
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
	}

	// ============================================================
	// COLOR PICKER CONTROLS FOR SKY CULTURE OPTIONS
	// ============================================================
	// Hook up color picker controls
	function connectColorPickers() {
			$('.color-picker-wrapper').each(function() {
					var $wrapper = $(this);
					var prop = $wrapper.data('prop');
					if (!prop) {
							console.error('Color picker missing data-prop attribute');
							return;
					}

					var $swatch = $wrapper.find('.color-swatch');
					var $rInput = $wrapper.find('.color-r');
					var $gInput = $wrapper.find('.color-g');
					var $bInput = $wrapper.find('.color-b');

					$rInput.data('prop', prop);
					$gInput.data('prop', prop);
					$bInput.data('prop', prop);
					$swatch.data('prop', prop);

					function updateSwatch($wrapper2) {
							var r = parseFloat($wrapper2.find('.color-r').val()) || 0;
							var g = parseFloat($wrapper2.find('.color-g').val()) || 0;
							var b = parseFloat($wrapper2.find('.color-b').val()) || 0;
							
							r = Math.max(0, Math.min(1, r));
							g = Math.max(0, Math.min(1, g));
							b = Math.max(0, Math.min(1, b));
							
							var rgb = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
							$wrapper2.find('.color-swatch').css('background-color', rgb);
							
							return [r, g, b];
					}

					function sendColor(prop2, r, g, b) {
							r = Math.max(0, Math.min(1, r));
							g = Math.max(0, Math.min(1, g));
							b = Math.max(0, Math.min(1, b));
							
							var value = '[' + r.toFixed(2) + ', ' + g.toFixed(2) + ', ' + b.toFixed(2) + ']';
							propApi.setStelProp(prop2, value);
					}

					// Listen for server changes
					$(propApi).on('stelPropertyChanged:' + prop, function(evt, propData) {
							var value = propData.value;
							var colorArray = null;
							
							if (typeof value === 'string') {
									try {
											colorArray = JSON.parse(value);
									} catch(e) {
											var cleaned = value.replace(/[\[\]]/g, '').trim().split(',').map(function(v) {
													return parseFloat(v.trim());
											});
											if (cleaned.length === 3 && !cleaned.some(isNaN)) {
													colorArray = cleaned;
											}
									}
							} else if (Array.isArray(value) && value.length === 3) {
									colorArray = value;
							}
							
							if (colorArray && colorArray.length === 3) {
									var r = parseFloat(colorArray[0]) || 0;
									var g = parseFloat(colorArray[1]) || 0;
									var b = parseFloat(colorArray[2]) || 0;
									
									$rInput.val(r.toFixed(2));
									$gInput.val(g.toFixed(2));
									$bInput.val(b.toFixed(2));
									
									var rgb = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
									$swatch.css('background-color', rgb);
							}
					});

					// Get initial value
					var initialValue = propApi.getStelProp(prop);
					if (initialValue !== undefined) {
							var colorArray = null;
							if (typeof initialValue === 'string') {
									try {
											colorArray = JSON.parse(initialValue);
									} catch(e) {
											var cleaned = initialValue.replace(/[\[\]]/g, '').trim().split(',').map(function(v) {
													return parseFloat(v.trim());
											});
											if (cleaned.length === 3 && !cleaned.some(isNaN)) {
													colorArray = cleaned;
											}
									}
							} else if (Array.isArray(initialValue) && initialValue.length === 3) {
									colorArray = initialValue;
							}
							
							if (colorArray && colorArray.length === 3) {
									var r = parseFloat(colorArray[0]) || 0;
									var g = parseFloat(colorArray[1]) || 0;
									var b = parseFloat(colorArray[2]) || 0;
									$rInput.val(r.toFixed(2));
									$gInput.val(g.toFixed(2));
									$bInput.val(b.toFixed(2));
									var rgb = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
									$swatch.css('background-color', rgb);
							}
					}

					// Bind events
					$rInput.off('input.colorPicker').on('input.colorPicker', function() {
							var $wrapper2 = $(this).closest('.color-picker-wrapper');
							var prop2 = $wrapper2.data('prop');
							var colors = updateSwatch($wrapper2);
							sendColor(prop2, colors[0], colors[1], colors[2]);
					});

					$gInput.off('input.colorPicker').on('input.colorPicker', function() {
							var $wrapper2 = $(this).closest('.color-picker-wrapper');
							var prop2 = $wrapper2.data('prop');
							var colors = updateSwatch($wrapper2);
							sendColor(prop2, colors[0], colors[1], colors[2]);
					});

					$bInput.off('input.colorPicker').on('input.colorPicker', function() {
							var $wrapper2 = $(this).closest('.color-picker-wrapper');
							var prop2 = $wrapper2.data('prop');
							var colors = updateSwatch($wrapper2);
							sendColor(prop2, colors[0], colors[1], colors[2]);
					});

					// Swatch click - open native color picker
					$swatch.off('click.colorPicker').on('click.colorPicker', function() {
							var $wrapper2 = $(this).closest('.color-picker-wrapper');
							var r = parseFloat($wrapper2.find('.color-r').val()) || 0;
							var g = parseFloat($wrapper2.find('.color-g').val()) || 0;
							var b = parseFloat($wrapper2.find('.color-b').val()) || 0;
							
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
									var prop2 = $wrapper2.data('prop');
									$wrapper2.find('.color-r').val(r2.toFixed(2));
									$wrapper2.find('.color-g').val(g2.toFixed(2));
									$wrapper2.find('.color-b').val(b2.toFixed(2));
									var rgb = 'rgb(' + Math.round(r2 * 255) + ',' + Math.round(g2 * 255) + ',' + Math.round(b2 * 255) + ')';
									$wrapper2.find('.color-swatch').css('background-color', rgb);
									sendColor(prop2, r2, g2, b2);
							});
							input.click();
					});
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

		// CONNECT COLOR PICKER CONTROLS
		connectColorPickers();

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
		// INITIALIZE SKY CULTURE BUTTONS MODULE
		// =====================================================================
		// Check if skyculture module is available and initialize it with all required containers
		// The module now supports multiple data panels: constellations, asterisms, zodiac, lunar mansions, and stars
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
										artwork: "#artwork-container"  // Add artwork container
								};
								
								// Initialize the skyculture module with culture selector, patterns containers, and info iframe
								skyculture.init(
										"#skyculture-buttons-container",    // Culture buttons container
										containers,                         // Patterns containers object
										"#vo_skycultureinfo",                // iframe skyculture info container
										{ cultureCount: "#skyculture-culture-count" } // skyculture count
								);
								console.log("[MainUI] Sky culture buttons module initialized with multi-panel support");
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
												} else if (window.CodeMirror) {
														var cmElements = document.querySelectorAll('.CodeMirror');
														for (var i = 0; i < cmElements.length; i++) {
																if (cmElements[i].CodeMirror) {
																		cmElements[i].CodeMirror.refresh();
																}
														}
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
										doubleClickToExecute: true,
										autoRefreshStates: false,
										refreshInterval: 0,
										animateChanges: true,
										showTriggerIcons: true,
										apiEndpoint: '/api/stelaction/list',
										apiActionEndpoint: '/api/stelaction/do',
										loadingText: 'Loading actions...',
										errorText: 'Failed to load actions. Check Stellarium connection.',
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
								console.log("[MainUI] Actions Categorized module initialized");
						} else {
								console.warn("[MainUI] Actions container #stelaction not found");
						}
				}, 100);
		} else {
				console.warn("[MainUI] Actions Categorized module not available");
		}
		
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
