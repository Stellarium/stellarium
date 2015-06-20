"use strict";

var Search = (new function($) {
    //Private variables
    var $srch_input;
    var $srch_results;
    var $srch_button;
    var $srch_simbad;

    var simbadDelay = 500;

    var lastXHR;
    var simbadXHR;
    var simbadTimeout;

    var searchFinished = false;

    var $selectedElem;

    function initControls() {
        $srch_input = $("#srch_input");
        $srch_results = $("#srch_results");
        $srch_simbad = $("#srch_simbad");

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
            Main.postCmd("/api/main/focus",{
                position: posStr
            });
        } else {
            //post by name
            Main.postCmd("/api/main/focus", {
                target: $selectedElem.data("value")
            }, undefined, function(data) {
                console.log("focus change ok: " + data);
            });
        }
        $srch_input[0].value = "";
        performSearch("");
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

    function handeSimbadResults(data) {
        if (data.status === "error") {
            $srch_simbad.text("Error (" + data.errorString + ")");
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

                if(lastResult.length === 0)
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
            $srch_simbad.text("idle");
        } else {
            //got search string, perform ajax

            lastXHR = $.ajax({
                url: "/api/search/find",
                data: {
                    str: str
                },
                success: handleSearchResults,
                error: function(xhr, status, errorThrown) {
                    if (status !== "abort") {
                        console.log("Error performing search");
                        console.log("Error: " + errorThrown);
                        console.log("Status: " + status);
                        alert("Error performing search");

                        //cancel a pending simbad search
                        cancelSearch();
                    }
                }
            });

            //queue simbad search
            simbadTimeout = setTimeout(function() {
                performSimbadSearch(str);
            }, simbadDelay);

            $srch_simbad.text("waiting");
        }

    }

    function performSimbadSearch(str) {
        //check if the normal search has finished, otherwise we wait longer
        if(!searchFinished){
                console.log("defer Simbad search because normal search is not finished");
                simbadTimeout = setTimeout(function(){
                    performSimbadSearch(str);
                }, simbadDelay);
                return;
        }

        $srch_simbad.text("querying");
        simbadXHR = $.ajax({
            url: "/api/search/simbad",
            data: {
                str: str
            },
            success: handeSimbadResults,
            error: function(xhr, status, errorThrown) {
                if (status !== "abort") {
                    console.log("Error performing simbad lookup");
                    console.log("Error: " + errorThrown);
                    console.log("Status: " + status);
                    alert("Error performing Simbad lookup");
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

    //Public stuff
    return {
        init: function() {

            initControls();

        }
    }
}(jQuery));