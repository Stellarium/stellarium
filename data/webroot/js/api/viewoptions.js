define(["jquery", "./remotecontrol"], function($, rc) {
    "use strict";

    //Private variables
    var currentProjection;
    var currentLandscape;
    var currentSkyculture;


    //register server data handlers
    $(rc).on("serverDataReceived", function(evt,data) {
        if (currentProjection !== data.view.projection) {
            currentProjection = data.view.projection;
            $(publ).trigger("projectionChanged", [currentProjection, data.view.projectionStr]);
        }

        if (currentLandscape !== data.view.landscape) {
            currentLandscape = data.view.landscape;
            $(publ).trigger("landscapeChanged", currentLandscape);
        }

        if (currentSkyculture !== data.view.skyculture) {
            currentSkyculture = data.view.skyculture;
            $(publ).trigger("skycultureChanged", currentSkyculture);
        }
    });

    //Public stuff
    var publ = {
        init: function() {

            initControls();
            fillProjectionList();
            fillLandscapeList();
            fillSkycultureList();

        },

        loadProjectionList: function(callback) {
            $.ajax({
                url: "/api/view/listprojection",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating projection list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve projection list"));
                }
            });
        },

        loadLandscapeList: function(callback) {
            $.ajax({
                url: "/api/view/listlandscape",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating landscape list");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve landscape list"));
                }
            });
        },

        loadSkycultureList: function(callback) {
            $.ajax({
                url: "/api/view/listskyculture",
                success: callback,
                error: function(xhr, status, errorThrown) {
                    console.log("Error updating landscape list");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(rc.tr("Could not retrieve landscape list"));
                }
            });
        },

        setProjection: function(proj) {
            rc.postCmd("/api/view/setprojection", {
                type: proj
            });
        },

        setLandscape: function(landscape) {
            rc.postCmd("/api/view/setlandscape", {
                id: landscape
            });
        },

        setSkyculture: function(skyculture) {
            rc.postCmd("/api/view/setskyculture", {
                id: skyculture
            });
        }
    };

    return publ;

});