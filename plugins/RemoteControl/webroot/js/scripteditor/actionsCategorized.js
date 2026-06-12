/* ========================================================================
 * actionsCategorized.js - Sky Culture Style Action Buttons
 * ========================================================================
 * 
 * This module replaces the old action dropdown with categorized button grids
 * matching the Sky Culture tab design. Provides real-time state synchronization
 * with Stellarium server, search/filter functionality, and visual feedback.
 * 
 * Features:
 * - Categorized tabs matching Stellarium's internal action organization
 * - Button-based UI with visual state indicators (✓ for ON, ✗ for OFF)
 * - Real-time action state synchronization via stelActionChanged events
 * - Search/filter with case-insensitive matching
 * - Double-click execution support
 * - Visual feedback on state change
 * 
 * Technical Implementation:
 * - Uses actionsApi for action state synchronization
 * - Dynamically builds category tabs and buttons from server data
 * - Supports jQuery UI tabs with manual fallback
 * - Real-time state updates without page refresh
 * - Visual feedback animations for state changes
 * 
 * Events:
 * - stelActionChanged: Listens to action state changes from server
 * 
 * @module actionsCategorized
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * 
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @date 2026-06-06
 * @license GPLv2+
 * @version 1.0.0
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
    
    // DOM Elements
    var $searchInput = null;
    var $countDisplay = null;
    var $categoryTabs = null;
    var $categoryPanels = null;

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
        
        // UI Behavior
        doubleClickToExecute: true,
        autoRefreshStates: false,
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
     * Escape HTML special characters
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
     * Escape for HTML attributes
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
     * Initialize the categorized actions module
     * @param {Object} opts - Configuration options
     */
    function init(opts) {
        options = $.extend(true, {}, defaultOptions, opts || {});
        
        console.log('[ActionsCategorized] Initializing...');
        
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
     * Cache DOM element references
     */
    function cacheDomElements() {
        $searchInput = $('#' + options.searchInputId);
        $countDisplay = $('#' + options.countDisplayId);
        $categoryTabs = $('#' + options.categoryTabListId);
        $categoryPanels = $('#' + options.categoryPanelsId);
        
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
     * Listens to stelActionChanged events emitted by api/actions module.
     */
    function setupStateSyncListener() {
        if (typeof actionsApi === 'undefined') {
            console.warn('[ActionsCategorized] actionsApi not available, state sync disabled');
            return;
        }
        
        $(actionsApi).on("stelActionChanged", function(event, actionId, actionData) {
            console.log("[ActionsCategorized] State changed:", actionId, actionData.isChecked);
            
            var $btn = $('.action-btn-item[data-action-id="' + actionId + '"]');
            if ($btn.length) {
                updateButtonStateFromServer($btn, actionId, actionData);
            }
        });
        
        console.log('[ActionsCategorized] State sync listener established');
    }

    /**
     * Update button UI based on server state change
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
     * Load actions from Stellarium server
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
     * Show loading indicator
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
     * Handle load error
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
     * Process raw actions data from API
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
     * Build complete UI from processed actions data
     */
    function buildUI() {
        buildCategoryTabs();
        buildCategoryPanels();
        updateActionCount();
        initTabs();
    }

    /**
     * Build category tabs navigation
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
     * Build category panels with action buttons
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
            html += ' · Click to select, double-click to execute instantly';
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
     * Build HTML for a single action button
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
            titleText += ' (Trigger - Double-click to execute)';
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
     * Initialize jQuery UI tabs or fallback to manual tabs
     */
    function initTabs() {
        var tabsSelector = '#' + options.categoryTabsId;
        
        if ($.fn.tabs && $(tabsSelector).tabs) {
            try {
                $(tabsSelector).tabs({
                    active: 0,
                    create: function() {
                        console.log('[ActionsCategorized] jQuery UI Tabs created');
                    },
                    activate: function(event, ui) {
                        updateActionCount();
                        
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
     * Build manual tabs (fallback when jQuery UI is not available)
     */
    function buildManualTabs() {
        $('.actions-category-panel').hide();
        
        if (categoryOrder.length > 0) {
            $('#actions-panel-0').show();
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
     * Bind UI events
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
        
        if (options.autoRefreshStates && options.refreshInterval > 0) {
            setInterval(refreshActionStates, options.refreshInterval);
        }
    }

    /**
     * Bind action button events (click, double-click)
     */
    function bindActionButtonEvents() {
        $('.action-btn-item').off('click.actionsBtn').on('click.actionsBtn', function(e) {
            e.preventDefault();
            e.stopPropagation();
            
            var $btn = $(this);
            var actionId = $btn.data('action-id');
            var isCheckable = $btn.data('ischeckable');
            
            if (options.animateChanges) {
                $('.action-btn-item.selected').css('transition', 'all 0.15s ease');
            }
            $('.action-btn-item').removeClass(options.buttonSelectedClass);
            
            $btn.addClass(options.buttonSelectedClass);
            selectedActionId = actionId;
            
            if (isCheckable === true || isCheckable === 'true') {
                executeAction(actionId);
            }
        });
        
        if (options.doubleClickToExecute) {
            $('.action-btn-item').off('dblclick.actionsBtn').on('dblclick.actionsBtn', function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                var actionId = $(this).data('action-id');
                executeAction(actionId);
            });
        }
    }

    // =====================================================================
    // EXECUTE ACTION
    // =====================================================================

    /**
     * Execute an action by ID
     * @param {string} actionId - Action identifier
     */
    function executeAction(actionId) {
        console.log('[ActionsCategorized] Executing action:', actionId);
        
        var $btn = $('.action-btn-item[data-action-id="' + actionId + '"]');
        if (options.animateChanges && $btn.length) {
            $btn.css('transform', 'scale(0.95)');
            setTimeout(function() { $btn.css('transform', ''); }, 150);
        }
        
        var success = true;
        
        if (typeof actionsApi !== 'undefined' && actionsApi.doAction) {
            actionsApi.doAction(actionId)
                .then(function() {
                    handleActionSuccess(actionId);
                })
                .fail(function(error) {
                    handleActionError(actionId, error);
                    success = false;
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
                }
            });
        }
        
        if (typeof options.onActionExecuted === 'function') {
            setTimeout(function() {
                options.onActionExecuted(actionId, success);
            }, 300);
        }
    }

    /**
     * Handle successful action execution
     * @param {string} actionId - Action identifier
     */
    function handleActionSuccess(actionId) {
        console.log('[ActionsCategorized] Action executed successfully:', actionId);
        setTimeout(refreshActionStates, 300);
    }

    /**
     * Handle action execution error
     * @param {string} actionId - Action identifier
     * @param {string|Object} error - Error information
     */
    function handleActionError(actionId, error) {
        console.error('[ActionsCategorized] Failed to execute action:', actionId, error);
    }

    // =====================================================================
    // REFRESH ACTION STATES
    // =====================================================================

    /**
     * Refresh all action states from server
     */
    function refreshActionStates() {
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
                error: function() {}
            });
        }
    }

    /**
     * Update action states from server data
     * @param {Object} data - Action data from server
     */
    function updateActionStates(data) {
        if (!data || typeof data !== 'object') return;
        
        var categories = Object.keys(data);
        
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
            });
        });
        
        updateActionCount();
    }

    // =====================================================================
    // FILTER / SEARCH
    // =====================================================================

    /**
     * Filter actions by search query
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
     * Reset filter and show all actions
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
     * Update action count display
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
     * Get currently active category name
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
        init: init,
        loadActions: loadActions,
        executeAction: executeAction,
        filterActions: filterActions,
        refreshActionStates: refreshActionStates,
        getActions: function() { return allActions; },
        getCategorizedActions: function() { return categorizedActions; },
        getCategoryOrder: function() { return categoryOrder; },
        getSelectedActionId: function() { return selectedActionId; }
    };
});