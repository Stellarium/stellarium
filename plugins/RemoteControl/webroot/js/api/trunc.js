define(function() {
	"use strict";
	
	//ensure that the browser has a trunc function
	Math.trunc = Math.trunc || function(x) {
		return x < 0 ? Math.ceil(x) : Math.floor(x);
	};
});