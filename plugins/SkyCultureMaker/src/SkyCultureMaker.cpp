/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Vincent Gerlach
 * Copyright (C) 2025 Luca-Philipp Grumbach
 * Copyright (C) 2025 Fabian Hofer
 * Copyright (C) 2025 Mher Mnatsakanyan
 * Copyright (C) 2025 Richard Hofmann
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SkyCultureMaker.hpp"
#include "StelActionMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelModuleMgr.hpp"
#include "StelProjector.hpp"
#include "gui/ScmConstellationDialog.hpp"
#include "gui/ScmHideOrAbortMakerDialog.hpp"
#include "gui/ScmSkyCultureDialog.hpp"
#include "gui/ScmSkyCultureExportDialog.hpp"
#include "gui/ScmStartDialog.hpp"

#include "ScmDraw.hpp"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>

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
	info.id = "SkyCultureMaker";
	info.displayedName = N_("Sky Culture Maker");
	info.authors = "Vincent Gerlach (RivinHD), Luca-Philipp Grumbach (xLPMG), Fabian Hofer (Integer-Ctrl), Richard Hofmann (ZeyxRew), Mher Mnatsakanyan (MherMnatsakanyan03)";
	info.contact = N_("Contact us using our GitHub usernames, via an Issue or the Discussion tab in the Stellarium repository.");
	info.description = N_("Plugin to draw and export sky cultures in Stellarium.");
	info.version = SKYCULTUREMAKER_PLUGIN_VERSION;
	info.license = SKYCULTUREMAKER_PLUGIN_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
SkyCultureMaker::SkyCultureMaker()
	: isScmEnabled(false)
	, isLineDrawEnabled(false)
	, dialogMap({
		  {scm::DialogID::StartDialog,            new ScmStartDialog(this)           },
		  {scm::DialogID::SkyCultureDialog,       new ScmSkyCultureDialog(this)      },
		  {scm::DialogID::ConstellationDialog,    new ScmConstellationDialog(this)   },
		  {scm::DialogID::SkyCultureExportDialog, new ScmSkyCultureExportDialog(this)},
		  {scm::DialogID::HideOrAbortMakerDialog, new ScmHideOrAbortMakerDialog(this)}
})
{
	qDebug() << "SkyCultureMaker constructed";

	setObjectName("SkyCultureMaker");

	drawObj = new scm::ScmDraw();
	for (const auto &dialogId : dialogMap.keys())
	{
		dialogVisibilityMap[dialogId] = false;
	}

	// Settings
	QSettings *conf = StelApp::getInstance().getSettings();
	conf->beginGroup("SkyCultureMaker");

	initSetting(conf, "fixedLineColor", "1.0,0.5,0.5");
	initSetting(conf, "fixedLineAlpha", 1.0);
	initSetting(conf, "floatingLineColor", "1.0,0.7,0.7");
	initSetting(conf, "floatingLineAlpha", 0.5);
	initSetting(conf, "maxSnapRadiusInPixels", 25);
	initSetting(conf, "mergeLinesOnExport", true);

	conf->endGroup();
}

