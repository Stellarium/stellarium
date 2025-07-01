#include "SkyCultureMaker.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "gui/ScmConstellationDialog.hpp"
#include "gui/ScmHideOrAbortMakerDialog.hpp"
#include "gui/ScmSkyCultureDialog.hpp"
#include "gui/ScmSkyCultureExportDialog.hpp"
#include "gui/ScmStartDialog.hpp"

#include "ScmDraw.hpp"
#include <vector>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>

/**
 * Managing the creation process of a new sky culture.
 * 1. Navigate in stellarium (UI) to the location of interest (from where the culture should be created)
 * 2. Starting creation process (click in UI)
 * 3. Draw lines from start to star
 *   a) Only stars should be selectable
 *   b) Add functionality to draw separated/unconnected lines (e.g. cross constelation)
 *   c) Add functionality to delete a line
 *     I)  Deleting a inner line of a stick figure should split the figure into two stick figures
 *     II) Connecting two stick figures should merge them into one stick figure
 * 4. Add button to save sky culture
 * 5. Click save button opens dialog to name: sky culture, lines, aliass, ...
 * 6. Completing the dialog (check that all needed arguments are existing and valid) converts intern c++ object to json
 */

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just
 after loading the dynamic library
*************************************************************************/
StelModule *SkyCultureMakerStelPluginInterface::getStelModule() const
{
	return new SkyCultureMaker();
}

StelPluginInfo SkyCultureMakerStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(SkyCultureMaker);

	StelPluginInfo info;
	info.id            = "SkyCultureMaker";
	info.displayedName = "Sky Culture Maker";
	info.authors = "Vincent Gerlach (RivinHD), Luca-Philipp Grumbach (xLPMG), Fabian Hofer (Integer-Ctrl), Richard "
		       "Hofmann (ZeyxRew), Mher Mnatsakanyan (MherMnatsakanyan03)";
	info.contact = "Contact us using our GitHub usernames, via an Issue or the Discussion tab in the Stellarium "
		       "repository.";
	info.description = "Plugin to draw and export sky cultures in Stellarium.";
	info.version     = SKYCULTUREMAKER_PLUGIN_VERSION;
	info.license     = SKYCULTUREMAKER_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
SkyCultureMaker::SkyCultureMaker()
	: isScmEnabled(false)
	, isLineDrawEnabled(false)
{
	qDebug() << "SkyCulture Maker constructed";

	setObjectName("SkyCultureMaker");
	font.setPixelSize(25);

	drawObj                   = new scm::ScmDraw();
	scmStartDialog            = new ScmStartDialog(this);
	scmSkyCultureDialog       = new ScmSkyCultureDialog(this);
	scmConstellationDialog    = new ScmConstellationDialog(this);
	scmSkyCultureExportDialog = new ScmSkyCultureExportDialog(this);
	scmHideOrAbortMakerDialog = new ScmHideOrAbortMakerDialog(this);
}

/*************************************************************************
 Destructor
*************************************************************************/
SkyCultureMaker::~SkyCultureMaker()
{
	// Initalized on start
	delete drawObj;
	delete scmStartDialog;
	delete scmSkyCultureDialog;
	delete scmConstellationDialog;
	delete scmSkyCultureExportDialog;
	delete scmHideOrAbortMakerDialog;

	if (currentSkyCulture != nullptr)
	{
		delete currentSkyCulture;
	}

	qDebug() << "Unloaded plugin \"SkyCultureMaker\"";
}

void SkyCultureMaker::setActionToggle(const QString &id, bool toggle)
{
	StelActionMgr *actionMgr = StelApp::getInstance().getStelActionManager();
	auto action              = actionMgr->findAction(id);
	if (action)
	{
		action->setChecked(toggle);
	}
	else
	{
		qDebug() << "Sky Culture Maker: Could not find action: " << id;
	}
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SkyCultureMaker::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName) + 10.;
	if (actionName == StelModule::ActionHandleMouseClicks) return -11;
	return 0;
}

