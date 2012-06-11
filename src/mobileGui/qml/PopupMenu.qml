import QtQuick 1.0

//PopupMenu, for general popups where a list of clickable items needs to be shown
//Height, width, lineHeight, position ought to be adjusted for different popups
//Contains a checkbox; if this contains a checkable Clickable, it gets a checkbox
Item {
	id:popupMenu

	property ListModel model //the menu model to display

	property int lineHeight: dp(48)  //the height of each item
	property int fontSize: dp(14)

	anchors.fill: parent

	property int position: 0

	//Position constants (really, don't change these):
	property int positionCenter: 0
	property int positionTopRight: 1

	state: "dialog"

	MouseArea {
		anchors.fill: parent
		onClicked: popupMenu.opacity = 0;
	}

	states: [
		State {
			 name: "dialog"
			 when: position == positionCenter

			 PropertyChanges {
				 target: menu
				 anchors.rightMargin: dp(48)
				 anchors.leftMargin: dp(48)
				 anchors.topMargin: dp(64)
				 anchors.bottomMargin: dp(64)
			 }
		 },
		State {
			 name: "menu"

			 when: position == positionTopRight

			 PropertyChanges {
				 target: menu
				 anchors.rightMargin: 0
				 anchors.leftMargin: popupMenu.width - menu.widest
				 anchors.topMargin: dp(48)
				 anchors.bottomMargin: popupMenu.height - (list.childrenRect.height + dp(48))
			 }
		 }
	]

	function widestWidth (width)
	{
		if(width + dp(4) > menu.widest)
		{
			menu.widest = width + dp(4);
		}

		console.log("*B* " + width + " " + menu.widest);

		return menu.widest;
	}

	Rectangle {
		id: menu
		color:"#282828"
		anchors.fill: parent
		property int widest: dp(54)

		/* Expected model format:
		ListModel {
				 ListElement { action: "some_action";
							   text: "label";
							   onClicked: function; }
				 ...
			 }*/

		Component {
			id: popupMenuDelegate
			Clickable {
				action: model.action

				state: "UNCHECKABLE"

				states: [
					State {
						name: "ENABLED"
						when: checked && checkable
						PropertyChanges { target: checkbox; checked: true }
					},
					State {
						name: "DISABLED"
						when: !checked && checkable
						PropertyChanges { target: checkbox; checked: false }
					},
					State {
						name: "UNCHECKABLE"
						when: !checkable
						PropertyChanges { target: checkbox; opacity: 0	 }
					}
				]

				Row
				{
					id: rowContent
					height: lineHeight
					width: childrenRect.width
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.verticalCenter: parent.verticalCenter

					Text {
						text: model.text
						anchors.verticalCenter: parent.verticalCenter
						font.pixelSize: popupMenu.fontSize
						color: "white"
					}

					Checkbox
					{
						id: checkbox
						anchors.verticalCenter: parent.verticalCenter
					}
				}
				onClicked: model.onClicked
				width: widestWidth(rowContent.width)
			}
		}

		ListView {
			id: list
			boundsBehavior: Flickable.StopAtBounds
			anchors.fill: parent
			delegate:popupMenuDelegate
			model:popupMenu.model
		}
	}
}
