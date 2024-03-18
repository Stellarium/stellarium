//This includes some easily changeable properties of the interface

define(function() {
  "use strict";

  var data = {};

  // Automatically poll the server for updates? Otherwise, only updated after commands are sent.
  data.updatePoll = true;
  //the interval for automatic polling
  data.updateInterval = 1000;
  //use the Browser's requestAnimationFrame for animation instead of setTimeout
  data.useAnimationFrame = true;
  //If animation frame is not used, this is the delay between 2 animation steps
  data.animationDelay = 500;
  //If no user changes happen after this time, the changes are sent to the Server
  data.editUpdateDelay = 500;
  //This is the delay after which the AJAX request spinner is shown.
  data.spinnerDelay = 100;
  //how often the joystick control sends updates in ms, smaller numbers are more reponsive but have more overhead
  //you could even set this to 0
  data.joystickDelay = 200;

  return data;
});
