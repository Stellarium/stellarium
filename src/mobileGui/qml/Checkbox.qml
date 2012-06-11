import QtQuick 1.0

/* A themed checkbox. It can be checked or unchecked. */

Item {
	width: dp(32)
	height: dp(32)
	property bool checked: false

	state: "ENABLED"

	states: [
		State {
			name: "ENABLED"
			when: checked
			PropertyChanges { target: checkedImage; opacity: 1.0}
			PropertyChanges { target: uncheckedImage; opacity: 0.0}
		},
		State {
			name: "DISABLED"
			when: !checked
			PropertyChanges { target: checkedImage; opacity: 0.0}
			PropertyChanges { target: uncheckedImage; opacity: 1.0}
		}
	]

	Image {
		id: checkedImage
		fillMode: Image.PreserveAspectFit
		sourceSize.width: dp(32)
		sourceSize.height: dp(32)
		anchors.centerIn: parent
		source: "image://mobileGui/btn_check_on"
	}

	Image {
		id: uncheckedImage
		fillMode: Image.PreserveAspectFit
		sourceSize.width: dp(32)
		sourceSize.height: dp(32)
		anchors.centerIn: parent
		source: "image://mobileGui/btn_check_off"
	}
}
