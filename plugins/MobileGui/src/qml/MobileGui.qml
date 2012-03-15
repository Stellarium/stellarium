import QtQuick 1.0

Item {
	//minimum dp for each category

	//smallScreen: touchscreen phone sized
	//tweenerScreenDp: ~5" mini tablets/phablets
	//mediumScreen: ~7" tablets
	//largeScreen: ~10" tablets
	property int normalPhoneScreenDp: 290
	property int tweenerScreenDp: 460
	property int mediumScreenDp: 580
	property int largeScreenDp: 700

	focus: true

	function mmX(number)
	{
		return Math.round(number * displayInfo.getDPIWidth() / 25.4)
	}

	function mmY(number)
	{
		return Math.round(number * displayInfo.getDPIHeight() / 25.4)
	}

	function dpX(number)
	{
		return Math.round(number * displayInfo.getDPIWidth() / 160)
	}

	function dpY(number)
	{
		return Math.round(number * displayInfo.getDPIHeight() / 160)
	}

	function totalDpWidth()
	{
		return width * 160 / displayInfo.getDPIHeight()
	}

	/* Placeholder values so the designer can be used;
	  these are set to fill 100% of the screen by the
	  MobileGui C++ class */
	width: 480
	height: 800

	/* Choose a layout based on the screen size */
	function chooseLayout()
	{
		var screenWidth = totalDpWidth();
		return "LayoutSmall.qml";

		if(screenWidth < normalPhoneScreenDp)
		{
			return "LayoutExtraSmall.qml";
		}
		else if(screenWidth < tweenerScreenDp)
		{
			return "LayoutSmall.qml";
		}
		else if(screenWidth < mediumScreenDp)
		{
			return "LayoutSmall.qml";
			//return "LayoutTweener.qml";
		}
		else if(screenWidth < largeScreenDp)
		{
			return "LayoutMedium.qml";
		}
		else
		{
			return "LayoutLarge.qml";
		}
	}

	Loader {
		id: layoutLoader
		anchors.fill: parent
		source: chooseLayout()
	}
}
