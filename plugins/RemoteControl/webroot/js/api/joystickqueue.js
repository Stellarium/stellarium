define(["jquery", "settings"],
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
          queueTimeout = setTimeout(checkAndRequeueMove, settings.joystickDelay - delta);
        }
      }

      function doXHR(x, y) {
        //console.log("xhr " + x + ", " + y);
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
            if (isDragging || x !== queuedPos[0] || y !== queuedPos[1]) {
              checkAndRequeueMove();
            }
          });
      }

      function queueMove(x, y) {
        queuedPos = [x, y];
        checkAndRequeueMove();
      }

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
