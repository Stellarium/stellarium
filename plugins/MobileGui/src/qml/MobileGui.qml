import QtQuick 1.0

Item
{
	//smallScreen: touchscreen phone sized
	// <right here: 5" mini tablets?>
	//mediumScreen: ~7" tablets
	//largeScreen: ~10" tablets
	property int mediumScreenSizemm: 290
	property int largeScreenSizemm: 440

	function mmX(number) {
		return Math.round(number * displayInfo.getDPIWidth() / 25.4)
	}

	function mmY(number) {
		return Math.round(number * displayInfo.getDPIHeight() / 25.4)
	}

	function dpX(number) {
		return Math.round(number * displayInfo.getDPIWidth() / 160)
	}

	function dpY(number) {
		return Math.round(number * displayInfo.getDPIHeight() / 160)
	}

	/* Placeholder values so the designer can be used;
	  these are set to fill 100% of the screen by the
	  MobileGui C++ class */
	width: 480
	height: 800

	/* Choose a layout based on the screen size */
	function chooseLayout()
	{
		var screenWidth = displayInfo.physicalWidth();
		if(screenWidth < mediumScreenSizemm || true)
		{
			return "LayoutSmall.qml";
		}
		else if(screenWidth < largeScreenSizemm)
		{
			return "LayoutMedium.qml";
		}
		else
		{
			return "LayoutLarge.qml";
		}
	}

	Keys.onPressed:
	{
		if(event.key == Qt.Key_MediaPrevious)
		{
			//hack: this is (or, will be) the Android 'back' button; see
			//http://stackoverflow.com/questions/9219747/how-to-block-the-back-key-in-android-when-using-qt
			event.accepted = true;
			console.log("Qt.Key_MediaPrevious !");
		}
		if(event.key == Qt.Key_Menu)
		{
			event.accepted = true;
			console.log("Qt.Key_Menu !");
		}
	}

	Loader
	{
		id: layoutLoader
		anchors.fill: parent
		source: chooseLayout()
	}
}
