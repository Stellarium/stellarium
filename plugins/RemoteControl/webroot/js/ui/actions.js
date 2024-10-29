define(["jquery", "api/remotecontrol", "api/actions"], function($, rc, actionApi) {
	"use strict";

	var $actionlist;

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

	$(function() {
		initControls();
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

		//init stelaction checkboxes
		$("input[type='checkbox'].stelaction").each(function() {
			var self = $(this);
			var id = this.name;

			if (!id) {
				console.error('Error: no StelAction name defined on an "stelaction" element, element follows...');
				console.dir(this);
				alert('Error: no StelAction name defined on an "stelaction" element, see log for details');
			}

			self[0].checked = actionApi.isChecked(id);
		});

		$(document).on("click","input[type='checkbox'].stelaction, input[type='button'].stelaction, button.stelaction",function(){
			actionApi.execute(this.name);
		});
	});

	$(actionApi).on("stelActionChanged",function(evt,id,data){
		var option = $actionlist.find('option[value="'+id+'"]');
		if(option.length>0)
			option[0].textContent = data.text + " (" + (data.isChecked ? rc.tr("on") : rc.tr("off")) + ")";

		//update checkboxes
		$("input[type='checkbox'][name='"+id+"'].stelaction").prop("checked",data.isChecked);
		$("input[type='button'][name='"+id+"'].stelaction, button[name='"+id+"'].stelaction").toggleClass("active",data.isChecked);
	});

});