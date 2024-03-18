// The main functionality including communication with the server

define(["jquery", "settings", "translationdata"], function($, settings, TranslationData) {
    "use strict";

    //Private variables
    var connectionLost = false;
    var lastDataTime;

    //keeps track of StelAction & StelProperty updates from server
    var lastActionId = -2;
    var lastPropId = -2;

    // Translates a string using Stellariums current locale.
    // String must be present in translationdata.js
    // All strings from tr() calls in the .js files will be written in translationdata.js when update_translationdata.py is executed
    function tr(str) {
        //extract varargs if existing
        var args = arguments;
        var trStr = str;
        if (str in TranslationData) {
            trStr = TranslationData[str];
        } else {
            console.log("Error: string '" + str + "' not present in translation data");
        }

        if (args.length > 1) {
            //replace args, like QString::arg does (%1, %2,...)
            trStr = trStr.replace(/%(\d+)/g, function(match, num) {
                return typeof args[num] != 'undefined' ? args[num] : match;
            });
        }
        return trStr;
    }

    //main update function, which is executed each second
    function update(requeue) {
        $.ajax({
            url: "/api/main/status",
            data: {
                actionId: lastActionId,
                propId: lastPropId
            },
            dataType: "json",
            success: function(data) {
                lastDataTime = $.now();

                //allow interested modules to react to the event
                $(rc).trigger('serverDataReceived', data);

                if (data.actionChanges.id !== lastActionId) {
                    var evt = $.Event("stelActionsChanged");
                    $(rc).trigger(evt, data.actionChanges.changes, lastActionId);
                    if (evt.isDefaultPrevented()) {
                        //if this is set, dont update the action id
                        //this is required to make sure the actions are loaded first
                        console.log("action change error, resending same id on next update");
                    } else {
                        lastActionId = data.actionChanges.id;
                    }
                }

                if (data.propertyChanges.id !== lastPropId) {
                    var evt = $.Event("stelPropertiesChanged");
                    $(rc).trigger(evt, data.propertyChanges.changes, lastPropId);
                    if (evt.isDefaultPrevented()) {
                        //if this is set, dont update the prop id
                        //this is required to make sure the props are loaded first
                        console.log("prop change error, resending same id on next update");
                    } else {
                        lastPropId = data.propertyChanges.id;
                    }
                }

                connectionLost = false;
            },
            error: function(xhr, status, errorThrown) {

                $(rc).trigger("serverDataError", errorThrown);

                connectionLost = true;
                //reset IDs to force a full reload when connection is re-established
                lastActionId = -2;
                lastPropId = -2;
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

    //remove panels for disabled plugins and load additional JS files if required for enabled ones
    function processPluginInfo(data) {
        //iterate over all stelplugin elements
        $(".stelplugin").each(function() {
            var self = $(this);
            var pluginId = self.data("plugin");
            if (!pluginId) {
                console.error("Plugin element needs a data-plugin attribute!");
                console.dir(this);
            } else {
                //check if the plugin is known and is enabled
                if(data[pluginId] && data[pluginId].loaded)
                {
                    console.log("plugin " + pluginId + " is enabled");
                    //check if additional JS files are required
                    var js = self.data("pluginjs");
                    if(js)
                    {
                        require([js],function(){
                            console.log("additional plugin JS files loaded: " + js);
                        });
                    }
                }
                else
                {
                    //plugin is not loaded, remove elements
                    console.log("plugin " + pluginId + " not enabled, removing DOM elements");
                    self.remove();
                }

            }
        });
    }

    // load plugin list, and disable/load elements if required
    function loadPlugins() {
        $.ajax({
            //we somehow should make sure this always finishes before the UI starts loading
            //or we may have a race condition
            //probably make the mainui wait on an event from this, and all the other UIs on the mainui
            async: false, //TODO fix so this can be removed
            url: '/api/main/plugins',
            type: 'GET',
            dataType: 'json',
            success: function(data) {
                //when the document is ready (can be now), process plugin info
                $(function() {
                    processPluginInfo(data);
                });
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error loading plugin information");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Error loading plugin information");
            }
        });
    }

    //start this as soon as possible
    loadPlugins();

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
