
// This file contains time-related functions and conversion helpers

var ONE_OVER_JD_MILLISECOND = 24*60*60*1000;
var JD_SECOND = 0.000011574074074074074074;

//ensure that the browser has a trunc function
Math.trunc = Math.trunc || function(x) {
  return x < 0 ? Math.ceil(x) : Math.floor(x);
}

function isRealTimeSpeed() {
	return Math.abs(lastData.time.timerate-JD_SECOND)<0.0000001;
}

//Gregorian calendar crossover date
var JD_GREG_CAL = 2299161;

//We needs some custom support to convert Julian day numbers to a displayable year/month/day combo.
function jdayToDate(jDay){
    /*
	 * This algorithm is taken from
	 * "Numerical Recipes in c, 2nd Ed." (1992), pp. 14-15
	 * with some JavaScript adaptations.
	 * The electronic version of the book is freely available
	 * at http://www.nr.com/ , under "Obsolete Versions - Older
	 * book and code versions.
	 */
    var ja,jalpha,jb,jc,jd,je;
    
    //julian needs to be rounded down first
    //need to add 0.5 for correct truncation because julian date starts at noon and not midnight
    var julian = Math.floor(jDay + 0.5);
    
    if(julian >= JD_GREG_CAL) {
        jalpha = Math.trunc(((julian - 1867216)-0.25) / 36524.25);
        ja = julian + 1 + jalpha - Math.trunc(0.25 * jalpha);
    } else if ( julian < 0) {
        ja = julian + 36525*( 1 - Math.trunc(julian / 36525));
    } else {
        ja = julian;
    }
    jb = ja+1524;
    jc = Math.trunc(6680 + ((jb - 2439870)-122.1) / 365.25);
    jd = Math.trunc(365 * jc + (0.25*jc));
    je = Math.trunc((jb-jd)/30.6001);
    
    var result = {};
    result.day = jb - jd - Math.trunc(30.6001*je);
    result.month = je-1;
    if(result.month > 12) result.month -= 12;
    result.year = jc - 4715;
    if(result.month > 2) --result.year;
    //Stellarium omits the following because it uses a year "zero"
    //if(result.year <= 0) --result.year;
    if(julian < 0) result.year -= 100*(1-Math.trunc(julian / 36525));
    return result;
}

function dateTimeToJd(y,m,d,h,min,s){
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
    var ja, jy=y, jm;
    var IGREG = 15 + 31 * (10 + 12*1582); //Greg Calendar adopted 15 Oct 1582
    
    //Stellarium has a year zero, so the following is ommitted
    //if(jy<0) ++jy;
    if(m > 2) jm = m+1;
    else {
        --jy;
        jm = m + 13;
    }
    jul = Math.trunc(Math.floor(365.25*jy) + Math.floor(30.6001*jm) + d + 1720995);
    if(d + 31 * (m + 12*y ) >= IGREG)    {
        //need Gregorian conversion
        ja = Math.trunc(0.01*jy);
        jul += 2 - ja + Math.trunc(0.25*ja);
    }
    
    return (jul + deltaTime);
}

function jdayToTime(jd)
{
	//This should be less complicated than the date
    //Getting the fractional part of a number should really be part of the JavaScript language / library
	var frac = jd - Math.floor(jd);
	var s = Math.floor(frac * 24 * 60 * 60 + 0.0001);

	var obj = {
		hour : (Math.floor(s / (60*60))+12)%24,
		minute : Math.floor(s/60)%60,
		second : s % 60
	}
	return obj;
}

//converts a javascript date to a JD
function dateToJd(date)
{
    //This is quite easy
	//divide msec by 24*60*60*1000 and add unix time epoch (1.1.1970)
	return date.getTime() / 86400000 + 2440587.5;
}

