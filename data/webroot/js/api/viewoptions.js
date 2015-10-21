var ViewOptions = (function ($) {
    "use strict";

    //Private variables
    var $selectedoption;
    var $vo_projectionlist;
    var $vo_landscapelist;
    var $vo_skyculturelist;
    var currentLandscape;
    var currentSkyculture;

    function initControls() {
        $vo_landscapelist = $("#vo_landscapelist");
        $vo_skyculturelist = $("#vo_skyculturelist");
        Actions.connectActionContainer($("#tab_view"));
        Actions.connectActionContainer($("#tab_landscape"));
        Actions.connectActionContainer($("#tab_skyculture"));
        $vo_projectionlist = $("#vo_projectionlist");
        $vo_projectionlist.dblclick(function () {
            if (this.selectedIndex >= 0) {
                var proj = this.options[this.selectedIndex].value;

                Main.postCmd("/api/view/setprojection", {
                    type: proj
                });
            }
        });

        $vo_landscapelist.dblclick(function(evt,data){
            if(this.selectedIndex >= 0) {
                var ls = this.options[this.selectedIndex].value;

                Main.postCmd("/api/view/setlandscape", {
                    id : ls
                });
            }
        });

        $vo_skyculturelist.dblclick(function(evt,data){
            if(this.selectedIndex >= 0) {
                var sc = this.options[this.selectedIndex].value;

                Main.postCmd("/api/view/setskyculture", {
                    id : sc
                });
            }
        });
    }

    function fillProjectionList() {
        //fill the projection list
        $.ajax({
            url: "/api/view/listprojection",
            success: function (data) {

                var $projectionlist = $("#vo_projectionlist");
                $projectionlist.empty();
                var parent = $projectionlist.parent();
                $projectionlist.detach();

                for (var val in data) {
                    var option = document.createElement("option");
                    option.value = val;
                    option.textContent = data[val];
                    $projectionlist.append(option);
                }

                $projectionlist.appendTo(parent);

            },
            error: function (xhr, status, errorThrown) {
                console.log("Error updating projection list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve projection list"));
            }
        });
    }

    function fillLandscapeList() {
        $.ajax({
            url: "/api/view/listlandscape",
            success: function (data) {
                var sortable = [];
                var val;
                for (val in data)
                    sortable.push([val, data[val]]);

                sortable.sort(function (a, b) {
                    if (a[1] < b[1])
                        return -1;
                    if (a[1] > b[1])
                        return 1;
                    return 0;
                });

                $vo_landscapelist.empty();

                sortable.forEach(function (val) {
                    var option = document.createElement("option");
                    option.value = val[0];
                    option.textContent = val[1];
                    $vo_landscapelist.append(option);
                });

            },
            error: function (xhr, status, errorThrown) {
                console.log("Error updating landscape list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve landscape list"));
            }
        });
    }

    function fillSkycultureList() {
        $.ajax({
            url: "/api/view/listskyculture",
            success: function (data) {
                var sortable = [];
                var val;
                for (val in data)
                    sortable.push([val, data[val]]);

                sortable.sort(function (a, b) {
                    if (a[1] < b[1])
                        return -1;
                    if (a[1] > b[1])
                        return 1;
                    return 0;
                });

                $vo_skyculturelist.empty();

                sortable.forEach(function (val) {
                    var option = document.createElement("option");
                    option.value = val[0];
                    option.textContent = val[1];
                    $vo_skyculturelist.append(option);
                });

            },
            error: function (xhr, status, errorThrown) {
                console.log("Error updating landscape list");
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve landscape list"));
            }
        });
    }

    //Public stuff
    return {
        init: function () {

            initControls();
            fillProjectionList();
            fillLandscapeList();
            fillSkycultureList();

        },

        updateFromServer: function (view) {
            if (!$selectedoption || $selectedoption[0].value !== view.projection) {
                if ($selectedoption) {
                    $selectedoption.removeClass("select_selected");
                }
                $vo_projectionlist.val(view.projection);

                $selectedoption = $('#vo_projectionlist option[value="' + view.projection + '"]');
                $selectedoption.addClass("select_selected");
            }

            if (currentLandscape !== view.landscape) {
                //this forces a reload of the iframe
                $("#vo_landscapeinfo").attr('src', function (i, val) {
                    return val;
                });
                currentLandscape = view.landscape;

                $vo_landscapelist.children(".select_selected").removeClass("select_selected");
                $vo_landscapelist.val(view.landscape);
                $vo_landscapelist.children("option[value='"+view.landscape+"']").addClass("select_selected");
            }
            if(currentSkyculture !== view.skyculture) {
                //this forces a reload of the iframe
                $("#vo_skycultureinfo").attr('src', function (i, val) {
                    return val;
                });
                currentSkyculture = view.skyculture;

                $vo_skyculturelist.children(".select_selected").removeClass("select_selected");
                $vo_skyculturelist.val(view.skyculture);
                $vo_skyculturelist.children("option[value='"+view.skyculture+"']").addClass("select_selected");
            }
        }
    };
})(jQuery);