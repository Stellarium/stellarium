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

#include "TelescopeControl.hpp"
#include "TelescopeClient.hpp"
#include "TelescopeDialog.hpp"
#include "SlewDialog.hpp"
#include "LogFile.hpp"

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
#include "StelProjector.hpp"
#include "StelShortcutMgr.hpp"
#include "StelStyle.hpp"
#include "renderer/StelGeometryBuilder.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMapIterator>
#include <QSettings>
#include <QString>
#include <QStringList>

#include <QDebug>

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
	info.authors = "Bogdan Marinov, Johannes Gajdosik";
	info.contact = "http://stellarium.org";
	info.description = N_("This plug-in allows Stellarium to send \"slew\" commands to a telescope on a computerized mount (a \"GoTo telescope\").");
	return info;
}

Q_EXPORT_PLUGIN2(TelescopeControl, TelescopeControlStelPluginInterface)


////////////////////////////////////////////////////////////////////////////////
// Constructor and destructor
TelescopeControl::TelescopeControl()
	: pixmapHover(NULL)
	, pixmapOnIcon(NULL)
	, pixmapOffIcon(NULL)
	, reticleTexture(NULL)
	, selectionTexture(NULL)
	, telescopeDialog(NULL)
	, slewDialog(NULL)
	, actionGroupId("PluginTelescopeControl")
	, moveToSelectedActionId("actionMove_Telescope_To_Selection_%1")
	, moveToCenterActionId("actionSlew_Telescope_To_Direction_%1")
{
	setObjectName("TelescopeControl");

	connectionTypeNames.insert(ConnectionVirtual, "virtual");
	connectionTypeNames.insert(ConnectionInternal, "internal");
	connectionTypeNames.insert(ConnectionLocal, "local");
	connectionTypeNames.insert(ConnectionRemote, "remote");
}

TelescopeControl::~TelescopeControl()
{

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
			qWarning() << "TelescopeControl: No device model descriptions have been loaded. Stellarium will not be able to control a telescope on its own, but it is still possible to do it through an external application or to connect to a remote host.";
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
		
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		StelShortcutMgr* shMgr = StelApp::getInstance().getStelShortcutManager();

		//Create telescope key bindings
		/* QAction-s with these key bindings existed in Stellarium prior to
			revision 6311. Any future backports should account for that. */
		for (int i = MIN_SLOT_NUMBER; i <= MAX_SLOT_NUMBER; i++)
		{
			// "Slew to object" commands
			QString name = moveToSelectedActionId.arg(i);
			QString shortcut = QString("Ctrl+%1").arg(i);
			QAction* action = shMgr->addGuiAction(name, true, "",
			                                      shortcut, "", actionGroupId,
			                                      false);
			connect(action, SIGNAL(triggered()),
			        this, SLOT(slewTelescopeToSelectedObject()));

			// "Slew to the center of the screen" commands
			name = moveToCenterActionId.arg(i);
			shortcut = QString("Alt+%1").arg(i);
			action = shMgr->addGuiAction(name, true, "",
			                             shortcut, "", actionGroupId,
			                             false, false);
			connect(action, SIGNAL(triggered()), this,
			        SLOT(slewTelescopeToViewDirection()));
		}
		// Also updates descriptions if the actions have been loaded from file
		translateActionDescriptions();
		connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
		        this, SLOT(translateActionDescriptions()));
	
		//Create and initialize dialog windows 
		telescopeDialog = new TelescopeDialog();
		slewDialog = new SlewDialog();
		
		connect(shMgr->getGuiAction("actionShow_Slew_Window"), SIGNAL(toggled(bool)), slewDialog, SLOT(setVisible(bool)));
		connect(slewDialog, SIGNAL(visibleChanged(bool)), shMgr->getGuiAction("actionShow_Slew_Window"), SLOT(setChecked(bool)));
		
		//Create toolbar button
		pixmapHover   = new QPixmap(":/graphicGui/glow32x32.png");
		pixmapOnIcon  = new QPixmap(":/telescopeControl/button_Slew_Dialog_on.png");
		pixmapOffIcon = new QPixmap(":/telescopeControl/button_Slew_Dialog_off.png");
        toolbarButton = new StelButton(NULL, *pixmapOnIcon, *pixmapOffIcon, *pixmapHover, gui->getGuiAction("actionShow_Slew_Window"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "TelescopeControl::init() error: " << e.what();
		return;
	}
	
	GETSTELMODULE(StelObjectMgr)->registerStelObjectMgr(this);
	
	//Initialize style, as it is not called at startup:
	//(necessary to initialize the reticle/label/circle colors)
	setStelStyle(StelApp::getInstance().getCurrentStelStyle());
	connect(&StelApp::getInstance(), SIGNAL(colorSchemeChanged(const QString&)), this, SLOT(setStelStyle(const QString&)));
}

