import QtQuick 1.1

Item {
	id: button

	implicitWidth: 50
	implicitHeight: 50

	property color highlightColor: "#3E78AB"
	property string action: ""
	property string labelText: "test"
	property string imageSource: ""

	signal clicked()

	PropertyAnimation
	{
		id: highlightFadeIn;
		target: bgFill;
		properties: "opacity";
		to: 1;
		duration: 100

		easing {type: Easing.OutQuad;}
	}
	PropertyAnimation
	{
		id: highlightFadeOut;
		target: bgFill;
		properties: "opacity";
		to: 0;
		duration: 100

		easing {type: Easing.InQuad;}
	}
	Rectangle
	{
		id: bgFill
		anchors.fill: parent
		opacity: 0
		color: highlightColor
	}

	Text
	{
		id: label
		anchors.centerIn: parent
		text: labelText
		color: "white"
	}

	Image
	{
		id: image
		fillMode: Image.PreserveAspectFit
		source: imageSource
	}

	onActionChanged :
	{
		clicked.connect(baseGui.getGuiActions(action).trigger());
		baseGui.getGuiActions(action).onTriggered.connect(doSomething);
	}

	function doSomething()
	{
		color = "red";
	}

	function processClick()
	{
		if(action != "")
		{
			//baseGui.getGuiActions(action).trigger();
		}
		clicked();
	}

	/*Connections
	{
		target: baseGui.getGuiActions(action)
		onTriggered: color = red
	}*/

	MouseArea
	{
		anchors.fill: parent
		onClicked: processClick()
		onPressed: highlightFadeIn.start()
		onReleased: highlightFadeOut.start()
	}
}
