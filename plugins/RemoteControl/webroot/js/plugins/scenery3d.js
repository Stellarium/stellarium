define(["jquery", "api/remotecontrol", "api/properties", "jquery-ui"], function(
    $, rc,
    propApi) {
    "use strict";

    if (!Date.now) {
        Date.now = function now() {
            return new Date().getTime();
        };
    }

    var $s3d_list;
    var $s3d_info;
    var $s3d_curscene;
    var $s3d_joy;

    var sceneMap;
    var currentSceneID;
    var loadingSceneID;
    var lastXHR;
    var lastXHRTime = 0;
    var queuedPos;
    var queueTimeout;
    var isDragging;

    function updateCurrentSceneName() {
        if (!sceneMap) return;

        var str = rc.tr("-none-");
        if (currentSceneID) {
            str = sceneMap[currentSceneID];
        }

        if (loadingSceneID) {
            str = str + " " + rc.tr("(Currently loading: %1)",
                sceneMap[
                    loadingSceneID]);
        }

        $s3d_curscene.text(str);
    }

    function loadScenes() {
        $.ajax({
            url: "/api/scenery3d/listscene",
            success: function(data) {
                var sortable = [];
                var val;
                for (val in data)
                    sortable.push([val, data[val]]);

                sortable.sort(function(a, b) {
                    if (a[1] < b[1])
                        return -1;
                    if (a[1] > b[1])
                        return 1;
                    return 0;
                });

                $s3d_list.empty();

                sortable.forEach(function(val) {
                    var option = document.createElement(
                        "option");
                    option.value = val[0];
                    option.textContent = val[1];
                    $s3d_list.append(option);
                });

                sceneMap = data;
                updateCurrentSceneName();
            },
            error: function(xhr, status, errorThrown) {
                console.log("Error updating Scenery3d list");
                console.log("Error: " + errorThrown.message);
                console.log("Status: " + status);
                alert(rc.tr(
                    "Could not retrieve Scenery3d list"
                ));
            }
        });
    }

    function setScene() {
        var selID = $s3d_list.val();
        rc.postCmd("/api/scenery3d/loadscene", {
            id: selID
        });
    }

    function checkAndRequeueMove() {
        if (lastXHR) //there is already a request running, wait until it completes
            return;

        if (queueTimeout) {
            clearTimeout(queueTimeout);
            queueTimeout = undefined;
        }
        var delta = Date.now() - lastXHRTime;
        if (delta > 250) {
            //perform AJAX now
            doXHR(queuedPos[0], queuedPos[1]);
        } else {
            //requeue to check again later
            queueTimeout = setTimeout(checkAndRequeueMove, 250 - delta);
        }
    }

    function doXHR(x, y) {
        //console.log("xhr " + x + ", " + y);
        lastXHRTime = Date.now();
        lastXHR = $.ajax({
                url: "/api/scenery3d/move",
                method: "POST",
                data: {
                    x: x,
                    y: y
                }
            })
            .always(function() {
                lastXHR = null; //indicate request is not running
                //check if we need to requeue
                if (isDragging || x !== queuedPos[0] || y !==
                    queuedPos[1]) {
                    checkAndRequeueMove();
                }
            });
    }

    //this function makes sure the updates are sent at most each 250ms
    function queueMove(x, y) {
        queuedPos = [x, y];
        checkAndRequeueMove();
    }

    function initControls() {
        $s3d_list = $("#s3d_list");
        $s3d_info = $("#s3d_info");
        $s3d_curscene = $("#s3d_curscene");
        $s3d_joy = $("#s3d_joy");

        $s3d_joy.on("unitDrag", function(evt, pos) {
            //up to 50 times speedup like the base app
            var moveX = 50 * pos.x * pos.x * pos.x;
            var moveY = 50 * pos.y * pos.y * pos.y;
            queueMove(moveX, moveY);
        });

        $s3d_joy.on("dragstart", function() {
            //console.log("dragstart");
            isDragging = true;
        });

        $s3d_joy.on("dragstop", function() {
            //console.log("dragstop");
            isDragging = false;
            queueMove(0, 0);
        });

        loadScenes();

        $(propApi).on("stelPropertyChanged:Scenery3d.currentSceneID",
            function(evt, data) {
                currentSceneID = data.value;
                updateCurrentSceneName();
            });
        currentSceneID = propApi.getStelProp("Scenery3d.currentSceneID");

        $(propApi).on("stelPropertyChanged:Scenery3d.loadingSceneID",
            function(evt, data) {
                loadingSceneID = data.value;
                updateCurrentSceneName();
            });
        loadingSceneID = propApi.getStelProp("Scenery3d.loadingSceneID");

        updateCurrentSceneName();

        $s3d_list.change(function() {
            $s3d_info.attr("src",
                "/api/scenery3d/scenedescription/" +
                encodeURIComponent($s3d_list.val()) + "/");
        });
        $s3d_list.dblclick(setScene);
        $("#s3d_load").click(setScene);
    }

    $(initControls);
});
