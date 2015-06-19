"use strict";

var Search = (new function($) {
    //Private variables
    var $srch_input;
    var $srch_results;
    var $srch_button;

    var lastXHR;
    var $selectedElem;

    function initControls() {
        $srch_input = $("#srch_input");
        $srch_results = $("#srch_results");

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
        Main.postCmd("/api/main/focus", {
            target: $selectedElem.data("value")
        }, undefined, function(data) {
            console.log("focus change ok: " + data);
        });
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
                        alert("Error peforming search");
                    }
                }
            });
        }

    }

    function cancelSearch() {
        if (lastXHR) {
            lastXHR.abort();
            lastXHR = undefined;
        }
    }

    //Public stuff
    return {
        init: function() {

            initControls();

        }
    }
}(jQuery));