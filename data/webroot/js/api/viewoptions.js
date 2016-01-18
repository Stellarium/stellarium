define(["jquery", "./remotecontrol", "./flags", "./properties"], function($, rc, Flags, propApi) {
    "use strict";

    //Private variables
    var currentProjection;
    var currentLandscape;
    var currentSkyculture;

    var catalogFlags;
    var typeFlags;

    //register server data handlers
    $(rc).on("serverDataReceived", function(evt, data) {
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

    $(propApi).on("stelPropertyChanged:prop_NebulaMgr_catalogFilters", function(evt, data) {
        if (catalogFlags)
            catalogFlags.setValue(data.value);
    });

    $(propApi).on("stelPropertyChanged:prop_NebulaMgr_typeFilters", function(evt, data) {
        if (typeFlags)
            typeFlags.setValue(data.value);
    });


    function setDSOCatalog(flags) {
        propApi.setStelProp("prop_NebulaMgr_catalogFilters", flags);
    }

    function setDSOType(flags) {
        propApi.setStelProp("prop_NebulaMgr_typeFilters", flags);
    }

    //Public stuff
    var publ = {

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

        registerCatalogFlags: function(elem) {
            catalogFlags = new Flags(elem, setDSOCatalog);
            catalogFlags.setValue(propApi.getStelProp("prop_NebulaMgr_catalogFilters"));
        },

        registerTypeFlags: function(elem) {
            typeFlags = new Flags(elem, setDSOType);
            typeFlags.setValue(propApi.getStelProp("prop_NebulaMgr_typeFilters"));
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
        },

        setDSOCatalog: setDSOCatalog,
        setDSOType: setDSOType
    };

    return publ;

});