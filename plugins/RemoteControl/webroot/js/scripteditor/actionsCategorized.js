/* ========================================================================
 * actionsCategorized.js - Categorized Action Buttons (Unified v3.0)
 * ========================================================================
 * 
 * This module provides a categorized button grid interface for Stellarium
 * actions, similar to the Sky Culture tab design. It displays actions in
 * categorized tabs with visual state indicators and real-time synchronization.
 * 
 * KEY IMPROVEMENTS (v3.0):
 * - Uses unified button system (unifiedButtons.js)
 * - Uses stelaction class with jQuery UI icons
 * - Uses 'name' attribute instead of 'data-action-id' (standard)
 * - Removed pendingActionUpdates (no longer needed)
 * - Simplified executeAction (just calls actionApi.execute)
 * - State updates from server are now automatic via stelActionChanged
 * - No self-triggered update detection (server is source of truth)
 * - Search filter properly resets when query is cleared or Escape pressed
 * 
 * @module actionsCategorized
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * @requires scripteditor/unifiedButtons
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-09-01
 * @license GPLv2+
 * @version 3.0.1
 * 
 * ======================================================================== */

define(["jquery", "api/remotecontrol", "api/actions", "scripteditor/unifiedButtons"], 
    function($, rc, actionsApi, unifiedButtons) {
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
        doubleClickToExecute: true,
        animateChanges: true,
        maxButtonsPerCategory: 0,
        showTriggerIcons: true,
        
        // Search behavior
        searchMinChars: 1,
        
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

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Initialize the categorized actions module.
     * @param {Object} opts - Configuration options
     */
    function init(opts) {
        options = $.extend(true, {}, defaultOptions, opts || {});
        
        console.log('[ActionsCategorized] Initializing v3.0.1 (unified buttons)...');
        
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
    // REAL-TIME STATE SYNCHRONIZATION
    // =====================================================================

    /**
     * Set up event listener for real-time action state changes from server.
     * The server is the single source of truth - no pending update tracking.
     */
    function setupStateSyncListener() {
        if (typeof actionsApi === 'undefined') {
            console.warn('[ActionsCategorized] actionsApi not available, state sync disabled');
            return;
        }
        
        $(actionsApi).on("stelActionChanged", function(event, actionId, actionData) {
            console.log("[ActionsCategorized] State change from server:", actionId, actionData.isChecked);
            
            // Find and update the button
            var $btn = $('.' + unifiedButtons.CLASSES.ACTION + '[name="' + actionId + '"]');
            if ($btn.length) {
                updateButtonStateFromServer($btn, actionId, actionData);
            }
        });
        
        console.log('[ActionsCategorized] State sync listener established');
    }

    /**
     * Update button UI based on server state change.
     * @param {jQuery} $btn - The button element
     * @param {string} actionId - Action identifier
     * @param {Object} actionData - Action data from server
     */
    function updateButtonStateFromServer($btn, actionId, actionData) {
        var isChecked = actionData.isChecked === true;
        
        // Use unified button system to update state
        unifiedButtons.updateState($btn, isChecked);
        
        // Update cache
        if (actionDataCache[actionId]) {
            actionDataCache[actionId].isChecked = isChecked;
        }
    }

    // =====================================================================
    // LOAD ACTIONS FROM API
    // =====================================================================

    /**
     * Load actions from Stellarium rc server.
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
            html += ' · Click to execute';
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
     * Uses unified 'stelaction' class and 'name' attribute.
     * @param {Object} action - Action object
     * @returns {string} Button HTML
     */
    function buildActionButton(action) {
        var isToggleable = action.isCheckable === true;
        var isChecked = action.isChecked === true;
        var label = action.text || action.id;

        return unifiedButtons.createButton({
            type: 'action',
            name: action.id,
            label: label,
            isChecked: isChecked,
            isCheckable: isToggleable,
            attrs: {
                category: action.category || '',
                'action-text': label
            }
        });
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
                        // Reset filter when switching tabs to ensure clean state
                        resetFilter();
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
            
            // Reset filter when switching tabs
            resetFilter();
            
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
     * Bind UI events for search input and keyboard shortcuts.
     */
    function bindEvents() {
        var searchTimeout = null;
        
        // Search input with debounce for better performance
        $searchInput.off('input.actionsCat').on('input.actionsCat', function() {
            var query = $(this).val().toLowerCase().trim();
            
            clearTimeout(searchTimeout);
            searchTimeout = setTimeout(function() {
                filterActions(query);
            }, 150);
        });
        
        // Keyboard shortcuts
        $searchInput.off('keydown.actionsCat').on('keydown.actionsCat', function(e) {
            // Escape key: Clear search and reset filter
            if (e.key === 'Enter' && selectedActionId) {
                e.preventDefault();
                executeAction(selectedActionId);
            } else if (e.key === 'Escape') {
                e.preventDefault();
                // Clear the input field
                $(this).val('');
                // Reset the filter to show all actions
                resetFilter();
                // Remove focus from search input
                $(this).trigger('blur');
            }
        });
        
        // Clear filter if input becomes empty on blur
        $searchInput.off('blur.actionsCat').on('blur.actionsCat', function() {
            var query = $(this).val().toLowerCase().trim();
            if (!query) {
                resetFilter();
            }
        });
    }

    /**
     * Bind action button events (click, double-click).
     * Uses unified 'stelaction' class selector.
     */
    function bindActionButtonEvents() {
        // Click event: Select and execute the action
        $('.' + unifiedButtons.CLASSES.ACTION).off('click.actionsCatSelect').on('click.actionsCatSelect', function(e) {
            var $btn = $(this);
            var actionId = $btn.attr('name');
            
            // Deselect all other buttons
            $('.' + unifiedButtons.CLASSES.ACTION).removeClass('selected');
            
            // Select this button
            $btn.addClass('selected');
            selectedActionId = actionId;
        });
        
        // Double-click event (backward compatibility)
        if (options.doubleClickToExecute) {
            $('.' + unifiedButtons.CLASSES.ACTION).off('dblclick.actionsCat').on('dblclick.actionsCat', function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                var actionId = $(this).attr('name');
                console.log('[ActionsCategorized] Executing action via double-click:', actionId);
                executeAction(actionId);
            });
        }
    }

    // =====================================================================
    // EXECUTE ACTION
    // =====================================================================

    /**
     * Execute an action by ID.
     * Uses the unified actionsApi.execute method.
     * The server will broadcast the state change via stelActionChanged.
     * @param {string} actionId - Action identifier
     * @returns {boolean} Success status
     */
    function executeAction(actionId) {
        console.log('[ActionsCategorized] Executing action:', actionId);
        
        var $btn = $('.' + unifiedButtons.CLASSES.ACTION + '[name="' + actionId + '"]');
        
        // Visual feedback
        if (options.animateChanges && $btn.length) {
            $btn.css('transform', 'scale(0.95)');
            setTimeout(function() { $btn.css('transform', ''); }, 150);
        }
        
        var success = true;
        
        // Execute via unified API
        if (typeof actionsApi !== 'undefined' && actionsApi.execute) {
            actionsApi.execute(actionId);
        } else {
            // Fallback to direct AJAX
            $.ajax({
                url: options.apiActionEndpoint,
                method: 'POST',
                data: { id: actionId },
                error: function(xhr, status, error) {
                    console.error('[ActionsCategorized] Failed to execute action:', actionId, error);
                    success = false;
                }
            });
        }
        
        if (typeof options.onActionExecuted === 'function') {
            setTimeout(function() {
                options.onActionExecuted(actionId, success);
            }, 300);
        }
        
        return true;
    }

    // =====================================================================
    // FILTER / SEARCH
    // =====================================================================

    /**
     * Filter actions by search query.
     * 
     * This function handles the search/filter functionality for action buttons.
     * It delegates the actual filtering to unifiedButtons.filterButtons().
     * 
     * BEHAVIOR:
     * - If query is empty or less than searchMinChars, calls resetFilter() to restore normal view
     * - If query is valid, applies filter and shows only matching buttons
     * - Updates the action count display with visible count
     * - Triggers onSearchChanged callback with query and visible count
     * 
     * @param {string} query - Search query (case-insensitive, trimmed)
     */
    function filterActions(query) {
        // If query is empty or below minimum characters, reset to normal view
        if (!query || query.length < options.searchMinChars) {
            resetFilter();
            return;
        }
        
        // Apply filter using unified button system
        unifiedButtons.filterButtons($categoryPanels, query, {
            minChars: options.searchMinChars,
            onFilter: function(visibleCount) {
                updateActionCount(visibleCount);
                if (typeof options.onSearchChanged === 'function') {
                    options.onSearchChanged(query, visibleCount);
                }
            }
        });
    }

    /**
     * Reset filter and restore normal view.
     * 
     * This function reverts the UI to its normal state by:
     * 1. Removing all filter-related CSS classes from buttons
     * 2. Hiding all category panels
     * 3. Showing only the currently active tab's panel
     * 4. Updating the action count display
     * 5. Clearing the search input if it contains text
     * 
     * This is called when:
     * - Search query is cleared or below minimum characters
     * - Escape key is pressed in search input
     * - User switches between category tabs
     * - Search input loses focus while empty
     * 
     * @public
     */
    function resetFilter() {
        // Use unifiedButtons to reset the filter if available
        if (typeof unifiedButtons.resetFilter === 'function') {
            unifiedButtons.resetFilter($categoryPanels);
        } else {
            // Fallback: manually remove filter classes
            $('.' + unifiedButtons.CLASSES.ACTION).removeClass(
                'filtered-hidden search-highlight'
            );
        }
        
        // Show only the active tab panel
        $('.actions-category-panel').hide();
        
        var activeTab = $categoryTabs.find('li.ui-tabs-active a');
        if (activeTab.length) {
            $(activeTab.attr('href')).show();
        } else if (categoryOrder.length > 0) {
            $('#actions-panel-0').show();
        }
        
        // Update the action count display
        updateActionCount();
        
        // Clear the search input if it contains text
        if ($searchInput && $searchInput.length && $searchInput.val() !== '') {
            $searchInput.val('');
        }
        
        // Notify that the filter has been reset
        if (typeof options.onSearchChanged === 'function') {
            options.onSearchChanged('', allActions.length);
        }
        
        console.log('[ActionsCategorized] Filter reset, view restored to normal state');
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
         * Reset filter and restore normal view.
         * Can be called externally to clear the search filter programmatically.
         * @public
         */
        resetFilter: resetFilter,
        
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
        getSelectedActionId: function() { return selectedActionId; }
    };
});