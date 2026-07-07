// ========================================================================
// btnGenerator.js - Advanced Button Generator Module (v1.0.0)
// ========================================================================
//
// This module provides a comprehensive button generation and management
// interface for Stellarium actions and properties. It allows users to
// browse, preview, generate HTML code, and create persistent custom
// control panels with full support for multiple property types including
// boolean toggles, numeric sliders, color pickers, and text inputs.
//
// KEY FEATURES:
// - Browse and search actions and properties from all categories
// - Live preview with instant property updates (optimistic UI)
// - Automatic HTML code generation with multiple implementation options
// - Persistent custom button storage via localStorage
// - Bulk import/export of custom button configurations
// - Real-time synchronization with server state changes
// - Smart echo detection to prevent value rebounding
// - Support for boolean, numeric, color, and text property types
// - Category-based batch addition of actions and properties
//
// KEY FIXES (v1.0.0):
// - Fixed boolean property toggle value rebounding issue
// - Fixed convertStelProp boolean string conversion in properties API
// - Proper pendingUpdates tracking for toggleBooleanProperty function
// - Fixed numeric slider initialization and visibility
// - Added min/max/step input controls for numeric properties
// - Fixed color picker functionality in custom buttons section
// - Added "Add Category" buttons for batch operations
// - Fixed duplicate custom button rendering
// - Proper event delegation for dynamically created controls
// - Fixed text property apply/reset button functionality
//
// @module btnGenerator
// @requires jquery
// @requires api/remotecontrol
// @requires api/actions
// @requires api/properties
//
// @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
// @date 2026-06-28
// @license GPLv2+
// @version 1.0.0
//
// ========================================================================

