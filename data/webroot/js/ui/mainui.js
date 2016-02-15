/* jshint expr: true */

define(["jquery", "settings", "api/remotecontrol", "api/actions", "api/properties", "./time", "./actions", "./viewoptions", "./scripts", "./viewcontrol", "./location", "./search", "jquery-ui"], function($, settings, rc, actionApi, propApi, timeui) {
	"use strict";

	var animationSupported = (window.requestAnimationFrame !== undefined);
	//controls
	var $noresponse;
	var $noresponsetime;

	var sel_infostring;

	var activeTab = 0;
	//keep preloaded images to prevent browser from releasing them
	var preloadedImgs=[];


	if (!animationSupported) {
		console.log("animation frame not supported");
	} else {
		console.log("animation frame supported");
	}

	function animate() {

		if (activeTab === 0)
			timeui.updateTimeDisplay();

		if (rc.isConnectionLost()) {
			var elapsed = ($.now() - rc.getLastDataTime()) / 1000;
			var text = Math.floor(elapsed).toString();
			$noresponsetime[0].textContent = text;
		}

		if (settings.useAnimationFrame && animationSupported) {
			window.requestAnimationFrame(animate);
		} else {
			setTimeout(animate, settings.animationDelay);
		}
	}

	//DOM-ready
	$(function() {
		//preload the error images, otherwise they may be loaded when the connection is lost, which of course wont work

		var preLoadImages = [
			"/external/images/ui-icons_fbc856_256x240.png",
			"/external/images/ui-bg_glass_35_dddddd_1x400.png"
		];

		preLoadImages.forEach(function(val) {
			var img = new Image();
			img.src = val;
			preloadedImgs.push(img);
		});

		//find and setup some controls
		$noresponse = $("#noresponse");
		$noresponsetime = $("#noresponsetime");

		$noresponse.dialog({
			autoOpen: false,
			modal: true,
			draggable: false,
			resizable: false,
			dialogClass: "fixed-dialog ui-state-error"
		});

		$(window).resize(function() {
			$noresponse.dialog("option", "position", {
				my: "center",
				at: "center",
				of: window
			});
		});

		//automatically setup spinners
		$("input.spinner").each(function() {
			var self = $(this),
				min = self.data("min"),
				max = self.data("max"),
				step = self.data("step"),
				format = self.data("numberformat");
			self.spinner({
				min: min,
				max: max,
				step: step,
				numberFormat: format
			});
		});

		//setup sliders
		$("div.slider").each(function() {
			var self = $(this),
				min = self.data("min"),
				max = self.data("max"),
				step = self.data("step");
			self.slider({
				min: min,
				max: max,
				step: step
			});
		});

		//hook up automatic stelproperty spinners
		$("input.spinner.stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");
			if (!prop) {
				console.error('Error: no StelProperty name defined on an "stelproperty" element, element follows...');
				console.dir(this);
				alert('Error: no StelProperty name defined on an "stelproperty" element, see log for details');
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				if(!self.data("updatePaused"))
					self.spinner("value", prop.value);
			});
			self.spinner("value", propApi.getStelProp(prop));

			self.on("focus",function(evt){
				self.data("updatePaused",true);
			});

			self.on("blur",function(evt){
				self.data("updatePaused",false);
			});

			self.on("spinuserinput", function(evt, ui) {
				propApi.setStelPropQueued(prop, ui.value);
			});
		});

		//hook up stelproperty checkboxes
		$("input[type='checkbox'].stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");
			if (!prop) {
				console.error('Error: no StelProperty name defined on an "stelproperty" element, element follows...');
				console.dir(this);
				alert('Error: no StelProperty name defined on an "stelproperty" element, see log for details');
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self[0].checked = prop.value;
			});
			self[0].checked = propApi.getStelProp(prop);
			self.click(function() {
				propApi.setStelProp(prop, this.checked);
			});
		});

		$("div.slider.stelproperty").each(function() {
			var self = $(this);
			var prop = self.data("prop");
			if (!prop) {
				console.error('Error: no StelProperty name defined on an "stelproperty" element, element follows...');
				console.dir(this);
				alert('Error: no StelProperty name defined on an "stelproperty" element, see log for details');
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self.slider("value", prop.value);
			});
			self.slider("value", propApi.getStelProp(prop));
			self.on("slide", function(evt, ui) {
				propApi.setStelPropQueued(prop, ui.value);
			});
		});

		//hook up span stelproperty display
		$("span.stelproperty").each(function() {
			var self = $(this);
			var prop = self.data("prop");

			if (!prop) {
				console.error('Error: no StelProperty name defined on an "stelproperty" element, element follows...');
				console.dir(this);
				alert('Error: no StelProperty name defined on an "stelproperty" element, see log for details');
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self.text(prop.value);
			});
			self.text(propApi.getStelProp(prop));
		});

		//create jquery ui buttons
		$("button.jquerybutton").button();

		//main tabs
		//remember which tab was active after refresh by storing id in sessionstore
		var oldTabId = 0;
		var tabDataKey = "activeMainTab";

		var webStorageSupported = typeof(Storage) !== "undefined";

		if (webStorageSupported) {
			oldTabId = parseInt(sessionStorage.getItem(tabDataKey), 10);
			if (isNaN(oldTabId))
				oldTabId = 0;
		} else {
			console.log("webstorage API unsupported");
		}

		activeTab = oldTabId;

		var $tabs = $("#tabs");
		$tabs.tabs({
			active: oldTabId,
			activate: function(evt, ui) {
				var idx = ui.newTab.index();
				activeTab = idx;
				if (webStorageSupported) {
					sessionStorage.setItem(tabDataKey, idx);
				}
			}
		});

		sel_infostring = document.getElementById("sel_infostring");

		var $loading = $("#loadindicator").hide(),
			timer;
		$(document).ajaxStart(function() {
			timer && clearTimeout(timer);
			timer = setTimeout(function() {
				$loading.show();
			}, settings.spinnerDelay);
		});
		$(document).ajaxStop(function() {
			clearTimeout(timer);
			$loading.hide();
		});

		//start animation & update loop
		animate();
		rc.startUpdateLoop();

		$("#loadoverlay").fadeOut();
	});

	//new server data
	$(rc).on('serverDataReceived', function(event, data) {
		if (data.selectioninfo) {
			sel_infostring.innerHTML = data.selectioninfo;
			sel_infostring.className = "";
		} else {
			sel_infostring.innerHTML = rc.tr("No current selection");
			sel_infostring.className = "bold";
		}

		//this will get reset after the event is processed
		//this means the connection WAS lost before, but now is not anymore
		if (rc.isConnectionLost()) {
			$noresponse.dialog("close");
		}
	});

	$(rc).on("serverDataError", function(evt) {
		if (!rc.isConnectionLost()) {
			$noresponse.dialog("open");
		}
	});

});