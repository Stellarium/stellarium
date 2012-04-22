import QtQuick 1.0

//PopupMenu, for general popups where a list of clickable items needs to be shown
//Height, width, lineHeight, position ought to be adjusted for different popups
//Images can be shown for each item, but are optional.
//The only indication as to whether a checkable action is checked is image opacity
Item {
	id:popupMenu

	property ListModel model //the menu model to display
	property int lineHeight: dp(48)  //the height of each item
	property int menuWidth
	property int menuHeight
	property int menuX
	property int menuY

	anchors.fill: parent

	MouseArea {
		anchors.fill: parent
		onClicked: popupMenu.opacity = 0;
	}

	Rectangle {
		color:"#282828"
		width: menuWidth
		height: menuHeight
		x: menuX
		y: menuY

		/* Expected model format:
		ListModel {
				 ListElement { action: "some_action";
							   text: "label";
							   onClicked: function;
							   image: url; (optional) }
				 ...
			 }*/

		Component {
			id: popupMenuDelegate
			Clickable {
				action: model.action

				state: "ENABLED"

				states: [
					State {
						name: "ENABLED"
						when: checked
						PropertyChanges { target: image; opacity: 1.0}
					},
					State {
						name: "DISABLED"
						when: !checked
						PropertyChanges { target: image; opacity: 0.4}
					}
				]

				Column
				{
					anchors.fill: parent
					Image
					{
						id: image
						fillMode: Image.PreserveAspectFit
						sourceSize.width: popupMenu.lineHeight - dp(16) //todo: to accomodate tablet layouts, this and other similar values may need to lie instead inside Layout___.qml
						sourceSize.height: popupMenu.lineHeight - dp(16)
						anchors.verticalCenter: parent.verticalCenter
						source: model.image
					}

					Text {
						text: model.text
						anchors.verticalCenter: parent.verticalCenter
						font.pixelSize: dp(14)
						color: "white"
					}
				}
				height: popupMenu.lineHeight
				width: popupMenu.menuWidth
				onClicked: model.onClicked
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
