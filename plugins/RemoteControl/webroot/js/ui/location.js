/* jshint expr: true */
define(["jquery", "api/location", "settings", "api/trunc", "./combobox"], function($, locationApi, settings) {
	"use strict";

	//Private variables
	var $loc_mapimg;
	var $loc_mappointer;
	var $loc_list;
	var $loc_search;

	var $loc_latitude;
	var $loc_longitude;
	var $loc_altitude;
	var $loc_name;
	var $loc_region;
	var $loc_planet;

	var locSearchTimeout;

	//convert x/y coords from map to lat/lon and queue an update
	function locationFromMap(e) {
		var $this = $(this);
		var x = e.pageX - $this.offset().left,
			y = e.pageY - $this.offset().top;

		var lon = x / $this.width() * 360 - 180;
		var lat = 90 - y / $this.height() * 180;

		$loc_latitude.spinner("value", lat);
		$loc_longitude.spinner("value", lon);

		locationApi.setLatitude(lat);
		locationApi.setLongitude(lon);

		$loc_latitude.spinner("value", lat);
		$loc_longitude.spinner("value", lon);
		movePointer(lat, lon);

		//also, perform a search for locations near here
		locationApi.performNearbySearch(lat, lon, handleSearchResults);
	}

	//moves the pointer to the specified lat/lon, no side effects
	function movePointer(latitude, longitude) {
		var x = Math.trunc((longitude + 180) / 360 * $loc_mapimg.width());
		var y = Math.trunc((latitude - 90) / -180 * $loc_mapimg.height());

		var top = y - $loc_mappointer.height() / 2;
		var left = x - $loc_mappointer.width() / 2;

		$loc_mappointer.css({
			"top": top,
			"left": left
		});
	}

	function setLocationFromList() {
		var e = $loc_list[0];
		if (e.selectedIndex >= 0) {
			var loc = e.options[e.selectedIndex].text;
			locationApi.setLocationById(loc);
		}
	}

	function handleSearchResults(data) {
		var parent = $loc_list.parent();
		$loc_list.detach();
		$loc_list.empty();

		for (var i = 0; i < data.length; ++i) {
			var op = document.createElement("option");
			op.textContent = data[i];
			$loc_list[0].appendChild(op);
		}

		parent.prepend($loc_list);
	}

	function localizedSort(a, b) {
		if (a.name_i18n > b.name_i18n) {
			return 1;
		}
		if (a.name_i18n < b.name_i18n) {
			return -1;
		}
		// a must be equal to b
		return 0;
	}

	function fillRegionList(data) {

		//detach for performance
		var parent = $loc_region.parent();
		$loc_region.detach();

		//sort by localized text
		data.sort(localizedSort);

		for (var i = 0; i < data.length; i++) {
			var op = document.createElement("option");
			op.innerHTML = data[i].name_i18n; //translated
			op.value = data[i].name;
		        $loc_region[0].appendChild(op);
		}

		parent.append($loc_region);
	}

	function fillPlanetList(data) {
		//detach for performance
		var parent = $loc_planet.parent();
		$loc_planet.detach();

		//sort by localized text
		data.sort(localizedSort);

		for (var i = 0; i < data.length; i++) {
			var op = document.createElement("option");
			op.innerHTML = data[i].name_i18n; //translated
			op.value = data[i].name;
			$loc_planet[0].appendChild(op);
		}

		parent.append($loc_planet);
	}

	function initControls() {
		$loc_mapimg = $("#loc_mapimg");
		$loc_mapimg.click(locationFromMap);
		$loc_mappointer = $("#loc_mappointer");
		$loc_list = $("#loc_list");
		$loc_list.dblclick(setLocationFromList);
		$loc_search = $("#loc_search");
		$loc_search.on("input", function(evt) {
			//queue a search if search string is at least 2 chars
			if (this.value.length >= 2) {
				locSearchTimeout && clearTimeout(locSearchTimeout);
				locSearchTimeout = setTimeout($.proxy(function() {
					locationApi.performLocationSearch(this.value, handleSearchResults);
				}, this), settings.editUpdateDelay);
			} else {
				locSearchTimeout && clearTimeout(locSearchTimeout);
				//clear results to avoid confusion
				$loc_list.empty();
			}
		});

		$loc_latitude = $("#loc_latitude");
		$loc_longitude = $("#loc_longitude");
		$loc_altitude = $("#loc_altitude");
		$loc_name = $("#loc_name");
		$loc_region = $("#loc_region");
		$loc_planet = $("#loc_planet");

		$("#loc_latitude, #loc_longitude").spinner({
			step: 0.00001,
			incremental: function(i) {
				return Math.floor(i * i * i / 100 - i * i / 10 + i + 1);
			}
		});
		$loc_latitude.spinner("option", {
			min: -90,
			max: 90
		}).on("spin", function(evt, ui) {
			locationApi.setLatitude(ui.value);
		});
		$loc_longitude.spinner("option", {
			min: -180,
			max: 180
		}).on("spin", function(evt, ui) {
			locationApi.setLongitude(ui.value);
		});
		$loc_altitude.spinner({
			step: 1,
			spin: function(evt, ui) {
				locationApi.setAltitude(ui.value);
			}
		});

		$loc_name.change(function(evt) {
			locationApi.setName($loc_name.val());
		});

		$loc_region.combobox({
			select: function(evt, data) {
				locationApi.setPlanet(data.item.value);
			}
		});
		$loc_planet.combobox({
			select: function(ev, data) {
				locationApi.setPlanet(data.item.value);
			}
		});

		locationApi.loadPlanetList(fillPlanetList);
		locationApi.loadRegionList(fillRegionList);
	}

	$(locationApi).on("nameChanged", function(evt, name) {
		$loc_name.val(name);
	});

	$(locationApi).on("regionChanged", function(evt, region) {
		$loc_region.combobox("autocomplete", region);
	});

	$(locationApi).on("planetChanged", function(evt, planet) {

		$loc_planet.combobox("autocomplete", planet);

		var planetUrl = "/api/location/planetimage?planet=" + encodeURIComponent(planet);
		if (planet === "Earth")
			planetUrl = "images/world.png";
		console.log("updating planet map image: " + planetUrl);

		$loc_mapimg.attr("src", planetUrl);
	});

	$(locationApi).on("altitudeChanged", function(evt, altitude) {
		$loc_altitude.spinner("value", altitude);
	});

	$(locationApi).on("longitudeChanged", function(evt, longitude) {
		$loc_longitude.spinner("value", longitude);
	});

	$(locationApi).on("latitudeChanged", function(evt, latitude) {
		$loc_latitude.spinner("value", latitude);
	});

	$(locationApi).on("positionChanged", function(evt, latitude, longitude) {
		movePointer(latitude, longitude);
	});


	$(initControls);
});
