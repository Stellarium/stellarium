//
// Name: ScreenImageMgr Demo
// License: Public Domain
// Author: Matthew Gates
// Description: An example script showing how to use the ScreenImageMgr module.
//

var rootLabelText = "";
var statusLabelId = -1;
function useStatusLabel(root, x, y, size, color)
{
	rootLabelText = root
	statusLabelId = LabelMgr.labelScreen("", x, y, true, size, color);
}

function status(text)
{
	if (statusLabelId!=-1)
	{
		if (text!="")
		{
			LabelMgr.setLabelText(statusLabelId, rootLabelText + text);
		}
		else
		{
			LabelMgr.setLabelText(statusLabelId, "");
		}
	}
}


