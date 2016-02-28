define(["jquery", "./remotecontrol"], function($, rc) {
    "use strict";

    var activescript;

    function changeActiveScript(scriptName) {
        if (activescript !== scriptName) {
            activescript = scriptName;
            $(publ).trigger("activeScriptChanged", activescript);
        }
    }

    $(rc).on("serverDataReceived", function(evt, data) {

        if (data.script.scriptIsRunning) {
            changeActiveScript(data.script.runningScriptId);
        } else if (activescript) {
            //a script was running before, but now is not
            changeActiveScript(undefined);
        }

    });

    //Public stuff
    var publ = {
        loadScriptList: function(callback) {
            $.ajax({
                url: "/api/scripts/list",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating script list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve script list"));
                }
            });
        },

        runScript: function(name) {
            //change the local active script for improved responsiveness
            changeActiveScript(name);

            //post a run requests
            rc.postCmd("/api/scripts/run", {
                id: name
            });
        },

        //sends the given code to the server for direct execution
        //if useIncludes is true, the default script directories are used for preprocessing (i.e. include statements)
        runDirectScript: function(code, useIncludes) {
            console.log("Sending script: " + code);
            rc.postCmd("/api/scripts/direct",{
                code: code,
                useIncludes: useIncludes
            });
        },

        stopScript: function() {
            //change the local active script for improved responsiveness
            changeActiveScript(undefined);

            //post a stop request
            rc.postCmd("/api/scripts/stop");
        }
    };

    return publ;

});