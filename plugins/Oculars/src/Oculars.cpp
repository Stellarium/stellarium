/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Oculars.hpp"

#include "GridLinesMgr.hpp"
#include "LabelMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMovementMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelMainWindow.hpp"
#include "StelTranslator.hpp"

#include <QAction>
#include <QKeyEvent>
#include <QDebug>
#include <QMouseEvent>
#include <QtNetwork>
#include <QPixmap>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlTableModel>

#include <cmath>

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>	/* Header File For The GLU Library */
#else
#include <GL/glu.h>	/* Header File For The GLU Library */
#endif


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModuleMgr Methods
#endif
/* ********************************************************************* */
//! This method is the one called automatically by the StelModuleMgr just
//! after loading the dynamic library
StelModule* OcularsStelPluginInterface::getStelModule() const
{
	return new Oculars();
}

StelPluginInfo OcularsStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(Oculars);

	StelPluginInfo info;
	info.id = "Oculars";
	info.displayedName = "Ocular";
	info.authors = "Timothy Reaves";
	info.contact = "treaves@silverfieldstech.com";
	info.description = "Shows the sky as if looking through a telescope eyepiece";
	return info;
}

Q_EXPORT_PLUGIN2(Oculars, OcularsStelPluginInterface)


/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Instance Methods
#endif
/* ********************************************************************* */
Oculars::Oculars() : selectedOcularIndex(-1), flagShowOculars(false), usageMessageLabelID(-1),
				   pxmapGlow(NULL), pxmapOnIcon(NULL), pxmapOffIcon(NULL), toolbarButton(NULL)
{
	flagShowOculars = false;
	flagShowCrosshairs = false;
	font.setPixelSize(14);
	maxImageCircle = 0.0;
	oculars = QList<Ocular *>();
	ready = true;
	selectedOcularIndex = 0;
	selectedTelescopeIndex = 0;
	setObjectName("Oculars");
	telescopes = QList<Telescope *>();
	useMaxImageCircle = true;
	visible = false;
}

Oculars::~Oculars()
{
	delete ocularsTableModel;
	ocularsTableModel = NULL;
	delete telescopesTableModel;
	telescopesTableModel = NULL;
	delete ocularDialog;
	ocularDialog = NULL;
	delete normalStyleSheet;
	delete nightStyleSheet;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark StelModule Methods
#endif
/* ********************************************************************* */
bool Oculars::configureGui(bool show)
{
	if (show)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		gui->getGuiActions("actionShow_Ocular_Window")->setChecked(true);
	}
	
	return true;
}

//! Draw any parts on the screen which are for our module
void Oculars::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	StelPainter painter(prj);
	painter.setFont(font);

	// Insure there is a selected ocular & telescope
	if (selectedOcularIndex > oculars.count()) {
		qWarning() << "Oculars: the selected ocular index of " << selectedOcularIndex << " is greate that the ocular count of "
		<< oculars.count() << ". Module disable.";
		ready = false;
	}
	if (selectedTelescopeIndex > telescopes.count()) {
		qWarning() << "Oculars: the selected telescope index of " << selectedTelescopeIndex << " is greate that the telescope count of "
		<< telescopes.count() << ". Module disable.";
		ready = false;
	}

	if (ready && flagShowOculars) {
		paintMask();
		if (flagShowCrosshairs)  {
			drawCrosshairs();
		}
		Ocular *ocular = oculars[selectedOcularIndex];
		Telescope *telescope = telescopes[selectedTelescopeIndex];
		QString widthString = "MMMMMMMMMMMMMMMMMMM";
		QString string1 = "Ocular #" + QVariant(selectedOcularIndex).toString();
		if (ocular->getName() != QString(""))  {
			string1.append(" : ").append(ocular->getName());
		}
		QString string2 = "Ocular aFOV: " + QVariant(ocular->getAppearentFOV()).toString() + QChar(0x00B0);
		QString string4 = "Telescope #" + QVariant(selectedTelescopeIndex).toString();
		if (telescope->getName() != QString(""))  {
			string4.append(" : ").append(telescope->getName());
		}
		QString string5 = "Magnification: " + QVariant(((int)(ocular->getMagnification(telescope) * 10.0)) / 10.0).toString() + "x";
		QString string6 = "Image Circle: " + QVariant(((int)(ocular->getExitCircle(telescope) * 10.0)) / 10.0).toString() + "mm";
		QString string7 = "FOV: " + QVariant(((int)(ocular->getAcutalFOV(telescope) * 10000.00)) / 10000.0).toString() + QChar(0x00B0);

		float insetFromRHS = painter.getFontMetrics().width(widthString);

		StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
		int xPosition = projectorParams.viewportXywh[2];
		xPosition -= insetFromRHS;
		int yPosition = projectorParams.viewportXywh[3];
		yPosition -= 40;

		painter.setColor(0.8, 0.48, 0.0, 1);
		glDisable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		const int lineHeight = painter.getFontMetrics().height();
		painter.drawText(xPosition, yPosition, string1);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, string2);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, string4);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, string5);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, string6);
		yPosition-=lineHeight;
		painter.drawText(xPosition, yPosition, string7);
	}
	newIntrument = false; // Now that it's been drawn once
}

