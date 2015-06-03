 "use strict";

//update every second
var updateInterval = 1000;
var updatePoll = true;
var connectionLost = false;

var animationSupported;

//the last received update data
var lastData;
//the time when the last data was received
var lastDataTime;
//the time when the time was last synced
var lastTimeSync;

//main update function, which is executed each second
function update(requeue){
	$.ajax({
		url: "/api/main/status",
		dataType: "json",
		success: function(data){
			lastData = data;
			lastDataTime = lastTimeSync = $.now();

			//update time NOW
			if(!animationSupported)
				updateTime();

			updateTimeButtonState();

			var textelem = $("#activescript");
			if(data.script.scriptIsRunning)
			{
				textelem.text(data.script.runningScriptId);
			}
			else
			{
				textelem.text("-none-");
			}
			$("#bt_stopscript").prop({	disabled: !data.script.scriptIsRunning	});
			$("#noresponse").hide();
			connectionLost = false;
		},
		error: function(xhr, status,errorThrown){
			$("#noresponse").show();
			connectionLost = true;
			console.log("Error fetching updates");
			console.log("Error: " + errorThrown );
	        console.log("Status: " + status );
		},
		complete: function(){
			if(requeue && updatePoll)
			{
				setTimeout(function(){
					update(true);
				},updateInterval);
			}
		}
	});
}

function postCmd(url,data){
	$.ajax({
		url: url,
		method: "POST",
		data: data,
		dataType: "text",
		timeout: 3000,
		success: function(data){
			console.log("server replied: " + data);
			if(data !== "ok")
			{
				alert(data);
			}
			update();
		},
		error: function(xhr,status,errorThrown){
			console.log("Error posting command " + url);
			console.log( "Error: " + errorThrown );
			console.log( "Status: " + status );
			alert("Could not call server")
		}
	});
}

function postAction(actionName) {
    $.ajax({
        url: "api/stelaction/do",
        method: "POST",
        async: false,
        dataType: "text",
        data: {
            id: actionName
        },
        success: function(resp){
            if(resp === "ok") {
                //non-checkable action, cant change text
            }
            else if(resp === "true") {
                $("#actionlist").trigger("action:"+actionName,true);
            }
            else if (resp === "false") {
                $("#actionlist").trigger("action:"+actionName,false);
            }
            else {
                alert(resp);
            }
        },
        error: function(xhr,status,errorThrown){
			console.log("Error posting action " + actionName);
			console.log( "Error: " + errorThrown );
			console.log( "Status: " + status );
			alert("Could not call server")
		}
    });
}

var ONE_OVER_JD_MILLISECOND = 24*60*60*1000;
var JD_SECOND = 0.000011574074074074074074;

function isRealTimeSpeed() {
	return Math.abs(lastData.time.timerate-JD_SECOND)<0.0000001;
}

function updateTimeButtonState() {
	//updates the state of the time buttons
	//replicates functionality from StelGui.cpp update() function
    $("#bt_timerewind").toggleClass("active",lastData.time.timerate<-0.99*JD_SECOND);
    $("#bt_timeforward").toggleClass("active", lastData.time.timerate>1.01*JD_SECOND);
    $("#bt_timenow").toggleClass("active",lastData.time.isTimeNow);
    
	if(lastData.time.timerate===0) {
		$("#bt_timeplaypause").removeClass("active").addClass("paused");
	}
	else if(isRealTimeSpeed())	{
		$("#bt_timeplaypause").removeClass("paused").addClass("active");
	}
	else	{
		$("#bt_timeplaypause").removeClass("paused").removeClass("active");
	}
}


//this replicates time functions from Stellarium core (getPrintableDate)
//because the date range of Stellarium is quite a bit larger than JavaScript Date() can handle, it needs some custom support
function jdayToDate(jd) {
	//This conversion is from http://blog.bahrenburgs.com/2011/01/javascript-julian-day-conversions.html
	//In particular, it misses the pre-Gregorian calendar correction, and has probably some less accuracy than Stellariums algorithm
	//The algorithm used by Stellarium itself can't be used without some modifications because it relies heavily on C datatypes

	var X = jd + 0.5;
	var Z = Math.floor(X); //Get day without time
    var F = X - Z; //Get time
    var Y = Math.floor((Z-1867216.25)/36524.25);
    var A = Z+1+Y-Math.floor(Y/4);
    var B = A+1524;
    var C = Math.floor((B-122.1)/365.25);
    var D = Math.floor(365.25*C);
    var G = Math.floor((B-D)/30.6001);

    //must get number less than or equal to 12)
	var month = (G<13.5) ? (G-1) : (G-13);
    //if Month is January or February, or the rest of year
    var year = (month<2.5) ? (C-4715) : (C-4716);
    var UT = B-D-Math.floor(30.6001*G)+F;
    var day = Math.floor(UT);

    var obj = {
    	month : month,
    	year : year,
    	day : day
    }

    return obj;
}

