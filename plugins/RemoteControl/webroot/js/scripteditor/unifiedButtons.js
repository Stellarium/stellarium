/* ========================================================================
 * unifiedButtons.js - Unified Button System for Stellarium Remote Control
 * ========================================================================
 * 
 * This module provides a single, unified system for creating and managing
 * all Stellarium buttons (Actions and Properties) across the entire 
 * application. It ensures consistent behavior, appearance, and interaction
 * for all button types.
 * 
 * BUTTON TYPES:
 * - stelaction      : For StelAction (toggleable or trigger)
 * - stelproperty    : For StelProperty (direct value set)
 * - stelproperty-toggle : For StelProperty (toggle boolean)
 * - stelproperty-text   : For StelProperty (text input)
 * 
 * USAGE:
 *   <button class="stelaction" name="actionShow_Planets">Planets</button>
 *   <button class="stelproperty" name="MilkyWay.intensity" value="1.0">Reset</button>
 *   <button class="stelproperty-toggle" name="ConstellationMgr.isolateSelected">Isolate</button>
 *   <input type="text" class="stelproperty-text" name="ArchaeoLines.customAzimuth2Label" />
 *
 * You can add styled icons to the manually created buttons:
 *   <span class="action-state-icon">&#x2713;</span>  <!-- ✓ -->
 *   <span class="action-state-icon">&#x2717;</span>  <!-- ✗ -->
 *   <span class="action-state-icon">&#x25B6;</span>  <!-- ▶ -->
 *
 * @module unifiedButtons
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * @requires api/properties
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-08-15
 * @license GPLv2+
 * @version 1.0.0
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/actions", "api/properties"], 
    function($, rc, actionApi, propApi) {
    "use strict";

    // ============================================================
    // PRIVATE VARIABLES
    // ============================================================

    var initialized = false;
    var pendingActionUpdates = {};
    var updateCounter = 0;

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
    // GENERATE UNIQUE UPDATE ID
    // ============================================================

    function generateUpdateId(actionId) {
        return actionId + '_' + (++updateCounter) + '_' + Date.now();
    }

    // ============================================================
    // BUTTON SYSTEM API
    // ============================================================

    var ButtonSystem = {

        // ============================================================
        // CONSTANTS
        // ============================================================

			 ICONS: {
						CHECKED: '\u2713',   // ✓
						UNCHECKED: '\u2717', // ✗
						TRIGGER: '\u25B6'    // ▶
					},

        ICON_COLORS: {
            CHECKED: 'icon-checked',
            UNCHECKED: 'icon-unchecked',
            TRIGGER: 'icon-trigger',
            INDETERMINATE: 'icon-indeterminate'
        },

        CLASSES: {
            ACTION: 'stelaction',
            PROPERTY: 'stelproperty',
            PROPERTY_TOGGLE: 'stelproperty-toggle',
            PROPERTY_TEXT: 'stelproperty-text',
            ACTIVE: 'active',
            TRIGGER: 'action-trigger',
            STATE_CHANGING: 'state-changing',
            SEARCH_HIGHLIGHT: 'search-highlight',
            FILTERED_HIDDEN: 'filtered-hidden',
            ICON: 'action-state-icon'
        },

        // ============================================================
        // CREATE BUTTON HTML
        // ============================================================

        /**
         * Create a unified button HTML string with text icons.
         * 
         * @param {Object} config - Button configuration
         * @param {string} config.type - 'action' | 'property' | 'property-toggle' | 'property-text'
         * @param {string} config.name - The Stellarium ID (action ID or property name)
         * @param {string} [config.label] - Display label (defaults to name)
         * @param {*} [config.value] - Value (for property buttons)
         * @param {boolean} [config.isChecked=false] - Current state (for toggleable)
         * @param {boolean} [config.isCheckable=false] - Is this a toggleable action?
         * @param {Object} [config.attrs] - Additional data attributes
         * @param {string} [config.icon] - Custom icon (overrides default)
         * @param {boolean} [config.showIcon=true] - Whether to show the state icon
         * @returns {string} HTML string
         */
        createButton: function(config) {
            var type = config.type || 'action';
            var name = config.name;
            var label = config.label || name || '';
            var value = config.value;
            var isChecked = config.isChecked || false;
            var isCheckable = config.isCheckable || false;
            var attrs = config.attrs || {};
            var customIcon = config.icon || null;
            var showIcon = config.showIcon !== undefined ? config.showIcon : true;

            if (!name) {
                console.error('[ButtonSystem] createButton: Missing name');
                return '';
            }

            var cssClass = '';
            var stateIcon = '';
            var title = name;
            var extraAttrs = '';

            // Build attributes
            for (var key in attrs) {
                if (attrs.hasOwnProperty(key)) {
                    extraAttrs += ' data-' + key + '="' + escapeAttr(attrs[key]) + '"';
                }
            }

            // Add value attribute for property buttons
            if (value !== undefined && (type === 'property' || type === 'property-text')) {
                extraAttrs += ' value="' + escapeAttr(value) + '"';
            }

            // Determine CSS class and icon based on type
            switch (type) {
                case 'action':
                    cssClass = this.CLASSES.ACTION + ' button';
                    if (isCheckable) {
                        if (isChecked) cssClass += ' ' + this.CLASSES.ACTIVE;
                        title += ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')';
                        stateIcon = this._buildIcon(
                            customIcon || (isChecked ? this.ICONS.CHECKED : this.ICONS.UNCHECKED),
                            customIcon ? '' : (isChecked ? this.ICON_COLORS.CHECKED : this.ICON_COLORS.UNCHECKED)
                        );
                    } else {
                        cssClass += ' ' + this.CLASSES.TRIGGER;
                        title += ' (Trigger - Click to execute)';
                        stateIcon = this._buildIcon(
                            customIcon || this.ICONS.TRIGGER,
                            customIcon ? '' : this.ICON_COLORS.TRIGGER
                        );
                    }
                    break;

                case 'property-toggle':
                    cssClass = this.CLASSES.PROPERTY_TOGGLE;
                    if (isChecked) cssClass += ' ' + this.CLASSES.ACTIVE;
										isCheckable = true;
                    title += ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')';
                    stateIcon = this._buildIcon(
                        customIcon || (isChecked ? this.ICONS.CHECKED : this.ICONS.UNCHECKED),
                        customIcon ? '' : (isChecked ? this.ICON_COLORS.CHECKED : this.ICON_COLORS.UNCHECKED)
                    );
                    break;

                case 'property':
                    cssClass = this.CLASSES.PROPERTY;
                    if (value !== undefined) {
                        title += ' (Set value: ' + value + ')';
                    } else {
                        title += ' (Set value)';
                    }
                    break;

                case 'property-text':
                    cssClass = this.CLASSES.PROPERTY_TEXT;
                    title += ' (Text input)';
                    break;

                default:
                    console.warn('[ButtonSystem] Unknown button type:', type);
                    return '';
            }

            // Build the button HTML
            var html = '<button type="button" ' +
                       'class="' + cssClass + '" ' +
                       'name="' + escapeAttr(name) + '" ' +
                       'data-type="' + escapeAttr(type) + '" ' +
                       'data-ischecked="' + isChecked + '" ' +
                       'data-ischeckable="' + isCheckable + '" ' +
                       'title="' + escapeAttr(title) + '" ' +
                       'aria-label="' + escapeAttr(label) + '" ' +
                       //'role="button"' +
                       extraAttrs +
                       '>';

            // Add state icon
            if (showIcon && stateIcon) {
                html += stateIcon;
            }

            // Add label
            if (label) {
                html += '<span class="action-text">' + escapeHtml(label) + '</span>';
            }

            html += '</button>';
            return html;
        },

        /**
         * Build an icon span element with text icon.
         * @private
         */
        _buildIcon: function(iconText, colorClass) {
            var cls = this.CLASSES.ICON;
            if (colorClass) cls += ' ' + colorClass;
            return '<span class="' + cls + '">' + escapeHtml(iconText) + '</span>';
        },

        // ============================================================
        // CREATE TEXT INPUT HTML
        // ============================================================

        /**
         * Create a text input with apply/reset buttons.
         * 
         * @param {Object} config - Configuration
         * @param {string} config.name - Property name
         * @param {string} config.label - Display label
         * @param {string} config.value - Current value
         * @param {Object} [config.attrs] - Additional data attributes
         * @returns {string} HTML string
         */
        createTextInput: function(config) {
            var name = config.name;
            var label = config.label || name || '';
            var value = config.value || '';
            var attrs = config.attrs || {};

            if (!name) {
                console.error('[ButtonSystem] createTextInput: Missing name');
                return '';
            }

            var extraAttrs = '';
            for (var key in attrs) {
                if (attrs.hasOwnProperty(key)) {
                    extraAttrs += ' data-' + key + '="' + escapeAttr(attrs[key]) + '"';
                }
            }

            var html = '<div class="stelproperty-text-wrap" data-prop="' + escapeAttr(name) + '">';
            html += '    <label>' + escapeHtml(label) + '</label>';
            html += '    <div class="text-input-row">';
            html += '        <input type="text" class="' + this.CLASSES.PROPERTY_TEXT + '" name="' + escapeAttr(name) + '" ';
            html += '            value="' + escapeAttr(value) + '" ';
            html += '            placeholder="' + escapeAttr(tr("Enter value...")) + '" ';
            html += extraAttrs + ' />';
            html += '        <button class="stelproperty-text-apply" title="' + escapeAttr(tr("Apply value")) + '">';
            html += '            ' + escapeHtml(tr("Apply"));
            html += '        </button>';
            html += '        <button class="stelproperty-text-reset" title="' + escapeAttr(tr("Reset to server value")) + '">';
            html += '            ' + escapeHtml(tr("Reset"));
            html += '        </button>';
            html += '    </div>';
            html += '</div>';

            return html;
        },

        // ============================================================
        // BIND EVENTS TO CONTAINER
        // ============================================================

        /**
         * Bind all button events to a container.
         * 
         * @param {jQuery} $container - Container element
         * @param {Object} handlers - Event handlers
         * @param {Function} [handlers.onAction] - Called when an action is clicked
         * @param {Function} [handlers.onProperty] - Called when a property is clicked
         * @param {Function} [handlers.onToggle] - Called when a toggle property is clicked
         * @param {Function} [handlers.onPropertyReset] - Called when reset button is clicked
         */
        bindEvents: function($container, handlers) {
            handlers = handlers || {};

            // ============================================================
            // BIND ACTIONS (stelaction) - with double-click prevention
            // ============================================================
            $container.off('click.stelaction').on('click.stelaction', '.' + this.CLASSES.ACTION, function(e) {
                e.preventDefault();
                e.stopPropagation();

                var $btn = $(this);
                var actionId = $btn.attr('name');
                var isCheckable = $btn.data('ischeckable') === true;

                if (!actionId) {
										// silent error
                    //console.error('[ButtonSystem] Missing action name');
                    return;
                }

                // Prevent double execution
                if ($btn.data('executing') === true) {
                    return;
                }
                $btn.data('executing', true);

                // Visual feedback
                $btn.addClass(this.CLASSES.STATE_CHANGING);
                setTimeout(function() { 
                    $btn.removeClass(ButtonSystem.CLASSES.STATE_CHANGING);
                    $btn.data('executing', false);
                }, 300);

                if (typeof handlers.onAction === 'function') {
                    handlers.onAction(actionId, $btn, isCheckable);
                }
            }.bind(this));

            // ============================================================
            // BIND PROPERTIES (stelproperty)
            // ============================================================
            $container.off('click.stelproperty').on('click.stelproperty', '.' + this.CLASSES.PROPERTY, function(e) {
                e.preventDefault();
                e.stopPropagation();

                var $btn = $(this);
                var propName = $btn.attr('name');
                var value = $btn.attr('value');

                if (!propName) {
                    //silent error
										//console.error('[ButtonSystem] Missing property name');
                    return;
                }

                if (value === undefined) {
                    console.warn('[ButtonSystem] Missing value for stelproperty button:', propName);
                    return;
                }

                // Visual feedback
                $btn.addClass(this.CLASSES.STATE_CHANGING);
                setTimeout(function() { $btn.removeClass(ButtonSystem.CLASSES.STATE_CHANGING); }, 200);

                if (typeof handlers.onProperty === 'function') {
                    handlers.onProperty(propName, value, $btn);
                }
            }.bind(this));

            // ============================================================
            // BIND TOGGLE PROPERTIES (stelproperty-toggle)
            // ============================================================
            $container.off('click.stelproperty-toggle').on('click.stelproperty-toggle', '.' + this.CLASSES.PROPERTY_TOGGLE, function(e) {
                e.preventDefault();
                e.stopPropagation();

                var $btn = $(this);
                var propName = $btn.attr('name');

                if (!propName) {
                    console.error('[ButtonSystem] Missing property name');
                    return;
                }

                // Visual feedback
                $btn.addClass(ButtonSystem.CLASSES.STATE_CHANGING);
                setTimeout(function() { $btn.removeClass(ButtonSystem.CLASSES.STATE_CHANGING); }, 200);

                if (typeof handlers.onToggle === 'function') {
                    handlers.onToggle(propName, $btn);
                }
            });
						
            // ============================================================
            // BIND TEXT PROPERTIES (stelproperty-text) - Enter key
            // ============================================================
            $container.off('keydown.stelproperty-text').on('keydown.stelproperty-text', '.' + this.CLASSES.PROPERTY_TEXT, function(e) {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    var $input = $(this);
                    var propName = $input.attr('name');
                    var value = $input.val();

                    if (propName && value !== undefined) {
                        if (typeof handlers.onProperty === 'function') {
                            handlers.onProperty(propName, value, $input);
                        }
                    }
                }
            });

            // ============================================================
            // BIND TEXT PROPERTIES - Apply button
            // ============================================================
            $container.off('click.stelproperty-text-apply').on('click.stelproperty-text-apply', '.stelproperty-text-apply', function() {
                var $btn = $(this);
                var $wrap = $btn.closest('.stelproperty-text-wrap');
                var $input = $wrap.find('.' + ButtonSystem.CLASSES.PROPERTY_TEXT);
                var propName = $input.attr('name');
                var value = $input.val();

                if (propName && value !== undefined) {
                    if (typeof handlers.onProperty === 'function') {
                        handlers.onProperty(propName, value, $input);
                    }
                }
            });

            // ============================================================
            // BIND TEXT PROPERTIES - Reset button
            // ============================================================
            $container.off('click.stelproperty-text-reset').on('click.stelproperty-text-reset', '.stelproperty-text-reset', function() {
                var $btn = $(this);
                var $wrap = $btn.closest('.stelproperty-text-wrap');
                var $input = $wrap.find('.' + ButtonSystem.CLASSES.PROPERTY_TEXT);
                var propName = $input.attr('name');

                if (propName) {
                    if (typeof handlers.onPropertyReset === 'function') {
                        handlers.onPropertyReset(propName, $input);
                    }
                }
            });
        },

        // ============================================================
        // UPDATE BUTTON STATE
        // ============================================================

        /**
         * Update the visual state of a toggle button with text icons.
         * 
         * @param {jQuery} $btn - The button element
         * @param {boolean} isChecked - New state
         * @param {Object} [options] - Additional options
         * @param {boolean} [options.animate=true] - Whether to animate the change
         */
        updateState: function($btn, isChecked, options) {
            options = options || {};
            var animate = options.animate !== undefined ? options.animate : true;

            var type = $btn.data('type');
            var isCheckable = $btn.data('ischeckable') === true;

            // Only update toggleable buttons
            if (type === 'property-toggle' || (type === 'action' && isCheckable)) {
                // Prevent multiple updates to the same state
                var currentState = $btn.data('ischecked') || false;
                if (currentState === isChecked) {
                    return;
                }
                
                // Toggle active class
                $btn.toggleClass(this.CLASSES.ACTIVE, isChecked);
                $btn.data('ischecked', isChecked);

                // Update text icon
                var $icon = $btn.find('.' + this.CLASSES.ICON);
                if ($icon.length) {
                    // Remove all color classes
                    $icon.removeClass('icon-checked icon-unchecked icon-trigger icon-indeterminate');
                    
                    // Set text and color class
                    if (isChecked) {
                        $icon.text(this.ICONS.CHECKED);
                        $icon.addClass('icon-checked');
                    } else {
                        $icon.text(this.ICONS.UNCHECKED);
                        $icon.addClass('icon-unchecked');
                    }
                }

                // Update tooltip
                var name = $btn.attr('name');
                if (name) {
                    $btn.attr('title', name + ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')');
                }

                // Animation feedback
                if (animate) {
                    $btn.addClass(this.CLASSES.STATE_CHANGING);
                    setTimeout(function() { $btn.removeClass(ButtonSystem.CLASSES.STATE_CHANGING); }, 200);
                }
            }
        },

        /**
         * Update the displayed value of a property element.
         * 
         * @param {jQuery} $el - The element (button, input, span, etc.)
         * @param {*} value - The value to display
         * @param {Object} [options] - Additional options
         */
        updatePropertyDisplay: function($el, value, options) {
            options = options || {};

            var type = $el.data('type') || $el.attr('type');

            if (type === 'property-toggle' || type === 'action') {
                this.updateState($el, value === true || value === 'true' || value === 1 || value === '1', options);
            } else if (type === 'text' || $el.is('input[type="text"]')) {
                $el.val(value !== undefined && value !== null ? String(value) : '');
            } else if ($el.is('span')) {
                var numberformat = $el.data('numberformat');
                if (numberformat && window.Globalize) {
                    $el.text(Globalize.format(value, numberformat));
                } else {
                    $el.text(value !== undefined && value !== null ? String(value) : '');
                }
            } else if ($el.is('button') || $el.is('input[type="button"]')) {
                // For buttons, update the text content if no icon
                var $icon = $el.find('.' + this.CLASSES.ICON);
                if ($icon.length === 0) {
                    $el.text(value !== undefined && value !== null ? String(value) : '');
                }
            }
        },

        // ============================================================
        // FILTER / SEARCH HELPERS
        // ============================================================

        /**
         * Filter buttons by search query.
         * 
         * @param {jQuery} $container - Container with buttons
         * @param {string} query - Search query
         * @param {Object} options - Filter options
         * @param {string} [options.hiddenClass='filtered-hidden'] - Class for hidden items
         * @param {string} [options.highlightClass='search-highlight'] - Class for highlighted items
         * @param {number} [options.minChars=1] - Minimum characters to trigger filter
         * @param {Function} [options.onFilter] - Called with visible count
         */
        filterButtons: function($container, query, options) {
            options = options || {};
            var hiddenClass = options.hiddenClass || this.CLASSES.FILTERED_HIDDEN;
            var highlightClass = options.highlightClass || this.CLASSES.SEARCH_HIGHLIGHT;
            var minChars = options.minChars || 1;
            var onFilter = options.onFilter || null;

            var totalVisible = 0;

            if (!query || query.length < minChars) {
                // Reset filter
                $container.find('.' + this.CLASSES.ACTION + ', .' + this.CLASSES.PROPERTY + ', .' + this.CLASSES.PROPERTY_TOGGLE)
                    .removeClass(hiddenClass + ' ' + highlightClass);
                $container.find('.actions-category-panel, .btn-gen-custom-item').show();
                if (onFilter) onFilter(totalVisible);
                return;
            }

            query = query.toLowerCase();

            $container.find('.' + this.CLASSES.ACTION + ', .' + this.CLASSES.PROPERTY + ', .' + this.CLASSES.PROPERTY_TOGGLE).each(function() {
                var $btn = $(this);
                var name = ($btn.attr('name') || '').toLowerCase();
                var text = ($btn.find('.action-text').text() || '').toLowerCase();
                var category = ($btn.data('category') || '').toLowerCase();
                var label = ($btn.data('label') || '').toLowerCase();

                var matches = name.indexOf(query) >= 0 ||
                              text.indexOf(query) >= 0 ||
                              category.indexOf(query) >= 0 ||
                              label.indexOf(query) >= 0;

                if (matches) {
                    $btn.removeClass(hiddenClass);
                    $btn.addClass(highlightClass);
                    totalVisible++;
                } else {
                    $btn.addClass(hiddenClass);
                    $btn.removeClass(highlightClass);
                }
            });

            // Show/hide panels
            $container.find('.actions-category-panel, .btn-gen-custom-item').each(function() {
                var $panel = $(this);
                var visible = $panel.find('.' + ButtonSystem.CLASSES.ACTION + ':not(.' + hiddenClass + '), ' +
                                         '.' + ButtonSystem.CLASSES.PROPERTY + ':not(.' + hiddenClass + '), ' +
                                         '.' + ButtonSystem.CLASSES.PROPERTY_TOGGLE + ':not(.' + hiddenClass + ')').length > 0;
                $panel.toggle(visible);
            });

            if (onFilter) onFilter(totalVisible);
        },

        // ============================================================
        // INITIALIZE GLOBAL BINDING
        // ============================================================

        /**
         * Initialize the unified button system globally.
         * This binds events to the entire document.
         */
        init: function() {
            if (initialized) {
                console.log('[ButtonSystem] Already initialized');
                return;
            }

            console.log('[ButtonSystem] Initializing...');

            // ============================================================
            // GLOBAL EVENT BINDING
            // ============================================================
            this.bindEvents($(document), {
                onAction: function(actionId, $btn, isCheckable) {
                    console.log('[ButtonSystem] Executing action:', actionId);
                    
                    // Track this update to prevent rebounding
                    var updateId = generateUpdateId(actionId);
                    pendingActionUpdates[updateId] = actionId;
                    
                    // Optimistic UI update (for toggleable actions)
                    if (isCheckable) {
                        var currentState = $btn.data('ischecked') || false;
                        ButtonSystem.updateState($btn, !currentState, { animate: false });
                    }
                    
                    if (typeof actionApi !== 'undefined' && actionApi.execute) {
                        actionApi.execute(actionId);
                    } else {
                        console.warn('[ButtonSystem] actionApi not available');
                    }
                    
                    // Clean up pending update after delay
                    setTimeout(function() {
                        for (var key in pendingActionUpdates) {
                            if (pendingActionUpdates[key] === actionId) {
                                delete pendingActionUpdates[key];
                            }
                        }
                    }, 500);
                },

                onProperty: function(propName, value, $el) {
                    console.log('[ButtonSystem] Setting property:', propName, '=', value);
                    if (typeof propApi !== 'undefined' && propApi.setStelProp) {
                        propApi.setStelProp(propName, value);
                    } else {
                        console.warn('[ButtonSystem] propApi not available');
                    }
                },

                onToggle: function(propName, $btn) {
                    console.log('[ButtonSystem] Toggling property:', propName);
                    if (typeof propApi !== 'undefined' && propApi.getStelProp && propApi.setStelProp) {
                        var currentValue = propApi.getStelProp(propName);
                        var newValue = !(currentValue === true || currentValue === 'true' || currentValue === 1 || currentValue === '1');
                        propApi.setStelProp(propName, newValue);
                    } else {
                        console.warn('[ButtonSystem] propApi not available');
                    }
                },

                onPropertyReset: function(propName, $input) {
                    console.log('[ButtonSystem] Resetting property:', propName);
                    if (typeof propApi !== 'undefined' && propApi.getStelProp) {
                        var serverValue = propApi.getStelProp(propName);
                        if (serverValue !== undefined) {
                            $input.val(serverValue);
                        }
                    } else {
                        console.warn('[ButtonSystem] propApi not available');
                    }
                }
            });

            // ============================================================
            // SERVER STATE SYNC
            // ============================================================

            // Property changes
            if (typeof propApi !== 'undefined') {
                $(propApi).on("stelPropertyChanged", function(evt, propName, propData) {
                    var value = propData.value;

                    // Update toggle buttons
                    $('.' + ButtonSystem.CLASSES.PROPERTY_TOGGLE + '[name="' + propName + '"]').each(function() {
                        ButtonSystem.updateState($(this), value === true || value === 'true' || value === 1 || value === '1');
                    });

                    // Update text inputs
                    $('.' + ButtonSystem.CLASSES.PROPERTY_TEXT + '[name="' + propName + '"]').each(function() {
                        $(this).val(value !== undefined && value !== null ? String(value) : '');
                    });

                    // Update display spans
                    $('span.stelproperty[data-prop="' + propName + '"]').each(function() {
                        var $span = $(this);
                        var numberformat = $span.data('numberformat');
                        if (numberformat && window.Globalize) {
                            $span.text(Globalize.format(value, numberformat));
                        } else {
                            $span.text(value !== undefined && value !== null ? String(value) : '');
                        }
                    });
                });
            }

            // ============================================================
            // ACTION CHANGES - with rebounding protection
            // ============================================================
            if (typeof actionApi !== 'undefined') {
                $(actionApi).on("stelActionChanged", function(evt, actionId, actionData) {
                    // Check if this update was initiated by us
                    var isPending = false;
                    for (var key in pendingActionUpdates) {
                        if (pendingActionUpdates[key] === actionId) {
                            isPending = true;
                            delete pendingActionUpdates[key];
                            break;
                        }
                    }
                    
                    if (isPending) {
                        console.log('[ButtonSystem] Skipping self-triggered update for:', actionId);
                        return;
                    }
                    
                    console.log('[ButtonSystem] External action change detected:', actionId, actionData.isChecked);
                    
                    var isChecked = actionData.isChecked === true;
                    
                    $('.' + ButtonSystem.CLASSES.ACTION + '[name="' + actionId + '"]').each(function() {
                        var currentState = $(this).data('ischecked') || false;
                        if (currentState !== isChecked) {
                            ButtonSystem.updateState($(this), isChecked);
                        }
                    });
                });
            }

            initialized = true;
            console.log('[ButtonSystem] Initialized successfully');
        },

        // ============================================================
        // UTILITY: Check if initialized
        // ============================================================

        isInitialized: function() {
            return initialized;
        }
    };

    // ============================================================
    // PUBLIC API
    // ============================================================

    return {
        init: ButtonSystem.init.bind(ButtonSystem),
        createButton: ButtonSystem.createButton.bind(ButtonSystem),
        createTextInput: ButtonSystem.createTextInput.bind(ButtonSystem),
        bindEvents: ButtonSystem.bindEvents.bind(ButtonSystem),
        updateState: ButtonSystem.updateState.bind(ButtonSystem),
        updatePropertyDisplay: ButtonSystem.updatePropertyDisplay.bind(ButtonSystem),
        filterButtons: ButtonSystem.filterButtons.bind(ButtonSystem),
        isInitialized: ButtonSystem.isInitialized.bind(ButtonSystem),
        CLASSES: ButtonSystem.CLASSES,
        ICONS: ButtonSystem.ICONS,
        ICON_COLORS: ButtonSystem.ICON_COLORS
    };
});