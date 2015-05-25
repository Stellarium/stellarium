//update every second
var updateInterval = 1000;
var updatePoll = true;

function retrieveScriptStatus() {
	$.ajax({
		url: "/api/scripts/status",
		dataType: "json",
		success: function(data)	{
			
			var textelem = $("#activescript");
			textelem.empty();
			if(data.scriptIsRunning)
			{
				textelem.append(data.runningScriptId);
			}
			else
			{
				textelem.append("-none-");
			}

			$("#bt_stopscript").prop({
					disabled: !data.scriptIsRunning
				});

			if(updatePoll){
				setTimeout(retrieveScriptStatus,updateInterval);
			}
		},
		error: function(xhr, status, errorThrown) {
			console.log("Error updating scripts");
			console.log( "Error: " + errorThrown );
	        console.log( "Status: " + status );
			setTimeout(retrieveScriptStatus,updateInterval);
		}
	});
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

    				//retrieveScriptStatus();
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
    				//TODO find out why one-shot update does not work here
    				//retrieveScriptStatus();
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

	//initial update
	fillScriptList();
	retrieveScriptStatus();
})