//! Determine which "layer" the plagin's drawing will happen on.
double Oculars::getCallOrder(StelModuleActionName actionName) const
{
	// TODO; this really doesn't seem to have any effect.  I've tried everything from -100 to +100,
	//		and a calculated value.  It all seems to work the same regardless.
	double order = 1000.0;
	if (actionName==StelModule::ActionHandleKeys || actionName==StelModule::ActionHandleMouseMoves)
	{
		order = StelApp::getInstance().getModuleMgr().getModule("StelMovementMgr")->getCallOrder(actionName) - 1.0;
	}
	else if (actionName==StelModule::ActionDraw)
	{
		order = GETSTELMODULE(LabelMgr)->getCallOrder(actionName) + 100.0;
	}

	return order;
}

void Oculars::handleMouseClicks(class QMouseEvent* event)
{
	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr::StelMovementMgr *movementManager = core->getMovementMgr();
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()){
		LabelMgr *labelManager = GETSTELMODULE(LabelMgr);

		if (flagShowOculars)
		{
			// center the selected object in the ocular, and track.
			movementManager->setFlagTracking(true);
		}
		else
		{
			// remove the usage label if it is being displayed.
			if (usageMessageLabelID > -1)
			{
				labelManager->setLabelShow(usageMessageLabelID, false);
				labelManager->deleteLabel(usageMessageLabelID);
				usageMessageLabelID = -1;
			}
		}
	}
	else if(flagShowOculars)
	{
		// The ocular is displayed, but no object is selected.  So don't track the stars.
		movementManager->setFlagLockEquPos(false);
	}
	event->setAccepted(false);
}

bool Oculars::handleMouseMoves(int x, int y, Qt::MouseButtons b)
{
	// Allow it to go up the stack
	return false;
}

void Oculars::init()
{
	qDebug() << "Ocular plugin - press Command-o to toggle eyepiece view mode. Press ALT-o for configuration.";

	// Load settings from ocular.ini
	validateIniFile();
	if (initializeDB()) {
		ready = true;
		ocularDialog = new OcularDialog(ocularsTableModel, telescopesTableModel);
		initializeActions();
	}
	try {
		StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
		QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";

		QSettings settings(ocularIniPath, QSettings::IniFormat);
		useMaxImageCircle = settings.value("use_max_exit_circle", 0.0).toBool();
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable to locate ocular.ini file or create a default one for Ocular plugin: " << e.what();
	}
	
	//Load the module's custom style sheets
	QFile styleSheetFile;
	styleSheetFile.setFileName(":/ocular/normalStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		normalStyleSheet = new QByteArray(styleSheetFile.readAll());
	}
	styleSheetFile.close();
	styleSheetFile.setFileName(":/ocular/nightStyle.css");
	if(styleSheetFile.open(QFile::ReadOnly|QFile::Text))
	{
		nightStyleSheet = new QByteArray(styleSheetFile.readAll());
	}
	styleSheetFile.close();
}

