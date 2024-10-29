define(["jquery", "./remotecontrol", "./flags", "./properties"], function($, rc, Flags, propApi) {
    "use strict";

    //Private variables
    var catalogFlags;
    var typeFlags;

    $(propApi).on("stelPropertyChanged:NebulaMgr.catalogFilters", function(evt, data) {
        if (catalogFlags)
            catalogFlags.setValue(data.value);
    });

    $(propApi).on("stelPropertyChanged:NebulaMgr.typeFilters", function(evt, data) {
        if (typeFlags)
            typeFlags.setValue(data.value);
    });


    function setDSOCatalog(flags) {
        propApi.setStelProp("NebulaMgr.catalogFilters", flags);
    }

    function setDSOType(flags) {
        propApi.setStelProp("NebulaMgr.typeFilters", flags);
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
            catalogFlags.setValue(propApi.getStelProp("NebulaMgr.catalogFilters"));
        },

        registerTypeFlags: function(elem) {
            typeFlags = new Flags(elem, setDSOType);
            typeFlags.setValue(propApi.getStelProp("NebulaMgr.typeFilters"));
        },

        setDSOCatalog: setDSOCatalog,
        setDSOType: setDSOType
    };

    return publ;

});