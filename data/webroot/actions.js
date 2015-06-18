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