void Oculars::deinit()
{
	QSqlDatabase db = QSqlDatabase::database("oculars");
	db.close();
	QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

void Oculars::setStelStyle(const StelStyle& style)
{
	ocularDialog->setStelStyle(style);
}

const StelStyle Oculars::getModuleStyleSheet(const StelStyle& style)
{
	StelStyle pluginStyle(style);
	if (style.confSectionName == "color")
	{
		pluginStyle.qtStyleSheet.append(*normalStyleSheet);
	}
	else
	{
		pluginStyle.qtStyleSheet.append(*nightStyleSheet);
	}
	return pluginStyle;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private slots Methods
#endif
/* ********************************************************************* */
void Oculars::determineMaxImageCircle()
{
	if (ready) {
		QListIterator<Ocular *> ocularIterator(oculars);
		while (ocularIterator.hasNext()) {
			Ocular *ocular = ocularIterator.next();

			QListIterator<Telescope *> telescopeIterator(telescopes);
			while (telescopeIterator.hasNext()) {
				Telescope *telescope = telescopeIterator.next();

				if (ocular->getExitCircle(telescope) > maxImageCircle) {
					maxImageCircle = ocular->getExitCircle(telescope);
				}
			}
		}
	}
	// insure it is not zero
	if (maxImageCircle == 0.0) {
		maxImageCircle = 1.0;
	}
}

void Oculars::instrumentChanged()
{
	newIntrument = true;
	zoom(true);
}

void Oculars::loadOculars()
{
	oculars.clear();
	int rowCount = ocularsTableModel->rowCount();
	for (int row = 0; row < rowCount; row++) {
		oculars.append(new Ocular(ocularsTableModel->record(row)));
	}
	if (oculars.size() == 0) {
		ready = false;
		qWarning() << "WARNING: no oculars found.  Feature will be disabled.";
	}

}

void Oculars::loadTelescopes()
{
	telescopes.clear();
	int rowCount = telescopesTableModel->rowCount();
	for (int row = 0; row < rowCount; row++) {
		telescopes.append(new Telescope(telescopesTableModel->record(row)));
	}
	if (telescopes.size() == 0) {
		ready = false;
		qWarning() << "WARNING: no telescopes found.  Feature will be disabled.";
	}

}

void Oculars::setScaleImageCircle(bool state)
{
	if (state) {
		determineMaxImageCircle();
	}
	useMaxImageCircle = state;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Slots Methods
#endif
/* ********************************************************************* */
void Oculars::enableOcular(bool b)
{
	if (!ready) {
		qDebug() << "The Oculars module has been disabled.";
		return;
	}

	if (b) {
		loadDatabaseObjects();
	}

	StelCore::StelCore *core = StelApp::getInstance().getCore();
	LabelMgr* labelManager = GETSTELMODULE(LabelMgr);
	// Toggle the plugin on & off.  To toggle on, we want to ensure there is a selected object.
	if (!flagShowOculars && !StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		if (usageMessageLabelID == -1) {
			QFontMetrics metrics(font);
			QString labelText = "Please select an object before enabling Ocular.";
			StelProjector::StelProjectorParams projectorParams = core->getCurrentStelProjectorParams();
			int xPosition = projectorParams.viewportCenter[0];
			xPosition = xPosition - 0.5 * (metrics.width(labelText));
			int yPosition = projectorParams.viewportCenter[1];
			yPosition = yPosition - 0.5 * (metrics.height());
			usageMessageLabelID = labelManager->labelScreen(labelText, xPosition, yPosition, true, font.pixelSize(), "#99FF99");
		}
		// we didn't accept the new status - make sure the toolbar button reflects this
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		gui->getGuiActions("actionShow_Ocular")->setChecked(false);
	} else {
		if (selectedOcularIndex != -1) {
			// remove the usage label if it is being displayed.
			if (usageMessageLabelID > -1) {
				labelManager->setLabelShow(usageMessageLabelID, false);
				labelManager->deleteLabel(usageMessageLabelID);
				usageMessageLabelID = -1;
			}
			flagShowOculars = b;
			zoom(false);
		}
	}
}

void Oculars::decrementOcularIndex()
{
	selectedOcularIndex--;
	if (selectedOcularIndex == -1) {
		selectedOcularIndex = oculars.count() - 1;
	}
	emit(selectedOcularChanged());
}

void Oculars::decrementTelescopeIndex()
{
	selectedTelescopeIndex--;
	if (selectedTelescopeIndex == -1) {
		selectedTelescopeIndex = telescopes.count() - 1;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::incrementOcularIndex()
{
	selectedOcularIndex++;
	if (selectedOcularIndex == oculars.count()) {
		selectedOcularIndex = 0;
	}
	emit(selectedOcularChanged());
}

void Oculars::incrementTelescopeIndex()
{
	selectedTelescopeIndex++;
	if (selectedTelescopeIndex == telescopes.count()) {
		selectedTelescopeIndex = 0;
	}
	emit(selectedTelescopeChanged());
}

void Oculars::toggleCrosshair()
{
	flagShowCrosshairs = !flagShowCrosshairs;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Private Methods
#endif
/* ********************************************************************* */
void Oculars::drawCrosshairs()
{
	const StelProjectorP projector = StelApp::getInstance().getCore()->getProjection(StelCore::FrameEquinoxEqu);
	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();
	// Center of screen
	Vec2i centerScreen(projector->getViewportPosX()+projector->getViewportWidth()/2,
					   projector->getViewportPosY()+projector->getViewportHeight()/2);
	GLdouble length = 0.5 * params.viewportFovDiameter;
	// See if we need to scale the length
	if (useMaxImageCircle && oculars[selectedOcularIndex]->getExitCircle(telescopes[selectedTelescopeIndex]) > 0.0) {
		length = oculars[selectedOcularIndex]->getExitCircle(telescopes[selectedTelescopeIndex]) * length / maxImageCircle;
	}

	// Draw the lines
	StelPainter painter(projector);
	painter.setColor(0.77, 0.14, 0.16, 1);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] + length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0], centerScreen[1] - length);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] + length, centerScreen[1]);
	painter.drawLine2d(centerScreen[0], centerScreen[1], centerScreen[0] - length, centerScreen[1]);
}

void Oculars::initializeActions()
{
	QString group = "Oculars Plugin";
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);

	gui->addGuiActions("actionShow_Ocular", N_("Enable ocular"), "Ctrl+O", "Plugin Key Bindings", true);
	gui->getGuiActions("actionShow_Ocular")->setChecked(flagShowOculars);
	gui->addGuiActions("actionShow_Ocular_Crosshair", N_("Toggle Crosshair"), "ALT+C", group, true);
	gui->addGuiActions("actionShow_Ocular_Window", N_("Configuration Window"), "ALT+O", group, true);

	gui->addGuiActions("actionShow_Ocular_increment", N_("Select next ocular"), "Ctrl+]", group, false);
	gui->addGuiActions("actionShow_Ocular_decrement", N_("Select previous ocular"), "Ctrl+[", group, false);
	gui->addGuiActions("actionShow_Telescope_increment", N_("Select next telescope"), "Shift+]", group, false);
	gui->addGuiActions("actionShow_Telescope_decrement", N_("Select previous telescope"), "Shift+[", group, false);

	connect(gui->getGuiActions("actionShow_Ocular"), SIGNAL(toggled(bool)), this, SLOT(enableOcular(bool)));
	connect(gui->getGuiActions("actionShow_Ocular_Crosshair"), SIGNAL(toggled(bool)), this, SLOT(toggleCrosshair()));
	connect(gui->getGuiActions("actionShow_Ocular_Window"), SIGNAL(toggled(bool)), ocularDialog, SLOT(setVisible(bool)));
	connect(ocularDialog, SIGNAL(visibleChanged(bool)), gui->getGuiActions("actionShow_Ocular_Window"), SLOT(setChecked(bool)));

	connect(gui->getGuiActions("actionShow_Ocular_increment"), SIGNAL(triggered()), this, SLOT(incrementOcularIndex()));
	connect(gui->getGuiActions("actionShow_Ocular_decrement"), SIGNAL(triggered()), this, SLOT(decrementOcularIndex()));
	connect(gui->getGuiActions("actionShow_Telescope_increment"), SIGNAL(triggered()), this, SLOT(incrementTelescopeIndex()));
	connect(gui->getGuiActions("actionShow_Telescope_decrement"), SIGNAL(triggered()), this, SLOT(decrementTelescopeIndex()));
	/*
	 connect(telescopesTableModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged()));
	 connect(telescopesTableModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted()));
	 connect(telescopesTableModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsRemoved()));
	 */
	connect(this, SIGNAL(selectedOcularChanged()), this, SLOT(instrumentChanged()));
	connect(this, SIGNAL(selectedTelescopeChanged()), this, SLOT(instrumentChanged()));
	connect(ocularDialog, SIGNAL(scaleImageCircleChanged(bool)), this, SLOT(setScaleImageCircle(bool)));


	// Make a toolbar button
	try {
		pxmapGlow = new QPixmap(":/graphicGui/gui/glow32x32.png");
		pxmapOnIcon = new QPixmap(":/ocular/bt_ocular_on.png");
		pxmapOffIcon = new QPixmap(":/ocular/bt_ocular_off.png");

		toolbarButton = new StelButton(NULL,
									   *pxmapOnIcon,
									   *pxmapOffIcon,
									   *pxmapGlow,
									   gui->getGuiActions("actionShow_Ocular"));
		gui->getButtonBar()->addButton(toolbarButton, "065-pluginsGroup");
	} catch (std::runtime_error& e) {
		qWarning() << "WARNING: unable create toolbar button for Oculars plugin: " << e.what();
	}

}