/*************************************************************************
 Init our module
*************************************************************************/
void SkyCultureMaker::init()
{
	qDebug() << "init called for SkyCultureMaker";

	StelApp &app = StelApp::getInstance();

	addAction(actionIdLine, groupId, N_("Sky Culture Maker"), "enabledScm");

	// Add a SCM toolbar button for starting creation process
	try
	{
		QPixmap iconScmDisabled(":/SkyCultureMaker/bt_SCM_Off.png");
		QPixmap iconScmEnabled(":/SkyCultureMaker/bt_SCM_On.png");
		qDebug() << (iconScmDisabled.isNull() ? "Failed to load image: bt_SCM_Off.png"
		                                      : "Loaded image: bt_SCM_Off.png");
		qDebug() << (iconScmEnabled.isNull() ? "Failed to load image: bt_SCM_On.png"
		                                     : "Loaded image: bt_SCM_On.png");

		StelGui *gui = dynamic_cast<StelGui *>(app.getGui());
		if (gui != Q_NULLPTR)
		{
			toolbarButton = new StelButton(Q_NULLPTR, iconScmEnabled, iconScmDisabled,
			                               QPixmap(":/graphicGui/miscGlow32x32.png"), actionIdLine, false);
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "Unable create toolbar button for SkyCultureMaker plugin: " << e.what();
	}
}

/***********************
 Manage creation process
***********************/

void SkyCultureMaker::setToolbarButtonState(bool b)
{
	setActionToggle(actionIdLine, b);
	toolbarButton->setChecked(b);
}

void SkyCultureMaker::startScmProcess()
{
	if (!isScmEnabled)
	{
		isScmEnabled = true;
		emit eventIsScmEnabled(true);
		setToolbarButtonState(true); // Toggle the toolbar button to enabled
	}

	if (isAnyDialogHidden())
	{
		restoreScmDialogVisibilityState();
	}
	else
	{
		setStartDialogVisibility(true);
	}
}

void SkyCultureMaker::stopScmProcess()
{
	// If the start dialog is visible, hide it
	if (scmStartDialog->visible())
	{
		setStartDialogVisibility(false);
		setToolbarButtonState(false); // Turn OFF the toolbar button (image OFF)
		return;
	}

	// If the converter dialog is visible, hide it
	if (scmStartDialog->isConverterDialogVisible())
	{
		scmStartDialog->setConverterDialogVisibility(false);
		if (isScmEnabled)
		{
			isScmEnabled = false;
			emit eventIsScmEnabled(false);
			setToolbarButtonState(false); // Toggle the toolbar button to disabled
		}
	}

	// If any other dialog is visible, don't stop the process — just keep UI state ON
	if (isAnyDialogVisible())
	{
		setHideOrAbortMakerDialogVisibility(true);
		setToolbarButtonState(true); // Keep the toolbar button ON
		return;
	}

	// Otherwise, actually stop the process
	if (isScmEnabled)
	{
		isScmEnabled = false;
		emit eventIsScmEnabled(false);
		setToolbarButtonState(false); // Toggle the toolbar button to disabled
	}
}

void SkyCultureMaker::draw(StelCore *core)
{
	if (isLineDrawEnabled && drawObj != nullptr)
	{
		drawObj->drawLine(core);
	}

	if (isScmEnabled && currentSkyCulture != nullptr)
	{
		currentSkyCulture->draw(core);
	}

	if (isScmEnabled && tempArtwork != nullptr)
	{
		tempArtwork->draw(core);
	}
}

bool SkyCultureMaker::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	if (isLineDrawEnabled)
	{
		if (drawObj->handleMouseMoves(x, y, b))
		{
			return true;
		}
	}

	return false;
}

void SkyCultureMaker::handleMouseClicks(QMouseEvent *event)
{
	if (isLineDrawEnabled)
	{
		drawObj->handleMouseClicks(event);
		if (event->isAccepted())
		{
			return;
		}
	}

	// Continue any other events to be handled...
}
void SkyCultureMaker::handleKeys(QKeyEvent *e)
{
	if (isLineDrawEnabled)
	{
		drawObj->handleKeys(e);
		if (e->isAccepted())
		{
			return;
		}
	}
}

void SkyCultureMaker::setIsScmEnabled(bool b)
{
	if (b == true)
	{
		startScmProcess();
	}
	else
	{
		stopScmProcess();
	}
}

void SkyCultureMaker::setStartDialogVisibility(bool b)
{
	if (b != scmStartDialog->visible())
	{
		scmStartDialog->setVisible(b);
	}
}

void SkyCultureMaker::setSkyCultureDialogVisibility(bool b)
{
	if (b != scmSkyCultureDialog->visible())
	{
		scmSkyCultureDialog->setVisible(b);
	}
}

void SkyCultureMaker::setConstellationDialogVisibility(bool b)
{
	if (b != scmConstellationDialog->visible())
	{
		scmConstellationDialog->setVisible(b);
	}

	setIsLineDrawEnabled(b);
}

void SkyCultureMaker::setSkyCultureExportDialogVisibility(bool b)
{
	if (b != scmSkyCultureExportDialog->visible())
	{
		scmSkyCultureExportDialog->setVisible(b);
	}
}

void SkyCultureMaker::setHideOrAbortMakerDialogVisibility(bool b)
{
	if (b != scmHideOrAbortMakerDialog->visible())
	{
		scmHideOrAbortMakerDialog->setVisible(b);
	}
}

void SkyCultureMaker::setIsLineDrawEnabled(bool b)
{
	isLineDrawEnabled = b;
}

void SkyCultureMaker::triggerDrawUndo()
{
	if (isLineDrawEnabled)
	{
		drawObj->undoLastLine();
	}
}

