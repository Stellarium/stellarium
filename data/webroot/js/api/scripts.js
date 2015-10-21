var Scripts = (function($) {
    "use strict";

    //Private variables
    var $activescript;
    var $bt_runscript;
    var $bt_stopscript;
    var $scriptlist;

    function initControls() {
        $scriptlist = $("#scriptlist");
        $bt_runscript = $("#bt_runscript");
        $bt_stopscript = $("#bt_stopscript");
        $activescript = $("#activescript");

        var runscriptfn = function() {
            var selection = $scriptlist.children("option").filter(":selected").text();
            if (selection) {
                //post a run requests
                Main.postCmd("/api/scripts/run", {
                    id: selection
                });
            }
        };

        $scriptlist.change(function() {
            var selection = $scriptlist.children("option").filter(":selected").text();

            $bt_runscript.prop({
                disabled: false
            });
            console.log("selected: " + selection);
        }).dblclick(runscriptfn);

        $bt_runscript.dblclick(runscriptfn);

        $bt_stopscript.click(function() {
            //post a stop request
            Main.postCmd("/api/scripts/stop");
        });
    }

    //Public stuff
    return {
        init: function() {

            initControls();

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
                    alert(Main.tr("Could not retrieve script list"));
                }
            });
        },

        updateFromServer: function(script) {
            if (script.scriptIsRunning) {
                $activescript.text(script.runningScriptId);
            } else {
                $activescript.text(Main.tr("-none-"));
            }
            $bt_stopscript.prop({
                disabled: !script.scriptIsRunning
            });
        }
    };
})(jQuery);