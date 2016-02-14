define(["jquery", "api/viewcontrol", "api/viewoptions", "api/actions", "jquery-ui"], function($, viewControlApi, viewOptionApi, actionApi) {
	"use strict";

	var $view_fov;
	var view_fov_text;
	var view_projection;

	var minFov = 0.001389;
	//TODO make this depend on current projection
	var maxFov = 360;
	var fovSteps = 1000;

	//sets the FOV slider from a given fov
	function setFovSlider(fov) {
		//inverse of handleFovSlide
		var val = Math.pow(((fov - minFov) / (maxFov - minFov)), 1 / 4);

		var slVal = Math.round(val * fovSteps);
		$view_fov.slider("value", fovSteps - slVal);
	}

	//convert fov slider to fov value and queue update
	function handleFovSlide(val) {
		var s = (fovSteps - val) / fovSteps;
		var fov = minFov + Math.pow(s, 4) * (maxFov - minFov);

		console.log(val + " / " + fov);

		setFovText(fov);
		viewControlApi.setFOV(fov);
	}

	function setFovText(fov) {
		view_fov_text.textContent = fov.toPrecision(3);
	}

	function initControls() {
		$("#view_upleft").mousedown(viewControlApi.moveUpLeft);
		$("#view_up").mousedown(viewControlApi.moveUp);
		$("#view_upright").mousedown(viewControlApi.moveUpRight);
		$("#view_left").mousedown(viewControlApi.moveLeft);
		$("#view_right").mousedown(viewControlApi.moveRight);
		$("#view_downleft").mousedown(viewControlApi.moveDownLeft);
		$("#view_down").mousedown(viewControlApi.moveDown);
		$("#view_downright").mousedown(viewControlApi.moveDownRight);

        // GZ TODO: How would I initialize those buttons? 
		$("#fov_1").mouseup(viewControlApi.setFOV(1));
		$("#fov_10").mouseup(viewControlApi.setFOV(10));
		$("#fov_20").mouseup(viewControlApi.setFOV(20));
		$("#fov_30").mouseup(viewControlApi.setFOV(30));
		$("#fov_40").mouseup(viewControlApi.setFOV(40));
		$("#fov_60").mouseup(viewControlApi.setFOV(60));
		$("#fov_90").mouseup(viewControlApi.setFOV(90));
		$("#fov_120").mouseup(viewControlApi.setFOV(120));
		$("#fov_150").mouseup(viewControlApi.setFOV(150));
		$("#fov_180").mouseup(viewControlApi.setFOV(180));
		$("#fov_230").mouseup(viewControlApi.setFOV(230));

		$("#view_controls div").on("mouseup mouseleave", viewControlApi.stopMovement);

		$view_fov = $("#view_fov");
		$view_fov.slider({
			min: 0,
			max: fovSteps,

			slide: function(evt, ui) {
				handleFovSlide(ui.value);
			},
			start: function(evt, ui) {
				console.log("slide start");
			},
			stop: function(evt, ui) {
				console.log("slide stop");
			}
		});

		$("#view_center").click(function(evt) {
			actionApi.execute("actionGoto_Selected_Object");
		});

		view_fov_text = document.getElementById("view_fov_text");
		view_projection = document.getElementById("view_projection");
	}

	$(viewOptionApi).on("projectionChanged", function(evt, proj, str) {
		view_projection.textContent = str;
	});

	$(viewControlApi).on("fovChanged", function(evt, fov) {
		setFovText(fov);
		setFovSlider(fov);
	});

	$(initControls);
});