void SkyCultureMaker::setDrawTool(scm::DrawTools tool)
{
	drawObj->setTool(tool);
}

void SkyCultureMaker::setNewSkyCulture()
{
	if (currentSkyCulture != nullptr)
	{
		delete currentSkyCulture;
	}
	currentSkyCulture = new scm::ScmSkyCulture();
}

scm::ScmSkyCulture *SkyCultureMaker::getCurrentSkyCulture()
{
	return currentSkyCulture;
}

scm::ScmDraw *SkyCultureMaker::getScmDraw()
{
	return drawObj;
}

void SkyCultureMaker::resetScmDraw()
{
	if (drawObj != nullptr)
	{
		drawObj->resetDrawing();
	}
}

void SkyCultureMaker::updateSkyCultureDialog()
{
	if (scmSkyCultureDialog == nullptr || currentSkyCulture == nullptr)
	{
		return;
	}
	scmSkyCultureDialog->setConstellations(currentSkyCulture->getConstellations());
}

void SkyCultureMaker::setSkyCultureDialogInfoLabel(const QString &text)
{
	if (scmSkyCultureDialog != nullptr)
	{
		scmSkyCultureDialog->setInfoLabel(text);
	}
}

void SkyCultureMaker::setSkyCultureDescription(const scm::Description &description)
{
	if (currentSkyCulture != nullptr)
	{
		currentSkyCulture->setDescription(description);
	}
}

QFile SkyCultureMaker::getScmDescriptionFile(const QDir &directory)
{
	return QFile(directory.absoluteFilePath("description.md"));
}

void SkyCultureMaker::setTempArtwork(const scm::ScmConstellationArtwork *artwork)
{
	tempArtwork = artwork;
}

bool SkyCultureMaker::saveSkyCultureDescription(const QDir &directory)
{
	if (currentSkyCulture != nullptr)
	{
		return currentSkyCulture->saveDescriptionAsMarkdown(getScmDescriptionFile(directory));
	}

	return false;
}

void SkyCultureMaker::hideAllDialogs()
{
	setHideOrAbortMakerDialogVisibility(false);
	setSkyCultureDialogVisibility(false);
	setConstellationDialogVisibility(false);
	setSkyCultureExportDialogVisibility(false);
}

void SkyCultureMaker::saveScmDialogVisibilityState()
{
	if (scmSkyCultureDialog != nullptr)
	{
		scmDialogVisibilityMap[scm::DialogID::SkyCultureDialog] = scmSkyCultureDialog->visible();
	}
	if (scmConstellationDialog != nullptr)
	{
		scmDialogVisibilityMap[scm::DialogID::ConstellationDialog] = scmConstellationDialog->visible();
	}
	if (scmSkyCultureExportDialog != nullptr)
	{
		scmDialogVisibilityMap[scm::DialogID::SkyCultureExportDialog] = scmSkyCultureExportDialog->visible();
	}
}

void SkyCultureMaker::restoreScmDialogVisibilityState()
{
	if (scmSkyCultureDialog != nullptr)
	{
		setSkyCultureDialogVisibility(scmDialogVisibilityMap[scm::DialogID::SkyCultureDialog]);
	}
	if (scmConstellationDialog != nullptr)
	{
		setConstellationDialogVisibility(scmDialogVisibilityMap[scm::DialogID::ConstellationDialog]);
	}
	if (scmSkyCultureExportDialog != nullptr)
	{
		setSkyCultureExportDialogVisibility(scmDialogVisibilityMap[scm::DialogID::SkyCultureExportDialog]);
	}
}

bool SkyCultureMaker::isAnyDialogHidden() const
{
	for (bool visible : scmDialogVisibilityMap.values())
	{
		if (visible)
		{
			return true;
		}
	}
	return false;
}

void SkyCultureMaker::resetScmDialogsVisibilityState()
{
	for (auto key : scmDialogVisibilityMap.keys())
	{
		scmDialogVisibilityMap[key] = false;
	}
}

bool SkyCultureMaker::isAnyDialogVisible() const
{
	if (scmSkyCultureDialog != nullptr && scmSkyCultureDialog->visible())
	{
		return true;
	}
	if (scmConstellationDialog != nullptr && scmConstellationDialog->visible())
	{
		return true;
	}
	if (scmSkyCultureExportDialog != nullptr && scmSkyCultureExportDialog->visible())
	{
		return true;
	}
	if (scmHideOrAbortMakerDialog != nullptr && scmHideOrAbortMakerDialog->visible())
	{
		return true;
	}
	if (scmStartDialog != nullptr && scmStartDialog->visible())
	{
		return true;
	}
	return false;
}

void SkyCultureMaker::resetScmDialogs()
{
	resetScmDialogsVisibilityState(); // Reset the visibility state of all dialogs
	scmSkyCultureDialog->resetDialog();
	scmConstellationDialog->resetDialog();
}
