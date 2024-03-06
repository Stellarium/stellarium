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

	function selectObjectByName(name) {
		searchApi.selectObjectByName(name,
			$select_SelectionMode.val());
	}

	function handleObjectListResults(data) {
		var parent = $srch_list_objectlist.parent();
		$srch_list_objectlist.detach();

		data.forEach(function(elem) {
			var op = document.createElement("option");
			op.innerHTML = elem;
			$srch_list_objectlist[0].appendChild(op);
		});

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

		$srch_tabs.tabs({
			heightStyle: "content"
		});

		$srch_list_objecttype.combobox({
			select: function(evt, data) {
				//empty the list before searching
				$srch_list_objectlist.empty();
				searchApi.loadObjectList(data.item.value, $srch_list_english[0].checked,
					handleObjectListResults);
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
