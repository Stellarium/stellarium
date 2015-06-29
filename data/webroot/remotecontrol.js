"use strict";

var UISettings = (new function($) {

    // Automatically poll the server for updates? Otherwise, only updated after commands are sent.
    this.updatePoll = true;
    //the interval for automatic polling
    this.updateInterval = 1000;
    //use the Browser's requestAnimationFrame for animation instead of setTimeout
    this.useAnimationFrame = true;
    //If animation frame is not used, this is the delay between 2 animation steps
    this.animationDelay = 500;
    //If no user changes happen after this time, the changes are sent to the Server
    this.editUpdateDelay = 500;


}(jQuery));

//DOM-ready hook, starting the GUI
$(document).ready(function() {
    Main.init();
});

// The main functionality including communication with the server
var Main = (new function($) {
    //Private variables
    var connectionLost = false;
    var animationSupported;
    var lastDataTime;

    //keeps track of StelAction updates from server
    var lastActionId = -2;

    //controls
    var $noresponse;
    var $noresponsetime;

    var sel_infostring;

    function animate() {

        Time.updateAnimation();

        if (connectionLost) {
            var elapsed = ($.now() - lastDataTime) / 1000;
            var text = Math.floor(elapsed).toString();
            $noresponsetime.html(text);
        }

        if (UISettings.useAnimationFrame && animationSupported) {
            window.requestAnimationFrame(animate);
        } else {
            setTimeout(animate, UISettings.animationDelay);
        }
    }

    //main update function, which is executed each second
    function update(requeue) {
        $.ajax({
            url: "/api/main/status",
            data: {
                actionId: lastActionId
            },
            dataType: "json",
            success: function(data) {
                lastDataTime = $.now();

                if (data.selectioninfo) {
                    sel_infostring.innerHTML = data.selectioninfo;
                    sel_infostring.className = "";
                } else {
                    sel_infostring.innerHTML = "No current selection";
                    sel_infostring.className = "bold";
                }


                //update modules with data
                Time.updateFromServer(data.time);
                Scripts.updateFromServer(data.script);
                Locations.updateFromServer(data.location);
                ViewControl.updateFromServer(data.view);
                ViewOptions.updateFromServer(data.view);

                if (data.actionChanges.id !== lastActionId) {
                    var error = Actions.updateFromServer(data.actionChanges.changes);
                    if (error) {
                        //this is required to make sure the actions are loaded first
                        console.log("action change error, resending same id");
                    } else {
                        lastActionId = data.actionChanges.id;
                    }
                }

                $noresponse.hide();
                connectionLost = false;
            },
            error: function(xhr, status, errorThrown) {
                $noresponse.show();
                connectionLost = true;
                lastActionId = -2;
                console.log("Error fetching updates");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
            },
            complete: function() {
                if (requeue && UISettings.updatePoll) {
                    setTimeout(function() {
                        update(true);
                    }, UISettings.updateInterval);
                }
            }
        });
    }

    function fixJQueryUI() {
        //fix for jQuery UI autocomplete to respect the size of the "of" positional parameter
        //http://bugs.jqueryui.com/ticket/9056
        var proto = $.ui.autocomplete.prototype;
        $.extend(proto, {
            _resizeMenu: function() {
                var ul = this.menu.element;
                ul.outerWidth(Math.max(
                    // Firefox wraps long text (possibly a rounding bug)
                    // so we add 1px to avoid the wrapping (#7513)
                    ul.width("").outerWidth() + 1,
                    this.options.position.of == null ? this.element.outerWidth() : this.options.position.of.outerWidth()
                ));
            }
        });

        //disable forced spinner rounding
        //http://stackoverflow.com/questions/19063956/ui-spinner-bug-when-changing-value-using-mouse-up-and-down

        /* //disabled for now until globalize.js works
        proto = $.ui.spinner.prototype;
        $.extend(proto, {
            _adjustValue: function(value) {
                var base, aboveMin,
                    options = this.options;

                // fix precision from bad JS floating point math
                //value = parseFloat(value.toFixed(this._precision()));

                // clamp the value
                if (options.max !== null && value > options.max) {
                    return options.max;
                }
                if (options.min !== null && value < options.min) {
                    return options.min;
                }

                return value;
            }
        });
*/
    }

    function initControls() {
        //find some controls
        $noresponse = $("#noresponse");
        $noresponsetime = $("#noresponsetime");

        sel_infostring = document.getElementById("sel_infostring");

        var $loading = $("#loadindicator").hide(),
            timer;
        $(document).ajaxStart(function() {
            timer && clearTimeout(timer);
            timer = setTimeout(function() {
                $loading.show();
            }, 100)
        });
        $(document).ajaxStop(function() {
            clearTimeout(timer);
            $loading.hide();
        });
    }

    //an simple Update queue class that updates the server after a specified interval if the user does nothing else inbetween
    function UpdateQueue(url, finishedCallback) {
        if (!(this instanceof UpdateQueue))
            return new UpdateQueue(url);

        this.url = url;
        this.isQueued = false;
        this.timer = undefined;
        this.finishedCallback = finishedCallback;
    }

    UpdateQueue.prototype.enqueue = function(data) {
        //what to do when enqueuing while sending?
        this.isQueued = true;

        this.timer && clearTimeout(this.timer);
        this.timer = setTimeout($.proxy(function() {
            this.isTransmitting = true;
            //post an update
            Main.postCmd(this.url, data, $.proxy(function() {
                Main.forceUpdate();
                this.isQueued = false;
                if (this.finishedCallback) {
                    this.finishedCallback();
                }
            }, this));
        }, this), UISettings.editUpdateDelay);

    };

    //Public stuff
    return {
        init: function() {
            initControls();

            Actions.init();
            Scripts.init();
            Search.init();
            Time.init();
            Locations.init();
            ViewControl.init();
            ViewOptions.init();

            fixJQueryUI();

            animationSupported = (window.requestAnimationFrame !== undefined);

            if (!animationSupported) {
                console.log("animation frame not supported")
            } else {
                console.log("animation frame supported")
            }
            //kick off animation
            animate();

            //start the update loop
            update(true);
        },

        //POST a command to the server
        postCmd: function(url, data, completeFunc, successFunc) {
            $.ajax({
                url: url,
                method: "POST",
                data: data,
                dataType: "text",
                success: function(data) {
                    if (successFunc) {
                        successFunc(data);
                    } else {
                        console.log("server replied: " + data);
                        if (data !== "ok") {
                            alert(data);
                        }
                    }
                    update();
                },
                error: function(xhr, status, errorThrown) {
                    console.log("Error posting command " + url);
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Could not update the server!")
                },
                complete: completeFunc
            });
        },

        forceUpdate: function() {
            update();
        },

        UpdateQueue: UpdateQueue

    }
}(jQuery));


//some custom controls
(function($) {
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
                .attr("title", "Show All Items")
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
                .attr("title", value + " didn't match any item")
                .tooltip("open");
            this.element.val("");
            this._delay(function() {
                this.input.tooltip("close").attr("title", "");
            }, 2500);
            this.input.autocomplete("instance").term = "";
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
})(jQuery);