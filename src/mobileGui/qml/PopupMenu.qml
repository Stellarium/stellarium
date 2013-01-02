import QtQuick 1.0

//PopupMenu, for general popups where a list of clickable items needs to be shown
//Height, width, lineHeight, position ought to be adjusted for different popups
//Contains a checkbox; if this contains a checkable Clickable, it gets a checkbox
Item {
	id:popupMenu

    property variant model //the menu model to display

	property int lineHeight: dp(48)  //the height of each item
    property int imageSize: dp(36) //size (rather, either height or width) of images for each item (if they have them)
    property int fontSize: dp(18)

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
        if(width > menu.widest)
        {
            menu.widest = width;
        }

		return menu.widest;
	}

    Rectangle {
		id: menu
        color:"#55000000"
		anchors.fill: parent
		property int widest: dp(54)

		/* Expected model format:
		ListModel {
                 ListElement { action: baseGui.getGuiAction("actionSettings_Dialog")
                               imageSource: "image://mobileGui/settings"
                               overrideActionText: true //if false or omitted, the action's text property is used instead of the "text" property here
                               text: "Settings" }
				 ...
			 }*/

		Component {
			id: popupMenuDelegate
			Clickable {
                id: myClickable
                action: model.action == undefined ? baseGui.getGuiAction(model.actionString) : model.action
                height: lineHeight

				state: "UNCHECKABLE"

				states: [
					State {
						name: "ENABLED"
                        when: checked && checkable
                        PropertyChanges { target: checkbox; checked: true }
                        StateChangeScript {
                                 script: {
                                     if(model.imageSource != "" && image.sourceSize.height > 0)
                                     {
                                         image.opacity = 0.8;
                                         checkbox.opacity = 0.0;
                                     }
                                     else
                                     {
                                         image.opacity = 0;
                                         checkbox.opacity = 1;
                                     }
                                 }
                             }
					},
					State {
						name: "DISABLED"
                        when: !checked && checkable
						PropertyChanges { target: checkbox; checked: false }
                        StateChangeScript {
                                 script: {
                                     if(model.imageSource != "" && image.sourceSize.height > 0)
                                     {
                                         image.opacity = 0.2;
                                         checkbox.opacity = 0.0;
                                     }
                                     else
                                     {
                                         image.opacity = 0;
                                         checkbox.opacity = 1;
                                     }
                                 }
                             }
					},
					State {
						name: "UNCHECKABLE"
                        when: !checkable
						PropertyChanges { target: checkbox; opacity: 0	 }
                        StateChangeScript {
                                 script: {
                                     if(model.imageSource != "" && image.sourceSize.height > 0)
                                     {
                                         image.opacity = 0.8;
                                     }
                                     else
                                     {
                                         image.opacity = 0;
                                     }
                                 }
                             }
					}
				]

				Row
				{
					id: rowContent
					height: lineHeight
                    width: parent.width - dp(24)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: dp(12)

                    Image {
                        id: image
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: imageSize
                        sourceSize.height: imageSize
                        anchors.verticalCenter: parent.verticalCenter
                        source: model.imageSource
                    }

					Text {
                        id: textField
                        text: myClickable.action.text
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
                //onClicked: model.onClicked
                width: widestWidth(rowContent.childrenRect.width + dp(24))
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
