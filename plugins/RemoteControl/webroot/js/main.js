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
		},
		// CodeMirror is loaded via script tags, not as AMD module
		// We just need to ensure it's available
		"codemirror": {
			exports: "CodeMirror"
		}
	},
		// Ensure CodeMirror is recognized as a global
	packages: [
		{
			name: "codemirror",
			location: "/js/scripteditor/codemirror/js",
			main: "codemirror.min"
		}
	]
});

// Load main UI module
require(["ui/mainui"], function(mainui) {
	"use strict";
		// Check if CodeMirror is loaded globally
	if (typeof CodeMirror === 'undefined') {
		console.warn('[main] CodeMirror not loaded yet, waiting...');
		// CodeMirror will be available from script tags
	}
	// Initialize gamepad controller after main UI is ready
	// The actual initialization will happen in mainui.js when UI is ready
});