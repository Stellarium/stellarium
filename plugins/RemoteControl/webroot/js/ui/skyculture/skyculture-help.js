/* ========================================================================
 * skyculture-help.js - Control Panels and Help System
 * ========================================================================
 * 
 * This module provides two distinct UI features for each Sky Culture tab:
 * 
 * 1. CONTROL PANEL (top of tab, above the buttons)
 *    - Filter checkboxes (visible above horizon only)
 *    - Name display mode select (default/localized/cultural)
 *    - Sort options (for stars: by name/constellation/magnitude/type)
 *    - Navigation FOV spinner
 *    - Multi-select mode toggle
 *    - Star type filter (stars only)
 *    - Help button
 * 
 * 2. HELP BUTTON (in the tour toolbar)
 *    - Opens a modal dialog explaining tour script generation
 * 
 * DESIGN:
 * -------
 * The Control Panel is created ONCE per tab and persists across
 * re-renders. All IDs follow a canonical, documented contract that is
 * shared with skyculture-multiselect.js and skyculture-tours.js.
 * 
 * ID CONTRACT (canonical reference):
 * ----------------------------------
 * | Type          | filterIdPrefix | Example IDs                              |
 * |---------------|----------------|------------------------------------------|
 * | constellation | constellation  | #constellation-filter-visible            |
 * |               |                | #constellation-name-display              |
 * |               |                | #constellation-nav-fov                   |
 * |               |                | #constellation-multi-select-mode         |
 * |               |                | #constellation-selection-count           |
 * | asterism      | asterisms      | #asterisms-filter-visible                |
 * |               |                | #asterisms-name-display                  |
 * |               |                | #asterisms-nav-fov                       |
 * |               |                | #asterisms-multi-select-mode             |
 * |               |                | #asterisms-selection-count               |
 * | lunar         | lunar          | #lunar-filter-visible                    |
 * |               |                | #lunar-name-display                      |
 * |               |                | #lunar-nav-fov                           |
 * |               |                | #lunar-multi-select-mode                 |
 * |               |                | #lunar-selection-count                   |
 * | star          | stars          | #stars-filter-visible                    |
 * |               |                | #stars-name-display                      |
 * |               |                | #stars-sort-by                           |
 * |               |                | #stars-sort-direction                    |
 * |               |                | #stars-apply-sort                        |
 * |               |                | #stars-star-type                         |
 * |               |                | #stars-nav-fov                           |
 * |               |                | #stars-multi-select-mode                 |
 * |               |                | #stars-selection-count                   |
 * 
 * @module skyculture/help
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 2.0.0 - Canonical ID contract, unified control panel
 * ======================================================================== */

