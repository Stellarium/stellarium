import QtQuick 1.0

//BarIcon is a Clickable item which features a centered image; for use on the "ActionBar"
Clickable {
	id: barIcon

	property string imageSource: ""
	property string enabledImageSource: ""
	property string disabledImageSource: ""
	property int imageSize

	Image {
		id: image
		fillMode: Image.PreserveAspectFit
		sourceSize.width: imageSize
		sourceSize.height: imageSize
		anchors.centerIn: parent
		opacity: 0.8
		source: imageSource
	}
}
