"use strict";

var ViewOptions = (new function ($) {
    //Private variables
    var $selectedoption;
    var $vo_landscapelist;
    var $vo_skyculturelist;
    var currentLandscape;
    var currentSkyculture;

    function initControls() {
        var tabs = $("#vo_tabs").tabs();
        $vo_landscapelist = $("#vo_landscapelist");
        $vo_skyculturelist = $("#vo_skyculturelist");
        Actions.connectActionContainer(tabs);
        $("#vo_projectionlist").dblclick(function () {
            if (this.selectedIndex >= 0) {
                var proj = this.options[this.selectedIndex].value;

                Main.postCmd("/api/view/setprojection", {
                    type: proj
                });
            }
        });

        $vo_landscapelist.combobox({
            select: function(evt,data){
                Main.postCmd("/api/view/setlandscape", {
                    id : data.item.value
                });
            }
        });

        $vo_skyculturelist.combobox({
            select: function(evt,data){
                Main.postCmd("/api/view/setskyculture", {
                    id : data.item.value
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
                };

                $projectionlist.appendTo(parent);

            },
            error: function (xhr, status, errorThrown) {
                console.log("Error updating projection list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve projection list")
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
                alert("Could not retrieve landscape list")
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
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve landscape list")
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
                    $selectedoption.removeClass("italic");
                }

                $selectedoption = $('#vo_projectionlist option[value="' + view.projection + '"]');
                $selectedoption.addClass("italic");
            }

            if (currentLandscape !== view.landscape) {
                //this forces a reload of the iframe
                $("#vo_landscapeinfo").attr('src', function (i, val) {
                    return val;
                });
                currentLandscape = view.landscape;

                $("#vo_landscapelist").combobox("autocomplete",view.landscape);
            }
            if(currentSkyculture !== view.skyculture) {       
                //this forces a reload of the iframe
                $("#vo_skycultureinfo").attr('src', function (i, val) {
                    return val;
                });
                currentSkyculture = view.skyculture;

                $("#vo_skyculturelist").combobox("autocomplete",view.skyculture);
            }
        }
    }
}(jQuery));