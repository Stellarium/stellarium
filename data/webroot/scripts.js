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