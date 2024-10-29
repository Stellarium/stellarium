define(["jquery", "api/remotecontrol", "api/viewoptions", "api/actions", "api/properties", "jquery-ui"], function($, rc, viewOptionApi, actionApi, propApi) {
	"use strict";

	var $vo_projectionlist;
	var $vo_landscapelist;
	var $vo_skyculturelist;

	function fillProjectionList(data) {
		var $projectionlist = $("#vo_projectionlist");
		$projectionlist.empty();
		var prev = $projectionlist.prev();
		$projectionlist.detach();

		for (var val in data) {
			var option = document.createElement("option");
			option.value = val;
			option.textContent = data[val];
			$projectionlist.append(option);
		}

		prev.after($projectionlist);

		var curSel = $projectionlist.data("currentSelection");
		if(curSel)
			$projectionlist.val(curSel);
	}

	function fillLandscapeList(data) {
		var sortable = [];
		var val;
		for (val in data)
			sortable.push([val, data[val]]);

		sortable.sort(function(a, b) {
			if (a[1] < b[1])
				return -1;
			if (a[1] > b[1])
				return 1;
			return 0;
		});

		$vo_landscapelist.empty();

		sortable.forEach(function(val) {
			var option = document.createElement("option");
			option.value = val[0];
			option.textContent = val[1];
			$vo_landscapelist.append(option);
		});

		var curSel = $vo_landscapelist.data("currentSelection");
		if(curSel)
			$vo_landscapelist.val(curSel);
	}

	function fillSkycultureList(data) {
		var sortable = [];
		var val;
		for (val in data)
			sortable.push([val, data[val]]);

		sortable.sort(function(a, b) {
			if (a[1] < b[1])
				return -1;
			if (a[1] > b[1])
				return 1;
			return 0;
		});

		$vo_skyculturelist.empty();

		sortable.forEach(function(val) {
			var option = document.createElement("option");
			option.value = val[0];
			option.textContent = val[1];
			$vo_skyculturelist.append(option);
		});

		var curSel = $vo_skyculturelist.data("currentSelection");
		if(curSel)
			$vo_skyculturelist.val(curSel);
	}

	function initControls() {
		$vo_landscapelist = $("#vo_landscapelist");
		$vo_skyculturelist = $("#vo_skyculturelist");
		$vo_projectionlist = $("#vo_projectionlist");

		viewOptionApi.loadProjectionList(fillProjectionList);
		viewOptionApi.loadLandscapeList(fillLandscapeList);
		viewOptionApi.loadSkycultureList(fillSkycultureList);

		viewOptionApi.registerCatalogFlags($("#vo_dsocatalog"));
		viewOptionApi.registerTypeFlags($("#vo_dsotype > div")); //needs a stricter selector to prevent capturing the header checkbox

		$(actionApi).on("stelActionChanged:actionShow_LightPollutionFromDatabase", function(evt, data) {
			$("#atmosphere_bortlescaleindex").spinner("option", "disabled", data.isChecked);
		});

		$(propApi).on("stelPropertyChanged:LandscapeMgr.flagLandscapeUseMinimalBrightness", function(evt, data) {
			$("#landscape_defaultMinimalBrightness").spinner("option", "disabled", !data.value);
			$("#landscape_flagLandscapeSetsMinimalBrightness").prop("disabled", !data.value);
		});

	}

	$(propApi).on("stelPropertyChanged:LandscapeMgr.currentLandscapeID", function(evt,data){
		//reload iframe
		$("#vo_landscapeinfo").attr("src", function(i, val) {
			return val;
		});
	});

	$(propApi).on("stelPropertyChanged:StelSkyCultureMgr.currentSkyCultureID", function(evt,data){
		//this forces a reload of the iframe
		$("#vo_skycultureinfo").attr('src', function(i, val) {
			return val;
		});
	});

	$(propApi).on("stelPropertyChanged:StelCore.currentProjectionType", function(evt,data){
		//this forces a reload of the iframe
		$("#vo_projectioninfo").attr('src', function(i, val) {
			return val;
		});
	});

	$(actionApi).on("stelActionChanged:actionSet_Nebula_TypeFilterUsage", function(evt, data) {
		$("#vo_dsotype > div input[type='checkbox']").prop("disabled", !data.isChecked);
	});

	$(rc).on("uiReady",initControls);
});