define([
    "jquery",
    "api/remotecontrol",
    "api/actions",
    "api/properties"
], function($, rc, actionApi, propApi) {
    "use strict";

    // ============================================================
    // CONSTANTS & CONFIGURATION
    // ============================================================

    var TYPE_ENUMS = {
        BOOL: 1,
        INT: 2,
        UINT: 3,
        LONG: 4,
        ULONG: 5,
        DOUBLE: 6,
        FLOAT: 38,
        VECTOR3: 65536,
        VECTOR4: 131072
    };

    // ============================================================
    // PRIVATE VARIABLES
    // ============================================================

    var isInitialized = false;
    var actionDataCache = {};
    var propertyDataCache = {};
    var customButtons = [];
    var selectedType = null;
    var selectedActionId = null;
    var selectedPropertyName = null;
    var customLabel = '';
    var isAddingAll = false;
		var isAddingButton = false;    // Prevent double addition
		var isRendering = false;       // Prevent double rendering
    
    // Track pending updates to prevent rebounds
    var pendingUpdates = {};
    var updateCounter = 0;
    
    // DOM References
    var $container, $categorySelect, $actionList, $propertyList;
    var $searchInput, $countDisplay, $previewContainer, $codeContainer;
    var $copyHtmlBtn, $addBtn, $customContainer, $exportBtn, $importBtn, $clearBtn;
    var $customLabelInput, $propertyInfoContainer;
    var $addAllActionsBtn, $addAllPropertiesBtn;
    var $addCategoryActionsBtn, $addCategoryPropertiesBtn;
    var $categoryActionSelect, $categoryPropertySelect;
    var $customCountDisplay, $statusDisplay;

    // Store original options for search/filter
    var originalActionOptions = null;
    var originalPropertyOptions = null;
    
    // Numeric property custom values
    var numericMin = 0;
    var numericMax = 100;
    var numericStep = 1;

    // ============================================================
    // TRANSLATION HELPER
    // ============================================================

    function tr(str) {
        if (rc && typeof rc.tr === 'function') {
            return rc.tr.apply(rc, arguments);
        }
        return str;
    }

    // ============================================================
    // LOGGING
    // ============================================================

    function log(msg) {
        console.log("[BtnGen] " + msg);
    }

    function logError(msg) {
        console.error("[BtnGen] " + msg);
    }

    // ============================================================
    // HTML ESCAPE HELPERS
    // ============================================================

    function escapeHtml(str) {
        if (!str) return '';
        return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
    }

    function escapeAttr(str) {
        if (!str) return '';
        return String(str).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/'/g, '&#39;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }

    // ============================================================
    // PROPERTY TYPE DETECTION
    // ============================================================

    function isBooleanProperty(info) {
        return info.typeEnum === TYPE_ENUMS.BOOL || info.typeString === 'bool';
    }

    function isNumericProperty(info) {
        var te = info.typeEnum;
        return (te >= TYPE_ENUMS.INT && te <= TYPE_ENUMS.DOUBLE) || 
               te === TYPE_ENUMS.FLOAT ||
               info.typeString === 'int' || info.typeString === 'double' || 
               info.typeString === 'float';
    }

    function isColorProperty(info, propName) {
        return (info.typeEnum === TYPE_ENUMS.VECTOR3 || info.typeString === 'Vector3<float>') && 
               propName.toLowerCase().includes('color');
    }

    // ============================================================
    // PROPERTY VALUE CONVERSION
    // ============================================================

    function convertValueForServer(propName, value) {
        var info = propertyDataCache[propName];
        if (!info) return String(value);
        
        var typeEnum = info.typeEnum;
        
        if (typeEnum === TYPE_ENUMS.BOOL) {
            return value ? "true" : "false";
        }
        
        if (typeEnum >= TYPE_ENUMS.INT && typeEnum <= TYPE_ENUMS.ULONG) {
            var num = parseInt(value, 10);
            return isNaN(num) ? 0 : num;
        }
        
        if (typeEnum === TYPE_ENUMS.DOUBLE || typeEnum === TYPE_ENUMS.FLOAT) {
            var num = parseFloat(value);
            return isNaN(num) ? 0 : num;
        }
        
        if (typeEnum === TYPE_ENUMS.VECTOR3) {
            if (Array.isArray(value)) {
                return '[' + value.join(', ') + ']';
            }
            if (typeof value === 'string') {
                return value;
            }
            return String(value);
        }
        
        return String(value);
    }

    // ============================================================
    // PROPERTY UPDATE SYSTEM
    // ============================================================

		function updateProperty(propName, value) {
				var info = propertyDataCache[propName];
				if (!info) {
						logError("Property not found: " + propName);
						return false;
				}

				if (info.isWritable === false) {
						logError("Property is read-only: " + propName);
						alert(tr("This property is read-only and cannot be modified."));
						return false;
				}

				var updateId = propName + '_' + (++updateCounter);
				var serverValue = convertValueForServer(propName, value);
				
				log("Updating property: " + propName + " = " + serverValue + " (original: " + value + ")");
				
				pendingUpdates[updateId] = {
						propName: propName,
						value: value,
						serverValue: serverValue,
						timestamp: Date.now()
				};
				
				// Update local cache
				propertyDataCache[propName].value = value;
				
				// Update UI immediately
				updatePropertyUI(propName, value);
				
				// Send to server
				try {
						propApi.setStelProp(propName, serverValue);
						
						setTimeout(function() {
								cleanupPendingUpdates(propName);
						}, 1000);
						
						return true;
				} catch (error) {
						logError("Failed to send property update: " + error.message);
						var cachedValue = propertyDataCache[propName] ? propertyDataCache[propName].value : value;
						updatePropertyUI(propName, cachedValue);
						return false;
				}
		}

    function cleanupPendingUpdates(propName) {
        var now = Date.now();
        var keysToRemove = [];
        
        for (var key in pendingUpdates) {
            var update = pendingUpdates[key];
            if (update.propName === propName) {
                if (now - update.timestamp > 2000) {
                    keysToRemove.push(key);
                }
            }
        }
        
        keysToRemove.forEach(function(key) {
            delete pendingUpdates[key];
        });
    }

    // ============================================================
    // TOGGLE BOOLEAN PROPERTY (FIXED)
    // ============================================================
		
		function toggleBooleanProperty(propName) {
				var info = propertyDataCache[propName];
				if (!info) {
						logError("Property not found: " + propName);
						return false;
				}

				if (info.isWritable === false) {
						logError("Property is read-only: " + propName);
						alert(tr("This property is read-only and cannot be modified."));
						return false;
				}

				var currentValue = info.value;
				var currentBool = false;
				if (typeof currentValue === 'string') {
						currentBool = (currentValue.toLowerCase() === 'true' || currentValue === '1');
				} else if (typeof currentValue === 'number') {
						currentBool = (currentValue === 1);
				} else if (typeof currentValue === 'boolean') {
						currentBool = currentValue;
				} else {
						currentBool = !!currentValue;
				}
				
				var newValue = !currentBool;
				var serverValue = newValue ? "true" : "false";
				
				// Add pendingUpdates
				var updateId = propName + '_' + (++updateCounter);
				pendingUpdates[updateId] = {
						propName: propName,
						value: newValue,
						serverValue: serverValue,
						timestamp: Date.now()
				};

				log("Toggling property: " + propName + " from " + currentBool + " to " + newValue);

				// Updating cash & UI
				propertyDataCache[propName].value = newValue;
				updatePropertyUI(propName, newValue);

				try {
						propApi.setStelProp(propName, serverValue);
						
						setTimeout(function() {
								cleanupPendingUpdates(propName);
						}, 2000);
						
						return true;
				} catch (error) {
						logError("Failed to send property update: " + error.message);
						delete pendingUpdates[updateId];
						propertyDataCache[propName].value = currentValue;
						updatePropertyUI(propName, currentValue);
						return false;
				}
		}

    // ============================================================
    // UPDATE PROPERTY UI (FIXED for sliders)
    // ============================================================

    function updatePropertyUI(propName, value) {
        var info = propertyDataCache[propName];
        if (!info) return;
        
				// Update the cache first
				propertyDataCache[propName].value = value;
				
        var isBool = isBooleanProperty(info);
        var isNum = isNumericProperty(info);
        var isColor = isColorProperty(info, propName);
        
    // Update preview - Boolean toggle buttons
    if (isBool) {
        var boolVal = (value === true || value === 'true' || value === 1 || value === '1');
        
        $('.btn-gen-preview-btn[data-prop="' + propName + '"]').each(function() {
            var $btn = $(this);
            $btn.data('ischecked', boolVal);
            $btn.toggleClass('active', boolVal);
            var $icon = $btn.find('.action-state-icon');
            if ($icon.length) {
                $icon.text(boolVal ? '✓' : '✗');
                $icon.removeClass('checked unchecked').addClass(boolVal ? 'checked' : 'unchecked');
            }
        });
				        // Update checkboxes or toggle buttons in custom section
        $('.btn-gen-custom-item .btn-gen-custom-toggle-btn[data-prop="' + propName + '"]').each(function() {
            var $btn = $(this);
            $btn.data('ischecked', boolVal);
            $btn.toggleClass('active', boolVal);
            $btn.find('.action-state-icon').text(boolVal ? '✓' : '✗');
        });
    }
        
        // Update preview - Numeric sliders (FIXED visibility)
        if (isNum) {
            var numVal = parseFloat(value);
            if (!isNaN(numVal)) {
                $('.btn-gen-preview-slider[data-prop="' + propName + '"]').each(function() {
                    var $slider = $(this);
                    // Check if slider is initialized
                    if ($slider.hasClass('ui-slider')) {
                        $slider.slider('value', numVal);
                    } else {
                        // Initialize slider if not yet initialized
                        var min = parseFloat($slider.data('min')) || 0;
                        var max = parseFloat($slider.data('max')) || 100;
                        var step = parseFloat($slider.data('step')) || 1;
                        $slider.slider({
                            min: min,
                            max: max,
                            step: step,
                            value: numVal,
                            slide: function(evt, ui) {
                                var prop = $(this).data('prop');
                                updateProperty(prop, ui.value);
                                var $display = $(this).closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                                if ($display.length) {
                                    $display.text(ui.value);
                                }
                            }
                        });
                    }
                    var $display = $slider.closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                    if ($display.length) {
                        $display.text(numVal);
                    }
                });
            }
        }
        
        // Update preview - Color pickers
        if (isColor) {
            var colorArray = value;
            if (typeof colorArray === 'string') {
                try { colorArray = JSON.parse(colorArray); } catch(e) { colorArray = [1, 1, 1]; }
            }
            if (!Array.isArray(colorArray) || colorArray.length !== 3) {
                colorArray = [1, 1, 1];
            }
            
            $('.btn-gen-color-wrap[data-prop="' + propName + '"]').each(function() {
                var $wrap = $(this);
                var r = parseFloat(colorArray[0]) || 0;
                var g = parseFloat(colorArray[1]) || 0;
                var b = parseFloat(colorArray[2]) || 0;
                
                var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
                $wrap.find('.btn-gen-color-swatch').css('background-color', color);
                $wrap.find('.btn-gen-color-r').val(r);
                $wrap.find('.btn-gen-color-g').val(g);
                $wrap.find('.btn-gen-color-b').val(b);
            });
        }
        
        // Update custom buttons
        if (isBool) {
            var boolVal2 = (value === true || value === 'true' || value === 1 || value === '1');
            $('.btn-gen-custom-item .btn-gen-custom-toggle-btn[data-prop="' + propName + '"]').each(function() {
                var $btn = $(this);
                $btn.data('ischecked', boolVal2);
                $btn.toggleClass('active', boolVal2);
                $btn.find('.action-state-icon').text(boolVal2 ? '✓' : '✗');
            //$icon.removeClass('checked unchecked').addClass(boolVal2 ? 'checked' : 'unchecked');
            });
        }
        
        if (isNum) {
            var numVal2 = parseFloat(value);
            if (!isNaN(numVal2)) {
                $('.btn-gen-custom-slider[data-prop="' + propName + '"]').each(function() {
                    var $slider = $(this);
                    if ($slider.hasClass('ui-slider')) {
                        $slider.slider('value', numVal2);
                    } else {
                        var min = parseFloat($slider.data('min')) || 0;
                        var max = parseFloat($slider.data('max')) || 100;
                        var step = parseFloat($slider.data('step')) || 1;
                        $slider.slider({
                            min: min,
                            max: max,
                            step: step,
                            value: numVal2,
                            slide: function(evt, ui) {
                                var prop = $(this).data('prop');
                                updateProperty(prop, ui.value);
                                var $display = $(this).closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
                                if ($display.length) {
                                    $display.text(ui.value);
                                }
                            }
                        });
                    }
                    var $display = $slider.closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
                    if ($display.length) {
                        $display.text(numVal2);
                    }
                });
            }
        }
        
        if (isColor) {
            var colorArray2 = value;
            if (typeof colorArray2 === 'string') {
                try { colorArray2 = JSON.parse(colorArray2); } catch(e) { colorArray2 = [1, 1, 1]; }
            }
            if (!Array.isArray(colorArray2) || colorArray2.length !== 3) {
                colorArray2 = [1, 1, 1];
            }
            
            $('.btn-gen-custom-color[data-prop="' + propName + '"]').each(function() {
                var $wrap = $(this);
                var r = parseFloat(colorArray2[0]) || 0;
                var g = parseFloat(colorArray2[1]) || 0;
                var b = parseFloat(colorArray2[2]) || 0;
                var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
                $wrap.find('.btn-gen-color-swatch').css('background-color', color);
            });
        }
        
        // Update property info if selected
        if (selectedType === 'property' && selectedPropertyName === propName) {
            var infoDisplay = propertyDataCache[propName];
            if (infoDisplay) {
                var $infoValue = $propertyInfoContainer.find('td:last-child code');
                if ($infoValue.length) {
                    $infoValue.text(formatValue(value));
                }
                renderPropertyPreview(propName, infoDisplay);
                generateHtmlCode(propName, infoDisplay);
            }
        }
    }


    function formatValue(value) {
        if (value === undefined || value === null) return 'undefined';
        if (typeof value === 'string') return '"' + value + '"';
        if (Array.isArray(value)) return '[' + value.join(', ') + ']';
        if (typeof value === 'object') return JSON.stringify(value);
        return String(value);
    }

    // ============================================================
    // ACTION EXECUTION
    // ============================================================

    function executeAction(actionId) {
        log("Executing action: " + actionId);
        
        var action = findAction(actionId);
        if (!action) {
            logError("Action not found: " + actionId);
            return false;
        }
        
        if (action.isCheckable) {
            var newState = !action.isChecked;
            action.isChecked = newState;
            updateActionUI(actionId, newState);
        }
        
        actionApi.execute(actionId);
        return true;
    }

    function updateActionUI(actionId, isChecked) {
        $('.btn-gen-preview-btn[data-action-id="' + actionId + '"]').each(function() {
            var $btn = $(this);
            $btn.data('ischecked', isChecked);
            $btn.toggleClass('active', isChecked);
            var $icon = $btn.find('.action-state-icon');
            if ($icon.length) {
                $icon.text(isChecked ? '✓' : '✗');
                $icon.removeClass('checked unchecked').addClass(isChecked ? 'checked' : 'unchecked');
            }
        });
        
        $('.btn-gen-custom-item .btn-gen-custom-toggle-btn[data-action-id="' + actionId + '"]').each(function() {
            var $btn = $(this);
            $btn.data('ischecked', isChecked);
            $btn.toggleClass('active', isChecked);
            $btn.find('.action-state-icon').text(isChecked ? '✓' : '✗');
        });
    }

    function findAction(actionId) {
        for (var cat in actionDataCache) {
            var found = actionDataCache[cat].filter(function(a) { return a.id === actionId; });
            if (found.length) return found[0];
        }
        return null;
    }

    // ============================================================
    // GET CATEGORY NAMES
    // ============================================================

    function getActionCategories() {
        return Object.keys(actionDataCache).sort();
    }

    function getPropertyCategories() {
        var groups = {};
        Object.keys(propertyDataCache).forEach(function(name) {
            var parts = name.split('.');
            var category = parts.length > 1 ? parts[0] : 'other';
            if (!groups[category]) groups[category] = [];
            groups[category].push(name);
        });
        return Object.keys(groups).sort();
    }

    function getActionsByCategory(category) {
        return actionDataCache[category] || [];
    }

    function getPropertiesByCategory(category) {
        var result = [];
        Object.keys(propertyDataCache).forEach(function(name) {
            var parts = name.split('.');
            var cat = parts.length > 1 ? parts[0] : 'other';
            if (cat === category) {
                result.push({ id: name, info: propertyDataCache[name] });
            }
        });
        return result;
    }

    // ============================================================
    // ADD CATEGORY FUNCTIONS
    // ============================================================

    function addCategoryActions(category) {
        if (!category) {
            alert(tr("Please select a category first."));
            return;
        }
        
        var actions = getActionsByCategory(category);
        if (actions.length === 0) {
            alert(tr("No actions found in this category."));
            return;
        }
        
        var added = 0;
        var skipped = 0;
        
        actions.forEach(function(action) {
            var exists = customButtons.some(function(b) { 
                return b.type === 'action' && b.id === action.id; 
            });
            if (exists) {
                skipped++;
            } else {
                addCustomButton('action', action, true);
                added++;
            }
        });
        
        var msg = "Added " + added + " actions from category '" + category + "'";
        if (skipped > 0) msg += " (" + skipped + " already existed)";
        log(msg);
        updateStatus(msg);
        renderCustomButtons();
    }

    function addCategoryProperties(category) {
        if (!category) {
            alert(tr("Please select a category first."));
            return;
        }
        
        var props = getPropertiesByCategory(category);
        if (props.length === 0) {
            alert(tr("No writable properties found in this category."));
            return;
        }
        
        var added = 0;
        var skipped = 0;
        
        props.forEach(function(prop) {
            if (prop.info.isWritable === false) {
                skipped++;
                return;
            }
            var exists = customButtons.some(function(b) { 
                return b.type === 'property' && b.id === prop.id; 
            });
            if (exists) {
                skipped++;
            } else {
                addCustomButton('property', { 
                    id: prop.id, 
                    info: prop.info, 
                    label: prop.id 
                }, true);
                added++;
            }
        });
        
        var msg = "Added " + added + " properties from category '" + category + "'";
        if (skipped > 0) msg += " (" + skipped + " skipped: read-only or already existed)";
        log(msg);
        updateStatus(msg);
        renderCustomButtons();
    }

    // ============================================================
    // INITIALIZATION
    // ============================================================

    function init() {
        if (isInitialized) return;
        log("Initializing Button Generator v4.1...");

        $container = $("#btn-gen-container");
        if (!$container.length) {
            logError("Container #btn-gen-container not found!");
            return;
        }

        cacheDomElements();
        loadCustomButtons();
        setupEvents();
        setupSearchFilter();
        loadActions();
        loadProperties();
        setupServerListeners();
        populateCategorySelects();

        isInitialized = true;
        updateStatus("Ready");
        log("Button Generator initialized successfully");
    }

    // ============================================================
    // DOM CACHING
    // ============================================================

    function cacheDomElements() {
        $categorySelect = $("#btn-gen-category");
        $actionList = $("#btn-gen-action-list");
        $propertyList = $("#btn-gen-property-list");
        $searchInput = $("#btn-gen-search");
        $countDisplay = $("#btn-gen-count");
        $previewContainer = $("#btn-gen-preview");
        $codeContainer = $("#btn-gen-code");
        $copyHtmlBtn = $("#btn-gen-copy-html");
        $addBtn = $("#btn-gen-add-btn");
        $customContainer = $("#btn-gen-custom-container");
        $exportBtn = $("#btn-gen-export");
        $importBtn = $("#btn-gen-import");
        $clearBtn = $("#btn-gen-clear");
        $customLabelInput = $("#btn-gen-custom-label");
        $propertyInfoContainer = $("#btn-gen-property-info");
        $addAllActionsBtn = $("#btn-gen-add-all-actions");
        $addAllPropertiesBtn = $("#btn-gen-add-all-properties");
        $addCategoryActionsBtn = $("#btn-gen-add-category-actions");
        $addCategoryPropertiesBtn = $("#btn-gen-add-category-properties");
        $categoryActionSelect = $("#btn-gen-category-actions-select");
        $categoryPropertySelect = $("#btn-gen-category-properties-select");
        $customCountDisplay = $("#btn-gen-custom-count");
        $statusDisplay = $("#btn-gen-status");
    }

    // ============================================================
    // STATUS DISPLAY
    // ============================================================

    function updateStatus(message, isError) {
        if ($statusDisplay) {
            var color = isError ? '#F92672' : '#A6E22E';
            $statusDisplay.html('<span style="color:' + color + ';">' + escapeHtml(message) + '</span>');
        }
    }

    // ============================================================
    // SERVER LISTENERS
    // ============================================================

		function setupServerListeners() {
				$(propApi).on("stelPropertyChanged", function(evt, propName, propData) {
						var value = propData.value;
						log("Server sent property change: " + propName + " = " + value);
						
						if (propertyDataCache[propName]) {
								propertyDataCache[propName].value = value;
						}
						
						// ============================================================
						// FIX: Update the property list dropdown
						// ============================================================
						updatePropertyListOption(propName, value);
						
						var isOurUpdate = false;
						for (var key in pendingUpdates) {
								var update = pendingUpdates[key];
								if (update.propName === propName) {
										var sentValue = update.serverValue;
										var receivedValue = value;
										
										if (typeof sentValue === 'string' && typeof receivedValue === 'string') {
												if (sentValue.toLowerCase() === receivedValue.toLowerCase()) {
														isOurUpdate = true;
														break;
												}
										} else if (sentValue === receivedValue) {
												isOurUpdate = true;
												break;
										}
								}
						}
						
							if (isOurUpdate) {
									log("Ignoring our own echo for: " + propName);
									for (var key2 in pendingUpdates) {
											if (pendingUpdates[key2].propName === propName) {
													delete pendingUpdates[key2];
													break;
											}
									}
									return;
							}
						
						log("External property change detected: " + propName);
						updatePropertyUI(propName, value);
						
						if (selectedType === 'property' && selectedPropertyName === propName) {
								var info = propertyDataCache[propName];
								if (info) {
										renderPropertyPreview(propName, info);
										generateHtmlCode(propName, info);
								}
						}
				});

				$(actionApi).on("stelActionChanged", function(evt, actionId, actionData) {
						log("Server sent action change: " + actionId + " = " + actionData.isChecked);
						
						for (var cat in actionDataCache) {
								var found = actionDataCache[cat].filter(function(a) { return a.id === actionId; });
								if (found.length) {
										found[0].isChecked = actionData.isChecked;
										break;
								}
						}
						
						// ============================================================
						// FIX: Update the action list dropdown
						// ============================================================
						updateActionListOption(actionId, actionData.isChecked);
						
						updateActionUI(actionId, actionData.isChecked);
						
						if (selectedType === 'action' && selectedActionId === actionId) {
								var action = findAction(actionId);
								if (action) {
										renderActionPreview(action);
										generateHtmlCode(action);
								}
						}
				});
		}

		// ============================================================
		// FUNCTIONS TO UPDATE DROPDOWN OPTIONS
		// ============================================================

		function updateActionListOption(actionId, isChecked) {
				var $option = $actionList.find('option[value="' + actionId + '"]');
				if ($option.length) {
						var action = findAction(actionId);
						if (action) {
								var label = action.text;
								if (action.isCheckable) {
										label += ' [' + (isChecked ? 'ON' : 'OFF') + ']';
								}
								$option.text(label);
								log("Updated action list option: " + actionId + " -> " + label);
						}
				}
		}

		function updatePropertyListOption(propName, value) {
				var $option = $propertyList.find('option[value="' + propName + '"]');
				if ($option.length) {
						var info = propertyDataCache[propName];
						if (info) {
								var typeLabel = info.typeString || 'unknown';
								var writable = info.isWritable ? '' : ' [read-only]';
								
								// ============================================================
								// FIX: Properly format the value for display
								// ============================================================
								var valueDisplay = '';
								if (info.value !== undefined && info.value !== null) {
										// For boolean values, show as true/false
										if (typeof info.value === 'boolean') {
												valueDisplay = ' = ' + (info.value ? 'true' : 'false');
										}
										// For string values that are boolean strings
										else if (typeof info.value === 'string' && (info.value === 'true' || info.value === 'false')) {
												valueDisplay = ' = ' + info.value;
										}
										// For numeric values
										else if (typeof info.value === 'number') {
												valueDisplay = ' = ' + info.value;
										}
										// For array values (colors)
										else if (Array.isArray(info.value)) {
												valueDisplay = ' = [' + info.value.join(', ') + ']';
										}
										// For string values
										else if (typeof info.value === 'string') {
												// If it's a long string, truncate
												if (info.value.length > 30) {
														valueDisplay = ' = "' + info.value.substring(0, 27) + '..."';
												} else {
														valueDisplay = ' = "' + info.value + '"';
												}
										}
										// For objects
										else if (typeof info.value === 'object') {
												try {
														var jsonStr = JSON.stringify(info.value);
														if (jsonStr.length > 30) {
																valueDisplay = ' = ' + jsonStr.substring(0, 27) + '...';
														} else {
																valueDisplay = ' = ' + jsonStr;
														}
												} catch(e) {
														valueDisplay = ' = [object]';
												}
										}
										// Fallback
										else {
												valueDisplay = ' = ' + String(info.value);
										}
								}
								
								var newText = propName + ' (' + typeLabel + ')' + writable + valueDisplay;
								$option.text(newText);
								log("Updated property list option: " + propName + " -> " + newText);
						}
				}
		}

    // ============================================================
    // DATA LOADING
    // ============================================================

    function loadActions() {
        log("Loading actions...");
        $.ajax({
            url: "/api/stelaction/list",
            type: "GET",
            dataType: "json",
            success: function(data) {
                actionDataCache = data;
                log("Actions loaded: " + Object.keys(data).length + " categories");
                populateActionList(data);
                populateCategorySelects();
                updateCount();
                updateStatus("Actions loaded");
            },
            error: function() {
                logError("Failed to load actions");
                $actionList.html('<option value="">' + tr("Error loading actions") + '</option>');
                updateStatus("Failed to load actions", true);
            }
        });
    }

    function loadProperties() {
        log("Loading properties...");
        $.ajax({
            url: "/api/stelproperty/list",
            type: "GET",
            dataType: "json",
            success: function(data) {
                propertyDataCache = data;
                log("Properties loaded: " + Object.keys(data).length);
                populatePropertyList(data);
                populateCategorySelects();
                updateCount();
                updateStatus("Properties loaded");
            },
            error: function() {
                logError("Failed to load properties");
                $propertyList.html('<option value="">' + tr("Error loading properties") + '</option>');
                updateStatus("Failed to load properties", true);
            }
        });
    }

    // ============================================================
    // POPULATE CATEGORY SELECTS
    // ============================================================

    function populateCategorySelects() {
        // Actions categories
        var actionCats = getActionCategories();
        $categoryActionSelect.empty();
        $categoryActionSelect.append('<option value="">-- ' + tr("Select action category") + ' --</option>');
        actionCats.forEach(function(cat) {
            var count = getActionsByCategory(cat).length;
            $categoryActionSelect.append('<option value="' + escapeAttr(cat) + '">' + escapeHtml(cat) + ' (' + count + ')</option>');
        });
        
        // Properties categories
        var propCats = getPropertyCategories();
        $categoryPropertySelect.empty();
        $categoryPropertySelect.append('<option value="">-- ' + tr("Select property category") + ' --</option>');
        propCats.forEach(function(cat) {
            var count = getPropertiesByCategory(cat).length;
            $categoryPropertySelect.append('<option value="' + escapeAttr(cat) + '">' + escapeHtml(cat) + ' (' + count + ')</option>');
        });
    }

    // ============================================================
    // LIST POPULATION
    // ============================================================

    function populateActionList(data) {
        var $select = $actionList;
        $select.empty();
        $select.append('<option value="">-- ' + tr("Select an action") + ' --</option>');

        var categories = Object.keys(data).sort();
        var total = 0;

        categories.forEach(function(category) {
            var actions = data[category];
            total += actions.length;
            var $group = $('<optgroup>').attr('label', category + ' (' + actions.length + ')');
            
            actions.forEach(function(action) {
                var label = action.text;
                if (action.isCheckable) {
                    label += ' [' + (action.isChecked ? 'ON' : 'OFF') + ']';
                }
                $group.append($('<option>').val(action.id).text(label).data('action', action));
            });
            
            $select.append($group);
        });

        originalActionOptions = $select.html();
        log("Populated action list: " + categories.length + " categories, " + total + " actions");
    }

    function populatePropertyList(data) {
        var $select = $propertyList;
        $select.empty();
        $select.append('<option value="">-- ' + tr("Select a property") + ' --</option>');

        var groups = {};
        Object.keys(data).forEach(function(name) {
            var parts = name.split('.');
            var category = parts.length > 1 ? parts[0] : 'other';
            if (!groups[category]) groups[category] = [];
            groups[category].push(name);
        });

        var categories = Object.keys(groups).sort();
        var total = 0;

        categories.forEach(function(category) {
            var props = groups[category].sort();
            total += props.length;
            var $group = $('<optgroup>').attr('label', category + ' (' + props.length + ')');
            
            props.forEach(function(name) {
                var info = data[name];
                var typeLabel = info ? info.typeString : 'unknown';
                var writable = info && info.isWritable ? '' : ' [read-only]';
                var valueDisplay = info && info.value !== undefined ? ' = ' + formatValue(info.value) : '';
                $group.append($('<option>').val(name).text(name + ' (' + typeLabel + ')' + writable + valueDisplay).data('property', info));
            });
            
            $select.append($group);
        });

        originalPropertyOptions = $select.html();
        log("Populated property list: " + categories.length + " categories, " + total + " properties");
    }

    // ============================================================
    // SEARCH / FILTER
    // ============================================================

    function setupSearchFilter() {
        $searchInput.on('input', function() {
            var query = $(this).val().toLowerCase().trim();
            filterLists(query);
        });

        $searchInput.on('keydown', function(e) {
            if (e.key === 'Escape') {
                $(this).val('');
                filterLists('');
                $(this).blur();
            }
        });
    }

    function filterLists(query) {
        var type = $categorySelect.val();
        var $activeList = getActiveList();
        if (!$activeList.length) return;

        if (type === 'action' && originalActionOptions) {
            $activeList.html(originalActionOptions);
        } else if (type === 'property' && originalPropertyOptions) {
            $activeList.html(originalPropertyOptions);
        }

        if (!query || query.length === 0) {
            $activeList.find('option').show();
            $activeList.find('optgroup').show();
            var totalVisible = $activeList.find('option').length - 1;
            updateCount(totalVisible);
            return;
        }

        var visibleCount = 0;
        $activeList.find('option').each(function() {
            var $opt = $(this);
            var val = $opt.val();
            if (!val) {
                $opt.show();
                return;
            }
            var text = $opt.text().toLowerCase();
            var matches = text.indexOf(query) >= 0 || val.toLowerCase().indexOf(query) >= 0;
            $opt.toggle(matches);
            if (matches) visibleCount++;
        });

        $activeList.find('optgroup').each(function() {
            var hasVisible = $(this).find('option:visible').length > 0;
            $(this).toggle(hasVisible);
        });

        updateCount(visibleCount);
    }

    function getActiveList() {
        var type = $categorySelect.val();
        if (type === 'action') return $actionList;
        if (type === 'property') return $propertyList;
        return $();
    }

    function updateCount(visibleCount) {
        var type = $categorySelect.val();
        var total = 0;
        var text = '';

        if (type === 'action') {
            var cats = Object.keys(actionDataCache);
            cats.forEach(function(c) { total += actionDataCache[c].length; });
        } else if (type === 'property') {
            total = Object.keys(propertyDataCache).length;
        }

        if (visibleCount !== undefined && visibleCount < total) {
            text = visibleCount + ' / ' + total + ' ' + tr('visible');
        } else if (total > 0) {
            text = total + ' ' + tr('items');
        } else {
            text = '';
        }

        $countDisplay.text(text);
        updateCustomCount();
    }

    function updateCustomCount() {
        if ($customCountDisplay) {
            $customCountDisplay.text(customButtons.length);
        }
    }

    // ============================================================
    // EVENT SETUP
    // ============================================================

    function setupEvents() {
        // Category select
        $categorySelect.on('change', function() {
            var val = $(this).val();
            selectedType = val;
            log("Category selected: " + val);

            $searchInput.val('');
            
            if (val === 'action') {
                $actionList.closest('.btn-gen-step').show();
                $propertyList.closest('.btn-gen-step').hide();
                $propertyInfoContainer.hide();
                $customLabelInput.closest('.btn-gen-step').hide();
                $previewContainer.html('<div class="btn-gen-placeholder">' + tr("Select an action from the list") + '</div>');
                $codeContainer.html('<div class="btn-gen-placeholder">' + tr("Select an item to generate HTML") + '</div>');
                if (originalActionOptions) {
                    $actionList.html(originalActionOptions);
                }
                updateCount();
            } else if (val === 'property') {
                $actionList.closest('.btn-gen-step').hide();
                $propertyList.closest('.btn-gen-step').show();
                $propertyInfoContainer.show();
                $customLabelInput.closest('.btn-gen-step').show();
                $previewContainer.html('<div class="btn-gen-placeholder">' + tr("Select a property from the list") + '</div>');
                $codeContainer.html('<div class="btn-gen-placeholder">' + tr("Select an item to generate HTML") + '</div>');
                if (originalPropertyOptions) {
                    $propertyList.html(originalPropertyOptions);
                }
                updateCount();
            } else {
                $actionList.closest('.btn-gen-step').hide();
                $propertyList.closest('.btn-gen-step').hide();
                $propertyInfoContainer.hide();
                $customLabelInput.closest('.btn-gen-step').hide();
                $previewContainer.html('<div class="btn-gen-placeholder">' + tr("Select a category") + '</div>');
                $codeContainer.html('<div class="btn-gen-placeholder">' + tr("Select an item to generate HTML") + '</div>');
            }
            updateStatus("Ready");
        });

        // Action list selection
        $actionList.on('change', function() {
            var val = $(this).val();
            if (val) {
                selectedActionId = val;
                var action = findAction(val);
                if (action) {
                    renderActionPreview(action);
                    generateHtmlCode(action);
                    updateStatus("Action selected: " + action.text);
                }
            }
        });

        // Property list selection
        $propertyList.on('change', function() {
            var val = $(this).val();
            if (val) {
                selectedPropertyName = val;
                var info = propertyDataCache[val];
                if (info) {
                    if (info.isWritable === false) {
                        var msg = tr("This property is read-only and cannot be modified.");
                        $previewContainer.html('<div class="btn-gen-placeholder" >' + msg + '</div>');
                        $codeContainer.html('<div class="btn-gen-placeholder">' + msg + '</div>');
                        showPropertyInfo(val, info);
                        updateStatus("Read-only property selected", true);
                        return;
                    }
                    renderPropertyPreview(val, info);
                    generateHtmlCode(val, info);
                    showPropertyInfo(val, info);
                    updateStatus("Property selected: " + val);
                }
            }
        });

        // Custom label
        $customLabelInput.on('input', function() {
            customLabel = $(this).val();
            if (selectedType === 'property' && selectedPropertyName) {
                var info = propertyDataCache[selectedPropertyName];
                if (info && info.isWritable !== false) {
                    renderPropertyPreview(selectedPropertyName, info);
                    generateHtmlCode(selectedPropertyName, info);
                }
            }
        });

        // Add single button
        $addBtn.on('click', function() {
            if (selectedType === 'action' && selectedActionId) {
                var action = findAction(selectedActionId);
                if (action) addCustomButton('action', action);
            } else if (selectedType === 'property' && selectedPropertyName) {
                var info = propertyDataCache[selectedPropertyName];
                if (info) {
                    if (info.isWritable === false) {
                        alert(tr("This property is read-only and cannot be added as a control."));
                        return;
                    }
                    addCustomButton('property', { id: selectedPropertyName, info: info, label: customLabel || selectedPropertyName });
                }
            }
            updateStatus("Button added");
        });

        // Add All Actions
        $addAllActionsBtn.on('click', function() {
            if (isAddingAll) return;
            isAddingAll = true;
            
            var totalActions = getAllActionsCount();
            if (totalActions === 0) {
                log("No actions to add");
                isAddingAll = false;
                return;
            }
            
            if (!confirm(tr("Add all ") + totalActions + tr(" actions? This may take a moment."))) {
                isAddingAll = false;
                return;
            }
            
            $addAllActionsBtn.text(tr("Adding...")).prop('disabled', true);
            
            setTimeout(function() {
                var cats = Object.keys(actionDataCache);
                var added = 0;
                cats.forEach(function(cat) {
                    actionDataCache[cat].forEach(function(action) {
                        addCustomButton('action', action, true);
                        added++;
                    });
                });
                $addAllActionsBtn.text(tr("Add All Actions")).prop('disabled', false);
                isAddingAll = false;
                log("Added all actions: " + added);
                updateStatus("Added " + added + " actions");
            }, 100);
        });

        // Add All Properties
        $addAllPropertiesBtn.on('click', function() {
            if (isAddingAll) return;
            isAddingAll = true;
            
            var totalProps = getAllPropertiesCount();
            if (totalProps === 0) {
                log("No properties to add");
                isAddingAll = false;
                return;
            }
            
            if (!confirm(tr("Add all ") + totalProps + tr(" properties? This may take a moment."))) {
                isAddingAll = false;
                return;
            }
            
            $addAllPropertiesBtn.text(tr("Adding...")).prop('disabled', true);
            
            setTimeout(function() {
                var props = Object.keys(propertyDataCache);
                var added = 0;
                props.forEach(function(name) {
                    var info = propertyDataCache[name];
                    if (info && info.isWritable !== false) {
                        addCustomButton('property', { id: name, info: info, label: name }, true);
                        added++;
                    }
                });
                $addAllPropertiesBtn.text(tr("Add All Properties")).prop('disabled', false);
                isAddingAll = false;
                log("Added all writable properties: " + added);
                updateStatus("Added " + added + " properties");
            }, 100);
        });

        // Add Category Actions
        $addCategoryActionsBtn.on('click', function() {
            var category = $categoryActionSelect.val();
            if (!category) {
                alert(tr("Please select a category first."));
                return;
            }
            addCategoryActions(category);
        });

        // Add Category Properties
        $addCategoryPropertiesBtn.on('click', function() {
            var category = $categoryPropertySelect.val();
            if (!category) {
                alert(tr("Please select a category first."));
                return;
            }
            addCategoryProperties(category);
        });

        // Export
        $exportBtn.on('click', function() {
            exportButtonsJSON();
        });

        // Import
        $importBtn.on('click', function() {
            importButtonsJSON();
        });

        // Clear
        $clearBtn.on('click', function() {
            if (customButtons.length && confirm(tr("Remove all custom buttons?"))) {
                customButtons = [];
                saveCustomButtons();
                renderCustomButtons();
                updateStatus("Cleared all buttons");
            }
        });

        // Copy HTML button
        $copyHtmlBtn.on('click', function() {
            var code = $codeContainer.find('.btn-gen-code').text();
            if (code && code !== tr("Select an item to generate HTML")) {
                copyToClipboard(code);
                updateStatus("HTML copied to clipboard");
            } else {
                alert(tr("No HTML code to copy. Please select an action or property first."));
            }
        });

        // Min/Max/Step inputs for numeric properties
        $(document).on('input', '.btn-gen-numeric-min, .btn-gen-numeric-max, .btn-gen-numeric-step', function() {
            var $wrap = $(this).closest('.btn-gen-numeric-controls');
            var minVal = parseFloat($wrap.find('.btn-gen-numeric-min').val()) || 0;
            var maxVal = parseFloat($wrap.find('.btn-gen-numeric-max').val()) || 100;
            var stepVal = parseFloat($wrap.find('.btn-gen-numeric-step').val()) || 1;
            
            numericMin = minVal;
            numericMax = maxVal;
            numericStep = stepVal;
            
            // Update slider if it exists
            var propName = $wrap.data('prop');
            if (propName) {
                var $slider = $('.btn-gen-preview-slider[data-prop="' + propName + '"]');
                if ($slider.length && $slider.hasClass('ui-slider')) {
                    $slider.slider('option', 'min', minVal);
                    $slider.slider('option', 'max', maxVal);
                    $slider.slider('option', 'step', stepVal);
                }
                // Update generated code
                var info = propertyDataCache[propName];
                if (info) {
                    generateHtmlCode(propName, info);
                }
            }
        });
    }

    // ============================================================
    // COUNT HELPERS
    // ============================================================

    function getAllActionsCount() {
        var total = 0;
        var cats = Object.keys(actionDataCache);
        cats.forEach(function(c) { total += actionDataCache[c].length; });
        return total;
    }

    function getAllPropertiesCount() {
        var total = 0;
        var props = Object.keys(propertyDataCache);
        props.forEach(function(name) {
            if (propertyDataCache[name] && propertyDataCache[name].isWritable !== false) {
                total++;
            }
        });
        return total;
    }

    // ============================================================
    // PROPERTY INFO DISPLAY
    // ============================================================

    function showPropertyInfo(propName, info) {
        var type = info.typeString || 'unknown';
        var value = info.value;
        var writable = info.isWritable !== false ? tr("Yes") : tr("No (read-only)");
        var notifiable = info.canNotify ? tr("Yes") : tr("No");
        var min = info.min !== undefined ? info.min : '-';
        var max = info.max !== undefined ? info.max : '-';
        var step = info.step !== undefined ? info.step : '-';
        var typeEnum = info.typeEnum || '?';

        var writableClass = (info.isWritable === false) ? 'btn-gen-readonly' : '';

        var html = '<div class="btn-gen-property-info ' + writableClass + '">';
        html += '<h4>' + tr("Property Information") + '</h4>';
        html += '<table class="btn-gen-info-table">';
        html += '<tr><td>' + tr("Name") + ':</td><td><strong>' + escapeHtml(propName) + '</strong></td></tr>';
        html += '<tr><td>' + tr("Type") + ':</td><td><strong>' + escapeHtml(type) + '</strong> (enum: ' + typeEnum + ')</td></tr>';
        html += '<tr><td>' + tr("Current Value") + ':</td><td><code>' + escapeHtml(formatValue(value)) + '</code></td></tr>';
        html += '<tr><td>' + tr("Writable") + ':</td><td><strong>' + writable + '</strong></td></tr>';
        html += '<tr><td>' + tr("Notifiable") + ':</td><td>' + notifiable + '</td></tr>';
        if (min !== '-') html += '<tr><td>' + tr("Min") + ':</td><td>' + min + '</td></tr>';
        if (max !== '-') html += '<tr><td>' + tr("Max") + ':</td><td>' + max + '</td></tr>';
        if (step !== '-') html += '<tr><td>' + tr("Step") + ':</td><td>' + step + '</td></tr>';
        html += '</table>';
        html += '</div>';

        $propertyInfoContainer.html(html).show();
    }

    // ============================================================
    // PREVIEW RENDERING (FIXED - with numeric controls and slider)
    // ============================================================

    function renderActionPreview(action) {
        var isCheckable = action.isCheckable;
        var isChecked = action.isChecked;
        var label = action.text;

        var html = '<div class="btn-gen-preview-box">';
        html += '<div class="btn-gen-preview-label">' + tr("Preview") + '</div>';
        html += '<div class="btn-gen-preview-content">';
        
        var cssClass = 'btn-gen-preview-btn';
        if (isCheckable && isChecked) cssClass += ' active';
        html += '<button class="' + cssClass + '" data-action-id="' + escapeAttr(action.id) + '" ';
        html += 'data-ischeckable="' + isCheckable + '" data-ischecked="' + isChecked + '">';
        if (isCheckable) {
            html += '<span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
                    (isChecked ? '✓' : '✗') + '</span>';
        } else {
            html += '<span class="action-state-icon">▶</span>';
        }
        html += '<span class="action-text">' + escapeHtml(label) + '</span>';
        html += '</button>';
        
        html += '</div>';
        html += '<div class="btn-gen-preview-info">';
        html += '<span class="btn-gen-preview-type">' + (isCheckable ? tr("Toggle") : tr("Trigger")) + '</span>';
        html += '<span class="btn-gen-preview-id">' + escapeHtml(action.id) + '</span>';
        html += '<span class="btn-gen-preview-state">' + tr("State") + ': ' + (isChecked ? 'ON' : 'OFF') + '</span>';
        html += '</div>';
        html += '</div>';
        
        $previewContainer.html(html);
        
        $previewContainer.find('.btn-gen-preview-btn[data-action-id="' + action.id + '"]')
            .off('click.btnGenPreview')
            .on('click.btnGenPreview', function() {
                var id = $(this).data('action-id');
                executeAction(id);
            });
    }

		// ============================================================
		// NUMERIC PROPERTY CONTROLS (FIXED)
		// ============================================================

		// Store numeric values per property
		var numericProperties = {};

		function getNumericValues(propName) {
				if (!numericProperties[propName]) {
						var info = propertyDataCache[propName];
						numericProperties[propName] = {
								min: info && info.min !== undefined ? info.min : 0,
								max: info && info.max !== undefined ? info.max : 100,
								step: info && info.step !== undefined ? info.step : 1
						};
				}
				return numericProperties[propName];
		}

		function updateNumericValues(propName, min, max, step) {
				if (!numericProperties[propName]) {
						numericProperties[propName] = {};
				}
				if (min !== undefined) numericProperties[propName].min = parseFloat(min) || 0;
				if (max !== undefined) numericProperties[propName].max = parseFloat(max) || 100;
				if (step !== undefined) numericProperties[propName].step = parseFloat(step) || 1;
				
				// Update the slider if it exists
				var $slider = $('.btn-gen-preview-slider[data-prop="' + propName + '"]');
				if ($slider.length && $slider.hasClass('ui-slider')) {
						$slider.slider('option', 'min', numericProperties[propName].min);
						$slider.slider('option', 'max', numericProperties[propName].max);
						$slider.slider('option', 'step', numericProperties[propName].step);
				}
				
				// Update custom slider if it exists
				var $customSlider = $('.btn-gen-custom-slider[data-prop="' + propName + '"]');
				if ($customSlider.length && $customSlider.hasClass('ui-slider')) {
						$customSlider.slider('option', 'min', numericProperties[propName].min);
						$customSlider.slider('option', 'max', numericProperties[propName].max);
						$customSlider.slider('option', 'step', numericProperties[propName].step);
				}
				
				// Update the generated code
				var info = propertyDataCache[propName];
				if (info && selectedType === 'property' && selectedPropertyName === propName) {
						generateHtmlCode(propName, info);
				}
		}

    function renderPropertyPreview(propName, info) {
        var type = info.typeString || 'unknown';
        var value = info.value;
        var label = customLabel || propName;
        var isWritable = info.isWritable !== false;
        var typeEnum = info.typeEnum || 0;

        if (!isWritable) {
            var msg = tr("This property is read-only.");
            $previewContainer.html('<div class="btn-gen-placeholder" >' + msg + '</div>');
            return;
        }

        var html = '<div class="btn-gen-preview-box">';
        html += '<div class="btn-gen-preview-label">' + tr("Preview") + '</div>';
        html += '<div class="btn-gen-preview-content">';

        // BOOLEAN
        if (isBooleanProperty(info)) {
            var isChecked = (value === true || value === 'true' || value === 1 || value === '1');
            
            var cssClass = 'btn-gen-preview-btn';
            if (isChecked) cssClass += ' active';
            html += '<button class="' + cssClass + '" data-prop="' + escapeAttr(propName) + '" ';
            html += 'data-ischecked="' + isChecked + '">';
            html += '<span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
                    (isChecked ? '✓' : '✗') + '</span>';
            html += '<span class="action-text">' + escapeHtml(label) + '</span>';
            html += '</button>';
            // NUMERIC - with min/max/step controls
				} else if (isNumericProperty(info)) {
						// Get stored values or defaults
						var numVals = getNumericValues(propName);
						var min = numVals.min;
						var max = numVals.max;
						var step = numVals.step;
						var currentVal = value !== undefined ? value : 0;
						
						html += '<div class="btn-gen-numeric-controls" data-prop="' + escapeAttr(propName) + '">';
						html += '<div class="btn-gen-slider-wrap" data-prop="' + escapeAttr(propName) + '">';
						html += '<div class="btn-gen-slider-header">';
						html += '<label>' + escapeHtml(label) + '</label>';
						html += '<span class="btn-gen-value-display">' + currentVal + '</span>';
						html += '</div>';
						html += '<div class="btn-gen-preview-slider stelproperty" data-prop="' + escapeAttr(propName) + '" ';
						html += 'data-min="' + min + '" data-max="' + max + '" data-step="' + step + '" ';
						html += 'data-value="' + currentVal + '"></div>';
						html += '</div>';
						
						// Min/Max/Step controls
						html += '<div class="btn-gen-slider-controls" style="display:flex; gap:8px; margin-top:6px; flex-wrap:wrap;">';
						html += '<div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '<label style="font-size:9px; color:#8A8C8E;">' + tr("Min") + ':</label>';
						html += '<input type="number" class="btn-gen-numeric-min" data-prop="' + escapeAttr(propName) + '" value="' + min + '" step="any" style="width:55px; padding:3px 5px; background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; border-radius:3px; font-size:10px;">';
						html += '</div>';
						html += '<div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '<label style="font-size:9px; color:#8A8C8E;">' + tr("Max") + ':</label>';
						html += '<input type="number" class="btn-gen-numeric-max" data-prop="' + escapeAttr(propName) + '" value="' + max + '" step="any" style="width:55px; padding:3px 5px; background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; border-radius:3px; font-size:10px;">';
						html += '</div>';
						html += '<div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '<label style="font-size:9px; color:#8A8C8E;">' + tr("Step") + ':</label>';
						html += '<input type="number" class="btn-gen-numeric-step" data-prop="' + escapeAttr(propName) + '" value="' + step + '" step="any" style="width:55px; padding:3px 5px; background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; border-radius:3px; font-size:10px;">';
						html += '</div>';
						html += '</div>';
						html += '</div>';
				
				// COLOR
				} else if (isColorProperty(info, propName)) {

            var colorArray = value;
            if (typeof colorArray === 'string') {
                try { colorArray = JSON.parse(colorArray); } catch(e) { colorArray = [1, 1, 1]; }
            }
            if (!Array.isArray(colorArray) || colorArray.length !== 3) {
                colorArray = [1, 1, 1];
            }
            
            html += '<div class="btn-gen-color-wrap" data-prop="' + escapeAttr(propName) + '">';
            html += '<div class="btn-gen-color-picker">';
            html += '<div class="btn-gen-color-swatch" style="background-color: rgb(' + 
                Math.round(colorArray[0] * 255) + ',' + 
                Math.round(colorArray[1] * 255) + ',' + 
                Math.round(colorArray[2] * 255) + ')" title="' + tr("Click to pick color") + '"></div>';
            html += '<div class="btn-gen-color-inputs">';
            html += '<label>R <input type="number" class="btn-gen-color-input btn-gen-color-r" min="0" max="1" step="0.01" value="' + colorArray[0] + '" /></label>';
            html += '<label>G <input type="number" class="btn-gen-color-input btn-gen-color-g" min="0" max="1" step="0.01" value="' + colorArray[1] + '" /></label>';
            html += '<label>B <input type="number" class="btn-gen-color-input btn-gen-color-b" min="0" max="1" step="0.01" value="' + colorArray[2] + '" /></label>';
            html += '</div>';
            html += '</div>';
            html += '<span class="btn-gen-prop-name">' + escapeHtml(label) + '</span>';
            html += '</div>';
        
				} else {
						// ============================================================
						// QString / Text Property - with input and apply button
						// ============================================================
						var currentTextValue = value !== undefined ? String(value) : '';
						
						html += '<div class="btn-gen-text-wrap" data-prop="' + escapeAttr(propName) + '">';
						html += '    <label>' + escapeHtml(label) + '</label>';
						html += '    <div style="display:flex; gap:6px; flex:1; flex-wrap:wrap;">';
						html += '        <input type="text" class="stelproperty-text" name="' + escapeAttr(propName) + '" ';
						html += '            value="' + escapeAttr(currentTextValue) + '" />';
						html += '        <button class="btn-gen-text-apply" data-prop="' + escapeAttr(propName) + '">';
						html += '            ' + tr("Apply") + '';
						html += '        </button>';
						html += '        <button class="btn-gen-text-reset" data-prop="' + escapeAttr(propName) + '">';
						html += '            ↺';
						html += '        </button>';
						html += '    </div>';
						html += '</div>';
				}

        html += '</div>';
        html += '<div class="btn-gen-preview-info">';
        html += '<span class="btn-gen-preview-type">' + escapeHtml(type) + '</span>';
        html += '<span class="btn-gen-preview-id">' + escapeHtml(propName) + '</span>';
        if (value !== undefined) {
            html += '<span class="btn-gen-preview-value">' + tr("Value") + ': ' + escapeHtml(formatValue(value)) + '</span>';
        }
        html += '</div>';
        html += '</div>';
        
        $previewContainer.html(html);
        
    // ============================================================
    // BIND EVENTS
    // ============================================================
    
    // Boolean toggle button
    $previewContainer.find('.btn-gen-preview-btn[data-prop="' + propName + '"]')
        .off('click.btnGenPreview')
        .on('click.btnGenPreview', function() {
            var prop = $(this).data('prop');
            toggleBooleanProperty(prop);
        });
    
    // Numeric slider
    $previewContainer.find('.btn-gen-preview-slider[data-prop="' + propName + '"]')
        .off('slide.btnGenPreview')
        .on('slide.btnGenPreview', function(evt, ui) {
            var prop = $(this).data('prop');
            // Store the current value in the display
            var $display = $(this).closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
            if ($display.length) {
                $display.text(ui.value);
            }
            // Update the property
            updateProperty(prop, ui.value);
        });
    
    // Initialize slider with stored values
    $previewContainer.find('.btn-gen-preview-slider[data-prop="' + propName + '"]').each(function() {
        var $slider = $(this);
        var numVals = getNumericValues(propName);
        var min = numVals.min;
        var max = numVals.max;
        var step = numVals.step;
        var value = parseFloat($slider.data('value')) || 0;
        
        $slider.slider({
            min: min,
            max: max,
            step: step,
            value: value,
            slide: function(evt, ui) {
                var prop = $(this).data('prop');
                var $display = $(this).closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                if ($display.length) {
                    $display.text(ui.value);
                }
                updateProperty(prop, ui.value);
            }
        });
    });
    
    // ============================================================
    // BIND MIN/MAX/STEP INPUT EVENTS (FIXED)
    // ============================================================
    
    $previewContainer.find('.btn-gen-numeric-min, .btn-gen-numeric-max, .btn-gen-numeric-step')
        .off('input.btnGenNumeric')
        .on('input.btnGenNumeric', function() {
            var $input = $(this);
            var propName2 = $input.data('prop');
            if (!propName2) return;
            
            var $wrap = $input.closest('.btn-gen-numeric-controls');
            var minVal = parseFloat($wrap.find('.btn-gen-numeric-min').val());
            var maxVal = parseFloat($wrap.find('.btn-gen-numeric-max').val());
            var stepVal = parseFloat($wrap.find('.btn-gen-numeric-step').val());
            
            // Validate values
            if (isNaN(minVal)) minVal = 0;
            if (isNaN(maxVal)) maxVal = 100;
            if (isNaN(stepVal)) stepVal = 1;
            if (minVal > maxVal) {
                // Swap if min > max
                var temp = minVal;
                minVal = maxVal;
                maxVal = temp;
                $wrap.find('.btn-gen-numeric-min').val(minVal);
                $wrap.find('.btn-gen-numeric-max').val(maxVal);
            }
            if (stepVal <= 0) stepVal = 1;
            
            // Update stored values
            updateNumericValues(propName2, minVal, maxVal, stepVal);
            
            // Update the slider
            var $slider2 = $('.btn-gen-preview-slider[data-prop="' + propName2 + '"]');
            if ($slider2.length && $slider2.hasClass('ui-slider')) {
                $slider2.slider('option', 'min', minVal);
                $slider2.slider('option', 'max', maxVal);
                $slider2.slider('option', 'step', stepVal);
            }
            
            // Update generated code
            var info2 = propertyDataCache[propName2];
            if (info2 && selectedType === 'property' && selectedPropertyName === propName2) {
                generateHtmlCode(propName2, info2);
            }
        });
        
        // Color picker
        $previewContainer.find('.btn-gen-color-wrap .btn-gen-color-input')
            .off('input.btnGenPreview')
            .on('input.btnGenPreview', function() {
                var $wrap = $(this).closest('.btn-gen-color-wrap');
                var propName2 = $wrap.data('prop');
                var r = parseFloat($wrap.find('.btn-gen-color-r').val()) || 0;
                var g = parseFloat($wrap.find('.btn-gen-color-g').val()) || 0;
                var b = parseFloat($wrap.find('.btn-gen-color-b').val()) || 0;
                
                r = Math.max(0, Math.min(1, r));
                g = Math.max(0, Math.min(1, g));
                b = Math.max(0, Math.min(1, b));
                
                $wrap.find('.btn-gen-color-swatch').css('background-color', 'rgb(' + 
                    Math.round(r * 255) + ',' + 
                    Math.round(g * 255) + ',' + 
                    Math.round(b * 255) + ')'
                );
                
                if (propName2) {
                    updateProperty(propName2, [r, g, b]);
                }
            });
        
        $previewContainer.find('.btn-gen-color-swatch')
            .off('click.btnGenPreview')
            .on('click.btnGenPreview', function() {
                var $wrap = $(this).closest('.btn-gen-color-wrap');
                var r = parseFloat($wrap.find('.btn-gen-color-r').val()) || 0;
                var g = parseFloat($wrap.find('.btn-gen-color-g').val()) || 0;
                var b = parseFloat($wrap.find('.btn-gen-color-b').val()) || 0;
                var propName2 = $wrap.data('prop');
                
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
                    $wrap.find('.btn-gen-color-r').val(r2);
                    $wrap.find('.btn-gen-color-g').val(g2);
                    $wrap.find('.btn-gen-color-b').val(b2);
                    $wrap.find('.btn-gen-color-swatch').css('background-color', hexVal);
                    if (propName2) {
                        updateProperty(propName2, [r2, g2, b2]);
                    }
                });
                input.click();
            });
						
						// ============================================================
						// TEXT PROPERTY - Bind events for QString
						// ============================================================
						$previewContainer.find('.btn-gen-text-apply[data-prop="' + propName + '"]')
								.off('click.btnGenText')
								.on('click.btnGenText', function() {
										var prop = $(this).data('prop');
										var $wrap = $(this).closest('.btn-gen-text-wrap');
										var $input = $wrap.find('input[type="text"]');
										var newValue = $input.val();
										if (newValue !== undefined) {
												updateProperty(prop, newValue);
												updateStatus("Text property updated: " + prop);
										}
								});

						// Apply on Enter key
						$previewContainer.find('.btn-gen-text-wrap input[type="text"][name="' + propName + '"]')
								.off('keydown.btnGenText')
								.on('keydown.btnGenText', function(e) {
										if (e.key === 'Enter') {
												e.preventDefault();
												var prop = $(this).attr('name');
												var newValue = $(this).val();
												if (newValue !== undefined) {
														updateProperty(prop, newValue);
														updateStatus("Text property updated: " + prop);
												}
										}
								});

						// Reset button - restore original value from cache
						$previewContainer.find('.btn-gen-text-reset[data-prop="' + propName + '"]')
								.off('click.btnGenTextReset')
								.on('click.btnGenTextReset', function() {
										var prop = $(this).data('prop');
										var info = propertyDataCache[prop];
										if (info && info.value !== undefined) {
												var $wrap = $(this).closest('.btn-gen-text-wrap');
												$wrap.find('input[type="text"]').val(String(info.value));
												updateStatus("Reset to original value");
										}
								});
    }

    // ============================================================
    // HTML CODE GENERATION (with min/max/step)
		// (FIXED - Uses existing Stellarium system)
		// ============================================================

		function generateHtmlCode(actionOrProp, info) {
				var html = '';
				var explanation = '';
				var htmlCheckbox = '';
				var htmlButton = '';
				
				if (selectedType === 'action') {
						var action = actionOrProp;
						var isCheckable = action.isCheckable;
						var isChecked = action.isChecked;
						
						// ============================================================
						// VERSION 1: CHECKBOX (for toggleable actions)
						// ============================================================
						if (isCheckable) {
								htmlCheckbox = '<!-- Checkbox for StelAction: ' + action.id + ' -->\n';
								htmlCheckbox += '<label class="btn-gen-preview-toggle">\n';
								htmlCheckbox += '    <input type="checkbox" class="stelaction" name="' + action.id + '" ' + 
															 (isChecked ? 'checked' : '') + ' />\n';
								htmlCheckbox += '    ' + action.text + '\n';
								htmlCheckbox += '</label>';
						} else {
								htmlCheckbox = '<!-- Trigger actions cannot be checkboxes -->\n';
								htmlCheckbox += '<!-- Use the button version below -->';
						}
						
						// ============================================================
						// VERSION 2: BUTTON (for all actions)
						// ============================================================
						htmlButton = '<!-- Button for StelAction: ' + action.id + ' -->\n';
						htmlButton += '<button class="btn-gen-preview-btn action-trigger" data-action-id="' + action.id + '" ';
						htmlButton += 'onclick="require([\'api/actions\'], function(a) { a.execute(\'' + action.id + '\'); })">\n';
						if (isCheckable) {
								htmlButton += '    <span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
														 (isChecked ? '✓' : '✗') + '</span>\n';
						} else {
								htmlButton += '    <span class="action-state-icon">▶</span>\n';
						}
						htmlButton += '    <span class="action-text">' + action.text + '</span>\n';
						htmlButton += '</button>';
						
						html = '<!-- Two versions available -->\n\n' +
									 '<!-- VERSION 1: Checkbox (toggleable actions) -->\n' + 
									 htmlCheckbox + '\n\n' +
									 '<!-- VERSION 2: Button (all actions) -->\n' +
									 htmlButton;
						
						explanation = '<p><strong>' + escapeHtml(action.text) + '</strong></p>';
						explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(action.id) + '</code></p>';
						explanation += '<p><strong>Type:</strong> ' + (isCheckable ? 'Toggle' : 'Trigger') + '</p>';
						explanation += '<p><strong>Current State:</strong> ' + (isChecked ? 'ON' : 'OFF') + '</p>';
						explanation += '<p><strong>Usage:</strong> Choose either version. Checkbox works only for toggleable actions.</p>';
						
				} else if (selectedType === 'property') {
						var propName = selectedPropertyName;
						var infoData = info;
						var label = customLabel || propName;
						var typeEnum = infoData.typeEnum || 0;
						var isBool = isBooleanProperty(infoData);
						var isNum = isNumericProperty(infoData);
						var isColor = isColorProperty(infoData, propName);
						var currentVal = infoData.value;
						
						// ============================================================
						// BOOLEAN PROPERTY
						// ============================================================
						if (isBool) {
								var isChecked = (currentVal === true || currentVal === 'true' || currentVal === 1 || currentVal === '1');
								
								// VERSION 1: Checkbox (uses existing stelproperty system)
								htmlCheckbox = '<!-- Checkbox for StelProperty: ' + propName + ' -->\n';
								htmlCheckbox += '<label class="btn-gen-preview-toggle">\n';
								htmlCheckbox += '    <input type="checkbox" class="stelproperty" name="' + propName + '" ' + 
															 (isChecked ? 'checked' : '') + ' />\n';
								htmlCheckbox += '    ' + label + '\n';
								htmlCheckbox += '</label>';
								
								// VERSION 2: Button (custom toggle)
								htmlButton = '<!-- Button for StelProperty: ' + propName + ' -->\n';
								htmlButton += '<button class="btn-gen-preview-btn" data-prop="' + propName + '" ';
								htmlButton += 'data-ischecked="' + (isChecked ? 'true' : 'false') + '" ';
								htmlButton += 'onclick="toggleBooleanProperty(\'' + propName + '\')">\n';
								htmlButton += '    <span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
														 (isChecked ? '✓' : '✗') + '</span>\n';
								htmlButton += '    <span class="action-text">' + label + '</span>\n';
								htmlButton += '</button>';
								
								html = '<!-- Two versions available -->\n\n' +
											 '<!-- VERSION 1: Checkbox (RECOMMENDED - uses existing system) -->\n' + 
											 htmlCheckbox + '\n\n' +
											 '<!-- VERSION 2: Button (requires toggleBooleanProperty function) -->\n' +
											 htmlButton + '\n\n' +
											 '<!-- IMPORTANT: For the button to work, add this to your page: -->\n' +
											 '<!-- <script>\n' +
											 '  function toggleBooleanProperty(prop) {\n' +
											 '    require(["api/properties"], function(p) {\n' +
											 '      var current = p.getStelProp(prop);\n' +
											 '      p.setStelProp(prop, !current);\n' +
											 '    });\n' +
											 '  }\n' +
											 ' </script> -->';
								
						// ============================================================
						// NUMERIC PROPERTY
						// ============================================================
						} else if (isNum) {
								var numVals = getNumericValues(propName);
								var min = numVals.min;
								var max = numVals.max;
								var step = numVals.step;
								var value = currentVal !== undefined ? currentVal : 0;
								
								// VERSION 1: Slider (like Sky Culture tab)
								htmlCheckbox = '<!-- Slider for StelProperty: ' + propName + ' -->\n';
								htmlCheckbox += '<div class="btn-gen-slider-wrap">\n';
								htmlCheckbox += '    <div class="btn-gen-slider-header">\n';
								htmlCheckbox += '        <label>' + label + '</label>\n';
								htmlCheckbox += '        <span class="btn-gen-value-display">' + value + '</span>\n';
								htmlCheckbox += '    </div>\n';
								htmlCheckbox += '    <div class="slider stelproperty" data-prop="' + propName + '" ';
								htmlCheckbox += 'data-min="' + min + '" data-max="' + max + '" data-step="' + step + '" ';
								htmlCheckbox += 'data-value="' + value + '"></div>\n';
								htmlCheckbox += '</div>';
								
								// VERSION 2: Spinner (like time tab)
								htmlButton = '<!-- Spinner for StelProperty: ' + propName + ' -->\n';
								htmlButton += '<div class="btn-gen-slider-wrap">\n';
								htmlButton += '    <label>' + label + '</label>\n';
								htmlButton += '    <input class="spinner stelproperty" name="' + propName + '" ';
								htmlButton += 'data-min="' + min + '" data-max="' + max + '" data-step="' + step + '" ';
								htmlButton += 'value="' + value + '" />\n';
								htmlButton += '</div>';
								
								html = '<!-- Two versions available -->\n\n' +
											 '<!-- VERSION 1: Slider (RECOMMENDED - like Sky Culture tab) -->\n' + 
											 htmlCheckbox + '\n\n' +
											 '<!-- VERSION 2: Spinner (like Time tab) -->\n' +
											 htmlButton + '\n\n' +
											 '<!-- Range: min=' + min + ', max=' + max + ', step=' + step + ' -->';
								
						// ============================================================
						// COLOR PROPERTY
						// ============================================================
						} else if (isColor) {
								var colorArray = currentVal;
								if (typeof colorArray === 'string') {
										try { colorArray = JSON.parse(colorArray); } catch(e) { colorArray = [1, 1, 1]; }
								}
								if (!Array.isArray(colorArray) || colorArray.length !== 3) {
										colorArray = [1, 1, 1];
								}
								
								html = '<!-- Color Picker for StelProperty: ' + propName + ' -->\n';
								html += '<div class="btn-gen-color-wrap">\n';
								html += '    <div class="btn-gen-color-picker">\n';
								html += '        <div class="btn-gen-color-swatch" style="background-color: rgb(' + 
										Math.round(colorArray[0] * 255) + ',' + 
										Math.round(colorArray[1] * 255) + ',' + 
										Math.round(colorArray[2] * 255) + ');" title="Click to pick color"></div>\n';
								html += '        <div class="btn-gen-color-inputs">\n';
								html += '            <label>R <input type="number" class="btn-gen-color-input btn-gen-color-r" min="0" max="1" step="0.01" value="' + colorArray[0] + '" /></label>\n';
								html += '            <label>G <input type="number" class="btn-gen-color-input btn-gen-color-g" min="0" max="1" step="0.01" value="' + colorArray[1] + '" /></label>\n';
								html += '            <label>B <input type="number" class="btn-gen-color-input btn-gen-color-b" min="0" max="1" step="0.01" value="' + colorArray[2] + '" /></label>\n';
								html += '        </div>\n';
								html += '    </div>\n';
								html += '    <span class="btn-gen-prop-name">' + label + '</span>\n';
								html += '</div>\n\n';
								html += '<!-- IMPORTANT: Add this JavaScript to your page for color picker support: -->\n';
								html += '<!-- <script>\n';
								html += '  $(document).on("input", ".btn-gen-color-input", function() {\n';
								html += '    var $wrap = $(this).closest(".btn-gen-color-wrap");\n';
								html += '    var prop = $wrap.data("prop");\n';
								html += '    var r = parseFloat($wrap.find(".btn-gen-color-r").val()) || 0;\n';
								html += '    var g = parseFloat($wrap.find(".btn-gen-color-g").val()) || 0;\n';
								html += '    var b = parseFloat($wrap.find(".btn-gen-color-b").val()) || 0;\n';
								html += '    r = Math.max(0, Math.min(1, r));\n';
								html += '    g = Math.max(0, Math.min(1, g));\n';
								html += '    b = Math.max(0, Math.min(1, b));\n';
								html += '    $wrap.find(".btn-gen-color-swatch").css("background-color", "rgb(" + \n';
								html += '      Math.round(r * 255) + "," + \n';
								html += '      Math.round(g * 255) + "," + \n';
								html += '      Math.round(b * 255) + ")"\n';
								html += '    );\n';
								html += '    require(["api/properties"], function(p) { p.setStelProp(prop, "[" + r + ", " + g + ", " + b + "]"); });\n';
								html += '  });\n';
								html += '  $(document).on("click", ".btn-gen-color-swatch", function() {\n';
								html += '    var $wrap = $(this).closest(".btn-gen-color-wrap");\n';
								html += '    var prop = $wrap.data("prop");\n';
								html += '    var r = parseFloat($wrap.find(".btn-gen-color-r").val()) || 0;\n';
								html += '    var g = parseFloat($wrap.find(".btn-gen-color-g").val()) || 0;\n';
								html += '    var b = parseFloat($wrap.find(".btn-gen-color-b").val()) || 0;\n';
								html += '    var hex = "#" + \n';
								html += '      Math.round(r * 255).toString(16).padStart(2, "0") +\n';
								html += '      Math.round(g * 255).toString(16).padStart(2, "0") +\n';
								html += '      Math.round(b * 255).toString(16).padStart(2, "0");\n';
								html += '    var input = document.createElement("input");\n';
								html += '    input.type = "color";\n';
								html += '    input.value = hex;\n';
								html += '    input.addEventListener("input", function() {\n';
								html += '      var hexVal = this.value;\n';
								html += '      var r2 = parseInt(hexVal.substring(1,3), 16) / 255;\n';
								html += '      var g2 = parseInt(hexVal.substring(3,5), 16) / 255;\n';
								html += '      var b2 = parseInt(hexVal.substring(5,7), 16) / 255;\n';
								html += '      $wrap.find(".btn-gen-color-r").val(r2);\n';
								html += '      $wrap.find(".btn-gen-color-g").val(g2);\n';
								html += '      $wrap.find(".btn-gen-color-b").val(b2);\n';
								html += '      $wrap.find(".btn-gen-color-swatch").css("background-color", hexVal);\n';
								html += '      require(["api/properties"], function(p) { p.setStelProp(prop, "[" + r2 + ", " + g2 + ", " + b2 + "]"); });\n';
								html += '    });\n';
								html += '    input.click();\n';
								html += '  });\n';
								html += ' </script> -->';
						
						// ============================================================
						// OTHER PROPERTY TYPES
						
						// ============================================================
						// TEXT PROPERTY (QString) - HTML generation
						// ============================================================
						} else {
								var currentTextValue = infoData.value !== undefined ? String(infoData.value) : '';
								
								html = '<!-- Text Input for StelProperty: ' + propName + ' -->\n';
								html += '<div class="btn-gen-text-wrap">\n';
								html += '    <label>' + escapeHtml(label) + '</label>\n';
								html += '    <div style="display:flex; gap:6px; flex:1; flex-wrap:wrap;">\n';
								html += '        <input type="text" class="stelproperty-text" name="' + escapeAttr(propName) + '" ';
								html += '            value="' + escapeAttr(currentTextValue) + '" ';
								html += '            style="flex:1; min-width:120px; padding:4px 8px; background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; border-radius:3px;" />\n';
								html += '        <button class="btn-gen-text-apply" data-prop="' + escapeAttr(propName) + '" ';
								html += '            style="padding:4px 12px; background:linear-gradient(#5D5F62, #3A3C3E); border:1px solid #2A2C2E; border-radius:3px; color:#000; font-weight:bold; cursor:pointer;">\n';
								html += '            ' + tr("Apply") + '\n';
								html += '        </button>\n';
								html += '        <button class="btn-gen-text-reset" data-prop="' + escapeAttr(propName) + '" ';
								html += '            style="padding:4px 12px; background:#2A2C2E; border:1px solid #5D5F62; border-radius:3px; color:#DCDBDA; cursor:pointer;">\n';
								html += '            ↺\n';
								html += '        </button>\n';
								html += '    </div>\n';
								html += '</div>\n';
								html += '\n';
								html += '<!-- For the buttons to work, add this JavaScript: -->\n';
								html += '<!-- <script>\n';
								html += '  $(document).on("click", ".btn-gen-text-apply", function() {\n';
								html += '    var prop = $(this).data("prop");\n';
								html += '    var $wrap = $(this).closest(".btn-gen-text-wrap");\n';
								html += '    var value = $wrap.find("input[type=\'text\']").val();\n';
								html += '    require(["api/properties"], function(p) { p.setStelProp(prop, value); });\n';
								html += '  });\n';
								html += '  $(document).on("keydown", ".stelproperty-text", function(e) {\n';
								html += '    if (e.key === "Enter") {\n';
								html += '      e.preventDefault();\n';
								html += '      var prop = $(this).attr("name");\n';
								html += '      var value = $(this).val();\n';
								html += '      require(["api/properties"], function(p) { p.setStelProp(prop, value); });\n';
								html += '    }\n';
								html += '  });\n';
								html += ' </script> -->';
						}
						
						// Explanation for properties
						var valueDisplay = currentVal !== undefined ? formatValue(currentVal) : 'undefined';
						explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
						explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
						explanation += '<p><strong>Type:</strong> ' + escapeHtml(infoData.typeString || 'unknown') + '</p>';
						explanation += '<p><strong>Current Value:</strong> <code>' + escapeHtml(valueDisplay) + '</code></p>';
						explanation += '<p><strong>Writable:</strong> ' + (infoData.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
						explanation += '<p><strong>Usage:</strong> Copy the HTML and paste it anywhere in your page.</p>';
						if (isBool) {
								explanation += '<p><strong>Note:</strong> The checkbox version uses the existing Stellarium system. ';
								explanation += 'The button version requires the toggleBooleanProperty function.</p>';
						}
						if (isNum) {
								var numVals2 = getNumericValues(propName);
								explanation += '<p><strong>Range:</strong> min=' + numVals2.min + ', max=' + numVals2.max + ', step=' + numVals2.step + '</p>';
						}
				}
				
				// Update the code display
				var $codeElement = $codeContainer.find('.btn-gen-code');
				if ($codeElement.length) {
						$codeElement.text(html);
				} else {
						$codeContainer.html('<pre class="btn-gen-code">' + escapeHtml(html) + '</pre>');
				}
				
				// Update explanation
				var $explanation = $codeContainer.find('.btn-gen-code-explanation');
				if ($explanation.length) {
						$explanation.html(explanation);
				} else if (explanation) {
						$codeContainer.append('<div class="btn-gen-code-explanation">' + explanation + '</div>');
				}
		}

    // ============================================================
    // COPY TO CLIPBOARD
    // ============================================================

    function copyToClipboard(text) {
        if (navigator.clipboard && navigator.clipboard.writeText) {
            navigator.clipboard.writeText(text).then(function() {
                showCopyFeedback(true);
            }).catch(function() {
                fallbackCopy(text);
            });
        } else {
            fallbackCopy(text);
        }
    }

    function fallbackCopy(text) {
        var textarea = document.createElement('textarea');
        textarea.value = text;
        textarea.style.position = 'fixed';
        textarea.style.opacity = '0';
        document.body.appendChild(textarea);
        textarea.select();
        try {
            document.execCommand('copy');
            showCopyFeedback(true);
        } catch(e) {
            showCopyFeedback(false);
        }
        document.body.removeChild(textarea);
    }

    function showCopyFeedback(success) {
        var originalText = $copyHtmlBtn.text();
        $copyHtmlBtn.text(success ? '✓ ' + tr("Copied!") : '✗ ' + tr("Failed"));
        setTimeout(function() {
            $copyHtmlBtn.text(tr("Copy HTML"));
        }, 2000);
    }

    // ============================================================
    // CUSTOM BUTTONS MANAGEMENT
    // ============================================================

		function addCustomButton(type, data, silent) {
				if (type === 'action') {
						var exists = customButtons.some(function(b) { 
								return b.type === 'action' && b.id === data.id; 
						});
						if (exists) {
								if (!silent) alert(tr("This action is already added"));
								return;
						}
						customButtons.push({
								type: 'action',
								id: data.id,
								text: data.text,
								isCheckable: data.isCheckable,
								isChecked: data.isChecked,
								label: data.text
						});
				} else if (type === 'property') {
						var info = data.info;
						if (info.isWritable === false) {
								if (!silent) alert(tr("This property is read-only and cannot be added as a control."));
								return;
						}
						var exists = customButtons.some(function(b) { 
								return b.type === 'property' && b.id === data.id; 
						});
						if (exists) {
								if (!silent) alert(tr("This property is already added"));
								return;
						}
						
						// Get the current min/max/step values from storage
						var numVals = getNumericValues(data.id);
						var minVal = numVals.min;
						var maxVal = numVals.max;
						var stepVal = numVals.step;
						
						// Override with custom values if provided
						if (data.min !== undefined) minVal = data.min;
						if (data.max !== undefined) maxVal = data.max;
						if (data.step !== undefined) stepVal = data.step;
						
						// ============================================================
						// FIXED: Only ONE push, not two
						// ============================================================
						customButtons.push({
								type: 'property',
								id: data.id,
								typeString: info.typeString || 'unknown',
								typeEnum: info.typeEnum || 0,
								isWritable: info.isWritable || false,
								value: info.value,
								label: data.label || data.id,
								min: minVal,
								max: maxVal,
								step: stepVal
						});
				}

				saveCustomButtons();
				renderCustomButtons();
				updateCustomCount();
				if (!silent) {
						log("Added: " + (data.label || data.id));
						updateStatus("Added: " + (data.label || data.id));
				}
		}

		function removeCustomButton(index) {
				// Prevent duplicate removal
				if (index < 0 || index >= customButtons.length) {
						logError("Invalid index for removal: " + index);
						return;
				}
				
				// Remove the button
				customButtons.splice(index, 1);
				saveCustomButtons();
				renderCustomButtons();
				updateCustomCount();
				updateStatus("Button removed");
		}

    function editCustomButtonLabel(index, newLabel) {
        if (customButtons[index]) {
            customButtons[index].label = newLabel;
            saveCustomButtons();
            renderCustomButtons();
            updateStatus("Label updated");
        }
    }

		// ============================================================
		// RENDER CUSTOM BUTTONS (FIXED - Prevents duplicate rendering)
		// ============================================================

		function renderCustomButtons() {
				// Prevent concurrent rendering
				if (isRendering) {
						log("Already rendering, skipping duplicate call");
						return;
				}
				isRendering = true;
				
				try {
						if (!customButtons.length) {
								$customContainer.html('<div class="btn-gen-empty-state">' + tr("No custom buttons yet. Add some from the generator above.") + '</div>');
								updateCustomCount();
								isRendering = false;
								return;
						}

						// Remove all existing event listeners before re-rendering
						$customContainer.find('.btn-gen-custom-item').off();
						
						var html = '<div class="btn-gen-custom-grid">';
						
						customButtons.forEach(function(btn, index) {
								html += '<div class="btn-gen-custom-item" data-index="' + index + '" data-prop="' + (btn.type === 'property' ? escapeAttr(btn.id) : '') + '">';
								html += '<div class="btn-gen-custom-header">';
								html += '<span class="btn-gen-custom-type">' + (btn.type === 'action' ? '[A]' : '[P]') + '</span>';
								html += '<span class="btn-gen-custom-label">' + escapeHtml(btn.label) + '</span>';
								html += '<button class="btn-gen-custom-edit" data-index="' + index + '" title="' + tr("Edit label") + '">Edit</button>';
								html += '<button class="btn-gen-custom-remove" data-index="' + index + '" title="' + tr("Remove") + '">✕</button>';
								html += '</div>';
								html += '<div class="btn-gen-custom-control">';
								
								if (btn.type === 'action') {
										html += renderActionControl(btn);
								} else if (btn.type === 'property') {
										html += renderPropertyControl(btn);
								}
								
								html += '</div>';
								html += '</div>';
						});

						html += '</div>';
						$customContainer.html(html);

						// ============================================================
						// BIND EVENTS FOR CUSTOM BUTTONS (ONLY ONCE)
						// ============================================================
						
						// Remove button - using delegated event to ensure single binding
						$customContainer.off('click.btnGenCustomRemove').on('click.btnGenCustomRemove', '.btn-gen-custom-remove', function() {
								var index = parseInt($(this).data('index'));
								removeCustomButton(index);
						});

						// Edit label button
						$customContainer.off('click.btnGenCustomEdit').on('click.btnGenCustomEdit', '.btn-gen-custom-edit', function() {
								var index = parseInt($(this).data('index'));
								var currentLabel = customButtons[index] ? customButtons[index].label : '';
								var newLabel = prompt(tr("Enter new label:"), currentLabel);
								if (newLabel !== null && newLabel.trim() !== '') {
										editCustomButtonLabel(index, newLabel.trim());
								}
						});

						// Action toggle buttons
						$customContainer.off('click.btnGenCustomAction').on('click.btnGenCustomAction', '.btn-gen-custom-toggle-btn[data-action-id]', function() {
								var actionId = $(this).data('action-id');
								if (actionId) {
										executeAction(actionId);
								}
						});

						// Property toggle buttons
						$customContainer.off('click.btnGenCustomProp').on('click.btnGenCustomProp', '.btn-gen-custom-toggle-btn[data-prop]', function() {
								var propName = $(this).data('prop');
								if (propName) {
									  // Use the dedicated toggle function
										toggleBooleanProperty(propName);
								}
						});

						// Property sliders
						$customContainer.off('slide.btnGenCustomSlider').on('slide.btnGenCustomSlider', '.btn-gen-custom-slider', function(evt, ui) {
								var prop = $(this).data('prop');
								updateProperty(prop, ui.value);
								var $display = $(this).closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
								if ($display.length) {
										$display.text(ui.value);
								}
						});

						// Initialize custom sliders
						$customContainer.find('.btn-gen-custom-slider').each(function() {
								var $slider = $(this);
								// Skip if already initialized
								if ($slider.hasClass('ui-slider')) {
										return;
								}
								
								var prop = $slider.data('prop');
								var info = propertyDataCache[prop];
								if (!info) return;
								
								// Find the button data to get min/max/step
								var btnData = null;
								for (var i = 0; i < customButtons.length; i++) {
										if (customButtons[i].id === prop) {
												btnData = customButtons[i];
												break;
										}
								}
								
								var min = parseFloat($slider.data('min')) || (btnData ? btnData.min : (info.min !== undefined ? info.min : 0));
								var max = parseFloat($slider.data('max')) || (btnData ? btnData.max : (info.max !== undefined ? info.max : 100));
								var step = parseFloat($slider.data('step')) || (btnData ? btnData.step : (info.step !== undefined ? info.step : 1));
								var value = parseFloat($slider.data('value')) || (info.value !== undefined ? info.value : 0);
								
								$slider.slider({
										min: min,
										max: max,
										step: step,
										value: value,
										slide: function(evt, ui) {
												var prop2 = $(this).data('prop');
												updateProperty(prop2, ui.value);
												var $display = $(this).closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
												if ($display.length) {
														$display.text(ui.value);
												}
										}
								});
						});

						// Property color pickers
						$customContainer.off('click.btnGenCustomColor').on('click.btnGenCustomColor', '.btn-gen-custom-color .btn-gen-color-swatch', function() {
								var $item = $(this).closest('.btn-gen-custom-item');
								var propName = $item.data('prop');
								if (propName) {
										var $wrap = $(this).closest('.btn-gen-custom-color');
										var bgColor = $(this).css('background-color');
										var rgb = bgColor.match(/\d+/g);
										if (rgb && rgb.length >= 3) {
												var r = parseInt(rgb[0]) / 255;
												var g = parseInt(rgb[1]) / 255;
												var b = parseInt(rgb[2]) / 255;
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
														$wrap.find('.btn-gen-color-swatch').css('background-color', hexVal);
														if (propName) {
																updateProperty(propName, [r2, g2, b2]);
														}
												});
												input.click();
										}
								}
						});
								
						// Color picker R/G/B inputs in custom section
						$customContainer.off('input.btnGenCustomColorInput').on('input.btnGenCustomColorInput', '.btn-gen-custom-color .btn-gen-color-input', function() {
								var $wrap = $(this).closest('.btn-gen-custom-color');
								var $item = $wrap.closest('.btn-gen-custom-item');
								var propName = $item.data('prop');
								var r = parseFloat($wrap.find('.btn-gen-color-r').val()) || 0;
								var g = parseFloat($wrap.find('.btn-gen-color-g').val()) || 0;
								var b = parseFloat($wrap.find('.btn-gen-color-b').val()) || 0;
								
								r = Math.max(0, Math.min(1, r));
								g = Math.max(0, Math.min(1, g));
								b = Math.max(0, Math.min(1, b));
								
								$wrap.find('.btn-gen-color-swatch').css('background-color', 'rgb(' + 
										Math.round(r * 255) + ',' + 
										Math.round(g * 255) + ',' + 
										Math.round(b * 255) + ')'
								);
								
								if (propName) {
										updateProperty(propName, [r, g, b]);
								}
						});
						
						// Text property - Apply button
						$customContainer.off('click.btnGenCustomText').on('click.btnGenCustomText', '.btn-gen-custom-text-apply', function() {
								var prop = $(this).data('prop');
								var $wrap = $(this).closest('.btn-gen-custom-text');
								var $input = $wrap.find('input[type="text"]');
								var newValue = $input.val();
								if (newValue !== undefined) {
										updateProperty(prop, newValue);
										updateStatus("Text property updated: " + prop);
								}
						});

						// Text property - Enter key
						$customContainer.off('keydown.btnGenCustomText').on('keydown.btnGenCustomText', '.btn-gen-custom-text input[type="text"]', function(e) {
								if (e.key === 'Enter') {
										e.preventDefault();
										var prop = $(this).attr('name');
										var newValue = $(this).val();
										if (newValue !== undefined) {
												updateProperty(prop, newValue);
												updateStatus("Text property updated: " + prop);
										}
								}
						});
						
				} catch (error) {
						logError("Error rendering custom buttons: " + error.message);
				}
				
				isRendering = false;
		}

    function renderActionControl(btn) {
        var isChecked = btn.isChecked || false;
        var isCheckable = btn.isCheckable || false;
        var id = btn.id;
        var label = btn.label;

        var cssClass = 'btn-gen-custom-toggle-btn';
        if (isCheckable && isChecked) cssClass += ' active';
        if (!isCheckable) cssClass += ' action-trigger';
        
        var html = '<button class="' + cssClass + '" data-action-id="' + escapeAttr(id) + '" ';
        html += 'data-ischeckable="' + isCheckable + '" data-ischecked="' + isChecked + '">';
        if (isCheckable) {
            html += '<span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
                    (isChecked ? '✓' : '✗') + '</span>';
        } else {
            html += '<span class="action-state-icon">▶</span>';
        }
        html += '<span class="action-text">' + escapeHtml(label) + '</span>';
        html += '</button>';
        
        return html;
    }

    function renderPropertyControl(btn) {
        var type = btn.typeString || 'unknown';
        var id = btn.id;
        var label = btn.label;
        var value = btn.value;
        var typeEnum = btn.typeEnum || 0;

        // BOOLEAN - Toggle Button
        if (typeEnum === TYPE_ENUMS.BOOL || type === 'bool') {
            var isChecked = (value === true || value === 'true' || value === 1 || value === '1');
            var cssClass = 'btn-gen-custom-toggle-btn';
            if (isChecked) cssClass += ' active';
            
            var html = '<button class="' + cssClass + '" data-prop="' + escapeAttr(id) + '" ';
            html += 'data-ischecked="' + isChecked + '">';
            html += '<span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' + 
                    (isChecked ? '✓' : '✗') + '</span>';
            html += '<span class="action-text">' + escapeHtml(label) + '</span>';
            html += '</button>';
            return html;
        
        // NUMERIC - Slider
				} else if ((typeEnum >= TYPE_ENUMS.INT && typeEnum <= TYPE_ENUMS.DOUBLE) || 
									 typeEnum === TYPE_ENUMS.FLOAT ||
									 type === 'int' || type === 'double' || type === 'float') {
						// Use stored values from the button object
						var min = btn.min !== undefined ? btn.min : 0;
						var max = btn.max !== undefined ? btn.max : 100;
						var step = btn.step !== undefined ? btn.step : 1;
						var currentVal = value !== undefined ? value : 0;
						
						var html = '<div class="btn-gen-custom-slider-wrap" data-prop="' + escapeAttr(id) + '">';
						html += '<div class="btn-gen-slider-header">';
						html += '<label>' + escapeHtml(label) + '</label>';
						html += '<span class="btn-gen-custom-value">' + currentVal + '</span>';
						html += '</div>';
						html += '<div class="btn-gen-custom-slider" data-prop="' + escapeAttr(id) + '" ';
						html += 'data-min="' + min + '" data-max="' + max + '" data-step="' + step + '" ';
						html += 'data-value="' + currentVal + '"></div>';
						html += '</div>';
						return html;
        
        // COLOR - Color Picker (FIXED - with data-prop on container)
        } else if ((typeEnum === TYPE_ENUMS.VECTOR3 || type === 'Vector3<float>' || type === 'Vector3') && 
                   id.toLowerCase().includes('color')) {
            var colorArray = value;
            if (typeof colorArray === 'string') {
                try { colorArray = JSON.parse(colorArray); } catch(e) { colorArray = [1, 1, 1]; }
            }
            if (!Array.isArray(colorArray) || colorArray.length !== 3) {
                colorArray = [1, 1, 1];
            }
            
            var html = '<div class="btn-gen-custom-color" data-prop="' + escapeAttr(id) + '">';
            html += '<div class="btn-gen-color-picker">';
            html += '<div class="btn-gen-color-swatch" style="background-color: rgb(' + 
                Math.round(colorArray[0] * 255) + ',' + 
                Math.round(colorArray[1] * 255) + ',' + 
                Math.round(colorArray[2] * 255) + ');"></div>';
            html += '<div class="btn-gen-color-inputs">';
            html += '<label>R <input type="number" class="btn-gen-color-input btn-gen-color-r" min="0" max="1" step="0.01" value="' + colorArray[0] + '" /></label>';
            html += '<label>G <input type="number" class="btn-gen-color-input btn-gen-color-g" min="0" max="1" step="0.01" value="' + colorArray[1] + '" /></label>';
            html += '<label>B <input type="number" class="btn-gen-color-input btn-gen-color-b" min="0" max="1" step="0.01" value="' + colorArray[2] + '" /></label>';
            html += '</div>';
            html += '</div>';
            html += '<span>' + escapeHtml(label) + '</span>';
            html += '</div>';
            return html;
        
        // OTHER - Text input
				// OTHER - Text input with apply button
				} else {
						var currentTextValue = value !== undefined ? String(value) : '';
						
						var html = '<div class="btn-gen-custom-text" data-prop="' + escapeAttr(id) + '">';
						html += '    <label>' + escapeHtml(label) + '</label>';
						html += '    <div style="display:flex; gap:4px; flex:1; flex-wrap:wrap;">';
						html += '        <input type="text" class="stelproperty-text" name="' + escapeAttr(id) + '" ';
						html += '            value="' + escapeAttr(currentTextValue) + '"/>';
						html += '        <button class="btn-gen-custom-text-apply" data-prop="' + escapeAttr(id) + '">';
						html += '            ' + tr("Apply") + '';
						html += '        </button>';
						html += '    </div>';
						html += '</div>';
						return html;
				}
    }

    // ============================================================
    // LOCAL STORAGE
    // ============================================================

    function saveCustomButtons() {
        try {
            localStorage.setItem('btnGenCustomButtons', JSON.stringify(customButtons));
        } catch(e) {
            logError("Failed to save buttons: " + e.message);
        }
    }

    function loadCustomButtons() {
        try {
            var data = localStorage.getItem('btnGenCustomButtons');
            if (data) {
                customButtons = JSON.parse(data);
                log("Loaded " + customButtons.length + " custom buttons from localStorage");
                renderCustomButtons();
                updateCustomCount();
            }
        } catch(e) {
            logError("Failed to load buttons: " + e.message);
            customButtons = [];
        }
    }

    // ============================================================
    // EXPORT / IMPORT
    // ============================================================

    function exportButtonsJSON() {
        if (!customButtons.length) {
            alert(tr("No custom buttons to export"));
            return;
        }

        var json = JSON.stringify(customButtons, null, 2);
        var blob = new Blob([json], { type: 'application/json' });
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'custom_buttons_' + new Date().toISOString().slice(0,10) + '.json';
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        log("Exported " + customButtons.length + " buttons to JSON");
        updateStatus("Exported " + customButtons.length + " buttons");
    }

    function importButtonsJSON() {
        var input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        input.onchange = function(e) {
            var file = e.target.files[0];
            if (!file) return;
            var reader = new FileReader();
            reader.onload = function(ev) {
                try {
                    var data = JSON.parse(ev.target.result);
                    if (Array.isArray(data) && data.length) {
                        if (customButtons.length && !confirm(tr("This will replace your current buttons. Continue?"))) {
                            return;
                        }
                        customButtons = data;
                        saveCustomButtons();
                        renderCustomButtons();
                        updateCustomCount();
                        log("Imported " + customButtons.length + " buttons from JSON");
                        updateStatus("Imported " + customButtons.length + " buttons");
                    } else {
                        alert(tr("Invalid JSON format"));
                    }
                } catch(err) {
                    alert(tr("Failed to parse JSON: ") + err.message);
                }
            };
            reader.readAsText(file);
        };
        input.click();
    }

    // ============================================================
    // PUBLIC API
    // ============================================================

    return {
        init: init,
        addCustomButton: addCustomButton,
        removeCustomButton: removeCustomButton,
        getCustomButtons: function() { return customButtons; },
        exportButtonsJSON: exportButtonsJSON,
        importButtonsJSON: importButtonsJSON,
        clearAll: function() { 
            customButtons = []; 
            saveCustomButtons(); 
            renderCustomButtons(); 
            updateCustomCount(); 
            updateStatus("Cleared all buttons");
        },
        toggleBooleanProperty: toggleBooleanProperty
    };
});