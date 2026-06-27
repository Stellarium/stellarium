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

    // =====================================================================
    // NEW FUNCTIONS FOR CULTURE-SPECIFIC DATA
    // =====================================================================

    /**
     * Loads constellations from the current sky culture's index.json
     * This returns culture-specific constellations (e.g., 51 for Arabic Al-Sufi)
     * @param {boolean} useEnglish - Whether to return English names or native names
     * @param {function} callback - Callback function with the results
     */
    function loadCultureConstellations(useEnglish, callback) {
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            dataType: "JSON",
            success: function(data) {
                var constellations = [];
                if (data.constellations && Array.isArray(data.constellations)) {
                    for (var i = 0; i < data.constellations.length; i++) {
                        var cons = data.constellations[i];
                        var name = null;
                        if (cons.common_name) {
                            name = useEnglish ? (cons.common_name.english || cons.common_name.native) : (cons.common_name.native || cons.common_name.english);
                        }
                        if (!name && cons.id) {
                            name = cons.id;
                        }
                        if (name) {
                            constellations.push(name);
                        }
                    }
                }
                constellations.sort();
                callback(constellations);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading culture constellations");
                    console.log("Error: " + errorThrown.message);
                    callback([]);
                }
            }
        });
    }

    /**
     * Loads asterisms from the current sky culture's index.json
     * @param {boolean} useEnglish - Whether to return English names or native names
     * @param {function} callback - Callback function with the results
     */
    function loadCultureAsterisms(useEnglish, callback) {
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            dataType: "JSON",
            success: function(data) {
                var asterisms = [];
                if (data.asterisms && Array.isArray(data.asterisms)) {
                    for (var i = 0; i < data.asterisms.length; i++) {
                        var aster = data.asterisms[i];
                        if (aster.is_ray_helper === true) continue;
                        var name = null;
                        if (aster.common_name) {
                            name = useEnglish ? (aster.common_name.english || aster.common_name.native) : (aster.common_name.native || aster.common_name.english);
                        }
                        if (name) {
                            asterisms.push(name);
                        }
                    }
                }
                asterisms.sort();
                callback(asterisms);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading culture asterisms");
                    callback([]);
                }
            }
        });
    }

    /**
     * Loads zodiac signs from the current sky culture's index.json
     * @param {boolean} useEnglish - Whether to return English names or native names
     * @param {function} callback - Callback function with the results
     */
    function loadCultureZodiac(useEnglish, callback) {
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            dataType: "JSON",
            success: function(data) {
                var zodiac = [];
                if (data.zodiac && data.zodiac.names && Array.isArray(data.zodiac.names)) {
                    for (var i = 0; i < data.zodiac.names.length; i++) {
                        var sign = data.zodiac.names[i];
                        var name = useEnglish ? (sign.english || sign.native) : (sign.native || sign.english);
                        if (name) {
                            zodiac.push(name);
                        }
                    }
                }
                callback(zodiac);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading culture zodiac");
                    callback([]);
                }
            }
        });
    }

    /**
     * Loads lunar mansions from the current sky culture's index.json
     * @param {boolean} useEnglish - Whether to return English names or native names
     * @param {function} callback - Callback function with the results
     */
    function loadCultureLunarMansions(useEnglish, callback) {
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            dataType: "JSON",
            success: function(data) {
                var lunar = [];
                if (data.lunar_system && data.lunar_system.names && Array.isArray(data.lunar_system.names)) {
                    for (var i = 0; i < data.lunar_system.names.length; i++) {
                        var mansion = data.lunar_system.names[i];
                        var name = useEnglish ? (mansion.english || mansion.native) : (mansion.native || mansion.english);
                        if (name) {
                            lunar.push(name);
                        }
                    }
                }
                callback(lunar);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading culture lunar mansions");
                    callback([]);
                }
            }
        });
    }

    /**
     * Loads notable stars from the current sky culture's index.json
     * @param {boolean} useEnglish - Whether to return English names or native names
     * @param {function} callback - Callback function with the results
     */
    function loadCultureStars(useEnglish, callback) {
        $.ajax({
            url: "/api/view/skyculturedescription/index.json",
            dataType: "JSON",
            success: function(data) {
                var stars = [];
                if (data.common_names && typeof data.common_names === 'object') {
                    var processedNames = {};
                    for (var hipId in data.common_names) {
                        if (data.common_names.hasOwnProperty(hipId)) {
                            var entries = data.common_names[hipId];
                            if (entries && Array.isArray(entries)) {
                                for (var i = 0; i < entries.length; i++) {
                                    var entry = entries[i];
                                    var name = useEnglish ? (entry.english || entry.native) : (entry.native || entry.english);
                                    if (name && !processedNames[name]) {
                                        processedNames[name] = true;
                                        stars.push(name);
                                    }
                                }
                            }
                        }
                    }
                }
                stars.sort();
                // Limit to first 200 stars
                //stars = stars.slice(0, 200);
								stars = stars.slice(0, 3200); // use this value to ensure 3072 chinese sky culture stars count
                callback(stars);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading culture stars");
                    callback([]);
                }
            }
        });
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

        // NEW: Culture-specific data loaders
        loadCultureConstellations: loadCultureConstellations,
        loadCultureAsterisms: loadCultureAsterisms,
        loadCultureZodiac: loadCultureZodiac,
        loadCultureLunarMansions: loadCultureLunarMansions,
        loadCultureStars: loadCultureStars,

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