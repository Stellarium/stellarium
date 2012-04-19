import QtQuick 1.1

//A clickable is an abstract Item with partially-defined appearance.
//A clickable handles click events, and can (optionally) contain actions.
//Clicks are passed to actions, and actions can tell the clickable whether it's "checked" or not.
//Subclassed items are expected to use the "checked" status if they want it; clickable does nothing with it.
//Clickable's only appearance is that of a background highlight which appears if the clickable is clicked.
// (So far, every clickable thing needs that highlighting, no sense in breaking it apart further)
Item {
	id: clickable

	implicitWidth: 50
	implicitHeight: 50

	//ICS blue:
	//#7cf1ff
	//#33B5E5
	//#0099CC
	property color highlightColor: "#33B5E5"
	property string action: ""

	property real bgOpacity: 0.3

	signal clicked()
	signal pressed()
	signal released()

	property bool checked: true
	property bool checkable: false

	PropertyAnimation {
		id: highlightFadeIn;
		target: bgFill;
		properties: "opacity";
		to: bgOpacity;
		duration: 80

		easing {type: Easing.OutQuad;}
	}
	PropertyAnimation {
		id: highlightFadeOut;
		target: bgFill;
		properties: "opacity";
		to: 0;
		duration: 200

		easing {type: Easing.InQuad;}
	}
	Rectangle {
		id: bgFill
		anchors.fill: parent
		opacity: 0
		color: highlightColor
	}

	onActionChanged : {
		clicked.connect(baseGui.getGuiActions(action).toggle());
		clicked.connect(baseGui.getGuiActions(action).trigger());

		checkable = baseGui.getGuiActions(action).checkable;

		if(baseGui.getGuiActions(action).checked)
		{
			checked = true;
		}
		else
		{
			checked = false;
		}
	}

	function processClick()
	{
		if(action != "")
		{
			baseGui.getGuiActions(action).trigger();
		}
		clicked();

		// Possible to get stuck in an unreleased state
		clickable.released();
		highlightFadeOut.start();
	}

	Connections
	{
		target: baseGui.getGuiActions(action)
		onToggled: {
			//checkable = baseGui.getGuiActions(action).checkable;

			if(baseGui.getGuiActions(action).checked)
			{
				checked = true
			}
			else
			{
				checked = false;
			}
		}

		onTriggered: {
			//Flash the highlight or something?
			//checkable = baseGui.getGuiActions(action).checkable;
		}
	}

	MouseArea {
		anchors.fill: parent
		onClicked: processClick()
		onPressed: {
			clickable.pressed();
			//highlightFadeIn.start(); //trying without the fade-in: appears more responsive to the user
			bgFill.opacity = bgOpacity;
		}
		onReleased: {
			clickable.released();
			highlightFadeOut.start();
		}
	}
}
