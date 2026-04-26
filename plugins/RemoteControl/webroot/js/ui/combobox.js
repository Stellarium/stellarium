define(["jquery", "api/remotecontrol", "jquery-ui"], function($, rc) {
    "use strict";

    $.widget("custom.combobox", {
        _create: function() {
            this.wrapper = $("<span>")
                .addClass("custom-combobox ui-widget ui-widget-content")
                .insertAfter(this.element);

            this.element.hide();
            this._createAutocomplete();
            this._createShowAllButton();
        },

        _createAutocomplete: function() {
            var selected = this.element.children(":selected"),
                value = selected.val() ? selected.text() : "";

            this.input = $("<input>")
                .appendTo(this.wrapper)
                .val(value)
                .attr("title", "")
                .addClass("custom-combobox-input")
                .autocomplete({
                    delay: 0,
                    minLength: 0,
                    source: $.proxy(this, "_source"),
                    position: {
                        my: "left top",
                        at: "left bottom",
                        of: this.wrapper,
                        collision: "flipfit"
                    }
                })
                .tooltip({
                    tooltipClass: "ui-state-highlight"
                });

            this._on(this.input, {
                autocompleteselect: function(event, ui) {
                    ui.item.option.selected = true;
                    this.input.blur();
                    this._trigger("select", event, {
                        item: ui.item.option
                    });
                },

                autocompletechange: "_removeIfInvalid"
            });
        },

        _createShowAllButton: function() {
            var input = this.input,
                wasOpen = false;

            $("<a>")
                .attr("tabIndex", -1)
                .attr("title", rc.tr("Show All Items"))
                .tooltip()
                .appendTo(this.wrapper)
                .button({
                    icons: {
                        primary: "ui-icon-triangle-1-s"
                    },
                    text: false
                })
                .removeClass("ui-corner-all")
                .addClass("custom-combobox-toggle")
                .mousedown(function() {
                    wasOpen = input.autocomplete("widget").is(":visible");
                })
                .click(function() {
                    input.focus();

                    // Close if already visible
                    if (wasOpen) {
                        return;
                    }

                    // Pass empty string as value to search for, displaying all results
                    input.autocomplete("search", "");
                });
        },

        _source: function(request, response) {
            var matcher = new RegExp($.ui.autocomplete.escapeRegex(request.term), "i");
            response(this.element.children("option").map(function() {
                var text = $(this).text();
                if (this.value && (!request.term || matcher.test(text)))
                    return {
                        label: text,
                        value: text,
                        option: this
                    };
            }));
        },

        _removeIfInvalid: function(event, ui) {

            // Selected an item, nothing to do
            if (ui.item) {
                return;
            }

            // Search for a match (case-insensitive)
            var value = this.input.val(),
                valueLowerCase = value.toLowerCase(),
                valid = false;
            this.element.children("option").each(function() {
                if ($(this).text().toLowerCase() === valueLowerCase) {
                    this.selected = valid = true;
                    return false;
                }
            });

            // Found a match, nothing to do
            if (valid) {
                return;
            }

            // Remove invalid value
            this.input
                .val("")
                .attr("title", rc.tr("Input did not match any item"))
                .tooltip("open");
            this.element.val("");
            this._delay(function() {
                this.input.tooltip("close").attr("title", "");
            }, 2500);
            this.input.autocomplete("instance").term = "";
        },

        // NEW: Set value programmatically
        setValue: function(value) {
            var $option = this.element.find("option[value='" + value + "']");
            if ($option.length) {
                this.element.val(value);
                this.input.val($option.text());
                // Update selected property
                this.element.find("option").prop("selected", false);
                $option.prop("selected", true);
                return true;
            }
            return false;
        },

        // NEW: Get current value
        getValue: function() {
            return this.element.val();
        },

        // NEW: Get current display text
        getDisplayText: function() {
            return this.input.val();
        },

        // NEW: Refresh the combobox display based on current selection
        refresh: function() {
            var selectedOption = this.element.find("option:selected");
            if (selectedOption.length) {
                this.input.val(selectedOption.text());
            }
        },

        _destroy: function() {
            this.wrapper.remove();
            this.element.show();
        },

        autocomplete: function(value) {
            this.element.val(value);
            var e = this.element[0];
            if (e.selectedIndex >= 0) {
                this.input.val(e.options[e.selectedIndex].text);
            } else {
                this.input.val(value);
            }
        }
    });
});