import QtQuick 1.0

Item {
	/* Placeholder values so the designer can be used;
	  these are set to fill 100% of the screen by the
	  MobileGui C++ class */
	width: 480
	height: 800

	/* Dummy elements just to make it look "cool". */

	Rectangle
	{
		height: 80
		anchors.left: parent.left
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		color: "#545454"
		Text
		{
			text: "Buttons can go in here!"
			font.pointSize: 10
			color: "white"
		}
	}

	Text
	{
		anchors.left: parent.left
		anchors.top: parent.top
		text: "M42"
		font.pointSize: 18
		color: "white"
	}
}
