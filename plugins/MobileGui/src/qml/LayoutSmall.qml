import QtQuick 1.1

Item {
	id: layout

	implicitHeight: 700
	implicitWidth: 400

	anchors.fill: parent

	Item {
		id: bottomBar

		anchors.left: parent.left
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		implicitHeight: 100
		height: mmY(8.6)

		Rectangle {
			anchors.fill: parent
			color: "black"
			opacity: 0.3
		}

		/* Buttons should be, at most, 8.6mm wide,
			but they can shrink to fit the screen */

		Row {
			id: buttonBar
			anchors.fill: parent
			state: "SHOWN"

			states: [
				State {
					name: "SHOWN"
					PropertyChanges {
						target: buttonBar
						opacity: 1
					}
				}
				State {
					name: "HIDDEN"
					PropertyChanges {
						target: buttonBar
						opacity: 0
					}
				}

			]

			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "A"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "B"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "C"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "D"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "E"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "F"
			}
		}

		Row {
			id: timeBar
			anchors.fill: parent
			opacity: 0`

			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "RW"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "PLAY"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "NOW"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "FF"
			}
			Button {
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: parent.width / 6
				labelText: "DATE/TIME"
			}
		}
	}


	Rectangle {
		id: infoBox
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.right: parent.right

		Connections	{
			target: baseGui
			onUpdated: title.text = skyInfo.getInfoText()
		}

		Column {
			Text {
				id: title
				text: ""
				color: "white"
			}
			Text {
				id: body
				text: ""
				color: "white"
			}
		}
	}
}
