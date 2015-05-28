//update every second
var updateInterval = 1000;
var updatePoll = true;

var animationSupported;

//the last received update data
var lastData;
//the time when the last data was received
var lastDataTime;

//main update function
function update(requeue){
	$.ajax({
		url: "/api/main/status",
		dataType: "json",
		success: function(data){
			lastData = data;
			lastDataTime = $.now();

			//update time NOW
			if(!animationSupported)
				updateTime();

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
		},
		error: function(xhr, status,errorThrown){
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

var ONE_OVER_JD_MILLISECOND = 24*60*60*1000;
var JD_SECOND = 0.000011574074074074074074;
//this replicates time functions from Stellarium core (getPrintableDate)
//because the date range of Stellarium is quite a bit larger than JavaScript Date() can handle, it needs some custom support
function jdayToDate(jd)
{
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

function updateTime()
{
	if(lastData)
	{
		var currentJday;

		//progress the time depending on the elapsed time between last update and now, and the set time rate
		if(Math.abs(lastData.time.timerate-JD_SECOND)<0.0000001){
			//the timerate is 1:1 with real time
			//console.log("realtime");
			currentJday = lastData.time.jday + ($.now() - lastDataTime) / ONE_OVER_JD_MILLISECOND;
		}
		else
		{
			//console.log("unrealtime");
			currentJday = lastData.time.jday + (($.now() - lastDataTime) / 1000.0) * lastData.time.timerate;
		}
		var correctedTime = currentJday - lastData.time.deltaT;
		//apply timezone
		var localTime = correctedTime + lastData.time.gmtShift;

		//this seems to work MUCH smoother in animation than using jquery for that
		document.getElementById("date").innerHTML = getDateString(localTime);
		document.getElementById("time").innerHTML = getTimeString(localTime);
	}
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

function animate() {
	updateTime();

	if(animationSupported)
	{
		window.requestAnimationFrame(animate);
	}
}

$(document).ready(function () {

	$("#bt_runscript").click(function(){
    	var selection = $("#scriptlist").children("option").filter(":selected").text();

    	if(selection)
    	{
    		//post a run request
    		$.ajax({
    			url: "/api/scripts/run",
    			method: "POST",
    			dataType: "text",
    			data: {
    				id: selection
    			},
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
    				console.log("Error updating script list");
    				console.log( "Error: " + errorThrown );
    				console.log( "Status: " + status );
    				alert("Could not call server")
    			}
    		});
    	}
	});

	$("#bt_stopscript").click(function(){
		//post a stop request
    		$.ajax({
    			url: "/api/scripts/stop",
    			method: "POST",
    			dataType: "text",
    			timeout: 3000,
    			success: function(data){
    				console.log("server replied: " + data);
    				update();
    			},
    			error: function(xhr,status,errorThrown){
    				console.log("Error updating script list");
    				console.log( "Error: " + errorThrown );
    				console.log( "Status: " + status );
    				alert("Could not call server")
    			}
    		});
	});

    $("#scriptlist").change(function(){
    	var selection = $("#scriptlist").children("option").filter(":selected").text();

    	$("#bt_runscript").prop({
					disabled: false
				});
    	console.log("selected: " + selection);
    });

    animationSupported = (window.requestAnimationFrame !== undefined);

	//initial update
	fillScriptList();
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