bool Oculars::initializeDB()
{
	bool result = false;
	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString dbPath = StelFileMgr::findFile("modules/Oculars/", flags);
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "oculars");
	db.setDatabaseName(dbPath + "oculars.sqlite");
	if (db.open()) {
		qDebug() << "Oculars opened the DB successfully.";
		// See if the tables alreadt exist.
		QStringList tableList = db.tables();
		if (!tableList.contains("oculars")) {
			QSqlQuery query = QSqlQuery(db);
			query.exec("create table oculars (id INTEGER PRIMARY KEY, name VARCHAR, afov FLOAT, efl FLOAT, fieldStop FLOAT)");
			query.exec("INSERT INTO oculars (name, afov, efl, fieldStop) VALUES ('one', 43, 40, 0)");
			query.exec("INSERT INTO oculars (name, afov, efl, fieldStop) VALUES ('two', 82, 31, 0)");
			query.exec("INSERT INTO oculars (name, afov, efl, fieldStop) VALUES ('three', 52, 10.5, 0)");
			query.exec("INSERT INTO oculars (name, afov, efl, fieldStop) VALUES ('four', 52, 26, 0)");
			query.exec("INSERT INTO oculars (name, afov, efl, fieldStop) VALUES ('five', 82, 20, 0)");

		}
		if (!tableList.contains("telescopes")) {
			QSqlQuery query = QSqlQuery(db);
			query.exec("create table telescopes (id INTEGER PRIMARY KEY, name VARCHAR, focalLength FLOAT, diameter FLOAT, vFlip VARCHAR, hFlip VARCHAR)");
			query.exec("INSERT INTO telescopes (name, focalLength, diameter, vFlip, hFlip) VALUES ('C1400', 3190, 355.6, 'false', 'true')");
			query.exec("INSERT INTO telescopes (name, focalLength, diameter, vFlip, hFlip) VALUES ('80EDF', 500, 80, 'false', 'false')");
		}

		// Set the table models
		ocularsTableModel = new QSqlTableModel(0, db);
		ocularsTableModel->setTable("oculars");
		ocularsTableModel->select();
		telescopesTableModel = new QSqlTableModel(0, db);
		telescopesTableModel->setTable("telescopes");
		telescopesTableModel->select();

		result = true;
	} else {
		qDebug() << "Oculars could not open its databse; disableing module.";
		result = false;
	}
	return result;
}

