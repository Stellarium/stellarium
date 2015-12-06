define(["jquery", "api/viewoptions", "api/actions"], function($, viewOptionApi, actionApi) {
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
	}

	function initControls() {
		$vo_landscapelist = $("#vo_landscapelist");
		$vo_skyculturelist = $("#vo_skyculturelist");
		actionApi.connectActionContainer($("#tab_view"));
		actionApi.connectActionContainer($("#tab_landscape"));
		actionApi.connectActionContainer($("#tab_skyculture"));
		$vo_projectionlist = $("#vo_projectionlist");
		$vo_projectionlist.dblclick(function() {
			if (this.selectedIndex >= 0) {
				var proj = this.options[this.selectedIndex].value;
				viewOptionApi.setProjection(proj);
			}
		});

		$vo_landscapelist.dblclick(function(evt, data) {
			if (this.selectedIndex >= 0) {
				var ls = this.options[this.selectedIndex].value;
				viewOptionApi.setLandscape(ls);
			}
		});

		$vo_skyculturelist.dblclick(function(evt, data) {
			if (this.selectedIndex >= 0) {
				var sc = this.options[this.selectedIndex].value;
				viewOptionApi.setSkyculture(sc);
			}
		});

		viewOptionApi.loadProjectionList(fillProjectionList);
		viewOptionApi.loadLandscapeList(fillLandscapeList);
		viewOptionApi.loadSkycultureList(fillSkycultureList);
	}


	$(viewOptionApi).on("projectionChanged", function(evt, proj) {
		//this forces a reload of the iframe
		$("#vo_projectioninfo").attr("src", function(i,val){
			return val;
		});

		$vo_projectionlist.children('option.select_selected').removeClass('select_selected');
		$vo_projectionlist.val(proj);
		$vo_projectionlist.children("option[value='" + proj + "']").addClass('select_selected');
	});

	$(viewOptionApi).on("landscapeChanged", function(evt, landscape) {
		//this forces a reload of the iframe
		$("#vo_landscapeinfo").attr('src', function(i, val) {
			return val;
		});

		$vo_landscapelist.children("option.select_selected").removeClass("select_selected");
		$vo_landscapelist.val(landscape);
		$vo_landscapelist.children("option[value='" + landscape + "']").addClass("select_selected");
	});

	$(viewOptionApi).on("skycultureChanged", function(evt, skyculture) {
		//this forces a reload of the iframe
		$("#vo_skycultureinfo").attr('src', function(i, val) {
			return val;
		});

		$vo_skyculturelist.children(".select_selected").removeClass("select_selected");
		$vo_skyculturelist.val(skyculture);
		$vo_skyculturelist.children("option[value='" + skyculture + "']").addClass("select_selected");
	});

	$(initControls);
});