function jdayToTime(jd)
{
	//this should be less complicated than the date
	var frac = jd % 1;
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

function updateTime()
{
	if(lastData)
	{
		var currentJday = getCurrentTime();
		
		var correctedTime = currentJday - lastData.time.deltaT;
		//apply timezone
		var localTime = correctedTime + lastData.time.gmtShift;

		//this seems to work MUCH smoother in animation than using jquery for that
		document.getElementById("date").innerHTML = getDateString(localTime);
		document.getElementById("time").innerHTML = getTimeString(localTime);
	}
	if(connectionLost)
	{
		var obj = $("#noresponsetime");
		var elapsed = ($.now() - lastDataTime) / 1000;
		var text = Math.floor(elapsed).toString();
		obj.text(text);
	}
}

function resyncTime()
{
	lastData.time.jday = getCurrentTime();
	lastTimeSync = $.now();
}

function setDateNow(){
	//for now,, this is only calculated here in JS
	//depending on latency, etc, this may not be the same result as pressing the NOW button in the GUI
	var jd = dateToJd(new Date());

	//we have to apply deltaT
	lastData.time.jday = jd + lastData.time.deltaT;
	resyncTime();

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

function fillScriptList(){
	$.ajax({
		url: "/api/scripts/list",
		success: function(data){
			var list = $("#scriptlist");
			list.empty();

			//sort it and insert
			$.each(data.sort(), function(idx,elem) {
				$("<option/>").text(elem).appendTo(list);
			});
		},
		error: function(xhr,status,errorThrown){
			console.log("Error updating script list");
			console.log( "Error: " + errorThrown );
	        console.log( "Status: " + status );
			alert("Could not retrieve script list")
		}
	});
}

function fillActionList(){
	$.ajax({
		url: "/api/stelaction/list",
		success: function(data){
			var list = $("#actionlist");
			list.empty();

			$.each(data, function(key,val) {
                //the key is an optgroup
                var group = $("<optgroup/>").attr("label", key);
                //the val is an array of StelAction objects
                $.each(val, function(idx,v2) {
                    var option = $("<option/>");
                    option.data(v2); //store the data
                    if(v2.isCheckable) {
                        option.addClass("checkableaction");
                        option.text(v2.text + " (" + v2.isChecked + ")");
                        
                        list.on("action:"+v2.id,{elem: option},function(event,newValue){
                            var el = event.data.elem;
                            el.data("isChecked",newValue);
                            el.text(el.data("text") + " (" + newValue + ")");
                        });
                    }
                    else{
                        option.text(v2.text);
                    }
                    option.appendTo(group);
                });
                group.appendTo(list);
			});
		},
		error: function(xhr,status,errorThrown){
			console.log("Error updating script list");
			console.log( "Error: " + errorThrown );
	        console.log( "Status: " + status );
			alert("Could not retrieve script list")
		}
	});
}

function animate() {
	updateTime();

	if(animationSupported)
	{
		window.requestAnimationFrame(animate);
	}
}

$(document).ready(function () {

    $("#scriptlist").change(function(){
    	var selection = $("#scriptlist").children("option").filter(":selected").text();

    	$("#bt_runscript").prop({
					disabled: false
				});
    	console.log("selected: " + selection);
    });
    
    $("#actionlist").change(function(){
        $("#bt_doaction").prop("disabled",false);
    });

    animationSupported = (window.requestAnimationFrame !== undefined);

	//initial update
	fillScriptList();
    fillActionList();
	update(true);
	//update the time even when no server updates arrive


	if(!animationSupported)
	{
		console.log("animation frame not supported")
		setInterval(animate,500);
	}
	else
	{
		console.log("animation frame supported")
		window.requestAnimationFrame(animate);
	}
})