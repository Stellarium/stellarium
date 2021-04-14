/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 *
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This class used to be called TelescopeMgr before the split.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelUtils.hpp"
#include "TelescopeControl.hpp"
#include "TelescopeClient.hpp"
#include "gui/TelescopeDialog.hpp"
#include "gui/SlewDialog.hpp"
#include "common/LogFile.hpp"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelStyle.hpp"
#include "StelTextureMgr.hpp"
#include "StelActionMgr.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMapIterator>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QDir>

#include <QDebug>
#include <stdexcept>

#define DEFAULT_RTS2_REFRESH    500000

////////////////////////////////////////////////////////////////////////////////
//
StelModule* TelescopeControlStelPluginInterface::getStelModule() const
{
	return new TelescopeControl();
}

StelPluginInfo TelescopeControlStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(TelescopeControl);

	StelPluginInfo info;
	info.id = "TelescopeControl";
	info.displayedName = N_("Telescope Control");
	info.authors = "Bogdan Marinov, Johannes Gajdosik, Alessandro Siniscalchi, Gion Kunz, Petr Kubánek";
	info.contact = STELLARIUM_URL;
	info.description = N_("This plug-in allows Stellarium to send \"slew\" commands to a telescope on a computerized mount (a \"GoTo telescope\").");
	info.version = TELESCOPE_CONTROL_PLUGIN_VERSION;
	info.license = TELESCOPE_CONTROL_PLUGIN_LICENSE;
	return info;
}

////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
TelescopeControl::TelescopeControl()
	: toolbarButton(Q_NULLPTR)
	, useTelescopeServerLogs(false)
	, useServerExecutables(false)
	, telescopeDialog(Q_NULLPTR)
	, slewDialog(Q_NULLPTR)
	, actionGroupId("PluginTelescopeControl")
	, moveToSelectedActionId("actionMove_Telescope_To_Selection_%1")
	, syncActionId("actionSync_Telescope_To_Selection_%1")
	, abortSlewActionId("actionAbortSlew_Telescope_Slew_%1")
	, moveToCenterActionId("actionSlew_Telescope_To_Direction_%1")
{
	setObjectName("TelescopeControl");

	connectionTypeNames.insert(ConnectionVirtual, "virtual");
	connectionTypeNames.insert(ConnectionInternal, "internal");
	connectionTypeNames.insert(ConnectionLocal, "local");
	connectionTypeNames.insert(ConnectionRemote, "remote");
	connectionTypeNames.insert(ConnectionRTS2, "RTS2");
	connectionTypeNames.insert(ConnectionINDI, "INDI");
	connectionTypeNames.insert(ConnectionASCOM, "ASCOM");
}

TelescopeControl::~TelescopeControl()
{
	delete slewDialog; slewDialog = Q_NULLPTR;
	delete telescopeDialog; telescopeDialog = Q_NULLPTR;
}


////////////////////////////////////////////////////////////////////////////////
// Methods inherited from the StelModule class
// init(), update(), draw(),  getCallOrder()
void TelescopeControl::init()
{
	//TODO: I think I've overdone the try/catch...
	try
	{
		//Main configuration
		loadConfiguration();
		//Make sure that such a section is created, if it doesn't exist
		saveConfiguration();
		
		//Make sure that the module directory exists
		QString moduleDirectoryPath = StelFileMgr::getUserDir() + "/modules/TelescopeControl";
		if(!StelFileMgr::exists(moduleDirectoryPath))
			StelFileMgr::mkDir(moduleDirectoryPath);
		
		//Load the device models
		loadDeviceModels();
		if(deviceModels.isEmpty())
		{
			qWarning() << "[TelescopeControl] No device model descriptions have been loaded. Stellarium will not be able to control a telescope on its own, but it is still possible to do it through an external application or to connect to a remote host.";
		}
		
		//Unload Stellarium's internal telescope control module
		//(not necessary since revision 6308; remains as an example)
		//StelApp::getInstance().getModuleMgr().unloadModule("TelescopeMgr", false);
		/*If the alsoDelete parameter is set to true, Stellarium crashes with a
			segmentation fault when an object is selected. TODO: Find out why.
			unloadModule() didn't work prior to revision 5058: the module unloaded
			normally, but Stellarium crashed later with a segmentation fault,
			because LandscapeMgr::getCallOrder() depended on the module's
			existence to return a value.*/
		
		//Load and start all telescope clients
		loadTelescopes();
		
		//Load OpenGL textures
		reticleTexture = StelApp::getInstance().getTextureManager().createTexture(":/telescopeControl/telescope_reticle.png");
		selectionTexture = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/pointeur2.png");

		//Create telescope key bindings
		/* StelAction-s with these key bindings existed in Stellarium prior to
			revision 6311. Any future backports should account for that. */
		QString section = N_("Telescope Control");
		for (int i = MIN_SLOT_NUMBER; i <= MAX_SLOT_NUMBER; i++)
		{
			// "Slew to object" commands
			QString name = moveToSelectedActionId.arg(i);
			QString shortcut = QString("Ctrl+%1").arg(i);
			QString text;
			text = q_("Move telescope #%1 to selected object").arg(i);
			addAction(name, section, text, this, [=](){slewTelescopeToSelectedObject(i);}, shortcut);

			// "Slew to the center of the screen" commands
			name = moveToCenterActionId.arg(i);
			shortcut = QString("Alt+%1").arg(i);
			text = q_("Move telescope #%1 to the point currently in the center of the screen").arg(i);
			addAction(name, section, text, this, [=](){slewTelescopeToViewDirection(i);}, shortcut);

			// "Sync to object" commands
			name = syncActionId.arg(i);
			shortcut = QString("Ctrl+Shift+%1").arg(i);
			text = q_("Sync telescope #%1 position to selected object").arg(i);
			addAction(name, section, text, this, [=](){syncTelescopeWithSelectedObject(i);}, shortcut);

			// "Abort Slew" commands
			name = abortSlewActionId.arg(i);
			shortcut = QString("Ctrl+Shift+Alt+%1").arg(i);
			text = q_("Abort last slew command of telescope #%1").arg(i);
			addAction(name, section, text, this, [=](){abortTelescopeSlew(i);}, shortcut);
		}
		connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(translateActionDescriptions()));

		//Create and initialize dialog windows
		telescopeDialog = new TelescopeDialog();
		slewDialog = new SlewDialog();

		addAction("actionShow_Slew_Window", N_("Telescope Control"), N_("Move a telescope to a given set of coordinates"), slewDialog, "visible", "Ctrl+0");

		//Create toolbar button
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui!=Q_NULLPTR)
		{
			toolbarButton =	new StelButton(Q_NULLPTR,
						       QPixmap(":/telescopeControl/button_Slew_Dialog_on.png"),
						       QPixmap(":/telescopeControl/button_Slew_Dialog_off.png"),
						       QPixmap(":/graphicGui/miscGlow32x32.png"),
						       "actionShow_Slew_Window");
			gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
		}
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "[TelescopeControl] init() error: " << e.what();
		return;
	}
	
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
}

