define(["jquery", "api/remotecontrol", "api/actions"], function($, rc, actionApi) {
	"use strict";

	var $actionlist;

	//this is a list of all actions together with their button image
	//that automatically should be created and connected
	var buttonActions = [
		[
			"actionShow_Constellation_Lines", "btConstellationLines"
		],
		[
			"actionShow_Constellation_Labels", "btConstellationLabels"
		],
		[
			"actionShow_Constellation_Art", "btConstellationArt"
		],
		[
			"actionShow_Equatorial_Grid", "btEquatorialGrid"
		],
		[
			"actionShow_Azimuthal_Grid", "btAzimuthalGrid"
		],
		[
			"actionShow_Ground", "btGround"
		],
		[
			"actionShow_Cardinal_Points", "btCardinalPoints"
		],
		[
			"actionShow_Atmosphere", "btAtmosphere"
		],
		[
			"actionShow_Nebulas", "btNebula"
		],
		[
			"actionShow_Planets_Labels", "btPlanets"
		],
		[
			"actionSwitch_Equatorial_Mount", "btEquatorialMount"
		],
		[ //this is actually a different action than the main GUI, but toggles properly (which the one from the GUI does not for some reason)
			"actionSet_Tracking", "btGotoSelectedObject"
		],
		[
			"actionShow_Night_Mode", "btNightView"
		],
		[
			"actionSet_Full_Screen_Global", "btFullScreen"
		]
	];

	function initControls() {
		$actionlist = $("#actionlist");

		var runactionfn = function() {
			var e = $actionlist[0];
			var id = e.options[e.selectedIndex].value;

			actionApi.execute(id);
		};

		$actionlist.change(function() {
			$("#bt_doaction").prop("disabled", false);
		}).dblclick(runactionfn);

		$("#bt_doaction").click(runactionfn);
	}


	function createAutoButtons() {
		var $ul = $("#autobuttons");
		var parent = $ul.parent();
		$ul.detach();

		buttonActions.forEach(function(val) {
			var btn = document.createElement("button");
			btn.className = "icon32 " + val[1];
			btn.value = val[0];

			var li = document.createElement("li");
			li.appendChild(btn);

			$ul.append(li);
		});

		//this automatically connects to StelAction
		actionApi.connectActionContainer($ul);

		$ul.appendTo(parent);
	}

	$(function() {
		initControls();
		createAutoButtons();
	});

	$(actionApi).on("actionListLoaded", function(evt, data) {
		//fill the action list
		var parent = $actionlist.parent();
		$actionlist.detach();
		$actionlist.empty();

		$.each(data, function(key, val) {
			//the key is an optgroup
			var group = $("<optgroup/>").attr("label", key);
			//the val is an array of StelAction objects
			$.each(val, function(idx, v2) {

				var option = document.createElement("option");
				option.value = v2.id;

				if (v2.isCheckable) {
					option.className = "checkableaction";
					option.textContent = v2.text + " (" + (v2.isChecked ? rc.tr("on") : rc.tr("off")) + ")";
				} else {
					option.textContent = v2.text;
				}

				group.append(option);
			});
			group.appendTo($actionlist);
		});
		$actionlist.prependTo(parent);
	});

	$(actionApi).on("stelActionChanged",function(evt,id,data){

		var option = $actionlist.find('option[value="'+id+'"]');
		if(option.length>0)
			option[0].textContent = data.text + " (" + (data.isChecked ? rc.tr("on") : rc.tr("off")) + ")";
	});

});