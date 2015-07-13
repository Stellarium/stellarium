/* jshint expr: true */

var Search = (function($) {
    "use strict";

    //Private variables
    var $srch_input;
    var $srch_results;
    var $srch_button;
    var $srch_simbad;
    var $srch_tabs;
    var $srch_list_objecttype;
    var $srch_list_objectlist;
    var $srch_list_english;

    var simbadDelay = 500;

    var lastXHR;
    var simbadXHR;
    var listXHR;
    var simbadTimeout;

    var searchFinished = false;

    var $selectedElem;

    var objectTypes;

    function initControls() {
        $srch_input = $("#srch_input");
        $srch_results = $("#srch_results");
        $srch_simbad = $("#srch_simbad");
        $srch_tabs = $("#srch_tabs");
        $srch_list_objecttype = $("#srch_list_objecttype");
        $srch_list_objectlist = $("#srch_list_objectlist");
        $srch_list_english = $("#srch_list_english");

        $srch_list_english.change(function(evt) {
            reloadObjectTypes();
            $srch_list_objectlist.empty();
        });

        $srch_list_objectlist.dblclick(function(evt) {
            var e = $srch_list_objectlist[0];
            if (e.selectedIndex >= 0) {
                var loc = e.options[e.selectedIndex].text;
                selectObjectByName(loc);
            }
        });

        $srch_tabs.tabs();

        $srch_list_objecttype.combobox({
            select: function(evt, data) {
                loadObjectList(data.item.value);
            }
        });

        $srch_input.on("input", function() {
            performSearch(this.value);
        }).keydown(function(e) {
            if (e.which == 9) {
                //use tab to switch to next search result
                selectNextResult();
                e.preventDefault();
            }
        });

        $("#srch_greek button").button();

        $("#srch_greek").on("click", "button", function() {
            $srch_input[0].value += this.textContent;
            performSearch($srch_input[0].value);
            $srch_input.focus();
        });

        $srch_button = $("#srch_button");
        $srch_button.button({
            text: false,
            disabled: true,
            icons: {
                primary: "ui-icon-search"
            }
        }).click(function(e) {
            submitObjectSelection();
            e.preventDefault();
        });
    }

    function submitObjectSelection() {
        var str = $selectedElem.data("value");

        //check if the selection is a simbad result
        var pos = $selectedElem.data("viewpos");

        if (pos) {
            var posStr = JSON.stringify(pos);
            Main.postCmd("/api/main/focus", {
                position: posStr
            });
        } else {
            //post by name
            selectObjectByName($selectedElem.data("value"));
        }
        $srch_input[0].value = "";
        performSearch("");
    }

    function selectObjectByName(str) {
        Main.postCmd("/api/main/focus", {
            target: str
        }, undefined, function(data) {
            console.log("focus change ok: " + data);
        });
    }

    function selectNextResult() {
        if ($selectedElem) {
            var idx = $selectedElem.index();
            selectIndex(idx + 1);
        } else {
            selectIndex(0);
        }
    }

    function selectIndex(idx) {
        if ($selectedElem) {
            $selectedElem.removeClass("state-selected");
            $selectedElem = undefined;
        }

        var res = $srch_results[0].children;

        if (res.length > 0) {
            if (idx >= res.length || idx < 0)
                idx = 0;

            $selectedElem = $(res[idx]).addClass("state-selected");
            $srch_button.button("enable");
        }
    }

    function handleSearchResults(data) {
        var parent = $srch_results.parent();
        $srch_results.detach();

        clearSearchResults();

        data.forEach(function(val, idx) {
            var li = document.createElement("span");
            li.textContent = val;
            if (idx < data.length - 1)
                li.textContent += ", ";
            $(li).data("value", val);
            li.className = "ui-menu-item";
            $srch_results[0].appendChild(li);
        });

        selectIndex(0);

        $srch_results.appendTo(parent);

        searchFinished = true;
    }

    function handleSimbadResults(data) {
        if (data.status === "error") {
            $srch_simbad.text(Main.tr("Error (%1)",data.errorString));
        } else {
            $srch_simbad.text(data.status_i18n);

            if (data.results.names.length > 0) {
                //find out if some results exist already, and append a comma to the last one
                var lastResult = $("#srch_results span:last");
                if (lastResult.length > 0) {
                    lastResult[0].textContent += ", ";
                }

                data.results.names.forEach(function(val, idx) {
                    var li = document.createElement("span");
                    li.textContent = val;
                    if (idx < data.results.names.length - 1)
                        li.textContent += ", ";
                    $(li).data("value", val);
                    $(li).data("viewpos", data.results.positions[idx]);
                    li.className = "ui-menu-item";
                    $srch_results[0].appendChild(li);
                });

                if (lastResult.length === 0)
                    selectIndex(0);
            }
        }
    }

    function clearSearchResults() {
        $srch_results.empty();
        $srch_button.button("disable");
        $selectedElem = undefined;
    }

    function performSearch(str) {
        //abort old search if it is running
        cancelSearch();

        str = str.trim().toLowerCase();

        if (!str) {
            //empty search string, clear results
            clearSearchResults();
            $srch_simbad.text(Main.tr("idle"));
        } else {
            //got search string, perform ajax

            lastXHR = $.ajax({
                url: "/api/objects/find",
                data: {
                    str: str
                },
                success: handleSearchResults,
                error: function(xhr, status, errorThrown) {
                    if (status !== "abort") {
                        console.log("Error performing search");
                        console.log("Error: " + errorThrown.message);
                        console.log("Status: " + status);
                        
                        //cancel a pending simbad search
                        cancelSearch();

                        alert(Main.tr("Error performing search"));
                    }
                }
            });

            //queue simbad search
            simbadTimeout = setTimeout(function() {
                performSimbadSearch(str);
            }, simbadDelay);

            $srch_simbad.text(Main.tr("waiting"));
        }

    }

    function performSimbadSearch(str) {
        //check if the normal search has finished, otherwise we wait longer
        if (!searchFinished) {
            console.log("defer Simbad search because normal search is not finished");
            simbadTimeout = setTimeout(function() {
                performSimbadSearch(str);
            }, simbadDelay);
            return;
        }

        $srch_simbad.text(Main.tr("querying"));
        simbadXHR = $.ajax({
            url: "/api/simbad/lookup",
            data: {
                str: str
            },
            success: handleSimbadResults,
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error performing simbad lookup");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(Main.tr("Error performing Simbad lookup"));
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
    }

    function reloadObjectTypes() {

        if (!objectTypes) //no data yet, cant load
            return;

        $srch_list_objecttype.combobox("autocomplete", "");

        var useEnglish = $srch_list_english[0].checked;

        //detach for performance
        var parent = $srch_list_objecttype.parent();
        $srch_list_objecttype.detach();
        $srch_list_objecttype.empty();

        //sort by name or localized name
        if (useEnglish) {
            objectTypes.sort(function(a, b) {
                if (a.name > b.name) {
                    return 1;
                }
                if (a.name < b.name) {
                    return -1;
                }
                // a must be equal to b
                return 0;
            });

        } else {
            objectTypes.sort(function(a, b) {
                if (a.name_i18n > b.name_i18n) {
                    return 1;
                }
                if (a.name_i18n < b.name_i18n) {
                    return -1;
                }
                // a must be equal to b
                return 0;
            });
        }

        objectTypes.forEach(function(element) {
            var op = document.createElement("option");
            op.innerHTML = useEnglish ? element.name : element.name_i18n;
            op.value = element.key;
            $srch_list_objecttype[0].appendChild(op);
        });

        parent.prepend($srch_list_objecttype);
    }

    function loadObjectTypes() {
        $.ajax({
            url: "/api/objects/listobjecttypes",
            dataType: "JSON",
            success: function(data) {

                objectTypes = data;
                reloadObjectTypes();

            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error loading object types");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(Main.tr("Error loading object types"));
                }
            }
        });
    }

    function loadObjectList(key) {
        if (listXHR) {
            listXHR.abort();
            listXHR = undefined;
        }

        $srch_list_objectlist.empty();

        var useEnglish = $srch_list_english[0].checked;

        listXHR = $.ajax({
            url: "/api/objects/listobjectsbytype",
            dataType: "JSON",
            data: {
                type: key,
                english: (useEnglish ? 1 : 0)
            },
            success: function(data) {

                var parent = $srch_list_objectlist.parent();
                $srch_list_objectlist.detach();

                data.forEach(function(elem) {
                    var op = document.createElement("option");
                    op.innerHTML = elem;
                    $srch_list_objectlist[0].appendChild(op);
                });

                $srch_list_objectlist.appendTo(parent);
            },
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error getting object list");
                    console.log("Error: " + errorThrown.message);
                    console.log("Status: " + status);
                    alert(Main.tr("Error getting object list"));
                }
            }
        });
    }

    //Public stuff
    return {
        init: function() {

            initControls();
            loadObjectTypes();
        }
    };
})(jQuery);