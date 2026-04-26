/**
 * Actions UI Module for Stellarium Remote Control
 * 
 * @module ui/actions
 * @description Implements the action list panel and manages StelAction buttons and checkboxes.
 * It dynamically loads all available StelActions from Stellarium, displays them in a categorized
 * select list, and provides real-time search/filtering capabilities to quickly locate actions.
 * 
 * Key Features:
 * - Dynamic loading of StelActions from Stellarium's action system via `api/actions`.
 * - Categorized display with optgroups matching Stellarium's internal organization.
 * - Real-time action state updates (checked/unchecked for toggle actions).
 * - **Search/Filter functionality to quickly find actions by name (case-insensitive).**
 * - Keyboard shortcut: 'Escape' clears the search and restores the full list.
 * - Integration with `stelaction` CSS class for buttons and checkboxes elsewhere in the UI.
 * 
 * Technical Architecture:
 * - Depends on `api/actions` to fetch data and execute commands.
 * - Caches the original action list structure for efficient filtering without re-fetching.
 * - Rebuilds the filtered list dynamically based on user input.
 * - Listens to `actionListLoaded` and `stelActionChanged` events from `api/actions`.
 * 
 * @requires jquery
 * @requires api/remotecontrol
 * @requires api/actions
 * 
 * @license GPLv2+
 */

