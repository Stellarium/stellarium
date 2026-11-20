/* jshint expr: true */
/**
 * archaeolines.js - ArchaeoLines Plugin Extension
 * ========================================================================
 * 
 * Reads the fixed "target" buttons declared in index.html and binds
 * click handlers that fetch the currently selected object's data from
 * /api/objects/info, extracting the requested coordinate into the
 * matching spinner and label input.
 * 
 * Loaded automatically via data-pluginjs="plugins/archaeolines".
 * 
 * @module plugins/archaeolines
 * @requires jquery
 * @requires api/remotecontrol
 */

define(["jquery", "api/remotecontrol"], function($, rc) {
    "use strict";

    var tr = rc.tr;
    var bound = false;

    // =====================================================================
    // OBJECT INFO FETCHING
    // =====================================================================

    /**
     * Fetch info about the currently selected object.
     * @param {Function} callback - Receives object info, or null.
     */
    function fetchSelectedObjectInfo(callback) {
        $.ajax({
            url:      '/api/objects/info',
            method:   'GET',
            data:     { format: 'json' },
            dataType: 'json',
            timeout:  5000
        }).done(function(data) {
            if (!data || data.found !== true) {
                callback(null);
                return;
            }
            callback(data);
        }).fail(function(xhr, status, err) {
            console.error('[ArchaeoLines] Failed to fetch object info:', err);
            callback(null);
        });
    }

    /**
     * Extract a coordinate from object info.
     * @param {Object} info      - /api/objects/info result.
     * @param {string} coordType - 'altitude' | 'azimuth' | 'declination'.
     * @returns {number|null}
     */
    function extractCoordinate(info, coordType) {
        if (!info) return null;
        switch (coordType) {
            case 'altitude':
                return typeof info.altitude === 'number' ? info.altitude : null;
            case 'azimuth':
                return typeof info.azimuth  === 'number' ? info.azimuth  : null;
            case 'declination':
                if (typeof info.decJ2000 === 'number') return info.decJ2000;
                if (typeof info.dec      === 'number') return info.dec;
                return null;
            default:
                return null;
        }
    }

    /**
     * Return the best display name.
     */
    function getObjectName(info) {
        if (!info) return '';
        return info['localized-name'] || info.name || '';
    }

    // =====================================================================
    // UI FEEDBACK
    // =====================================================================

    function flashSuccess($btn) {
        $btn.addClass('archaeo-target-success');
        setTimeout(function() {
            $btn.removeClass('archaeo-target-success');
        }, 700);
    }

    function showStatusMessage(message) {
        if ($.fn.dialog) {
            $('<div title="ArchaeoLines"></div>')
                .text(message)
                .dialog({
                    modal: true,
                    width: 400,
                    buttons: {
                        OK: function() {
                            $(this).dialog('close');
                        }
                    },
                    close: function() {
                        $(this).remove();
                    }
                });
        } else {
            alert(message);
        }
    }

    // =====================================================================
    // TARGET BUTTON HANDLER
    // =====================================================================

    /**
     * Handle a click on a target button.
     * 
     * Reads data-* attributes to find the target spinner and label.
     * 
     * @param {jQuery} $btn
     */
    function handleTargetClick($btn) {
        if ($btn.hasClass('loading')) return;

        var coordType    = $btn.data('coord-type');
        var spinnerProp  = $btn.data('spinner-prop');
        var labelProp    = $btn.data('label-prop');

        $btn.addClass('loading').prop('disabled', true);

        fetchSelectedObjectInfo(function(info) {
            $btn.removeClass('loading').prop('disabled', false);

            if (!info) {
                showStatusMessage(
                    tr('No object selected. Please select an object in Stellarium first.')
                );
                return;
            }

            var value = extractCoordinate(info, coordType);
            var name  = getObjectName(info);

            if (value === null || value === undefined) {
                showStatusMessage(
                    tr('Could not extract the requested coordinate from the selected object.')
                );
                return;
            }

            // Fill the value spinner
            var $spinner = $('input[name="' + spinnerProp + '"]');
            if ($spinner.length) {
                if ($spinner.data('ui-spinner')) {
                    $spinner.spinner('value', value);
                } else {
                    $spinner.val(Number(value).toFixed(3));
                }
                // Notify mainui.js so it pushes the value to the server
                $spinner.trigger('change');
            }

            // Fill the label input
            if (name && labelProp) {
                var $labelInput = $('input[name="' + labelProp + '"]');
                if ($labelInput.length) {
                    $labelInput.val(name);
                    $labelInput.trigger('change');
                }
            }

            flashSuccess($btn);
        });
    }

    // =====================================================================
    // INITIALIZATION
    // =====================================================================

    /**
     * Bind click handlers to all target buttons that are declared in HTML.
     * Idempotent.
     */
    function init() {
        if (bound) {
            console.log('[ArchaeoLines] Already bound, skipping.');
            return;
        }

        var $btns = $('#sub_tab_ArchaeoLines .archaeo-target-btn');
        if (!$btns.length) {
            console.warn('[ArchaeoLines] No target buttons found in DOM.');
            return;
        }

        $btns.off('click.archaeo').on('click.archaeo', function(e) {
            e.preventDefault();
            e.stopPropagation();
            handleTargetClick($(this));
        });

        bound = true;
        console.log('[ArchaeoLines] Bound ' + $btns.length + ' target buttons.');
    }

    // remotecontrol.js calls require() from inside $(document).ready(),
    // so DOM is ready. We still guard with $(function(){...}) for safety.
    $(function() {
        init();
    });

    return {
        init: init,
        _fetchObjectInfo: fetchSelectedObjectInfo,
        _extractCoordinate: extractCoordinate
    };
});