"use strict";

var Locations = (new function($) {
    //Private variables
    var $loc_mapimg;
    var $loc_mappointer;
    var $loc_list;
    var $loc_search;

    var $loc_latitude;
    var $loc_longitude;
    var $loc_altitude;
    var $loc_name;
    var $loc_country;
    var $loc_planet;

    var locationList;
    var lastLat;
    var lastLon;
    var lastPlanet = "";

    var editMode;
    var queuedData = {};
    var updateQueue;

    function initControls() {
        $loc_mapimg = $("#loc_mapimg");
        $loc_mapimg.click(locationFromMap);
        $loc_mappointer = $("#loc_mappointer");
        $loc_list = $("#loc_list");
        $loc_list.dblclick(setLocationFromList);
        $loc_search = $("#loc_search");

        $loc_search.autocomplete({
            delay: 500
        });

        $loc_latitude = $("#loc_latitude");
        $loc_longitude = $("#loc_longitude");
        $loc_altitude = $("#loc_altitude");
        $loc_name = $("#loc_name");
        $loc_country = $("#loc_country");
        $loc_planet = $("#loc_planet");

        $("#loc_latitude, #loc_longitude").spinner({
            step: 0.00001,
            incremental: function(i){
                return Math.floor(i*i*i/100 - i*i / 10 + i + 1);
            }
        });
        $("#loc_latitude").spinner("option",{
            min: -90,
            max: 90
        });
        $("#loc_longitude").spinner("option", {
            min: -180,
            max: 180
        });

        $loc_altitude.spinner({
            step: 1
        }).data("field", "altitude");
        $loc_latitude.data("field", "latitude");
        $loc_longitude.data("field", "longitude");

        //edit mode setup
        var scope = "input";
        $("#loc_inputs").on("focusin", scope, function(evt) {
            console.log("edit mode on");
            editMode = true;
        }).on("focusout", scope, function(evt) {
            console.log("edit mode off");
            editMode = false;
        });

        scope = "input.ui-spinner-input"
        $("#loc_inputs").on("spin", scope, function(evt, ui) {
            var field = $(this).data("field");
            queueData(field,ui.value);
        });

        $loc_name.change(function(evt) {
            queueData("name",$loc_name.val());
        });

        $loc_country.combobox({
            select: function(evt, data) {
                queueData("country", data.item.value);
            }
        });
        $loc_planet.combobox({
            select: function(ev, data) {
                console.log("Selected: " + data.item.value);
                queueData("planet", data.item.value);
            }
        });
    }

    function setLocationFromList() {
        var e = $loc_list[0];
        if(e.selectedIndex>=0){
            var loc = e.options[e.selectedIndex].text;
            queueData("id",loc);
        }
    }

    function locationFromMap(e) {
        var $this = $(this);
        var x = e.pageX - $this.offset().left,
            y = e.pageY - $this.offset().top;

        var lon = x / $this.width() * 360 - 180;
        var lat = 90 - y / $this.height() * 180;

        $loc_latitude.spinner("value", lat);
        $loc_longitude.spinner("value", lon);

        queueData("latitude", lat);
        queueData("longitude", lon);
    }

    function queueData(field, value) {
        queuedData[field] = value;
        updateQueue.enqueue(queuedData);

        if(field === "latitude" || field === "longitude") {
            var lat = queueData.latitude || $loc_latitude.spinner("value");
            var lon = queueData.longitude || $loc_longitude.spinner("value");

            $loc_latitude.spinner("value",lat);
            $loc_longitude.spinner("value",lon);
            movePointer(lat, lon);
        }
    }

    function updateLocationList() {
        $.ajax({
            url: "/api/location/list",
            success: function(data) {
                locationList = data;

                //detach for performance
                var parent = $loc_list.parent();
                $loc_list.detach();

                //dont use jquery here for better performance
                //it is quite the large list!
                for (var i = 0; i < data.length; i++) {
                    var op = document.createElement("option");
                    op.innerHTML = data[i];
                    $loc_list[0].appendChild(op);
                };

                parent.prepend($loc_list);

                $loc_search.autocomplete("option", "source", locationList);

            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating location list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve location list");
            }
        });
    }

    function localizedSort(a, b) {
        if (a.name_i18n > b.name_i18n) {
            return 1;
        }
        if (a.name_i18n < b.name_i18n) {
            return -1;
        }
        // a must be equal to b
        return 0;
    }

    function updateCountryList() {
        $.ajax({
            url: "/api/location/countrylist",
            success: function(data) {

                //detach for performance
                var parent = $loc_country.parent();
                $loc_country.detach();

                //sort by localized text
                data.sort(localizedSort);

                for (var i = 0; i < data.length; i++) {
                    var op = document.createElement("option");
                    op.innerHTML = data[i].name_i18n; //translated
                    op.value = data[i].name;
                    $loc_country[0].appendChild(op);
                };

                parent.append($loc_country);
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating country list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve country list");
            }
        });
    }

    function updatePlanetList() {
        $.ajax({
            url: "/api/location/planetlist",
            success: function(data) {

                //detach for performance
                var parent = $loc_planet.parent();
                $loc_planet.detach();

                //sort by localized text
                data.sort(localizedSort);

                for (var i = 0; i < data.length; i++) {
                    var op = document.createElement("option");
                    op.innerHTML = data[i].name_i18n; //translated
                    op.value = data[i].name;
                    $loc_planet[0].appendChild(op);
                };

                parent.append($loc_planet);
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating planet list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert("Could not retrieve planet list");
            }
        });
    }

    function serverUpdateFinished() {
        //reset queued data
        queuedData = {};
    }

    function movePointer(latitude, longitude) {
        var x = Math.trunc((longitude + 180) / 360 * $loc_mapimg.width());
        var y = Math.trunc((latitude - 90) / -180 * $loc_mapimg.height());

        var top = y - $loc_mappointer.height() / 2;
        var left = x - $loc_mappointer.width() / 2;

        $loc_mappointer.css({
            "top": top,
            "left": left
        });
    }

    //Public stuff
    return {
        init: function() {
            //init location data
            updateLocationList();
            updateCountryList();
            updatePlanetList();

            updateQueue = new Main.UpdateQueue("/api/location/setlocationfields", serverUpdateFinished);

            initControls();
        },

        updateFromServer: function(loc) {
            if (editMode || updateQueue.isQueued)
                return; //dont update when editing or updates wait

            $loc_name.val(loc.name);

            $loc_country.combobox("autocomplete", loc.country);
            $loc_planet.combobox("autocomplete", loc.planet);
            $loc_latitude.spinner("value", loc.latitude);
            $loc_longitude.spinner("value", loc.longitude);
            $loc_altitude.spinner("value", loc.altitude);

            if (lastPlanet !== loc.planet) {
                var planetUrl = "/api/location/planetimage?planet=" + encodeURIComponent(loc.planet);
                if (loc.planet === "Earth")
                    planetUrl = "world.png";
                console.log("updating planet map image: " + planetUrl);

                $loc_mapimg.attr("src", planetUrl);

                lastPlanet = loc.planet;
            }

            if (lastLat !== loc.latitude || lastLon !== loc.longitude) {
                lastLat = loc.latitude;
                lastLon = loc.longitude;
                movePointer(loc.latitude, loc.longitude);
            }
        }
    }
}(jQuery));