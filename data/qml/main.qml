
import QtQuick 1.0
import Qt.labs.shaders 1.0
import Stellarium 1.0

Rectangle {

	Item {
		id: all
		anchors.fill: parent
		StelSky {
			anchors.fill: parent
		}

		// This will generate a single big QGraphicsWidget, and initialize
		// Stellarium GUI into it.
		StelGui {
			anchors.fill: parent
		}
	}

	ShaderEffectItem {
		 property variant source: ShaderEffectSource { sourceItem: all; hideSource: true }
		 anchors.fill: all
		 blending: false
		 visible: stelApp.nightMode

		 fragmentShader: "
		 varying highp vec2 qt_TexCoord0;
		 uniform sampler2D source;
		 void main(void)
		 {
			mediump vec4 color = texture2D(source, qt_TexCoord0);
			mediump float luminance = dot(color, vec4(0.299, 0.587, 0.114, 0.0));
			gl_FragColor = vec4(luminance, 0.0, 0.0, 1.0);

		 }
		 "
	 }
}
