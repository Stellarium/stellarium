import QtQuick 1.1

Item {
	id: button

	implicitWidth: 50
	implicitHeight: 50

	//ICS blue:
	//#7cf1ff
	//#33B5E5
	//#0099CC
	property color highlightColor: "#33B5E5"
	property string action: ""
	property string labelText: "test"
	property string imageSource: ""
	property string enabledImageSource: ""
	property string disabledImageSource: ""

	property real bgOpacity: 0.3

	signal clicked()
	signal pressed()
	signal released()

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

	Text {
		id: label
		anchors.centerIn: parent
		text: labelText
		color: "white"
	}

	Image {
		id: image
		fillMode: Image.PreserveAspectFit
		source: imageSource
	}

	onActionChanged : {
		clicked.connect(baseGui.getGuiActions(action).toggle());
		clicked.connect(baseGui.getGuiActions(action).trigger());

		if(baseGui.getGuiActions(action).checkable)
		{
			if(baseGui.getGuiActions(action).checked)
			{
				label.color = "red";
			}
			else
			{
				label.color = "blue";
			}
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
		button.released();
		highlightFadeOut.start();
	}

	Connections
	{
		target: baseGui.getGuiActions(action)
		onToggled: {
			console.log(label.text + " - we've been " + baseGui.getGuiActions(action).checked);
			if(baseGui.getGuiActions(action).checked)
			{
				label.color = "red";
			}
			else
			{
				label.color = "blue";
			}
		}

		onTriggered: {
			console.log("we've been triggered without a checkable");
		}
	}

	MouseArea {
		anchors.fill: parent
		onClicked: processClick()
		onPressed: {
			button.pressed();
			highlightFadeIn.start();
		}
		onReleased: {
			button.released();
			highlightFadeOut.start();
		}
	}
}