void TelescopeControl::translateActionDescriptions()
{
	StelActionMgr* actionMgr = StelApp::getInstance().getStelActionManager();
	
	for (int i = MIN_SLOT_NUMBER; i <= MAX_SLOT_NUMBER; i++)
	{
		QString name;
		QString description;

		name = moveToSelectedActionId.arg(i);
		description = q_("Move telescope #%1 to selected object").arg(i);
		actionMgr->findAction(name)->setText(description);

		name = syncActionId.arg(i);
		description = q_("Abort last slew command of telescope #%1").arg(i);
		actionMgr->findAction(name)->setText(description);
		
		name = moveToCenterActionId.arg(i);
		description = q_("Move telescope #%1 to the point currently in the center of the screen").arg(i);
		actionMgr->findAction(name)->setText(description);

		name = abortSlewActionId.arg(i);
		description = q_("Abort last slew command of telescope #%1").arg(i);
		actionMgr->findAction(name)->setText(description);
	}
}


void TelescopeControl::deinit()
{
	//Destroy all clients first in order to avoid displaying a TCP error
	deleteAllTelescopes();

	for (auto iterator = telescopeServerProcess.constBegin(); iterator != telescopeServerProcess.constEnd();
		 ++iterator)
	{
		int slotNumber = iterator.key();
#ifdef Q_OS_WIN
		telescopeServerProcess[slotNumber]->close();
#else
		telescopeServerProcess[slotNumber]->terminate();
#endif
		telescopeServerProcess[slotNumber]->waitForFinished();
		delete telescopeServerProcess[slotNumber];
		qDebug() << "[TelescopeControl] deinit(): Server process at slot" << slotNumber << "terminated successfully.";
	}

	//TODO: Decide if it should be saved on change
	//Save the configuration on exit
	saveConfiguration();
}

void TelescopeControl::update(double deltaTime)
{
	labelFader.update(static_cast<int>(deltaTime*1000));
	reticleFader.update(static_cast<int>(deltaTime*1000));
	circleFader.update(static_cast<int>(deltaTime*1000));
	// communicate with the telescopes:
	communicate();
}

void TelescopeControl::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(labelFont);
	reticleTexture->bind();
	for (const auto& telescope : telescopeClients)
	{
		if (telescope->isConnected() && telescope->hasKnownPosition())
		{
			Vec3d XY;
			if (prj->projectCheck(telescope->getJ2000EquatorialPos(core), XY))
			{
				//Telescope circles appear synchronously with markers
				if (circleFader.getInterstate() >= 0)
				{
					sPainter.setColor(circleColor[0], circleColor[1], circleColor[2], circleFader.getInterstate());
					for (auto circle : telescope->getOculars())
					{
						sPainter.drawCircle(XY[0], XY[1], 0.5 * prj->getPixelPerRadAtCenter() * (M_PI/180) * (circle));
					}
				}
				if (reticleFader.getInterstate() >= 0)
				{
					sPainter.setBlending(true, GL_SRC_ALPHA, GL_ONE);
					sPainter.setColor(reticleColor[0], reticleColor[1], reticleColor[2], reticleFader.getInterstate());
					sPainter.drawSprite2dMode(XY[0],XY[1],15.f);
				}
				if (labelFader.getInterstate() >= 0)
				{
					sPainter.setColor(labelColor[0], labelColor[1], labelColor[2], labelFader.getInterstate());
					//TODO: Different position of the label if circles are shown?
					//TODO: Remove magic number (text spacing)
					sPainter.drawText(XY[0], XY[1], telescope->getNameI18n(), 0, 6 + 10, -4, false);
					//Same position as the other objects: doesn't work, telescope label overlaps object label
					//sPainter.drawText(XY[0], XY[1], scope->getNameI18n(), 0, 10, 10, false);
					reticleTexture->bind();
				}
			}
		}
	}

	if(GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
		drawPointer(prj, core, sPainter);
}

double TelescopeControl::getCallOrder(StelModuleActionName actionName) const
{
	//TODO: Remove magic number (call order offset)
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName) + 2.;
	return 0.;
}


////////////////////////////////////////////////////////////////////////////////
// Methods inherited from the StelObjectModule class
//
QList<StelObjectP> TelescopeControl::searchAround(const Vec3d& vv, double limitFov, const StelCore* core) const
{
	QList<StelObjectP> result;
	if (!getFlagTelescopeReticles())
		return result;
	Vec3d v(vv);
	v.normalize();
	double cosLimFov = cos(limitFov * M_PI/180.);
	for (const auto& telescope : telescopeClients)
	{
		if (telescope->getJ2000EquatorialPos(core).dot(v) >= cosLimFov)
		{
			result.append(qSharedPointerCast<StelObject>(telescope));
		}
	}
	return result;
}

StelObjectP TelescopeControl::searchByNameI18n(const QString &nameI18n) const
{
	for (const auto& telescope : telescopeClients)
	{
		if (telescope->getNameI18n() == nameI18n)
			return qSharedPointerCast<StelObject>(telescope);
	}
	return Q_NULLPTR;
}

StelObjectP TelescopeControl::searchByName(const QString &name) const
{
	for (const auto& telescope : telescopeClients)
	{
		if (telescope->getEnglishName() == name)
			return qSharedPointerCast<StelObject>(telescope);
	}
	return Q_NULLPTR;
}

QString TelescopeControl::getStelObjectType() const
{
	return TelescopeClient::TELESCOPECLIENT_TYPE;
}

