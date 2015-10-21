
// This file contains time-related functions and conversion helpers

//ensure that the browser has a trunc function
Math.trunc = Math.trunc || function(x) {
    return x < 0 ? Math.ceil(x) : Math.floor(x);
};

var Time = ( function($) {
    "use strict";

    //Constants
    var ONE_OVER_JD_MILLISECOND = 24 * 60 * 60 * 1000;
    var JD_SECOND = 0.000011574074074074074074;
    //Gregorian calendar crossover date
    var JD_GREG_CAL = 2299161;

    //Private stuff
    var timeData; //the current time data
    var lastTimeSync; //The real time which timeData.jday corresponds to
    var timeEditMode; //If the user currently focuses a time-edit control
    var updateQueue; //The UpdateQueue object for time updates
    var currentDisplayTime = {}; //the time shown in the controls, not automatically synced with timeData

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


        $("#bt_timerewind").click(Time.decreaseTimeRate);
        $("#bt_timeforward").click(Time.increaseTimeRate);
        $("#bt_timeplaypause").click(Time.togglePlayPause);
        $("#bt_timenow").click(Time.setDateNow);

        //jQuery UI initialization
        $("#timewidget").tabs();

        $("#date_year").spinner({
            min: -100000,
            max: 100000
        }).data({
            type: "date",
            field: "year"
        });
        $("#date_month").spinner({
            min: 0,
            max: 13
        }).data({
            type: "date",
            field: "month"
        });
        $("#date_day").spinner({
            min: 0,
            max: 32
        }).data({
            type: "date",
            field: "day"
        });
        $("#time_hour").spinner({
            min: -1,
            max: 24
        }).data({
            type: "time",
            field: "hour"
        });
        $("#time_minute").spinner({
            min: -1,
            max: 60
        }).data({
            type: "time",
            field: "minute"
        });
        $("#time_second").spinner({
            min: -1,
            max: 60
        }).data({
            type: "time",
            field: "second"
        });
        $("#input_jd").spinner({
            min: -100000000,
            max: 100000000,
            numberFormat: "d5",
            step: 0.00001
        });
        $("#input_mjd").spinner({
            min: -100000000,
            max: 100000000,
            numberFormat: "d5",
            step: 0.00001
        });

        //use delegated event style here to reduce number of unique events
        var scope = ".timedisplay input";
        $("#timewidget").on("focusin", scope, function(evt) {
            Time.enterTimeEditMode();
        }).on("focusout", scope, function(evt) {
            Time.leaveTimeEditMode();
        });

        $("#time_local").on('input', scope, function() {
            if ($(this).data('onInputPrevented')) return;
            var val = this.value,
                $this = $(this),
                max = $this.spinner('option', 'max'),
                min = $this.spinner('option', 'min');
            console.log("val: " + val);
            if (!val.match(/^\d+$/)) val = $this.data('prevData'); //we want only number, no alpha
            val = parseFloat(val);
            this.value = val > max ? max : val < min ? min : val;
            //for some obscure reason, this.value may be a string instead of a float, so use val directly!
            if (val !== $this.data('prevData')) Time.setDateTimeField($(this).data("type"), $(this).data("field"), val);
        }).on('keydown', scope, function(e) { // to allow 'Backspace' key behaviour
            $(this).data('onInputPrevented', e.which === 8 ? true : false);
            $(this).data('prevData', this.value);
        }).on("spin", scope, function(evt, ui) {
            Time.setDateTimeField($(this).data("type"), $(this).data("field"), ui.value);
            return false;
        });

        $("#input_jd").on('input', function() {
            if ($(this).data('onInputPrevented')) return;
            var val = this.value,
                $this = $(this),
                max = $this.spinner('option', 'max'),
                min = $this.spinner('option', 'min');
            console.log("val: " + val);
            if (!val.match(/^\d+\.?\d*$/)) val = $this.data('prevData'); //we want only number, no alpha
            var val2 = parseFloat(val);
            this.value = val2 > max ? max : val2 < min ? min : val2;
            //for some obscure reason, this.value may be a string instead of a float, so use val directly!
            if (val !== $this.data('prevData')) Time.setJDay(val2);
        }).on('keydown', function(e) { // to allow 'Backspace' key behaviour
            $(this).data('onInputPrevented', e.which === 8 ? true : false);
            $(this).data('prevData', this.value);
        }).on("spin", function(evt, ui) {
            Time.setJDay(ui.value);
        });

        $("#input_mjd").on('input', function() {
            if ($(this).data('onInputPrevented')) return;
            var val = this.value,
                $this = $(this),
                max = $this.spinner('option', 'max'),
                min = $this.spinner('option', 'min');
            console.log("val: " + val);
            if (!val.match(/^\d+\.?\d*$/)) val = $this.data('prevData'); //we want only number, no alpha
            var val2 = parseFloat(val);
            this.value = val2 > max ? max : val2 < min ? min : val2;
            //for some obscure reason, this.value may be a string instead of a float, so use val directly!
            if (val !== $this.data('prevData')) Time.setJDay(val2 + 2400000.5);
        }).on('keydown', function(e) { // to allow 'Backspace' key behaviour
            $(this).data('onInputPrevented', e.which === 8 ? true : false);
            $(this).data('prevData', this.value);
        }).on("spin", function(evt, ui) {
            Time.setJDay(ui.value + 2400000.5);
        });
    }

    function animate() {
        if (timeData && !timeEditMode) {
            var currentJday = getCurrentTime();

            //what we have to animate depends on which tab is shown
            if ($timewidget.tabs('option', 'active') === 0) {
                //apply timezone
                var localTime = currentJday + timeData.gmtShift;

                //we only update the spinners when there is a discrepancy, to improve performance
                var date = jdayToDate(localTime);
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

                var time = jdayToTime(localTime);
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

    //Finds out if the time flows in 1:1 relation to real time
    function isRealTimeSpeed() {
        return Math.abs(timeData.timerate - JD_SECOND) < 0.0000001;
    }

    //Converts a JDay to Day/Month/Year format (not using JavaScript Date object)
    function jdayToDate(jDay) {
        /*
         * This algorithm is taken from
         * "Numerical Recipes in c, 2nd Ed." (1992), pp. 14-15
         * with some JavaScript adaptations.
         * The electronic version of the book is freely available
         * at http://www.nr.com/ , under "Obsolete Versions - Older
         * book and code versions.
         */
        var ja, jalpha, jb, jc, jd, je;

        //julian needs to be rounded down first
        //need to add 0.5 for correct truncation because julian date starts at noon and not midnight
        var julian = Math.floor(jDay + 0.5);

        if (julian >= JD_GREG_CAL) {
            jalpha = Math.trunc(((julian - 1867216) - 0.25) / 36524.25);
            ja = julian + 1 + jalpha - Math.trunc(0.25 * jalpha);
        } else if (julian < 0) {
            ja = julian + 36525 * (1 - Math.trunc(julian / 36525));
        } else {
            ja = julian;
        }
        jb = ja + 1524;
        jc = Math.trunc(6680 + ((jb - 2439870) - 122.1) / 365.25);
        jd = Math.trunc(365 * jc + (0.25 * jc));
        je = Math.trunc((jb - jd) / 30.6001);

        var result = {};
        result.day = jb - jd - Math.trunc(30.6001 * je);
        result.month = je - 1;
        if (result.month > 12) result.month -= 12;
        result.year = jc - 4715;
        if (result.month > 2) --result.year;
        //Stellarium omits the following because it uses a year "zero"
        //if(result.year <= 0) --result.year;
        if (julian < 0) result.year -= 100 * (1 - Math.trunc(julian / 36525));
        return result;
    }

    //Converts the fractional part of a JDay to hour/minute/second of a day
    function jdayToTime(jd) {
        //This should be less complicated than the date
        //Getting the fractional part of a number should really be part of the JavaScript language / library
        var frac = jd - Math.floor(jd);
        var s = Math.floor(frac * 24 * 60 * 60 + 0.0001);

        var obj = {
            hour: (Math.floor(s / (60 * 60)) + 12) % 24,
            minute: Math.floor(s / 60) % 60,
            second: s % 60
        };
        return obj;
    }

    //Converts a date in the common Y/M/D-H/MIN/S format to a JDay
    function dateTimeToJd(y, m, d, h, min, s) {
        //compute the time part, and add the noon offset of 0.5
        var deltaTime = (h / 24) + (min / (24 * 60)) + (s / (24 * 60 * 60)) - 0.5;

        /*
         * This algorithm is taken from
         * "Numerical Recipes in c, 2nd Ed." (1992), pp. 11-12
         * with some JavaScript adaptations.
         * The electronic version of the book is freely available
         * at http://www.nr.com/ , under "Obsolete Versions - Older
         * book and code versions.
         */
        var jul;
        var ja, jy = y,
            jm;
        var IGREG = 15 + 31 * (10 + 12 * 1582); //Greg Calendar adopted 15 Oct 1582

        //Stellarium has a year zero, so the following is ommitted
        //if(jy<0) ++jy;
        if (m > 2) jm = m + 1;
        else {
            --jy;
            jm = m + 13;
        }
        jul = Math.trunc(Math.floor(365.25 * jy) + Math.floor(30.6001 * jm) + d + 1720995);
        if (d + 31 * (m + 12 * y) >= IGREG) {
            //need Gregorian conversion
            ja = Math.trunc(0.01 * jy);
            jul += 2 - ja + Math.trunc(0.25 * ja);
        }

        return (jul + deltaTime);
    }

    //converts a javascript date object to a JD
    function jsDateToJd(date) {
        //This is quite easy
        //divide msec by 24*60*60*1000 and add unix time epoch (1.1.1970)
        return date.getTime() / 86400000 + 2440587.5;
    }

    //Retrieves the currently valid time, depending on the last known JD change, the timerate and the current real-world time
    function getCurrentTime() {
        //progress the time depending on the elapsed time between last update and now, and the set time rate
        if (isRealTimeSpeed()) {
            //the timerate is 1:1 with real time
            //console.log("realtime");
            return timeData.jday + ($.now() - lastTimeSync) / ONE_OVER_JD_MILLISECOND;
        } else {
            //console.log("unrealtime");
            return timeData.jday + (($.now() - lastTimeSync) / 1000.0) * timeData.timerate;
        }
    }

    function resyncTime() {
        timeData.jday = getCurrentTime();
        lastTimeSync = $.now();
    }

    function setTimeRate(timerate) {
        //we have to re-sync the time for correctness
        resyncTime();

        //the last data rate gets set to the new rate NOW to provide a responsive GUI
        timeData.timerate = timerate;

        //update time button now
        updateTimeButtonState();

        //we also update the time we have to reduce the effect of network latency
        Main.postCmd("/api/main/time", {
            timerate: timerate,
            time: getCurrentTime()
        });
    }

    // --- editing-related stuff ----
    function numberOfDaysInMonthInYear(month, year) {
        switch (month) {
            case 1:
            case 3:
            case 5:
            case 7:
            case 8:
            case 10:
            case 12:
                return 31;
            case 4:
            case 6:
            case 9:
            case 11:
                return 30;
            case 2:
                if (year > 1582) {
                    if (year % 4 === 0) {
                        if (year % 100 === 0) {
                            if (year % 400 === 0) {
                                return 29;
                            } else {
                                return 28;
                            }
                        } else {
                            return 29;
                        }
                    } else {
                        return 28;
                    }
                } else {
                    if (year % 4 === 0) {
                        return 29;
                    } else {
                        return 28;
                    }
                }
                break;
            case 0:
                return numberOfDaysInMonthInYear(12, year - 1);
            case 13:
                return numberOfDaysInMonthInYear(1, year + 1);
            default:
                break;
        }

        return 0;
    }

    //Like StelUtils::changeDateTimeForRollover
    function dateTimeForRollover(datetime) {

        while (datetime.time.second > 59) {
            datetime.time.second -= 60;
            datetime.time.minute += 1;
        }
        while (datetime.time.second < 0) {
            datetime.time.second += 60;
            datetime.time.minute -= 1;
        }
        while (datetime.time.minute > 59) {
            datetime.time.minute -= 60;
            datetime.time.hour += 1;
        }
        while (datetime.time.minute < 0) {
            datetime.time.minute += 60;
            datetime.time.hour -= 1;
        }
        while (datetime.time.hour > 23) {
            datetime.time.hour -= 24;
            datetime.date.day += 1;
        }
        while (datetime.time.hour < 0) {
            datetime.time.hour += 24;
            datetime.date.day -= 1;
        }
        while (datetime.date.day > numberOfDaysInMonthInYear(datetime.date.month, datetime.date.year)) {
            datetime.date.day -= numberOfDaysInMonthInYear(datetime.date.month, datetime.date.year);
            datetime.date.month += 1;
            if (datetime.date.month > 12) {
                datetime.date.month -= 12;
                datetime.date.year += 1;
            }
        }
        while (datetime.date.day < 1) {
            datetime.date.day += numberOfDaysInMonthInYear(datetime.date.month - 1, datetime.date.year);
            datetime.date.month -= 1;
            if (datetime.date.month < 1) {
                datetime.date.month += 12;
                datetime.date.year -= 1;
            }
        }
        while (datetime.date.month > 12) {
            datetime.date.month -= 12;
            datetime.date.year += 1;
        }
        while (datetime.date.month < 1) {
            datetime.date.month += 12;
            datetime.date.year -= 1;
        }

        if (datetime.date.year === 1582 && datetime.date.month === 10 && (datetime.date.day > 4 && datetime.date.day < 15)) {
            datetime.date.day = 15;
        }
    }

    function updateTimeButtonState() {
        //updates the state of the time buttons
        //replicates functionality from StelGui.cpp update() function
        $bt_timerewind.toggleClass("active", timeData.timerate < -0.99 * JD_SECOND);
        $bt_timeforward.toggleClass("active", timeData.timerate > 1.01 * JD_SECOND);
        $bt_timenow.toggleClass("active", timeData.isTimeNow);

        if (timeData.timerate === 0) {
            $bt_timeplaypause.removeClass("active").addClass("paused");
        } else if (isRealTimeSpeed()) {
            $bt_timeplaypause.removeClass("paused").addClass("active");
        } else {
            $bt_timeplaypause.removeClass("paused").removeClass("active");
        }
    }

    function resetCurrentDisplayTime() {
        currentDisplayTime.date = {
            year: undefined,
            month: undefined,
            day: undefined
        };
        currentDisplayTime.time = {
            hour: undefined,
            minute: undefined,
            second: undefined
        };
        currentDisplayTime.jd = 0;
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

    //Uses the currently set display time to queue an update of the server
    function setTimeFromCurrentDisplayTime() {
        //we have to change an eventual rollover
        dateTimeForRollover(currentDisplayTime);

        //calculate a new JD in local time
        var newJD = dateTimeToJd(currentDisplayTime.date.year, currentDisplayTime.date.month, currentDisplayTime.date.day,
            currentDisplayTime.time.hour, currentDisplayTime.time.minute, currentDisplayTime.time.second);

        //we dont have access to the exact timezone shift for the new JD (would require a server poll), so this may introduce errors, most notably when entering/exiting summer time
        newJD -= (timeData.gmtShift);

        //set the new jDay locally
        resyncTime();
        timeData.jday = newJD;

        forceSpinnerUpdate();
        updateQueue.enqueue({
            time: newJD
        });
    }

    //Public stuff
    return {
        init: function() {
            initControls();
            updateQueue = new Main.UpdateQueue("/api/main/time");

            resetCurrentDisplayTime();
        },

        updateFromServer: function(time) {
            timeData = time;
            lastTimeSync = $.now();

            //update deltaT display
            var deltaTinSec = time.deltaT * (60 * 60 * 24);
            $deltat[0].textContent=deltaTinSec.toFixed(2) + "s";

            updateTimeButtonState();

            if (!(timeEditMode || updateQueue.isQueued)) {
                //if user is focusing the edit controls, or we have a pending time push, don't do this
                //force an update
                animate();
            }
        },

        updateAnimation: animate,

        increaseTimeRate: function() {
            var s = timeData.timerate;
            if (s >= JD_SECOND) s *= 10;
            else if (s < -JD_SECOND) s /= 10;
            else if (s >= 0 && s < JD_SECOND) s = JD_SECOND;
            else if (s >= -JD_SECOND && s < 0) s = 0;
            setTimeRate(s);
        },

        decreaseTimeRate: function() {
            var s = timeData.timerate;
            if (s > JD_SECOND) s /= 10;
            else if (s <= -JD_SECOND) s *= 10;
            else if (s > -JD_SECOND && s <= 0) s = -JD_SECOND;
            else if (s > 0 && s <= JD_SECOND) s = 0;
            setTimeRate(s);
        },

        togglePlayPause: function() {
            if (isRealTimeSpeed()) {
                setTimeRate(0);
            } else {
                setTimeRate(JD_SECOND);
            }
        },

        setDateNow: function() {
            //for now, this is only calculated here in JS
            //this may not be the same result as pressing the NOW button in the GUI
            var jd = jsDateToJd(new Date());

            resyncTime();
            timeData.jday = jd;

            Main.postCmd("/api/main/time", {
                time: timeData.jday
            });
        },

        setJDay: function(val) {
            if (isNaN(val)) {
                console.log("Prevented NaN value");
                return;
            }
            if (currentDisplayTime.jday !== val) {
                console.log('setJDay: ' + val);

                currentDisplayTime.jday = val;

                resyncTime();
                timeData.jday = val;

                updateQueue.enqueue({
                    time: val
                });
            }
        },


        setDateTimeField: function(type, field, val) {
            if (currentDisplayTime[type][field] !== val) {
                currentDisplayTime[type][field] = val;
                setTimeFromCurrentDisplayTime();
            }
        },

        enterTimeEditMode: function() {
            console.log("Entering time edit mode...");
            timeEditMode = true;
        },

        leaveTimeEditMode: function() {
            console.log("Leaving time edit mode...");
            timeEditMode = false;
        }
    };
})(jQuery);