void TelescopeControl::deinit()
{
	//Destroy all clients first in order to avoid displaying a TCP error
	deleteAllTelescopes();

	QHash<int, QProcess*>::const_iterator iterator = telescopeServerProcess.constBegin();
	while(iterator != telescopeServerProcess.constEnd())
	{
		int slotNumber = iterator.key();
#ifdef Q_OS_WIN32
		telescopeServerProcess[slotNumber]->close();
#else
		telescopeServerProcess[slotNumber]->terminate();
#endif
		telescopeServerProcess[slotNumber]->waitForFinished();
		delete telescopeServerProcess[slotNumber];
		qDebug() << "TelescopeControl::deinit(): Server process at slot" << slotNumber << "terminated successfully.";

		++iterator;
	}

	if(NULL != reticleTexture)   {delete reticleTexture;}
	if(NULL != selectionTexture) {delete selectionTexture;}
	if(NULL != telescopeDialog)  {delete telescopeDialog;}
	if(NULL != slewDialog)       {delete slewDialog;}
	if(NULL != pixmapHover)      {delete pixmapHover;}
	if(NULL != pixmapOnIcon)     {delete pixmapOnIcon;}
	if(NULL != pixmapOffIcon)    {delete pixmapOffIcon;}
	reticleTexture = selectionTexture = NULL;
	telescopeDialog = NULL;
	slewDialog = NULL;
	pixmapHover = pixmapOnIcon = pixmapOffIcon;

	//TODO: Decide if it should be saved on change
	//Save the configuration on exit
	saveConfiguration();
}

void TelescopeControl::update(double deltaTime)
{
	labelFader.update((int)(deltaTime*1000));
	reticleFader.update((int)(deltaTime*1000));
	circleFader.update((int)(deltaTime*1000));
	// communicate with the telescopes:
	communicate();
}

void TelescopeControl::draw(StelCore* core, StelRenderer* renderer)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	renderer->setFont(labelFont);
	if(NULL == reticleTexture)
	{
		Q_ASSERT_X(NULL == selectionTexture, Q_FUNC_INFO, "Textures should be created simultaneously");
		reticleTexture   = renderer->createTexture(":/telescopeControl/telescope_reticle.png");
		selectionTexture = renderer->createTexture("textures/pointeur2.png");
		
	}
	foreach (const TelescopeClientP& telescope, telescopeClients)
	{
		if (telescope->isConnected() && telescope->hasKnownPosition())
		{
			Vec3d XY;
			if (prj->projectCheck(telescope->getJ2000EquatorialPos(core), XY))
			{
				//Telescope circles appear synchronously with markers
				if (circleFader.getInterstate() >= 0)
				{
					renderer->setGlobalColor(circleColor[0], circleColor[1], circleColor[2],
					                         circleFader.getInterstate());
					renderer->setBlendMode(BlendMode_None);
					StelVertexBuffer<VertexP2>* circleBuffer =
						renderer->createVertexBuffer<VertexP2>(PrimitiveType_LineStrip);
					foreach (double circle, telescope->getOculars())
					{
						StelGeometryBuilder()
							.buildCircle(circleBuffer, XY[0], XY[1],
							             0.5f * prj->getPixelPerRadAtCenter() * (M_PI / 180.0f) * circle);
						renderer->drawVertexBuffer(circleBuffer);
					
						circleBuffer->unlock();
						circleBuffer->clear();
					}
					delete circleBuffer;
				}
				if (reticleFader.getInterstate() >= 0)
				{
					renderer->setBlendMode(BlendMode_Alpha);
					reticleTexture->bind();
					renderer->setGlobalColor(reticleColor[0], reticleColor[1], reticleColor[2],
					                         reticleFader.getInterstate());
					renderer->drawTexturedRect(XY[0] - 15.0f, XY[1] - 15.0f, 30.0f, 30.0f);
				}
				if (labelFader.getInterstate() >= 0)
				{
					renderer->setGlobalColor(labelColor[0], labelColor[1], labelColor[2],
					                         labelFader.getInterstate());
					//TODO: Different position of the label if circles are shown?
					//TODO: Remove magic number (text spacing)
					renderer->drawText(TextParams(XY[0], XY[1], telescope->getNameI18n())
					                   .shift(6 + 10, - 4).useGravity());
					//Same position as the other objects: doesn't work, telescope label overlaps object label
					//sPainter.drawText(XY[0], XY[1], scope->getNameI18n(), 0, 10, 10, false);
				}
			}
		}
	}

	if(GETSTELMODULE(StelObjectMgr)->getFlagSelectedObjectPointer())
	{
		drawPointer(prj, core, renderer);
	}
}

