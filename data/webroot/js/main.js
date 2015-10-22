//This is the require.js main file for the main interface
requirejs.config({
	paths: {
		jquery: "jquery-1.11.3"
	},
	map:{
		//add some fixes to jquery ui
		"*":{
			"jquery-ui":"ui/jqueryuifixes"
		},
		//allow jquery fix to access jquery-ui
		"ui/jqueryuifixes":{
			"jquery-ui":"jquery-ui"
		}
	}
});

require(["ui/mainui"]);