/*************************************************************************
 Destructor
*************************************************************************/
SkyCultureMaker::~SkyCultureMaker()
{
	delete drawObj;
	// Delete all dialogs
	for (auto dialog : dialogMap)
	{
		delete dialog;
	}

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
		qDebug() << "SkyCultureMaker: Could not find action: " << id;
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
	qDebug() << "SkyCultureMaker: Init called for SkyCultureMaker";

	StelApp &app = StelApp::getInstance();

	addAction(actionIdLine, groupId, N_("Sky Culture Maker"), "enabledScm");

	// Add a SCM toolbar button for starting creation process
	try
	{
		QPixmap iconScmDisabled(":/SkyCultureMaker/bt_SCM_Off.png");
		QPixmap iconScmEnabled(":/SkyCultureMaker/bt_SCM_On.png");
		qDebug() << "SkyCultureMaker: "
			 << (iconScmDisabled.isNull() ? "Failed to load image: bt_SCM_Off.png"
		                                      : "Loaded image: bt_SCM_Off.png");
		qDebug() << "SkyCultureMaker: "
			 << (iconScmEnabled.isNull() ? "Failed to load image: bt_SCM_On.png"
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
		qWarning() << "SkyCultureMaker: Unable create toolbar button for SkyCultureMaker plugin: " << e.what();
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

void SkyCultureMaker::startScm()
{
	if (isScmEnabled)
	{
		qWarning() << "SkyCultureMaker: SCM is already running.";
		return;
	}

	// Check whether in the dialogVisibilityMap any dialog was visible.
	// If this is the case, restore the previous visibility state,
	// because the user has only hidden the dialogs temporarily.
	if (isAnyDialogVisible())
	{
		for (const auto &dialogId : dialogMap.keys())
		{
			if (dialogVisibilityMap[dialogId])
			{
				setDialogVisibility(dialogId, true);
			}
		}
	}
	else // Otherwise, only show the start dialog
	{
		setDialogVisibility(scm::DialogID::StartDialog, true);
	}

	isScmEnabled = true;
	emit eventIsScmEnabled(true);
	setToolbarButtonState(true);
}

void SkyCultureMaker::hideScm()
{
	// save which dialogs were open and hide them
	for (const auto &dialogId : dialogMap.keys())
	{
		dialogVisibilityMap[dialogId] = dialogMap[dialogId]->visible();
		setDialogVisibility(dialogId, false);
	}
	// the HideOrAbortMakerDialog should not be visible next time
	dialogVisibilityMap[scm::DialogID::HideOrAbortMakerDialog] = false;

	isScmEnabled = false;
	emit eventIsScmEnabled(false);
	setToolbarButtonState(false);
}

void SkyCultureMaker::stopScm()
{
	// hide all SCM dialogs
	for (const auto &dialogId : dialogMap.keys())
	{
		setDialogVisibility(dialogId, false);
		// also reset visibility map so that next time SCM is started, all dialogs are closed
		dialogVisibilityMap[dialogId] = false;
	}
	// If the converter dialog is visible, hide it
	if (isValidDialog(scm::DialogID::StartDialog))
	{
		ScmStartDialog *scmStartDialog = static_cast<ScmStartDialog *>(dialogMap[scm::DialogID::StartDialog]);
		scmStartDialog->setConverterDialogVisibility(false);
	}
	// reset dialog content
	resetScmDialogs();
	// Create a new empty sky culture
	setNewSkyCulture();

	isScmEnabled = false;
	emit eventIsScmEnabled(false);
	setToolbarButtonState(false);
}

void SkyCultureMaker::draw(StelCore *core)
{
	if (isLineDrawEnabled && drawObj != nullptr)
	{
		drawObj->drawLines(core);
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
	b ? startScm() : stopScm();
}

void SkyCultureMaker::setDialogVisibility(scm::DialogID dialogId, bool b)
{
	if (!isValidDialog(dialogId))
	{
		qDebug() << "SkyCultureMaker: setDialogVisibility called with invalid dialogId:"
			 << static_cast<int>(dialogId);
		return;
	}

	if (b != dialogMap[dialogId]->visible())
	{
		dialogMap[dialogId]->setVisible(b);
	}
}

void SkyCultureMaker::setConstellationDialogIsDarkConstellation(bool isDarkConstellation)
{
	if (isValidDialog(scm::DialogID::ConstellationDialog))
	{
		static_cast<ScmConstellationDialog *>(dialogMap[scm::DialogID::ConstellationDialog])
			->setIsDarkConstellation(isDarkConstellation);
	}
}

void SkyCultureMaker::setCanCreateConstellations(bool b)
{
	if (isValidDialog(scm::DialogID::SkyCultureDialog))
	{
		static_cast<ScmSkyCultureDialog *>(dialogMap[scm::DialogID::SkyCultureDialog])
			->updateAddConstellationButtons(b);
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
	if (!isValidDialog(scm::DialogID::SkyCultureDialog) || currentSkyCulture == nullptr)
	{
		return;
	}
	static_cast<ScmSkyCultureDialog *>(dialogMap[scm::DialogID::SkyCultureDialog])
		->setConstellations(currentSkyCulture->getConstellations());
}

void SkyCultureMaker::setSkyCultureDescription(const scm::Description &description)
{
	if (currentSkyCulture != nullptr)
	{
		currentSkyCulture->setDescription(description);
	}
}

void SkyCultureMaker::setTempArtwork(const scm::ScmConstellationArtwork *artwork)
{
	tempArtwork = artwork;
}

bool SkyCultureMaker::saveSkyCultureDescription(const QDir &directory)
{
	if (currentSkyCulture != nullptr)
	{
		QFile descriptionFile = QFile(directory.absoluteFilePath("description.md"));
		return currentSkyCulture->saveDescriptionAsMarkdown(descriptionFile);
	}

	return false;
}

bool SkyCultureMaker::isAnyDialogVisible() const
{
	return std::any_of(dialogVisibilityMap.cbegin(), dialogVisibilityMap.cend(),
	                   [](bool visible) { return visible; });
}

void SkyCultureMaker::resetScmDialogs()
{
	// not every dialog has data that needs to be reset
	// therefore, this is done manually here
	if (isValidDialog(scm::DialogID::SkyCultureDialog))
	{
		static_cast<ScmSkyCultureDialog *>(dialogMap[scm::DialogID::SkyCultureDialog])->resetDialog();
	}
	if (isValidDialog(scm::DialogID::ConstellationDialog))
	{
		static_cast<ScmConstellationDialog *>(dialogMap[scm::DialogID::ConstellationDialog])->resetDialog();
	}
}

void SkyCultureMaker::initSetting(QSettings *conf, const QString key, const QVariant &defaultValue)
{
	if (conf == nullptr)
	{
		qWarning() << "SkyCultureMaker: QSettings pointer is null.";
		return;
	}

	if (!conf->contains(key))
	{
		conf->setValue(key, defaultValue);
	}
}

void SkyCultureMaker::showUserInfoMessage(QWidget *parent, const QString &dialogName, const QString &message)
{
	const QString level = q_("INFO");
	const QString title = dialogName.isEmpty() ? level : dialogName + ": " + level;
	QMessageBox::information(parent, title, message);
}

void SkyCultureMaker::showUserWarningMessage(QWidget *parent, const QString &dialogName, const QString &message)
{
	const QString level = q_("WARNING");
	const QString title = dialogName.isEmpty() ? level : dialogName + ": " + level;
	QMessageBox::warning(parent, title, message);
}

void SkyCultureMaker::showUserErrorMessage(QWidget *parent, const QString &dialogName, const QString &message)
{
	const QString level = q_("ERROR");
	const QString title = dialogName.isEmpty() ? level : dialogName + ": " + level;
	QMessageBox::critical(parent, title, message);
}

bool SkyCultureMaker::isValidDialog(scm::DialogID dialogId) const
{
	return dialogMap.contains(dialogId) && dialogMap[dialogId] != nullptr;
}

void SkyCultureMaker::loadDialogFromConstellation(scm::ScmConstellation *constellation)
{
	if (isValidDialog(scm::DialogID::ConstellationDialog))
	{
		static_cast<ScmConstellationDialog *>(dialogMap[scm::DialogID::ConstellationDialog])
			->loadFromConstellation(constellation);
	}
}
