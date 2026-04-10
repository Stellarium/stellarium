define(["jquery", "api/remotecontrol", "api/search", "./combobox"], function($, rc, searchApi) {
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
    var $select_SelectionMode;
    var $select_clearButton;
    var sel_infostring;

    var $selectedElem;

    var objectTypes;

    // NEW: Store current selected object type for refresh
    var currentSelectedType = null;

    function fillObjectTypes(data) {
        objectTypes = data;
        reloadObjectTypes();
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

        // Add standard object types
        if (useEnglish) {
            objectTypes.sort(function(a, b) {
                if (a.name > b.name) {
                    return 1;
                }
                if (a.name < b.name) {
                    return -1;
                }
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

        // Add culture-specific options to the dropdown
        addCultureSpecificOptions();
    }

    // Add culture-specific object types to the dropdown
    function addCultureSpecificOptions() {
        var cultureTypes = [
            { key: "CULTURE_CONSTELLATIONS", name: _tr("Culture Constellations") },
            { key: "CULTURE_ASTERISMS", name: _tr("Culture Asterisms") },
            { key: "CULTURE_ZODIAC", name: _tr("Culture Zodiac Signs") },
            { key: "CULTURE_LUNAR", name: _tr("Culture Lunar Mansions") },
            { key: "CULTURE_STARS", name: _tr("Culture Notable Stars") }
        ];

        var useEnglish = $srch_list_english[0].checked;

        for (var i = 0; i < cultureTypes.length; i++) {
            var op = document.createElement("option");
            op.innerHTML = useEnglish ? cultureTypes[i].name : cultureTypes[i].name;
            op.value = cultureTypes[i].key;
            $srch_list_objecttype[0].appendChild(op);
        }
    }

    function selectObjectByName(name) {
        searchApi.selectObjectByName(name,
            $select_SelectionMode.val());
    }

    function handleObjectListResults(data) {
        var parent = $srch_list_objectlist.parent();
        $srch_list_objectlist.detach();
        $srch_list_objectlist.empty();

        data.forEach(function(elem) {
            var op = document.createElement("option");
            op.innerHTML = elem;
            $srch_list_objectlist[0].appendChild(op);
        });

        $srch_list_objectlist.appendTo(parent);
    }

    // Handle culture-specific list results
    function handleCultureListResults(data, type) {
        var parent = $srch_list_objectlist.parent();
        $srch_list_objectlist.detach();
        $srch_list_objectlist.empty();

        if (data && data.length > 0) {
            data.forEach(function(elem) {
                var op = document.createElement("option");
                op.innerHTML = elem;
                $srch_list_objectlist[0].appendChild(op);
            });
        } else {
            var placeholder = document.createElement("option");
            placeholder.innerHTML = rc.tr("No items available for this culture");
            placeholder.disabled = true;
            $srch_list_objectlist[0].appendChild(placeholder);
        }

        $srch_list_objectlist.appendTo(parent);
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

    function submitObjectSelection() {
        var str = $selectedElem.data("value");

        //check if the selection is a simbad result
        var pos = $selectedElem.data("viewpos");

        if (pos) {
            searchApi.focusPosition(pos);
        } else {
            //post by name
            selectObjectByName(str);
        }
        $srch_input[0].value = "";
        startSearch("");
    }


    function clearSearchResults() {
        $srch_results.empty();
        $srch_button.button("disable");
        $selectedElem = undefined;
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

    function handleSimbadResults(data) {
        if (data.status !== "error") {
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

    function startSearch(str) {

        if (!searchApi.performSearch(str, handleSearchResults, handleSimbadResults)) {
            //search not executed, clear results
            clearSearchResults();
        }
    }

    // Handle culture-specific type selection
    function onCultureTypeSelected(typeKey, useEnglish) {
        $srch_list_objectlist.empty();
        currentSelectedType = typeKey;
        
        switch(typeKey) {
            case "CULTURE_CONSTELLATIONS":
                searchApi.loadCultureConstellations(useEnglish, function(data) {
                    handleCultureListResults(data, "constellations");
                });
                break;
            case "CULTURE_ASTERISMS":
                searchApi.loadCultureAsterisms(useEnglish, function(data) {
                    handleCultureListResults(data, "asterisms");
                });
                break;
            case "CULTURE_ZODIAC":
                searchApi.loadCultureZodiac(useEnglish, function(data) {
                    handleCultureListResults(data, "zodiac");
                });
                break;
            case "CULTURE_LUNAR":
                searchApi.loadCultureLunarMansions(useEnglish, function(data) {
                    handleCultureListResults(data, "lunar");
                });
                break;
            case "CULTURE_STARS":
                searchApi.loadCultureStars(useEnglish, function(data) {
                    handleCultureListResults(data, "stars");
                });
                break;
            default:
                // Standard object type
                searchApi.loadObjectList(typeKey, useEnglish, handleObjectListResults);
        }
    }

    function initControls() {
        $srch_input = $("#srch_input");
        $srch_results = $("#srch_results");
        $srch_simbad = $("#srch_simbad");
        $srch_tabs = $("#srch_tabs");
        $srch_list_objecttype = $("#srch_list_objecttype");
        $srch_list_objectlist = $("#srch_list_objectlist");
        $srch_list_english = $("#srch_list_english");
        $select_SelectionMode = $("#select_SelectionMode");
        $select_clearButton = $("#select_clearButton");
        sel_infostring = document.getElementById("sel_infostring");

				/**
				 * Handles language toggle (English / localized names) for the object list.
				 * 
				 * When the user checks/unchecks the "names in English" checkbox, this function:
				 * 1. Preserves the currently selected object type (e.g., "StarMgr", "ConstellationMgr")
				 * 2. Rebuilds the object types dropdown with names in the new language
				 * 3. Restores the previously selected type using the combobox setValue method
				 * 4. Reloads the object list for the same type but with the new language setting
				 * 
				 * This ensures that toggling the language does NOT reset the selection or clear the list,
				 * providing a smooth user experience without losing context.
				 * 
				 * @param {Event} evt - The change event from the checkbox
				 */
				$srch_list_english.change(function(evt) {
						// Store the currently selected value before any DOM changes
						var currentVal = $srch_list_objecttype.val();
						var useEnglish = $srch_list_english[0].checked;
						
						// Rebuild the object types dropdown with names in the new language
						// This will reset the selection to the first option
						reloadObjectTypes();
						
						// Restore the previously selected value using the combobox API
						if (currentVal) {
								$srch_list_objecttype.combobox("setValue", currentVal);
						}
						
						// Reload the object list for the same type with the new language
						if (currentVal) {
								if (currentVal.indexOf("CULTURE_") === 0) {
										// For culture-specific types (e.g., CULTURE_CONSTELLATIONS)
										onCultureTypeSelected(currentVal, useEnglish);
								} else {
										// For standard Stellarium types (e.g., StarMgr, ConstellationMgr)
										$srch_list_objectlist.empty();
										searchApi.loadObjectList(currentVal, useEnglish, handleObjectListResults);
								}
						}
				});

        $srch_list_objectlist.dblclick(function(evt) {
            var e = $srch_list_objectlist[0];
            if (e.selectedIndex >= 0) {
                var loc = e.options[e.selectedIndex].text;
                selectObjectByName(loc);
            }
        });

        $("#srch_button button").click(function(evt) {
            var e = $srch_list_objectlist[0];
            if (e.selectedIndex >= 0) {
                var loc = e.options[e.selectedIndex].text;
                console.log("selecting " + loc);
                selectObjectByName(loc);
            }
        });

        $srch_tabs.tabs({
            heightStyle: "content"
        });

        $srch_list_objecttype.combobox({
            select: function(evt, data) {
                var selectedValue = data.item.value;
                var useEnglish = $srch_list_english[0].checked;
                currentSelectedType = selectedValue;
                
                if (selectedValue && selectedValue.indexOf("CULTURE_") === 0) {
                    onCultureTypeSelected(selectedValue, useEnglish);
                } else {
                    //empty the list before searching
                    $srch_list_objectlist.empty();
                    searchApi.loadObjectList(selectedValue, useEnglish, handleObjectListResults);
                }
            }
        });

        $srch_input.on("input", function() {
            startSearch(this.value);
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
            startSearch($srch_input[0].value);
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

        searchApi.loadObjectTypes(fillObjectTypes);

        //setup quick select buttons
        $("#quickselect").on("click", "button", function(evt) {
            console.log("selecting " + evt.target.value);
            selectObjectByName(evt.target.value);
        });

        $("#selection button").click(function(evt) {
            console.log("selecting " + evt.target.value);
            selectObjectByName(evt.target.value);
        });
    }

    // Translation helper
    function _tr(text) {
        if (typeof window.tr === 'function') {
            return window.tr.apply(window, arguments);
        }
        if (typeof rc !== 'undefined' && rc && typeof rc.tr === 'function') {
            return rc.tr.apply(rc, arguments);
        }
        return text;
    }

    $(rc).on("serverDataReceived", function(evt, data) {
        if (data.selectioninfo) {
            sel_infostring.innerHTML = data.selectioninfo;
            sel_infostring.className = "";
            $select_clearButton.show();
        } else {
            sel_infostring.innerHTML = rc.tr("No current selection");
            sel_infostring.className = "bold";
            $select_clearButton.hide();
        }
    });

    $(searchApi).on("simbadStateChanged", function(evt, state) {
        $srch_simbad.text(state);
    });

    $(initControls);
});