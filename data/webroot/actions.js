

var Actions = (function ($) {
    "use strict";

    //Private variables
    var $actionlist;
    var actionsInitialized = false;

    var actionData = {};


    //this is a list of all actions together with their button image
    //that automatically should be created and connected
    var buttonActions = [
        [
            "actionShow_Constellation_Lines", "btConstellationLines"
        ],
        [
            "actionShow_Constellation_Labels", "btConstellationLabels"
        ],
        [
            "actionShow_Constellation_Art", "btConstellationArt"
        ],
        [
            "actionShow_Equatorial_Grid", "btEquatorialGrid"
        ],
        [
            "actionShow_Azimuthal_Grid", "btAzimuthalGrid"
        ],
        [
            "actionShow_Ground", "btGround"
        ],
        [
            "actionShow_Cardinal_Points", "btCardinalPoints"
        ],
        [
            "actionShow_Atmosphere", "btAtmosphere"
        ],
        [
            "actionShow_Nebulas", "btNebula"
        ],
        [
            "actionShow_Planets_Labels", "btPlanets"
        ],
        [
            "actionSwitch_Equatorial_Mount", "btEquatorialMount"
        ],
        [   //this is actually a different action than the main GUI, but toggles properly (which the one from the GUI does not for some reason)
            "actionSet_Tracking", "btGotoSelectedObject"
        ],
        [
            "actionShow_Night_Mode", "btNightView"
        ],
        [
            "actionSet_Full_Screen_Global", "btFullScreen"
        ]
    ];

    function initControls() {
        $actionlist = $("#actionlist");

        var runactionfn = function () {
            var e = $actionlist[0];
            var id = e.options[e.selectedIndex].value;

            Actions.execute(id);
        };

        $actionlist.change(function () {
            $("#bt_doaction").prop("disabled", false);
        }).dblclick(runactionfn);

        $("#bt_doaction").click(runactionfn);
    }

    function createAutoButtons() {
        var $ul = $("#autobuttons");
        var parent = $ul.parent();
        $ul.detach();

        buttonActions.forEach(function (val) {
            var btn = document.createElement("button");
            btn.className = "button32 " + val[1];
            btn.value = val[0];

            var li = document.createElement("li");
            li.className = "button32wrapper";
            li.appendChild(btn);

            $ul.append(li);
        });

        //this automatically connects to StelAction
        connectActionContainer($ul);

        $ul.appendTo(parent);
    }

    function fireActionChanged(actionId) {
        console.log("stelActionChanged: " + actionId);
        $(document).trigger("stelActionChanged", {
            id: actionId,
            isChecked: actionData[actionId].isChecked
        });
    }

    function handleActionChanged(evt, data) {
        var action = actionData[data.id];
        action.option.textContent = action.text + " (" + data.isChecked + ")";
        if (action.checkboxes) {
            action.checkboxes.forEach(function (val) {
                val.checked = data.isChecked;
            });
        }
        if (action.buttons) {
            action.buttons.forEach(function (val) {
                $(val).toggleClass("active", data.isChecked);
            });
        }
    }

    function fillActionList() {
        //fill the action list
        $.ajax({
            url: "/api/stelaction/list",
            success: function (data) {
                $actionlist.empty();

                $.each(data, function (key, val) {
                    //the key is an optgroup
                    var group = $("<optgroup/>").attr("label", key);
                    //the val is an array of StelAction objects
                    $.each(val, function (idx, v2) {

                        var option = document.createElement("option");

                        var newData = {};
                        newData[v2.id] = {
                            isCheckable: v2.isCheckable,
                            isChecked: v2.isChecked,
                            text: v2.text,
                            option: option
                        };

                        $.extend(true, actionData, newData);

                        if (actionData[v2.id].buttons) {
                            //change tooltips
                            actionData[v2.id].buttons.forEach(function (val) {
                                val.title = v2.text;
                            });
                        }

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
                actionsInitialized = true;
            },
            error: function (xhr, status, errorThrown) {
                console.log("Error updating action list");
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve action list"));
            }
        });
    }


    function executeAction(actionName) {
        $.ajax({
            url: "/api/stelaction/do",
            method: "POST",
            dataType: "text",
            data: {
                id: actionName
            },
            success: function (resp) {
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
            error: function (xhr, status, errorThrown) {
                console.log("Error posting action " + actionName);
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(Main.tr("Error sending action to server: ") + errorThrown.message);
            }
        });
    }

    function connectActionContainer(elem) {
        var e = $(elem);

        //remember checkboxes for updating
        e.find("input[type='checkbox']").each(function () {
            if (this.value.lastIndexOf("action", 0) === 0) {
                var newData = {};
                newData[this.value] = {
                    checkboxes: [this]
                };
                $.extend(true, actionData, newData);

                //check if action has a checked value, apply it if it does
                if ("isChecked" in actionData[this.value]) {
                    this.checked = actionData[this.value].isChecked;
                }
            }
        });

        //give buttons the "active" class if the action is active
        e.find("input[type='button'], button").each(function () {
            if (this.value.lastIndexOf("action", 0) === 0) {
                var newData = {};
                newData[this.value] = {
                    buttons: [this]
                };
                $.extend(true, actionData, newData);

                //check if action has a checked value, apply it if it does
                if ("isChecked" in actionData[this.value]) {
                    this.className += "active";
                }
            }
        });

        //react to clicks by toggling/triggering the action
        e.on("click", "input[type='checkbox'], input[type='button'], button", function (evt) {
            //assume all values start with "action"
            if (this.value.lastIndexOf("action", 0) === 0) {
                executeAction(this.value);
            }
        });
    }

    //Public stuff
    return {
        init: function () {

            initControls();
            createAutoButtons();

            fillActionList();
            //attach handler after filling to prevent unnessecary calls
            $(document).on("stelActionChanged", handleActionChanged);

        },

        updateFromServer: function (changes) {
            if (!actionsInitialized)
                return true;

            var error = false;

            for (var name in changes) {
                if (name in actionData) {
                    if (changes[name] !== actionData[name].isChecked) {
                        actionData[name].isChecked = changes[name];
                        fireActionChanged(name);
                    }
                } else error = true;
            }

            return error;
        },

        //Trigger an StelAction on the Server
        execute: executeAction,

        //Connects all checkbox input elements below this container to the StelAction that corresponds to its value
        connectActionContainer: connectActionContainer
    };
})(jQuery);