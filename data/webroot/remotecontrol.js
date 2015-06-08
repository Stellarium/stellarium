 "use strict";

//update every second
var updateInterval = 1000;
var updatePoll = true;
var connectionLost = false;

var timeEditMode = false;
var timeEditDelay = 500; //the delay in ms after which time edits are sent to server

var animationSupported;

//the last received update data
var lastData;
//the time when the last data was received
var lastDataTime;
//the time when the time was last synced
var lastTimeSync;

//stores some jQuery selection results for faster re-use
var elemCache = {
}

//stores last displayed time to avoid potentially slow jquery update
var currentDisplayTime = {
}

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
            
            //update deltaT display
            var deltaTinSec =  data.time.deltaT * (60 * 60 * 24);
            elemCache.deltat.innerHTML = deltaTinSec.toFixed(2)+"s";

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

function postCmd(url,data,completeFunc){
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
		},
        complete: completeFunc
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

function forceSpinnerUpdate(){
    elemCache.date_year.spinner("value",currentDisplayTime.date.year);
    elemCache.date_month.spinner("value",currentDisplayTime.date.month);
    elemCache.date_day.spinner("value",currentDisplayTime.date.day);
    
    elemCache.time_hour.spinner("value",currentDisplayTime.time.hour);
    elemCache.time_minute.spinner("value",currentDisplayTime.time.minute);
    elemCache.time_second.spinner("value",currentDisplayTime.time.second);
    
    elemCache.input_jd.spinner("value",currentDisplayTime.jd);
    elemCache.input_mjd.spinner("value",currentDisplayTime.jd-2400000.5);
}

function updateTime()
{
	if(lastData && !timeEditMode)
	{
        var currentJday = getCurrentTime();
        var correctedTime = currentJday - lastData.time.deltaT;
        
        //what we have to animate depends on which tab is shown
        if(elemCache.timewidget.tabs('option', 'active') === 0) {
            //apply timezone
            var localTime = correctedTime + lastData.time.gmtShift;
            
            //this seems to work MUCH smoother in animation than using jquery for that
            var date = jdayToDate(localTime);
            if(currentDisplayTime.date.year !== date.year) {
                elemCache.date_year.spinner("value",date.year);
            }
            if(currentDisplayTime.date.month !== date.month) {
                elemCache.date_month.spinner("value",date.month);
            }
            if(currentDisplayTime.date.day !== date.day) {
                elemCache.date_day.spinner("value",date.day);
            }
            currentDisplayTime.date = date;
            
            var time = jdayToTime(localTime);
            if(currentDisplayTime.time.hour !== time.hour) {
                elemCache.time_hour.spinner("value",time.hour);
            }
            if(currentDisplayTime.time.minute !== time.minute) {
                elemCache.time_minute.spinner("value",time.minute);
            }
            if(currentDisplayTime.time.second !== time.second) {
                elemCache.time_second.spinner("value",time.second);
            }
            currentDisplayTime.time = time;
        }
        else {
            var jd = correctedTime.toFixed(5);
            
            if(currentDisplayTime.jd.toFixed(5) !== jd) {
                if(!elemCache.input_jd.is(":focus"))
                    elemCache.input_jd.spinner("value", correctedTime);
                //MJD directly depends on JD
                var mjd = (correctedTime-2400000.5);
                if(!elemCache.input_mjd.is(":focus"))
                    elemCache.input_mjd.spinner("value",mjd);
            }
            currentDisplayTime.jd = correctedTime;
        }
	}
	if(connectionLost)
	{
		var elapsed = ($.now() - lastDataTime) / 1000;
		var text = Math.floor(elapsed).toString();
        elemCache.noresponsetime.innerHTML = text;
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
			console.log("Error updating action list");
			console.log( "Error: " + errorThrown );
	        console.log( "Status: " + status );
			alert("Could not retrieve action list")
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
    
    //Cache some elements in a variable.
    //This seems to make access faster if used in an animation.
    elemCache.date_year = $("#date_year");
    elemCache.date_month = $("#date_month");
    elemCache.date_day = $("#date_day");
    elemCache.time_hour = $("#time_hour");
    elemCache.time_minute = $("#time_minute");
    elemCache.time_second = $("#time_second");
    elemCache.input_jd = $("#input_jd");
    elemCache.input_mjd = $("#input_mjd");
    elemCache.deltat = document.getElementById("deltat");
    elemCache.noresponsetime = document.getElementById("noresponsetime");
    //this needs a jquery wrapper
    elemCache.timewidget = $("#timewidget");
    
    //init display time
    resetCurrentDisplayTime();
    
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