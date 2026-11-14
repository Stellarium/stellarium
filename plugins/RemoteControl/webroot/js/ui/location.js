/* jshint expr: true */
define(["jquery", "api/remotecontrol", "api/location", "settings", "api/trunc", "./combobox", "./planet_data", "./timezone", "api/properties", "globalize"],
function($, rc, locationApi, settings, trunc, combobox, planetData, timezone, propApi, globalize) {
	"use strict";

	// =====================================================================
	// DOM REFERENCES
	// =====================================================================
	var $loc_mapimg;          // hidden <img> used to load the planet map
	var $loc_map_viewport;
	var $loc_map_canvas;
	var $loc_mappointer;      // visible pointer <img>
	var $loc_list;
	var $loc_search;

	var $loc_latitude;
	var $loc_longitude;
	var $loc_altitude;
	var $loc_name;
	var $loc_region;
	var $loc_planet;

	var $loc_radius_deg;
	var $loc_radius_km;
	var $loc_timezone;
	var $loc_use_custom_tz;
	
	var locSearchTimeout;

	// =====================================================================
	// LOCATION CACHE
	// =====================================================================
	var allLocations = null;
	var allLocationsLoaded = false;
	var allLocationsLoading = false;
	var activeRegionFilter = null;
	var currentPlanet = "Earth";
	// Coordinate display mode.
	//
	// When true, the DMS <span>s show decimal degrees (e.g. "+12.871°");
	// when false, they show degrees/minutes/seconds
	// (e.g. "N 12° 52' 15.55\"").
	//
	// This mirrors StelApp.flagUseDecDegreesCoords, which also drives
	// the native AngleSpinBox in the Qt LocationDialog.
	var useDecimalCoords = false;
	
	// =====================================================================
	// MAP STATE
	// =====================================================================
	var mapState = {
		// The planet map image (an offscreen HTMLImageElement)
		image: null,
		imgW: 0,               // natural width  of the current image
		imgH: 0,               // natural height of the current image

		// Viewport (CSS pixels)
		vpW: 0,
		vpH: 0,

		// Device pixel ratio (cached; refreshed on resize)
		dpr: 1,

		// Zoom and pan, in viewport pixels
		zoom: 1.0,
		minZoom: 1.0,          // never smaller than "fill the viewport"
		maxZoom: 8.0,
		offsetX: 0,
		offsetY: 0,

		// Marker geographic position
		markerLat: 0,
		markerLon: 0,

		// Search radius in degrees
		radiusDeg: 5,

		// Canvas 2D context
		ctx: null,

		// Pending redraw flag
		redrawPending: false
	};

	// Mouse / single-finger drag state
	var dragState = {
		active: false,
		startX: 0,
		startY: 0,
		startOffsetX: 0,
		startOffsetY: 0,
		moved: false
	};

	// Two-finger pinch-zoom state
	//
	// We capture the gesture's initial geometry ONCE at touchstart, and
	// then derive every subsequent zoom from that snapshot. This keeps
	// the midpoint anchored to the same image point throughout the
	// gesture, which feels natural even if the fingers drift slightly.
	var touchState = {
		active: false,
		initialDistance: 0,    // distance between the two fingers at start
		initialZoom: 1,        // mapState.zoom at start
		initialCenterX: 0,     // midpoint X at start (viewport pixels)
		initialCenterY: 0,     // midpoint Y at start (viewport pixels)
		initialOffsetX: 0,     // mapState.offsetX at start
		initialOffsetY: 0      // mapState.offsetY at start
	};

	// Suppress spinner feedback loops
	var updatingRadiusSpinners = false;

	// Last region sent to the server
	var lastSentRegion = null;

	// =====================================================================
	// GEODETIC HELPERS
	// =====================================================================

	/**
	 * Compute the destination point on a sphere given a start point,
	 * an angular distance, and a bearing.
	 *
	 * Standard great-circle formula:
	 *   lat2 = asin( sin(lat1)*cos(d) + cos(lat1)*sin(d)*cos(brg) )
	 *   lon2 = lon1 + atan2( sin(brg)*sin(d)*cos(lat1),
	 *                        cos(d) - sin(lat1)*sin(lat2) )
	 *
	 * @param {number} lat1 - Start latitude (radians).
	 * @param {number} lon1 - Start longitude (radians).
	 * @param {number} d    - Angular distance (radians).
	 * @param {number} brg  - Bearing (radians, 0 = north).
	 * @returns {{lat:number, lon:number}} Destination in radians.
	 */
	function destinationPoint(lat1, lon1, d, brg) {
		var sinLat1 = Math.sin(lat1);
		var cosLat1 = Math.cos(lat1);
		var sinD = Math.sin(d);
		var cosD = Math.cos(d);
		var sinBrg = Math.sin(brg);
		var cosBrg = Math.cos(brg);

		var lat2 = Math.asin(sinLat1 * cosD + cosLat1 * sinD * cosBrg);
		var lon2 = lon1 + Math.atan2(
			sinBrg * sinD * cosLat1,
			cosD - sinLat1 * Math.sin(lat2)
		);

		return { lat: lat2, lon: lon2 };
	}

	/**
	 * Normalize a longitude (in degrees) to [-180, 180).
	 */
	function normalizeLonDeg(lon) {
		while (lon > 180)   lon -= 360;
		while (lon < -180)  lon += 360;
		return lon;
	}

	// =====================================================================
	// MAP COORDINATE CONVERSIONS
	//
	// The image always fills the viewport at zoom=1 (base fit). All
	// drawing and hit-testing uses viewport coordinates, so the math
	// stays simple and there is no ambiguity between "image pixels"
	// and "screen pixels".
	//
	// Layout inside the viewport:
	//
	//   screenX = imgX * fitScale * zoom + offsetX
	//   screenY = imgY * fitScale * zoom + offsetY
	//
	// where imgX/imgY are the coordinates in the natural image space
	// (0..imgW, 0..imgH) and fitScale = max(vpW/imgW, vpH/imgH) so the
	// image always covers the viewport (no black bars).
	//
	// Since the viewport aspect ratio always matches the image aspect
	// ratio (set in updateViewportAspect), fitScale is the same for
	// both axes and the image fills the viewport exactly.
	// =====================================================================

	/**
	 * Return the base fit scale that makes the image fill the viewport
	 * at zoom=1. Because the viewport aspect ratio always matches the
	 * image aspect ratio, scaleX and scaleY are equal; we still use
	 * max() as a defensive measure.
	 */
	function computeFitScale() {
		if (!mapState.imgW || !mapState.imgH || !mapState.vpW || !mapState.vpH) {
			return 1;
		}
		var sx = mapState.vpW / mapState.imgW;
		var sy = mapState.vpH / mapState.imgH;
		return Math.max(sx, sy);
	}

	/**
	 * Convert (lat, lon) in degrees to viewport pixel coordinates.
	 *
	 * @param {number} lat
	 * @param {number} lon
	 * @returns {{x:number, y:number}} Viewport pixels.
	 */
	function geoToViewport(lat, lon) {
		var fit = computeFitScale();
		var imgX = (lon + 180) / 360 * mapState.imgW;
		var imgY = (90 - lat) / 180 * mapState.imgH;
		return {
			x: imgX * fit * mapState.zoom + mapState.offsetX,
			y: imgY * fit * mapState.zoom + mapState.offsetY
		};
	}

	/**
	 * Convert viewport pixel coordinates to (lat, lon) in degrees.
	 *
	 * @param {number} vx
	 * @param {number} vy
	 * @returns {{lat:number, lon:number}}
	 */
	function viewportToGeo(vx, vy) {
		var fit = computeFitScale();
		var imgX = (vx - mapState.offsetX) / (fit * mapState.zoom);
		var imgY = (vy - mapState.offsetY) / (fit * mapState.zoom);
		return {
			lon: imgX / mapState.imgW * 360 - 180,
			lat: 90 - imgY / mapState.imgH * 180
		};
	}

	// =====================================================================
	// TOUCH HELPERS (two-finger pinch)
	// =====================================================================

	/**
	 * Euclidean distance between two Touch objects, in CSS pixels.
	 *
	 * @param {Touch} t0
	 * @param {Touch} t1
	 * @returns {number}
	 */
	function touchDistance(t0, t1) {
		var dx = t1.clientX - t0.clientX;
		var dy = t1.clientY - t0.clientY;
		return Math.sqrt(dx * dx + dy * dy);
	}

	/**
	 * Midpoint between two Touch objects, expressed in viewport
	 * coordinates (i.e. relative to the viewport's top-left corner).
	 *
	 * @param {Touch} t0
	 * @param {Touch} t1
	 * @param {DOMRect} rect - The viewport's bounding client rect.
	 * @returns {{x:number, y:number}}
	 */
	function touchMidpoint(t0, t1, rect) {
		return {
			x: (t0.clientX + t1.clientX) / 2 - rect.left,
			y: (t0.clientY + t1.clientY) / 2 - rect.top
		};
	}

	// =====================================================================
	// VIEWPORT SIZING
	// =====================================================================

	/**
	 * Match the viewport aspect ratio to the current image aspect ratio.
	 * This guarantees the image fills the viewport exactly at zoom=1
	 * and avoids any letter-boxing or pillarboxing.
	 */
	function updateViewportAspect() {
		if (!mapState.imgW || !mapState.imgH) {
			return;
		}
		var ratio = mapState.imgW / mapState.imgH;
		$loc_map_viewport.css("aspect-ratio", ratio);
	}

	/**
	 * Recompute the viewport size, resize the canvas backing store,
	 * refresh the DPR, reset zoom to 100% and redraw.
	 *
	 * Called on init, on image load, on planet change, and on resize.
	 */
	function updateViewportSize() {
		var vp = $loc_map_viewport[0];
		if (!vp) {
			return;
		}
		var rect = vp.getBoundingClientRect();
		if (!rect.width || !rect.height) {
			return;
		}

		mapState.vpW = rect.width;
		mapState.vpH = rect.height;
		mapState.dpr = window.devicePixelRatio || 1;

		// Resize the canvas backing store
		var canvas = $loc_map_canvas[0];
		canvas.width  = Math.round(rect.width  * mapState.dpr);
		canvas.height = Math.round(rect.height * mapState.dpr);

		// Fresh context with the DPR transform applied
		mapState.ctx = canvas.getContext("2d");
		mapState.ctx.setTransform(mapState.dpr, 0, 0, mapState.dpr, 0, 0);

		// Reset zoom and center the image
		mapState.zoom = 1.0;
		mapState.offsetX = 0;
		mapState.offsetY = 0;
		clampOffsets();

		updateZoomLabel();
		redraw();
	}

	/**
	 * Clamp the pan offsets so the image never leaves the viewport.
	 *
	 * At zoom=1 the image exactly fills the viewport, so the offsets
	 * are clamped to 0. At zoom>1 the image is larger than the
	 * viewport and can be panned, but its edges cannot move inside the
	 * viewport bounds.
	 */
	function clampOffsets() {
		var fit = computeFitScale();
		var scaledW = mapState.imgW * fit * mapState.zoom;
		var scaledH = mapState.imgH * fit * mapState.zoom;

		// Horizontal
		if (scaledW <= mapState.vpW + 0.5) {
			// Image fits horizontally: center it
			mapState.offsetX = (mapState.vpW - scaledW) / 2;
		} else {
			// Image wider than viewport: clamp to edges
			var minX = mapState.vpW - scaledW;
			if (mapState.offsetX > 0)        mapState.offsetX = 0;
			if (mapState.offsetX < minX)     mapState.offsetX = minX;
		}

		// Vertical
		if (scaledH <= mapState.vpH + 0.5) {
			mapState.offsetY = (mapState.vpH - scaledH) / 2;
		} else {
			var minY = mapState.vpH - scaledH;
			if (mapState.offsetY > 0)        mapState.offsetY = 0;
			if (mapState.offsetY < minY)     mapState.offsetY = minY;
		}
	}

	// =====================================================================
	// REDRAW (coalesced through requestAnimationFrame)
	// =====================================================================

	function scheduleRedraw() {
		if (mapState.redrawPending) {
			return;
		}
		mapState.redrawPending = true;
		window.requestAnimationFrame(function() {
			mapState.redrawPending = false;
			redraw();
		});
	}

	/**
	 * Redraw the canvas: background, image, radius ellipse.
	 * The pointer is a separate <img> element, updated here too.
	 */
	function redraw() {
		var ctx = mapState.ctx;
		if (!ctx) {
			return;
		}

		// Clear
		ctx.clearRect(0, 0, mapState.vpW, mapState.vpH);

		// Black background (visible during pan beyond image edges)
		ctx.fillStyle = "#000";
		ctx.fillRect(0, 0, mapState.vpW, mapState.vpH);

		if (!mapState.image || !mapState.imgW || !mapState.imgH) {
			return;
		}

		var fit = computeFitScale();
		var scale = fit * mapState.zoom;

		// Draw the image at its computed position and size
		ctx.drawImage(
			mapState.image,
			mapState.offsetX, mapState.offsetY,
			mapState.imgW * scale, mapState.imgH * scale
		);

		// Draw the radius ellipse (in viewport coordinates)
		drawRadiusEllipse(ctx);

		// Update the pointer's screen position
		updatePointerPosition();
	}

	// =====================================================================
	// RADIUS ELLIPSE (geodetic, drawn in viewport coordinates)
	// =====================================================================

	/**
	 * Draw the search-radius ellipse on the canvas.
	 *
	 * The ellipse is the set of points at angular distance R (in
	 * degrees) from the marker. We sample 72 bearings and convert each
	 * destination point to viewport coordinates.
	 *
	 * Segments are split at the ±180° meridian to avoid the wrap-around
	 * artefact.
	 *
	 * @param {CanvasRenderingContext2D} ctx
	 */
	function drawRadiusEllipse(ctx) {
		var R = mapState.radiusDeg;
		if (!(R > 0)) {
			return;
		}

		var lat0 = mapState.markerLat * Math.PI / 180;
		var lon0 = mapState.markerLon * Math.PI / 180;
		var Rrad = R * Math.PI / 180;

		var NUM_POINTS = 72;
		var segments = [[]];
		var prevLon = null;

		for (var i = 0; i <= NUM_POINTS; i++) {
			var bearing = (i / NUM_POINTS) * 2 * Math.PI;
			var p = destinationPoint(lat0, lon0, Rrad, bearing);

			var lat = p.lat * 180 / Math.PI;
			var lon = normalizeLonDeg(p.lon * 180 / Math.PI);

			if (prevLon !== null && Math.abs(lon - prevLon) > 180) {
				segments.push([]);
			}
			prevLon = lon;

			var pt = geoToViewport(lat, lon);
			segments[segments.length - 1].push(pt);
		}

		// Styling
		ctx.strokeStyle = "turquoise";
		ctx.fillStyle = "rgba(64, 224, 208, 0.12)";
		ctx.lineWidth = 2;
		ctx.lineJoin = "round";

		for (var s = 0; s < segments.length; s++) {
			var seg = segments[s];
			if (seg.length < 2) {
				continue;
			}
			ctx.beginPath();
			ctx.moveTo(seg[0].x, seg[0].y);
			for (var k = 1; k < seg.length; k++) {
				ctx.lineTo(seg[k].x, seg[k].y);
			}
			if (segments.length === 1) {
				ctx.closePath();
				ctx.fill();
			}
			ctx.stroke();
		}
	}

	// =====================================================================
	// COORDINATE DISPLAY (DMS + Decimal)
	// =====================================================================

	/**
	 * Format an angle for display next to a coordinate spinner.
	 *
	 * When `useDecimalCoords` is true, the value is shown as signed
	 * decimal degrees with a trailing "°" (e.g. "+12.871°"). The
	 * number is formatted through Globalize with 6 decimal places.
	 *
	 * Otherwise the value is shown as a DMS string with a hemisphere
	 * letter (N/S for latitude, E/W for longitude) and the seconds
	 * rounded to 2 decimal places through Globalize.
	 *
	 * Elevation is always shown as an integer number of meters with a
	 * " m" suffix, regardless of the coordinate mode.
	 *
	 * @param {number} deg  - Angle in decimal degrees (or meters for alt).
	 * @param {string} type - "lat", "lon" or "alt".
	 * @returns {string} Formatted display string.
	 */
	function formatAngle(deg, type) {
		if (type === "alt") {
			// Elevation: integer meters, with thousands separator.
			return globalize.format(deg, "n0") + " m";
		}

		if (useDecimalCoords) {
			// Signed decimal degrees, 6 decimals.
			var sign = deg >= 0 ? "+" : "-";
			return sign + globalize.format(Math.abs(deg), "n6") + "°";
		}

		// DMS
		var dir;
		if (type === "lat") {
			dir = deg >= 0 ? "N" : "S";
		} else {
			dir = deg >= 0 ? "E" : "W";
		}

		var absDeg = Math.abs(deg);
		var d = Math.floor(absDeg);
		var minFloat = (absDeg - d) * 60;
		var m = Math.floor(minFloat);
		var s = (minFloat - m) * 60;

		// Round seconds to 2 decimals. Globalize gives us the correct
		// decimal separator for the current interface language (but
		// since the server sends strings already translated, this is
		// usually "." for en).
		var sStr = globalize.format(s, "n2");
		// Guard against 60.00 after rounding.
		if (parseFloat(sStr.replace(",", ".")) >= 60) {
			sStr = globalize.format(0, "n2");
			m += 1;
		}
		if (m >= 60) {
			m = 0;
			d += 1;
		}

		return dir + " " + d + "° " + m + "' " + sStr + "\"";
	}

	/**
	 * Refresh all coordinate displays (the <span>s next to the
	 * latitude, longitude and elevation spinners).
	 *
	 * Called whenever the marker position changes or the coordinate
	 * display mode toggles.
	 */
	function updateCoordinateDisplays() {
		var $lat = $("#loc_latitude_dms");
		var $lon = $("#loc_longitude_dms");
		var $alt = $("#loc_altitude_display");

		if ($lat.length) {
			$lat.text(formatAngle(mapState.markerLat, "lat"));
		}
		if ($lon.length) {
			$lon.text(formatAngle(mapState.markerLon, "lon"));
		}
		if ($alt.length) {
			// Read the current elevation from the spinner, since
			// mapState does not track it.
			var alt = parseFloat($loc_altitude.spinner("value")) || 0;
			$alt.text(formatAngle(alt, "alt"));
		}
	}

	// =====================================================================
	// POINTER MARKER (a separate <img>, constant on-screen size)
	// =====================================================================

	function updatePointerPosition() {
		if (!$loc_mappointer || !$loc_mappointer.length) {
			return;
		}
		var pos = geoToViewport(mapState.markerLat, mapState.markerLon);

		// Hide the marker if the point is well outside the viewport
		var margin = 40;
		if (pos.x < -margin || pos.x > mapState.vpW + margin ||
		    pos.y < -margin || pos.y > mapState.vpH + margin) {
			$loc_mappointer.css("display", "none");
			return;
		}

		$loc_mappointer.css({
			"display": "block",
			"left": pos.x + "px",
			"top": pos.y + "px"
		});
	}

	// =====================================================================
	// ZOOM
	// =====================================================================

	function updateZoomLabel() {
		var pct = Math.round(mapState.zoom * 100);
		$("#loc_zoom_level").text(pct + "%");
	}

	/**
	 * Zoom by a multiplicative factor around a viewport point.
	 *
	 * The image point that lies under (vx, vy) at the current zoom is
	 * kept under the same viewport point at the new zoom. This makes
	 * both mouse-wheel zoom and pinch-zoom feel anchored to the cursor
	 * or the gesture midpoint.
	 *
	 * @param {number} vx - X in viewport pixels.
	 * @param {number} vy - Y in viewport pixels.
	 * @param {number} factor - Multiplicative zoom factor.
	 */
	function zoomAtPoint(vx, vy, factor) {
		var newZoom = mapState.zoom * factor;
		if (newZoom < mapState.minZoom) newZoom = mapState.minZoom;
		if (newZoom > mapState.maxZoom) newZoom = mapState.maxZoom;
		if (newZoom === mapState.zoom) {
			return;
		}

		// Image point under the cursor (in natural image space)
		var fit = computeFitScale();
		var imgX = (vx - mapState.offsetX) / (fit * mapState.zoom);
		var imgY = (vy - mapState.offsetY) / (fit * mapState.zoom);

		// Apply new zoom
		mapState.zoom = newZoom;

		// Recompute offsets so the same image point stays under the cursor
		mapState.offsetX = vx - imgX * fit * newZoom;
		mapState.offsetY = vy - imgY * fit * newZoom;

		clampOffsets();
		scheduleRedraw();
		updateZoomLabel();
	}

	/**
	 * Apply a two-finger pinch-zoom gesture.
	 *
	 * The zoom factor is the ratio of the current finger distance to
	 * the initial distance captured at touchstart. The zoom is anchored
	 * on the initial midpoint between the two fingers, which is stored
	 * in touchState so that slight finger drift does not make the map
	 * slide under the gesture.
	 *
	 * @param {number} currentDistance - Current distance between fingers.
	 */
	function applyPinchZoom(currentDistance) {
		if (touchState.initialDistance <= 0) {
			return;
		}

		var factor = currentDistance / touchState.initialDistance;
		var targetZoom = touchState.initialZoom * factor;

		// Clamp to [minZoom, maxZoom]
		if (targetZoom < mapState.minZoom) targetZoom = mapState.minZoom;
		if (targetZoom > mapState.maxZoom) targetZoom = mapState.maxZoom;

		// Skip micro-changes to avoid jitter
		if (Math.abs(targetZoom - mapState.zoom) < 0.001) {
			return;
		}

		// Find the image point that was under the initial midpoint at
		// the initial zoom, then keep it under the same midpoint at the
		// new zoom.
		var fit = computeFitScale();
		var imgX = (touchState.initialCenterX - touchState.initialOffsetX) /
		           (fit * touchState.initialZoom);
		var imgY = (touchState.initialCenterY - touchState.initialOffsetY) /
		           (fit * touchState.initialZoom);

		mapState.zoom = targetZoom;
		mapState.offsetX = touchState.initialCenterX - imgX * fit * targetZoom;
		mapState.offsetY = touchState.initialCenterY - imgY * fit * targetZoom;

		clampOffsets();
		scheduleRedraw();
		updateZoomLabel();
	}

	function resetZoom() {
		mapState.zoom = 1.0;
		mapState.offsetX = 0;
		mapState.offsetY = 0;
		clampOffsets();
		scheduleRedraw();
		updateZoomLabel();
	}

	// =====================================================================
	// LOCATION SEARCH
	// =====================================================================

	// Track the last location we sent to avoid duplicate requests
	// when both "change" and "dblclick" fire for the same pick.
	var lastSentLocationId = null;

	/**
	 * Place the observer at the location currently selected in the
	 * location list.
	 *
	 * Guarded against duplicate dispatches: on desktop, picking an
	 * <option> may fire both "change" and "dblclick" for the same item;
	 * on touch devices, only "change" is fired. We ignore any pick that
	 * matches the last one we sent.
	 */
	function setLocationFromList() {
		var e = $loc_list[0];
		if (e.selectedIndex < 0) {
			return;
		}
		var loc = e.options[e.selectedIndex].text;
		if (!loc || loc === lastSentLocationId) {
			return;
		}
		lastSentLocationId = loc;
		locationApi.setLocationById(loc);
	}

	function renderLocationList(items) {
		var parent = $loc_list.parent();
		$loc_list.detach();
		$loc_list.empty();

		for (var i = 0; i < items.length; ++i) {
			var op = document.createElement("option");
			op.textContent = items[i];
			$loc_list[0].appendChild(op);
		}

		parent.prepend($loc_list);
	}

	function extractRegion(locStr) {
		if (!locStr) {
			return "";
		}
		var idx = locStr.lastIndexOf(",");
		if (idx === -1) {
			return "";
		}
		return locStr.substring(idx + 1).trim();
	}

	function filterLocationsByRegion(region) {
		if (!allLocations || !allLocations.length) {
			return [];
		}
		if (!region) {
			return allLocations;
		}
		var results = [];
		for (var i = 0; i < allLocations.length; i++) {
			if (extractRegion(allLocations[i]) === region) {
				results.push(allLocations[i]);
			}
		}
		return results;
	}

	function handleSearchResults(data) {
		renderLocationList(data);
	}

	function handleNearbyResults(data) {
		renderLocationList(data);
	}

	function localizedSort(a, b) {
		if (a.name_i18n > b.name_i18n) {
			return 1;
		}
		if (a.name_i18n < b.name_i18n) {
			return -1;
		}
		return 0;
	}

	function fillRegionList(data) {
		var parent = $loc_region.parent();
		$loc_region.detach();
		$loc_region.empty();
		data.sort(localizedSort);

		for (var i = 0; i < data.length; i++) {
			var op = document.createElement("option");
			op.innerHTML = data[i].name_i18n;
			op.value = data[i].name;
		        $loc_region[0].appendChild(op);
		}

		parent.append($loc_region);
	}

	function fillPlanetList(data) {
		var parent = $loc_planet.parent();
		$loc_planet.detach();
		$loc_planet.empty();
		data.sort(localizedSort);

		for (var i = 0; i < data.length; i++) {
			var op = document.createElement("option");
			op.innerHTML = data[i].name_i18n;
			op.value = data[i].name;
			$loc_planet[0].appendChild(op);
		}

		parent.append($loc_planet);
	}

	// =====================================================================
	// FULL LOCATION LIST LOADER
	// =====================================================================

	function ensureAllLocationsLoaded(callback) {
		if (allLocationsLoaded) {
			callback(allLocations);
			return;
		}
		if (allLocationsLoading) {
			var waitHandle = setInterval(function() {
				if (allLocationsLoaded || !allLocationsLoading) {
					clearInterval(waitHandle);
					callback(allLocations || []);
				}
			}, 100);
			return;
		}

		allLocationsLoading = true;
		var t0 = (window.performance && performance.now) ? performance.now() : Date.now();
		console.log("[location] loading full location list via locationApi.loadAllLocations() ...");

		locationApi.loadAllLocations(function(data) {
			allLocations = data || [];
			allLocationsLoaded = true;
			allLocationsLoading = false;

			var t1 = (window.performance && performance.now) ? performance.now() : Date.now();
			console.log("[location] loaded " + allLocations.length +
				" locations in " + Math.round(t1 - t0) + " ms");

			callback(allLocations);
		});
	}

	function searchLocalLocations(term) {
		if (!allLocations || !allLocations.length) {
			return [];
		}
		var pattern = term
			.replace(/[.*+?^${}()|[\]\\]/g, "\\$&")
			.replace(/\\\*/g, ".*");
		var re = new RegExp(pattern, "i");

		var results = [];
		for (var i = 0; i < allLocations.length; i++) {
			if (re.test(allLocations[i])) {
				results.push(allLocations[i]);
			}
		}
		return results;
	}

	// =====================================================================
	// RADIUS MANAGEMENT
	// =====================================================================

	function applyDefaultRadiusForPlanet(planet) {
		var defaultDeg = (planet === "Earth") ? 5 : 30;
		setRadiusDegrees(defaultDeg);
	}

	function setRadiusDegrees(deg) {
		deg = parseFloat(deg);
		if (!isFinite(deg) || deg <= 0) {
			return;
		}
		mapState.radiusDeg = deg;

		updatingRadiusSpinners = true;
		$loc_radius_deg.spinner("value", deg);
		// Convert using the radius at the current marker latitude.
		// planet_data uses an oblate-spheroid model, so the local
		// radius depends on |lat| (small for Earth, large for Jupiter
		// and Saturn).
		$loc_radius_km.spinner("value",
			Math.round(planetData.degreesToKm(
				deg, currentPlanet, mapState.markerLat)));
		updatingRadiusSpinners = false;

		scheduleRedraw();

		if (mapState.markerLat !== null && mapState.markerLon !== null) {
			locationApi.performNearbySearch(
				mapState.markerLat, mapState.markerLon,
				handleNearbyResults, mapState.radiusDeg);
		}
	}

	// =====================================================================
	// POINTER UPDATES FROM SPINNERS
	// =====================================================================

	var pointerUpdateTimeout;

	function updatePointerFromInputs() {
		var lat = $loc_latitude.spinner("value");
		var lon = $loc_longitude.spinner("value");
		mapState.markerLat = lat;
		mapState.markerLon = lon;
		scheduleRedraw();
	}

	function debouncedUpdatePointer() {
		if (pointerUpdateTimeout) clearTimeout(pointerUpdateTimeout);
		pointerUpdateTimeout = setTimeout(updatePointerFromInputs, 50);
	}

	// =====================================================================
	// PLANET MAP IMAGE LOADING
	// =====================================================================

	/**
	 * Load the planet map image for the given planet.
	 *
	 * Uses an offscreen <img> element. On load, reads the natural size,
	 * updates the viewport aspect ratio, resizes the canvas and redraws.
	 *
	 * @param {string} planet - English planet name.
	 */
	function loadPlanetMap(planet) {
		var url = "/api/location/planetimage?planet=" + encodeURIComponent(planet);
		if (planet === "Earth") {
			url = "images/world.png";
		}
		console.log("[location] loading planet map: " + url);

		// Disable the marker while the image is loading
		$loc_mappointer.css("display", "none");

		var img = new Image();
		img.onload = function() {
			mapState.image = img;
			mapState.imgW = img.naturalWidth;
			mapState.imgH = img.naturalHeight;
			console.log("[location] map loaded: " + mapState.imgW + "×" + mapState.imgH);

			// Update the viewport aspect ratio to match the image,
			// then resize the canvas and reset the view.
			updateViewportAspect();

			// Let the browser apply the new aspect-ratio before
			// measuring the viewport size.
			window.requestAnimationFrame(function() {
				updateViewportSize();
			});
		};
		img.onerror = function() {
			console.error("[location] failed to load planet map: " + url);
		};
		img.src = url;
	}
	
	/**
	 * Place the marker at a viewport pixel coordinate.
	 *
	 * This is the shared implementation behind both the desktop "click"
	 * handler and the touch "touchend" handler. It converts the viewport
	 * point to geographic coordinates, updates the map state, refreshes
	 * the lat/lon spinners, sends the new position to the server and
	 * triggers a nearby search.
	 *
	 * @param {number} vx - X in viewport pixels.
	 * @param {number} vy - Y in viewport pixels.
	 */
	function placeMarkerAtViewport(vx, vy) {
		var geo = viewportToGeo(vx, vy);

		// Ignore clicks outside the valid latitude range
		if (geo.lat < -90 || geo.lat > 90) {
			return;
		}

		mapState.markerLat = geo.lat;
		mapState.markerLon = geo.lon;
		scheduleRedraw();

		$loc_latitude.spinner("value", geo.lat);
		$loc_longitude.spinner("value", geo.lon);

		locationApi.setLatitude(geo.lat);
		locationApi.setLongitude(geo.lon);

		locationApi.performNearbySearch(
			geo.lat, geo.lon, handleNearbyResults, mapState.radiusDeg);
			updateCoordinateDisplays();
	}

	// =====================================================================
	// TIME ZONE LIST
	// =====================================================================

	/**
	 * Populate the time-zone <select> with all valid names.
	 *
	 * The special Stellarium names (LMST, LTST, system_default) are
	 * placed at the top with human-readable labels, followed by the
	 * full IANA list.
	 *
	 * This must run BEFORE connectStelProperties() so that the
	 * generic StelProperty handler can bind to the <select>. In
	 * practice, ui/mainui.js calls connectStelProperties() inside
	 * its own DOM-ready callback, and this function is called from
	 * initControls(), which is also a DOM-ready callback. RequireJS
	 * guarantees ui/location.js is evaluated before ui/mainui.js's
	 * callback runs, so the order is safe.
	 *
	 * The element itself carries class="stelproperty" and
	 * name="StelCore.currentTimeZone", so no per-element wiring is
	 * needed here: the generic handler in ui/mainui.js will
	 * automatically read the current value and write it back on
	 * change.
	 */
	function fillTimezoneList() {
		var $select = $("#loc_timezone");
		if (!$select.length) {
			return;
		}

		$select.empty();

		// 1. Special names first, with localized labels.
		//    The labels are passed through rc.tr() so they follow the
		//    interface language selected in Stellarium.
		var special = timezone.getSpecial();
		for (var i = 0; i < special.length; i++) {
			$select.append(
				$("<option>")
					.val(special[i].value)
					.text(rc.tr(special[i].label))
			);
		}

		// 2. IANA names, as returned by QTimeZone::availableTimeZoneIds()
		var iana = timezone.getIana();
		for (var j = 0; j < iana.length; j++) {
			$select.append(
				$("<option>")
					.val(iana[j])
					.text(iana[j])
			);
		}
	}
	
	/**
	 * Enable or disable the time-zone <select> based on the current
	 * value of StelCore.flagUseCTZ.
	 *
	 * When the flag is false, Stellarium derives the time zone from
	 * the observer's location automatically, so manual selection is
	 * meaningless and the <select> is disabled.
	 *
	 * When the flag is true, the user can pick a time zone that is
	 * independent of the observer's location, so the <select> is
	 * enabled.
	 *
	 * @param {boolean} useCustom - Current value of StelCore.flagUseCTZ.
	 */
	function updateTimezoneSelectState(useCustom) {
		var $select = $("#loc_timezone");
		if (!$select.length) {
			return;
		}
		$select.prop("disabled", !useCustom);
	}	
	
	// =====================================================================
	// INIT
	// =====================================================================

	function initControls() {
		// ---- DOM references ----
		$loc_map_viewport = $("#loc_map_viewport");
		$loc_map_canvas   = $("#loc_map_canvas");
		$loc_mappointer   = $("#loc_mappointer");

		$loc_list = $("#loc_list");

		// Selection from the location list.
		//
		// We use the native "change" event as the primary trigger
		// because <select> is a native HTML element, and jQuery UI
		// Touch Punch does NOT translate touch events on it. On touch
		// devices, tapping an <option> fires "change" but never
		// "dblclick", so the previous dblclick-only handler made the
		// list unusable on tablets and phones.
		//
		// "dblclick" is kept as a desktop convenience, but the
		// setLocationFromList() guard prevents duplicate requests when
		// both events fire for the same pick.
		$loc_list.on("change", setLocationFromList);
		$loc_list.dblclick(setLocationFromList);

		$loc_search = $("#loc_search");

		$loc_latitude  = $("#loc_latitude");
		$loc_longitude = $("#loc_longitude");
		$loc_altitude  = $("#loc_altitude");
		$loc_name      = $("#loc_name");
		$loc_region    = $("#loc_region");
		$loc_planet    = $("#loc_planet");
		$loc_timezone  = $("#loc_timezone");
		$loc_use_custom_tz = $("#loc_use_custom_tz");

		$loc_radius_deg = $("#loc_radius_deg");
		$loc_radius_km  = $("#loc_radius_km");

		// ---- Coordinate display mode ----
		//
		// The checkbox #loc_use_decimal_degrees is wired to
		// StelApp.flagUseDecDegreesCoords by connectStelProperties()
		// (it carries class="stelproperty"). We only need to:
		//   1. read the initial value,
		//   2. listen for changes to refresh the DMS <span>s.
		var initialUseDecimal = propApi.getStelProp(
			"StelApp.flagUseDecDegreesCoords");
		useDecimalCoords = (initialUseDecimal === true ||
		                    initialUseDecimal === "true" ||
		                    initialUseDecimal === 1 ||
		                    initialUseDecimal === "1");

		$(propApi).on(
			"stelPropertyChanged:StelApp.flagUseDecDegreesCoords",
			function(evt, prop) {
				useDecimalCoords = (prop.value === true ||
				                    prop.value === "true" ||
				                    prop.value === 1 ||
				                    prop.value === "1");
				updateCoordinateDisplays();
			});

		// ---- Load the initial Earth map ----
		loadPlanetMap("Earth");

		// ---- Resize observer ----
		if (window.ResizeObserver) {
			new ResizeObserver(function() {
				// The viewport size changed: re-measure and redraw.
				updateViewportSize();
			}).observe($loc_map_viewport[0]);
		}
		$(window).on("resize", function() {
			updateViewportSize();
		});

		// ---- Zoom controls ----
		$("#loc_zoom_in").on("click", function(e) {
			e.preventDefault();
			zoomAtPoint(mapState.vpW / 2, mapState.vpH / 2, 1.25);
		});
		$("#loc_zoom_out").on("click", function(e) {
			e.preventDefault();
			zoomAtPoint(mapState.vpW / 2, mapState.vpH / 2, 1 / 1.25);
		});
		$("#loc_zoom_reset").on("click", function(e) {
			e.preventDefault();
			resetZoom();
		});

		// ---- Mouse wheel zoom ----
		$loc_map_viewport.on("wheel", function(e) {
			e.preventDefault();
			var rect = this.getBoundingClientRect();
			var vx = e.clientX - rect.left;
			var vy = e.clientY - rect.top;
			var factor = (e.originalEvent.deltaY < 0) ? 1.15 : 1 / 1.15;
			zoomAtPoint(vx, vy, factor);
		});

		// ---- Mouse drag to pan ----
		$loc_map_viewport.on("mousedown", function(e) {
			if (e.which !== undefined && e.which !== 1) {
				return;
			}
			dragState.active = true;
			dragState.moved = false;
			dragState.startX = e.clientX;
			dragState.startY = e.clientY;
			dragState.startOffsetX = mapState.offsetX;
			dragState.startOffsetY = mapState.offsetY;
			$loc_map_viewport.addClass("dragging");
			e.preventDefault();
		});

		$(document).on("mousemove.locmap", function(e) {
			if (!dragState.active) {
				return;
			}
			var dx = e.clientX - dragState.startX;
			var dy = e.clientY - dragState.startY;
			if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
				dragState.moved = true;
			}
			mapState.offsetX = dragState.startOffsetX + dx;
			mapState.offsetY = dragState.startOffsetY + dy;
			clampOffsets();
			scheduleRedraw();
		});

		$(document).on("mouseup.locmap", function() {
			if (dragState.active) {
				dragState.active = false;
				$loc_map_viewport.removeClass("dragging");
			}
		});

		// ---- Touch: single-finger pan + two-finger pinch zoom ----
		//
		// We handle touch events directly rather than relying on
		// jquery.ui.touch-punch, because touch-punch does not support
		// multi-touch gestures (pinch). Handling them here gives us
		// full control over both pan and zoom, and avoids the
		// mousedown events that touch-punch would otherwise fire for
		// each finger during a pinch.
		//
		// `touch-action: none` is set on #loc_map_viewport in CSS to
		// suppress the browser's default panning and zooming.

		$loc_map_viewport.on("touchstart", function(e) {
			var touches = e.originalEvent.touches;

			if (touches.length === 1) {
				// Single finger: begin pan
				e.preventDefault();
				dragState.active = true;
				dragState.moved = false;
				dragState.startX = touches[0].clientX;
				dragState.startY = touches[0].clientY;
				dragState.startOffsetX = mapState.offsetX;
				dragState.startOffsetY = mapState.offsetY;
			} else if (touches.length === 2) {
				// Two fingers: begin pinch
				e.preventDefault();
				var t0 = touches[0];
				var t1 = touches[1];
				var rect = this.getBoundingClientRect();

				touchState.active = true;
				touchState.initialDistance = touchDistance(t0, t1);
				touchState.initialZoom = mapState.zoom;
				touchState.initialOffsetX = mapState.offsetX;
				touchState.initialOffsetY = mapState.offsetY;

				var mid = touchMidpoint(t0, t1, rect);
				touchState.initialCenterX = mid.x;
				touchState.initialCenterY = mid.y;

				// Cancel any in-progress pan; the pinch takes over
				dragState.active = false;
				$loc_map_viewport.removeClass("dragging");
			}
		});

		$loc_map_viewport.on("touchmove", function(e) {
			var touches = e.originalEvent.touches;

			if (touches.length === 1 && dragState.active && !touchState.active) {
				// Single-finger pan
				e.preventDefault();
				var dx = touches[0].clientX - dragState.startX;
				var dy = touches[0].clientY - dragState.startY;
				if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
					dragState.moved = true;
				}
				mapState.offsetX = dragState.startOffsetX + dx;
				mapState.offsetY = dragState.startOffsetY + dy;
				clampOffsets();
				scheduleRedraw();
			} else if (touches.length === 2 && touchState.active) {
				// Two-finger pinch
				e.preventDefault();
				var currentDistance = touchDistance(touches[0], touches[1]);
				applyPinchZoom(currentDistance);
			}
		});

		$loc_map_viewport.on("touchend touchcancel", function(e) {
			var touches = e.originalEvent.touches;

			if (touches.length === 0) {
				// All fingers lifted.
				//
				// If the gesture was a single tap (no pan, no pinch),
				// place the marker at the tap position. We use the
				// touchend event rather than "click" because
				// touchstart calls preventDefault, which suppresses
				// the synthetic click event on touch devices.
				if (!dragState.moved && !touchState.active) {
					var t = e.originalEvent.changedTouches[0];
					var rect = this.getBoundingClientRect();
					placeMarkerAtViewport(
						t.clientX - rect.left,
						t.clientY - rect.top);
				}

				dragState.active = false;
				touchState.active = false;
				$loc_map_viewport.removeClass("dragging");
			} else if (touches.length === 1) {
				// One finger remains after a pinch: restart pan from
				// the remaining finger's current position.
				touchState.active = false;
				dragState.active = true;
				dragState.moved = false;
				dragState.startX = touches[0].clientX;
				dragState.startY = touches[0].clientY;
				dragState.startOffsetX = mapState.offsetX;
				dragState.startOffsetY = mapState.offsetY;
			}
		});

		// ---- Click to place marker ----
		//
		// We use "click" for both mouse and touch. The dragState.moved
		// flag suppresses the click that would otherwise fire after a
		// pan or a pinch.
		$loc_map_viewport.on("click", function(e) {
			// Suppress the click that fires after a mouse drag
			if (dragState.moved) {
				dragState.moved = false;
				return;
			}
			// Suppress the click that fires after a touch pinch
			if (touchState.active) {
				return;
			}
			var rect = this.getBoundingClientRect();
			placeMarkerAtViewport(
				e.clientX - rect.left,
				e.clientY - rect.top);
		});

		// ---- Preload the full location list ----
		ensureAllLocationsLoaded(function(list) {
			console.log("[location] preload finished, " + list.length + " entries cached");
		});

		// ---- Search box ----
		$loc_search.on("input", function(evt) {
			var term = this.value;

			if (term.length >= 2) {
				locSearchTimeout && clearTimeout(locSearchTimeout);
				locSearchTimeout = setTimeout($.proxy(function() {
					if (allLocationsLoaded) {
						var local = searchLocalLocations(term);
						renderLocationList(local.slice(0, 1000));
						return;
					}
					locationApi.performLocationSearch(term, handleSearchResults);
				}, this), settings.editUpdateDelay);
			} else {
				locSearchTimeout && clearTimeout(locSearchTimeout);
				if (activeRegionFilter && allLocationsLoaded) {
					var regionList = filterLocationsByRegion(activeRegionFilter);
					renderLocationList(regionList.slice(0, 5000));
				} else {
					$loc_list.empty();
				}
			}
		});

		// ---- Lat / Lon spinners ----
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
			mapState.markerLat = ui.value;
			scheduleRedraw();
			updateCoordinateDisplays();			
		});
		$loc_longitude.spinner("option", {
			min: -180,
			max: 180
		}).on("spin", function(evt, ui) {
			locationApi.setLongitude(ui.value);
			mapState.markerLon = ui.value;
			scheduleRedraw();
			updateCoordinateDisplays();			
		});
		$loc_altitude.spinner({
			step: 1,
			spin: function(evt, ui) {
				locationApi.setAltitude(ui.value);
				updateCoordinateDisplays();				
			}
		});

		// ---- Radius spinners ----
		//
		// Both spinners are two views of the same value (the search
		// radius in degrees). Editing one updates the other through
		// planetData, which uses the oblate-spheroid model to convert
		// between degrees and kilometres at the current marker
		// latitude. The updatingRadiusSpinners flag prevents the two
		// handlers from feeding each other.
		$loc_radius_deg.spinner({
			min: 0.1,
			max: 180,
			step: 0.5
		}).on("spinchange spin", function() {
			if (updatingRadiusSpinners) {
				return;
			}
			var deg = parseFloat($loc_radius_deg.spinner("value"));
			if (!isFinite(deg) || deg <= 0) {
				return;
			}
			mapState.radiusDeg = deg;
			updatingRadiusSpinners = true;
			$loc_radius_km.spinner("value",
				Math.round(planetData.degreesToKm(
					deg, currentPlanet, mapState.markerLat)));
			updatingRadiusSpinners = false;

			scheduleRedraw();
			if (mapState.markerLat !== null && mapState.markerLon !== null) {
				locationApi.performNearbySearch(
					mapState.markerLat, mapState.markerLon,
					handleNearbyResults, mapState.radiusDeg);
			}
		});

		$loc_radius_km.spinner({
			min: 10,
			max: 20000,
			step: 10
		}).on("spinchange spin", function() {
			if (updatingRadiusSpinners) {
				return;
			}
			var km = parseFloat($loc_radius_km.spinner("value"));
			if (!isFinite(km) || km <= 0) {
				return;
			}
			var deg = planetData.kmToDegrees(
				km, currentPlanet, mapState.markerLat);
			mapState.radiusDeg = deg;
			updatingRadiusSpinners = true;
			$loc_radius_deg.spinner("value", deg.toFixed(2));
			updatingRadiusSpinners = false;

			scheduleRedraw();
			if (mapState.markerLat !== null && mapState.markerLon !== null) {
				locationApi.performNearbySearch(
					mapState.markerLat, mapState.markerLon,
					handleNearbyResults, mapState.radiusDeg);
			}
		});

		// ---- Name / region / planet ----
		$loc_name.change(function(evt) {
			locationApi.setName($loc_name.val());
		});

		// FIX #1: region combobox calls setRegion (not setPlanet)
		$loc_region.combobox({
			select: function(evt, data) {
				locationApi.setRegion(data.item.value);
			}
		});

		// FIX #2: direct change handler as safety net
		$loc_region.on("change", function() {
			var selected = $(this).val();
			if (selected && selected !== lastSentRegion) {
				lastSentRegion = selected;
				locationApi.setRegion(selected);
			}
		});

		$loc_planet.combobox({
			select: function(ev, data) {
				locationApi.setPlanet(data.item.value);
			}
		});

		// Fill the time-zone list BEFORE loading planet/region lists.
		// This ensures the <select> has options when
		// connectStelProperties() (in ui/mainui.js) binds to it.
		fillTimezoneList();

		// Bind to StelCore.flagUseCTZ to enable/disable the <select>.
		// We use a direct event listener rather than the generic
		// stelproperty handler because we need custom logic (disabling
		// the select) in addition to the checkbox state.
		$(propApi).on("stelPropertyChanged:StelCore.flagUseCTZ",
			function(evt, prop) {
				updateTimezoneSelectState(prop.value);
			});

		// Set the initial state.
		var initialUseCustom = propApi.getStelProp("StelCore.flagUseCTZ");
		updateTimezoneSelectState(initialUseCustom === true ||
		                          initialUseCustom === "true" ||
		                          initialUseCustom === 1 ||
		                          initialUseCustom === "1");
															
		locationApi.loadPlanetList(fillPlanetList);
		locationApi.loadRegionList(fillRegionList);

		// ---- Initial radius for Earth ----
		applyDefaultRadiusForPlanet("Earth");
		
		// ---- Initial coordinate display ----
		//
		// The latitude/longitude/altitude spinners will be filled by
		// the server on the first status poll, which triggers the
		// latitudeChanged / longitudeChanged / altitudeChanged
		// handlers above. To avoid an empty <span> for a moment, we
		// force one initial refresh.
		window.requestAnimationFrame(function() {
			updateCoordinateDisplays();
		});		
	}

	// =====================================================================
	// SERVER EVENT HANDLERS
	// =====================================================================

	$(locationApi).on("nameChanged", function(evt, name) {
		$loc_name.val(name);
	});

	$(locationApi).on("regionChanged", function(evt, region) {
		lastSentRegion = region;
		$loc_region.combobox("autocomplete", region);

		activeRegionFilter = region || null;

		if (allLocationsLoaded) {
			if (activeRegionFilter) {
				var filtered = filterLocationsByRegion(activeRegionFilter);
				console.log("[location] region filter='" + activeRegionFilter +
					"' -> " + filtered.length + " locations");
				renderLocationList(filtered.slice(0, 5000));
			} else {
				$loc_list.empty();
			}
		}
	});

	$(locationApi).on("planetChanged", function(evt, planet) {
		currentPlanet = planet;

		$loc_planet.combobox("autocomplete", planet);

		// Load the new planet map
		loadPlanetMap(planet);

		// Reset the radius to the planet-appropriate default
		applyDefaultRadiusForPlanet(planet);

		// Reload the region list whenever the planet changes
		locationApi.loadRegionList(fillRegionList, planet);
	});

	$(locationApi).on("altitudeChanged", function(evt, altitude) {
		$loc_altitude.spinner("value", altitude);
		updateCoordinateDisplays();		
	});

	$(locationApi).on("longitudeChanged", function(evt, longitude) {
		$loc_longitude.spinner("value", longitude);
		mapState.markerLon = longitude;
		scheduleRedraw();
		updateCoordinateDisplays();
	});

	$(locationApi).on("latitudeChanged", function(evt, latitude) {
		$loc_latitude.spinner("value", latitude);
		mapState.markerLat = latitude;
		scheduleRedraw();
		updateCoordinateDisplays();
		
		// The local (oblate-spheroid) radius depends on latitude, so
		// the kilometre equivalent of the current search radius must
		// be refreshed whenever the marker moves north or south.
		// Guard against re-entrancy: the spinner "spinchange" handler
		// also touches these values.
		if (!updatingRadiusSpinners) {
			updatingRadiusSpinners = true;
			$loc_radius_km.spinner("value",
				Math.round(planetData.degreesToKm(
					mapState.radiusDeg, currentPlanet, latitude)));
			updatingRadiusSpinners = false;
		}
	});

	$(locationApi).on("positionChanged", function(evt, latitude, longitude) {
		mapState.markerLat = latitude;
		mapState.markerLon = longitude;
		scheduleRedraw();
		updateCoordinateDisplays();		
	});

	$(initControls);
});