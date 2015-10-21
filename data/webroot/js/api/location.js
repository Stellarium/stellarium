/* jshint expr: true */

var Locations = (function($) {
    "use strict";

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

    var lastLat;
    var lastLon;
    var lastPlanet = "";

    var editMode;
    var queuedData = {};
    var updateQueue;

    var locSearchTimeout;
    var locSearchXHR;

    function initControls() {
        $loc_mapimg = $("#loc_mapimg");
        $loc_mapimg.click(locationFromMap);
        $loc_mappointer = $("#loc_mappointer");
        $loc_list = $("#loc_list");
        $loc_list.dblclick(setLocationFromList);
        $loc_search = $("#loc_search");
        $loc_search.on("input",function(evt){
            //queue a search if search string is at least 2 chars
            if(this.value.length >= 2)
            {
                locSearchTimeout && clearTimeout(locSearchTimeout);
                locSearchTimeout = setTimeout( $.proxy(function() { performLocationSearch(this.value); }, this) , UISettings.editUpdateDelay);
            }
            else
            {
                locSearchTimeout && clearTimeout(locSearchTimeout);
                //clear results to avoid confusion
                $loc_list.empty();
                //var op = document.createElement("option");
                //op.textContent = Main.tr("Need at least 2 characters");
                //$loc_list.append(op);
            }
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

        scope = "input.ui-spinner-input";
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

    function performLocationSearch(str) {
        //kill old XHR if running
        locSearchXHR && locSearchXHR.abort();

        locSearchXHR = $.ajax({
                url: "/api/locationsearch/search",
                data: {
                    term: str
                },
                success: handleSearchResults,
                error: function(xhr, status, errorThrown) {
                    if (status !== "abort") {
                        console.log("Error performing search");
                        console.log("Error: " + errorThrown.message);
                        console.log("Status: " + status);

                        alert(Main.tr("Error performing search"));
                    }
                }
        });
    }

    function performNearbySearch(lat,lon) {
        //kill old XHR if running (use the same one as the string-based search)
        locSearchXHR && locSearchXHR.abort();

        //replicate LocationDialog behaviour
        var radius = lastPlanet === "Earth" ? 3 : 30;

        locSearchXHR = $.ajax({
            url: "/api/locationsearch/nearby",
            data: {
                planet: lastPlanet,
                latitude: lat,
                longitude: lon,
                radius: radius
            },
            success: handleSearchResults,
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error performing search");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);

                    alert(Main.tr("Error performing search"));
                }
            }
        });
    }

    function handleSearchResults(data)
    {
        var parent = $loc_list.parent();
        $loc_list.detach();
        $loc_list.empty();

        for(var i = 0; i < data.length; ++i)
        {
            var op = document.createElement("option");
            op.textContent = data[i];
            $loc_list[0].appendChild(op);
        }

        parent.prepend($loc_list);
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

        //also, perform a search for locations near here
        performNearbySearch(lat,lon);
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
                }

                parent.append($loc_country);
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating country list");
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve country list"));
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
                }

                parent.append($loc_planet);
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating planet list");
                console.log("Error: " + errorThrown);
                console.log("Status: " + status);
                alert(Main.tr("Could not retrieve planet list"));
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
                    planetUrl = "images/world.png";
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
    };
})(jQuery);