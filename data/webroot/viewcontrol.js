"use strict";

var ViewControl = (new function($) {
    //Private variables
    var lastXHR;
    var updateTimeout;
    var isMoving;

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

    function initControls() {
        $("#view_upleft").mousedown(function(evt) {
            move(-1, 1);
        });

        $("#view_up").mousedown(function(evt) {
            move(0, 1);
        });

        $("#view_upright").mousedown(function(evt) {
            move(1, 1);
        });

        $("#view_left").mousedown(function(evt) {
            move(-1, 0);
        });

        $("#view_right").mousedown(function(evt) {
            move(1, 0);
        });

        $("#view_downleft").mousedown(function(evt) {
            move(-1, -1);
        });

        $("#view_down").mousedown(function(evt) {
            move(0, -1);
        });

        $("#view_downright").mousedown(function(evt) {
            move(1, -1);
        });

        $("#view_controls div").on("mouseup mouseleave", stopMovement);
    }

    //Public stuff
    return {
        init: function() {
            initControls();
        }
    }
}(jQuery));