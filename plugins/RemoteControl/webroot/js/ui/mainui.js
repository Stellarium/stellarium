/* jshint expr: true */

define(["jquery", "settings", "globalize", "api/remotecontrol", "api/actions",
	"api/properties", "./time", "./joystickqueue", "./actions", "./viewoptions",
	"./scripts",
	"./viewcontrol", "./location", "./search", "jquery-ui"
], function($, settings, globalize, rc, actionApi, propApi, timeui,
	JoystickQueue) {
	"use strict";

	var animationSupported = (window.requestAnimationFrame !== undefined);
	//controls
	var $noresponse;
	var $noresponsetime;

	var activeTab = 0;
	//keep preloaded images to prevent browser from releasing them
	var preloadedImgs = [];


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

	// create
	function createAutomaticGUIElements() {
		console.log("setting up GUI elements");

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

		//create jquery ui buttons + selectmenu
		$("button.jquerybutton").button();
		$("select.selectmenu").selectmenu({
			width: 'auto'
		});

		//setup joysticks
		$(".joystickcontainer > .joystick").each(function() {
			var joy = $(this);
			new JoystickQueue(joy, joy.data("joyurl"), joy.data("joymax"));
		});
	}

	function connectStelProperties() {
		//hook up automatic stelproperty spinners
		$("input.spinner.stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");
			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				if (!self.data("updatePaused"))
					self.spinner("value", prop.value);
			});
			self.spinner("value", propApi.getStelProp(prop));

			self.on("focus", function(evt) {
				self.data("updatePaused", true);
			});

			self.on("blur", function(evt) {
				self.data("updatePaused", false);
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
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
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
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
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
			var elem = this;
			var self = $(this);
			var prop = self.data("prop");
			var numberformat = self.data("numberformat");

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}

			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				var val = prop.value;
				if (numberformat)
					val = globalize.format(val, numberformat);
				elem.textContent = val;
			});
			var val = propApi.getStelProp(prop);
			if (numberformat)
				val = globalize.format(val, numberformat);
			elem.textContent = val;
		});

		$("select.stelproperty").each(function() {
			var self = $(this);
			var prop = self.attr("name");

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
			}
			$(propApi).on("stelPropertyChanged:" + prop, function(evt, prop) {
				self.val(prop.value);
				self.data("currentSelection", prop.value);
				//if this is a jquery UI selectmenu, we have to refresh
				if (self.hasClass('selectmenu')) {
					self.selectmenu("refresh");
				}
			});
			var curVal = propApi.getStelProp(prop);
			self.val(curVal);
			//store the selection value also in the element itself
			self.data("currentSelection", curVal);
			self.on("change selectmenuchange", function(evt) {
				propApi.setStelProp(prop, self.val());
			});
		});

		//stelproperty direct value change
		$("button.stelproperty, input[type='button'].stelproperty").click(function() {
			var prop = this.name;
			var val = this.value;

			if (!prop) {
				console.error(
					'Error: no StelProperty name defined on an "stelproperty" element, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no StelProperty name defined on an "stelproperty" element, see log for details'
				);
				return;
			}

			if (!val) {
				console.error(
					'Error: no value defined for an "stelproperty" button, element follows...'
				);
				console.dir(this);
				alert(
					'Error: no value defined for an "stelproperty" button, see log for details'
				);
				return;
			}

			propApi.setStelProp(prop, val);
		});
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

		//create and connect automatic GUI elements defined in the DOM
		createAutomaticGUIElements();
		connectStelProperties();

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
		$(rc).trigger("uiReady"); //signal other components that the main UI init is done (some may need the jQueryUI stuff set up)
	});

	//new server data
	$(rc).on('serverDataReceived', function(event, data) {
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
