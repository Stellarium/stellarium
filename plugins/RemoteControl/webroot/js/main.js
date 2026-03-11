//This is the require.js main file for the main interface
requirejs.config({
	paths: {
		jquery: "jquery-3.7.1"
	},
	//prolong js timeout
	waitSeconds: 60,
	map:{
		//add some fixes to jquery ui
		"*":{
			"jquery-ui":"ui/jqueryuifixes"
		},
		//allow jquery fix to access jquery-ui
		"jquery.ui.touch-punch":{
			"jquery-ui":"jquery-ui"
		}
	},
	shim:{
		"globalize":{
			exports: "Globalize"
		},
		"jquery-ui":{
			deps: ["globalize"]
		},
		"jquery.ui.touch-punch":{
			deps: ["jquery-ui"]
		}
	}
});

// Load main UI module and Gamepad controller
require(["ui/mainui", "ui/gpcontroller"], function(mainui, gpcontroller) {
	"use strict";
	
	// Initialize gamepad controller after main UI is ready
	// The actual initialization will happen in mainui.js when UI is ready
});

require(["ui/mainui"]);