void TelescopeControl::setStelStyle(const QString& section)
{
	if (section == "night_color")
	{
		setLabelColor(labelNightColor);
		setReticleColor(reticleNightColor);
		setCircleColor(circleNightColor);
	}
	else
	{
		setLabelColor(labelNormalColor);
		setReticleColor(reticleNormalColor);
		setCircleColor(circleNormalColor);
	}

	telescopeDialog->updateStyle();
}

double TelescopeControl::getCallOrder(StelModuleActionName actionName) const
{
	//TODO: Remove magic number (call order offset)
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("MeteorMgr")->getCallOrder(actionName) + 2.;
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
	foreach (const TelescopeClientP& telescope, telescopeClients)
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
	foreach (const TelescopeClientP& telescope, telescopeClients)
	{
		if (telescope->getNameI18n() == nameI18n)
			return qSharedPointerCast<StelObject>(telescope);
	}
	return 0;
}

StelObjectP TelescopeControl::searchByName(const QString &name) const
{
	foreach (const TelescopeClientP& telescope, telescopeClients)
	{
		if (telescope->getEnglishName() == name)
			return qSharedPointerCast<StelObject>(telescope);
	}
	return 0;
}

QStringList TelescopeControl::listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem) const
{
	QStringList result;
	if (maxNbItem==0) return result;

	QString objw = objPrefix.toUpper();
	foreach (const TelescopeClientP& telescope, telescopeClients)
	{
		QString constw = telescope->getNameI18n().mid(0, objw.size()).toUpper();
		if (constw==objw)
		{
			result << telescope->getNameI18n();
		}
	}
	result.sort();
	if (result.size()>maxNbItem)
	{
		result.erase(result.begin() + maxNbItem, result.end());
	}
	return result;
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

void TelescopeControl::slewTelescopeToSelectedObject()
{
	// Find out for which telescope is the command
	if (sender() == NULL)
		return;
	int slotNumber = sender()->objectName().right(1).toInt();

	// Find out the coordinates of the target
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	StelObjectP selectObject = omgr->getSelectedObject().at(0);
	if (!selectObject)  // should never happen
		return;

	Vec3d objectPosition = selectObject->getJ2000EquatorialPos(StelApp::getInstance().getCore());

	telescopeGoto(slotNumber, objectPosition);
}

void TelescopeControl::slewTelescopeToViewDirection()
{
	// Find out for which telescope is the command
	if (sender() == NULL)
		return;
	int slotNumber = sender()->objectName().right(1).toInt();

	// Find out the coordinates of the target
	Vec3d centerPosition = GETSTELMODULE(StelMovementMgr)->getViewDirectionJ2000();

	telescopeGoto(slotNumber, centerPosition);
}

void TelescopeControl::drawPointer(const StelProjectorP& prj, const StelCore* core, StelRenderer* renderer)
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
		renderer->setGlobalColor(c[0], c[1], c[2]);
		selectionTexture->bind();
		renderer->setBlendMode(BlendMode_Alpha);
		renderer->drawTexturedRect(screenpos[0] - 25.0f, screenpos[1] - 25.0f, 50.0f, 50.0f,
		                           StelApp::getInstance().getTotalRunTime() * 40.0f);
	}
