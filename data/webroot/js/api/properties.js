define(["jquery", "./remotecontrol", "./updatequeue"], function($, rc, UpdateQueue) {
	"use strict";

	var propsInitialized = false;
	var propData = {};
	var updateQueues = {};

	//used for initial property load
	function loadProperties() {
		$.ajax({
			url: "/api/stelproperty/list",
			success: function(data) {

				$.extend(true, propData, data);
				$.each(data, function(key, val) {
					firePropertyChanged(key);
				});

				console.log("initial property load completed");
				propsInitialized = true;

				$(publ).trigger("propertyListLoaded", data);
			},
			error: function(xhr, status, errorThrown) {
				console.log("Error updating property list");
				console.log("Error: " + errorThrown.message);
				console.log("Status: " + status);
				alert(rc.tr("Could not retrieve property list"));
			}
		});
	}

	function firePropertyChanged(id) {
		var prop = propData[id];
		console.log("stelPropertyChanged: " + id + " = " + prop.value);
		//trigger changed events, one generic and one specific
		$(publ).trigger("stelPropertyChanged", [id, prop]);
		$(publ).trigger("stelPropertyChanged:" + id, prop);
	}

	function getStelProp(id) {
		return propData[id] ? propData[id].value : undefined;
	}

	function setStelProp(id, val) {
		$.ajax({
			url: "/api/stelproperty/set",
			method: "POST",
			dataType: "text",
			data: {
				id: id,
				value: val
			},
			success: function(resp) {
				if (resp === "ok") {
					propData[id].value = val;
					firePropertyChanged(id);
				} else {
					alert(rc.tr("Property '%1' not accepted by server: ", id) + "\n" + resp);
				}
			},
			error: function(xhr, status, errorThrown) {
				console.log("Error changing property " + id);
				console.log("Error: " + errorThrown.message);
				console.log("Status: " + status);
				alert(rc.tr("Error sending property '%1' to server: ", id) + errorThrown.message);
			}
		});
	}

	function updateQueueCallback(jqXHR, textStatus) {
		if (textStatus === "success") {
			var id = this.lastSentData.id;
			var val = this.lastSentData.value;
			var resp = jqXHR.responseText;
			if (resp === "ok") {
				propData[id].value = val;
				firePropertyChanged(id);
			} else {
				alert(rc.tr("Property '%1' not accepted by server: ", id) + "\n" + resp);
			}
		}
	}

	function setStelPropQueued(id, val) {
		if (!updateQueues[id])
			updateQueues[id] = new UpdateQueue("/api/stelproperty/set", updateQueueCallback);
		updateQueues[id].enqueue({
			id: id,
			value: val
		});
	}

	$(rc).on("stelPropertiesChanged", function(evt, changes) {
		if (!propsInitialized) {
			//dont handle, tell main obj to send same id again
			evt.preventDefault();
			return;
		}

		var error = false;

		for (var name in changes) {
			if (name in propData) {
				if (changes[name] !== propData[name].value) {
					propData[name].value = changes[name];
					firePropertyChanged(name);
				}
			} else {
				//not really sure why this was necessary
				console.log("unknown property " + name + " changed");
				error = true;
			}
		}

		if (error)
			evt.preventDefault();
	});

	$(function() {
		loadProperties();
	});

	var publ = {
		getStelProp: getStelProp,
		setStelProp: setStelProp,
		setStelPropQueued: setStelPropQueued
	};
	return publ;

});