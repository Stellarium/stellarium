define(["jquery", "./remotecontrol"], function($, rc) {
    "use strict";

    //Private variables

    var actionsInitialized = false;
    var actionData = {};

    function fireActionChanged(actionId) {
        var action = actionData[actionId];
        console.log("stelActionChanged: " + actionId + ", " + action.isChecked);

        //update buttons/checkboxes
        if (action.checkboxes) {
            action.checkboxes.forEach(function(val) {
                val.checked = action.isChecked;
            });
        }
        if (action.buttons) {
            action.buttons.forEach(function(val) {
                $(val).toggleClass("active", action.isChecked);
            });
        }

        //trigger changed events, one generic and one specific
        $(publ).trigger("stelActionChanged", [actionId, action]);
        $(publ).trigger("stelActionChanged:" + actionId, action);
    }

    function fillActionList() {
        //fill the action list
        $.ajax({
            url: "/api/stelaction/list",
            success: function(data) {

                $.each(data, function(key, val) {
                    //the key is an optgroup
                    //the val is an array of StelAction objects
                    $.each(val, function(idx, v2) {

                        var newData = {};
                        newData[v2.id] = {
                            isCheckable: v2.isCheckable,
                            isChecked: v2.isChecked,
                            text: v2.text,
                        };

                        $.extend(true, actionData, newData);

                        if (actionData[v2.id].buttons) {
                            //change tooltips
                            actionData[v2.id].buttons.forEach(function(val) {
                                val.title = v2.text;
                            });
                        }

                        if (v2.isCheckable) {
                            fireActionChanged(v2.id);
                        }
                    });
                });

                console.log("initial action load completed");
                actionsInitialized = true;

                $(publ).trigger("actionListLoaded", data);
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating action list");
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(rc.tr("Could not retrieve action list"));
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
                    alert(rc.tr("Action '%1' not accepted by server: ", actionName) + "\n" + resp);
                }
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error posting action " + actionName);
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(rc.tr("Error sending action '%1' to server: ", actionName) + errorThrown.message);
            }
        });
    }

    function connectCheckbox(elem) {
        $(elem).each(function() {
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
        }).on("click", function(evt) {
            //assume all values start with "action"
            if (this.value.lastIndexOf("action", 0) === 0) {
                executeAction(this.value);
            }
        });
    }

    function connectActionContainer(elem) {
        //console.log("connecting container");
        var e = $(elem);

        //remember checkboxes for updating
        e.find("input[type='checkbox']").each(function() {
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
        e.find("input[type='button'], button").each(function() {
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
        e.on("click", "input[type='checkbox'], input[type='button'], button", function(evt) {
            //assume all values start with "action"
            if (this.value.lastIndexOf("action", 0) === 0) {
                executeAction(this.value);
            }
        });
    }

    $(rc).on("stelActionsChanged", function(evt, changes) {
        if (!actionsInitialized) {
            //dont handle, tell main obj to send same id again
            evt.preventDefault();
            return;
        }

        var error = false;

        for (var name in changes) {
            if (name in actionData) {
                if (changes[name] !== actionData[name].isChecked) {
                    actionData[name].isChecked = changes[name];
                    fireActionChanged(name);
                }
            } else {
                //not really sure why this was necessary
                console.log("unknown action " + name + " changed");
                error = true;
            }
        }

        if (error)
            evt.preventDefault();
    });

    $(function() {
        fillActionList();
    });

    //Public stuff
    var publ = {
        //Trigger an StelAction on the Server
        execute: executeAction,

        //Connects all checkbox input elements and buttons below this container to the StelAction that corresponds to its value
        connectActionContainer: connectActionContainer,
        connectCheckbox: connectCheckbox,

        isChecked: function(id)
        {
            return actionData[id] ? actionData[id].isChecked : undefined;
        }
    };

    return publ;
});