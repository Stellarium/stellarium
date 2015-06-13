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
    var lastData;
    var lastDataTime;

    //controls
    var $noresponse;
    var $noresponsetime;

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
            dataType: "json",
            success: function(data) {
                //data is stored here mainly for debugging
                lastData = data;
                lastDataTime = $.now();

                //update modules with data
                Time.updateFromServer(data.time);
                Scripts.updateFromServer(data.script);

                $noresponse.hide();
                connectionLost = false;
            },
            error: function(xhr, status, errorThrown) {
                $noresponse.show();
                connectionLost = true;
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

    //Public stuff
    return {
        init: function() {

            Actions.init();
            Scripts.init();
            Time.init();

            animationSupported = (window.requestAnimationFrame !== undefined);

            if (!animationSupported) {
                console.log("animation frame not supported")
            } else {
                console.log("animation frame supported")
            }
            //kick off animation
            animate();

            //find some controls
            $noresponse = $("#noresponse");
            $noresponsetime = $("#noresponsetime");

            //start the update loop
            update(true);
        },

        //POST a command to the server
        postCmd: function(url, data, completeFunc) {
            $.ajax({
                url: url,
                method: "POST",
                data: data,
                dataType: "text",
                timeout: 3000,
                success: function(data) {
                    console.log("server replied: " + data);
                    if (data !== "ok") {
                        alert(data);
                    }
                    update();
                },
                error: function(xhr, status, errorThrown) {
                    console.log("Error posting command " + url);
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Could not call server")
                },
                complete: completeFunc
            });
        },

        forceUpdate: function() {
            update();
        }
    }


}(jQuery));

var Actions = (new function($) {
    //Private variables
    var $actionlist;

    //Public stuff
    return {
        init: function() {

            $actionlist = $("#actionlist");

            $actionlist.change(function() {
                $("#bt_doaction").prop("disabled", false);
            });

            //fill the action list
            $.ajax({
                url: "/api/stelaction/list",
                success: function(data) {
                    $actionlist.empty();

                    $.each(data, function(key, val) {
                        //the key is an optgroup
                        var group = $("<optgroup/>").attr("label", key);
                        //the val is an array of StelAction objects
                        $.each(val, function(idx, v2) {
                            var option = $("<option/>");
                            option.data(v2); //store the data
                            if (v2.isCheckable) {
                                option.addClass("checkableaction");
                                option.text(v2.text + " (" + v2.isChecked + ")");

                                $actionlist.on("action:" + v2.id, {
                                    elem: option
                                }, function(event, newValue) {
                                    var el = event.data.elem;
                                    el.data("isChecked", newValue);
                                    el.text(el.data("text") + " (" + newValue + ")");
                                });
                            } else {
                                option.text(v2.text);
                            }
                            option.appendTo(group);
                        });
                        group.appendTo($actionlist);
                    });
                },
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating action list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Could not retrieve action list")
                }
            });
        },

        //Trigger an StelAction on the Server
        execute: function(actionName) {
            $.ajax({
                url: "/api/stelaction/do",
                method: "POST",
                async: false,
                dataType: "text",
                data: {
                    id: actionName
                },
                success: function(resp) {
                    if (resp === "ok") {
                        //non-checkable action, cant change text
                    } else if (resp === "true") {
                        $actionlist.trigger("action:" + actionName, true);
                    } else if (resp === "false") {
                        $actionlist.trigger("action:" + actionName, false);
                    } else {
                        alert(resp);
                    }
                },
                error: function(xhr, status, errorThrown) {
                    console.log("Error posting action " + actionName);
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Could not call server")
                }
            });
        }
    }
}(jQuery));

var Scripts = (new function($) {
    //Private variables
    var $activescript;
    var $bt_runscript;
    var $bt_stopscript;
    var $scriptlist;

    //Public stuff
    return {
        init: function() {

            $scriptlist = $("#scriptlist");
            $bt_runscript = $("#bt_runscript");
            $bt_stopscript = $("#bt_stopscript");
            $activescript = $("#activescript");

            $scriptlist.change(function() {
                var selection = $scriptlist.children("option").filter(":selected").text();

                $("#bt_runscript").prop({
                    disabled: false
                });
                console.log("selected: " + selection);
            });

            //init script list
            $.ajax({
                url: "/api/scripts/list",
                success: function(data) {
                    $scriptlist.empty();

                    //sort it and insert
                    $.each(data.sort(), function(idx, elem) {
                        $("<option/>").text(elem).appendTo($scriptlist);
                    });
                },
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating script list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Could not retrieve script list")
                }
            });
        },

        updateFromServer: function(script) {
            if (script.scriptIsRunning) {
                $activescript.text(script.runningScriptId);
            } else {
                $activescript.text("-none-");
            }
            $bt_stopscript.prop({
                disabled: !script.scriptIsRunning
            });
        }
    }
}(jQuery));
