/* ========================================================================
 * skyculture-artwork.js - Artwork Panel Logic
 * ========================================================================
 * 
 * This module handles the "Artwork" tab in the sky culture panel:
 * - Rendering the artwork grid (delegated to skyculture/render)
 * - Click handling for navigation to constellations
 * - Synchronization with the constellation tab (bidirectional)
 * - Modal display of full-size images
 * 
 * The artwork panel shows constellation illustrations from the current
 * culture's index.json. Clicking an image navigates to the corresponding
 * constellation, and clicking a constellation button in the
 * "Constellations" tab highlights the matching artwork item.
 * 
 * @module skyculture/artwork
 * @requires jquery
 * @requires ./skyculture-config
 * @requires ./skyculture-state
 * @requires ./skyculture-render
 * @author kutaibaa akraa (GitHub: @kutaibaa-akraa)
 * @license GPLv2+
 * @version 1.0.0
 * ======================================================================== */

define([
    "jquery",
    "./skyculture-config",
    "./skyculture-state",
    "./skyculture-render",
    "ui/stellarium-utils"
], function($, Config, State, Render, stelUtils) {
    "use strict";

    // =====================================================================
    // CLICK LOCK
    // =====================================================================

    /** @type {boolean} Prevents rapid double-clicks from firing twice */
    var clickLocked = false;

    // =====================================================================
    // RENDER
    // =====================================================================

    /**
     * Render the artwork panel with the given artworks.
     * 
     * @param {Array} artworks - Array of artwork objects
     * @param {Object} tr - Translation function
     */
    function render(artworks, tr) {
        var $container = State.raw.ui.containers.artwork;
        if (!$container || !$container.length) return;

        // Store data for later access
        State.raw.data.artwork = artworks || [];

        // Update counter
        Render.updatePatternCount('artwork', State.raw.data.artwork.length);

        // Delegate to render module
        Render.renderArtworkGrid($container, State.raw.data.artwork, { tr: tr });

        // Attach handlers
        attachHandlers(tr);
    }

    // =====================================================================
    // EVENT HANDLERS
    // =====================================================================

    /**
     * Attach click handlers for artwork items and zoom buttons.
     * 
     * @param {Object} tr - Translation function
     */
    function attachHandlers(tr) {
        var $container = State.raw.ui.containers.artwork;
        if (!$container || !$container.length) return;

        // Item click → navigate to constellation
        $container.find('.artwork-item').off('click.artwork').on('click.artwork', function(e) {
            // Ignore clicks on the zoom button
            if ($(e.target).closest('.artwork-zoom-btn').length) return;

            if (clickLocked) return;
            clickLocked = true;

            var $item       = $(this);
            var searchName  = $item.data('search-name');
            var artworkId   = $item.data('artwork-id');
            var artworkName = $item.data('artwork-name');

            if (searchName) {
                // Toggle constellation highlight
                var isNowActive = stelUtils.toggleConstellationHighlight(searchName, artworkId);

                if (isNowActive) {
                    // Mark this artwork as selected
                    $container.find('.artwork-item').removeClass('selected-artwork');
                    $item.addClass('selected-artwork');

                    // Update State selection
                    State.setSelection(artworkName, artworkId, 'constellation');
                } else {
                    // Clear selection
                    $container.find('.artwork-item').removeClass('selected-artwork');
                    State.clearSelection();
                }
            }

            setTimeout(function() { clickLocked = false; }, 300);
        });

        // Zoom button → show modal
        $container.find('.artwork-zoom-btn').off('click.artworkZoom').on('click.artworkZoom', function(e) {
            e.preventDefault();
            e.stopPropagation();

            var $item   = $(this).closest('.artwork-item');
            var imgSrc  = $item.find('.artwork-image').attr('src');
            var imgAlt  = $item.find('.artwork-image').attr('alt');

            Render.showArtworkModal(imgSrc, imgAlt, tr);
        });
    }

    // =====================================================================
    // SELECTION STATE
    // =====================================================================

    /**
     * Highlight the artwork item matching a constellation ID.
     * Called when a constellation is selected elsewhere.
     * 
     * @param {string|null} constellationId - Constellation ID
     */
    function highlight(constellationId) {
        var $container = State.raw.ui.containers.artwork;
        if (!$container || !$container.length) return;

        $container.find('.artwork-item').removeClass('selected-artwork');

        if (!constellationId) return;

        $container.find('.artwork-item').each(function() {
            if ($(this).data('artwork-id') === constellationId) {
                $(this).addClass('selected-artwork');
                return false;
            }
        });
    }

    /**
     * Clear all artwork selections.
     */
    function clearHighlight() {
        var $container = State.raw.ui.containers.artwork;
        if (!$container || !$container.length) return;
        $container.find('.artwork-item').removeClass('selected-artwork');
    }

    // =====================================================================
    // PUBLIC API
    // =====================================================================

    return {
        render:         render,
        attachHandlers: attachHandlers,
        highlight:      highlight,
        clearHighlight: clearHighlight
    };
});