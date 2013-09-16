
import QtQuick 1.0
import Stellarium 1.0

Rectangle {

	StelSky {
		anchors.fill: parent
	}
	
	// This will generate a single big QGraphicsWidget, and initialize
	// Stellarium GUI into it.
	StelGui {
		anchors.fill: parent
	}
}
