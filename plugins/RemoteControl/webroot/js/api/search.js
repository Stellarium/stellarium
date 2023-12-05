/* jshint expr: true */
define(["jquery", "./remotecontrol"], function($, rc) {
    "use strict";

    var simbadDelay = 500;

    var lastXHR;
    var simbadXHR;
    var listXHR;
    var simbadTimeout;

    var searchFinished = false;

    function selectObjectByName(str, mode) {
        rc.postCmd("/api/main/focus", {
            target: str,
            mode: mode
        }, undefined, function(data) {
            console.log("focus change ok: " + data);
        });
    }

    function performSimbadSearch(str, callback) {
        //check if the normal search has finished, otherwise we wait longer
        if (!searchFinished) {
            console.log(
                "defer Simbad search because normal search is not finished"
            );
            simbadTimeout = setTimeout(function() {
                performSimbadSearch(str, callback);
            }, simbadDelay);
            return;
        }

        changeSimbadState(rc.tr("querying"));

        simbadXHR = $.ajax({
            url: "/api/simbad/lookup",
            data: {
                str: str
            },
            success: function(data) {
                if (data.status === "error") {
                    changeSimbadState(rc.tr("Error (%1)",
                        data.errorString));
                } else {
                    changeSimbadState(data.status_i18n);
                }

                callback(data);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log(
                        "Error performing simbad lookup"
                    );
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(rc.tr(
                        "Error performing Simbad lookup"
                    ));
                }
            }
        });
    }

    function cancelSearch() {
        searchFinished = false;

        if (lastXHR) {
            lastXHR.abort();
            lastXHR = undefined;
        }
        if (simbadXHR) {
            simbadXHR.abort();
            simbadXHR = undefined;
        }
        simbadTimeout && clearTimeout(simbadTimeout);
        changeSimbadState(rc.tr("idle"));
    }

    function changeSimbadState(state) {
        $(publ).trigger("simbadStateChanged", state);
    }

    //Public stuff
    var publ = {
        loadObjectTypes: function(callback) {
            $.ajax({
                url: "/api/objects/listobjecttypes",
                dataType: "JSON",
                success: callback,
                error: function(xhr, status,
                    errorThrown) {
                    if (status !== "abort") {
                        console.log(
                            "Error loading object types"
                        );
                        console.log("Error: " +
                            errorThrown.message
                        );
                        console.log("Status: " +
                            status);
                        alert(rc.tr(
                            "Error loading object types"
                        ));
                    }
                }
            });
        },

        loadObjectList: function(objectType, useEnglish, callback) {
            if (listXHR) {
                listXHR.abort();
                listXHR = undefined;
            }

            listXHR = $.ajax({
                url: "/api/objects/listobjectsbytype",
                dataType: "JSON",
                data: {
                    type: objectType,
                    english: (useEnglish ? 1 : 0)
                },
                success: callback,
                error: function(xhr, status,
                    errorThrown) {
                    if (status !== "abort") {
                        console.log(
                            "Error getting object list"
                        );
                        console.log("Error: " +
                            errorThrown.message
                        );
                        console.log("Status: " +
                            status);
                        alert(rc.tr(
                            "Error getting object list"
                        ));
                    }
                }
            });
        },

        selectObjectByName: selectObjectByName,
        focusPosition: function(pos) {
            var posStr = JSON.stringify(pos);
            rc.postCmd("/api/main/focus", {
                position: posStr
            });
        },

        performSearch: function(str, resultCallback, simbadCallback) {
            //abort old search if it is running
            cancelSearch();
            str = str.trim().toLowerCase();

            if (!str) {
                return false;
            } else {
                //got search string, perform ajax

                lastXHR = $.ajax({
                    url: "/api/objects/find",
                    data: {
                        str: str
                    },
                    success: function(data) {
                        resultCallback(data);
                        searchFinished = true;
                    },
                    error: function(xhr, status,
                        errorThrown) {
                        if (status !== "abort") {
                            console.log(
                                "Error performing search"
                            );
                            console.log("Error: " +
                                errorThrown.message
                            );
                            console.log("Status: " +
                                status);

                            //cancel a pending simbad search
                            cancelSearch();

                            alert(rc.tr(
                                "Error performing search"
                            ));
                        }
                    }
                });

                //queue simbad search
                simbadTimeout = setTimeout(function() {
                    performSimbadSearch(str,
                        simbadCallback);
                }, simbadDelay);

                changeSimbadState(rc.tr("waiting"));
                return true;
            }

        }
    };
    return publ;
});