define([
    "jquery",
		"api/remotecontrol",
    "./skyculture-config",
    "./skyculture-state"
], function($, rc, Config, State) {
    "use strict";

    // =====================================================================
    // CONTROL PANEL CONFIGURATION
    // =====================================================================
		var tr = rc.tr;
		
    /**
     * Per-tab control panel configuration.
     * 
     * Fields:
     * - containerId:    ID of the buttons container (panel is inserted before it)
     * - filterVisible:  show the "visible above horizon only" checkbox
     * - nameDisplay:    show the name display mode select
     * - sortOptions:    array of sort options (empty = no sort)
     * - fovSpinner:     show the navigation FOV spinner
     * - multiSelect:    show the multi-select toggle
     * - noIsolation:    show the "no isolation" checkbox (constellations only)
     * - starTypeFilter: show the star-type filter (stars only)
     * - filterIdPrefix: prefix for all control IDs (canonical contract)
     * - fovKey:         key in State.raw.fov
     * - fovRange:       {min, max, step} for the FOV spinner
     * 
     * @constant {Object.<string, Object>}
     */
    var CONTROL_PANEL_CONFIG = {
        constellation: {
            containerId:    'constellations-buttons-container',
            filterVisible:  true,
            nameDisplay:    true,
            sortOptions:    ['name'],
            fovSpinner:     true,
            multiSelect:    true,
            noIsolation:    true,
            starTypeFilter: false,
            filterIdPrefix: 'constellation',
            fovKey:         'constellation',
            fovRange:       Config.FOV_RANGES.constellation
        },
        asterism: {
            containerId:    'asterisms-buttons-container',
            filterVisible:  true,
            nameDisplay:    true,
            sortOptions:    ['name'],
            fovSpinner:     true,
            multiSelect:    true,
            noIsolation:    false,
            starTypeFilter: false,
            filterIdPrefix: 'asterisms',
            fovKey:         'asterism',
            fovRange:       Config.FOV_RANGES.asterism
        },
        zodiac: {
            containerId:    'zodiac-buttons-container',
            filterVisible:  false,
            nameDisplay:    false,
            sortOptions:    [],
            fovSpinner:     false,
            multiSelect:    false,
            noIsolation:    false,
            starTypeFilter: false,
            filterIdPrefix: 'zodiac',
            fovKey:         'zodiac',
            fovRange:       null
        },
        lunar: {
            containerId:    'lunar-buttons-container',
            filterVisible:  true,
            nameDisplay:    true,
            sortOptions:    ['name'],
            fovSpinner:     true,
            multiSelect:    true,
            noIsolation:    false,
            starTypeFilter: false,
            filterIdPrefix: 'lunar',
            fovKey:         'lunar',
            fovRange:       Config.FOV_RANGES.lunar
        },
        star: {
            containerId:    'stars-buttons-container',
            filterVisible:  true,
            nameDisplay:    true,
            sortOptions:    ['name', 'constellation', 'magnitude', 'type'],
            fovSpinner:     true,
            multiSelect:    true,
            noIsolation:    false,
            starTypeFilter: true,
            filterIdPrefix: 'stars',
            fovKey:         'star',
            fovRange:       Config.FOV_RANGES.star
        }
    };

    // =====================================================================
    // HELP MODAL REGISTRY
    // =====================================================================

    /**
     * Registry of help modal IDs and their human-readable titles.
     * The modal content is defined in index.html; this module only
     * initializes them as jQuery UI dialogs on first use.
     * 
     * @constant {Object.<string, string>}
     */
    var HELP_MODALS = {
        'help-constellations':         'Constellations Tour Generator',
        'help-asterisms':              'Asterisms Tour Generator',
        'help-lunar':                  'Lunar Mansions Tour Generator',
        'help-stars':                  'Stars Tour Generator',
        'help-controls-constellation': 'Constellation Controls',
        'help-controls-asterism':      'Asterism Controls',
        'help-controls-lunar':         'Lunar Mansion Controls',
        'help-controls-star':          'Star Controls',
				'help-controls-zodiac':         'Zodiac Help'
    };

    // =====================================================================
    // SORT OPTION LABELS
    // =====================================================================

    /**
     * Human-readable labels for each sort criterion.
     * 
     * The keys MUST match the values used in `CONTROL_PANEL_CONFIG[type].sortOptions`.
     * When adding a new sort criterion:
     *   1. Add its key here with a translation-friendly label.
     *   2. Add it to the `sortOptions` array of the relevant types.
     *   3. Implement its comparator in `skyculture-filters.js`
     *      (`compareByName`, `compareByConstellation`, etc.).
     * 
     * @constant {Object.<string, string>}
     */
    var SORT_OPTION_LABELS = {
        'name':         'Name (Alphabetical)',
        'constellation':'Constellation',
        'magnitude':    'Brightness (Magnitude)',
        'type':         'Object Type'
    };
		
    /**
     * Map filterIdPrefix → help-controls modal key.
     * @constant {Object.<string, string>}
     */
    var HELP_KEY_BY_PREFIX = {
        'constellation': 'constellation',
        'asterisms':     'asterism',
        'lunar':         'lunar',
        'stars':         'star',
        'zodiac':        'zodiac'
    };

    /** @type {boolean} Guard against double-binding the help handler */
    var helpHandlersBound = false;

    // =====================================================================
    // HTML BUILDERS
    // =====================================================================

    /**
     * Build the "Visible above horizon only" checkbox group WITH the
     * visibility refresh mode radio group AND a countdown/item counter.
     * 
     * The checkbox enables filtering by visibility. The radio group
     * controls how often the visibility data is refreshed:
     *   - 60s: balanced
     *   - 180s (default): less server load
     *   - 420s (7min): minimal server load
     *   - manual: no automatic refresh; user clicks "Refresh" button		 
     * Layout:
     *   [✓] Visible above horizon only
     *       Refresh: [60s] [180s] [7min] [Manual] [↻]
     *       ⏱ 45s   •   12/88 items
     *       ↑        ↑
     *       countdown  items visible/total
     * 
     * The countdown shows time remaining until the next automatic refresh.
     * The item counter shows "visible/total" for this tab.
     * 
     * All radio inputs across all tabs share the SAME `name` attribute
     * ("visibility-refresh-mode") so they act as a single logical group.
     * This means changing the setting in any tab affects all tabs.		 
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildFilterVisibleGroup(prefix, tr) {
        var checkboxId = prefix + '-filter-visible';
        
        var html = '<fieldset class="control-group visibility-control-group">';
        html += '<legend>Visibility control</legend>';
				html += '<div class="block_row">';
        // ---- Checkbox: Visible above horizon only ----
        html += '<label class="option-label" for="' + checkboxId + '">' +
                    '<input type="checkbox" id="' + checkboxId + '" ' +
                        'title="' + tr('Show only objects currently above horizon') + '"/>' +
                    '<span>' + tr('Visible above horizon only') + '</span>' +
                '</label>';
        // ================================================================
        // COUNTERS — countdown + items visible/total
        // ================================================================
        // These are updated dynamically:
        //   - Countdown: by updateCountdownDisplay() every 1 second
        //   - Items:     by renderPanel() after each filter application
        // ================================================================
        html += '<span class="visibility-countdown" ' +
                    'data-prefix="' + prefix + '" ' +
                    'title="' + tr('Time until next automatic refresh') + '">' +
                    '⏱ —' +
                '</span>';
        
        html += '<span class="visibility-item-count" ' +
                    'id="' + prefix + '-visibility-count" ' +
                    'title="' + tr('Visible items / total items') + '">' +
                    '—/—' +
                '</span>';
				html += '</div>';
				
        // ---- Radio group: Refresh mode + counters ----
        html += '<div class="visibility-refresh-group" ' +
                    'data-prefix="' + prefix + '">' +
                    '<span class="visibility-refresh-label">' +
                        tr('Refresh visibility:') +
                    '</span>';
        
        // 60s
        html += '<label class="visibility-radio-label" ' +
                    'title="' + tr('Refresh every 60 seconds (balanced)') + '">' +
                    '<input type="radio" name="visibility-refresh-mode" ' +
                        'value="60s" ' +
                        'data-prefix="' + prefix + '"/>' +
                    '<span>60s</span>' +
                '</label>';
        
        // 180s (default)
        html += '<label class="visibility-radio-label" ' +
                    'title="' + tr('Refresh every 180 seconds (less server load)') + '">' +
                    '<input type="radio" name="visibility-refresh-mode" ' +
                        'value="180s" checked ' +
                        'data-prefix="' + prefix + '"/>' +
                    '<span>180s</span>' +
                '</label>';
        
        // 420s (7min)
        html += '<label class="visibility-radio-label" ' +
                    'title="' + tr('Refresh every 7 minutes (minimal server load)') + '">' +
                    '<input type="radio" name="visibility-refresh-mode" ' +
                        'value="420s" ' +
                        'data-prefix="' + prefix + '"/>' +
                    '<span>7min</span>' +
                '</label>';
        
        // Manual
        html += '<label class="visibility-radio-label" ' +
                    'title="' + tr('Refresh manually only — click the Refresh button') + '">' +
                    '<input type="radio" name="visibility-refresh-mode" ' +
                        'value="manual" ' +
                        'data-prefix="' + prefix + '"/>' +
                    '<span>' + tr('Manual') + '</span>' +
                '</label>';
        
        // ---- Manual refresh button ----
        html += '<button type="button" ' +
                    'class="jquerybutton visibility-refresh-btn" ' +
                    'data-prefix="' + prefix + '" ' +
                    'title="' + tr('Refresh visibility data now') + '">' + 
                        '<span class="ui-icon ui-icon-refresh"></span>' +
                        tr('Refresh') +
                '</button>';
        
        html += '</div>';   // close visibility-refresh-group
				
        // ================================================================
        // SNAPSHOT GROUP — Location + Time + JDay
        // ================================================================
        // This group is populated by `updateVisibilitySnapshot(prefix)`.
        // It shows the observer's location and the simulation time at
        // which the current visibility data was captured. It is refreshed
        // on the SAME cadence as the visibility data itself:
        //   - On first activation of the filter
        //   - On every tick of the visibility timer (60s / 180s / 420s)
        //   - On every manual Refresh click
        // 
        // When the filter is off, the group shows "—" placeholders.
        // ================================================================
        html += '<div class="visibility-snapshot-group" data-prefix="' + prefix + '">';
        html += '<span class="visibility-snapshot-item">' +
                    '<span class="snapshot-icon ui-icon ui-icon-pin-s"></span>' +
                    '<span class="snapshot-label">' + tr('Location:') + '</span>' +
                    '<span class="snapshot-value" id="' + prefix + '-vis-location">—</span>' +
                '</span>';
        html += '<span class="visibility-snapshot-item">' +
                    '<span class="snapshot-icon ui-icon ui-icon-calendar"></span>' +
                    '<span class="snapshot-label">' + tr('Time:') + '</span>' +
                    '<span class="snapshot-value" id="' + prefix + '-vis-time">—</span>' +
                '</span>';
        html += '<span class="visibility-snapshot-item">' +
                    '<span class="snapshot-icon ui-icon ui-icon-calendar"></span>' +
                    '<span class="snapshot-label">JDay:</span>' +
                    '<span class="snapshot-value" id="' + prefix + '-vis-jday">—</span>' +
                '</span>';
        html += '</div>';   // close visibility-snapshot-group
				
        html += '</fieldset>';   // close control-group
        
        return html;
    }

    /**
     * Build the "Navigate without isolation" checkbox group.
     * 
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildNoIsolationGroup(tr) {
        return '<div class="control-group">' +
            '<label class="option-label" for="highlight-no-isolation">' +
                '<input type="checkbox" id="highlight-no-isolation" ' +
                    'title="' + tr('Navigate to constellation without isolation') + '"/>' +
                '<span>' + tr('Navigate without Isolation') + '</span>' +
            '</label>' +
        '</div>';
    }

    /**
     * Build the "Name display mode" select group.
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {boolean} includeCultural - Whether to include "Cultural Names"
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildNameDisplayGroup(prefix, includeCultural, tr) {
        var id = prefix + '-name-display';
        var html = '<div class="control-group">' +
            '<label for="' + id + '">' + tr('Show names as:') + '</label>' +
            '<select id="' + id + '" class="stars-sort-select">' +
                '<option value="default">' + tr('Default') + '</option>';

        if (includeCultural) {
            html += '<option value="cultural">' + tr('Cultural Names') + '</option>';
        }

        html += '<option value="localized">' + tr('Translated Names') + '</option>' +
            '</select>' +
        '</div>';
        return html;
    }

    /**
     * Build the sort controls group.
     * 
     * The "Sort by" <select> is populated DYNAMICALLY from `sortOptions`.
     * This allows each tab to expose only the criteria that make sense
     * for its object type — e.g. constellations only need "name", while
     * stars can also sort by constellation / magnitude / type.
     * 
     * Behavior:
     *  - `sortOptions` empty or missing → returns '' (no sort controls).
     *  - `sortOptions.length === 1`    → the "Sort by" <select> is HIDDEN,
     *    because there is only one possible criterion; only the
     *    "Order" and "Apply" controls are shown.
     *  - `sortOptions.length >= 2`     → full controls (Sort by + Order + Apply).
     * 
     * The "Order" <select> is always shown (asc/desc), because even a
     * single-criterion sort benefits from a direction toggle.
     * 
     * @param {string} prefix - filterIdPrefix (used for element IDs)
     * @param {Array<string>} sortOptions - Allowed sort criteria values
     * @param {Object} tr - Translation function
     * @returns {string} HTML fragment
     */
    function buildSortGroups(prefix, sortOptions, tr) {
        if (!sortOptions || !sortOptions.length) return '';

        var html = '';
        var multipleOptions = sortOptions.length > 1;

        // -----------------------------------------------------------------
        // "Sort by" select — only shown when there are 2+ options.
        // When only one option exists, we still render a hidden input
        // carrying the value, so that `installControlPanel` can read it
        // uniformly without special-casing the single-option scenario.
        // -----------------------------------------------------------------
        if (multipleOptions) {
            html += '<div class="control-group">' +
                '<label for="' + prefix + '-sort-by">' + tr('Sort by:') + '</label>' +
                '<select id="' + prefix + '-sort-by" class="stars-sort-select">';

            for (var i = 0; i < sortOptions.length; i++) {
                var value = sortOptions[i];
                var labelKey = SORT_OPTION_LABELS[value] || value;
                html += '<option value="' + value + '">' + tr(labelKey) + '</option>';
            }

            html += '</select>' +
            '</div>';
        } else {
            // Single option → hidden input carrying the value.
            // Kept as a real element (not a data attribute) so that the
            // existing `installControlPanel` code path works unchanged:
            //     $panel.find('#' + prefix + '-sort-by').val()
            html += '<input type="hidden" ' +
                'id="' + prefix + '-sort-by" ' +
                'value="' + sortOptions[0] + '"/>';
        }

        // -----------------------------------------------------------------
        // "Order" select — always shown.
        // -----------------------------------------------------------------
        html += '<div class="control-group">' +
            '<label for="' + prefix + '-sort-direction">' + tr('Order:') + '</label>' +
            '<select id="' + prefix + '-sort-direction" class="stars-sort-select">' +
                '<option value="asc">' + tr('Ascending') + '</option>' +
                '<option value="desc">' + tr('Descending') + '</option>' +
            '</select>' +
        '</div>';

        // -----------------------------------------------------------------
        // "Apply" button + status span — always shown.
        // -----------------------------------------------------------------
        html += '<div class="control-group">' +
            '<button id="' + prefix + '-apply-sort" class="jquerybutton pattern-btn">' +
                tr("Apply") +
            '</button>' +
            '<span id="' + prefix + '-sort-status" class="stars-sort-status"></span>' +
        '</div>';

        return html;
    }

    /**
     * Build the star-type filter group (stars only).
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildStarTypeGroup(prefix, tr) {
        var id = prefix + '-star-type';
        return '<div class="control-group">' +
            '<label for="' + id + '">' + tr('Star Type:') + '</label>' +
            '<select id="' + id + '" class="stars-sort-select">' +
                '<option value="all">' + tr('All') + '</option>' +
                '<option value="star">' + tr('Regular Stars') + '</option>' +
                '<option value="double-star">' + tr('Double Stars') + '</option>' +
                '<option value="variable">' + tr('Variable Stars') + '</option>' +
            '</select>' +
        '</div>';
    }

    /**
     * Build the FOV spinner group.
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {Object} cfg - Control panel config
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildFovGroup(prefix, cfg, tr) {
        if (!cfg.fovSpinner || !cfg.fovRange) return '';

        var id = prefix + '-nav-fov';
        var currentValue = State.raw.fov[cfg.fovKey] || cfg.fovRange.min;

        return '<div class="control-group">' +
            '<label for="' + id + '">' + tr('Navigation FOV:') + '</label>' +
            '<input id="' + id + '" class="spinner fov-input-number" ' +
                'value="' + currentValue + '" ' +
                'data-min="' + cfg.fovRange.min + '" ' +
                'data-max="' + cfg.fovRange.max + '" ' +
                'data-step="' + cfg.fovRange.step + '"/>' +
            '<span>' + tr('deg') + '</span>' +
        '</div>';
    }

    /**
     * Build the multi-select toggle group.
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildMultiSelectGroup(prefix, tr) {
        var checkboxId = prefix + '-multi-select-mode';
        var counterId  = prefix + '-selection-count';

        return '<div class="control-group tour-multiselect-group">' +
            '<label class="option-label multi-select-label" for="' + checkboxId + '">' +
                '<input type="checkbox" id="' + checkboxId + '" ' +
                    'title="' + tr("Enable to select custom array for tour generation") + '"/>' +
                '<span style="font-weight:bold;cursor:pointer;">' + tr("Multi-Select") + '</span>' +
                '<small>(' + tr("Pick multiple") + ')</small>' +
            '</label>' +
            '<span class="tour-selection-count" id="' + counterId + '">' +
                '0 ' + tr('selected') +
            '</span>' +
        '</div>';
    }

    /**
     * Build the help button group.
     * 
     * @param {string} prefix - filterIdPrefix
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildHelpButtonGroup(prefix, tr) {
        var helpKey = HELP_KEY_BY_PREFIX[prefix] || 'constellation';
        return '<div class="control-group control-help-group">' +
            '<button type="button" ' +
                'class="pattern-btn jquerybutton tour-help-btn" ' +
                'data-help-target="help-controls-' + helpKey + '" ' +
                'title="' + tr('How to use the controls') + '">' +
                '<span class="ui-icon ui-icon-help"></span> ' +
                tr('Controls Help') +
            '</button>' +
        '</div>';
    }

    /**
     * Build the complete HTML for a control panel.
     * 
     * @param {Object} cfg - Config from CONTROL_PANEL_CONFIG
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function buildControlPanelHtml(cfg, tr) {
        var prefix = cfg.filterIdPrefix;
        var html = '<div class="skyculture-controls">';

        if (cfg.filterVisible) {
            html += buildFilterVisibleGroup(prefix, tr);
        }
				html += '<fieldset class="control-group visibility-control-group">';
				html += '<legend>Order and filter</legend>';				
        if (cfg.starTypeFilter) {
            html += buildStarTypeGroup(prefix, tr);
        }
        if (cfg.nameDisplay) {
            html += buildNameDisplayGroup(prefix, cfg.starTypeFilter, tr);
        }
        if (cfg.sortOptions && cfg.sortOptions.length > 0) {
            // ✅ Pass sortOptions through so only the allowed criteria
            //    are rendered.
            html += buildSortGroups(prefix, cfg.sortOptions, tr);
        }
				html += '</fieldset>';
				
				html += '<fieldset class="control-group visibility-control-group">';
				html += '<legend>Navigation Control</legend>';
        if (cfg.fovSpinner && cfg.fovRange) {
            html += buildFovGroup(prefix, cfg, tr);
        }
        if (cfg.noIsolation) {
            html += buildNoIsolationGroup(tr);
        }			
        if (cfg.multiSelect) {
            html += buildMultiSelectGroup(prefix, tr);
        }
				html += '</fieldset>';				
        html += buildHelpButtonGroup(prefix, tr);

        html += '</div>';
        return html;
    }

    // =====================================================================
    // SPINNER INITIALIZATION
    // =====================================================================

    /**
     * Initialize the jQuery UI spinner for the FOV input, attaching a
     * unified handler that fires for all input methods (arrow buttons,
     * keyboard, direct typing, and blur).
     * 
     * @param {jQuery} $panel - The control panel container
     * @param {Object} cfg - Control panel config
     * @param {Object} handlers - Event handlers
     * @param {string} type - Type key
     */
    function initFovSpinner($panel, cfg, handlers, type) {
        if (!cfg.fovSpinner || !cfg.fovRange) return;

        var prefix = cfg.filterIdPrefix;
        var $spin = $panel.find('#' + prefix + '-nav-fov');
        if (!$spin.length) return;

        // Initialize the spinner without callbacks
        $spin.spinner({
            min:  cfg.fovRange.min,
            max:  cfg.fovRange.max,
            step: cfg.fovRange.step
        });

        // Unified handler for any value change
        var handleChange = function() {
            var val = $spin.spinner('value');
            if (typeof val === 'number' && !isNaN(val)) {
                if (handlers && handlers.onFovChange) {
                    handlers.onFovChange(type, val);
                }
            }
        };

        // jQuery UI events
        $spin.on('spin',        handleChange);
        $spin.on('spinchange',  handleChange);
        $spin.on('change',      handleChange);

        // Native events for direct typing
        $spin.on('input',       handleChange);
        $spin.on('blur',        handleChange);
    }

    // =====================================================================
    // CONTROL PANEL INSTALLATION
    // =====================================================================

    /**
     * Install a control panel for a given tab.
     * 
     * @param {string} type - Type key ('constellation', 'asterism', 'lunar', 'star', 'zodiac')
     * @param {Object} tr - Translation function
     * @param {Object} handlers - Event handlers:
     *   - onFilterChange(type, value)
     *   - onNameModeChange(type, value)
     *   - onSortChange(type, sortBy, sortDir)
     *   - onApplySort(type)
     *   - onFovChange(type, value)
     *   - onMultiSelectChange(type, value)
     *   - onStarTypeChange(type, value)
     */
    function installControlPanel(type, tr, handlers) {
        var cfg = CONTROL_PANEL_CONFIG[type];
        if (!cfg) {
            console.warn('[SkyCulture:Help] No control panel config for type:', type);
            return;
        }

        var $buttonsContainer = $('#' + cfg.containerId);
        if (!$buttonsContainer.length) {
            console.warn('[SkyCulture:Help] Container not found:', cfg.containerId);
            return;
        }

        var prefix = cfg.filterIdPrefix;

        // Remove any existing panel
        $buttonsContainer.prev('.skyculture-controls').remove();

        // Build and insert panel BEFORE the buttons container
        var $panel = $(buildControlPanelHtml(cfg, tr));
        $buttonsContainer.before($panel);

        // Initialize spinner
        initFovSpinner($panel, cfg, handlers, type);

        // -----------------------------------------------------------------
        // Bind event handlers
        // -----------------------------------------------------------------

        // Filter visible
        $panel.find('#' + prefix + '-filter-visible').on('change', function() {
            if (handlers && handlers.onFilterChange) {
                handlers.onFilterChange(type, $(this).is(':checked'));
            }
        });

        // Name display mode
        $panel.find('#' + prefix + '-name-display').on('change', function() {
            if (handlers && handlers.onNameModeChange) {
                handlers.onNameModeChange(type, $(this).val());
            }
        });

        // Sort controls
        if (cfg.sortOptions && cfg.sortOptions.length > 0) {
            $panel.find('#' + prefix + '-apply-sort').on('click', function() {
                if (!handlers) return;
                var sortBy  = $panel.find('#' + prefix + '-sort-by').val();
                var sortDir = $panel.find('#' + prefix + '-sort-direction').val();
                if (handlers.onSortChange) {
                    handlers.onSortChange(type, sortBy, sortDir);
                }
                if (handlers.onApplySort) {
                    handlers.onApplySort(type);
                }
            });
        }

        // Star type filter
        if (cfg.starTypeFilter) {
            $panel.find('#' + prefix + '-star-type').on('change', function() {
                if (handlers && handlers.onStarTypeChange) {
                    handlers.onStarTypeChange(type, $(this).val());
                }
            });
        }

        // Multi-select toggle
        if (cfg.multiSelect) {
            $panel.find('#' + prefix + '-multi-select-mode').on('change', function() {
                if (handlers && handlers.onMultiSelectChange) {
                    handlers.onMultiSelectChange(type, $(this).is(':checked'));
                }
            });
        }

				// ============================================================
				// INITIALIZE JQUERY UI CHECKBOX-RADIO ON VISIBILITY RADIOS
				// ============================================================
				// jQuery UI's checkboxradio widget replaces the native radio
				// with a themeable button. Crucially, it provides a `refresh()`
				// method that updates the visual state — even for radios in
				// hidden tabs. This solves the "shared radio group visual sync"
				// problem that the browser does not handle natively.
				// ============================================================
				$panel.find('input[type="radio"][name="visibility-refresh-mode"]').each(function() {
						var $radio = $(this);
						if (!$radio.checkboxradio('instance')) {
								$radio.checkboxradio({
										icon: false   // We provide text labels (60s, 180s, 7min, Manual)
								});
						}
				});
				
        // ============================================================
        // VISIBILITY REFRESH MODE RADIO GROUP
        // ============================================================
        // All radio inputs across all tabs share the same `name` attribute
        // ("visibility-refresh-mode"). This means:
        //  - Only ONE radio is selected globally at any time.
        //  - Changing it in one tab updates the selected value for all tabs.
        //  - We only need to bind the change handler ONCE (any instance
        //    will fire for all).
        // 
        // We use a delegated handler on document to be safe.
        // ============================================================				
				$(document).off('change.skycultureVisRadio').on(
						'change.skycultureVisRadio',
						'input[name="visibility-refresh-mode"]',
						function() {
								var selectedValue = $(this).val();
								
								// ============================================================
								// SYNC ALL RADIO GROUPS
								// ============================================================
								// The radios share the same `name` attribute (so the browser
								// treats them as one logical group), but the visual state of
								// radios in hidden tabs is NOT updated by the browser.
								// 
								// We therefore manually set `checked` AND call the jQuery UI
								// `checkboxradio('refresh')` method to update the visual state
								// of each radio — even those in closed tabs.
								// ============================================================
								$('input[name="visibility-refresh-mode"]').each(function() {
										var $radio = $(this);
										var shouldBeChecked = ($radio.val() === selectedValue);
										
										if ($radio.prop('checked') !== shouldBeChecked) {
												$radio.prop('checked', shouldBeChecked);
												
												// Refresh the checkboxradio visual state
												if ($radio.checkboxradio('instance')) {
														$radio.checkboxradio('refresh');
												}
										}
								});
								
								// Show/hide the manual refresh button
								updateVisibilityRefreshButtons();
								
								console.log('[SkyCulture] Visibility refresh mode changed to:', selectedValue);
								
								// ============================================================
								// NOTIFY THE CALLER — ALWAYS
								// ============================================================
								// We call this handler for EVERY value, including
								// "manual". The handler is responsible for
								// reconciling the timer (stopping it in manual
								// mode, starting/restarting it otherwise, and
								// considering whether any type has the
								// visibility filter enabled).
								// ============================================================
								if (handlers.onVisibilityModeChange) {
												handlers.onVisibilityModeChange(selectedValue);
								}
						}
				);				

        // ============================================================
        // MANUAL REFRESH BUTTON
        // ============================================================
        $(document).off('click.skycultureVisRefresh').on(
            'click.skycultureVisRefresh',
            '.visibility-refresh-btn',
            function(e) {
                e.preventDefault();
                e.stopPropagation();
                
                var prefix = $(this).data('prefix');
                console.log('[SkyCulture] Manual visibility refresh triggered for:', prefix);
                
                // Notify the caller with the type derived from the prefix
                if (handlers.onManualRefresh) {
                    handlers.onManualRefresh(prefix);
                }
            }
        );

    }
    
    /**
     * Update the visibility of manual refresh buttons based on the
     * currently selected radio mode.
     * 
     * The Refresh button is visible ONLY when "manual" mode is active.
     */
    function updateVisibilityRefreshButtons() {
        var selectedValue = $('input[name="visibility-refresh-mode"]:checked').val();
        var isManual = (selectedValue === 'manual');
        
        $('.visibility-refresh-btn').toggle(isManual);
    }
    
    /**
     * Initialize the visibility refresh buttons state on first load.
     */
    function initVisibilityRefreshButtons() {
        updateVisibilityRefreshButtons();
    }

    // =====================================================================
    // VISIBILITY SNAPSHOT (Location + Time + JDay)
    // =====================================================================

		/**
		 * Update the snapshot display (location, time, JDay) for a given tab.
		 * 
		 * The snapshot shows the observer location, UTC time, and Julian Day
		 * at which the current visibility data was captured. It is refreshed
		 * on the SAME cadence as the visibility data:
		 *   - On first activation of the filter
		 *   - On every tick of the visibility timer (60s / 180s / 420s)
		 *   - On every manual Refresh click
		 * 
		 * This function is normally called from skyculture.js
		 * (refreshAllVisibilitySnapshots), not directly.
		 * 
		 * @param {string} prefix - filterIdPrefix (e.g. 'constellation')
		 * @param {Object|null} status - Result of Api.fetchStatus()
		 * @see clearVisibilitySnapshot - for the disable case
		 */
    function updateVisibilitySnapshot(prefix, status) {
        var $location = $('#' + prefix + '-vis-location');
        var $time     = $('#' + prefix + '-vis-time');
        var $jday     = $('#' + prefix + '-vis-jday');
        
        if (!$location.length || !$time.length || !$jday.length) {
            return;   // This tab doesn't have a snapshot group
        }
        
        if (!status || !status.location || !status.time) {
            // Fetch failed or no data → show placeholders
            $location.text('—');
            $time.text('—');
            $jday.text('—');
            return;
        }
        
        var loc = status.location;
        var t   = status.time;
        
        // ---- Location ----
        var locName = loc.name || 'Unknown';
        var lat = Number(loc.latitude).toFixed(4);
        var lon = Number(loc.longitude).toFixed(4);
        var planet = (loc.planet && loc.planet !== 'Earth') ? ' (' + loc.planet + ')' : '';
        $location.text(locName + planet + ' (' + lat + '°, ' + lon + '°)');
        
        // ---- Time (UTC) ----
        var utc = t.utc || '';
        // Format as "2026-10-20 01:57:38 UTC"
        var timeText = utc.replace('T', ' ').replace('Z', ' UTC');
        $time.text(timeText);
        
        // ---- JDay ----
        var jday = Number(t.jday).toFixed(6);
        $jday.text(jday);
        
        console.log('[SkyCulture] Visibility snapshot updated for', prefix, {
            location: locName,
            lat: lat, lon: lon,
            jday: jday
        });
    }

		/**
		 * Clear the snapshot display (show "—" placeholders).
		 * 
		 * Called when the visibility filter is turned OFF. The three
		 * counters (location, time, JDay) are reset to "—".
		 * 
		 * @param {string} prefix - filterIdPrefix
		 * @see updateVisibilitySnapshot - for the enable case
		 */
    function clearVisibilitySnapshot(prefix) {
        $('#' + prefix + '-vis-location').text('—');
        $('#' + prefix + '-vis-time').text('—');
        $('#' + prefix + '-vis-jday').text('—');
    }
		
    // =====================================================================
    // HELP MODAL SYSTEM
    // =====================================================================

    /**
     * Initialize all help modal dialogs. Called once at startup.
     * 
     * @param {Object} tr - Translation function
     */
    function initHelpModals(tr) {
        Object.keys(HELP_MODALS).forEach(function(modalId) {
            var $modal = $('#' + modalId);
            if (!$modal.length) return;

            // Wrap content if not already wrapped
            if (!$modal.find('.tour-help-content').length) {
                $modal.wrapInner('<div class="tour-help-content"></div>');
            }

            $modal.dialog({
                modal:       true,
                width:       Math.min(600, $(window).width() - 40),
                maxHeight:   Math.min(600, $(window).height() - 80),
                resizable:   true,
                draggable:   true,
                autoOpen:    false,
                closeText:   tr('Close'),
                dialogClass: 'tour-help-dialog',
                buttons: [{
                    text:  tr('Got it'),
                    class: 'jquerybutton',
                    click: function() { $(this).dialog('close'); }
                }]
            });
        });
    }

    /**
     * Bind the global click handler for help buttons. Idempotent.
     * 
     * @param {Object} tr - Translation function
     */
    function bindHelpButtons(tr) {
        if (helpHandlersBound) return;
        helpHandlersBound = true;

        $(document).off('click.skycultureHelp').on(
            'click.skycultureHelp',
            '.tour-help-btn',
            function(e) {
                e.preventDefault();
                e.stopPropagation();

                var targetId = $(this).data('help-target');
                if (!targetId) return;

                var $modal = $('#' + targetId);
                if (!$modal.length) {
                    console.warn('[SkyCulture:Help] Modal not found:', targetId);
                    return;
                }

                // Initialize on first use
                if (!$modal.hasClass('ui-dialog-content')) {
                    $modal.dialog({
                        modal:       true,
                        width:       Math.min(600, $(window).width() - 40),
                        maxHeight:   Math.min(600, $(window).height() - 80),
                        resizable:   true,
                        draggable:   true,
                        autoOpen:    false,
                        closeText:   tr('Close'),
                        dialogClass: 'tour-help-dialog',
                        buttons: [{
                            text:  tr('Got it'),
                            class: 'jquerybutton',
                            click: function() { $(this).dialog('close'); }
                        }]
                    });
                }

                $modal.dialog('open');
            }
        );
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        installControlPanel: installControlPanel,
        initHelpModals:      initHelpModals,
        bindHelpButtons:     bindHelpButtons,
        updateVisibilityRefreshButtons: updateVisibilityRefreshButtons,
        initVisibilityRefreshButtons:   initVisibilityRefreshButtons,
        updateVisibilitySnapshot:       updateVisibilitySnapshot,
        clearVisibilitySnapshot:        clearVisibilitySnapshot,				
        CONTROL_PANEL_CONFIG:CONTROL_PANEL_CONFIG,
        HELP_MODALS:         HELP_MODALS
    };
});