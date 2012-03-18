import QtQuick 1.1

Item {
	id: layout

	implicitHeight: 700
	implicitWidth: 400

	anchors.fill: parent

	function stretchButtonWidth()
	{
		return (parent.width - logoWidth()) / 4
	}

	function minButtonWidth()
	{
		return Math.min(parent.width / 5, dpX(48))
	}

	function logoWidth()
	{
		return Math.min(parent.width / 5, dpX(56))
	}

	function searchFieldWidth()
	{
		return (parent.width - logoWidth() - minButtonWidth())
	}

	Timer {
		id: fadeTimer
		interval: 1000
		running: true
		repeat : false
		onTriggered: {
			if(buttonBar.state == "SHOWN")
			{
				buttonBar.state = "FADED"
			}
		}
	}

	function freezeShowButtonBar()
	{
		buttonBar.state = "SHOWN";
		fadeTimer.stop();
	}

	function showButtonBar()
	{
		buttonBar.state = "SHOWN";
		fadeTimer.restart();
	}

	function hideButtonBar()
	{
		buttonBar.state = "HIDDEN";
		fadeTimer.stop();
	}

	Item {
		id: buttonBar


		anchors.left: parent.left
		anchors.top: parent.top
		anchors.right: parent.right
		implicitHeight: 100
		//48dp is Android's magic grid size number
		height: dpY(48)
		state: "SHOWN"

		states: [
			State {
				name: "SHOWN"
				PropertyChanges {
					target: buttonBar
					opacity: 1
				}
			},
			State {
				name: "FADED"
				PropertyChanges {
					target: buttonBar
					opacity: 0.3
				}
			}
		]

		transitions: [
			Transition {
				to: "SHOWN"
				NumberAnimation { target: buttonBar; property: "opacity"; duration: 0 }
			},
			Transition {
				to: "FADED"
				NumberAnimation { target: buttonBar; property: "opacity"; duration: 200 }
			}
		]

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
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "Srch"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
					onClicked: buttonRows.state = "SEARCH"
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "Play"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop(); buttonRows.state = "PLAYBACK"}
					onClicked: buttonRows.state = "PLAYBACK"
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "Accl"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: ". . ."
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
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
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onClicked: buttonRows.state = "START"
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "RW"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "Play"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "Now"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
				}
				Button {
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: stretchButtonWidth()
					labelText: "FF"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
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
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onClicked: buttonRows.state = "START"
					onReleased: fadeTimer.restart();
				}
				Button {
					id: goButton
					anchors.top: parent.top
					anchors.bottom: parent.bottom
					width: minButtonWidth()
					labelText: "Go"
					onPressed: {buttonBar.state = "SHOWN"; fadeTimer.stop()}
					onReleased: fadeTimer.restart();
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
						anchors.margins: dpX(8)
					}

					Flickable
					{
						//; I'm tempted to just use a Qt widget for this, since QML isn't cooperating well
						id: flickBox

						anchors.fill: parent
						anchors.margins: dpX(8)
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
		anchors.bottomMargin: dpY(16)
		anchors.leftMargin: dpX(16)

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
			}
			Text {
				id: body
				text: ""
				color: "white"
			}
		}
	}
}
