import QtQuick 1.0

//BarIcon is a Clickable item which features a centered image; for use on the "ActionBar"
Clickable {
	id: barIcon

	property string imageSource: ""
	property int imageSize

	state: "ENABLED"

	states: [
		State {
			name: "ENABLED"
			when: barIcon.checked
			PropertyChanges { target: image; opacity: 0.8}
		},
		State {
			name: "DISABLED"
			when: !barIcon.checked
			PropertyChanges { target: image; opacity: 0.3}
		}
	]

	Image {
		id: image
		fillMode: Image.PreserveAspectFit
		sourceSize.width: imageSize
		sourceSize.height: imageSize
		anchors.centerIn: parent
		source: imageSource
	}
}
