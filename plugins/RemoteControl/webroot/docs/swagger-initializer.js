window.onload = function() {
  //<editor-fold desc="Changeable Configuration Block">

  window.ui = SwaggerUIBundle({
    url: "./stelOpenAPI.yaml",
    dom_id: '#swagger-ui',
    deepLinking: true,
    presets: [
      SwaggerUIBundle.presets.apis,
      SwaggerUIStandalonePreset
    ],
    plugins: [
      SwaggerUIBundle.plugins.DownloadUrl
    ],
    layout: "StandaloneLayout"
  });

  //</editor-fold>
};