function getDateString(jday)
{
	var date = jdayToDate(jday);

	var yyyy = date.year.toString();
	var mm = date.month.toString();
	var dd = date.day.toString();

	//no fancy formatting for now
	return yyyy + "-" + (mm[1]?mm:"0"+mm[0]) + "-" + (dd[1]?dd:"0"+dd[0]);
}

function getTimeString(jday)
{
	var time = jdayToTime(jday);

	var hh = time.hour.toString();
	var mm = time.minute.toString();
	var ss = time.second.toString();

	return  (hh[1]?hh:"0"+hh[0]) + ":" + (mm[1]?mm:"0"+mm[0]) + ":" + (ss[1]?ss:"0"+ss[0]);
}

function getCurrentTime()
{
	//progress the time depending on the elapsed time between last update and now, and the set time rate
	if(isRealTimeSpeed()){
		//the timerate is 1:1 with real time
		//console.log("realtime");
		return lastData.time.jday + ($.now() - lastTimeSync) / ONE_OVER_JD_MILLISECOND;
	}
	else {
		//console.log("unrealtime");
		return lastData.time.jday + (($.now() - lastTimeSync) / 1000.0) * lastData.time.timerate;
	}
}

function numberOfDaysInMonthInYear(month,year) {
    switch(month)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			return 31;
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
			break;

		case 2:
			if ( year > 1582 ) {
				if ( year % 4 === 0 ) {
					if ( year % 100 === 0 ) {
						if ( year % 400 === 0 ){
							return 29;
						} else {
							return 28;
						}
					}
					else {
						return 29;
					}
				}
				else {
					return 28;
				}
			}
			else {
				if ( year % 4 === 0 ){
					return 29;
				} else {
					return 28;
				}
			}
			break;
		case 0:
			return numberOfDaysInMonthInYear(12, year-1);
			break;
		case 13:
			return numberOfDaysInMonthInYear(1, year+1);
			break;
		default:
			break;
	}

	return 0;
}

//Like StelUtils::changeDateTimeForRollover
function dateTimeForRollover(datetime) {
    
    while(datetime.time.second > 59) {
        datetime.time.second -= 60;
        datetime.time.minute += 1;
    }
    while(datetime.time.second < 0) {
        datetime.time.second += 60;
        datetime.time.minute -= 1;
    }
    while(datetime.time.minute > 59) {
        datetime.time.minute -= 60;
        datetime.time.hour += 1;
    }
    while(datetime.time.minute < 0) {
        datetime.time.minute += 60;
        datetime.time.hour -= 1;
    }
    while(datetime.time.hour > 23 ) {
        datetime.time.hour -= 24;
        datetime.date.day += 1;
    }
    while(datetime.time.hour < 0 ) {
        datetime.time.hour += 24;
        datetime.date.day -= 1;
    }
    while(datetime.date.day > numberOfDaysInMonthInYear(datetime.date.month, datetime.date.year)) {
        datetime.date.day -= numberOfDaysInMonthInYear(datetime.date.month, datetime.date.year);
        datetime.date.month += 1;
        if( datetime.date.month > 12 ) {
            datetime.date.month -= 12;
            datetime.date.year += 1;
        }
    }
    while(datetime.date.day < 1) {
        datetime.date.day += numberOfDaysInMonthInYear(datetime.date.month - 1, datetime.date.year);
        datetime.date.month -= 1;
        if( datetime.date.month < 1 ) {
            datetime.date.month += 12;
            datetime.date.year -= 1;
        }
    }
    while(datetime.date.month > 12) {
        datetime.date.month -= 12;
        datetime.date.year += 1;
    }
    while(datetime.date.month < 1) {
        datetime.date.month += 12;
        datetime.date.year -= 1;
    }
    
    if( datetime.date.year === 1582 && datetime.date.month === 10 && (datetime.date.day > 4 && datetime.date.day < 15)) {
        datetime.date.day = 15;
    }
}

function resyncTime()
{
	lastData.time.jday = getCurrentTime();
	lastTimeSync = $.now();
}

