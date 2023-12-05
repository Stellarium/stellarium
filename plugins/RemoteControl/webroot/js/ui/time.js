define(["jquery", "api/time", "api/actions", "jquery-ui"], function($, timeApi, actionApi) {
	"use strict";

	//controls
	var $deltat;
	var $bt_timerewind;
	var $bt_timeforward;
	var $bt_timeplaypause;
	var $bt_timenow;
	var $timewidget;
	var $date_year;
	var $date_month;
	var $date_day;
	var $time_hour;
	var $time_minute;
	var $time_second;
	var $input_jd;
	var $input_mjd;

	//the time shown in the controls, not automatically synced with timeData
	var currentDisplayTime = {
		date: {
			year: undefined,
			month: undefined,
			day: undefined
		},
		time: {
			hour: undefined,
			minute: undefined,
			second: undefined
		},
		jd: 0
	};
	var timeEditMode; //If the user currently focuses a time-edit control


	function initControls() {
		$deltat = $("#deltat");
		$bt_timerewind = $("#bt_timerewind");
		$bt_timeforward = $("#bt_timeforward");
		$bt_timenow = $("#bt_timenow");
		$bt_timeplaypause = $("#bt_timeplaypause");

		$timewidget = $("#timewidget");
		$date_year = $("#date_year");
		$date_month = $("#date_month");
		$date_day = $("#date_day");
		$time_hour = $("#time_hour");
		$time_minute = $("#time_minute");
		$time_second = $("#time_second");

		$input_jd = $("#input_jd");
		$input_mjd = $("#input_mjd");


		$("#bt_timerewind").click(timeApi.decreaseTimeRate);
		$("#bt_timeforward").click(timeApi.increaseTimeRate);
		$("#bt_timeplaypause").click(timeApi.togglePlayPause);
		$("#bt_timenow").click(timeApi.setDateNow);

		//jQuery UI initialization
		$("#timewidget").tabs();

		//use delegated event style here to reduce number of unique events
		var scope = ".timedisplay input";
		$("#timewidget").on("focus spinstart", scope, function(evt) {
			enterTimeEditMode();
		}).on("blur spinstop", scope, function(evt) {
			leaveTimeEditMode();
		});

		$("#time_local").on("spinuserinput", scope, function(evt,ui){
			setDateTimeField($(this).data("type"), $(this).data("field"), ui.value);
		});

		$("#input_jd").on("spinuserinput", function(evt, ui) {
			setJDay(ui.value);
		});

		$("#input_mjd").on("spinuserinput", function(evt, ui) {
			setJDay(ui.value + 2400000.5);
		});

		initTimeJumpButtons();
	}

	function initTimeJumpButtons() {

		var $items = $("#timejumplist li");
		$items.each(function(){
			var $this = $(this);
			$this.prepend($("<button>",{name: $this.data("next"), class:"stelaction button32 icon32 btTimeForward"}));
			$this.prepend($("<button>",{name: $this.data("prev"), class:"stelaction button32 icon32 btTimeRewind"}));
		});
	}

	function setDateTimeField(type, field, val) {
		if (currentDisplayTime[type][field] !== val) {
			currentDisplayTime[type][field] = val;
			timeApi.setTimeFromTimeObj(currentDisplayTime);
		}
	}

	function setJDay(val) {
		if (isNaN(val)) {
			console.log("Prevented NaN value");
			return;
		}
		if (currentDisplayTime.jd !== val) {
			console.log('setJDay: ' + val);

			currentDisplayTime.jd = val;
			timeApi.setJDay(val);
		}
	}

	function updateTimeButtonState() {
		//updates the state of the time buttons
		//replicates functionality from StelGui.cpp update() function
		$bt_timerewind.toggleClass("active", timeApi.isRewind());
		$bt_timeforward.toggleClass("active", timeApi.isFastForward());
		$bt_timenow.toggleClass("active", timeApi.isTimeNow());

		if (timeApi.isTimeStopped()) {
			$bt_timeplaypause.removeClass("active").addClass("paused");
		} else if (timeApi.isRealTimeSpeed()) {
			$bt_timeplaypause.removeClass("paused").addClass("active");
		} else {
			$bt_timeplaypause.removeClass("paused").removeClass("active");
		}
	}

	function forceSpinnerUpdate() {
		$date_year.spinner("value", currentDisplayTime.date.year);
		$date_month.spinner("value", currentDisplayTime.date.month);
		$date_day.spinner("value", currentDisplayTime.date.day);

		$time_hour.spinner("value", currentDisplayTime.time.hour);
		$time_minute.spinner("value", currentDisplayTime.time.minute);
		$time_second.spinner("value", currentDisplayTime.time.second);

		$input_jd.spinner("value", currentDisplayTime.jd);
		$input_mjd.spinner("value", currentDisplayTime.jd - 2400000.5);
	}

	function enterTimeEditMode() {
		console.log("Entering time edit mode...");
		timeEditMode = true;
	}

	function leaveTimeEditMode() {
		console.log("Leaving time edit mode...");
		timeEditMode = false;
	}

	$(timeApi).on("timeChangeQueued", forceSpinnerUpdate);
	$(timeApi).on("timeDataUpdated", function(evt, data) {
		//update deltaT display
		var deltaTinSec = timeApi.getTimeData().deltaT * (60 * 60 * 24);
		$deltat[0].textContent = deltaTinSec.toFixed(2) + "s";

		updateTimeButtonState();
	});

	$(function() {
		initControls();
	});

	function updateTimeDisplay() {
		if (timeApi.getTimeData() && !timeEditMode) {
			var timeData = timeApi.getTimeData();
			var currentJday = timeApi.getCurrentTime();

			//what we have to animate depends on which tab is shown
			if ($timewidget.tabs('option', 'active') === 0) {
				//apply timezone
				var localTime = currentJday + timeData.gmtShift;

				//we only update the spinners when there is a discrepancy, to improve performance
				var date = timeApi.jdayToDate(localTime);
				if (currentDisplayTime.date.year !== date.year) {
					$date_year.spinner("value", date.year);
				}
				if (currentDisplayTime.date.month !== date.month) {
					$date_month.spinner("value", date.month);
				}
				if (currentDisplayTime.date.day !== date.day) {
					$date_day.spinner("value", date.day);
				}
				currentDisplayTime.date = date;

				var time = timeApi.jdayToTime(localTime);
				if (currentDisplayTime.time.hour !== time.hour) {
					$time_hour.spinner("value", time.hour);
				}
				if (currentDisplayTime.time.minute !== time.minute) {
					$time_minute.spinner("value", time.minute);
				}
				if (currentDisplayTime.time.second !== time.second) {
					$time_second.spinner("value", time.second);
				}
				currentDisplayTime.time = time;
			} else {
				var jd = currentJday.toFixed(5);

				if (currentDisplayTime.jd.toFixed(5) !== jd) {
					if (!$input_jd.is(":focus"))
						$input_jd.spinner("value", currentJday);
					//MJD directly depends on JD
					var mjd = (currentJday - 2400000.5);
					if (!$input_mjd.is(":focus"))
						$input_mjd.spinner("value", mjd);
				}
				currentDisplayTime.jd = currentJday;
			}
		}
	}

	return {
		updateTimeDisplay: updateTimeDisplay
	};

});