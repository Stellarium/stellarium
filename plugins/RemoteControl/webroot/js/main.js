//This is the require.js main file for the main interface
requirejs.config({
	paths: {
		jquery: "jquery-3.5.0"
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

require(["ui/mainui"]);