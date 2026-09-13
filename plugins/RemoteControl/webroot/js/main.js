/* ========================================================================
 * main.js - RequireJS Bootstrap for Stellarium Remote Control
 * ========================================================================
 * 
 * This is the RequireJS main configuration file. It sets up module paths,
 * shims, packages, and map rules for the entire application.
 * 
 * CodeMirror 5 is loaded as an AMD package (via the `packages` option).
 * All CodeMirror addons are mapped through the `paths` option to allow
 * short, readable dependency names in modules like scriptEditor.js.
 * 
 * Important notes:
 * - The `packages` option is used for CodeMirror itself, as recommended
 *   by the official CodeMirror 5 documentation.
 * - The `map` option is used to fix CodeMirror's internal relative paths
 *   (like "../../lib/codemirror") which would otherwise be resolved
 *   relative to the baseUrl and cause 404 errors.
 * - Addon paths use short aliases prefixed with `cm-` to keep
 *   dependency arrays in consumer modules concise.
 * 
 * @license GPLv2+
 * ======================================================================== */

//This is the require.js main file for the main interface
requirejs.config({

	// BASE PATHS
	paths: {
			jquery: "jquery-4.0.0",
			
			// ================================================================
			// CodeMirror Addons - Short Aliases
			// ================================================================
			// These aliases map to actual CodeMirror addon files. They are
			// used in dependency arrays as "cm-addon-xxx" for readability.
			// ----------------------------------------------------------------
			
			// Modes
			"cm-mode-javascript":     "/js/scripteditor/codemirror/mode/javascript/javascript",
			
			// Edit addons
			"cm-addon-continuelist":  "/js/scripteditor/codemirror/addon/edit/continuelist",
			"cm-addon-matchbrackets": "/js/scripteditor/codemirror/addon/edit/matchbrackets",
			"cm-addon-closebrackets": "/js/scripteditor/codemirror/addon/edit/closebrackets",
			
			// Fold addons
			"cm-addon-foldcode":      "/js/scripteditor/codemirror/addon/fold/foldcode",
			"cm-addon-foldgutter":    "/js/scripteditor/codemirror/addon/fold/foldgutter",
			"cm-addon-brace-fold":    "/js/scripteditor/codemirror/addon/fold/brace-fold",
			"cm-addon-indent-fold":   "/js/scripteditor/codemirror/addon/fold/indent-fold",
			
			// Selection addons
			"cm-addon-active-line":   "/js/scripteditor/codemirror/addon/selection/active-line",
			
			// Hint addons
			"cm-addon-show-hint":       "/js/scripteditor/codemirror/addon/hint/show-hint",
			"cm-addon-javascript-hint": "/js/scripteditor/codemirror/addon/hint/javascript-hint",
			
			// Comment addon
			"cm-addon-comment":       "/js/scripteditor/codemirror/addon/comment/comment",
			
			// Search addons
			"cm-addon-searchcursor":  "/js/scripteditor/codemirror/addon/search/searchcursor",
			"cm-addon-search":        "/js/scripteditor/codemirror/addon/search/search",
			"cm-addon-dialog":        "/js/scripteditor/codemirror/addon/dialog/dialog",
			
			// Display addons
			"cm-addon-placeholder":   "/js/scripteditor/codemirror/addon/display/placeholder"
	},
	//prolong js timeout
	waitSeconds: 60,
	
	// ====================================================================
	// MAP RULES
	// ====================================================================
	// The `map` option is used to fix CodeMirror's internal relative
	// paths. CodeMirror 5 addons use relative paths like:
	//   "../../lib/codemirror"  (for the core module)
	//   "../dialog/dialog"      (for the dialog addon)
	//   "foldcode"              (for the foldcode addon)
	//   "searchcursor"          (for the searchcursor addon)
	// 
	// Without these map rules, RequireJS resolves them relative to the
	// baseUrl (/js/) instead of the addon's own location, resulting in
	// 404 errors.
	// 
	// The map rules below redirect those relative paths to the correct
	// absolute paths within the CodeMirror package.
	// ----------------------------------------------------------------
	map: {
		"*": {
			
			//add some fixes to jquery ui
			"jquery-ui": "ui/jqueryuifixes",
				
			// CodeMirror internal relative path fixes
			"../../lib/codemirror": "codemirror",
			"../dialog/dialog":     "cm-addon-dialog",
			"../foldcode":             "cm-addon-foldcode",
			"../searchcursor":         "cm-addon-searchcursor",
			"lib/codemirror":       "codemirror"
		},
		
		//allow jquery fix to access jquery-ui
		"jquery.ui.touch-punch": {
				"jquery-ui": "jquery-ui"
		}
	},
	
	// SHIMS
	shim: {
			"globalize": {
					exports: "Globalize"
			},
			"jquery-ui": {
					deps: ["globalize"]
			},
			"jquery.ui.touch-punch": {
					deps: ["jquery-ui"]
			}
	},
	
	// ====================================================================
	// PACKAGES
	// ====================================================================
	// CodeMirror 5 is loaded as a package. According to the official
	// documentation, the `packages` option must be used (instead of
	// `paths`) so that CodeMirror can resolve its own submodules
	// (modes and addons) through relative paths.
	// ----------------------------------------------------------------
	packages: [
			{
					name: "codemirror",
					location: "/js/scripteditor/codemirror",
					main: "lib/codemirror"
			}
	]
});

// Load main UI module
require(["ui/mainui"], function(mainui) {
	"use strict";
		console.log("[main] Stellarium Remote Control UI loaded");
		
	// Initialize gamepad controller after main UI is ready
	// The actual initialization will happen in mainui.js when UI is ready
});