define(["jquery", "api/remotecontrol", "api/actions"], function($, rc, actionApi) {
    "use strict";

    // =====================================================================
    // PRIVATE VARIABLES
    // =====================================================================

    var $actionlist;           // jQuery reference to the action select element
    var $searchInput;          // jQuery reference to the search input field
    var originalOptions = [];  // Stores original option structure for filtering

    // =====================================================================
    // FILTER / SEARCH FUNCTIONALITY
    // =====================================================================

    /**
     * Initializes the search/filter functionality for the action list.
     * Sets up event handlers for input filtering and keyboard shortcuts.
     * Stores the original action structure when actions are loaded.
     */
    function initSearchFilter() {
        // Store original option structure when actions are loaded
        $(actionApi).on("actionListLoaded", function() {
            storeOriginalOptions();
        });
        
        // Filter on input - real-time filtering as user types
        $searchInput.on("input", function() {
            var searchTerm = $(this).val().toLowerCase().trim();
            filterActionList(searchTerm);
        });
        
        // Clear search on Escape key
        $searchInput.on("keydown", function(e) {
            if (e.key === "Escape") {
                $(this).val("");
                filterActionList("");
                e.stopPropagation();
            }
        });
    }

    /**
     * Stores the original option structure for filtering.
     * This preserves the complete action list so we can filter without
     * re-fetching from the server.
     */
    function storeOriginalOptions() {
        originalOptions = [];
        $actionlist.find("optgroup").each(function() {
            var $group = $(this);
            var groupLabel = $group.attr("label");
            var groupOptions = [];
            
            $group.find("option").each(function() {
                var $option = $(this);
                groupOptions.push({
                    value: $option.val(),
                    text: $option.text(),
                    element: $option[0]
                });
            });
            
            originalOptions.push({
                label: groupLabel,
                options: groupOptions,
                element: $group[0]
            });
        });
    }

    /**
     * Filters the action list based on the search term.
     * Rebuilds the select element showing only actions that match the term.
     * Empty search term restores the complete original list.
     * 
     * @param {string} searchTerm - The term to search for (case-insensitive)
     */
    function filterActionList(searchTerm) {
        if (!originalOptions.length) return;
        
        // Clear current list
        $actionlist.empty();
        
        if (searchTerm === "") {
            // Restore original list when search is empty
            for (var i = 0; i < originalOptions.length; i++) {
                var group = originalOptions[i];
                var $optgroup = $("<optgroup>").attr("label", group.label);
                
                for (var j = 0; j < group.options.length; j++) {
                    var opt = group.options[j];
                    $("<option>").val(opt.value).text(opt.text).appendTo($optgroup);
                }
                $optgroup.appendTo($actionlist);
            }
        } else {
            // Build filtered list - only show matching options
            for (var i = 0; i < originalOptions.length; i++) {
                var group = originalOptions[i];
                var matchedOptions = [];
                
                for (var j = 0; j < group.options.length; j++) {
                    var opt = group.options[j];
                    if (opt.text.toLowerCase().indexOf(searchTerm) !== -1) {
                        matchedOptions.push(opt);
                    }
                }
                
                // Only create optgroup if it has matching options
                if (matchedOptions.length > 0) {
                    var $optgroup = $("<optgroup>").attr("label", group.label);
                    for (var k = 0; k < matchedOptions.length; k++) {
                        var opt = matchedOptions[k];
                        $("<option>").val(opt.value).text(opt.text).appendTo($optgroup);
                    }
                    $optgroup.appendTo($actionlist);
                }
            }
        }
        
        // Reset selection and button state after filtering
        $("#bt_doaction").prop("disabled", true);
    }

    // =====================================================================
    // UI CONTROL INITIALIZATION
    // =====================================================================

    /**
     * Initializes the action UI controls.
     * Sets up event handlers for the action select list and the run button.
     */
    function initControls() {
        $actionlist = $("#actionlist");
        $searchInput = $("#action-search-input");
        
        // Initialize search functionality if search input exists
        if ($searchInput.length) {
					// Set placeholder text using JavaScript translation
						$searchInput.attr("placeholder", rc.tr("Search actions..."));
            initSearchFilter();
        }

        var runactionfn = function() {
            var e = $actionlist[0];
            var id = e.options[e.selectedIndex].value;
            actionApi.execute(id);
        };

        $actionlist.change(function() {
            $("#bt_doaction").prop("disabled", false);
        }).dblclick(runactionfn);

        $("#bt_doaction").click(runactionfn);
    }

    // =====================================================================
    // EVENT HANDLERS
    // =====================================================================

    $(function() {
        initControls();
    });

    /**
     * Handles the actionListLoaded event from actionApi.
     * Populates the action select list with categorized actions from Stellarium.
     * Stores original options for filtering after list is built.
     */
    $(actionApi).on("actionListLoaded", function(evt, data) {
        // Fill the action list
        var parent = $actionlist.parent();
        $actionlist.detach();
        $actionlist.empty();

        $.each(data, function(key, val) {
            // The key is an optgroup category name
            var group = $("<optgroup/>").attr("label", key);
            // The val is an array of StelAction objects
            $.each(val, function(idx, v2) {

                var option = document.createElement("option");
                option.value = v2.id;

                if (v2.isCheckable) {
                    option.className = "checkableaction";
                    option.textContent = v2.text + " (" + (v2.isChecked ? rc.tr("on") : rc.tr("off")) + ")";
                } else {
                    option.textContent = v2.text;
                }

                group.append(option);
            });
            group.appendTo($actionlist);
        });
        $actionlist.prependTo(parent);
        
        // Store original options for filtering after list is built
        storeOriginalOptions();

        // Initialize stelaction checkboxes
        $("input[type='checkbox'].stelaction").each(function() {
            var self = $(this);
            var id = this.name;

            if (!id) {
                console.error('Error: no StelAction name defined on an "stelaction" element, element follows...');
                console.dir(this);
                alert('Error: no StelAction name defined on an "stelaction" element, see log for details');
            }

            self[0].checked = actionApi.isChecked(id);
        });

        $(document).on("click","input[type='checkbox'].stelaction, input[type='button'].stelaction, button.stelaction",function(){
            actionApi.execute(this.name);
        });
    });

    /**
     * Handles stelActionChanged events from actionApi.
     * Updates the display text for checkable actions when their state changes.
     */
    $(actionApi).on("stelActionChanged",function(evt,id,data){
        var option = $actionlist.find('option[value="'+id+'"]');
        if(option.length>0)
            option[0].textContent = data.text + " (" + (data.isChecked ? rc.tr("on") : rc.tr("off")) + ")";

        // Update checkboxes
        $("input[type='checkbox'][name='"+id+"'].stelaction").prop("checked",data.isChecked);
        $("input[type='button'][name='"+id+"'].stelaction, button[name='"+id+"'].stelaction").toggleClass("active",data.isChecked);
    });

});