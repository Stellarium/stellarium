/* jshint expr: true */
define(["jquery", "./remotecontrol"], function($, rc) {
    "use strict";

    //Private variables
    var lastXHR;
    var updateTimeout;
    var isMoving;

    var fovTimeout;
    var fovXHR;

    var lastServerFov;
    var queuedFov;

    function move(x, y) {
        updateTimeout && clearTimeout(updateTimeout);

        if (x !== 0 || y !== 0)
            isMoving = true;
        else
            isMoving = false;

        if (lastXHR)
            lastXHR.abort();

        lastXHR = $.ajax({
            url: "/api/main/move",
            method: "POST",
            data: {
                x: x,
                y: y
            },
            success: function() {
                if (x !== 0 || y !== 0) {
                    updateTimeout = setTimeout(function() {
                        move(x, y);
                    }, 250); //repeat each 250ms to avoid Stellarium thinking the interface crashed
                }
            }
        });
    }

    function stopMovement() {
        if (isMoving)
            move(0, 0);
    }

    function queueFovUpdate(fov) {
        queuedFov = fov;

        if (!fovXHR)
            fovServerUpdate();
    }

    function fovServerUpdate() {

        if (queuedFov === lastServerFov) {
            //dont do another request just yet, nothing changed for now
            fovTimeout = setTimeout(fovServerUpdate, 250);
        } else {
            var fov = queuedFov;
            fovXHR = $.ajax({
                url: "/api/main/fov",
                method: "POST",
                data: {
                    fov: fov
                },
                success: function(data) {
                    lastServerFov = fov;

                    if (lastServerFov !== queuedFov) {
                        //we have not yet reached the queued value, queue another update after some time
                        fovTimeout = setTimeout(fovServerUpdate, 250);
                    } else {
                        fovXHR = undefined;
                        fovTimeout = undefined;
                    }
                }
            });
        }
    }

    $(rc).on("serverDataReceived", function(evt, data) {
        if (data.view.fov !== lastServerFov) {
            lastServerFov = data.view.fov;
            $(publ).trigger("fovChanged", lastServerFov);
        }
    });

    //Public stuff
    var publ = {
        moveUpLeft: function() {
            move(-1, 1);
        },

        moveUp: function() {
            move(0, 1);
        },

        moveUpRight: function() {
            move(1, 1);
        },

        moveLeft: function() {
            move(-1, 0);
        },

        moveRight: function() {
            move(1, 0);
        },

        moveDownLeft: function() {
            move(-1, -1);
        },

        moveDown: function() {
            move(0, -1);
        },

        moveDownRight: function() {
            move(1, -1);
        },

        stopMovement: stopMovement,

        setFOV: queueFovUpdate
    };

    return publ;
});