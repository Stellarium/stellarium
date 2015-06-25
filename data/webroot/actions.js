"use strict";

var Actions = (new function($) {
    //Private variables
    var $actionlist;

    var actionData = {};

    function initControls() {
        $actionlist = $("#actionlist");

        var runactionfn = function() {
            var e = $actionlist[0];
            var id = e.options[e.selectedIndex].value;

            Actions.execute(id);
        }

        $actionlist.change(function() {
            $("#bt_doaction").prop("disabled", false);
        }).dblclick(runactionfn);

        $("#bt_doaction").click(runactionfn);
    }

    function fireActionChanged(actionId) {
        console.log("stelActionChanged: " + actionId);
        $(document).trigger("stelActionChanged", { id: actionId, isChecked: actionData[actionId].isChecked });
    }

    function handleActionChanged(evt,data) {
        var action = actionData[data.id];
        action.option.textContent = action.text + " (" + data.isChecked + ")";
    }

    function fillActionList() {
        //fill the action list
        $.ajax({
            url: "/api/stelaction/list",
            success: function(data) {
                $actionlist.empty();
                actionData = {};

                $.each(data, function(key, val) {
                    //the key is an optgroup
                    var group = $("<optgroup/>").attr("label", key);
                    //the val is an array of StelAction objects
                    $.each(val, function(idx, v2) {

                        var option = document.createElement("option");

                        actionData[v2.id] = {
                            isCheckable: v2.isCheckable,
                            isChecked: v2.isChecked,
                            text: v2.text,
                            option: option
                        };
            
                        option.value = v2.id;

                        if (v2.isCheckable) {
                            option.className = "checkableaction";
                            option.textContent = v2.text + " (" + v2.isChecked + ")";

                            fireActionChanged(v2.id);
                        } else {
                            option.textContent = v2.text;
                        }

                        group.append(option);
                    });
                    group.appendTo($actionlist);
                });
                console.log("initial action load completed");
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating action list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve action list")
            }
        });
    }

    //Public stuff
    return {
        init: function() {

            initControls();

            fillActionList();
            //attach handler after filling to prevent unnessecary calls
            $(document).on("stelActionChanged",handleActionChanged);

        },

        updateFromServer: function(changes) {
            var error = false;
            for (var name in changes) {
                if (name in actionData) {
                    if (changes[name] !== actionData[name].isChecked) {
                        actionData[name].isChecked = changes[name];
                        fireActionChanged(name);
                    }
                }
                else error = true;
            }

            return error;
        },

        //Trigger an StelAction on the Server
        execute: function(actionName) {
            $.ajax({
                url: "/api/stelaction/do",
                method: "POST",
                dataType: "text",
                data: {
                    id: actionName
                },
                success: function(resp) {
                    if (resp === "ok") {
                        //non-checkable action, dont fire an event
                    } else if (resp === "true") {
                        if (!actionData[actionName].isChecked) {
                            actionData[actionName].isChecked = true;
                            fireActionChanged(actionName);
                        }
                    } else if (resp === "false") {
                        if (actionData[actionName].isChecked) {
                            actionData[actionName].isChecked = false;
                            fireActionChanged(actionName);
                        }
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