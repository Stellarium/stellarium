define(["jquery", "settings", "jquery-ui"],
    function($, settings) {
        "use strict";

        function JoystickQueue(control, urlAdr, maxspeed) {
            var $joycontrol = $(control);
            var url = urlAdr;
            var isDragging = false;
            var queuedPos = [0, 0];
            var queueTimeout;
            var lastXHR;
            var lastXHRTime = 0;

            function checkAndRequeueMove() {
                if (lastXHR) //there is already a request running, wait until it completes
                    return;

                if (queueTimeout) {
                    clearTimeout(queueTimeout);
                    queueTimeout = undefined;
                }
                var delta = Date.now() - lastXHRTime;
                if (delta > settings.joystickDelay) {
                    //perform AJAX now
                    doXHR(queuedPos[0], queuedPos[1]);
                } else {
                    //requeue to check again later
                    queueTimeout = setTimeout(checkAndRequeueMove, settings
                        .joystickDelay - delta);
                }
            }

            function doXHR(x, y) {
                console.log("XHR to " + url + ": " + x + ", " + y);
                lastXHRTime = Date.now();
                lastXHR = $.ajax({
                        url: url,
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

            function queueMove(x, y) {
                queuedPos = [x, y];
                checkAndRequeueMove();
            }

            function createJQJoystick(control) {
                $(control).draggable({
                    revert: true,
                    revertDuration: 100,
                    cursor: "move",
                    //containment: "parent",
                    scroll: false,
                    drag: function(evt, ui) {
                        var ct = $(this).parent();
                        var halfHeight = ui.helper.height() / 2;
                        var halfWidth = ui.helper.width() / 2;

                        //calculate radius
                        var ctCenter = [ct.height() / 2, ct.width() /
                            2
                        ];
                        var pos = [halfHeight + ui.position.top -
                            ctCenter[0],
                            halfWidth + ui.position.left -
                            ctCenter[1]
                        ];

                        var curRad = Math.sqrt(pos[0] * pos[0] +
                            pos[1] * pos[1]);
                        var maxRad = Math.min(ctCenter[0],
                            ctCenter[1]);
                        var radFactor = Math.min(1.0, curRad /
                            maxRad);
                        var angle = Math.atan2(pos[0], pos[1]);

                        //position in the unit circle
                        var unitPos = [Math.sin(angle) *
                            radFactor, Math.cos(angle) *
                            radFactor
                        ];

                        if (curRad > maxRad) {
                            ui.position.top = Math.floor(
                                unitPos[0] * maxRad +
                                ctCenter[0] -
                                halfHeight);
                            ui.position.left = Math.floor(
                                unitPos[1] * maxRad +
                                ctCenter[1] -
                                halfWidth);
                        }

                        $(this).trigger("unitDrag", {
                            x: unitPos[1],
                            y: -unitPos[0]
                        });
                    }
                });
            }

            function createJoyButtons(control) {
                var $ctrl = $(control);
                var $parent = $ctrl.parent(".joystickcontainer");

                var dirData = {
                    "up": [0, 1],
                    "left": [-1, 0],
                    "right": [1, 0],
                    "down": [0, -1]
                };

                $.each(dirData, function(key, val) {
                    var btn = $(
                        "<div class='joystickbutton joystickbutton-" +
                        key + "'></div>");
                    btn.data("joydir", val);
                    $ctrl.before(btn);
                });

                $parent.on("mousedown", ".joystickbutton", function(evt) {
                    isDragging = true;
                    var moveDir = $(this).data("joydir");
                    queueMove(moveDir[0], moveDir[1]);
                });

                $parent.on("mouseup", ".joystickbutton", function(evt) {
                    isDragging = false;
                    queueMove(0, 0);
                });
            }

            createJQJoystick($joycontrol);
            createJoyButtons($joycontrol);

            $joycontrol.on("unitDrag", $.proxy(function(evt, pos) {
                //up to 50 times speedup like the base app
                var moveX = maxspeed * pos.x * pos.x * pos.x;
                var moveY = maxspeed * pos.y * pos.y * pos.y;
                queueMove(moveX, moveY);
            }, this));

            $joycontrol.on("dragstart", $.proxy(function() {
                //console.log("dragstart");
                isDragging = true;
            }, this));

            $joycontrol.on("dragstop", $.proxy(function() {
                //console.log("dragstop");
                isDragging = false;
                queueMove(0, 0);
            }, this));
        }

        return JoystickQueue;
    });
