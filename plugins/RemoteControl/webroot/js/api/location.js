/* jshint expr: true */
define(["jquery", "./remotecontrol", "./updatequeue"], function($, rc, UpdateQueue) {
    "use strict";

    var lastName;
    var lastRegion;
    var lastAltitude;
    var lastLat;
    var lastLon;
    var lastPlanet = "";

    var queuedData = {};
    var updateQueue = new UpdateQueue("/api/location/setlocationfields", serverUpdateFinished);

    var locSearchXHR;

    function performLocationSearch(str, callback) {
        //kill old XHR if running
        locSearchXHR && locSearchXHR.abort();

        locSearchXHR = $.ajax({
            url: "/api/locationsearch/search",
            data: {
                term: str
            },
            success: callback,
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error performing search");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);

                    alert(rc.tr("Error performing search"));
                }
            }
        });
    }

    function performNearbySearch(lat, lon, callback) {
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
            success: callback,
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error performing search");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);

                    alert(rc.tr("Error performing search"));
                }
            }
        });
    }

    function queueData(field, value) {
        queuedData[field] = value;
        updateQueue.enqueue(queuedData);
    }

    function serverUpdateFinished() {
        //reset queued data
        queuedData = {};
    }

    $(rc).on("serverDataReceived", function(evt, data) {
        var loc = data.location;

        if (lastPlanet !== loc.planet) {
            $(publ).trigger('planetChanged', loc.planet);
            lastPlanet = loc.planet;
        }
        if (lastName !== loc.name) {
            $(publ).trigger('nameChanged', loc.name);
            lastName = loc.name;
        }
	if (lastRegion !== loc.region) {
	    $(publ).trigger('regionChanged', loc.region);
	    lastRegion= loc.region;
        }
        if (lastAltitude !== loc.altitude) {
            $(publ).trigger('altitudeChanged', loc.altitude);
            lastAltitude = loc.altitude;
        }
        var posChanged = false;
        if (lastLat !== loc.latitude) {
            $(publ).trigger('latitudeChanged', loc.latitude);
            lastLat = loc.latitude;
            posChanged = true;
        }
        if (lastLon !== loc.longitude) {
            $(publ).trigger('longitudeChanged', loc.longitude);
            lastLon = loc.longitude;
            posChanged = true;
        }
        if (posChanged) {
            $(publ).trigger('positionChanged', [lastLat, lastLon]);
        }
    });

    //Public stuff
    var publ = {
        performNearbySearch: performNearbySearch,
        performLocationSearch: performLocationSearch,

        loadRegionList: function(callback) {
            $.ajax({
                url: "/api/location/regionlist",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating country list");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve country list"));
                }
            });
        },

        loadPlanetList: function(callback) {
            $.ajax({
                url: "/api/location/planetlist",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating planet list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve planet list"));
                }
            });
        },

        setRegion: function(region) {
            queueData("region", region);
        },

        setPlanet: function(planet) {
            queueData("planet", planet);
        },

        setName: function(name) {
            queueData("name", name);
        },

        setLatitude: function(lat) {
            queueData("latitude", lat);
        },

        setLongitude: function(lon) {
            queueData("longitude", lon);
        },

        setAltitude: function(alt) {
            queueData("altitude", alt);
        },

        setLocationById: function(id) {
            queueData("id", id);
        }
    };
    return publ;
});