function resetCurrentDisplayTime() {
    currentDisplayTime.date = { year: undefined, month: undefined, day: undefined };
    currentDisplayTime.time = { hour: undefined, minute: undefined, second: undefined };
    currentDisplayTime.jd = 0;
}

var editTimeout;

function onTimeUpdate(){
    timeEditMode = false;
    editTimeout = undefined;
    update();
}

function setTimeFromCurrentDisplayTime(){
    //we have to change an eventual rollover
    dateTimeForRollover(currentDisplayTime);
    
    //calculate a new JD, subtracting timezone shift and adding deltaT
    var newJD = dateTimeToJd(currentDisplayTime.date.year,currentDisplayTime.date.month,currentDisplayTime.date.day,
                                currentDisplayTime.time.hour,currentDisplayTime.time.minute,currentDisplayTime.time.second);
                                
    var inverseDate = jdayToDate(newJD);
    var inverseTime = jdayToTime(newJD);
    
    //we dont have access to the exact timezone shift and deltaT for the new JD (would require a server poll), so this may introduce errors, most notably when entering/exiting summer time
    newJD -= (lastData.time.gmtShift - lastData.time.deltaT);
    
    //set the new jDay locally
    resyncTime();
    lastData.time.jday = newJD;
    
    forceSpinnerUpdate();
    
    timeEditMode = true;
    editTimeout && clearTimeout(editTimeout);
    editTimeout = setTimeout(function() {
        //post an update
        postCmd("/api/main/time", { time: newJD  }, onTimeUpdate );
    },500);
}

function setJDay(val) {
    if(isNaN(val)) {
        console.log("Prevented NaN value");
        return;
    }
    if(currentDisplayTime.jday !== val) {
        console.log('setJDay: ' + val);
        
        currentDisplayTime.jday = val;
        
        //remove deltaT
        
        resyncTime();
        val = val + lastData.time.deltaT;
        lastData.time.jday = val;
        
        editTimeout && clearTimeout(editTimeout);
        editTimeout = setTimeout(function() {
            //post an update
            postCmd("/api/main/time", { time: val  },onTimeUpdate );
        },500);
    }
}

function setDateTimeField(type, field, val) {
    if(currentDisplayTime[type][field] !== val) {
        currentDisplayTime[type][field] = val;
        setTimeFromCurrentDisplayTime();
    }
}

//these functions are modeled after StelCore
function increaseTimeRate(){
	var s = lastData.time.timerate;
	if(s>=JD_SECOND) s*=10;
	else if (s<-JD_SECOND) s/=10;
	else if (s>=0 && s<JD_SECOND) s=JD_SECOND;
	else if (s>=-JD_SECOND && s<0.) s=0;
	setTimeRate(s);
}

function decreaseTimeRate(){
	var s = lastData.time.timerate;
	if (s>JD_SECOND) s/=10.;
	else if (s<=-JD_SECOND) s*=10.;
	else if (s>-JD_SECOND && s<=0.) s=-JD_SECOND;
	else if (s>0. && s<=JD_SECOND) s=0.;
	setTimeRate(s);
}

function setDateNow(){
	//for now,, this is only calculated here in JS
	//this may not be the same result as pressing the NOW button in the GUI
    //because of varying deltaT which may be quite different from the current time + latency issues
	var jd = dateToJd(new Date());

	//we have to apply reverse deltaT
	resyncTime();
   	lastData.time.jday = jd + lastData.time.deltaT;

	postCmd("/api/main/time",{time:lastData.time.jday});
}

function setTimeRate(timerate)
{
	//we have to re-sync the time for correctness
	resyncTime();

	//the last data rate gets set to the new rate NOW to provide a responsive GUI
	lastData.time.timerate = timerate;

	//update time button now
	updateTimeButtonState();

	//we also update the time we have to reduce the effect of network latency
	postCmd("/api/main/time",{timerate: timerate, time: getCurrentTime()});
}