void Oculars::interceptMovementKey(QKeyEvent* event)
{
	// We onle care about the arrow keys.  This flag tracks that.
	bool consumeEvent = false;

	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr::StelMovementMgr *movementManager = core->getMovementMgr();

	if (event->type() == QEvent::KeyPress)
	{
		// Direction and zoom deplacements
		switch (event->key())
		{
			case Qt::Key_Left:
				movementManager->turnLeft(true);
				consumeEvent = true;
				break;
			case Qt::Key_Right:
				movementManager->turnRight(true);
				consumeEvent = true;
				break;
			case Qt::Key_Up:
				if (!event->modifiers().testFlag(Qt::ControlModifier))
				{
					movementManager->turnUp(true);
				}
				consumeEvent = true;
				break;
			case Qt::Key_Down:
				if (!event->modifiers().testFlag(Qt::ControlModifier))
				{
					movementManager->turnDown(true);
				}
				consumeEvent = true;
				break;
			case Qt::Key_PageUp:
				movementManager->zoomIn(true);
				consumeEvent = true;
				break;
			case Qt::Key_PageDown:
				movementManager->zoomOut(true);
				consumeEvent = true;
				break;
			case Qt::Key_Shift:
				movementManager->moveSlow(true);
				consumeEvent = true;
				break;
		}
	}
	else
	{
		// When a deplacement key is released stop mooving
		switch (event->key())
		{
			case Qt::Key_Left:
				movementManager->turnLeft(false);
				consumeEvent = true;
				break;
			case Qt::Key_Right:
				movementManager->turnRight(false);
				consumeEvent = true;
				break;
			case Qt::Key_Up:
				movementManager->turnUp(false);
				consumeEvent = true;
				break;
			case Qt::Key_Down:
				movementManager->turnDown(false);
				consumeEvent = true;
				break;
			case Qt::Key_PageUp:
				movementManager->zoomIn(false);
				consumeEvent = true;
				break;
			case Qt::Key_PageDown:
				movementManager->zoomOut(false);
				consumeEvent = true;
				break;
			case Qt::Key_Shift:
				movementManager->moveSlow(false);
				consumeEvent = true;
				break;
		}
		if (consumeEvent)
		{
			// We don't want to re-center the object; just hold the current position.
			movementManager->setFlagLockEquPos(true);
		}
	}
	if (consumeEvent)
	{
		event->accept();
	}
	else
	{
		event->setAccepted(false);
	}
}