bool TelescopeControl::configureGui(bool show)
{
	if(show)
	{
		telescopeDialog->setVisible(true);
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////
// Misc methods (from TelescopeMgr; TODO: Better categorization)
void TelescopeControl::setFontSize(int fontSize)
{
	labelFont.setPixelSize(fontSize);
}

void TelescopeControl::slewTelescopeToSelectedObject(const int idx)
{
	// Find out the coordinates of the target
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	StelObjectP selectObject = omgr->getSelectedObject().at(0);
	if (!selectObject)  // should never happen
		return;

	Vec3d objectPosition = selectObject->getJ2000EquatorialPos(StelApp::getInstance().getCore());

	telescopeGoto(idx, objectPosition, selectObject);
}

void TelescopeControl::slewTelescopeToViewDirection(const int idx)
{
	// Find out the coordinates of the target
	Vec3d centerPosition = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();

	telescopeGoto(idx, centerPosition);
}

void TelescopeControl::syncTelescopeWithSelectedObject(const int idx)
{
	// Find out the coordinates of the target
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	StelObjectP selectObject = omgr->getSelectedObject().at(0);
	if (!selectObject)  // should never happen
		return;

	Vec3d objectPosition = selectObject->getJ2000EquatorialPos(StelApp::getInstance().getCore());

	telescopeSync(idx, objectPosition, selectObject);
}

void TelescopeControl::abortTelescopeSlew(const int idx) {
	if (telescopeClients.contains(idx))
		telescopeClients.value(idx)->telescopeAbortSlew();
}

void TelescopeControl::drawPointer(const StelProjectorP& prj, const StelCore* core, StelPainter& sPainter)
{
#ifndef COMPATIBILITY_001002
	//Leaves this whole routine empty if this is the backport version.
	//Otherwise, there will be two concentric selection markers drawn around the telescope pointer.
	//In 0.10.3, the plug-in unloads the module that draws the surplus marker.
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Telescope");
	if (!newSelected.empty())
	{
		const StelObjectP obj = newSelected[0];
		Vec3d pos = obj->getJ2000EquatorialPos(core);
		Vec3d screenpos;
		// Compute 2D pos and return if outside screen
		if (!prj->project(pos, screenpos))
			return;

		const Vec3f& c(obj->getInfoColor());
		sPainter.setColor(c[0], c[1], c[2]);
		selectionTexture->bind();
		sPainter.setBlending(true);
		sPainter.drawSprite2dMode(screenpos[0], screenpos[1], 25., StelApp::getInstance().getTotalRunTime() * 40.);
	}
#else
	Q_UNUSED(prj) Q_UNUSED(core) Q_UNUSED(sPainter)
#endif //COMPATIBILITY_001002
}

void TelescopeControl::telescopeGoto(int slotNumber, const Vec3d &j2000Pos, StelObjectP selectObject)
{
	//TODO: See the original code. I think that something is wrong here...
	if(telescopeClients.contains(slotNumber))
		telescopeClients.value(slotNumber)->telescopeGoto(j2000Pos, selectObject);
}

void TelescopeControl::telescopeSync(int slotNumber, const Vec3d &j2000Pos, StelObjectP selectObject)
{
	//TODO: See the original code. I think that something is wrong here...
	if(telescopeClients.contains(slotNumber))
		telescopeClients.value(slotNumber)->telescopeSync(j2000Pos, selectObject);
}

QSharedPointer<TelescopeClient> TelescopeControl::telescopeClient(int index) const
{
	if(!telescopeClients.contains(index))
		return QSharedPointer<TelescopeClient>();

	return telescopeClients.value(index);
}

void TelescopeControl::communicate(void)
{
	if (!telescopeClients.empty())
	{
		for (auto telescope = telescopeClients.constBegin(); telescope != telescopeClients.constEnd(); ++telescope)
		{
			logAtSlot(telescope.key());//If there's no log, it will be ignored
			if(telescope.value()->prepareCommunication())
			{
				telescope.value()->performCommunication();
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Methods for managing telescope client objects
//
void TelescopeControl::deleteAllTelescopes()
{
	//TODO: I really hope that this won't cause a memory leak...
	//for (auto* telescope : telescopeClients)
	//	delete telescope;
	telescopeClients.clear();
}

bool TelescopeControl::isExistingClientAtSlot(int slotNumber)
{
	return (telescopeClients.contains(slotNumber));
}

bool TelescopeControl::isConnectedClientAtSlot(int slotNumber)
{
	if(telescopeClients.contains(slotNumber))
		return telescopeClients.value(slotNumber)->isConnected();
	else
		return false;
}


////////////////////////////////////////////////////////////////////////////////
// Methods for managing telescope server process objects
//

void TelescopeControl::loadTelescopeServerExecutables(void)
{
	telescopeServers = QStringList();

	//TODO: I don't like how this turned out
	if(serverExecutablesDirectoryPath.isEmpty())
		return;

	//As StelFileMgr is quite limited, use Qt's handy methods (I love Qt!)
	//Get all executable files with names beginning with "TelescopeServer" in this directory
	QDir serverDirectory(serverExecutablesDirectoryPath);
	if(!serverDirectory.exists())
	{
		qWarning() << "[TelescopeControl] No telescope server directory has been found.";
		return;
	}
	QList<QFileInfo> telescopeServerExecutables = serverDirectory.entryInfoList(QStringList("TelescopeServer*"), (QDir::Files|QDir::Executable|QDir::CaseSensitive), QDir::Name);
	if(!telescopeServerExecutables.isEmpty())
	{
		for (auto telescopeServerExecutable : telescopeServerExecutables)
			telescopeServers.append(telescopeServerExecutable.baseName());//This strips the ".exe" suffix on Windows
	}
	else
	{
		qWarning() << "[TelescopeControl] No telescope server executables found in" << serverExecutablesDirectoryPath;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Methods for reading from and writing to the configuration file

void TelescopeControl::loadConfiguration()
{
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	settings->beginGroup("TelescopeControl");

	//Load display flags
	setFlagTelescopeReticles(settings->value("flag_telescope_reticles", true).toBool());
	setFlagTelescopeLabels(settings->value("flag_telescope_labels", true).toBool());
	setFlagTelescopeCircles(settings->value("flag_telescope_circles", true).toBool());

	//Load font size
#ifdef Q_OS_WIN
	setFontSize(settings->value("telescope_labels_font_size", 13).toInt()); //Windows Qt bug workaround
#else
	setFontSize(settings->value("telescope_labels_font_size", 12).toInt());
#endif

	//Load colours
	setReticleColor(Vec3f(settings->value("color_telescope_reticles", "0.6,0.4,0").toString()));
	setLabelColor(Vec3f(settings->value("color_telescope_labels", "0.6,0.4,0").toString()));
	setCircleColor(Vec3f(settings->value("color_telescope_circles", "0.6,0.4,0").toString()));

	//Load server executables flag and directory
	useServerExecutables = settings->value("flag_use_server_executables", false).toBool();
	serverExecutablesDirectoryPath = settings->value("server_executables_path").toString();

	//If no directory is specified in the configuration file, try to find the default one
	if(serverExecutablesDirectoryPath.isEmpty() || !QDir(serverExecutablesDirectoryPath).exists())
	{
		//Find out if the default server directory exists
		QString serverDirectoryPath = StelFileMgr::findFile("servers", StelFileMgr::Directory);
		if (serverDirectoryPath.isEmpty())
		{
			//qDebug() << "TelescopeControl: No telescope servers directory detected.";
			useServerExecutables = false;
			serverDirectoryPath = StelFileMgr::getUserDir() + "/servers";
		}
		else
		{
			serverExecutablesDirectoryPath = serverDirectoryPath;
		}
	}

	//Load logging flag
	useTelescopeServerLogs = settings->value("flag_enable_telescope_logs", false).toBool();

	settings->endGroup();
}

void TelescopeControl::saveConfiguration()
{
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);

	settings->beginGroup("TelescopeControl");

	//Save display flags
	settings->setValue("flag_telescope_reticles", getFlagTelescopeReticles());
	settings->setValue("flag_telescope_labels", getFlagTelescopeLabels());
	settings->setValue("flag_telescope_circles", getFlagTelescopeCircles());

	//Save colours
	settings->setValue("color_telescope_reticles", getReticleColor().toHtmlColor());
	settings->setValue("color_telescope_labels", getLabelColor().toHtmlColor());
	settings->setValue("color_telescope_circles", getCircleColor().toHtmlColor());

	//Save telescope server executables flag and directory
	settings->setValue("flag_use_server_executables", useServerExecutables);
	if(useServerExecutables)
	{
		settings->setValue("server_executables_path", serverExecutablesDirectoryPath);
	}
	else
	{
		settings->remove("server_executables_path");
	}

	//Save logging flag
	settings->setValue("flag_enable_telescope_logs", useTelescopeServerLogs);

	settings->endGroup();

	// Remove outdated config items
	if (settings->contains("TelescopeControl/night_color_telescope_reticles"))
		settings->remove("TelescopeControl/night_color_telescope_reticles");

	if (settings->contains("TelescopeControl/night_color_telescope_labels"))
		settings->remove("TelescopeControl/night_color_telescope_labels");

	if (settings->contains("TelescopeControl/night_color_telescope_circles"))
		settings->remove("TelescopeControl/night_color_telescope_circles");
}

void TelescopeControl::saveTelescopes()
{
	//Open/create the JSON file
	QString telescopesJsonPath = StelFileMgr::findFile("modules/TelescopeControl", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/telescopes.json";
	if (telescopesJsonPath.isEmpty())
	{
		qWarning() << "[TelescopeControl] Error saving telescopes";
		return;
	}
	QFile telescopesJsonFile(telescopesJsonPath);
	if(!telescopesJsonFile.open(QFile::WriteOnly|QFile::Text))
	{
		qWarning() << "[TelescopeControl] Telescopes can not be saved. A file can not be open for writing:" << QDir::toNativeSeparators(telescopesJsonPath);
		return;
	}

	//Add the version:
	telescopeDescriptions.insert("version", QString(TELESCOPE_CONTROL_PLUGIN_VERSION));

	//Convert the tree to JSON
	StelJsonParser::write(telescopeDescriptions, &telescopesJsonFile);
	telescopesJsonFile.flush();//Is this necessary?
	telescopesJsonFile.close();
}

void TelescopeControl::loadTelescopes()
{
	QVariantMap result;

	QString telescopesJsonPath = StelFileMgr::findFile("modules/TelescopeControl", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/telescopes.json";
	if (telescopesJsonPath.isEmpty())
	{
		qWarning() << "[TelescopeControl] Error loading telescopes";
		return;
	}
	if(!QFileInfo(telescopesJsonPath).exists())
	{
		qWarning() << "[TelescopeControl] loadTelescopes(): No telescopes loaded. File is missing:" << QDir::toNativeSeparators(telescopesJsonPath);
		telescopeDescriptions = result;
		return;
	}

	QFile telescopesJsonFile(telescopesJsonPath);

	QVariantMap map;

	if(!telescopesJsonFile.open(QFile::ReadOnly))
	{
		qWarning() << "[TelescopeControl] No telescopes loaded. Can't open for reading" << QDir::toNativeSeparators(telescopesJsonPath);
		telescopeDescriptions = result;
		return;
	}
	else
	{
		map = StelJsonParser::parse(&telescopesJsonFile).toMap();
		telescopesJsonFile.close();
	}

	//File contains any telescopes?
	if(map.isEmpty())
	{
		telescopeDescriptions = result;
		return;
	}

	QString version = map.value("version", "0.0.0").toString();
	if(StelUtils::compareVersions(version, QString(TELESCOPE_CONTROL_PLUGIN_VERSION))!=0)
	{
		QString newName = telescopesJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
		if(telescopesJsonFile.rename(newName))
		{
			qWarning() << "[TelescopeControl] The existing version of telescopes.json is obsolete. Backing it up as" << QDir::toNativeSeparators(newName);
			qWarning() << "[TelescopeControl] A blank telescopes.json file will have to be created.";
			telescopeDescriptions = result;
			return;
		}
		else
		{
			qWarning() << "[TelescopeControl] The existing version of telescopes.json is obsolete. Unable to rename.";
			telescopeDescriptions = result;
			return;
		}
	}
	map.remove("version"); // Otherwise it will try to read it as a telescope

	//Make sure that there are no telescope clients yet
	deleteAllTelescopes();

	//Read telescopes, if any
	int telescopesCount = 0;
	QMapIterator<QString, QVariant> node(map);
	bool ok;
	while(node.hasNext())
	{
		node.next();
		QString key = node.key();

		//If this is not a valid slot number, remove the node
		int slot = key.toInt(&ok);
		if(!ok || !isValidSlotNumber(slot))
		{
			qDebug() << "[TelescopeControl] loadTelescopes(): Deleted node unrecogised as slot:" << key;
			map.remove(key);
			continue;
		}

		QVariantMap telescope = node.value().toMap();

		//Essential parameters: Name, connection type, equinox
		//Validation: Name
		QString name = telescope.value("name").toString();
		if(name.isEmpty())
		{
			qDebug() << "[TelescopeControl] Unable to load telescope: No name specified at slot" << key;
			map.remove(key);
			continue;
		}

		//Validation: Connection
		QString connection = telescope.value("connection").toString();
		if(connection.isEmpty() || !connectionTypeNames.values().contains(connection))
		{
			qDebug() << "[TelescopeControl] Unable to load telescope: No valid connection type at slot" << key;
			map.remove(key);
			continue;
		}
		ConnectionType connectionType = connectionTypeNames.key(connection);

		QString equinox = telescope.value("equinox", "J2000").toString();
		if (equinox != "J2000" && equinox != "JNow")
		{
			qDebug() << "[TelescopeControl] Unable to load telescope: Invalid equinox value at slot" << key;
			map.remove(key);
			continue;
		}

		QString hostName("localhost");
		int portTCP = 0;
		int delay = 0;
		QString deviceModelName;
		QString portSerial;
		QString rts2Url("localhost");
		QString rts2Username("");
		QString rts2Password("");
		int rts2Refresh = DEFAULT_RTS2_REFRESH;
		QString ascomDeviceId("");
		bool ascomUseDeviceEqCoordType = true;

		if (connectionType == ConnectionInternal)
		{
			//Serial port and device model
			deviceModelName = telescope.value("device_model").toString();
			portSerial = telescope.value("serial_port").toString();

			if(deviceModelName.isEmpty())
			{
				qDebug() << "[TelescopeControl] Unable to load telescope: No device model specified at slot" << key;
				map.remove(key);
				continue;
			}

			//Do we have this server?
			if(!deviceModels.contains(deviceModelName))
			{
				qWarning() << "[TelescopeControl] Unable to load telescope at slot" << slot
						   << "because the specified device model is missing:" << deviceModelName;
				map.remove(key);
				continue;
			}

			if(portSerial.isEmpty())
			{
				qDebug() << "[TelescopeControl] Unable to load telescope: No valid serial port specified at slot" << key;
				map.remove(key);
				continue;
			}
		}

		if (connectionType == ConnectionRemote)
		{
			//Validation: Host name
			hostName = telescope.value("host_name").toString();
			if(hostName.isEmpty())
			{
				qDebug() << "[TelescopeControl] loadTelescopes(): No host name at slot" << key;
				map.remove(key);
				continue;
			}
		}

		if (connectionType == ConnectionINDI)
		{
			portTCP = telescope.value("tcp_port").toInt();
			hostName = telescope.value("host_name").toString();
			deviceModelName = telescope.value("device_model").toString();
		}

		if (connectionType == ConnectionASCOM)
		{
			ascomDeviceId = telescope.value("ascom_device_id").toString();
			ascomUseDeviceEqCoordType = telescope.value("ascom_use_device_eq_coord_type").toBool();
		}

		if (connectionType == ConnectionRTS2)
		{
			//Validation: Host name
			rts2Url = telescope.value("url").toString();
			if(rts2Url.isEmpty())
			{
				qDebug() << "[TelescopeControl] loadTelescopes(): No URL at slot" << key;
				map.remove(key);
				continue;
			}
			rts2Username = telescope.value("username").toString();
			rts2Password = telescope.value("password").toString();
			rts2Refresh = telescope.value("refresh", DEFAULT_RTS2_REFRESH).toInt();
		}

		if (connectionType != ConnectionVirtual)
		{
			if (connectionType != ConnectionRTS2)
			{
				//Validation: TCP port
				portTCP = telescope.value("tcp_port").toInt();
				if(!telescope.contains("tcp_port") || !isValidPort(portTCP))
				{
					qDebug() << "[TelescopeControl] Unable to load telescope: No valid TCP port at slot" << key;
					map.remove(key);
					continue;
				}
			}

			//Validation: Delay
			delay = telescope.value("delay", 0).toInt();
			if(!isValidDelay(delay))
			{
				qDebug() << "[TelescopeControl] Unable to load telescope: No valid delay at slot" << key;
				map.remove(key);
				continue;
			}
		}

		//Connect at startup
		bool connectAtStartup = telescope.value("connect_at_startup", false).toBool();

		//Validation: FOV circles
		QVariantList parsedJsonCircles = telescope.value("circles").toList();
		QList<double> internalCircles;
		for(int i = 0; i< parsedJsonCircles.size(); i++)
		{
			if(i >= MAX_CIRCLE_COUNT)
				break;
			internalCircles.append(parsedJsonCircles.value(i, -1.0).toDouble());
		}
		if(internalCircles.isEmpty())
		{
			//If the list is empty or invalid, make sure it's no longer in the file
			telescope.remove("circles");
			map.insert(key, telescope);
		}
		else
		{
			//Replace the existing list with the validated one
			QVariantList newJsonCircles;
			for(int i = 0; i < internalCircles.size(); i++)
				newJsonCircles.append(internalCircles.at(i));
			telescope.insert("circles", newJsonCircles);
			map.insert(key, telescope);
		}

		//Initialize a telescope client for this slot
		//TODO: Improve the flow of control
		if(connectAtStartup)
		{
			if (connectionType == ConnectionInternal)
			{
				//Use a sever if necessary
				if(deviceModels[deviceModelName].useExecutable)
				{
					if(startClientAtSlot(slot, connectionType, name, equinox, hostName, portTCP, delay, internalCircles))
					{
						if(!startServerAtSlot(slot, deviceModelName, portTCP, portSerial))
						{
							stopClientAtSlot(slot);
							qDebug() << "[TelescopeControl] Unable to launch a telescope server at slot" << slot;
						}
					}
					else
					{
						qDebug() << "[TelescopeControl] Unable to create a telescope client at slot" << slot;
						//Unnecessary due to if-else construction;
						//also, causes bug #608533
						//continue;
					}
				}
				else
				{
					addLogAtSlot(slot);
					logAtSlot(slot);
					if(!startClientAtSlot(slot, connectionType, name, equinox, QString(), 0, delay, internalCircles, deviceModelName, portSerial))
					{
						qDebug() << "[TelescopeControl] Unable to create a telescope client at slot" << slot;
						//Unnecessary due to if-else construction;
						//also, causes bug #608533
						//continue;
					}
				}
			}
			else
			{
				if(!startClientAtSlot(slot, connectionType, name, equinox, hostName, portTCP, delay, internalCircles, deviceModelName, portSerial, rts2Url, rts2Username, rts2Password, rts2Refresh, ascomDeviceId, ascomUseDeviceEqCoordType))
				{
					qDebug() << "[TelescopeControl] Unable to create a telescope client at slot" << slot;
					//Unnecessary due to if-else construction;
					//also, causes bug #608533
					//continue;
				}
			}
		}

		//If this line is reached, the telescope at this slot has been loaded successfully
		telescopesCount++;
	}

	if(telescopesCount > 0)
	{
		result = map;
		qDebug() << "[TelescopeControl] Loaded successfully" << telescopesCount
				 << "telescopes.";
	}

	telescopeDescriptions = result;
}

bool TelescopeControl::addTelescopeAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox, QString host, int portTCP, int delay, bool connectAtStartup, QList<double> circles, QString deviceModelName, QString portSerial, QString rts2Url, QString rts2Username, QString rts2Password, int rts2Refresh, QString ascomDeviceId, bool ascomUseDeviceEqCoordType)
{
	//Validation
	if(!isValidSlotNumber(slot) || name.isEmpty() || equinox.isEmpty() || connectionType <= ConnectionNA || connectionType >= ConnectionCount)
		return false;

	//Create a new map node and fill it with parameters
	QVariantMap telescope;
	telescope.insert("name", name);
	telescope.insert("connection", connectionTypeNames.value(connectionType));
	telescope.insert("equinox", equinox);//TODO: Validation!

	if (connectionType == ConnectionINDI)
	{
		telescope.insert("host_name", host);
		telescope.insert("tcp_port", portTCP);
		telescope.insert("device_model", deviceModelName);
	}

	if (connectionType == ConnectionASCOM)
	{
		telescope.insert("ascom_device_id", ascomDeviceId);
		telescope.insert("ascom_use_device_eq_coord_type", ascomUseDeviceEqCoordType);
	}

	if (connectionType == ConnectionRemote || connectionType == ConnectionLocal)
	{
		//TODO: Add more validation!
		if (!host.isEmpty())
			telescope.insert("host_name", host);
		if (isValidPort(portTCP))
			telescope.insert("tcp_port", portTCP);
		if (deviceModels.contains(deviceModelName))
			telescope.insert("device_model", deviceModelName);
		if (!portSerial.isEmpty())
			telescope.insert("serial_port", portSerial);
	}

	if(connectionType == ConnectionRTS2)
	{
		if (rts2Url.isEmpty())
			return false;
		telescope.insert("url", rts2Url);
		telescope.insert("username", rts2Username);
		telescope.insert("password", rts2Password);
		telescope.insert("refresh", rts2Refresh);
	}

	if(connectionType == ConnectionInternal)
	{
		if (!deviceModels.contains(deviceModelName))
			return false;
		telescope.insert("device_model", deviceModelName);

		if (portSerial.isEmpty())
			return false;
		telescope.insert("serial_port", portSerial);
	}

	if (connectionType != ConnectionVirtual)
	{
		if (connectionType != ConnectionRTS2)
		{
			if (!isValidPort(portTCP))
				return false;
			telescope.insert("tcp_port", portTCP);
		}

		if (!isValidDelay(delay))
			return false;
		telescope.insert("delay", delay);
	}
	telescope.insert("connect_at_startup", connectAtStartup);

	if(!circles.isEmpty())
	{
		QVariantList circleList;
		for(int i = 0; i < circles.size(); i++)
			circleList.append(circles[i]);
		telescope.insert("circles", circleList);
	}

	telescopeDescriptions.insert(QString::number(slot), telescope);

	return true;
}

bool TelescopeControl::getTelescopeAtSlot(int slot, ConnectionType& connectionType, QString& name, QString& equinox, QString& host, int& portTCP, int& delay, bool& connectAtStartup, QList<double>& circles, QString& deviceModelName, QString& portSerial, QString& rts2Url, QString& rts2Username, QString& rts2Password, int& rts2Refresh, QString& ascomDeviceId, bool& ascomUseDeviceEqCoordType)
{
	//Validation
	if(!isValidSlotNumber(slot))
		return false;

	//Read the node at that slot
	QVariantMap telescope = telescopeDescriptions.value(QString::number(slot)).toMap();
	if(telescope.isEmpty())
	{
		telescopeDescriptions.remove(QString::number(slot));
		return false;
	}

	//Read the parameters
	name = telescope.value("name").toString();
	equinox = telescope.value("equinox", "J2000").toString();
	host = telescope.value("host_name").toString();
	portTCP = telescope.value("tcp_port").toInt();
	delay = telescope.value("delay", DEFAULT_DELAY).toInt();
	connectAtStartup = telescope.value("connect_at_startup", false).toBool();

	QVariantList circleList = telescope.value("circles").toList();
	if(!circleList.isEmpty() && circleList.size() <= MAX_CIRCLE_COUNT)
	{
		for(int i = 0; i < circleList.size(); i++)
			circles.append(circleList.value(i, -1.0).toDouble());
	}

	QString connection = telescope.value("connection").toString();
	connectionType = connectionTypeNames.key(connection, ConnectionVirtual);
	if(connectionType == ConnectionInternal)
	{
		deviceModelName = telescope.value("device_model").toString();
		portSerial = telescope.value("serial_port").toString();
	}
	if(connectionType == ConnectionRTS2)
	{
		rts2Url = telescope.value("url").toString();
		rts2Username = telescope.value("username").toString();
		rts2Password = telescope.value("password").toString();
		rts2Refresh = telescope.value("refresh", DEFAULT_RTS2_REFRESH).toInt();
	}
	if(connectionType == ConnectionINDI)
	{
		deviceModelName = telescope.value("device_model").toString();
	}
	if(connectionType == ConnectionASCOM)
	{
		ascomDeviceId = telescope.value("ascom_device_id").toString();
		ascomUseDeviceEqCoordType = telescope.value("ascom_use_device_eq_coord_type").toBool();
	}

	return true;
}


bool TelescopeControl::removeTelescopeAtSlot(int slot)
{
	//Validation
	if(!isValidSlotNumber(slot))
		return false;

	return (telescopeDescriptions.remove(QString::number(slot)) == 1);
}

bool TelescopeControl::startTelescopeAtSlot(int slot)
{
	//Validation
	if(!isValidSlotNumber(slot))
		return false;

	//Read the telescope properties
	QString name;
	QString equinox;
	QString host;
	ConnectionType connectionType;
	int portTCP;
	int delay;
	bool connectAtStartup;
	QList<double> circles;
	QString deviceModelName;
	QString portSerial;
	QString rts2Url;
	QString rts2Username;
	QString rts2Password;
	int rts2Refresh;
	QString ascomDeviceId;
	bool ascomUseDeviceEqCoordType;

	if(!getTelescopeAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, deviceModelName, portSerial, rts2Url, rts2Username, rts2Password, rts2Refresh, ascomDeviceId, ascomUseDeviceEqCoordType))
	{
		//TODO: Add debug
		return false;
	}

	if(connectionType == ConnectionInternal && !deviceModelName.isEmpty())
	{
		if(deviceModels[deviceModelName].useExecutable)
		{
			if (startClientAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, circles))
			{
				if(!startServerAtSlot(slot, deviceModelName, portTCP, portSerial))
				{
					//If a server can't be started, remove the client too
					stopClientAtSlot(slot);
					return false;
				}

				emit clientConnected(slot, name);
				return true;
			}
		}
		else
		{
			addLogAtSlot(slot);
			logAtSlot(slot);
			if (startClientAtSlot(slot, connectionType, name, equinox, QString(), 0, delay, circles, deviceModelName, portSerial))
			{
				emit clientConnected(slot, name);
				return true;
			}
		}
	}
	else
	{
		if (startClientAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, circles, deviceModelName, portSerial, rts2Url, rts2Username, rts2Password, rts2Refresh, ascomDeviceId, ascomUseDeviceEqCoordType))
		{
			emit clientConnected(slot, name);
			return true;
		}
	}

	return false;
}

bool TelescopeControl::stopTelescopeAtSlot(int slot)
{
	//Validation
	if(!isValidSlotNumber(slot))
		return false;

	//If there is a server...
	if(telescopeServerProcess.contains(slot))
	{
		if(!stopServerAtSlot(slot))
		{
			//TODO: Add debug
			return false;
		}
	}

	return stopClientAtSlot(slot);
}

bool TelescopeControl::stopAllTelescopes()
{
	bool allStoppedSuccessfully = true;

	if (!telescopeClients.empty())
	{
		for (auto telescope = telescopeClients.constBegin(); telescope != telescopeClients.constEnd(); ++telescope)
		{
			allStoppedSuccessfully = allStoppedSuccessfully && stopTelescopeAtSlot(telescope.key());
		}
	}

	return allStoppedSuccessfully;
}

bool TelescopeControl::isValidSlotNumber(int slot)
{
	return ((slot >= MIN_SLOT_NUMBER) && (slot <= MAX_SLOT_NUMBER));
}

bool TelescopeControl::isValidPort(uint port)
{
	//Check if the port number is in IANA's allowed range
	return (port > 1023 && port <= 65535);
}

bool TelescopeControl::isValidDelay(int delay)
{
	return (delay > 0 && delay <= MICROSECONDS_FROM_SECONDS(10));
}

bool TelescopeControl::startServerAtSlot(int slotNumber, QString deviceModelName, int portTCP, QString portSerial)
{
	//Validate
	if(!isValidSlotNumber(slotNumber) || !isValidPort(portTCP))
		return false;
	//TODO: Validate the serial port? (Is this method going to be public?)

	if(!deviceModels.contains(deviceModelName))
	{
		//TODO: Add debug
		return false;
	}

	QString slotName = QString::number(slotNumber);
	QString serverName = deviceModels.value(deviceModelName).server;

	if (telescopeServers.contains(serverName))
	{
		QString serverExecutablePath = StelFileMgr::findFile(serverExecutablesDirectoryPath + TELESCOPE_SERVER_PATH.arg(serverName), StelFileMgr::File);
		if (serverExecutablePath.isEmpty())
		{
			qDebug() << "[TelescopeControl] Error starting telescope server: Can't find executable:"				 << QDir::toNativeSeparators(serverExecutablePath);
			return false;
		}

#ifdef Q_OS_WIN
		QString serialPortName = "\\\\.\\" + portSerial;
#else
		QString serialPortName = portSerial;
#endif //Q_OS_WIN
		QStringList serverArguments;
		serverArguments << QString::number(portTCP) << serialPortName;
		if(useTelescopeServerLogs)
			serverArguments << QString(StelFileMgr::getUserDir() + "/log_TelescopeServer" + slotName + ".txt");

		qDebug() << "[TelescopeControl] Starting tellescope server at slot" << slotName
				 << "with path"	 << QDir::toNativeSeparators(serverExecutablePath)
				 << "and arguments" << serverArguments.join(" ");

		//Starting the new process
		telescopeServerProcess.insert(slotNumber, new QProcess());
		//telescopeServerProcess[slotNumber]->setStandardOutputFile(StelFileMgr::getUserDir() + "/log_TelescopeServer" + slotName + ".txt");//In case the server does not accept logfiles
		telescopeServerProcess[slotNumber]->start(serverExecutablePath, serverArguments);
		//return telescopeServerProcess[slotNumber]->waitForStarted();

		//The server is supposed to start immediately
		return true;
	}
	else
		qDebug() << "[TelescopeControl] Error starting telescope server: No such server found:" << serverName;

	return false;
}

bool TelescopeControl::stopServerAtSlot(int slotNumber)
{
	//Validate
	if(!isValidSlotNumber(slotNumber) || !telescopeServerProcess.contains(slotNumber))
		return false;

	//Stop/close the process
#ifdef Q_OS_WIN
	telescopeServerProcess[slotNumber]->close();
#else
	telescopeServerProcess[slotNumber]->terminate();
#endif //Q_OS_WIN
	telescopeServerProcess[slotNumber]->waitForFinished();

	delete telescopeServerProcess[slotNumber];
	telescopeServerProcess.remove(slotNumber);

	return true;
}

bool TelescopeControl::startClientAtSlot(int slotNumber, ConnectionType connectionType, QString name, QString equinox, QString host, int portTCP, int delay, QList<double> circles, QString deviceModelName, QString portSerial, QString rts2Url, QString rts2Username, QString rts2Password, int rts2Refresh, QString ascomDeviceId, bool ascomUseDeviceEqCoordType)
{
	//Validation
	if(!isValidSlotNumber(slotNumber))
		return false;

	//Check if it's not already running
	//Should it return, or should it remove the old one and proceed?
	if(telescopeClients.contains(slotNumber))
	{
		//TODO: Add debug. Update logic?
		return false;
	}

	QString initString;
	switch (connectionType)
	{
	case ConnectionVirtual:
		initString = QString("%1:%2:%3").arg(name, "TelescopeServerDummy", "J2000");
		break;

	case ConnectionInternal:
		if(!deviceModelName.isEmpty() && !portSerial.isEmpty())
			initString = QString("%1:%2:%3:%4:%5").arg(name, deviceModels[deviceModelName].server, equinox, portSerial, QString::number(delay));
		break;

	case ConnectionLocal:
		if (isValidPort(portTCP))
			initString = QString("%1:TCP:%2:%3:%4:%5").arg(name, equinox, "localhost", QString::number(portTCP), QString::number(delay));
		break;

	case ConnectionRTS2:
		if (!rts2Url.isEmpty())
			initString = QString("%1:RTS2:%2:%3:http://%4:%5@%6").arg(name, equinox, QString::number(rts2Refresh), rts2Username, rts2Password, rts2Url);
		break;

	case ConnectionINDI:
		initString = QString("%1:%2:%3:%4:%5:%6").arg(name, "INDI", "J2000", host, QString::number(portTCP), deviceModelName);
		break;

	case ConnectionASCOM:
		initString = QString("%1:%2:%3:%4:%5").arg(name, "ASCOM", equinox, ascomDeviceId, ascomUseDeviceEqCoordType ? "true" : "false");
		break;

	case ConnectionRemote:
	default:
		if (isValidPort(portTCP) && !host.isEmpty())
			initString = QString("%1:TCP:%2:%3:%4:%5").arg(name, equinox, host, QString::number(portTCP), QString::number(delay));
	}

	qDebug() << "connectionType:" << connectionType << " initString:" << initString;

	TelescopeClient* newTelescope = TelescopeClient::create(initString);
	if (newTelescope)
	{
		//Add FOV circles to the client if there are any specified
		if(!circles.isEmpty() && circles.size() <= MAX_CIRCLE_COUNT)
			for (int i = 0; i < circles.size(); ++i)
				newTelescope->addOcular(circles[i]);

		telescopeClients.insert(slotNumber, TelescopeClientP(newTelescope));
		return true;
	}

	return false;
}

bool TelescopeControl::stopClientAtSlot(int slotNumber)
{
	//Validation
	if(!isValidSlotNumber(slotNumber))
		return false;

	//If it doesn't exist, it is stopped :)
	if(!telescopeClients.contains(slotNumber))
		return true;

	//If a telescope is selected, deselect it first.
	//(Otherwise trying to delete a selected telescope client causes Stellarium to crash.)
	//TODO: Find a way to deselect only the telescope client that is about to be deleted.
	const QList<StelObjectP> newSelected = GETSTELMODULE(StelObjectMgr)->getSelectedObject("Telescope");
	if(!newSelected.isEmpty())
	{
		GETSTELMODULE(StelObjectMgr)->unSelect();
	}
	telescopeClients.remove(slotNumber);

	//This is not needed by every client
	removeLogAtSlot(slotNumber);

	emit clientDisconnected(slotNumber);
	return true;
}

void TelescopeControl::loadDeviceModels()
{
	//qDebug() << "TelescopeControl: Loading device model descriptions...";
	
	//Make sure that the device models file exists
	bool useDefaultList = false;
	QString deviceModelsJsonPath = StelFileMgr::findFile("modules/TelescopeControl", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/device_models.json";
	if(!QFileInfo(deviceModelsJsonPath).exists())
	{
		if(!restoreDeviceModelsListTo(deviceModelsJsonPath))
		{
			qWarning() << "[TelescopeControl] Unable to find " << QDir::toNativeSeparators(deviceModelsJsonPath);
			useDefaultList = true;
		}
	}
	else
	{
		QFile deviceModelsJsonFile(deviceModelsJsonPath);
		if(!deviceModelsJsonFile.open(QFile::ReadOnly))
		{
			qWarning() << "[TelescopeControl] Can't open for reading " << QDir::toNativeSeparators(deviceModelsJsonPath);
			useDefaultList = true;
		}
		else
		{
			//Check the version and move the old file if necessary
			QVariantMap deviceModelsJsonMap;
			deviceModelsJsonMap = StelJsonParser::parse(&deviceModelsJsonFile).toMap();
			QString version = deviceModelsJsonMap.value("version", "0.0.0").toString();
			if(StelUtils::compareVersions(version, QString(TELESCOPE_CONTROL_PLUGIN_VERSION))!=0)
			{
				deviceModelsJsonFile.close();
				QString newName = deviceModelsJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
				if(deviceModelsJsonFile.rename(newName))
				{
					qWarning() << "[TelescopeControl] The existing version of device_models.json is obsolete. Backing it up as " << QDir::toNativeSeparators(newName);
					if(!restoreDeviceModelsListTo(deviceModelsJsonPath))
					{
						useDefaultList = true;
					}
				}
				else
				{
					qWarning() << "[TelescopeControl] The existing version of device_models.json is obsolete. Unable to rename.";
					useDefaultList = true;
				}
			}
			else
				deviceModelsJsonFile.close();
		}
	}

	if (useDefaultList)
	{
		qWarning() << "[TelescopeControl] Using embedded device models list.";
		deviceModelsJsonPath = ":/telescopeControl/device_models.json";
	}

	//Open the file and parse the device list
	QVariantList deviceModelsList;
	QFile deviceModelsJsonFile(deviceModelsJsonPath);
	if(deviceModelsJsonFile.open(QFile::ReadOnly))
	{
		deviceModelsList = (StelJsonParser::parse(&deviceModelsJsonFile).toMap()).value("list").toList();
		deviceModelsJsonFile.close();
	}
	else
	{
		return;
	}

	//Compile a list of the available telescope server executables
	loadTelescopeServerExecutables();
	if(telescopeServers.isEmpty())
	{
		//deviceModels = QHash<QString, DeviceModel>();
		//return;
		qWarning() << "[TelescopeControl] Only embedded telescope servers are available.";
	}

	//Clear the list of device models - it may not be empty.
	deviceModels.clear();

	//Cicle the list of telescope deifinitions
	for(int i = 0; i < deviceModelsList.size(); i++)
	{
		QVariantMap model = deviceModelsList.at(i).toMap();
		if(model.isEmpty())
			continue;

		//Model name
		QString name = model.value("name").toString();
		if(name.isEmpty())
		{
			//TODO: Add warning
			continue;
		}

		if(deviceModels.contains(name))
		{
			qWarning() << "[TelescopeControl] Skipping device model: Duplicate name:" << name;
			continue;
		}

		//Telescope server
		QString server = model.value("server").toString();
		//The default behaviour is to use embedded servers:
		bool useExecutable = false;
		if(server.isEmpty())
		{
			qWarning() << "[TelescopeControl] Skipping device model: No server specified for" << name;
			continue;
		}
		if(useServerExecutables)
		{
			if(telescopeServers.contains(server))
			{
				qDebug() << "[TelescopeControl] Using telescope server executable for" << name;
				useExecutable = true;
			}
			else if(EMBEDDED_TELESCOPE_SERVERS.contains(server))
			{
				qWarning() << "[TelescopeControl] No external telescope server executable found for" << name;
				qWarning() << "[TelescopeControl] Using embedded telescope server" << server
						   << "for" << name;
				useExecutable = false;
			}
			else
			{
				qWarning() << "[TelescopeControl] Skipping device model: No server" << server
						   << "found for" << name;
				continue;
			}
		}
		else
		{
			if(!EMBEDDED_TELESCOPE_SERVERS.contains(server))
			{
				qWarning() << "[TelescopeControl] Skipping device model: No server" << server
						   << "found for" << name;
				continue;
			}
			//else: everything is OK, using embedded server
		}
		
		//Description and default connection delay
		QString description = model.value("description", "No description is available.").toString();
		int delay = model.value("default_delay", DEFAULT_DELAY).toInt();
		
		//Add this to the main list
		DeviceModel newDeviceModel = {name, description, server, delay, useExecutable};
		deviceModels.insert(name, newDeviceModel);
		qDebug() << "[TelescopeControl] Adding device model:" << name << description << server << delay;
	}
}

const QHash<QString, DeviceModel>& TelescopeControl::getDeviceModels()
{
	return deviceModels;
}

QHash<int, QString> TelescopeControl::getConnectedClientsNames()
{
	QHash<int, QString> connectedClientsNames;
	if (telescopeClients.isEmpty())
		return connectedClientsNames;

	for (const auto slotNumber : telescopeClients.keys())
	{
		if (telescopeClients.value(slotNumber)->isConnected())
			connectedClientsNames.insert(slotNumber, telescopeClients.value(slotNumber)->getNameI18n());
	}

	return connectedClientsNames;
}

bool TelescopeControl::restoreDeviceModelsListTo(QString deviceModelsListPath)
{
	QFile defaultFile(":/telescopeControl/device_models.json");
	if (!defaultFile.copy(deviceModelsListPath))
	{
		qWarning() << "[TelescopeControl] Unable to copy the default device models list to" << QDir::toNativeSeparators(deviceModelsListPath);
		return false;
	}
	QFile newCopy(deviceModelsListPath);
	newCopy.setPermissions(newCopy.permissions() | QFile::WriteOwner);

	qDebug() << "[TelescopeControl] The default device models list has been copied to" << QDir::toNativeSeparators(deviceModelsListPath);
	return true;
}

const QString& TelescopeControl::getServerExecutablesDirectoryPath() const
{
	return serverExecutablesDirectoryPath;
}

bool TelescopeControl::setServerExecutablesDirectoryPath(const QString& newPath)
{
	//TODO: Reuse code.
	QDir newServerDirectory(newPath);
	if(!newServerDirectory.exists())
	{
		qWarning() << "[TelescopeControl] Can't find such a directory: " << QDir::toNativeSeparators(newPath);
		return false;
	}
	QList<QFileInfo> telescopeServerExecutables = newServerDirectory.entryInfoList(QStringList("TelescopeServer*"), (QDir::Files|QDir::Executable|QDir::CaseSensitive), QDir::Name);
	if(telescopeServerExecutables.isEmpty())
	{
		qWarning() << "[TelescopeControl] No telescope server executables found in"
				   << QDir::toNativeSeparators(serverExecutablesDirectoryPath);
		return false;
	}

	//If everything is fine...
	serverExecutablesDirectoryPath = newPath;

	stopAllTelescopes();
	loadDeviceModels();
	return true;
}

void TelescopeControl::setFlagUseServerExecutables(bool useExecutables)
{
	//If the new value is the same as the old, no action is required
	if(useServerExecutables == useExecutables)
		return;

	useServerExecutables = useExecutables;
	stopAllTelescopes();
	loadDeviceModels();
}

void TelescopeControl::addLogAtSlot(int slot)
{
	if(!telescopeServerLogFiles.contains(slot)) // || !telescopeServerLogFiles.value(slot)->isOpen()
	{
		//If logging is off, use an empty stream to avoid segmentation fault
		if(!useTelescopeServerLogs)
		{
			telescopeServerLogFiles.insert(slot, new QFile());
			telescopeServerLogStreams.insert(slot, new QTextStream(telescopeServerLogFiles.value(slot)));
			return;
		}

		QString filePath = StelFileMgr::getUserDir() + "/log_TelescopeServer" + QString::number(slot) + ".txt";
		QFile* logFile = new QFile(filePath);
		if (!logFile->open(QFile::WriteOnly|QFile::Text|QFile::Truncate|QFile::Unbuffered))
		{
			qWarning() << "[TelescopeControl] Unable to create a log file for slot" << slot
					   << ":" << QDir::toNativeSeparators(filePath);
			telescopeServerLogFiles.insert(slot, logFile);
			telescopeServerLogStreams.insert(slot, new QTextStream(new QFile()));
		}

		telescopeServerLogFiles.insert(slot, logFile);
		QTextStream * logStream = new QTextStream(logFile);
		telescopeServerLogStreams.insert(slot, logStream);
	}
}

void TelescopeControl::removeLogAtSlot(int slot)
{
	if(telescopeServerLogFiles.contains(slot))
	{
		telescopeServerLogFiles.value(slot)->close();
		telescopeServerLogStreams.remove(slot);
		telescopeServerLogFiles.remove(slot);
	}
}

void TelescopeControl::logAtSlot(int slot)
{
	if(telescopeServerLogStreams.contains(slot))
		log_file = telescopeServerLogStreams.value(slot);
}

QStringList TelescopeControl::listMatchingObjects(const QString& objPrefix, int maxNbItem, bool useStartOfWords) const
{
	QStringList result;
	if (maxNbItem<=0)
		return result;

	QString tn;
	bool find;
	for (const auto& telescope : telescopeClients)
	{
		tn = telescope->getNameI18n();
		find = false;
		if (useStartOfWords)
		{
			if (objPrefix.toUpper()==tn.mid(0, objPrefix.size()).toUpper())
				find = true;
		}
		else
		{
			if (tn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
			result << tn;

		tn = telescope->getEnglishName();
		find = false;
		if (useStartOfWords)
		{
			if (objPrefix.toUpper()==tn.mid(0, objPrefix.size()).toUpper())
				find = true;
		}
		else
		{
			if (tn.contains(objPrefix, Qt::CaseInsensitive))
				find = true;
		}
		if (find)
			result << tn;
	}
	result.sort();
	if (result.size()>maxNbItem)
		result.erase(result.begin()+maxNbItem, result.end());

	return result;
}

void TelescopeControl::translations()
{
#if 0
	// TRANSLATORS: Description for Meade AutoStar compatible mounts
	N_("Any telescope or telescope mount compatible with Meade's AutoStar controller.")
			// TRANSLATORS: Description for Meade LX200 (compatible) mounts
			N_("Any telescope or telescope mount compatible with Meade LX200.")
			// TRANSLATORS: Description for Meade ETX70 (#494 Autostar, #506 CCS) mounts
			N_("Meade's ETX70 with the #494 Autostar controller and the #506 Connector Cable Set.")
			// TRANSLATORS: Description for Losmandy G-11 mounts
			N_("Losmandy's G-11 telescope mount.")
			// TRANSLATORS: Description for Wildcard Innovations Argo Navis (Meade mode) mounts
			N_("Wildcard Innovations' Argo Navis DTC in Meade LX200 emulation mode.")
			// TRANSLATORS: Description for Celestron NexStar (compatible) mounts
			N_("Any telescope or telescope mount compatible with Celestron NexStar.")
			// TRANSLATORS: Description for Sky-Watcher SynScan (version 3 or later) mounts
			N_("Any Sky-Watcher mount that uses version 3 or later of the SynScan hand controller.")
			// TRANSLATORS: Description for Sky-Watcher SynScan AZ GOTO mounts
			N_("The Sky-Watcher SynScan AZ GOTO mount used in a number of telescope models.")
		#endif
}

