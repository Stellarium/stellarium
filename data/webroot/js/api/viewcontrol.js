/* jshint expr: true */

var ViewControl = (function($) {
    "use strict";

    //Private variables
    var lastXHR;
    var updateTimeout;
    var isMoving;

    var $view_fov;
    var userSliding = false;
    var minFov = 0.001389;
    //TODO make this depend on current projection
    var maxFov = 360;
    var fovSteps = 1000;
    var fovTimeout;
    var fovXHR;

    var lastServerFov;
    var queuedFov;

    var view_fov_text;
    var view_projection;


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

    //sets the FOV slider from a given fov
    function setFovSlider(fov) {
        //inverse of handleFovSlide
        var val = Math.pow(((fov - minFov) / (maxFov - minFov)), 1 / 4);

        var slVal = Math.round(val * fovSteps);
        $view_fov.slider("value", slVal);
    }

    function setFovText(fov) {
        view_fov_text.textContent = fov.toPrecision(3);
    }

    function handleFovSlide(val) {

        var s = val / fovSteps;
        var fov = minFov + Math.pow(s, 4) * (maxFov - minFov);

        console.log(val + " / " + fov);

        setFovText(fov);

        queueFovUpdate(fov);
    }

    function queueFovUpdate(fov) {
        queuedFov = fov;

        if (!fovXHR)
            fovServerUpdate();
    }

    function fovServerUpdate(noqueue) {
        var fov = queuedFov;
        fovXHR = $.ajax({
            url: "/api/main/fov",
            method: "POST",
            data: {
                fov: fov
            },
            success: function(data) {
                lastServerFov = fov;
                if (!noqueue) {
                    fovTimeout = setTimeout(fovServerUpdate, 250);
                }
            }
        });
    }

    function stopFovUpdate() {
        fovTimeout && clearTimeout(fovTimeout);
        if (fovXHR) {
            fovXHR.abort();
            fovXHR = undefined;
        }
        if(queuedFov !== lastServerFov) {
            fovServerUpdate(true);
        }
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


        $view_fov = $("#view_fov");
        $view_fov.slider({
            min: 0,
            max: fovSteps,

            slide: function(evt, ui) {
                handleFovSlide(ui.value);
            },
            start: function(evt, ui) {
                fovXHR = undefined;
                lastServerFov = 0;
                userSliding = true;
                console.log("slide start");
            },
            stop: function(evt, ui) {
                stopFovUpdate();
                userSliding = false;
                console.log("slide stop");
            }
        });

        $("#view_center").click(function(evt) {
            Actions.execute("actionGoto_Selected_Object");
        });

        view_fov_text = document.getElementById("view_fov_text");
        view_projection = document.getElementById("view_projection");
    }

    //Public stuff
    return {
        init: function() {
            initControls();
        },

        updateFromServer: function(data) {
            if (!userSliding) {
                setFovText(data.fov);
                setFovSlider(data.fov);
            }
            view_projection.textContent = data.projectionStr;
        }
    };
})(jQuery);