void Oculars::loadDatabaseObjects()
{
	loadOculars();
	loadTelescopes();
	if (useMaxImageCircle) {
		determineMaxImageCircle();
	}
}

void Oculars::paintMask()
{
	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelProjector::StelProjectorParams params = core->getCurrentStelProjectorParams();

	glDisable(GL_BLEND);
	glColor3f(0.f,0.f,0.f);
	glPushMatrix();
	glTranslated(params.viewportCenter[0], params.viewportCenter[1], 0.0);
	GLUquadricObj *quadric = gluNewQuadric();

	GLdouble inner = 0.5 * params.viewportFovDiameter;

	// See if we need to scale the mask
	if (useMaxImageCircle && oculars[selectedOcularIndex]->getExitCircle(telescopes[selectedTelescopeIndex]) > 0.0) {
		inner = oculars[selectedOcularIndex]->getExitCircle(telescopes[selectedTelescopeIndex]) * inner / maxImageCircle;
	}

	GLdouble outer = params.viewportXywh[2] + params.viewportXywh[3];
	// Draw the mask
	gluDisk(quadric, inner, outer, 256, 1);
	// the gray circle
	glColor3f(0.15f,0.15f,0.15f);
	gluDisk(quadric, inner - 1.0, inner, 256, 1);
	gluDeleteQuadric(quadric);
	glPopMatrix();
}

void Oculars::validateIniFile()
{
	// Insure the module directory exists
	StelFileMgr::makeSureDirExistsAndIsWritable(StelFileMgr::getUserDir()+"/modules/Oculars");

	StelFileMgr::Flags flags = (StelFileMgr::Flags)(StelFileMgr::Directory|StelFileMgr::Writable);
	QString ocularIniPath = StelFileMgr::findFile("modules/Oculars/", flags) + "ocular.ini";

	// If the ini file does not already exist, create it from the resource in the QT resource
	if(!QFileInfo(ocularIniPath).exists()) {
		QFile src(":/ocular/default_ocular.ini");
		if (!src.copy(ocularIniPath)) {
			qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " + ocularIniPath;
		} else {
			qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << ocularIniPath;
			// The resource is read only, and the new file inherits this, so set write-able.
			QFile dest(ocularIniPath);
			dest.setPermissions(dest.permissions() | QFile::WriteOwner);
		}
	} else {
		qDebug() << "Oculars::validateIniFile ocular.ini exists at: " << ocularIniPath << ". Checking version...";
		QSettings settings(ocularIniPath, QSettings::IniFormat);
		double ocularsVersion = settings.value("oculars_version", "0.0").toDouble();
		qWarning() << "Oculars::validateIniFile found existing ini file version " << ocularsVersion;

		if (ocularsVersion < MIN_OCULARS_INI_VERSION) {
			qWarning() << "Oculars::validateIniFile existing ini file version " << ocularsVersion
						<< " to old to use; required version is " << MIN_OCULARS_INI_VERSION << ". Coping over new one.";
			// delete last "old" file, if it exists
			QFile deleteFile(ocularIniPath + ".old");
			deleteFile.remove();

			// Rename the old one, and copy over a new one
			QFile oldFile(ocularIniPath);
			if (!oldFile.rename(ocularIniPath + ".old")) {
				qWarning() << "Oculars::validateIniFile cannot move ocular.ini resource to ocular.ini.old at path  " + ocularIniPath;
			} else {
				qWarning() << "Oculars::validateIniFile ocular.ini resource renamed to ocular.ini.old at path  " + ocularIniPath;
				QFile src(":/ocular/default_ocular.ini");
				if (!src.copy(ocularIniPath)) {
					qWarning() << "Oculars::validateIniFile cannot copy default_ocular.ini resource to [non-existing] " + ocularIniPath;
				} else {
					qDebug() << "Oculars::validateIniFile copied default_ocular.ini to " << ocularIniPath;
					// The resource is read only, and the new file inherits this...  make sure the new file
					// is writable by the Stellarium process so that updates can be done.
					QFile dest(ocularIniPath);
					dest.setPermissions(dest.permissions() | QFile::WriteOwner);
				}
			}
		}
	}
}

