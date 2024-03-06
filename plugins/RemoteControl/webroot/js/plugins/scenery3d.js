define(["jquery", "settings", "api/remotecontrol", "api/properties"],
  function($, settings, rc, propApi) {
    "use strict";

    if (!Date.now) {
      Date.now = function now() {
        return new Date().getTime();
      };
    }

    var $s3d_list;
    var $s3d_info;
    var $s3d_curscene;

    var sceneMap;
    var currentSceneID;
    var loadingSceneID;

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

    function initControls() {
      $s3d_list = $("#s3d_list");
      $s3d_info = $("#s3d_info");
      $s3d_curscene = $("#s3d_curscene");

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
