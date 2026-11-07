/* ========================================================================
 * skyculture-render.js - Panel and Button Rendering
 * ========================================================================
 * 
 * This module handles all DOM rendering for the Sky Culture subsystem:
 * 
 * - Culture selection buttons
 * - Pattern buttons (constellations, asterisms, zodiac, lunar, stars)
 * - Grouped star rendering
 * - Artwork grid rendering
 * - Counter badges
 * 
 * DESIGN PRINCIPLES:
 * ------------------
 * 1. Render functions are pure: given data + state, they produce DOM.
 * 2. Click handlers are attached in ONE place per mode (normal/multi-select).
 * 3. All rendering goes through a small set of shared helpers.
 * 4. The `toJQuery` helper normalizes any input to a jQuery object, so
 *    callers can pass selectors, jQuery objects, or null without breaking.
 * 5. The `renderPatternGrid` helper handles 90% of cases; specialized
 *    renderers (grouped stars, artwork) exist for the remaining 10%.
 * 
 * @module skyculture/render
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.1 - Added toJQuery normalization for defensive rendering
 * ======================================================================== */

define(["jquery", "./skyculture-config", "./skyculture-state"],
    function($, Config, State) {
    "use strict";

    var OBJECT_TYPES = Config.OBJECT_TYPES;

    // =====================================================================
    // JQUERY NORMALIZATION
    // =====================================================================

    /**
     * Normalize a value to a jQuery object.
     * 
     * Accepts:
     * - jQuery object → returned as-is
     * - String (CSS selector) → converted to jQuery object
     * - null / undefined / false / 0 → empty jQuery object
     * - Anything else → empty jQuery object (with warning in dev)
     * 
     * This is used defensively by every render function so that even if a
     * caller accidentally passes a string selector, the function still works.
     * 
     * @param {jQuery|string|null|undefined} val - Value to normalize
     * @returns {jQuery} jQuery object (possibly empty)
     */
    function toJQuery(val) {
        if (!val) return $();
        if (val instanceof $) return val;
        if (typeof val === 'string') return $(val);
        return $();
    }

    // =====================================================================
    // HTML ESCAPING
    // =====================================================================

    /**
     * Escape HTML special characters to prevent XSS.
     * 
     * @param {*} str - Value to escape
     * @returns {string} Safe HTML string
     */
    function escapeHtml(str) {
        if (str === null || str === undefined) return '';
        return String(str)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    // =====================================================================
    // PLACEHOLDER TEXT
    // =====================================================================

    /**
     * Get the empty-state placeholder text for a container type.
     * 
     * @param {string} containerName - Container key
     * @param {Object} tr - Translation function
     * @returns {string}
     */
    function getPlaceholder(containerName, tr) {
        var map = {
            constellations: 'No constellations available for this culture',
            asterisms:      'No asterisms available for this culture',
            zodiac:         'No zodiac data available for this culture',
            lunar:          'No lunar mansions data available for this culture',
            stars:          'No star name data available for this culture',
            artwork:        'No artwork available for this culture'
        };
        var key = map[containerName] || 'No data available';
        return tr ? tr(key) : key;
    }

    /**
     * Render a loading or empty placeholder into a container.
     * 
     * @param {jQuery|string} $container - Target container (jQuery or selector)
     * @param {string} text - Placeholder text
     */
    function renderPlaceholder($container, text) {
        $container = toJQuery($container);
        if (!$container.length) return;
        $container.html('<div class="loading-placeholder">' + escapeHtml(text) + '</div>');
    }

    // =====================================================================
    // COUNTER BADGES
    // =====================================================================

    /**
     * Update the pattern count badge in the tab header.
     * 
     * @param {string} type - Pattern type key (e.g., "constellations")
     * @param {number|string} count - Count or "visible/total" string
     */
    function updatePatternCount(type, count) {
        var $el = $('#' + type + '-count');
        if (!$el || !$el.length) return;

        var displayText = '';
        if (count === 0 || count === '0') {
            displayText = '';
        } else if (typeof count === 'string' && count.indexOf('/') !== -1) {
            displayText = '(' + count + ')';
        } else if (count > 0) {
            displayText = '(' + count + ')';
        }
        $el.text(displayText);
    }

		/**
		 * Update the visibility item counter for a specific type.
		 * 
		 * This counter appears inside the control panel (next to the
		 * "Visible above horizon only" checkbox), and shows the format
		 * "visible/total" when the filter is active, or "—/—" otherwise.
		 * 
		 * It is a separate element from the tab-header counter
		 * (`updatePatternCount`) — the header shows the same numbers, but
		 * the control-panel counter is positioned near the filter controls
		 * for quick reference.
		 * 
		 * @param {string} prefix - Filter ID prefix (e.g. 'constellation')
		 * @param {number} visibleCount - Number of items passing the filter
		 * @param {number} totalCount - Total number of items
		 * @param {boolean} isFiltered - If false, shows "—/—" (no filter active)
		 * 
		 * @see updatePatternCount - for the tab-header counter
		 */
    function updateVisibilityItemCount(prefix, visibleCount, totalCount, isFiltered) {
        var $el = $('#' + prefix + '-visibility-count');
        if (!$el.length) return;
        
        if (!isFiltered) {
            $el.text('—/—');
            return;
        }
        
        $el.text(visibleCount + '/' + totalCount);
    }

    // =====================================================================
    // CULTURE BUTTONS
    // =====================================================================

		/**
		 * Render the sky culture selection buttons.
		 * 
		 * The active state is determined by comparing each culture ID to the
		 * current activeId. The click handler does NOT compare IDs itself;
		 * it simply delegates to the onClick callback. The callback (setCulture)
		 * is responsible for ignoring clicks on the already-active culture.
		 * 
		 * This design avoids a subtle bug where the captured `activeId`
		 * snapshot would become stale after the user switches cultures,
		 * preventing them from returning to the initially loaded culture.
		 * 
		 * @param {Array<{id: string, name: string}>} cultures - Available cultures
		 * @param {string} activeId - Currently active culture ID (for visual state only)
		 * @param {Function} onClick - Click handler(cultureId)
		 * @param {Object} tr - Translation function
		 */
		function renderCultureButtons(cultures, activeId, onClick, tr) {
				var $container = toJQuery(State.raw.ui.containers.cultures);
				if (!$container.length) return;

				$container.empty();

				if (!cultures || !cultures.length) {
						renderPlaceholder($container, tr("No sky cultures available"));
						return;
				}

				// Update count badge
				var $count = toJQuery(State.raw.ui.cultureCount);
				if ($count.length) {
						$count.text('(' + cultures.length + ')');
				}

				var html = '<div class="skyculture-buttons-grid">';
				for (var i = 0; i < cultures.length; i++) {
						var c = cultures[i];
						var active = (c.id === activeId);
						html += '<button type="button" class="skyculture-btn' +
								(active ? ' active' : '') +
								'" data-culture-id="' + escapeHtml(c.id) + '">' +
								escapeHtml(c.name) + '</button>';
				}
				html += '</div>';

				$container.html(html);

				// -----------------------------------------------------------------
				// Attach click handlers
				// 
				// The handler simply forwards the click to onClick. The onClick
				// callback (setCulture) is responsible for ignoring clicks on the
				// currently active culture. This avoids relying on a stale
				// `activeId` snapshot captured at render time.
				// -----------------------------------------------------------------
				$container.find('.skyculture-btn').on('click', function() {
						var id = $(this).data('culture-id');
						if (id && onClick) {
								onClick(id);
						}
				});
		}

    /**
     * Update active state of culture buttons.
     * 
     * @param {string} activeId - Current culture ID
     */
    function updateCultureButtonStates(activeId) {
        var $container = toJQuery(State.raw.ui.containers.cultures);
        if (!$container.length) return;

        $container.find('.skyculture-btn').removeClass('active');
        $container.find('.skyculture-btn').each(function() {
            if ($(this).data('culture-id') === activeId) {
                $(this).addClass('active');
            }
        });
    }

    // =====================================================================
    // PATTERN GRID (SHARED HELPER)
    // =====================================================================

    /**
     * Render a grid of pattern buttons.
     * 
     * This is the primary rendering helper used by all pattern panels.
     * It handles:
     * - Empty state
     * - Multi-select styling
     * - Active (single-selection) styling
     * - Tooltips for stars
     * 
     * @param {jQuery|string} $container - Target container
     * @param {Array} patterns - Pattern objects
     * @param {string} type - Object type key for multi-select
     * @param {Object} options - {tr, onClick, disableHandler}
     */
    function renderPatternGrid($container, patterns, type, options) {
        options = options || {};
        var tr = options.tr;
        var onClick = options.onClick;
        var disableHandler = options.disableHandler;

        $container = toJQuery($container);
        if (!$container.length) return;
        $container.empty();

        if (!patterns || !patterns.length) {
            renderPlaceholder($container, getPlaceholder(type, tr));
            return;
        }

        var multiSelectActive = State.raw.multiSelect.active &&
                                 State.raw.multiSelect.mode === type;
        var selectedSet = State.raw.multiSelect.selected[type] || new Set();
        var selectedId = State.raw.selection.id;

        // Build HTML
        var html = '<div class="patterns-buttons-grid">';
        for (var i = 0; i < patterns.length; i++) {
            var p = patterns[i];
            var text = p._displayName || p.name || p.id;
            var isActive = (selectedId === p.id);
            var isSelected = multiSelectActive && selectedSet.has(p.id);

            var classNames = 'pattern-btn';
            var style = '';

            if (isActive && !multiSelectActive) {
                classNames += ' active';
            }
            if (isSelected) {
                classNames += ' selected-' + type;
                style = ' style="background:linear-gradient(#DCDBDA, #8A8C8E);' +
                        'border-color:#000000;color:#000000;"';
            }

            var tooltip = (p.type === OBJECT_TYPES.STAR) ?
                ' title="' + escapeHtml(p.id) + '"' : '';

            html += '<button type="button" class="' + classNames + '"' +
                ' data-pattern-id="' + escapeHtml(p.id) + '"' +
                ' data-pattern-name="' + escapeHtml(p.name || '') + '"' +
                ' data-pattern-type="' + escapeHtml(p.type || type) + '"' +
                ' data-search-name="' + escapeHtml(p.searchName || p.name || '') + '"' +
                ' data-display-name="' + escapeHtml(text) + '"' +
                style + tooltip + '>' +
                escapeHtml(text) + '</button>';
        }
        html += '</div>';

        $container.html(html);

        // Attach click handlers
        if (disableHandler) return;

        $container.find('.pattern-btn').off('click.pattern').on('click.pattern', function() {
            var $btn = $(this);
            var patternId   = $btn.data('pattern-id');
            var patternName = $btn.data('pattern-name');
            var searchName  = $btn.data('search-name');

            if (onClick) {
                onClick({
                    id:         patternId,
                    name:       patternName,
                    searchName: searchName,
                    type:       type,
                    $button:    $btn,
                    isActive:   $btn.hasClass('active')
                });
            }
        });
    }

    // =====================================================================
    // GROUPED STARS
    // =====================================================================

    /**
     * Render stars grouped by constellation or object type.
     * 
     * @param {jQuery|string} $container - Target container
     * @param {Object} groupedData - {groupName: {name, items[]}}
     * @param {Object} options - {tr, onClick}
     */
    function renderGroupedStars($container, groupedData, options) {
        options = options || {};
        var tr = options.tr;
        var onClick = options.onClick;

        $container = toJQuery($container);
        if (!$container.length) return;
        $container.empty();

        if (!groupedData || !Object.keys(groupedData).length) {
            renderPlaceholder($container, tr('No star names available for this culture'));
            return;
        }

        var multiSelectActive = State.raw.multiSelect.active &&
                                 State.raw.multiSelect.mode === 'star';
        var selectedSet = State.raw.multiSelect.selected.star || new Set();
        var selectedId  = State.raw.selection.id;

        var html = '';
        var groupKeys = Object.keys(groupedData);

        for (var g = 0; g < groupKeys.length; g++) {
            var groupName = groupKeys[g];
            var group     = groupedData[groupName];
            var items     = group.items || [];

            if (items.length === 0) continue;

            // Group header
            html += '<div class="star-group-header" data-group="' + escapeHtml(groupName) + '">';
            html += '<span class="star-group-name">' + escapeHtml(groupName) + '</span>';
            html += '<span class="pattern-count">(' + items.length + ' ' +
                    escapeHtml(tr('objects')) + ')</span>';

            if (multiSelectActive) {
                html += '<button class="star-group-select-all jquerybutton" ' +
                        'data-group="' + escapeHtml(groupName) + '" ' +
                        'style="margin-left:10px;padding:1px 10px;font-size:10px;' +
                        'background:linear-gradient(#5D5F62, #3A3C3E);border:1px solid #2A2C2E;' +
                        'border-radius:3px;color:#000;cursor:pointer;">' +
                        escapeHtml(tr('Select All')) + '</button>';
            }
            html += '</div>';

            // Buttons for this group
            html += '<div class="star-group-buttons" data-group="' + escapeHtml(groupName) + '">';
            for (var i = 0; i < items.length; i++) {
                var p = items[i];
                var text = p._displayName || p.name || p.id;
                var isActive   = (selectedId === p.id);
                var isSelected = multiSelectActive && selectedSet.has(p.id);

                var classNames = 'pattern-btn';
                var style = '';

                if (isActive && !multiSelectActive) classNames += ' active';
                if (isSelected) {
                    classNames += ' selected-star';
                    style = ' style="background:linear-gradient(#DCDBDA, #8A8C8E);' +
                            'border-color:#000000;color:#000000;"';
                }

                var tooltip = p.id ? ' title="' + escapeHtml(p.id) + '"' : '';

                html += '<button type="button" class="' + classNames + '"' +
                    ' data-pattern-id="' + escapeHtml(p.id) + '"' +
                    ' data-pattern-name="' + escapeHtml(p.name || '') + '"' +
                    ' data-pattern-type="star"' +
                    ' data-search-name="' + escapeHtml(p.searchName || p.name || '') + '"' +
                    ' data-display-name="' + escapeHtml(text) + '"' +
                    ' data-group="' + escapeHtml(groupName) + '"' +
                    style + tooltip + '>' +
                    escapeHtml(text) + '</button>';
            }
            html += '</div>';
        }

        $container.html(html);

        // Attach handlers if not multi-select (multi-select handlers attached separately)
        if (onClick && !multiSelectActive) {
            $container.find('.pattern-btn').off('click.pattern').on('click.pattern', function() {
                var $btn = $(this);
                onClick({
                    id:         $btn.data('pattern-id'),
                    name:       $btn.data('pattern-name'),
                    searchName: $btn.data('search-name'),
                    type:       'star',
                    $button:    $btn,
                    isActive:   $btn.hasClass('active')
                });
            });
        }
    }

    // =====================================================================
    // ARTWORK GRID
    // =====================================================================

    /**
     * Render the artwork (constellation illustrations) grid.
     * 
     * @param {jQuery|string} $container - Target container
     * @param {Array} artworks - Artwork objects from extractArtwork
     * @param {Object} options - {tr}
     */
    function renderArtworkGrid($container, artworks, options) {
        options = options || {};
        var tr = options.tr;

        $container = toJQuery($container);
        if (!$container.length) return;
        $container.empty();

        if (!artworks || !artworks.length) {
            renderPlaceholder($container, tr('No artwork available for this culture'));
            return;
        }

        var cacheBuster = '?t=' + new Date().getTime();
        var cultureId = State.raw.culture.id || 'unknown';

        var html = '<div class="artwork-grid" data-culture="' + escapeHtml(cultureId) + '">';

        for (var i = 0; i < artworks.length; i++) {
            var art = artworks[i];
            var imageUrl = art.imagePath + cacheBuster;

            html += '<div class="artwork-item' +
                    '" data-artwork-id="' + escapeHtml(art.id) +
                    '" data-search-name="' + escapeHtml(art.searchName) +
                    '" data-artwork-name="' + escapeHtml(art.name) + '">';

            html += '<div class="artwork-image-wrapper">';
            html += '<img src="' + escapeHtml(imageUrl) +
                    '" alt="' + escapeHtml(art.name) +
                    '" class="artwork-image" loading="lazy" ' +
                    'onerror="this.style.display=\'none\'; ' +
                    'this.parentNode.querySelector(\'.artwork-error\').style.display=\'flex\';" />';
            html += '<div class="artwork-error" style="display:none;position:absolute;' +
                    'top:0;left:0;width:100%;height:100%;align-items:center;' +
                    'justify-content:center;background:rgba(0,0,0,0.1);color:#8A8C8E;' +
                    'font-size:11px;">' + escapeHtml(tr('Image not found')) + '</div>';
            html += '<button class="artwork-zoom-btn" title="' +
                    escapeHtml(tr('Zoom in')) + '">' +
                    '<span class="artwork-zoom-icon">⊕</span></button>';
            html += '</div>';

            html += '<div class="artwork-name">' + escapeHtml(art.name) + '</div>';

            if (art.nativeName && art.nativeName !== art.name) {
                html += '<div class="artwork-native-name">' +
                        escapeHtml(art.nativeName) + '</div>';
            }

            html += '</div>';
        }

        html += '</div>';
        $container.html(html);
    }

    // =====================================================================
    // ARTWORK MODAL
    // =====================================================================

    /**
     * Show an artwork image in a full-screen modal.
     * 
     * @param {string} imageSrc - Image source URL
     * @param {string} imageName - Image display name
     * @param {Object} tr - Translation function
     */
    function showArtworkModal(imageSrc, imageName, tr) {
        $('#artwork-modal-overlay').remove();

        var html =
            '<div id="artwork-modal-overlay" class="artwork-modal-overlay">' +
                '<div class="artwork-modal-content">' +
                    '<button class="artwork-modal-close" title="' +
                    escapeHtml(tr('Close')) + '">✕</button>' +
                    '<div class="artwork-modal-image-wrapper">' +
                        '<img src="' + escapeHtml(imageSrc) +
                        '" alt="' + escapeHtml(imageName) +
                        '" class="artwork-modal-image" />' +
                    '</div>' +
                    '<div class="artwork-modal-name">' +
                    escapeHtml(imageName) + '</div>' +
                '</div>' +
            '</div>';

        $('body').append(html);

        $('#artwork-modal-overlay').on('click', function(e) {
            if (e.target === this || $(e.target).closest('.artwork-modal-close').length) {
                $(this).fadeOut(Config.ANIMATION_DURATION, function() {
                    $(this).remove();
                });
            }
        });

        $(document).off('keydown.artworkModal').on('keydown.artworkModal', function(e) {
            if (e.key === 'Escape') {
                $('#artwork-modal-overlay').fadeOut(Config.ANIMATION_DURATION, function() {
                    $(this).remove();
                    $(document).off('keydown.artworkModal');
                });
            }
        });
    }

    // =====================================================================
    // BUTTON STATE MANAGEMENT
    // =====================================================================

    /**
     * Clear active state from all pattern buttons across all containers.
     */
    function clearAllActiveButtons() {
        var containers = State.raw.ui.containers;
        var keys = ['constellations', 'asterisms', 'zodiac', 'lunar', 'stars'];

        for (var i = 0; i < keys.length; i++) {
            var $c = toJQuery(containers[keys[i]]);
            if ($c.length) {
                $c.find('.pattern-btn').removeClass('active');
            }
        }

        var $artwork = toJQuery(containers.artwork);
        if ($artwork.length) {
            $artwork.find('.artwork-item').removeClass('selected-artwork');
        }
    }

    /**
     * Highlight the button matching a given pattern ID in a given container.
     * Uses a simple, reliable matching strategy: exact ID match first,
     * then fallback to name matching.
     * 
     * @param {jQuery|string} $container - Target container
     * @param {string} patternId - Pattern ID
     * @param {string} patternName - Pattern name (fallback)
     */
    function highlightButton($container, patternId, patternName) {
        $container = toJQuery($container);
        if (!$container.length) return;

        $container.find('.pattern-btn').removeClass('active');

        if (!patternId && !patternName) return;

        $container.find('.pattern-btn').each(function() {
            var $btn = $(this);
            var btnId   = $btn.data('pattern-id');
            var btnName = $btn.data('pattern-name');

            if ((patternId && btnId === patternId) ||
                (patternName && btnName === patternName)) {
                $btn.addClass('active');
            }
        });
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        toJQuery:                 toJQuery,
        escapeHtml:               escapeHtml,
        renderPlaceholder:        renderPlaceholder,
        getPlaceholder:           getPlaceholder,
        updatePatternCount:       updatePatternCount,
        renderCultureButtons:     renderCultureButtons,
        updateCultureButtonStates:updateCultureButtonStates,
        renderPatternGrid:        renderPatternGrid,
        renderGroupedStars:       renderGroupedStars,
        renderArtworkGrid:        renderArtworkGrid,
        showArtworkModal:         showArtworkModal,
        updateVisibilityItemCount:updateVisibilityItemCount,				
        clearAllActiveButtons:    clearAllActiveButtons,
        highlightButton:          highlightButton
    };
});