#endif //COMPATIBILITY_001002
}

void TelescopeControl::telescopeGoto(int slotNumber, const Vec3d &j2000Pos)
{
	//TODO: See the original code. I think that something is wrong here...
	if(telescopeClients.contains(slotNumber))
		telescopeClients.value(slotNumber)->telescopeGoto(j2000Pos);
}

void TelescopeControl::communicate(void)
{
	if (!telescopeClients.empty())
	{
		QMap<int, TelescopeClientP>::const_iterator telescope = telescopeClients.constBegin();
		while (telescope != telescopeClients.end())
		{
			logAtSlot(telescope.key());//If there's no log, it will be ignored
			if(telescope.value()->prepareCommunication())
			{
				telescope.value()->performCommunication();
			}
			telescope++;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
// Methods for managing telescope client objects
//
void TelescopeControl::deleteAllTelescopes()
{
	//TODO: I really hope that this won't cause a memory leak...
	//foreach (TelescopeClient* telescope, telescopeClients)
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
		qWarning() << "TelescopeControl: No telescope server directory has been found.";
		return;
	}
	QList<QFileInfo> telescopeServerExecutables = serverDirectory.entryInfoList(QStringList("TelescopeServer*"), (QDir::Files|QDir::Executable|QDir::CaseSensitive), QDir::Name);
	if(!telescopeServerExecutables.isEmpty())
	{
		foreach(QFileInfo telescopeServerExecutable, telescopeServerExecutables)
			telescopeServers.append(telescopeServerExecutable.baseName());//This strips the ".exe" suffix on Windows
	}
	else
	{
		qWarning() << "TelescopeControl: No telescope server executables found in"
							 << serverExecutablesDirectoryPath;
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
#ifdef Q_OS_WIN32
	setFontSize(settings->value("telescope_labels_font_size", 13).toInt()); //Windows Qt bug workaround
#else
	setFontSize(settings->value("telescope_labels_font_size", 12).toInt());
#endif

	//Load colours
	reticleNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_reticles", "0.6,0.4,0").toString());
	reticleNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_reticles", "0.5,0,0").toString());
	labelNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_labels", "0.6,0.4,0").toString());
	labelNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_labels", "0.5,0,0").toString());
	circleNormalColor = StelUtils::strToVec3f(settings->value("color_telescope_circles", "0.6,0.4,0").toString());
	circleNightColor = StelUtils::strToVec3f(settings->value("night_color_telescope_circles", "0.5,0,0").toString());

	//Load server executables flag and directory
	useServerExecutables = settings->value("flag_use_server_executables", false).toBool();
	serverExecutablesDirectoryPath = settings->value("server_executables_path").toString();

	//If no directory is specified in the configuration file, try to find the default one
	if(serverExecutablesDirectoryPath.isEmpty() || !QDir(serverExecutablesDirectoryPath).exists())
	{
		//Find out if the default server directory exists
		QString serverDirectoryPath;
		try
		{
			serverDirectoryPath = StelFileMgr::findFile("servers", StelFileMgr::Directory);
		}
		catch(std::runtime_error &e)
		{
			//qDebug() << "TelescopeControl: No telescope servers directory detected.";
			useServerExecutables = false;
			serverDirectoryPath = StelFileMgr::getUserDir() + "/servers";
		}
		if(!serverDirectoryPath.isEmpty())
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
	settings->setValue("color_telescope_reticles", QString("%1,%2,%3").arg(reticleNormalColor[0], 0, 'f', 2).arg(reticleNormalColor[1], 0, 'f', 2).arg(reticleNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_reticles", QString("%1,%2,%3").arg(reticleNightColor[0], 0, 'f', 2).arg(reticleNightColor[1], 0, 'f', 2).arg(reticleNightColor[2], 0, 'f', 2));
	settings->setValue("color_telescope_labels", QString("%1,%2,%3").arg(labelNormalColor[0], 0, 'f', 2).arg(labelNormalColor[1], 0, 'f', 2).arg(labelNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_labels", QString("%1,%2,%3").arg(labelNightColor[0], 0, 'f', 2).arg(labelNightColor[1], 0, 'f', 2).arg(labelNightColor[2], 0, 'f', 2));
	settings->setValue("color_telescope_circles", QString("%1,%2,%3").arg(circleNormalColor[0], 0, 'f', 2).arg(circleNormalColor[1], 0, 'f', 2).arg(circleNormalColor[2], 0, 'f', 2));
	settings->setValue("night_color_telescope_circles", QString("%1,%2,%3").arg(circleNightColor[0], 0, 'f', 2).arg(circleNightColor[1], 0, 'f', 2).arg(circleNightColor[2], 0, 'f', 2));

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
}

void TelescopeControl::saveTelescopes()
{
	try
	{
		//Open/create the JSON file
		QString telescopesJsonPath = StelFileMgr::findFile("modules/TelescopeControl", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/telescopes.json";
		QFile telescopesJsonFile(telescopesJsonPath);
		if(!telescopesJsonFile.open(QFile::WriteOnly|QFile::Text))
		{
			qWarning() << "TelescopeControl: Telescopes can not be saved. A file can not be open for writing:" << telescopesJsonPath;
			return;
		}

		//Add the version:
		telescopeDescriptions.insert("version", QString(TELESCOPE_CONTROL_VERSION));

		//Convert the tree to JSON
		StelJsonParser::write(telescopeDescriptions, &telescopesJsonFile);
		telescopesJsonFile.flush();//Is this necessary?
		telescopesJsonFile.close();
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "TelescopeControl: Error saving telescopes: " << e.what();
		return;
	}
}

void TelescopeControl::loadTelescopes()
{
	QVariantMap result;
	try
	{
		QString telescopesJsonPath = StelFileMgr::findFile("modules/TelescopeControl", (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable)) + "/telescopes.json";

		if(!QFileInfo(telescopesJsonPath).exists())
		{
			qWarning() << "TelescopeControl::loadTelescopes(): No telescopes loaded. File is missing:" << telescopesJsonPath;
			telescopeDescriptions = result;
			return;
		}

		QFile telescopesJsonFile(telescopesJsonPath);

		QVariantMap map;

		if(!telescopesJsonFile.open(QFile::ReadOnly))
		{
			qWarning() << "TelescopeControl: No telescopes loaded. Can't open for reading" << telescopesJsonPath;
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
		if(version < QString(TELESCOPE_CONTROL_VERSION))
		{
			QString newName = telescopesJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
			if(telescopesJsonFile.rename(newName))
			{
				qWarning() << "TelescopeControl: The existing version of telescopes.json is obsolete. Backing it up as" << newName;
				qWarning() << "TelescopeControl: A blank telescopes.json file will have to be created.";
				telescopeDescriptions = result;
				return;
			}
			else
			{
				qWarning() << "TelescopeControl: The existing version of telescopes.json is obsolete. Unable to rename.";
				telescopeDescriptions = result;
				return;
			}
		}
		map.remove("version");//Otherwise it will try to read it as a telescope

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
				qDebug() << "TelescopeControl::loadTelescopes(): Deleted node unrecogised as slot:" << key;
				map.remove(key);
				continue;
			}

			QVariantMap telescope = node.value().toMap();

			//Essential parameters: Name, connection type, equinox
			//Validation: Name
			QString name = telescope.value("name").toString();
			if(name.isEmpty())
			{
				qDebug() << "TelescopeControl: Unable to load telescope: No name specified at slot" << key;
				map.remove(key);
				continue;
			}

			//Validation: Connection
			QString connection = telescope.value("connection").toString();
			if(connection.isEmpty() || !connectionTypeNames.values().contains(connection))
			{
				qDebug() << "TelescopeControl: Unable to load telescope: No valid connection type at slot" << key;
				map.remove(key);
				continue;
			}
			ConnectionType connectionType = connectionTypeNames.key(connection);

			QString equinox = telescope.value("equinox", "J2000").toString();
			if (equinox != "J2000" && equinox != "JNow")
			{
				qDebug() << "TelescopeControl: Unable to load telescope: Invalid equinox value at slot" << key;
				map.remove(key);
				continue;
			}

			QString hostName("localhost");
			int portTCP = 0;
			int delay = 0;
			QString deviceModelName;
			QString portSerial;

			if (connectionType == ConnectionInternal)
			{
				//Serial port and device model
				deviceModelName = telescope.value("device_model").toString();
				portSerial = telescope.value("serial_port").toString();

				if(deviceModelName.isEmpty())
				{
					qDebug() << "TelescopeControl: Unable to load telescope: No device model specified at slot" << key;
					map.remove(key);
					continue;
				}

				//Do we have this server?
				if(!deviceModels.contains(deviceModelName))
				{
					qWarning() << "TelescopeControl: Unable to load telescope at slot" << slot << "because the specified device model is missing:" << deviceModelName;
					map.remove(key);
					continue;
				}

				if(portSerial.isEmpty() || !portSerial.startsWith(SERIAL_PORT_PREFIX))
				{
					qDebug() << "TelescopeControl: Unable to load telescope: No valid serial port specified at slot" << key;
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
					qDebug() << "TelescopeControl::loadTelescopes(): No host name at slot" << key;
					map.remove(key);
					continue;
				}
			}

			if (connectionType != ConnectionVirtual)
			{
				//Validation: TCP port
				portTCP = telescope.value("tcp_port").toInt();
				if(!telescope.contains("tcp_port") || !isValidPort(portTCP))
				{
					qDebug() << "TelescopeControl: Unable to load telescope: No valid TCP port at slot" << key;
					map.remove(key);
					continue;
				}

				//Validation: Delay
				delay = telescope.value("delay", 0).toInt();
				if(!isValidDelay(delay))
				{
					qDebug() << "TelescopeControl: Unable to load telescope: No valid delay at slot" << key;
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
								qDebug() << "TelescopeControl: Unable to launch a telescope server at slot" << slot;
							}
						}
						else
						{
							qDebug() << "TelescopeControl: Unable to create a telescope client at slot" << slot;
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
							qDebug() << "TelescopeControl: Unable to create a telescope client at slot" << slot;
							//Unnecessary due to if-else construction;
							//also, causes bug #608533
							//continue;
						}
					}
				}
				else
				{
					if(!startClientAtSlot(slot, connectionType, name, equinox, hostName, portTCP, delay, internalCircles))
					{
						qDebug() << "TelescopeControl: Unable to create a telescope client at slot" << slot;
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
			qDebug() << "TelescopeControl: Loaded successfully" << telescopesCount << "telescopes.";
		}
	}
	catch(std::runtime_error &e)
	{
		qWarning() << "TelescopeControl: Error loading telescopes: " << e.what();
	}

	telescopeDescriptions = result;
}

bool TelescopeControl::addTelescopeAtSlot(int slot, ConnectionType connectionType, QString name, QString equinox, QString host, int portTCP, int delay, bool connectAtStartup, QList<double> circles, QString deviceModelName, QString portSerial)
{
	//Validation
	if(!isValidSlotNumber(slot) || name.isEmpty() || equinox.isEmpty() || connectionType <= ConnectionNA || connectionType >= ConnectionCount)
		return false;

	//Create a new map node and fill it with parameters
	QVariantMap telescope;
	telescope.insert("name", name);
	telescope.insert("connection", connectionTypeNames.value(connectionType));
	telescope.insert("equinox", equinox);//TODO: Validation!

	if (connectionType == ConnectionRemote)
	{
		//TODO: Add more validation!
		if (host.isEmpty())
			return false;
		telescope.insert("host_name", host);
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
		if (!isValidPort(portTCP))
			return false;
		telescope.insert("tcp_port", portTCP);

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

bool TelescopeControl::getTelescopeAtSlot(int slot, ConnectionType& connectionType, QString& name, QString& equinox, QString& host, int& portTCP, int& delay, bool& connectAtStartup, QList<double>& circles, QString& deviceModelName, QString& portSerial)
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
	if(!getTelescopeAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, deviceModelName, portSerial))
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
		if (startClientAtSlot(slot, connectionType, name, equinox, host, portTCP, delay, circles))
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
		QMap<int, TelescopeClientP>::const_iterator telescope = telescopeClients.constBegin();
		while (telescope != telescopeClients.end())
		{
			allStoppedSuccessfully = allStoppedSuccessfully && stopTelescopeAtSlot(telescope.key());
			telescope++;
		}
	}

	return allStoppedSuccessfully;
}

bool TelescopeControl::isValidSlotNumber(int slot)
{
	return ((slot < MIN_SLOT_NUMBER || slot >  MAX_SLOT_NUMBER) ? false : true);
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
		QString serverExecutablePath;
		//Is the try/catch really necessary?
		try
		{
			serverExecutablePath = StelFileMgr::findFile(serverExecutablesDirectoryPath + TELESCOPE_SERVER_PATH.arg(serverName), StelFileMgr::File);
		}
		catch (std::runtime_error& e)
		{
			qDebug() << "TelescopeControl: Error starting telescope server: Can't find executable:" << serverExecutablePath;
			return false;
		}

#ifdef Q_OS_WIN32
		QString serialPortName;
		if(portSerial.right(portSerial.size() - SERIAL_PORT_PREFIX.size()).toInt() > 9)
			serialPortName = "\\\\.\\" + portSerial;//"\\.\COMxx", not sure if it will work
		else
			serialPortName = portSerial;
#else
		QString serialPortName = portSerial;
#endif //Q_OS_WIN32
		QStringList serverArguments;
		serverArguments << QString::number(portTCP) << serialPortName;
		if(useTelescopeServerLogs)
			serverArguments << QString(StelFileMgr::getUserDir() + "/log_TelescopeServer" + slotName + ".txt");

		qDebug() << "TelescopeControl: Starting tellescope server at slot" << slotName << "with path" << serverExecutablePath << "and arguments" << serverArguments.join(" ");

		//Starting the new process
		telescopeServerProcess.insert(slotNumber, new QProcess());
		//telescopeServerProcess[slotNumber]->setStandardOutputFile(StelFileMgr::getUserDir() + "/log_TelescopeServer" + slotName + ".txt");//In case the server does not accept logfiles
		telescopeServerProcess[slotNumber]->start(serverExecutablePath, serverArguments);
		//return telescopeServerProcess[slotNumber]->waitForStarted();

		//The server is supposed to start immediately
		return true;
	}
	else
		qDebug() << "TelescopeControl: Error starting telescope server: No such server found:" << serverName;

	return false;
}

bool TelescopeControl::stopServerAtSlot(int slotNumber)
{
	//Validate
	if(!isValidSlotNumber(slotNumber) || !telescopeServerProcess.contains(slotNumber))
		return false;

	//Stop/close the process
#ifdef Q_OS_WIN32
	telescopeServerProcess[slotNumber]->close();
#else
	telescopeServerProcess[slotNumber]->terminate();
#endif //Q_OS_WIN32
	telescopeServerProcess[slotNumber]->waitForFinished();

	delete telescopeServerProcess[slotNumber];
	telescopeServerProcess.remove(slotNumber);

	return true;
}

bool TelescopeControl::startClientAtSlot(int slotNumber, ConnectionType connectionType, QString name, QString equinox, QString host, int portTCP, int delay, QList<double> circles, QString deviceModelName, QString portSerial)
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

		case ConnectionRemote:
		default:
			if (isValidPort(portTCP) && !host.isEmpty())
				initString = QString("%1:TCP:%2:%3:%4:%5").arg(name, equinox, host, QString::number(portTCP), QString::number(delay));
	}

	//qDebug() << "initString:" << initString;

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
			qWarning() << "TelescopeControl: Unable to find" << deviceModelsJsonPath;
			useDefaultList = true;
		}
	}
	else
	{
		QFile deviceModelsJsonFile(deviceModelsJsonPath);
		if(!deviceModelsJsonFile.open(QFile::ReadOnly))
		{
			qWarning() << "TelescopeControl: Can't open for reading" << deviceModelsJsonPath;
			useDefaultList = true;
		}
		else
		{
			//Check the version and move the old file if necessary
			QVariantMap deviceModelsJsonMap;
			deviceModelsJsonMap = StelJsonParser::parse(&deviceModelsJsonFile).toMap();
			QString version = deviceModelsJsonMap.value("version", "0.0.0").toString();
			if(version < QString(TELESCOPE_CONTROL_VERSION))
			{
				deviceModelsJsonFile.close();
				QString newName = deviceModelsJsonPath + ".backup." + QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm-ss");
				if(deviceModelsJsonFile.rename(newName))
				{
					qWarning() << "TelescopeControl: The existing version of device_models.json is obsolete. Backing it up as" << newName;
					if(!restoreDeviceModelsListTo(deviceModelsJsonPath))
					{
						useDefaultList = true;
					}
				}
				else
				{
					qWarning() << "TelescopeControl: The existing version of device_models.json is obsolete. Unable to rename.";
					useDefaultList = true;
				}
			}
			else
				deviceModelsJsonFile.close();
		}
	}

	if (useDefaultList)
	{
		qWarning() << "TelescopeControl: Using embedded device models list.";
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
		qWarning() << "TelescopeControl: Only embedded telescope servers are available.";
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
			qWarning() << "TelescopeControl: Skipping device model: Duplicate name:" << name;
			continue;
		}

		//Telescope server
		QString server = model.value("server").toString();
		//The default behaviour is to use embedded servers:
		bool useExecutable = false;
		if(server.isEmpty())
		{
			qWarning() << "TelescopeControl: Skipping device model: No server specified for" << name;
			continue;
		}
		if(useServerExecutables)
		{
			if(telescopeServers.contains(server))
			{
				qDebug() << "TelescopeControl: Using telescope server executable for" << name;
				useExecutable = true;
			}
			else if(EMBEDDED_TELESCOPE_SERVERS.contains(server))
			{
				qWarning() << "TelescopeControl: No external telescope server executable found for" << name;
				qWarning() << "TelescopeControl: Using embedded telescope server" << server << "for" << name;
				useExecutable = false;
			}
			else
			{
				qWarning() << "TelescopeControl: Skipping device model: No server" << server << "found for" << name;
				continue;
			}
		}
		else
		{
			if(!EMBEDDED_TELESCOPE_SERVERS.contains(server))
			{
				qWarning() << "TelescopeControl: Skipping device model: No server" << server << "found for" << name;
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
		//qDebug() << "TelescopeControl: Adding device model:" << name << description << server << delay;
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

	foreach (const int slotNumber, telescopeClients.keys())
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
		qWarning() << "TelescopeControl: Unable to copy the default device models list to" << deviceModelsListPath;
		return false;
	}
	QFile newCopy(deviceModelsListPath);
	newCopy.setPermissions(newCopy.permissions() | QFile::WriteOwner);

	qDebug() << "TelescopeControl: The default device models list has been copied to" << deviceModelsListPath;
	return true;
}

const QString& TelescopeControl::getServerExecutablesDirectoryPath()
{
	return serverExecutablesDirectoryPath;
}

bool TelescopeControl::setServerExecutablesDirectoryPath(const QString& newPath)
{
	//TODO: Reuse code.
	QDir newServerDirectory(newPath);
	if(!newServerDirectory.exists())
	{
		qWarning() << "TelescopeControl: Can't find such a directory:" << newPath;
		return false;
	}
	QList<QFileInfo> telescopeServerExecutables = newServerDirectory.entryInfoList(QStringList("TelescopeServer*"), (QDir::Files|QDir::Executable|QDir::CaseSensitive), QDir::Name);
	if(telescopeServerExecutables.isEmpty())
	{
		qWarning() << "TelescopeControl: No telescope server executables found in"
							 << serverExecutablesDirectoryPath;
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
			qWarning() << "TelescopeControl: Unable to create a log file for slot"
								 << slot << ":" << filePath;
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


void TelescopeControl::translateActionDescriptions()
{
	StelShortcutMgr* shMgr = StelApp::getInstance().getStelShortcutManager();
	if (!shMgr)
		return;
	
	for (int i = MIN_SLOT_NUMBER; i <= MAX_SLOT_NUMBER; i++)
	{
		QString name = moveToSelectedActionId.arg(i);
		QString description = q_("Move telescope #%1 to selected object")
		                      .arg(i);
		shMgr->setShortcutText(name, actionGroupId, description);
		
		name = moveToCenterActionId.arg(i);
		description = q_("Move telescope #%1 to the point currently in the center of the screen").arg(i);
		shMgr->setShortcutText(name, actionGroupId, description);
	}
}