void Oculars::unzoomOcular()
{
	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr::StelMovementMgr *movementManager = core->getMovementMgr();
	GridLinesMgr::GridLinesMgr *gridManager = (GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");

	gridManager->setFlagAzimuthalGrid(flagAzimuthalGrid);
	gridManager->setFlagEquatorGrid(flagEquatorGrid);
	gridManager->setFlagEquatorJ2000Grid(flagEquatorJ2000Grid);
	gridManager->setFlagEquatorLine(flagEquatorLine);
	gridManager->setFlagEclipticLine(flagEclipticLine);
	gridManager->setFlagMeridianLine(flagMeridianLine);
	movementManager->setFlagTracking(false);
	movementManager->setFlagEnableZoomKeys(true);
	movementManager->setFlagEnableMouseNavigation(true);

	// Set the screen display
	// core->setMaskType(StelProjector::MaskNone);
	core->setFlipHorz(false);
	core->setFlipVert(false);

	movementManager->zoomTo(movementManager->getInitFov());
}

void Oculars::zoom(bool rezoom)
{

	if (flagShowOculars)  {
		if (!rezoom)  {
			GridLinesMgr::GridLinesMgr *gridManager = (GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");
			// Current state
			flagAzimuthalGrid = gridManager->getFlagAzimuthalGrid();
			flagEquatorGrid = gridManager->getFlagEquatorGrid();
			flagEquatorJ2000Grid = gridManager->getFlagEquatorJ2000Grid();
			flagEquatorLine = gridManager->getFlagEquatorLine();
			flagEclipticLine = gridManager->getFlagEclipticLine();
			flagMeridianLine = gridManager->getFlagMeridianLine();
		}

		// set new state
		zoomOcular();
	} else {
		//reset to original state
		unzoomOcular();
	}
}

void Oculars::zoomOcular()
{
	StelCore::StelCore *core = StelApp::getInstance().getCore();
	StelMovementMgr::StelMovementMgr *movementManager = core->getMovementMgr();
	GridLinesMgr::GridLinesMgr *gridManager =
		(GridLinesMgr *)StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr");

	gridManager->setFlagAzimuthalGrid(false);
	gridManager->setFlagEquatorGrid(false);
	gridManager->setFlagEquatorJ2000Grid(false);
	gridManager->setFlagEquatorLine(false);
	gridManager->setFlagEclipticLine(false);
	gridManager->setFlagMeridianLine(false);
	movementManager->setFlagTracking(true);
	movementManager->setFlagEnableZoomKeys(false);
	movementManager->setFlagEnableMouseNavigation(false);
	// We won't always have a selected object
	if (StelApp::getInstance().getStelObjectMgr().getWasSelected()) {
		movementManager->
			moveToJ2000(StelApp::getInstance().getStelObjectMgr().getSelectedObject()[0]->getEquinoxEquatorialPos(core->getNavigator()),
						0.5,
						1);
	}

	// Set the screen display
	// core->setMaskType(StelProjector::MaskDisk);
	Ocular *ocular = oculars[selectedOcularIndex];
	Telescope *telescope = telescopes[selectedTelescopeIndex];
	core->setFlipHorz(telescope->isHFlipped());
	core->setFlipVert(telescope->isVFlipped());

	double actualFOV = ocular->getAcutalFOV(telescope);
	// See if the mask was scaled
	if (maxImageCircle > 0.0 && ocular->getExitCircle(telescope) > 0.0) {
		actualFOV = maxImageCircle * actualFOV / ocular->getExitCircle(telescope);
	}
	movementManager->zoomTo(actualFOV, 0.0);
}

