define(["jquery", "api/scripts", "api/remotecontrol"], function($, scriptApi, rc) {
	"use strict";

	//Private variables
	var $activescript;
	var $bt_runscript;
	var $bt_stopscript;
	var $scriptlist;

	function fillScriptList(data) {
		$scriptlist.empty();

		//sort it and insert
		$.each(data.sort(), function(idx, elem) {
			$("<option/>").text(elem).val(elem).appendTo($scriptlist);
		});
	}

	function initControls() {
		$scriptlist = $("#scriptlist");
		$bt_runscript = $("#bt_runscript");
		$bt_stopscript = $("#bt_stopscript");
		$activescript = $("#activescript");

		var runscriptfn = function() {
			var selection = $scriptlist.children("option").filter(":selected").val();
			if (selection) {
				scriptApi.runScript(selection);
			}
		};

		$scriptlist.change(function() {
			var selection = $scriptlist.children("option").filter(":selected").val();

			$bt_runscript.prop({
				disabled: false
			});
			console.log("selected: " + selection);
		}).dblclick(runscriptfn);

		$bt_runscript.dblclick(runscriptfn);
		$bt_stopscript.click(scriptApi.stopScript);

		scriptApi.loadScriptList(fillScriptList);
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

	$(initControls);

});