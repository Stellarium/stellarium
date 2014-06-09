
import QtQuick 1.0
import Qt.labs.shaders 1.0
import Stellarium 1.0

Rectangle {
    ShaderEffectItem {
	property variant source: ShaderEffectSource { sourceItem: all; hideSource: true }
	anchors.fill: all
	blending: false
	visible: stelApp.nightMode

	fragmentShader:
	"
	varying highp vec2 qt_TexCoord0;
	uniform sampler2D source;
	void main(void)
	{
	    mediump vec3 color = texture2D(source, qt_TexCoord0).rgb;
	    mediump float luminance = max(max(color.r, color.g), color.b);
	    gl_FragColor = vec4(luminance, 0.0, 0.0, 1.0);
	}
	"
    }

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
}
