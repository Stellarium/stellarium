/* ========================================================================
 * actionsCategorized.js - Sky Culture Style Action Buttons (REFACTORED v2.1)
 * ========================================================================
 * 
 * This module provides a categorized button grid interface for Stellarium
 * actions, similar to the Sky Culture tab design. It displays actions in
 * categorized tabs with visual state indicators and real-time synchronization.
 * 
 * KEY IMPROVEMENTS (v2.1):
 * - Click executes actions immediately (fast response)
 * - Double-click also executes (backward compatibility)
 * - Proper source tracking for updates (prevents self-triggered updates)
 * - Clean separation from btnGenerator.js (no more conflicts)
 * - Simplified event handling with pending updates tracking
 * - No double-execution or value rebounding
 * 
 * @module actionsCategorized
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-06-27
 * @license GPLv2+
 * @version 2.1.0
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/actions"], 
    function($, rc, actionsApi) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    var options = {};
    var allActions = [];
    var categorizedActions = {};
    var categoryOrder = [];
    var selectedActionId = null;
    var actionDataCache = {};
    var pendingActionUpdates = {};      // Track updates we initiated
    var isInternalUpdate = false;        // Flag to prevent self-triggered updates
    
    // DOM Elements
    var $searchInput = null;
    var $countDisplay = null;
    var $categoryTabs = null;
    var $categoryPanels = null;
    var $activeCategoryName = null;

    // =====================================================================
    // DEFAULT OPTIONS
    // =====================================================================

    var defaultOptions = {
        // Core DOM selectors
        containerSelector: '#stelaction-categorized',
        categoryTabsId: 'actions-category-tabs',
        categoryTabListId: 'actions-category-tab-list',
        categoryPanelsId: 'actions-category-panels',
        searchInputId: 'categorized-action-search',
        countDisplayId: 'categorized-action-count',
        activeCategoryDisplayId: 'categorized-active-category',
        
        // UI Behavior
        doubleClickToExecute: true,      // Keep for backward compatibility
        autoRefreshStates: false,        // DISABLED by default - prevents rebounding
        refreshInterval: 0,
        animateChanges: true,
        maxButtonsPerCategory: 0,
        showTriggerIcons: true,
        
        // Search behavior
        searchMinChars: 1,
        searchHighlightClass: 'search-highlight',
        searchHiddenClass: 'filtered-hidden',
        
        // CSS classes
        buttonActiveClass: 'active',
        buttonSelectedClass: 'selected',
        buttonTriggerClass: 'action-trigger',
        
        // UI Text
        loadingText: 'Loading actions...',
        errorText: 'Failed to load actions. Is Stellarium running?',
        emptyText: 'No actions available in this category',
        
        // API endpoints
        apiEndpoint: '/api/stelaction/list',
        apiActionEndpoint: '/api/stelaction/do',
        
        // Callbacks
        onActionExecuted: null,
        onActionsLoaded: null,
        onCategoryChanged: null,
        onSearchChanged: null,
        onError: null
    };

    // =====================================================================
    // HELPER FUNCTIONS
    // =====================================================================

    /**
     * Escape HTML special characters for safe DOM insertion.
     * @param {string} text - Text to escape
     * @returns {string} Escaped text
     */
    function escapeHtml(text) {
        if (!text) return '';
        var div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    /**
     * Escape HTML special characters for use in HTML attributes.
     * @param {string} text - Text to escape
     * @returns {string} Escaped text
     */
    function escapeAttr(text) {
        if (!text) return '';
        return String(text).replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/'/g, '&#39;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }

    /**
     * Generates a unique ID for tracking pending updates.
     * @param {string} actionId - The action identifier
     * @returns {string} Unique update ID
     */
    function generateUpdateId(actionId) {
        return actionId + '_' + Date.now() + '_' + Math.random().toString(36).substr(2, 5);
    }

    /**
     * Checks if an action update is pending (initiated by this module).
     * @param {string} actionId - The action identifier
     * @returns {boolean} True if the update is pending
     */
    function isActionUpdatePending(actionId) {
        for (var key in pendingActionUpdates) {
            if (pendingActionUpdates[key] === actionId) {
                return true;
            }
        }
        return false;
    }

    /**
     * Cleans up stale pending updates.
     * @param {string} actionId - Optional specific action ID to clean up
     */
    function cleanupPendingUpdates(actionId) {
        var now = Date.now();
        var keysToRemove = [];
        
        for (var key in pendingActionUpdates) {
            var parts = key.split('_');
            var timestamp = parseInt(parts[1], 10);
            
            // Remove updates older than 2 seconds
            if (now - timestamp > 2000) {
                keysToRemove.push(key);
            }
            
            // Remove specific action if provided
            if (actionId && pendingActionUpdates[key] === actionId) {
                keysToRemove.push(key);
            }
        }
        
        keysToRemove.forEach(function(key) {
            delete pendingActionUpdates[key];
        });
        
        if (keysToRemove.length > 0) {
            console.log('[ActionsCategorized] Cleaned up ' + keysToRemove.length + ' pending updates');
        }
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the categorized actions module.
     * @param {Object} opts - Configuration options
     */
    function init(opts) {
        options = $.extend(true, {}, defaultOptions, opts || {});
        
        console.log('[ActionsCategorized] Initializing v2.1 (refactored with click execution)...');
        
        var $container = $(options.containerSelector);
        if (!$container.length) {
            console.warn('[ActionsCategorized] Container not found:', options.containerSelector);
            return;
        }
        
        cacheDomElements();
        
        if (!$categoryTabs.length || !$categoryPanels.length) {
            console.warn('[ActionsCategorized] Required DOM elements not found');
            return;
        }
        
        // Reset pending updates on init
        pendingActionUpdates = {};
        isInternalUpdate = false;
        
        loadActions();
        bindEvents();
        setupStateSyncListener();
        
        console.log('[ActionsCategorized] Initialized successfully');
    }

    /**
     * Cache DOM element references for performance.
     */
    function cacheDomElements() {
        $searchInput = $('#' + options.searchInputId);
        $countDisplay = $('#' + options.countDisplayId);
        $categoryTabs = $('#' + options.categoryTabListId);
        $categoryPanels = $('#' + options.categoryPanelsId);
        $activeCategoryName = $('#' + options.activeCategoryDisplayId);
        
        if (!$searchInput.length) {
            console.warn('[ActionsCategorized] Search input not found: #' + options.searchInputId);
        }
        if (!$categoryTabs.length) {
            console.warn('[ActionsCategorized] Category tabs not found: #' + options.categoryTabListId);
        }
        
        // Create count display if missing
        if (!$countDisplay.length && options.countDisplayId) {
            $countDisplay = $('<span id="' + options.countDisplayId + '" class="pattern-count" style="margin-left:10px;"></span>');
            var $searchContainer = $searchInput.closest('.action-search-container');
            if ($searchContainer.length) {
                $searchContainer.append($countDisplay);
            }
        }
    }

    // =====================================================================
    // REAL-TIME STATE SYNCHRONIZATION (IMPROVED)
    // =====================================================================

    /**
     * Set up event listener for real-time action state changes from server.
     * IMPROVED: Now tracks pending updates to prevent self-triggered updates.
     */
    function setupStateSyncListener() {
        if (typeof actionsApi === 'undefined') {
            console.warn('[ActionsCategorized] actionsApi not available, state sync disabled');
            return;
        }
        
        $(actionsApi).on("stelActionChanged", function(event, actionId, actionData) {
            // ============================================================
            // CRITICAL: Check if this update was initiated by us
            // ============================================================
            var isPending = isActionUpdatePending(actionId);
            
            if (isPending) {
                console.log("[ActionsCategorized] Self-triggered update detected for:", actionId, 
                            " - this is our own echo, updating silently");
                // Clean up the pending update
                cleanupPendingUpdates(actionId);
                // But still update the UI with the server's confirmed state
                // (this prevents desync if the server state differs from our optimistic update)
                var $btn = $('.action-btn-item[data-action-id="' + actionId + '"]');
                if ($btn.length) {
                    updateButtonStateFromServer($btn, actionId, actionData);
                }
                return;
            }
            
            // ============================================================
            // External update (from Stellarium GUI, scripts, or other clients)
            // ============================================================
            console.log("[ActionsCategorized] External state change detected:", actionId, actionData.isChecked);
            
            var $btn = $('.action-btn-item[data-action-id="' + actionId + '"]');
            if ($btn.length) {
                // Update the button state from server data
                updateButtonStateFromServer($btn, actionId, actionData);
            }
        });
        
        console.log('[ActionsCategorized] State sync listener established (with pending updates tracking)');
    }

    /**
     * Update button UI based on server state change.
     * @param {jQuery} $btn - The button element
     * @param {string} actionId - Action identifier
     * @param {Object} actionData - Action data from server
     */
    function updateButtonStateFromServer($btn, actionId, actionData) {
        var isChecked = actionData.isChecked === true;
        var isCheckable = actionData.isCheckable === true;
        
        // Update cache
        if (actionDataCache[actionId]) {
            actionDataCache[actionId].isChecked = isChecked;
        }
        
        // Update button visual state
        $btn.toggleClass(options.buttonActiveClass, isCheckable && isChecked);
        $btn.data('ischecked', isChecked);
        
        // Update icon
        var $icon = $btn.find('.action-state-icon');
        if ($icon.length && isCheckable) {
            if (isChecked) {
                $icon.text('✓').removeClass('unchecked').addClass('checked');
            } else {
                $icon.text('✗').removeClass('checked').addClass('unchecked');
            }
        }
        
        // Update tooltip
        if (isCheckable) {
            $btn.attr('title', actionId + ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')');
        }
        
        // Animation feedback
        if (options.animateChanges) {
            $btn.addClass('state-changing');
            setTimeout(function() { $btn.removeClass('state-changing'); }, 200);
        }
    }

    // =====================================================================
    // LOAD ACTIONS FROM API
    // =====================================================================

    /**
     * Load actions from Stellarium server.
     */
    function loadActions() {
        console.log('[ActionsCategorized] Loading actions...');
        
        showLoading();
        
        if (typeof actionsApi !== 'undefined' && actionsApi.loadActionList) {
            actionsApi.loadActionList(function(data) {
                processActionsData(data);
            });
        } else {
            $.ajax({
                url: options.apiEndpoint,
                dataType: 'json',
                success: function(data) {
                    processActionsData(data);
                },
                error: function(xhr, status, error) {
                    handleLoadError(error);
                }
            });
        }
    }

    /**
     * Show loading indicator.
     */
    function showLoading() {
        $categoryTabs.html('<li><a href="#">⏳</a></li>');
        $categoryPanels.html(
            '<div class="actions-category-panel">' +
            '<div class="loading-placeholder">' + options.loadingText + '</div>' +
            '</div>'
        );
    }

    /**
     * Handle load error.
     * @param {string|Object} error - Error information
     */
    function handleLoadError(error) {
        console.error('[ActionsCategorized] Failed to load actions:', error);
        
        $categoryPanels.html(
            '<div class="actions-category-panel">' +
            '<div class="loading-placeholder" style="color: #F92672;">' +
            options.errorText +
            '</div></div>'
        );
        
        if (typeof options.onError === 'function') {
            options.onError(error);
        }
    }

    // =====================================================================
    // PROCESS ACTIONS DATA
    // =====================================================================

    /**
     * Process raw actions data from API.
     * @param {Object} data - Raw actions data from server
     */
    function processActionsData(data) {
        console.log('[ActionsCategorized] Processing actions data...');
        
        allActions = [];
        categorizedActions = {};
        categoryOrder = [];
        actionDataCache = {};
        
        if (!data || typeof data !== 'object') {
            console.warn('[ActionsCategorized] Invalid data received');
            handleLoadError('Invalid data format');
            return;
        }
        
        var categories = Object.keys(data);
        
        categories.forEach(function(categoryName) {
            var actions = data[categoryName];
            
            if (!Array.isArray(actions)) return;
            if (actions.length === 0) return;
            
            categoryOrder.push(categoryName);
            categorizedActions[categoryName] = [];
            
            var actionLimit = options.maxButtonsPerCategory || actions.length;
            var limitedActions = actions.slice(0, actionLimit);
            
            limitedActions.forEach(function(action) {
                action.category = categoryName;
                action.isToggleable = action.isCheckable === true;
                categorizedActions[categoryName].push(action);
                allActions.push(action);
                
                // Cache action data
                actionDataCache[action.id] = {
                    isCheckable: action.isCheckable,
                    isChecked: action.isChecked,
                    text: action.text
                };
            });
        });
        
        console.log('[ActionsCategorized] Loaded ' + allActions.length + 
                    ' actions in ' + categoryOrder.length + ' categories');
        
        // Reset pending updates after loading
        pendingActionUpdates = {};
        isInternalUpdate = false;
        
        buildUI();
        
        if (typeof options.onActionsLoaded === 'function') {
            options.onActionsLoaded(allActions.length, categoryOrder.length);
        }
    }

    // =====================================================================
    // BUILD USER INTERFACE
    // =====================================================================

    /**
     * Build complete UI from processed actions data.
     */
    function buildUI() {
        buildCategoryTabs();
        buildCategoryPanels();
        updateActionCount();
        initTabs();
    }

    /**
     * Build category tabs navigation.
     */
    function buildCategoryTabs() {
        var html = '';
        
        categoryOrder.forEach(function(categoryName, index) {
            var actionCount = categorizedActions[categoryName].length;
            html += '<li>';
            html += '<a href="#actions-panel-' + index + '">';
            html += escapeHtml(categoryName);
            html += '<span class="category-badge">' + actionCount + '</span>';
            html += '</a>';
            html += '</li>';
        });
        
        $categoryTabs.html(html);
    }

    /**
     * Build category panels with action buttons.
     */
    function buildCategoryPanels() {
        var html = '';
        
        categoryOrder.forEach(function(categoryName, index) {
            var actions = categorizedActions[categoryName];
            var toggleableCount = actions.filter(function(a) { return a.isToggleable; }).length;
            var triggerCount = actions.length - toggleableCount;
            
            html += '<div id="actions-panel-' + index + '" class="actions-category-panel">';
            
            html += '<p class="actions-note">';
            html += '<strong>' + actions.length + '</strong> action(s)';
            if (toggleableCount > 0) {
                html += ' · <span class="toggleable-badge">' + toggleableCount + ' toggleable</span>';
            }
            if (triggerCount > 0) {
                html += ' · <span class="trigger-badge">' + triggerCount + ' trigger</span>';
            }
            html += ' · Click to execute, double-click also executes';
            html += '</p>';
            
            html += '<div class="actions-buttons-grid" data-category="' + escapeAttr(categoryName) + '">';
            
            actions.forEach(function(action) {
                html += buildActionButton(action);
            });
            
            html += '</div>';
            html += '</div>';
        });
        
        $categoryPanels.html(html);
        
        bindActionButtonEvents();
    }

    /**
     * Build HTML for a single action button.
     * @param {Object} action - Action object
     * @returns {string} Button HTML
     */
    function buildActionButton(action) {
        var isToggleable = action.isCheckable === true;
        var isChecked = action.isChecked === true;
        var stateIcon = '';
        var cssClass = 'action-btn-item';
        var titleText = action.id;
        
        if (isToggleable) {
            stateIcon = '<span class="action-state-icon ' + (isChecked ? 'checked' : 'unchecked') + '">' +
                        (isChecked ? '✓' : '✗') + '</span>';
            if (isChecked) {
                cssClass += ' ' + options.buttonActiveClass;
            }
            titleText += ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')';
        } else {
            if (options.showTriggerIcons) {
                cssClass += ' ' + options.buttonTriggerClass;
                stateIcon = '<span class="action-state-icon">▶</span>';
            }
            titleText += ' (Trigger - Click to execute)';
        }
        
        return '<button type="button" ' +
               'class="' + cssClass + '" ' +
               'data-action-id="' + escapeAttr(action.id) + '" ' +
               'data-category="' + escapeAttr(action.category || '') + '" ' +
               'data-ischeckable="' + isToggleable + '" ' +
               'data-ischecked="' + isChecked + '" ' +
               'data-action-text="' + escapeAttr(action.text || action.id) + '" ' +
               'title="' + escapeAttr(titleText) + '" ' +
               'aria-label="' + escapeAttr(action.text || action.id) + '" ' +
               'role="button">' +
               stateIcon +
               '<span class="action-text">' + escapeHtml(action.text || action.id) + '</span>' +
               '</button>';
    }

    /**
     * Initialize jQuery UI tabs or fallback to manual tabs.
     */
    function initTabs() {
        var tabsSelector = '#' + options.categoryTabsId;
        
        if ($.fn.tabs && $(tabsSelector).tabs) {
            try {
                $(tabsSelector).tabs({
                    active: 0,
                    create: function() {
                        console.log('[ActionsCategorized] jQuery UI Tabs created');
                        updateActiveCategory();
                    },
                    activate: function(event, ui) {
                        updateActionCount();
                        updateActiveCategory();
                        
                        var categoryName = getActiveCategory();
                        if (typeof options.onCategoryChanged === 'function') {
                            options.onCategoryChanged(categoryName);
                        }
                    }
                });
            } catch(e) {
                console.warn('[ActionsCategorized] jQuery UI Tabs error:', e.message);
                buildManualTabs();
            }
        } else {
            buildManualTabs();
        }
    }

    /**
     * Update the active category display.
     */
    function updateActiveCategory() {
        var categoryName = getActiveCategory();
        if ($activeCategoryName && $activeCategoryName.length) {
            $activeCategoryName.text(categoryName || '');
        }
    }

    /**
     * Build manual tabs (fallback when jQuery UI is not available).
     */
    function buildManualTabs() {
        $('.actions-category-panel').hide();
        
        if (categoryOrder.length > 0) {
            $('#actions-panel-0').show();
            updateActiveCategory();
        }
        
        $categoryTabs.find('li a').off('click.actionsTabs').on('click.actionsTabs', function(e) {
            e.preventDefault();
            
            var $link = $(this);
            var href = $link.attr('href');
            
            $categoryTabs.find('li').removeClass('ui-tabs-active');
            $('.actions-category-panel').hide();
            
            $link.parent('li').addClass('ui-tabs-active');
            $(href).show();
            
            updateActionCount();
            updateActiveCategory();
            
            var categoryName = getActiveCategory();
            if (typeof options.onCategoryChanged === 'function') {
                options.onCategoryChanged(categoryName);
            }
        });
        
        $categoryTabs.find('li:first').addClass('ui-tabs-active');
    }

    // =====================================================================
    // EVENT BINDING
    // =====================================================================

    /**
     * Bind UI events.
     */
    function bindEvents() {
        $searchInput.off('input.actionsCat').on('input.actionsCat', function() {
            var query = $(this).val().toLowerCase().trim();
            filterActions(query);
        });
        
        $searchInput.off('keydown.actionsCat').on('keydown.actionsCat', function(e) {
            if (e.key === 'Enter' && selectedActionId) {
                e.preventDefault();
                executeAction(selectedActionId);
            } else if (e.key === 'Escape') {
                $(this).val('').trigger('input');
            }
        });
        
        // Auto-refresh is disabled by default to prevent rebounding
        if (options.autoRefreshStates && options.refreshInterval > 0) {
            console.warn('[ActionsCategorized] Auto-refresh is enabled - this may cause rebounding issues');
            setInterval(refreshActionStates, options.refreshInterval);
        }
    }

    /**
     * Bind action button events (click, double-click).
     * FIXED: Click executes the action immediately (fast response).
     * Double-click also executes (backward compatibility).
     */
    function bindActionButtonEvents() {
        // ============================================================
        // Click event: Execute the action with proper tracking
        // ============================================================
        $('.action-btn-item').off('click.actionsBtn').on('click.actionsBtn', function(e) {
            e.preventDefault();
            e.stopPropagation();
            
            var $btn = $(this);
            var actionId = $btn.data('action-id');
            
            // Visual feedback: slight scale animation
            if (options.animateChanges) {
                $btn.css('transform', 'scale(0.92)');
                setTimeout(function() { $btn.css('transform', ''); }, 150);
            }
            
            // Deselect all other buttons
            $('.action-btn-item').removeClass(options.buttonSelectedClass);
            
            // Select this button
            $btn.addClass(options.buttonSelectedClass);
            selectedActionId = actionId;
            
            // ============================================================
            // EXECUTE THE ACTION IMMEDIATELY
            // ============================================================
            console.log('[ActionsCategorized] Executing action via click:', actionId);
            executeAction(actionId);
        });
        
        // ============================================================
        // Double-click event: Also executes (as a fallback)
        // ============================================================
        if (options.doubleClickToExecute) {
            $('.action-btn-item').off('dblclick.actionsBtn').on('dblclick.actionsBtn', function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                var actionId = $(this).data('action-id');
                console.log('[ActionsCategorized] Executing action via double-click:', actionId);
                executeAction(actionId);
            });
        }
    }

    // =====================================================================
    // EXECUTE ACTION (WITH PROPER TRACKING)
    // =====================================================================

    /**
     * Execute an action by ID with proper tracking to prevent rebounding.
     * @param {string} actionId - Action identifier
     * @returns {boolean} Success status
     */
    function executeAction(actionId) {
        console.log('[ActionsCategorized] Executing action:', actionId);
        
        var $btn = $('.action-btn-item[data-action-id="' + actionId + '"]');
        
        // Visual feedback for execution
        if (options.animateChanges && $btn.length) {
            $btn.css('transform', 'scale(0.95)');
            setTimeout(function() { $btn.css('transform', ''); }, 150);
        }
        
        // ============================================================
        // CRITICAL: Track this update to prevent rebounding
        // ============================================================
        var updateId = generateUpdateId(actionId);
        pendingActionUpdates[updateId] = actionId;
        
        // Set the internal update flag
        isInternalUpdate = true;
        
        var success = true;
        
        // Execute the action via the API
        if (typeof actionsApi !== 'undefined' && actionsApi.doAction) {
            actionsApi.doAction(actionId)
                .then(function() {
                    handleActionSuccess(actionId);
                })
                .fail(function(error) {
                    handleActionError(actionId, error);
                    success = false;
                    // Clean up the pending update on error
                    cleanupPendingUpdates(actionId);
                    isInternalUpdate = false;
                });
        } else {
            $.ajax({
                url: options.apiActionEndpoint,
                method: 'POST',
                data: { id: actionId },
                success: function() {
                    handleActionSuccess(actionId);
                },
                error: function(xhr, status, error) {
                    handleActionError(actionId, error);
                    success = false;
                    cleanupPendingUpdates(actionId);
                    isInternalUpdate = false;
                }
            });
        }
        
        // Callback
        if (typeof options.onActionExecuted === 'function') {
            setTimeout(function() {
                options.onActionExecuted(actionId, success);
            }, 300);
        }
        
        return true;
    }

    /**
     * Handle successful action execution.
     * IMPROVED: Does NOT refresh all actions - only updates the specific button.
     * @param {string} actionId - Action identifier
     */
    function handleActionSuccess(actionId) {
        console.log('[ActionsCategorized] Action executed successfully:', actionId);
        
        // ============================================================
        // CRITICAL FIX: Do NOT call refreshActionStates() here
        // This prevents the rebounding issue
        // ============================================================
        
        // Instead, wait for the stelActionChanged event to update the button
        // The pending update tracking will handle the rest
        
        // Clean up the pending update after a delay
        setTimeout(function() {
            cleanupPendingUpdates(actionId);
            isInternalUpdate = false;
        }, 100);
    }

    /**
     * Handle action execution error.
     * @param {string} actionId - Action identifier
     * @param {string|Object} error - Error information
     */
    function handleActionError(actionId, error) {
        console.error('[ActionsCategorized] Failed to execute action:', actionId, error);
        // Clean up the pending update
        cleanupPendingUpdates(actionId);
        isInternalUpdate = false;
    }

    // =====================================================================
    // REFRESH ACTION STATES (DISABLED BY DEFAULT)
    // =====================================================================

    /**
     * Refresh all action states from server.
     * WARNING: This function is disabled by default to prevent rebounding.
     * Only use this if you explicitly enable autoRefreshStates in options.
     * @param {Object} [data] - Optional data to update from
     */
    function refreshActionStates(data) {
        // If data is provided, use it directly
        if (data) {
            updateActionStates(data);
            return;
        }
        
        // Otherwise, fetch from server
        if (typeof actionsApi !== 'undefined' && actionsApi.loadActionList) {
            actionsApi.loadActionList(function(data) {
                updateActionStates(data);
            });
        } else {
            $.ajax({
                url: options.apiEndpoint,
                dataType: 'json',
                success: function(data) {
                    updateActionStates(data);
                },
                error: function() {
                    console.warn('[ActionsCategorized] Failed to refresh action states');
                }
            });
        }
    }

    /**
     * Update action states from server data.
     * @param {Object} data - Action data from server
     */
    function updateActionStates(data) {
        if (!data || typeof data !== 'object') return;
        
        var categories = Object.keys(data);
        var updatedCount = 0;
        
        categories.forEach(function(categoryName) {
            var actions = data[categoryName];
            if (!Array.isArray(actions)) return;
            
            actions.forEach(function(action) {
                var $btn = $('.action-btn-item[data-action-id="' + action.id + '"]');
                if ($btn.length === 0) return;
                
                var isChecked = action.isChecked === true;
                var isCheckable = action.isCheckable === true;
                
                $btn.toggleClass(options.buttonActiveClass, isCheckable && isChecked);
                $btn.data('ischecked', isChecked);
                $btn.data('ischeckable', isCheckable);
                
                var $icon = $btn.find('.action-state-icon');
                if ($icon.length && isCheckable) {
                    if (isChecked) {
                        $icon.text('✓').removeClass('unchecked').addClass('checked');
                    } else {
                        $icon.text('✗').removeClass('checked').addClass('unchecked');
                    }
                }
                
                if (isCheckable) {
                    $btn.attr('title', action.id + ' (Toggle: ' + (isChecked ? 'ON' : 'OFF') + ')');
                }
                
                if (actionDataCache[action.id]) {
                    actionDataCache[action.id].isChecked = isChecked;
                }
                updatedCount++;
            });
        });
        
        console.log('[ActionsCategorized] Updated ' + updatedCount + ' action states');
        updateActionCount();
    }

    // =====================================================================
    // FILTER / SEARCH
    // =====================================================================

    /**
     * Filter actions by search query.
     * @param {string} query - Search query
     */
    function filterActions(query) {
        var totalVisible = 0;
        
        if (!query || query.length < options.searchMinChars) {
            resetFilter();
            return;
        }
        
        $('.actions-category-panel').show();
        
        $('.action-btn-item').each(function() {
            var $btn = $(this);
            var actionId = ($btn.data('action-id') || '').toLowerCase();
            var actionText = ($btn.data('action-text') || '').toLowerCase();
            var category = ($btn.data('category') || '').toLowerCase();
            
            var matches = actionId.indexOf(query) >= 0 || 
                          actionText.indexOf(query) >= 0 ||
                          category.indexOf(query) >= 0;
            
            if (matches) {
                $btn.removeClass(options.searchHiddenClass);
                $btn.addClass(options.searchHighlightClass);
                totalVisible++;
            } else {
                $btn.addClass(options.searchHiddenClass);
                $btn.removeClass(options.searchHighlightClass);
            }
        });
        
        $('.actions-category-panel').each(function() {
            var $panel = $(this);
            var visibleButtons = $panel.find('.action-btn-item:not(.' + options.searchHiddenClass + ')').length;
            $panel.toggle(visibleButtons > 0);
        });
        
        updateActionCount(totalVisible);
        
        if (typeof options.onSearchChanged === 'function') {
            options.onSearchChanged(query, totalVisible);
        }
    }

    /**
     * Reset filter and show all actions.
     */
    function resetFilter() {
        $('.action-btn-item').removeClass(options.searchHiddenClass + ' ' + options.searchHighlightClass);
        
        $('.actions-category-panel').hide();
        
        var activeTab = $categoryTabs.find('li.ui-tabs-active a');
        if (activeTab.length) {
            $(activeTab.attr('href')).show();
        } else {
            $('#actions-panel-0').show();
        }
        
        updateActionCount();
    }

    /**
     * Update action count display.
     * @param {number} [visibleCount] - Optional visible count (for filtered view)
     */
    function updateActionCount(visibleCount) {
        if (!$countDisplay || !$countDisplay.length) return;
        
        var total = allActions.length;
        var text = '';
        
        if (visibleCount !== undefined && visibleCount < total) {
            text = visibleCount + ' / ' + total + ' visible';
        } else {
            var activeCategory = getActiveCategory();
            if (activeCategory && categorizedActions[activeCategory]) {
                text = categorizedActions[activeCategory].length + ' actions';
            } else {
                text = total + ' total actions';
            }
        }
        
        $countDisplay.text(text);
    }

    /**
     * Get currently active category name.
     * @returns {string|null} Category name or null
     */
    function getActiveCategory() {
        var activeTab = $categoryTabs.find('li.ui-tabs-active a');
        if (activeTab.length) {
            var href = activeTab.attr('href');
            var $panel = $(href);
            if ($panel.length) {
                return $panel.find('.actions-buttons-grid').data('category');
            }
        }
        return null;
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        /**
         * Initialize the module with options.
         * @param {Object} opts - Configuration options
         */
        init: init,
        
        /**
         * Load actions from the server.
         */
        loadActions: loadActions,
        
        /**
         * Execute an action by ID.
         * @param {string} actionId - Action identifier
         * @returns {boolean} Success status
         */
        executeAction: executeAction,
        
        /**
         * Filter actions by search query.
         * @param {string} query - Search query
         */
        filterActions: filterActions,
        
        /**
         * Refresh all action states from server.
         * WARNING: This can cause rebounding issues.
         * Only use if necessary.
         */
        refreshActionStates: refreshActionStates,
        
        /**
         * Get all loaded actions.
         * @returns {Array} Array of all actions
         */
        getActions: function() { return allActions; },
        
        /**
         * Get categorized actions.
         * @returns {Object} Categorized actions object
         */
        getCategorizedActions: function() { return categorizedActions; },
        
        /**
         * Get category order.
         * @returns {Array} Array of category names in order
         */
        getCategoryOrder: function() { return categoryOrder; },
        
        /**
         * Get currently selected action ID.
         * @returns {string|null} Selected action ID
         */
        getSelectedActionId: function() { return selectedActionId; },
        
        /**
         * Check if an action update is pending.
         * @param {string} actionId - Action identifier
         * @returns {boolean} True if pending
         */
        isActionPending: function(actionId) {
            return isActionUpdatePending(actionId);
        }
    };
});