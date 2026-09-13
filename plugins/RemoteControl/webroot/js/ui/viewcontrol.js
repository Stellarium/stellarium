define(["jquery", "api/viewcontrol", "api/viewoptions", "api/actions", "jquery-ui"], function($, viewControlApi, viewOptionApi, actionApi) {
	"use strict";

	var $view_fov;
	var view_fov_text;

	var minFov = 0.001389;
	//TODO make this depend on current projection
	var maxFov = 360;
	var fovSteps = 1000;

	// Zoom speed presets (adjusted for logarithmic scaling)
	var ZOOM_SPEEDS = {
		SLOW: 0.5,
		MEDIUM: 1.5,
		FAST: 3.0
	};
	var currentZoomSpeed = ZOOM_SPEEDS.MEDIUM;

	// ============================================================
	// FOV <-> SLIDER CONVERSION (Logarithmic)
	// ============================================================

	/**
	 * Convert FOV to slider value (logarithmic scale)
	 */
	function fovToSlider(fov) {
		var normalized = (fov - minFov) / (maxFov - minFov);
		var val = Math.pow(normalized, 1 / 4);
		return Math.round(val * fovSteps);
	}

	/**
	 * Convert slider value to FOV (inverse logarithmic)
	 */
	function sliderToFov(sliderVal) {
		var s = (fovSteps - sliderVal) / fovSteps;
		return minFov + Math.pow(s, 4) * (maxFov - minFov);
	}

	//sets the FOV slider from a given fov
	function setFovSlider(fov) {
		var slVal = fovToSlider(fov);
		$view_fov.slider("value", fovSteps - slVal);
	}

	//convert fov slider to fov value and queue update
	function handleFovSlide(val) {
		var fov = sliderToFov(val);
		console.log(val + " / " + fov);
		setFovText(fov);
		viewControlApi.setFOV(fov);
	}

	function setFovText(fov) {
		view_fov_text.textContent = fov.toPrecision(3);
	}

	// ============================================================
	// LOGARITHMIC ZOOM FUNCTION
	// ============================================================

	/**
	 * Apply logarithmic zoom to FOV.
	 * Uses slider space for smooth, natural zoom behavior.
	 * 
	 * @param {number} currentFov - Current FOV value
	 * @param {number} direction - -1 (zoom in) or +1 (zoom out)
	 * @param {number} speed - Zoom speed multiplier
	 * @returns {number} New FOV value
	 */
	function applyLogarithmicZoom(currentFov, direction, speed) {
		// Convert current FOV to slider space
		var currentSlider = fovSteps - fovToSlider(currentFov);
		
		// Calculate step in slider space (logarithmic)
		// Speed is scaled to provide smooth feel across all FOV ranges
		var step = speed * 8;  // Base step in slider units
		
		// Apply change in slider space
		var newSlider = currentSlider + (direction * step);
		
		// Clamp to valid range
		newSlider = Math.max(0, Math.min(fovSteps, newSlider));
		
		// Convert back to FOV
		var newFov = sliderToFov(newSlider);
		
		return newFov;
	}

	// ============================================================
	// MOUSE WHEEL SUPPORT FOR FOV SLIDER WITH ADAPTIVE STEP
	// ============================================================
	/**
	 * Enable mouse wheel support for FOV slider with adaptive step size.
	 * Step size adapts to current FOV value for precise control.
	 */
	function enableFovSliderMouseWheel($slider, options) {
		if (!$slider || !$slider.length) return;

		options = options || {};
		var minFovVal = options.minFov || minFov;
		var maxFovVal = options.maxFov || maxFov;

		$slider.off('wheel.fovSlider');

		$slider.on('wheel.fovSlider', function(e) {
			e.preventDefault();
			e.stopPropagation();

			var currentFov = parseFloat(view_fov_text.textContent);
			if (isNaN(currentFov)) currentFov = 60;

			var delta = e.originalEvent.deltaY;
			var direction = delta < 0 ? 1 : -1;

			// Use logarithmic zoom with fixed step in slider space
			var speed = 0.5;  // Small step for precise control
			var newFov = applyLogarithmicZoom(currentFov, direction, speed);

			newFov = Math.max(minFovVal, Math.min(maxFovVal, newFov));

			if (Math.abs(newFov - currentFov) > 0.0001) {
				setFovText(newFov);
				setFovSlider(newFov);
				viewControlApi.setFOV(newFov);
			}

			return false;
		});

		console.log('[ViewControl] Logarithmic mouse wheel enabled for FOV slider');
	}

	// ============================================================
	// MOUSE WHEEL SUPPORT FOR JOYSTICK AREA
	// ============================================================
	/**
	 * Enable mouse wheel zoom control on joystick container area.
	 * Uses logarithmic scaling for smooth zoom behavior across all FOV ranges.
	 * Scroll forward (up) = Zoom In, Scroll backward (down) = Zoom Out (Stellarium GUI style)
	 */
	function enableJoystickMouseWheelZoom($container, options) {
		if (!$container || !$container.length) return;

		options = options || {};
		var speed = options.speed || currentZoomSpeed;
		var minFovVal = options.minFov || minFov;
		var maxFovVal = options.maxFov || maxFov;
		var throttleDelay = options.throttleDelay || 50;

		var isActive = false;
		var timeout = null;

		$container.off('wheel.joystickZoom');

		$container.on('wheel.joystickZoom', function(e) {
			// Prevent page scrolling
			e.preventDefault();
			e.stopPropagation();

			// Throttle rapid events
			if (isActive) return;
			isActive = true;

			var currentFov = parseFloat(view_fov_text.textContent);
			if (isNaN(currentFov)) currentFov = 60;

			var delta = e.originalEvent.deltaY;
			// Stellarium GUI style: scroll forward (delta < 0) = Zoom In (decrease FOV)
			// scroll backward (delta > 0) = Zoom Out (increase FOV)
			var direction = delta < 0 ? 1 : -1;

			// Apply logarithmic zoom
			var newFov = applyLogarithmicZoom(currentFov, direction, speed);
			newFov = Math.max(minFovVal, Math.min(maxFovVal, newFov));

			if (Math.abs(newFov - currentFov) > 0.0001) {
				setFovText(newFov);
				setFovSlider(newFov);
				viewControlApi.setFOV(newFov);
			}

			clearTimeout(timeout);
			timeout = setTimeout(function() {
				isActive = false;
			}, throttleDelay);

			return false;
		});

		// Prevent default touch actions to avoid conflicts
		$container.css('touch-action', 'none');

		console.log('[ViewControl] Logarithmic mouse wheel zoom enabled for joystick area (Speed: ' + speed + ')');
	}

	// ============================================================
	// ZOOM SPEED CONTROLS
	// ============================================================
	/**
	 * Initialize zoom speed controls from static HTML structure.
	 * Binds change event to radio buttons for updating zoom speed.
	 */
	function initZoomSpeedControls() {
		var $container = $('#zoom-speed-controls');
		if (!$container || !$container.length) return;

		// Read initial checked value
		var $checked = $container.find('input[name="zoomSpeed"]:checked');
		if ($checked.length) {
			var initialSpeed = parseFloat($checked.val());
			if (!isNaN(initialSpeed) && initialSpeed > 0) {
				currentZoomSpeed = initialSpeed;
				console.log('[ViewControl] Initial zoom speed loaded:', currentZoomSpeed);
			}
		}

		// Bind change event
		$container.find('input[name="zoomSpeed"]').on('change', function() {
			var speed = parseFloat($(this).val());
			if (!isNaN(speed) && speed > 0) {
				currentZoomSpeed = speed;
				console.log('[ViewControl] Zoom speed changed to:', speed);
				
				// Update all joystick containers with new speed
				updateJoystickZoomSpeed(speed);
			}
		});
	}

	/**
	 * Update zoom speed on all joystick containers.
	 * Re-initializes the mouse wheel handlers with new speed.
	 */
	function updateJoystickZoomSpeed(speed) {
		$('.joystickcontainer').each(function() {
			var $container = $(this);
			// Remove old handlers
			$container.off('wheel.joystickZoom');
			// Re-enable with new speed
			enableJoystickMouseWheelZoom($container, {
				speed: speed,
				minFov: minFov,
				maxFov: maxFov
			});
		});
	}

	function initControls() {
		//note: this is the "old" style joystick code, the new one is handled with the "joystick" css class automatically
		$("#view_upleft").mousedown(viewControlApi.moveUpLeft);
		$("#view_up").mousedown(viewControlApi.moveUp);
		$("#view_upright").mousedown(viewControlApi.moveUpRight);
		$("#view_left").mousedown(viewControlApi.moveLeft);
		$("#view_right").mousedown(viewControlApi.moveRight);
		$("#view_downleft").mousedown(viewControlApi.moveDownLeft);
		$("#view_down").mousedown(viewControlApi.moveDown);
		$("#view_downright").mousedown(viewControlApi.moveDownRight);

		//initialize FOV buttons
		$("button.fovbutton").click(function(event){
			viewControlApi.setFOV(this.value);
		});

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

		// ENABLE MOUSE WHEEL SUPPORT FOR FOV SLIDER
		enableFovSliderMouseWheel($view_fov, {
			minFov: minFov,
			maxFov: maxFov
		});

		// ============================================================
		// ENABLE MOUSE WHEEL ZOOM ON JOYSTICK AREA
		// ============================================================
		$('.joystickcontainer').each(function() {
			var $container = $(this);
			var speed = parseFloat($container.data('zoom-speed')) || currentZoomSpeed;
			enableJoystickMouseWheelZoom($container, {
				speed: speed,
				minFov: minFov,
				maxFov: maxFov
			});
		});

		// ============================================================
		// INIT ZOOM SPEED CONTROLS (Static HTML)
		// ============================================================
		initZoomSpeedControls();

		$("#view_center").click(function(evt) {
			actionApi.execute("actionGoto_Selected_Object");
		});

		view_fov_text = document.getElementById("view_fov_text");
		
	}

	$(viewControlApi).on("fovChanged", function(evt, fov) {
		setFovText(fov);
		setFovSlider(fov);
	});

	$(initControls);
});