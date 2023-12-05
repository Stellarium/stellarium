define(["jquery", "api/scripts", "api/remotecontrol"], function($, scriptApi, rc) {
	"use strict";

	//Private variables
	var $activescript;
	var $bt_runscript;
	var $bt_stopscript;
	var $scriptlist;
	var $scriptinfo;

	function fillScriptList(data) {
		//document must be ready
		$(function() {
			$scriptlist.empty();

			//sort it and insert
			$.each(data.sort(), function(idx, elem) {
				$("<option/>").text(elem).val(elem).appendTo($scriptlist);
			});
		});
	}

	//initialize the automatic script buttons
	function initScriptButtons() {
		$("button.stelssc, input[type='button'].stelssc").click(function() {
			var code = this.value;
			if (!code) {
				console.error("No code defined for this script button");
				return;
			}

			var useIncludes = false;
			if($(this).data("useIncludes"))
				useIncludes = true;

			scriptApi.runDirectScript(code,useIncludes);
		});
	}

	function initControls() {
		$scriptlist = $("#scriptlist");
		$bt_runscript = $("#bt_runscript");
		$bt_stopscript = $("#bt_stopscript");
		$activescript = $("#activescript");
		$scriptinfo = $("#scriptinfo");

		var runscriptfn = function() {
			var selection = $scriptlist.val();
			if (selection) {
				scriptApi.runScript(selection);
			}
		};

		$scriptlist.change(function() {
			var selection = $scriptlist.children("option").filter(":selected").val();

			$scriptinfo.attr('src', "/api/scripts/info?html=true&id="+selection);


			$bt_runscript.prop({
				disabled: false
			});
			console.log("selected: " + selection);
		}).dblclick(runscriptfn);

		$bt_runscript.click(runscriptfn);
		$bt_stopscript.click(scriptApi.stopScript);

		initScriptButtons();
	}

	$(scriptApi).on("activeScriptChanged", function(evt, script) {
		if (script) {
			$activescript.text(script);
		} else
			$activescript.text(rc.tr("-none-"));
		$bt_stopscript.prop({
			disabled: !script
		});

		$scriptlist.children("option.select_selected").removeClass("select_selected");
		$scriptlist.val(script);
		$scriptlist.children("option[value='" + script + "']").addClass("select_selected");
	});

	//queue script list loading as soon as this script is loaded instead of ready event
	scriptApi.loadScriptList(fillScriptList);
	$(initControls);

});