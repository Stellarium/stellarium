// The main functionality including communication with the server

define(["jquery", "settings", "translationdata"], function($, settings, TranslationData) {
    "use strict";

    //Private variables
    var connectionLost = false;
    var lastDataTime;

    //keeps track of StelAction updates from server
    var lastActionId = -2;

    /* Translates a string using Stellariums current locale. String must be present in translationdata.js */
    function tr(str) {
        if (str in TranslationData) {
            return TranslationData[str];
        }
        console.log("Error: string '" + str + "' not present in translation data");
        return str;
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

                //allow interested modules to react to the event
                $(rc).trigger('serverDataReceived', data);

                /*
                //update modules with data
                Time.updateFromServer(data.time);
                Scripts.updateFromServer(data.script);
                Locations.updateFromServer(data.location);
                ViewControl.updateFromServer(data.view);
                ViewOptions.updateFromServer(data.view);
                */

                if (data.actionChanges.id !== lastActionId) {
                    var evt = $.Event("stelActionsChanged");
                    $(rc).trigger(evt, data.actionChanges.changes, lastActionId);
                    if (evt.isDefaultPrevented()) {
                        //if this is set, dont update the action id
                        //this is required to make sure the actions are loaded first
                        console.log("action change error, resending same id");
                    } else {
                        lastActionId = data.actionChanges.id;
                    }
                }

                connectionLost = false;
            },
            error: function(xhr, status, errorThrown) {

                $(rc).trigger("serverDataError", errorThrown);

                connectionLost = true;
                lastActionId = -2;
                console.log("Error fetching updates");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
            },
            complete: function() {
                if (requeue && settings.updatePoll) {
                    setTimeout(function() {
                        update(true);
                    }, settings.updateInterval);
                }
            }
        });
    }

    //Public stuff
    var rc = {
        //Global translation helper function
        tr: tr,
        //Kicks off the update loop. If the loop is disabled, this still requests the data one time
        startUpdateLoop: function() {
            update(true);
        },
        isConnectionLost: function() {
            return connectionLost;
        },
        getLastDataTime: function() {
            return lastDataTime;
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
                    alert(tr("Error sending command to server: ") + errorThrown.message);
                },
                complete: completeFunc
            });
        },

        forceUpdate: function() {
            update();
        },
    };

    return rc;
});