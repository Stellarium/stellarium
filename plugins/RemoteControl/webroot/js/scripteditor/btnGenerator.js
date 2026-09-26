/* ========================================================================
 * btnGenerator.js - Advanced Button Generator Module (v2.0)
 * ========================================================================
 * 
 * This module provides a comprehensive button generation and management
 * interface for Stellarium actions and properties. It allows users to
 * browse, preview, generate HTML code, and create persistent custom
 * control panels with full support for multiple property types.
 * 
 * KEY IMPROVEMENTS (v2.0):
 * - Uses unified button system (unifiedButtons.js)
 * - Uses stelaction/stelproperty/stelproperty-toggle classes
 * - Uses jQuery UI icons for state indicators
 * - Removed pendingUpdates and manual cache updates
 * - Removed toggleBooleanProperty (use stelproperty-toggle instead)
 * - Simplified executeAction (uses actionApi.execute only)
 * - Server is the single source of truth for all values
 * - Consistent with official Stellarium Remote Control API
 * 
 * @module btnGenerator
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * @requires api/properties
 * @requires scripteditor/unifiedButtons
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-08-15
 * @license GPLv2+
 * @version 2.0.0
 * 
 * ======================================================================== */

define([
    "jquery",
    "api/remotecontrol",
    "api/actions",
    "api/properties",
    "scripteditor/unifiedButtons"
], function($, rc, actionApi, propApi, unifiedButtons) {
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
    var isAddingButton = false;
    var isRendering = false;

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
    var previewNumericValues = {};

    // Dialog state for editing custom buttons
    var editingIndex = -1;
    var editingButton = null;

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
    // SAFE SLIDER HELPER
    // ============================================================

    function safeSliderCall($slider, method, arg1, arg2) {
        if (!$slider || !$slider.length) return false;
        if (!$slider.hasClass('ui-slider')) return false;
        try {
            if (arg2 !== undefined) {
                $slider.slider(method, arg1, arg2);
            } else if (arg1 !== undefined) {
                $slider.slider(method, arg1);
            } else {
                $slider.slider(method);
            }
            return true;
        } catch(e) {
            console.warn('[BtnGen] safeSliderCall failed:', method, e.message);
            return false;
        }
    }
		
    // ============================================================
    // MOUSE WHEEL SUPPORT FOR SLIDERS
    // ============================================================

    function bindSliderWheel($slider) {
        if (!$slider || !$slider.length) return;
        if ($slider.data('wheel-bound')) return;
        $slider.data('wheel-bound', true);
        $slider.on('wheel', function(e) {
            e.preventDefault();
            var $s = $(this);
            if (!$s.hasClass('ui-slider')) return;
            var currentValue = $s.slider('value');
            var min = $s.slider('option', 'min') || 0;
            var max = $s.slider('option', 'max') || 100;
            var step = $s.slider('option', 'step') || 1;
            var delta = e.originalEvent.deltaY;
            var direction = delta < 0 ? 1 : -1;
            var newValue = Math.min(max, Math.max(min, currentValue + (direction * step)));
            $s.slider('value', newValue);
            $s.trigger('slide', { value: newValue });
            return false;
        });
    }		
		
    // ============================================================
    // HTML ESCAPE HELPERS
    // ============================================================

    function escapeHtml(str) {
        if (!str) return '';
        return String(str).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;') .replace(/'/g, '&#39;');
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
    // UPDATE PROPERTY - Send value to server
    // ============================================================

		function updateProperty(propName, value, useQueue) {
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

				var serverValue = convertValueForServer(propName, value);
				log("Updating property: " + propName + " = " + serverValue);

				try {
						if (useQueue) {
								propApi.setStelPropQueued(propName, serverValue);
						} else {
								propApi.setStelProp(propName, serverValue);
						}
						return true;
				} catch (error) {
						logError("Failed to send property update: " + error.message);
						return false;
				}
		}

    // ============================================================
    // UPDATE PROPERTY UI
    // ============================================================

    function updatePropertyUI(propName, value) {
        var info = propertyDataCache[propName];
        if (!info) return;

        var isBool = isBooleanProperty(info);
        var isNum = isNumericProperty(info);
        var isColor = isColorProperty(info, propName);

        // 1. UPDATE PREVIEW SECTION
        if (isBool) {
            var boolVal = (value === true || value === 'true' || value === 1 || value === '1');

            // Preview toggle buttons (using unified system)
            $('.btn-gen-preview-content .' + unifiedButtons.CLASSES.PROPERTY_TOGGLE + '[name="' + $.escapeSelector(propName) + '"]').each(function() {
                unifiedButtons.updateState($(this), boolVal);
            });

            // Custom section toggle buttons
            $('.btn-gen-custom-item .' + unifiedButtons.CLASSES.PROPERTY_TOGGLE + '[name="' + $.escapeSelector(propName) + '"]').each(function() {
                unifiedButtons.updateState($(this), boolVal);
            });

            // Regular checkbox controls
            $('input[type="checkbox"].stelproperty[name="' + $.escapeSelector(propName) + '"]').each(function() {
                $(this).prop('checked', boolVal);
            });

        }

        // Numeric sliders and displays
        if (isNum) {
            var numVal = parseFloat(value);
            if (!isNaN(numVal)) {

                // Preview sliders
                $('.btn-gen-preview-content .slider.stelproperty[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
                    var $slider = $(this);
                    safeSliderCall($slider, 'value', numVal);
                    var $display = $slider.closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                    if ($display.length) {
                        $display.text(numVal);
                    }
                });
                
								// Update preview info value display
                $('.btn-gen-preview-content').closest('.btn-gen-preview-box').find('.btn-gen-preview-value').each(function() {
                    $(this).text(tr("Value") + ': ' + formatValue(numVal));
                });
								
								// Update preview info value display
                $('.btn-gen-preview-value[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
                    $(this).text(tr("Value") + ': ' + formatValue(numVal));
                });
								
                // Custom section sliders
                $('.btn-gen-custom-item .slider.stelproperty[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
                    var $slider = $(this);
                    safeSliderCall($slider, 'value', numVal);
                    var $display = $slider.closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
                    if ($display.length) {
                        $display.text(numVal);
                    }
                });

                // Regular slider controls
                $('div.slider.stelproperty[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
                    safeSliderCall($(this), 'value', numVal);
                });

                // Spinner controls
                $('input.spinner.stelproperty[name="' + $.escapeSelector(propName) + '"]').each(function() {
                    if ($(this).hasClass('ui-spinner')) {
                        $(this).spinner('value', numVal);
                    }
                });

                // Update value display
                $('.btn-gen-slider-wrap[data-prop="' + $.escapeSelector(propName) + '"] .btn-gen-value-display').each(function() {
                    $(this).text(numVal);
                });
            }
        }

        // Color pickers
        if (isColor) {
            var colorArray = parseColorValue(value);
            var r = parseFloat(colorArray[0]) || 0;
            var g = parseFloat(colorArray[1]) || 0;
            var b = parseFloat(colorArray[2]) || 0;

            $('.color-picker-wrapper[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
                var $wrap = $(this);
                var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
                $wrap.find('.color-swatch').css('background-color', color);
                $wrap.find('.color-r').val(r.toFixed(2));
                $wrap.find('.color-g').val(g.toFixed(2));
                $wrap.find('.color-b').val(b.toFixed(2));
            });
        }

        // Text properties
        if (!isBool && !isNum && !isColor) {
            var textVal = value !== undefined && value !== null ? String(value) : '';

            // Preview text inputs
            $('.btn-gen-preview-content input[type="text"].' + unifiedButtons.CLASSES.PROPERTY_TEXT + '[name="' + $.escapeSelector(propName) + '"]').each(function() {
                $(this).val(textVal);
            });

            // Custom section text inputs
            $('.btn-gen-custom-text input[type="text"].' + unifiedButtons.CLASSES.PROPERTY_TEXT + '[name="' + $.escapeSelector(propName) + '"]').each(function() {
                $(this).val(textVal);
            });

            // Regular text inputs
            $('input[type="text"].' + unifiedButtons.CLASSES.PROPERTY_TEXT + '[name="' + $.escapeSelector(propName) + '"]').each(function() {
                $(this).val(textVal);
            });
        }

        // 2. UPDATE CODE EXPLANATION
        if (selectedType === 'property' && selectedPropertyName === propName) {
            var infoDisplay = propertyDataCache[propName];
            if (infoDisplay) {
                var $infoValue = $propertyInfoContainer.find('td:last-child code');
                if ($infoValue.length) {
                    $infoValue.text(formatValue(value));
                }

                var $explanation = $codeContainer.find('.btn-gen-code-explanation');
                if ($explanation.length) {
                    var label = customLabel || propName;
                    var type = infoDisplay.typeString || 'unknown';
                    var valueDisplay = formatValue(value);
                    var isBoolProp = isBooleanProperty(infoDisplay);
                    var isNumProp = isNumericProperty(infoDisplay);

                    var explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
                    explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
                    explanation += '<p><strong>Type:</strong> ' + escapeHtml(type) + '</p>';
                    explanation += '<p><strong>Current Value:</strong> <code>' + escapeHtml(valueDisplay) + '</code></p>';
                    explanation += '<p><strong>Writable:</strong> ' + (infoDisplay.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
                    if (isBoolProp) {
                        explanation += '<p><strong>Note:</strong> Use checkbox or toggle button.</p>';
                    }
                    if (isNumProp) {
                        var numVals = getNumericValues(propName);
                        explanation += '<p><strong>Range:</strong> min=' + numVals.min + ', max=' + numVals.max + ', step=' + numVals.step + '</p>';
                    }

                    $explanation.html(explanation);
                }
            }
        }

        // 3. UPDATE PROPERTY INFO PANEL
        if (selectedType === 'property' && selectedPropertyName === propName) {
            var infoDisplay = propertyDataCache[propName];
            if (infoDisplay) {
                var $infoTable = $propertyInfoContainer.find('.btn-gen-info-table');
                if ($infoTable.length) {
                    $infoTable.find('tr').each(function() {
                        var $cells = $(this).find('td');
                        if ($cells.length === 2 && $cells.eq(0).text().trim() === 'Current Value:') {
                            $cells.eq(1).html('<code>' + escapeHtml(formatValue(value)) + '</code>');
                        }
                    });
                }
            }
        }

        // 4. UPDATE PREVIEW INFO VALUE
        $('.btn-gen-preview-value[data-prop="' + $.escapeSelector(propName) + '"]').each(function() {
            $(this).text(tr("Value") + ': ' + formatValue(value));
        });

    }

    // ============================================================
    // PARSE COLOR VALUE
    // ============================================================

    function parseColorValue(value) {
        if (!value) return [1, 1, 1];

        if (Array.isArray(value) && value.length === 3) {
            return value.map(function(v) {
                return Math.max(0, Math.min(1, parseFloat(v) || 0));
            });
        }

        if (typeof value === 'string') {
            try {
                var parsed = JSON.parse(value);
                if (Array.isArray(parsed) && parsed.length === 3) {
                    return parsed.map(function(v) {
                        return Math.max(0, Math.min(1, parseFloat(v) || 0));
                    });
                }
            } catch(e) {
                var cleaned = value.replace(/[\[\]]/g, '').trim().split(',').map(function(v) {
                    return parseFloat(v.trim());
                });
                if (cleaned.length === 3 && !cleaned.some(isNaN)) {
                    return cleaned.map(function(v) {
                        return Math.max(0, Math.min(1, v));
                    });
                }
            }
        }

        return [1, 1, 1];
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

        actionApi.execute(actionId);
        return true;
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
				setTimeout(function() {
						refreshCustomButtonStates();
				}, 200);
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
		    setTimeout(function() {
					refreshCustomButtonStates();
				}, 200);
    }

    // ============================================================
    // INITIALIZATION
    // ============================================================

    function init() {
        if (isInitialized) return;
        log("Initializing Button Generator v2.0 (Unified)...");

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
    // SERVER LISTENERS SETUP
    // ============================================================

    function setupServerListeners() {
        // Listen for property changes from server
        $(propApi).on("stelPropertyChanged", function(evt, propName, propData) {
            var value = propData.value;
            log("Server sent property change: " + propName + " = " + value);

            // Update property list dropdown display
            updatePropertyListOption(propName, value);

            // Update all UI components that display this property
            updatePropertyUI(propName, value);

            // If this property is currently selected in preview, re-generate code only
            // (do NOT call renderPropertyPreview here - it would destroy and rebuild
            //  the slider on every server update, causing "cannot call methods on
            //  slider prior to initialization" errors in jQuery UI 4)
            // If this property is currently selected in preview, re-render
            if (selectedType === 'property' && selectedPropertyName === propName) {
                var info = propertyDataCache[propName];
                if (info) {
                    generateHtmlCode(propName, info);
                }
            }
        });

        // Listen for action changes from server
        $(actionApi).on("stelActionChanged", function(evt, actionId, actionData) {
            log("Server sent action change: " + actionId + " = " + actionData.isChecked);

            // Update action cache (state only, for display purposes)
            for (var cat in actionDataCache) {
                var found = actionDataCache[cat].filter(function(a) { return a.id === actionId; });
                if (found.length) {
                    found[0].isChecked = actionData.isChecked;
                    break;
                }
            }

            // Update UI components
            updateActionListOption(actionId, actionData.isChecked);
            updateActionUI(actionId, actionData.isChecked);

            // If this action is currently selected in preview, re-render
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
    // UPDATE DROPDOWN OPTIONS
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

                var valueDisplay = '';
                if (value !== undefined && value !== null) {
                    if (typeof value === 'boolean') {
                        valueDisplay = ' = ' + (value ? 'true' : 'false');
                    } else if (typeof value === 'string' && (value === 'true' || value === 'false')) {
                        valueDisplay = ' = ' + value;
                    } else if (typeof value === 'number') {
                        valueDisplay = ' = ' + value;
                    } else if (Array.isArray(value)) {
                        var displayArray = value.map(function(v) {
                            return typeof v === 'number' ? v.toFixed(2) : v;
                        });
                        valueDisplay = ' = [' + displayArray.join(', ') + ']';
                    } else if (typeof value === 'string') {
                        if (value.length > 30) {
                            valueDisplay = ' = "' + value.substring(0, 27) + '..."';
                        } else {
                            valueDisplay = ' = "' + value + '"';
                        }
                    } else if (typeof value === 'object') {
                        try {
                            var jsonStr = JSON.stringify(value);
                            if (jsonStr.length > 30) {
                                valueDisplay = ' = ' + jsonStr.substring(0, 27) + '...';
                            } else {
                                valueDisplay = ' = ' + jsonStr;
                            }
                        } catch(e) {
                            valueDisplay = ' = [object]';
                        }
                    } else {
                        valueDisplay = ' = ' + String(value);
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

                // After loading, update the dropdown with current values from the server
                setTimeout(function() {
                    Object.keys(data).forEach(function(propName) {
                        var serverValue = propApi.getStelProp(propName);
                        if (serverValue !== undefined) {
                            updatePropertyListOption(propName, serverValue);
                        }
                    });
                }, 500);
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

                // Get current value from server
                var currentValue = propApi.getStelProp(name);
                if (currentValue === undefined && info) {
                    currentValue = info.value;
                }

                var valueDisplay = '';
                if (currentValue !== undefined && currentValue !== null) {
                    if (typeof currentValue === 'boolean') {
                        valueDisplay = ' = ' + (currentValue ? 'true' : 'false');
                    } else if (typeof currentValue === 'string' && (currentValue === 'true' || currentValue === 'false')) {
                        valueDisplay = ' = ' + currentValue;
                    } else if (typeof currentValue === 'number') {
                        valueDisplay = ' = ' + currentValue;
                    } else if (Array.isArray(currentValue)) {
                        var displayArray = currentValue.map(function(v) {
                            return typeof v === 'number' ? v.toFixed(2) : v;
                        });
                        valueDisplay = ' = [' + displayArray.join(', ') + ']';
                    } else if (typeof currentValue === 'string') {
                        if (currentValue.length > 30) {
                            valueDisplay = ' = "' + currentValue.substring(0, 27) + '..."';
                        } else {
                            valueDisplay = ' = "' + currentValue + '"';
                        }
                    } else if (typeof currentValue === 'object') {
                        try {
                            var jsonStr = JSON.stringify(currentValue);
                            if (jsonStr.length > 30) {
                                valueDisplay = ' = ' + jsonStr.substring(0, 27) + '...';
                            } else {
                                valueDisplay = ' = ' + jsonStr;
                            }
                        } catch(e) {
                            valueDisplay = ' = [object]';
                        }
                    } else {
                        valueDisplay = ' = ' + String(currentValue);
                    }
                }

                $group.append($('<option>')
                    .val(name)
                    .text(name + ' (' + typeLabel + ')' + writable + valueDisplay)
                    .data('property', info));
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

            // Clear custom label when switching between action and property
            customLabel = '';
            $customLabelInput.val('').attr('placeholder', tr("Enter display name"));

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

            // Clear custom label when navigating between properties
            customLabel = '';
            $customLabelInput.val('').attr('placeholder', tr("Enter display name"));

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

        // Custom label - ESC to clear
        $customLabelInput.on('keydown', function(e) {
            if (e.key === 'Escape') {
                e.preventDefault();
                $(this).val('').attr('placeholder', tr("Enter display name")).blur();
                customLabel = '';
                if (selectedType === 'property' && selectedPropertyName) {
                    var info = propertyDataCache[selectedPropertyName];
                    if (info && info.isWritable !== false) {
                        renderPropertyPreview(selectedPropertyName, info);
                        generateHtmlCode(selectedPropertyName, info);
                    }
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

            var propName = $wrap.data('prop');
            if (propName) {
                var $slider = $('.btn-gen-preview-slider[data-prop="' + propName + '"]');
                if ($slider.length && $slider.hasClass('ui-slider')) {
                    $slider.slider('option', 'min', minVal);
                    $slider.slider('option', 'max', maxVal);
                    $slider.slider('option', 'step', stepVal);
                }
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
        var writable = info.isWritable !== false ? tr("Yes") : tr("No (read-only)");
        var notifiable = info.canNotify ? tr("Yes") : tr("No");
        var min = info.min !== undefined ? info.min : '-';
        var max = info.max !== undefined ? info.max : '-';
        var step = info.step !== undefined ? info.step : '-';
        var typeEnum = info.typeEnum || '?';

        // Get current value from server
        var currentValue = propApi.getStelProp(propName);
        if (currentValue !== undefined) {
            info.value = currentValue;
        }

        var writableClass = (info.isWritable === false) ? 'btn-gen-readonly' : '';

        var html = '<div class="btn-gen-property-info ' + writableClass + '">';
        html += '<h4>' + tr("Property Information") + '</h4>';
        html += '<table class="btn-gen-info-table">';
        html += '<tr><td>' + tr("Name") + ':</td><td><strong>' + escapeHtml(propName) + '</strong></td></tr>';
        html += '<tr><td>' + tr("Type") + ':</td><td><strong>' + escapeHtml(type) + '</strong> (enum: ' + typeEnum + ')</td></tr>';
        html += '<tr><td>' + tr("Current Value") + ':</td><td><code>' + escapeHtml(formatValue(currentValue)) + '</code></td></tr>';
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
    // NUMERIC PROPERTY CONTROLS
    // ============================================================

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

    // ============================================================
    // RENDER ACTION PREVIEW
    // ============================================================

    function renderActionPreview(action) {
        var isCheckable = action.isCheckable;
        var isChecked = action.isChecked;
        var label = action.text;

        var html = '<div class="btn-gen-preview-box">';
        html += '<div class="btn-gen-preview-label">' + tr("Preview") + '</div>';
        html += '<div class="btn-gen-preview-content">';

        // Use unified button system
        html += unifiedButtons.createButton({
            type: 'action',
            name: action.id,
            label: label,
            isChecked: isChecked,
            isCheckable: isCheckable
        });

        html += '</div>';
        html += '<div class="btn-gen-preview-info">';
        html += '<span class="btn-gen-preview-type">' + (isCheckable ? tr("Toggle") : tr("Trigger")) + '</span>';
        html += '<span class="btn-gen-preview-id">' + escapeHtml(action.id) + '</span>';
        html += '<span class="btn-gen-preview-state">' + tr("State") + ': ' + (isChecked ? 'ON' : 'OFF') + '</span>';
        html += '</div>';
        html += '</div>';

        $previewContainer.html(html);

        // Update action UI state from server
        updateActionUI(action.id, isChecked);
    }

    // ============================================================
    // RENDER PROPERTY PREVIEW
    // ============================================================

    function renderPropertyPreview(propName, info) {
        var type = info.typeString || 'unknown';
        var label = customLabel || propName;
        var isWritable = info.isWritable !== false;
        var typeEnum = info.typeEnum || 0;

        if (!isWritable) {
            var msg = tr("This property is read-only.");
            $previewContainer.html('<div class="btn-gen-placeholder" >' + msg + '</div>');
            return;
        }

        // Get current value from server (source of truth)
        var currentValue = propApi.getStelProp(propName);
        if (currentValue === undefined) {
            currentValue = info.value;
        }

        var html = '<div class="btn-gen-preview-box">';
        html += '<div class="btn-gen-preview-label">' + tr("Preview") + '</div>';
        html += '<div class="btn-gen-preview-content">';

        // BOOLEAN PROPERTY - uses stelproperty-toggle
        if (isBooleanProperty(info)) {
            var isChecked = (currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');

            // Use unified button system
            html += unifiedButtons.createButton({
                type: 'property-toggle',
                name: propName,
                label: label,
                isChecked: isChecked
            });
        }

        // NUMERIC PROPERTY - with min/max/step controls
				else if (isNumericProperty(info)) {
						var numVals = getNumericValues(propName);
						var min = numVals.min;
						var max = numVals.max;
						var step = numVals.step;
						var currentVal = currentValue !== undefined ? currentValue : 0;
						
						// Determine number format based on step precision
						var numberFormat = getNumberFormat(step);

						html += '<div class="btn-gen-slider-wrap" data-prop="' + escapeAttr(propName) + '">';
						html += '    <div class="btn-gen-slider-header">';
						html += '        <label>' + escapeHtml(label) + '</label>';
						html += '        <span class="stelproperty" data-prop="' + escapeAttr(propName) + '" data-numberformat="' + escapeAttr(numberFormat) + '">' + escapeHtml(String(currentVal)) + '</span>';
						html += '        <span class="stelproperty" data-prop="' + escapeAttr(propName) + '" data-numberformat="' + escapeAttr(numberFormat) + '"></span>';
						html += '    </div>';
						html += '    <div class="slider stelproperty" data-prop="' + escapeAttr(propName) + '" ';
						html += '         data-min="' + Number(min) + '" data-max="' + Number(max) + '" data-step="' + Number(step) + '"></div>';
						html += '</div>';

						// MIN/MAX/STEP controls
						html += '<div class="btn-gen-slider-controls" style="display:flex; gap:8px; margin-top:6px; flex-wrap:wrap;">';
						html += '    <div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '        <label style="font-size:9px; color:#8A8C8E;">' + tr("Min") + ':</label>';
						html += '        <input type="number" class="btn-gen-numeric-min" data-prop="' + escapeAttr(propName) + '" ';
						html += '               value="' + Number(min) + '" step="any" style="width:55px; padding:3px 5px; ';
						html += '               background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; ';
						html += '               border-radius:3px; font-size:10px;">';
						html += '    </div>';
						html += '    <div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '        <label style="font-size:9px; color:#8A8C8E;">' + tr("Max") + ':</label>';
						html += '        <input type="number" class="btn-gen-numeric-max" data-prop="' + escapeAttr(propName) + '" ';
						html += '               value="' + Number(max) + '" step="any" style="width:55px; padding:3px 5px; ';
						html += '               background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; ';
						html += '               border-radius:3px; font-size:10px;">';
						html += '    </div>';
						html += '    <div class="btn-gen-control-group" style="display:flex; align-items:center; gap:4px;">';
						html += '        <label style="font-size:9px; color:#8A8C8E;">' + tr("Step") + ':</label>';
						html += '        <input type="number" class="btn-gen-numeric-step" data-prop="' + escapeAttr(propName) + '" ';
						html += '               value="' + Number(step) + '" step="any" style="width:55px; padding:3px 5px; ';
						html += '               background:#2A2C2E; color:#DCDBDA; border:1px solid #5D5F62; ';
						html += '               border-radius:3px; font-size:10px;">';
						html += '    </div>';
						html += '</div>';
				}				

        // COLOR PROPERTY
        else if (isColorProperty(info, propName)) {
            var colorArray = parseColorValue(currentValue);
            var rVal = parseFloat(colorArray[0]) || 0;
            var gVal = parseFloat(colorArray[1]) || 0;
            var bVal = parseFloat(colorArray[2]) || 0;

            html += '<div class="option-sub-control color-control">\n';
            html += '    <div class="color-picker-wrapper" data-prop="' + escapeAttr(propName) + '">\n';
            html += '        <div class="color-swatch" style="background-color: rgb(' +
                            Math.round(rVal * 255) + ',' +
                            Math.round(gVal * 255) + ',' +
                            Math.round(bVal * 255) + ');" title="' + tr("Click to pick color") + '"></div>\n';
            html += '        <div class="color-inputs">\n';
            html += '            <input type="number" class="color-input color-r" min="0" max="1" step="0.01" value="' + rVal.toFixed(2) + '" />\n';
            html += '            <input type="number" class="color-input color-g" min="0" max="1" step="0.01" value="' + gVal.toFixed(2) + '" />\n';
            html += '            <input type="number" class="color-input color-b" min="0" max="1" step="0.01" value="' + bVal.toFixed(2) + '" />\n';
            html += '        </div>\n';
            html += '    </div>\n';
            html += '</div>\n';
            html += '<span class="btn-gen-prop-name">' + escapeHtml(label) + '</span>\n';

            // Store numeric values for this property
            if (!numericProperties[propName]) {
                numericProperties[propName] = {
                    min: info.min !== undefined ? info.min : 0,
                    max: info.max !== undefined ? info.max : 100,
                    step: info.step !== undefined ? info.step : 1
                };
            }
        }

        // TEXT PROPERTY (QString)
        else {
            var currentTextValue = currentValue !== undefined ? String(currentValue) : '';

            // Use unified text input system
            html += unifiedButtons.createTextInput({
                name: propName,
                label: label,
                value: currentTextValue
            });
        }

        html += '</div>'; // End preview-content
        html += '<div class="btn-gen-preview-info">';
        html += '<span class="btn-gen-preview-type">' + escapeHtml(type) + '</span>';
        html += '<span class="btn-gen-preview-id">' + escapeHtml(propName) + '</span>';
        if (currentValue !== undefined) {
            html += '<span class="btn-gen-preview-value" data-prop="' + escapeAttr(propName) + '">' + tr("Value") + ': ' + escapeHtml(formatValue(currentValue)) + '</span>';
        }
        html += '</div>';
        html += '</div>'; // End preview-box

        $previewContainer.html(html);

        // ============================================================
        // BIND EVENTS - Connect UI controls to server
        // ============================================================

        // Numeric slider - uses existing stelproperty system
        $previewContainer.find('.slider.stelproperty[data-prop="' + propName + '"]').each(function() {
            var self = $(this);
            var prop = self.data('prop');

            if (self.data('slider-initialized')) {
                return;
            }

            var min = parseFloat(self.data('min')) || 0;
            var max = parseFloat(self.data('max')) || 100;
            var step = parseFloat(self.data('step')) || 1;

            self.slider({
                min: min,
                max: max,
                step: step
            });
						
            bindSliderWheel(self);   // support mouse wheel for slider
            self.data('slider-initialized', true);

            // Listen for server changes
            $(propApi).on('stelPropertyChanged:' + prop, function(evt, propData) {
                self.slider('value', propData.value);
                var $display = self.closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                if ($display.length) {
                    $display.text(propData.value);
                }
            });

            // Get initial value from server
            var initialValue = propApi.getStelProp(prop);
            if (initialValue !== undefined) {
                self.slider('value', initialValue);
                var $display = self.closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
                if ($display.length) {
                    $display.text(initialValue);
                }
            }

            // Handle user slide - send to server
						self.off('slide.btnGenPreview').on('slide.btnGenPreview', function(evt, ui) {
								var propName2 = $(this).data('prop');
								propApi.setStelPropQueued(propName2, ui.value);
								var $display = $(this).closest('.btn-gen-slider-wrap').find('.btn-gen-value-display');
								if ($display.length) {
										$display.text(ui.value);
								}
								// Update preview info value display immediately (optimistic)
								$('.btn-gen-preview-value[data-prop="' + $.escapeSelector(propName2) + '"]').each(function() {
										$(this).text(tr("Value") + ': ' + formatValue(ui.value));
								});
						});
        });

        // MIN/MAX/STEP input events (for customization)
        $previewContainer.find('.btn-gen-numeric-min, .btn-gen-numeric-max, .btn-gen-numeric-step')
            .off('input.btnGenNumeric')
            .on('input.btnGenNumeric', function() {
                var $input = $(this);
                var propName2 = $input.data('prop');
                if (!propName2) return;

                var $wrap = $input.closest('.btn-gen-preview-content');
                var minVal = parseFloat($wrap.find('.btn-gen-numeric-min').val());
                var maxVal = parseFloat($wrap.find('.btn-gen-numeric-max').val());
                var stepVal = parseFloat($wrap.find('.btn-gen-numeric-step').val());

                if (isNaN(minVal)) minVal = 0;
                if (isNaN(maxVal)) maxVal = 100;
                if (isNaN(stepVal)) stepVal = 1;
                if (minVal > maxVal) {
                    var temp = minVal;
                    minVal = maxVal;
                    maxVal = temp;
                    $wrap.find('.btn-gen-numeric-min').val(minVal);
                    $wrap.find('.btn-gen-numeric-max').val(maxVal);
                }
                if (stepVal <= 0) stepVal = 1;

                // Store values for transfer to custom button
                updateNumericValues(propName2, minVal, maxVal, stepVal);

                if (!previewNumericValues) {
                    previewNumericValues = {};
                }
                previewNumericValues[propName2] = {
                    min: minVal,
                    max: maxVal,
                    step: stepVal
                };

                // Update the slider
                var $slider = $wrap.find('.slider.stelproperty');
                if ($slider.length && $slider.hasClass('ui-slider')) {
                    safeSliderCall($slider, 'option', 'min', minVal);
                    safeSliderCall($slider, 'option', 'max', maxVal);
                    safeSliderCall($slider, 'option', 'step', stepVal);

                    var currentValue = $slider.slider('value');
                    if (currentValue < minVal) {
                        safeSliderCall($slider, 'value', minVal);
                        var $display = $wrap.find('.btn-gen-value-display');
                        if ($display.length) {
                            $display.text(minVal);
                        }
                        updateProperty(propName2, minVal);
                    } else if (currentValue > maxVal) {
                        safeSliderCall($slider, 'value', maxVal);
                        var $display = $wrap.find('.btn-gen-value-display');
                        if ($display.length) {
                            $display.text(maxVal);
                        }
                        updateProperty(propName2, maxVal);
                    }
                }

                // Update generated code
                var info2 = propertyDataCache[propName2];
                if (info2 && selectedType === 'property' && selectedPropertyName === propName2) {
                    generateHtmlCode(propName2, info2);
                }
            });

        // Color picker - RGB inputs
        $previewContainer.find('.color-picker-wrapper .color-input')
            .off('input.colorPicker')
            .on('input.colorPicker', function() {
                var $wrap = $(this).closest('.color-picker-wrapper');
                var prop = $wrap.data('prop');
                var r = parseFloat($wrap.find('.color-r').val()) || 0;
                var g = parseFloat($wrap.find('.color-g').val()) || 0;
                var b = parseFloat($wrap.find('.color-b').val()) || 0;

                r = Math.max(0, Math.min(1, r));
                g = Math.max(0, Math.min(1, g));
                b = Math.max(0, Math.min(1, b));

                var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
                $wrap.find('.color-swatch').css('background-color', color);

                if (prop) {
                    updateProperty(prop, [r, g, b]);
                }
            });

        // Color picker - Swatch click (native color picker)
        $previewContainer.find('.color-picker-wrapper .color-swatch')
            .off('click.colorPicker')
            .on('click.colorPicker', function() {
                var $wrap = $(this).closest('.color-picker-wrapper');
                var prop = $wrap.data('prop');
                var r = parseFloat($wrap.find('.color-r').val()) || 0;
                var g = parseFloat($wrap.find('.color-g').val()) || 0;
                var b = parseFloat($wrap.find('.color-b').val()) || 0;

                var hex = '#' +
                        Math.round(r * 255).toString(16).padStart(2, '0') +
                        Math.round(g * 255).toString(16).padStart(2, '0') +
                        Math.round(b * 255).toString(16).padStart(2, '0');

                var input = document.createElement('input');
                input.type = 'color';
                input.value = hex;
                input.addEventListener('input', function() {
                    var hexVal = this.value;
                    var r2 = parseInt(hexVal.substring(1, 3), 16) / 255;
                    var g2 = parseInt(hexVal.substring(3, 5), 16) / 255;
                    var b2 = parseInt(hexVal.substring(5, 7), 16) / 255;
                    $wrap.find('.color-r').val(r2.toFixed(2));
                    $wrap.find('.color-g').val(g2.toFixed(2));
                    $wrap.find('.color-b').val(b2.toFixed(2));
                    $wrap.find('.color-swatch').css('background-color', hexVal);
                    if (prop) {
                        updateProperty(prop, [r2, g2, b2]);
                    }
                });
                input.click();
            });

        // After rendering the preview, update the dropdown to show current value
        var currentValue = propApi.getStelProp(propName);
        if (currentValue !== undefined) {
            updatePropertyListOption(propName, currentValue);
        }

        // Update the code explanation with current value
        var label = customLabel || propName;
        var type = info.typeString || 'unknown';
        var valueDisplay = formatValue(currentValue);
        var isBool = isBooleanProperty(info);
        var isNum = isNumericProperty(info);
        var isColor = isColorProperty(info, propName);

        var explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
        explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
        explanation += '<p><strong>Type:</strong> ' + escapeHtml(type) + '</p>';
        explanation += '<p><strong>Current Value:</strong> <code>' + escapeHtml(valueDisplay) + '</code></p>';
        explanation += '<p><strong>Writable:</strong> ' + (info.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
        if (isBool) {
            explanation += '<p><strong>Note:</strong> Use checkbox or toggle button with stelproperty-toggle.</p>';
        }
        if (isNum) {
            var numVals = getNumericValues(propName);
            explanation += '<p><strong>Range:</strong> min=' + numVals.min + ', max=' + numVals.max + ', step=' + numVals.step + '</p>';
        }
        if (isColor) {
            var colorArray = parseColorValue(currentValue);
            var r = parseFloat(colorArray[0]) || 0;
            var g = parseFloat(colorArray[1]) || 0;
            var b = parseFloat(colorArray[2]) || 0;
            explanation += '<p><strong>RGB:</strong> (' + r.toFixed(2) + ', ' + g.toFixed(2) + ', ' + b.toFixed(2) + ')</p>';
        }

        // First update the info object with the current value for generateHtmlCode
        var infoWithCurrentValue = $.extend({}, info);
        infoWithCurrentValue.value = currentValue !== undefined ? currentValue : info.value;

        // Generate the HTML code and explanation
        generateHtmlCode(propName, infoWithCurrentValue);

        // Update or create the explanation
        var $explanation = $codeContainer.find('.btn-gen-code-explanation');
        if ($explanation.length) {
            $explanation.html(explanation);
        } else {
            $codeContainer.append('<div class="btn-gen-code-explanation">' + explanation + '</div>');
        }
    }

		// ============================================================
		// DETERMINE NUMBER FORMAT FROM STEP VALUE
		// ============================================================
		/**
		 * Determines the appropriate number format based on step precision.
		 * 
		 * @param {number} step - The step value of the property
		 * @returns {string} The number format string (n0, n1, n2, n3)
		 */
		function getNumberFormat(step) {
				if (!step || step === 0) return 'n2';
				
				var stepStr = String(step);
				var decimalPlaces = 0;
				if (stepStr.indexOf('.') !== -1) {
						decimalPlaces = stepStr.split('.')[1].length;
				}
				
				if (decimalPlaces === 0) {
						return 'n0'; // Integer numbers
				} else if (decimalPlaces <= 1) {
						return 'n1'; // One decimal place
				} else if (decimalPlaces <= 2) {
						return 'n2'; // Two decimal places
				} else {
						return 'n3'; // Three decimal places
				}
		}

    // ============================================================
    // HTML CODE GENERATION
    // ============================================================

    function generateHtmlCode(actionOrProp, info) {
        var html = '';
        var explanation = '';
        var htmlCheckbox = '';
        var htmlToggle = '';
        var htmlButton = '';

				if (selectedType === 'action') {
						var action = actionOrProp;
						var isCheckable = action.isCheckable;
						var isChecked = action.isChecked;

						var versionsHtml = '';
						var versionExplanations = [];

						if (isCheckable) {
								// ============================================================
								// TOGGLEABLE ACTIONS - Generate Version 1 & 2 only
								// ============================================================
								
								// VERSION 1: CHECKBOX (for toggleable actions)
								var htmlCheckbox = '<!-- Checkbox for StelAction: ' + escapeHtml(action.id) + ' -->\n';
								htmlCheckbox += '<label class="btn-gen-preview-toggle">\n';
								htmlCheckbox += '    <input type="checkbox" class="stelaction" name="' + escapeAttr(action.id) + '" ' +
																 (isChecked ? 'checked' : '') + ' />\n';
								htmlCheckbox += '    ' + escapeHtml(action.text) + '\n';
								htmlCheckbox += '</label>';

								// VERSION 2: TOGGLE BUTTON (using stelaction)
								var htmlToggle = '<!-- Toggle Button for StelAction: ' + action.id + ' -->\n';
								htmlToggle += unifiedButtons.createButton({
										type: 'action',
										name: action.id,
										label: action.text,
										isChecked: isChecked,
										isCheckable: isCheckable  // true
								});

								// Build HTML with both versions
								versionsHtml = '<!-- Two versions available for toggleable action -->\n\n' +
															 '<!-- VERSION 1: Checkbox (toggleable actions only) -->\n' +
															 htmlCheckbox + '\n\n' +
															 '<!-- VERSION 2: Toggle Button (RECOMMENDED - uses stelaction) -->\n' +
															 htmlToggle;

								versionExplanations = [
										'• <strong>Version 1 (Checkbox):</strong> Simple checkbox for toggling',
										'• <strong>Version 2 (Toggle Button):</strong> Unified button with visual state indicator (RECOMMENDED)'
								];

						} else {
								// ============================================================
								// TRIGGER ACTIONS - Generate Version 3 only
								// ============================================================
								
								// VERSION 3: TRIGGER BUTTON (for all actions)
								var htmlTrigger = '<!-- Trigger Button for StelAction: ' + escapeHtml(action.id) + ' -->\n';
								htmlTrigger += unifiedButtons.createButton({
										type: 'action',
										name: action.id,
										label: action.text,
										isCheckable: false
								});

								versionsHtml = '<!-- Trigger action - single version available -->\n\n' +
															 '<!-- VERSION 3: Trigger Button -->\n' +
															 htmlTrigger;

								versionExplanations = [
										'• <strong>Version 3 (Trigger Button):</strong> Click to execute the action (no state)'
								];
						}

						// Build final HTML
						html = versionsHtml;

						// Build explanation
						explanation = '<p><strong>' + escapeHtml(action.text) + '</strong></p>';
						explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(action.id) + '</code></p>';
						explanation += '<p><strong>Type:</strong> ' + (isCheckable ? 'Toggle (Checkable)' : 'Trigger (One-shot)') + '</p>';
						if (isCheckable) {
								explanation += '<p><strong>Current State:</strong> ' + (isChecked ? 'ON \u2713' : 'OFF \u2717') + '</p>';
						}
						explanation += '<p><strong>Available Versions:</strong></p>';
						explanation += '<ul style="margin:5px 0; padding-left:20px;">';
						versionExplanations.forEach(function(item) {
								explanation += '<li>' + item + '</li>';
						});
						explanation += '</ul>';
						if (isCheckable) {
								explanation += '<p><strong>Note:</strong> Toggle buttons automatically sync with server state.</p>';
						} else {
								explanation += '<p><strong>Note:</strong> Trigger actions execute immediately on click.</p>';
						}
				} 
					else if (selectedType === 'property') {
            var propName = selectedPropertyName;
            var infoData = info;
            var label = customLabel || propName;
            var isBool = isBooleanProperty(infoData);
            var isNum = isNumericProperty(infoData);
            var isColor = isColorProperty(infoData, propName);

            // Get current value from server
            var currentVal = propApi.getStelProp(propName);
            if (currentVal === undefined) {
                currentVal = infoData.value;
            }

            // BOOLEAN PROPERTY
            if (isBool) {
                var isChecked = (currentVal === true || currentVal === 'true' || currentVal === 1 || currentVal === '1');

						// VERSION 1: Checkbox
						htmlCheckbox = '<!-- Checkbox for StelProperty: ' + escapeHtml(propName) + ' -->\n';
						htmlCheckbox += '<label class="btn-gen-preview-toggle">\n';
						htmlCheckbox += '    <input type="checkbox" class="stelproperty" name="' + escapeAttr(propName) + '" ' +
														 (isChecked ? 'checked' : '') + ' />\n';
						htmlCheckbox += '    ' + escapeHtml(label) + '\n';
						htmlCheckbox += '</label>';

                // VERSION 2: Toggle Button (using stelproperty-toggle)
                htmlToggle = '<!-- Toggle Button for StelProperty: ' + propName + ' -->\n';
                htmlToggle += unifiedButtons.createButton({
                    type: 'property-toggle',
                    name: propName,
                    label: label,
                    isChecked: isChecked
                });

                // VERSION 3: ON/OFF Buttons
                htmlButton = '<!-- ON/OFF Buttons for StelProperty: ' + propName + ' -->\n';
                htmlButton += '<div style="display:flex; gap:4px;">\n';
                htmlButton += '    <button class="stelproperty" name="' + escapeAttr(propName) + '" value="true" ' +
                              (isChecked ? 'class="stelproperty active"' : 'class="stelproperty"') + '>ON</button>\n';
                htmlButton += '    <button class="stelproperty" name="' + escapeAttr(propName) + '" value="false" ' +
                              (!isChecked ? 'class="stelproperty active"' : 'class="stelproperty"') + '>OFF</button>\n';
                htmlButton += '</div>';

                html = '<!-- Three versions available -->\n\n' +
                       '<!-- VERSION 1: Checkbox (RECOMMENDED - uses existing stelproperty system) -->\n' +
                       htmlCheckbox + '\n\n' +
                       '<!-- VERSION 2: Toggle Button (uses stelproperty-toggle) -->\n' +
                       htmlToggle + '\n\n' +
                       '<!-- VERSION 3: ON/OFF Buttons (uses stelproperty) -->\n' +
                       htmlButton + '\n';
            }

            // NUMERIC PROPERTY
						else if (isNum) {
								var numVals = getNumericValues(propName);
								var min = numVals.min;
								var max = numVals.max;
								var step = numVals.step;
								var value = currentVal !== undefined ? currentVal : 0;
								
								// Determine number format based on step precision
								var numberFormat = getNumberFormat(step);

								// VERSION 1: Slider
								htmlCheckbox = '<!-- Slider for StelProperty: ' + escapeHtml(propName) + ' -->\n';
								htmlCheckbox += '<div class="btn-gen-slider-wrap">\n';
								htmlCheckbox += '    <div class="btn-gen-slider-header">\n';
								htmlCheckbox += '        <label>' + escapeHtml(label) + '</label>\n';
								htmlCheckbox += '        <span class="stelproperty" data-prop="' + escapeAttr(propName) + '" data-numberformat="' + escapeAttr(numberFormat) + '"></span>\n';
								htmlCheckbox += '    </div>\n';
								htmlCheckbox += '    <div class="slider stelproperty" data-prop="' + escapeAttr(propName) + '" ';
								htmlCheckbox += 'data-min="' + Number(min) + '" data-max="' + Number(max) + '" data-step="' + Number(step) + '"></div>\n';
								htmlCheckbox += '</div>';

								// VERSION 2: Spinner
								htmlButton = '<!-- Spinner for StelProperty: ' + escapeHtml(propName) + ' -->\n';
								htmlButton += '<div class="btn-gen-slider-wrap">\n';
								htmlButton += '    <label>' + escapeHtml(label) + '</label>\n';
								htmlButton += '    <input class="spinner stelproperty" name="' + escapeAttr(propName) + '" ';
								htmlButton += 'data-min="' + Number(min) + '" data-max="' + Number(max) + '" data-step="' + Number(step) + '" data-numberformat="' + escapeAttr(numberFormat) + '" />\n';
								htmlButton += '</div>';

								html = '<!-- Two versions available -->\n\n' +
											 '<!-- VERSION 1: Slider (RECOMMENDED) -->\n' +
											 htmlCheckbox + '\n\n' +
											 '<!-- VERSION 2: Spinner -->\n' +
											 htmlButton + '\n\n' +
											 '<!-- Range: min=' + min + ', max=' + max + ', step=' + step + ' -->';
						}

            // COLOR PROPERTY
            else if (isColor) {
                var colorArray = parseColorValue(currentVal);
                var rVal = parseFloat(colorArray[0]) || 0;
                var gVal = parseFloat(colorArray[1]) || 0;
                var bVal = parseFloat(colorArray[2]) || 0;

                // ============================================================
                // VERSION 1: Plain number inputs (default HTML5 number)
                // ============================================================
                var htmlColorPlain = '<!-- ============================================================ -->\n';
                htmlColorPlain += '<!-- COLOR PICKER (Plain number inputs) for StelProperty: ' + escapeHtml(propName) + ' -->\n';
                htmlColorPlain += '<!-- ============================================================ -->\n';
                htmlColorPlain += '<div class="option-sub-control color-control">\n';
                htmlColorPlain += '    <div class="color-picker-wrapper" data-prop="' + escapeAttr(propName) + '">\n';
                htmlColorPlain += '        <div class="color-swatch" style="background-color: rgb(' +
                                Math.round(rVal * 255) + ',' +
                                Math.round(gVal * 255) + ',' +
                                Math.round(bVal * 255) + ');"></div>\n';
                htmlColorPlain += '        <div class="color-inputs">\n';
                htmlColorPlain += '            <input type="number" class="color-input color-r" min="0" max="1" step="0.01" value="' + rVal.toFixed(2) + '" />\n';
                htmlColorPlain += '            <input type="number" class="color-input color-g" min="0" max="1" step="0.01" value="' + gVal.toFixed(2) + '" />\n';
                htmlColorPlain += '            <input type="number" class="color-input color-b" min="0" max="1" step="0.01" value="' + bVal.toFixed(2) + '" />\n';
                htmlColorPlain += '        </div>\n';
                htmlColorPlain += '    </div>\n';
                htmlColorPlain += '</div>\n';

                // ============================================================
                // VERSION 2: jQuery UI spinners (with +/- buttons)
                // ============================================================
                var htmlColorSpinner = '<!-- ============================================================ -->\n';
                htmlColorSpinner += '<!-- COLOR PICKER (jQuery UI spinners) for StelProperty: ' + escapeHtml(propName) + ' -->\n';
                htmlColorSpinner += '<!-- ============================================================ -->\n';
                htmlColorSpinner += '<div class="option-sub-control color-control">\n';
                htmlColorSpinner += '    <div class="color-picker-wrapper" data-prop="' + escapeAttr(propName) + '">\n';
                htmlColorSpinner += '        <div class="color-swatch" style="background-color: rgb(' +
                                Math.round(rVal * 255) + ',' +
                                Math.round(gVal * 255) + ',' +
                                Math.round(bVal * 255) + ');"></div>\n';
                htmlColorSpinner += '        <div class="color-inputs">\n';
                htmlColorSpinner += '            <input class="spinner color-r" data-prop="' + escapeAttr(propName) + '" data-min="0" data-max="1" data-step="0.01" data-numberformat="n2" value="' + rVal.toFixed(2) + '" />\n';
                htmlColorSpinner += '            <input class="spinner color-g" data-prop="' + escapeAttr(propName) + '" data-min="0" data-max="1" data-step="0.01" data-numberformat="n2" value="' + gVal.toFixed(2) + '" />\n';
                htmlColorSpinner += '            <input class="spinner color-b" data-prop="' + escapeAttr(propName) + '" data-min="0" data-max="1" data-step="0.01" data-numberformat="n2" value="' + bVal.toFixed(2) + '" />\n';
                htmlColorSpinner += '        </div>\n';
                htmlColorSpinner += '    </div>\n';
                htmlColorSpinner += '</div>\n';

                // Combine both versions
                html = '<!-- Two versions available -->\n\n' +
                       '<!-- VERSION 1: Plain number inputs (lighter, no jQuery UI dependency) -->\n' +
                       htmlColorPlain + '\n\n' +
                       '<!-- VERSION 2: jQuery UI spinners - Recommende (with +/- buttons) -->\n' +
                       htmlColorSpinner + '\n\n' +
                       '<!-- For the color picker to work, ensure connectColorSpinners() is called -->\n' +
                       '<!-- This is handled by mainui.js -->\n';

                explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
                explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
                explanation += '<p><strong>Type:</strong> ' + escapeHtml(infoData.typeString || 'unknown') + '</p>';
                explanation += '<p><strong>Current Value:</strong> <code>[' + rVal.toFixed(2) + ', ' + gVal.toFixed(2) + ', ' + bVal.toFixed(2) + ']</code></p>';
                explanation += '<p><strong>Writable:</strong> ' + (infoData.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
                explanation += '<p><strong>Available Versions:</strong></p>';
                explanation += '<ul style="margin:5px 0; padding-left:20px;">';
                explanation += '<li><strong>Version 1 (Plain):</strong> Standard HTML5 number inputs</li>';
                explanation += '<li><strong>Version 2 (Spinner):</strong> jQuery UI spinner with +/- buttons and number formatting</li>';
                explanation += '</ul>';
                explanation += '<p><strong>Note:</strong> Both use the unified color picker system (connectColorSpinners). This is handled by mainui.js</p>';
            }

            // TEXT PROPERTY
            else {
                var currentTextValue = currentVal !== undefined ? String(currentVal) : '';

                // Use unified text input system
                html = unifiedButtons.createTextInput({
                    name: propName,
                    label: label,
                    value: currentTextValue
                });

                html += '\n<!-- This uses the unified text property system. -->\n';
                html += '<!-- connectTextProperties() in mainui.js handles the binding. -->';

                explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
                explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
                explanation += '<p><strong>Type:</strong> ' + escapeHtml(infoData.typeString || 'unknown') + '</p>';
                explanation += '<p><strong>Current Value:</strong> <code>"' + escapeHtml(currentTextValue) + '"</code></p>';
                explanation += '<p><strong>Writable:</strong> ' + (infoData.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
                explanation += '<p><strong>Usage:</strong> Use the Apply button or press Enter to send the value.</p>';
            }

            // Add explanation for all property types
            if (!explanation) {
                var valueDisplay = currentVal !== undefined ? formatValue(currentVal) : 'undefined';
                explanation = '<p><strong>' + escapeHtml(label) + '</strong></p>';
                explanation += '<p><strong>ID:</strong> <code>' + escapeHtml(propName) + '</code></p>';
                explanation += '<p><strong>Type:</strong> ' + escapeHtml(infoData.typeString || 'unknown') + '</p>';
                explanation += '<p><strong>Current Value:</strong> <code>' + escapeHtml(valueDisplay) + '</code></p>';
                explanation += '<p><strong>Writable:</strong> ' + (infoData.isWritable !== false ? 'Yes' : 'No (read-only)') + '</p>';
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
    // UPDATE ACTION UI
    // ============================================================

    function updateActionUI(actionId, isChecked) {
        // Update via unified system
        $('.btn-gen-preview-content .' + unifiedButtons.CLASSES.ACTION + '[name="' + actionId + '"]').each(function() {
            unifiedButtons.updateState($(this), isChecked);
        });

        $('.btn-gen-custom-item .' + unifiedButtons.CLASSES.ACTION + '[name="' + actionId + '"]').each(function() {
            unifiedButtons.updateState($(this), isChecked);
        });
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
        $copyHtmlBtn.text(success ? '\u2713 ' + tr("Copied!") : '\u2717 ' + tr("Failed"));
        setTimeout(function() {
            $copyHtmlBtn.text(tr("Copy HTML"));
        }, 2000);
    }

    // ============================================================
    // ADD CUSTOM BUTTON
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
                label: data.text
                // Note: isChecked is NOT stored - server is source of truth
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

            // Determine if property is numeric
            var isNum = false;
            var typeEnum = info.typeEnum || 0;
            var typeString = info.typeString || '';

            isNum = (typeEnum >= TYPE_ENUMS.INT && typeEnum <= TYPE_ENUMS.DOUBLE) ||
                    typeEnum === TYPE_ENUMS.FLOAT ||
                    typeString === 'int' || typeString === 'double' || typeString === 'float';

            var minVal = 0;
            var maxVal = 100;
            var stepVal = 1;

            if (isNum) {
                // Use values from previewNumericValues (customized in preview)
                if (previewNumericValues && previewNumericValues[data.id] !== undefined) {
                    var previewVal = previewNumericValues[data.id];
                    minVal = parseFloat(previewVal.min) || 0;
                    maxVal = parseFloat(previewVal.max) || 100;
                    stepVal = parseFloat(previewVal.step) || 1;
                    delete previewNumericValues[data.id];
                }

                if (minVal > maxVal) {
                    var temp = minVal;
                    minVal = maxVal;
                    maxVal = temp;
                }
                if (stepVal <= 0) stepVal = 1;
            }

            customButtons.push({
                type: 'property',
                id: data.id,
                typeString: info.typeString || 'unknown',
                typeEnum: info.typeEnum || 0,
                isWritable: info.isWritable || false,
                // Note: value is NOT stored - server is source of truth
                label: data.label || data.id,
                isNumeric: isNum,
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
        if (index < 0 || index >= customButtons.length) {
            logError("Invalid index for removal: " + index);
            return;
        }

        customButtons.splice(index, 1);
        saveCustomButtons();
        renderCustomButtons();
        updateCustomCount();
        updateStatus("Button removed");
    }

    // ============================================================
    // EDIT CUSTOM BUTTON DIALOG
    // ============================================================

    function openEditDialog(index) {
        var btn = customButtons[index];
        if (!btn) return;

        editingIndex = index;
        editingButton = btn;

        var isNumeric = false;
        var minVal = '', maxVal = '', stepVal = '';

        if (btn.type === 'property' && btn.isNumeric) {
            isNumeric = true;
            minVal = btn.min !== undefined ? btn.min : 0;
            maxVal = btn.max !== undefined ? btn.max : 100;
            stepVal = btn.step !== undefined ? btn.step : 1;
        }

        var dialogHtml = '<div id="btn-gen-edit-dialog" title="' + tr("Edit Custom Button") + '">';
        dialogHtml += '    <div class="btn-gen-dialog-content">';

        // Label field
        dialogHtml += '        <div class="btn-gen-dialog-field">';
        dialogHtml += '            <label class="btn-gen-dialog-label">' + tr("Label") + ':</label>';
        dialogHtml += '            <input type="text" id="btn-gen-edit-label" class="btn-gen-dialog-input" value="' + escapeAttr(btn.label) + '">';
        dialogHtml += '        </div>';

        // Numeric fields (only for numeric properties)
        if (isNumeric) {
            dialogHtml += '        <div class="btn-gen-dialog-section">';
            dialogHtml += '            <div class="btn-gen-dialog-section-title">' + tr("Numeric Range Settings") + '</div>';

            dialogHtml += '            <div class="btn-gen-dialog-row">';
            dialogHtml += '                <label class="btn-gen-dialog-row-label">' + tr("Min") + ':</label>';
            dialogHtml += '                <input type="number" id="btn-gen-edit-min" class="btn-gen-dialog-number" value="' + minVal + '" step="any">';
            dialogHtml += '            </div>';

            dialogHtml += '            <div class="btn-gen-dialog-row">';
            dialogHtml += '                <label class="btn-gen-dialog-row-label">' + tr("Max") + ':</label>';
            dialogHtml += '                <input type="number" id="btn-gen-edit-max" class="btn-gen-dialog-number" value="' + maxVal + '" step="any">';
            dialogHtml += '            </div>';

            dialogHtml += '            <div class="btn-gen-dialog-row">';
            dialogHtml += '                <label class="btn-gen-dialog-row-label">' + tr("Step") + ':</label>';
            dialogHtml += '                <input type="number" id="btn-gen-edit-step" class="btn-gen-dialog-number" value="' + stepVal + '" step="any">';
            dialogHtml += '            </div>';
            dialogHtml += '        </div>';
        }
				//else {
            dialogHtml += '        <div class="btn-gen-dialog-section">';
            dialogHtml += '            <div class="btn-gen-dialog-info">';
            dialogHtml += '                <span class="btn-gen-dialog-info-label">' + tr("Type") + ':</span> ';
            dialogHtml += '                <span class="btn-gen-dialog-info-value">' + escapeHtml(btn.typeString || 'unknown') + '</span>';
            dialogHtml += '            </div>';
            dialogHtml += '        </div>';
        //}

        dialogHtml += '    </div>';
        dialogHtml += '</div>';

        $('#btn-gen-edit-dialog').remove();
        $('body').append(dialogHtml);

        $('#btn-gen-edit-dialog').dialog({
            modal: true,
            width: 420,
            resizable: false,
            buttons: [
                {
                    text: tr("Save"),
                    click: function() { saveEditDialog(); },
                    class: 'btn-gen-dialog-save'
                },
                {
                    text: tr("Cancel"),
                    click: function() { $(this).dialog('close'); },
                    class: 'btn-gen-dialog-cancel'
                }
            ],
            open: function() {
                $('#btn-gen-edit-label').focus().select();
            },
            close: function() {
                $(this).remove();
                editingIndex = -1;
                editingButton = null;
            }
        });
    }

    function saveEditDialog() {
        if (editingIndex < 0 || editingIndex >= customButtons.length) {
            return;
        }

        var newLabel = $('#btn-gen-edit-label').val().trim();
        if (!newLabel) {
            alert(tr("Label cannot be empty."));
            $('#btn-gen-edit-label').focus();
            return;
        }

        customButtons[editingIndex].label = newLabel;

        var $minInput = $('#btn-gen-edit-min');
        var $maxInput = $('#btn-gen-edit-max');
        var $stepInput = $('#btn-gen-edit-step');

        if ($minInput.length && $maxInput.length && $stepInput.length) {
            var minVal = parseFloat($minInput.val());
            var maxVal = parseFloat($maxInput.val());
            var stepVal = parseFloat($stepInput.val());

            if (isNaN(minVal)) minVal = 0;
            if (isNaN(maxVal)) maxVal = 100;
            if (isNaN(stepVal)) stepVal = 1;

            if (minVal > maxVal) {
                alert(tr("Min value cannot be greater than Max value."));
                $('#btn-gen-edit-min').focus();
                return;
            }
            if (stepVal <= 0) {
                alert(tr("Step value must be greater than 0."));
                $('#btn-gen-edit-step').focus();
                return;
            }

            customButtons[editingIndex].min = minVal;
            customButtons[editingIndex].max = maxVal;
            customButtons[editingIndex].step = stepVal;

            var propName = customButtons[editingIndex].id;
            if (propName) {
                numericProperties[propName] = {
                    min: minVal,
                    max: maxVal,
                    step: stepVal
                };
            }

            console.log('[BtnGen] Updated numeric range: min=' + minVal + ', max=' + maxVal + ', step=' + stepVal);
        }

        saveCustomButtons();
        renderCustomButtons();
        updateCustomCount();
        updateStatus(tr("Button updated"));

        $('#btn-gen-edit-dialog').dialog('close');
    }

    // ============================================================
    // INITIALIZE CUSTOM SLIDERS
    // ============================================================

		function initCustomSliders() {
				$customContainer.find('.slider.stelproperty').each(function() {
						var self = $(this);
						var prop = self.data('prop');

						if (self.data('slider-initialized')) {
								return;
						}

						var min = parseFloat(self.data('min')) || 0;
						var max = parseFloat(self.data('max')) || 100;
						var step = parseFloat(self.data('step')) || 1;

						self.slider({
								min: min,
								max: max,
								step: step
						});

						self.data('slider-initialized', true);

						// Bind wheel support
						bindSliderWheel(self);

						// Handle user slide - send to server
						self.off('slide.btnGenCustom').on('slide.btnGenCustom', function(evt, ui) {
								var propName = $(this).data('prop');
								if (!propName) return;
								propApi.setStelPropQueued(propName, ui.value);
								var $display = $(this).closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
								if ($display.length) {
										$display.text(ui.value);
								}
						});

						// Listen for server changes
						$(propApi).off('stelPropertyChanged:' + prop + '.btnGenCustom');
						$(propApi).on('stelPropertyChanged:' + prop + '.btnGenCustom', function(evt, propData) {
								safeSliderCall(self, 'value', propData.value);
								var $display = self.closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
								if ($display.length) {
										$display.text(propData.value);
								}
						});

						var initialValue = propApi.getStelProp(prop);
						if (initialValue !== undefined) {
								var val = parseFloat(initialValue);
								if (!isNaN(val)) {
										safeSliderCall(self, 'value', val);
										var $display = self.closest('.btn-gen-custom-slider-wrap').find('.btn-gen-custom-value');
										if ($display.length) {
												$display.text(val);
										}
								}
						}
				});
		}

    // ============================================================
    // RENDER CUSTOM BUTTONS
    // ============================================================

    function renderCustomButtons() {
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

            $customContainer.find('.btn-gen-custom-item').off();

            var html = '<div class="btn-gen-custom-grid">';

            customButtons.forEach(function(btn, index) {
						var safeLabel = escapeHtml(btn.label);
						var safeIndex = Number(index);
						var safePropId = (btn.type === 'property') ? escapeAttr(btn.id) : '';
						var minVal = Number(btn.min) || 0;
						var maxVal = Number(btn.max) || 100;
						var stepVal = Number(btn.step) || 1;

						html += '<div class="btn-gen-custom-item" data-index="' + safeIndex + '" data-prop="' + safePropId + '">';
						html += '<div class="btn-gen-custom-header">';
						html += '<span class="btn-gen-custom-type">' + (btn.type === 'action' ? '[A]' : '[P]') + '</span>';
						html += '<span class="btn-gen-custom-label">' + safeLabel + '</span>';
						if (btn.type === 'property' && btn.isNumeric) {
								html += '<span class="btn-gen-custom-numeric-badge">' + minVal + '..' + maxVal + ' Δ' + stepVal + '</span>';
						}
						html += '<button class="btn-gen-custom-edit" data-index="' + safeIndex + '" title="' + escapeAttr(tr("Edit label")) + '">Edit</button>';
						html += '<button class="btn-gen-custom-remove" data-index="' + safeIndex + '" title="' + escapeAttr(tr("Remove")) + '">✕</button>';
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

            initCustomSliders();
						
						setTimeout(function() {
								refreshCustomButtonStates();
						}, 100);

            // BIND EVENTS FOR CUSTOM BUTTONS

            // Remove button
            $customContainer.off('click.btnGenCustomRemove').on('click.btnGenCustomRemove', '.btn-gen-custom-remove', function() {
                var index = parseInt($(this).data('index'));
                removeCustomButton(index);
            });

            // Edit label button
            $customContainer.off('click.btnGenCustomEdit').on('click.btnGenCustomEdit', '.btn-gen-custom-edit', function() {
                var index = parseInt($(this).data('index'));
                if (index >= 0 && index < customButtons.length) {
                    openEditDialog(index);
                }
            });

            // Min/Max/Step input events for custom buttons
            $customContainer.off('input.btnGenCustomNumeric').on('input.btnGenCustomNumeric', '.btn-gen-numeric-min, .btn-gen-numeric-max, .btn-gen-numeric-step', function() {
                var $input = $(this);
                var propName2 = $input.data('prop');
                if (!propName2) return;

                var $wrap = $input.closest('.btn-gen-custom-item');
                var minVal = parseFloat($wrap.find('.btn-gen-numeric-min').val());
                var maxVal = parseFloat($wrap.find('.btn-gen-numeric-max').val());
                var stepVal = parseFloat($wrap.find('.btn-gen-numeric-step').val());

                if (isNaN(minVal)) minVal = 0;
                if (isNaN(maxVal)) maxVal = 100;
                if (isNaN(stepVal)) stepVal = 1;
                if (minVal > maxVal) {
                    var temp = minVal;
                    minVal = maxVal;
                    maxVal = temp;
                    $wrap.find('.btn-gen-numeric-min').val(minVal);
                    $wrap.find('.btn-gen-numeric-max').val(maxVal);
                }
                if (stepVal <= 0) stepVal = 1;

                var $slider = $wrap.find('.slider.stelproperty');
                if ($slider.length && $slider.hasClass('ui-slider')) {
                    safeSliderCall($slider, 'option', 'min', minVal);
                    safeSliderCall($slider, 'option', 'max', maxVal);
                    safeSliderCall($slider, 'option', 'step', stepVal);

                    var currentValue = $slider.slider('value');
                    if (currentValue < minVal) {
                        safeSliderCall($slider, 'value', minVal);
                        var $display = $wrap.find('.btn-gen-custom-value');
                        if ($display.length) {
                            $display.text(minVal);
                        }
                        updateProperty(propName2, minVal);
                    } else if (currentValue > maxVal) {
                        safeSliderCall($slider, 'value', maxVal);
                        var $display = $wrap.find('.btn-gen-custom-value');
                        if ($display.length) {
                            $display.text(maxVal);
                        }
                        updateProperty(propName2, maxVal);
                    }
                }
            });

            // BIND COLOR PICKER EVENTS FOR CUSTOM BUTTONS
            $customContainer.off('input.btnGenCustomColor').on('input.btnGenCustomColor', '.color-picker-wrapper .color-input', function() {
                var $wrap = $(this).closest('.color-picker-wrapper');
                var prop = $wrap.data('prop');
                var r = parseFloat($wrap.find('.color-r').val()) || 0;
                var g = parseFloat($wrap.find('.color-g').val()) || 0;
                var b = parseFloat($wrap.find('.color-b').val()) || 0;

                r = Math.max(0, Math.min(1, r));
                g = Math.max(0, Math.min(1, g));
                b = Math.max(0, Math.min(1, b));

                var color = 'rgb(' + Math.round(r * 255) + ',' + Math.round(g * 255) + ',' + Math.round(b * 255) + ')';
                $wrap.find('.color-swatch').css('background-color', color);

                if (prop) {
                    updateProperty(prop, [r, g, b]);
                }
            });

            $customContainer.off('click.btnGenCustomColorSwatch').on('click.btnGenCustomColorSwatch', '.color-picker-wrapper .color-swatch', function() {
                var $wrap = $(this).closest('.color-picker-wrapper');
                var prop = $wrap.data('prop');
                var r = parseFloat($wrap.find('.color-r').val()) || 0;
                var g = parseFloat($wrap.find('.color-g').val()) || 0;
                var b = parseFloat($wrap.find('.color-b').val()) || 0;

                var hex = '#' +
                        Math.round(r * 255).toString(16).padStart(2, '0') +
                        Math.round(g * 255).toString(16).padStart(2, '0') +
                        Math.round(b * 255).toString(16).padStart(2, '0');

                var input = document.createElement('input');
                input.type = 'color';
                input.value = hex;
                input.addEventListener('input', function() {
                    var hexVal = this.value;
                    var r2 = parseInt(hexVal.substring(1, 3), 16) / 255;
                    var g2 = parseInt(hexVal.substring(3, 5), 16) / 255;
                    var b2 = parseInt(hexVal.substring(5, 7), 16) / 255;
                    $wrap.find('.color-r').val(r2.toFixed(2));
                    $wrap.find('.color-g').val(g2.toFixed(2));
                    $wrap.find('.color-b').val(b2.toFixed(2));
                    $wrap.find('.color-swatch').css('background-color', hexVal);
                    if (prop) {
                        updateProperty(prop, [r2, g2, b2]);
                    }
                });
                input.click();
            });

            // Text property events are handled by unified system via mainui.js
            // No need for manual binding here

        } catch (error) {
            logError("Error rendering custom buttons: " + error.message);
        }

        isRendering = false;
    }
		
		// ============================================================
		// REFRESH CUSTOM BUTTON STATES
		// ============================================================
		function refreshCustomButtonStates() {
				// Upadte all stelaction state
				$('.btn-gen-custom-item .' + unifiedButtons.CLASSES.ACTION).each(function() {
						var $btn = $(this);
						var actionId = $btn.attr('name');
						if (actionId && typeof actionApi !== 'undefined' && actionApi.isChecked) {
								var isChecked = actionApi.isChecked(actionId) === true;
								unifiedButtons.updateState($btn, isChecked);
						}
				});

				// Update all stelproperty state
				$('.btn-gen-custom-item .' + unifiedButtons.CLASSES.PROPERTY_TOGGLE).each(function() {
						var $btn = $(this);
						var propName = $btn.attr('name');
						if (propName) {
								var currentValue = propApi.getStelProp(propName);
								var isChecked = (currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');
								unifiedButtons.updateState($btn, isChecked);
						}
				});

				// Update all stelproperty Qsting 
				$('.btn-gen-custom-item input[type="text"].' + unifiedButtons.CLASSES.PROPERTY_TEXT).each(function() {
						var $input = $(this);
						var propName = $input.attr('name');
						if (propName) {
								var currentValue = propApi.getStelProp(propName);
								if (currentValue !== undefined && currentValue !== null) {
										$input.val(String(currentValue));
								}
						}
				});
		}

		// ============================================================
		// RENDER ACTION CONTROL (for custom buttons)
		// ============================================================
		function renderActionControl(btn) {
				var id = btn.id;
				var label = btn.label;
				var isCheckable = btn.isCheckable || false;
				
				// Read boolean stelaction state from server
				var isChecked = false;
				if (typeof actionApi !== 'undefined' && actionApi.isChecked) {
						isChecked = actionApi.isChecked(id) === true;
				}
				
				// Use unified button system - always use stelaction
				return unifiedButtons.createButton({
						type: 'action',
						name: id,
						label: label,
						isChecked: isChecked,
						isCheckable: isCheckable
				});
		}

    // ============================================================
    // RENDER PROPERTY CONTROL (for custom buttons)
    // ============================================================

    function renderPropertyControl(btn) {
        var type = btn.typeString || 'unknown';
        var id = btn.id;
        var label = btn.label;
        var typeEnum = btn.typeEnum || 0;

        // BOOLEAN - Toggle Button (using unified system)
        if (typeEnum === TYPE_ENUMS.BOOL || type === 'bool') {
            var currentValue = propApi.getStelProp(id);
            var isChecked = (currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');
            var isCheckable = (currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');
            return unifiedButtons.createButton({
                type: 'property-toggle',
                name: id,
                label: label,
                isChecked: isChecked,
								isCheckable: isCheckable
            });
        }

        // NUMERIC - Uses slider system
				else if ((typeEnum >= TYPE_ENUMS.INT && typeEnum <= TYPE_ENUMS.DOUBLE) ||
								 typeEnum === TYPE_ENUMS.FLOAT ||
								 type === 'int' || type === 'double' || type === 'float') {
						var min = btn.min !== undefined ? btn.min : 0;
						var max = btn.max !== undefined ? btn.max : 100;
						var step = btn.step !== undefined ? btn.step : 1;

						var currentVal = propApi.getStelProp(id);
						if (currentVal === undefined) {
								currentVal = 0;
						}
						
						// Determine number format based on step precision
						var numberFormat = getNumberFormat(step);

						var html = '<div class="btn-gen-custom-slider-wrap" data-prop="' + escapeAttr(id) + '">';
						html += '    <div class="btn-gen-slider-header">';
						html += '        <label>' + escapeHtml(label) + '</label>';
						html += '        <span class="stelproperty btn-gen-custom-numeric-badge" data-prop="' + escapeAttr(id) + '" data-numberformat="' + numberFormat + '">' + currentVal + '</span>'
						//html += '        <span class="stelproperty" data-prop="' + escapeAttr(id) + '" data-numberformat="' + numberFormat + '"></span>';
						html += '    </div>';
						html += '    <div class="slider stelproperty" data-prop="' + escapeAttr(id) + '" ';
						html += '         data-min="' + min + '" data-max="' + max + '" data-step="' + step + '"></div>';
						html += '</div>';
						return html;
				}

        // COLOR - Color Picker
        else if ((typeEnum === TYPE_ENUMS.VECTOR3 || type === 'Vector3<float>' || type === 'Vector3') &&
                 id.toLowerCase().indexOf('color') !== -1) {

            var currentValue = propApi.getStelProp(id);
            var colorArray = parseColorValue(currentValue);
            var rVal = parseFloat(colorArray[0]) || 0;
            var gVal = parseFloat(colorArray[1]) || 0;
            var bVal = parseFloat(colorArray[2]) || 0;

            var html = '<div class="option-sub-control color-control">\n';
            html += '    <div class="color-picker-wrapper" data-prop="' + escapeAttr(id) + '">\n';
            html += '        <div class="color-swatch" style="background-color: rgb(' +
                            Math.round(rVal * 255) + ',' +
                            Math.round(gVal * 255) + ',' +
                            Math.round(bVal * 255) + ');"></div>\n';
            html += '        <div class="color-inputs">\n';
            html += '            <input type="number" class="color-input color-r" min="0" max="1" step="0.01" value="' + rVal.toFixed(2) + '" />\n';
            html += '            <input type="number" class="color-input color-g" min="0" max="1" step="0.01" value="' + gVal.toFixed(2) + '" />\n';
            html += '            <input type="number" class="color-input color-b" min="0" max="1" step="0.01" value="' + bVal.toFixed(2) + '" />\n';
            html += '        </div>\n';
            html += '    </div>\n';
            html += '</div>\n';
            html += '<span class="btn-gen-prop-name">' + escapeHtml(label) + '</span>\n';
            return html;
        }

    		// OTHER - Text input (using unified system)
					else {
							// Get stelproperty values from server
							var currentValue = propApi.getStelProp(id);
							var currentTextValue = currentValue !== undefined && currentValue !== null ? String(currentValue) : '';

							return unifiedButtons.createTextInput({
									name: id,
									label: label,
									value: currentTextValue
							});
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
        a.download = 'custom_buttons_' + new Date().toISOString().slice(0, 10) + '.json';
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
        }
    };
});