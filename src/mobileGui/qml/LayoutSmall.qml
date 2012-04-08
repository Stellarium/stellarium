import QtQuick 1.1

Item {
	id: layout

	implicitHeight: 700
	implicitWidth: 400

	anchors.fill: parent

	function spacerWidth()
	{
		return (parent.width - logoWidth() - 4 * minButtonWidth())
	}

	function minButtonWidth()
	{
		return Math.min((parent.width - logoWidth()) / 4, dp(56))
	}

	function logoWidth()
	{
		return Math.min(parent.width / 5, dp(56))
	}

	function searchFieldWidth()
	{
		return (parent.width - logoWidth() - minButtonWidth())
	}

	Item {
		id: buttonBar


		anchors.left: parent.left
		anchors.top: parent.top
		anchors.right: parent.right
		implicitHeight: 100
		//48dp is Android's magic grid size number
		height: dp(48)

		Rectangle {
			anchors.fill: parent
			color: "black"
			opacity: 0.3
		}

		Item
		{
			id: buttonRows
			anchors.fill: parent
			state: "START"

			states: [
				State {
					name: "START"
					PropertyChanges {target: startRow; opacity: 1}
					PropertyChanges {target: playbackRow; opacity: 0}
					PropertyChanges {target: searchRow; opacity: 0}
				},
				State {
					name: "PLAYBACK"
					PropertyChanges {target: startRow; opacity: 0}
					PropertyChanges {target: playbackRow; opacity: 1}
					PropertyChanges {target: searchRow; opacity: 0}
				},
				State {
					name: "SEARCH"
					PropertyChanges {target: startRow; opacity: 0}
					PropertyChanges {target: playbackRow; opacity: 0}
					PropertyChanges {target: searchRow; opacity: 1}
				}
			]

			transitions: [
				Transition {
					NumberAnimation { target: startRow; property: "opacity"; duration: 200 }
					NumberAnimation { target: playbackRow; property: "opacity"; duration: 200 }
					NumberAnimation { target: searchRow; property: "opacity"; duration: 200 }
				}
			]

			Row {
				id: startRow
				anchors.fill: parent

				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: logoWidth()
					labelText: " Logo"
					bgOpacity: 0
				}
				Item {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: spacerWidth();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/search"
					imageSize: dp(32)
					onClicked: buttonRows.state = "SEARCH"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/playback"
					imageSize: dp(32)
					onClicked: buttonRows.state = "PLAYBACK"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/accelerometer"
					imageSize: dp(32)
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/menu"
					imageSize: dp(32)
				}
			}

			Row {
				id: playbackRow
				anchors.fill: parent

				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: logoWidth()
					labelText: "<Logo"
					onClicked: buttonRows.state = "START"
				}
				Item {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: spacerWidth();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/rewind"
					imageSize: dp(32)
					action: "actionDecrease_Time_Speed"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/play"
					imageSize: dp(32)
					action: "actionSet_Real_Time_Speed"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/currenttime"
					imageSize: dp(32)
					action: "actionReturn_To_Current_Time"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					imageSource: "image://mobileGui/fastforward"
					imageSize: dp(32)
					action: "actionIncrease_Time_Speed"
				}
			}

			Row {
				id: searchRow
				anchors.fill: parent

				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: logoWidth()
					labelText: "<Logo"
					onClicked: buttonRows.state = "START"
				}
				Button {
					id: goButton
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					labelText: "Go"
				}
				Item
				{
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: searchFieldWidth()

					Rectangle {
						color: "white"
						opacity: 0.3
						anchors.fill: parent
						anchors.margins: dp(8)
					}

					Flickable
					{
						//; I'm tempted to just use a Qt widget for this, since QML isn't cooperating well
						id: flickBox

						anchors.fill: parent
						anchors.margins: dp(8)
						width: parent.width; height: parent.height
						interactive: true

						boundsBehavior: Flickable.StopAtBounds
						flickableDirection: Flickable.HorizontalFlick

						clip: true

						function ensureVisible(r)
						{
							if (contentX >= r.x)
								contentX = r.x;
							else if (contentX+width <= r.x+r.width)
								contentX = r.X+r.width-width;
						}

						TextEdit {
							id: searchField
							anchors.fill: parent
							width: parent.width; height: parent.height

							color: "white"
							font.pointSize: 8

							onCursorRectangleChanged: flickBox.ensureVisible(cursorRectangle)

							states: [
								State {
									name: "TYPING"
									when: searchField.cursorVisible
									StateChangeScript {script: freezeShowButtonBar(); }
									},
								State {
									name: "INACTIVE"
									when: !searchField.cursorVisible
									StateChangeScript {script: showButtonBar(); }
								 }
							]
						}
					}
				}
			}
		}

	}


	Item {
		id: infoBox
		anchors.bottom: parent.bottom
		anchors.left: parent.left
		anchors.bottomMargin: dp(16)
		anchors.leftMargin: dp(16)

		width: childrenRect.width
		height: childrenRect.height

		Connections	{
			target: baseGui
			onUpdated: title.text = stel.getInfoText()
		}

		Column {
			Text {
				id: title
				text: ""
				color: "white"
				style: Text.Raised; styleColor: "#000000"
			}
			Text {
				id: body
				text: ""
				color: "white"
				style: Text.Raised; styleColor: "#000000"
			}
		}
	}
}
