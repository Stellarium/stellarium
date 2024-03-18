/*
 * Stellarium
 * Copyright (C) 2017 Alexander Wolf
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


#include "AsterismMgr.hpp"
#include "Asterism.hpp"
#include "StarMgr.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelObjectMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelSkyCultureMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "Planet.hpp"
#include "StelUtils.hpp"

#include <vector>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QDir>

// constructor which loads all data from appropriate files
AsterismMgr::AsterismMgr(StarMgr *_hip_stars)
	: hipStarMgr(_hip_stars)
	, linesDisplayed(false)
	, rayHelpersDisplayed(false)
	, namesDisplayed(false)
	, hasAsterism(false)
	, isolateAsterismSelected(false)
	, asterismLineThickness(1)
	, rayHelperThickness(1)	
{
	setObjectName("AsterismMgr");
	Q_ASSERT(hipStarMgr);
}

AsterismMgr::~AsterismMgr()
{
	for (auto* asterism : asterisms)
	{
		delete asterism;
	}
}

void AsterismMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	lastLoadedSkyCulture = "dummy";
	asterFont.setPixelSize(conf->value("viewing/asterism_font_size", 14).toInt());
	setFlagLines(conf->value("viewing/flag_asterism_drawing").toBool());
	setFlagRayHelpers(conf->value("viewing/flag_rayhelper_drawing").toBool());
	setFlagLabels(conf->value("viewing/flag_asterism_name").toBool());
	setAsterismLineThickness(conf->value("viewing/asterism_line_thickness", 1).toInt());
	setRayHelperThickness(conf->value("viewing/rayhelper_line_thickness", 1).toInt());

	// Load colors from config file
	QString defaultColor = conf->value("color/default_color").toString();
	setLinesColor(Vec3f(conf->value("color/asterism_lines_color", defaultColor).toString()));
	setLabelsColor(Vec3f(conf->value("color/asterism_names_color", defaultColor).toString()));
	setRayHelpersColor(Vec3f(conf->value("color/rayhelper_lines_color", defaultColor).toString()));

	StelObjectMgr *objectManager = GETSTELMODULE(StelObjectMgr);
	objectManager->registerStelObjectMgr(this);
	connect(objectManager, SIGNAL(selectedObjectChanged(StelModule::StelModuleSelectAction)),
			this, SLOT(selectedObjectChange(StelModule::StelModuleSelectAction)));
	StelApp *app = &StelApp::getInstance();
	connect(app, SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	connect(&app->getSkyCultureMgr(), SIGNAL(currentSkyCultureChanged(QString)), this, SLOT(updateSkyCulture(const QString&)));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_Asterism_Lines", displayGroup, N_("Asterism lines"), "linesDisplayed", "Alt+A");	
	addAction("actionShow_Asterism_Labels", displayGroup, N_("Asterism labels"), "namesDisplayed", "Alt+V");
	addAction("actionShow_Asterism_Isolated", displayGroup, N_("Toggle single asterism selection mode"), "switchSelectionMode()");
	addAction("actionShow_Ray_Helpers", displayGroup, N_("Ray helpers"), "rayHelpersDisplayed", "Alt+R");
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double AsterismMgr::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("GridLinesMgr")->getCallOrder(actionName)+11;
	return 0;
}

void AsterismMgr::updateSkyCulture(const QString& skyCultureDir)
{
	currentSkyCultureID = skyCultureDir;

	StelObjectMgr* objMgr = GETSTELMODULE(StelObjectMgr);
	const QList<StelObjectP> selectedObject = objMgr->getSelectedObject("Asterism");
	if (!selectedObject.isEmpty()) // Unselect asterism
		objMgr->unSelect();

	// Check if the sky culture changed since last load, if not don't load anything
	if (lastLoadedSkyCulture == skyCultureDir)
		return;

	QString fic = StelFileMgr::findFile("skycultures/"+skyCultureDir+"/asterism_lines.fab");
	if (fic.isEmpty())
	{
		hasAsterism = false;
		qWarning() << "No asterisms for skyculture" << currentSkyCultureID;
	}
	else
	{
		hasAsterism = true;
		loadLines(fic);
	}

	// load asterism names
	fic = StelFileMgr::findFile("skycultures/" + skyCultureDir + "/asterism_names.eng.fab");
	if (!fic.isEmpty())
		loadNames(fic);

	// Translate asterism names for the new sky culture
	updateI18n();

	lastLoadedSkyCulture = skyCultureDir;
}

void AsterismMgr::setLinesColor(const Vec3f& color)
{
	if (color != Asterism::lineColor)
	{
		Asterism::lineColor = color;
		emit linesColorChanged(color);
	}
}

Vec3f AsterismMgr::getLinesColor() const
{
	return Asterism::lineColor;
}

void AsterismMgr::setRayHelpersColor(const Vec3f& color)
{
	if (color != Asterism::rayHelperColor)
	{
		Asterism::rayHelperColor = color;
		emit rayHelpersColorChanged(color);
	}
}

Vec3f AsterismMgr::getRayHelpersColor() const
{
	return Asterism::rayHelperColor;
}


void AsterismMgr::setLabelsColor(const Vec3f& color)
{
	if (Asterism::labelColor != color)
	{
		Asterism::labelColor = color;
		emit namesColorChanged(color);
	}
}

Vec3f AsterismMgr::getLabelsColor() const
{
	return Asterism::labelColor;
}

void AsterismMgr::setFontSize(const float newFontSize)
{
	if ((static_cast<float>(asterFont.pixelSize()) - newFontSize) != 0.0f)
	{
		asterFont.setPixelSize(static_cast<int>(newFontSize));
		emit fontSizeChanged(newFontSize);
	}
}

float AsterismMgr::getFontSize() const
{
	return asterFont.pixelSize();
}

void AsterismMgr::setAsterismLineThickness(const int thickness)
{
	if(thickness!=asterismLineThickness)
	{
		asterismLineThickness = thickness;
		if (asterismLineThickness<=0) // The line can not be negative or zero thickness
			asterismLineThickness = 1;

		emit asterismLineThicknessChanged(thickness);
	}
}

void AsterismMgr::setRayHelperThickness(const int thickness)
{
	if(thickness!=rayHelperThickness)
	{
		rayHelperThickness = thickness;
		if (rayHelperThickness<=0) // The line can not be negative or zero thickness
			rayHelperThickness = 1;

		emit rayHelperThicknessChanged(thickness);
	}
}

void AsterismMgr::loadLines(const QString &fileName)
{
	QFile in(fileName);
	if (!in.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "Can't open asterism data file" << QDir::toNativeSeparators(fileName)  << "for culture" << currentSkyCultureID;
		Q_ASSERT(0);
	}

	int totalRecords=0;
	QString record;
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		if (!commentRx.match(record).hasMatch())
			totalRecords++;
	}
	in.seek(0);

	// delete existing data, if any
	for (auto* asterism : asterisms)
		delete asterism;

	asterisms.clear();
	Asterism *aster = Q_NULLPTR;

	// read the file of line patterns, adding a record per non-comment line
	int currentLineNumber = 0;	// line in file
	int readOk = 0;			// count of records processed OK
	while (!in.atEnd())
	{
		record = QString::fromUtf8(in.readLine());
		currentLineNumber++;
		if (commentRx.match(record).hasMatch())
			continue;

		aster = new Asterism;
		if(aster->read(record, hipStarMgr))
		{
			aster->setFlagLines(linesDisplayed);
			aster->setFlagLabels(namesDisplayed);
			aster->setFlagRayHelpers(rayHelpersDisplayed);
			asterisms.push_back(aster);
			++readOk;
		}
		else
		{
			qWarning() << "ERROR reading asterism lines record at line " << currentLineNumber << "for culture" << currentSkyCultureID;
			delete aster;
		}
	}
	in.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "asterism records successfully for culture" << currentSkyCultureID;

	// Set current states
	setFlagLines(linesDisplayed);
	setFlagLabels(namesDisplayed);
	setFlagRayHelpers(rayHelpersDisplayed);
}

void AsterismMgr::draw(StelCore* core)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameJ2000);
	StelPainter sPainter(prj);
	sPainter.setFont(asterFont);
	drawLines(sPainter, core);
	drawRayHelpers(sPainter, core);
	drawNames(sPainter);
}

// Draw asterisms lines
void AsterismMgr::drawLines(StelPainter& sPainter, const StelCore* core) const
{
	if (!hasAsterism)
		return;

	const float ppx = static_cast<float>(sPainter.getProjector()->getDevicePixelsPerPixel());
	sPainter.setBlending(true);
	if (asterismLineThickness>1 || ppx>1.f)
		sPainter.setLineWidth(asterismLineThickness*ppx); // set line thickness
	sPainter.setLineSmooth(true);

	const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();
	for (auto* asterism : asterisms)
	{
		if (asterism->isAsterism())
			asterism->drawOptim(sPainter, core, viewportHalfspace);
	}
	if (asterismLineThickness>1 || ppx>1.f)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw asterisms lines
void AsterismMgr::drawRayHelpers(StelPainter& sPainter, const StelCore* core) const
{
	if (!hasAsterism)
		return;

	const float ppx = static_cast<float>(sPainter.getProjector()->getDevicePixelsPerPixel());
	sPainter.setBlending(true);
	if (rayHelperThickness>1 || ppx>1.f)
		sPainter.setLineWidth(rayHelperThickness*ppx); // set line thickness
	sPainter.setLineSmooth(true);

	const SphericalCap& viewportHalfspace = sPainter.getProjector()->getBoundingCap();
	for (auto* asterism : asterisms)
	{
		if (!asterism->isAsterism())
			asterism->drawOptim(sPainter, core, viewportHalfspace);
	}
	if (rayHelperThickness>1 || ppx>1.f)
		sPainter.setLineWidth(1); // restore line thickness
	sPainter.setLineSmooth(false);
}

// Draw the names of all the asterisms
void AsterismMgr::drawNames(StelPainter& sPainter) const
{
	if (!hasAsterism)
		return;

	StelCore *core=StelApp::getInstance().getCore();
	Vec3d vel=core->getCurrentPlanet()->getHeliocentricEclipticVelocity();
	vel=StelCore::matVsop87ToJ2000*vel;
	vel*=core->getAberrationFactor() * (AU/(86400.0*SPEED_OF_LIGHT));

	sPainter.setBlending(true);
	for (auto* asterism : asterisms)
	{
		if (!asterism->flagAsterism) continue;
		// Check if in the field of view
		Vec3d XYZname=asterism->XYZname;
		if (core->getUseAberration())
		{
			XYZname.normalize();
			XYZname+=vel;
			XYZname.normalize();
		}

		if (sPainter.getProjector()->projectCheck(XYZname, asterism->XYname))
			asterism->drawName(sPainter);
	}
}

Asterism* AsterismMgr::findFromAbbreviation(const QString& abbreviation) const
{
	for (auto* asterism : asterisms)
	{
		if (asterism->abbreviation.compare(abbreviation, Qt::CaseInsensitive) == 0)
			return asterism;
	}
	return Q_NULLPTR;
}


// Can't find asterism from a position because it's not well localized
QList<StelObjectP> AsterismMgr::searchAround(const Vec3d&, double, const StelCore*) const
{
	return QList<StelObjectP>();
}

void AsterismMgr::loadNames(const QString& namesFile)
{
	// Asterism not loaded yet
	if (asterisms.empty()) return;

	// clear previous names
	for (auto* asterism : asterisms)
	{
		asterism->englishName.clear();
	}

	// Open file
	QFile commonNameFile(namesFile);
	if (!commonNameFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Cannot open file" << QDir::toNativeSeparators(namesFile);
		return;
	}

	// Now parse the file
	// lines to ignore which start with a # or are empty
	static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
	static const QRegularExpression recRx("^\\s*(\\S+)\\s+_[(]\"(.*)\"[)]\\s*([\\,\\d\\s]*)\\n");
	static const QRegularExpression ctxRx("(.*)\",\\s*\"(.*)");

	// keep track of how many records we processed.
	int totalRecords=0;
	int readOk=0;
	int lineNumber=0;
	while (!commonNameFile.atEnd())
	{
		QString record = QString::fromUtf8(commonNameFile.readLine());
		lineNumber++;

		// Skip comments
		if (commentRx.match(record).hasMatch())
			continue;

		totalRecords++;

		QRegularExpressionMatch recMatch=recRx.match(record);
		if (!recMatch.hasMatch())
		{
			qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in asterism names file" << QDir::toNativeSeparators(namesFile) << ":" << record;
		}
		else
		{
			QString shortName = recMatch.captured(1);
			Asterism *aster = findFromAbbreviation(shortName);
			// If the asterism exists, set the English name
			if (aster != Q_NULLPTR)
			{
				QString ctxt = recMatch.captured(2);
				QRegularExpressionMatch ctxMatch=ctxRx.match(ctxt);
				if (ctxMatch.hasMatch())
				{
					aster->englishName = ctxMatch.captured(1);
					aster->context = ctxMatch.captured(2);
				}
				else
				{
					aster->englishName = ctxt;
					aster->context = "";
				}
				readOk++;
			}
			else
			{
				qWarning() << "WARNING - asterism abbreviation" << shortName << "not found when loading asterism names";
			}
		}
	}
	commonNameFile.close();
	qDebug() << "Loaded" << readOk << "/" << totalRecords << "asterism names";
}

void AsterismMgr::updateI18n()
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyTranslator();
	for (auto* asterism : asterisms)
	{
		asterism->nameI18 = trans.qtranslate(asterism->englishName, asterism->context);
	}
}

// update faders
void AsterismMgr::update(double deltaTime)
{
	const int delta = static_cast<int>(deltaTime*1000);
	for (auto* asterism : asterisms)
	{
		asterism->update(delta);
	}
}

void AsterismMgr::setFlagLines(const bool displayed)
{
	if(linesDisplayed != displayed)
	{
		linesDisplayed = displayed;
		if (!selected.empty() && getFlagIsolateAsterismSelected())
		{
			for (auto* asterism : selected)
				asterism->setFlagLines(linesDisplayed);
		}
		else
		{
			for (auto* asterism : asterisms)
				asterism->setFlagLines(linesDisplayed);
		}
		emit linesDisplayedChanged(displayed);
	}
}

bool AsterismMgr::getFlagLines(void) const
{
	return linesDisplayed;
}

void AsterismMgr::setFlagRayHelpers(const bool displayed)
{
	if(rayHelpersDisplayed != displayed)
	{
		rayHelpersDisplayed = displayed;
		for (auto* asterism : asterisms)
		{
			asterism->setFlagRayHelpers(rayHelpersDisplayed);
		}
		emit rayHelpersDisplayedChanged(displayed);
	}
}

bool AsterismMgr::getFlagRayHelpers(void) const
{
	return rayHelpersDisplayed;
}

void AsterismMgr::setFlagLabels(const bool displayed)
{
	if (namesDisplayed != displayed)
	{
		namesDisplayed = displayed;
		if (!selected.empty() && getFlagIsolateAsterismSelected())
		{
			for (auto* asterism : selected)
				asterism->setFlagLabels(namesDisplayed);
		}
		else
		{
			for (auto* asterism : asterisms)
				asterism->setFlagLabels(namesDisplayed);
		}
		emit namesDisplayedChanged(displayed);
	}
}

bool AsterismMgr::getFlagLabels(void) const
{
	return namesDisplayed;
}

StelObjectP AsterismMgr::searchByNameI18n(const QString& nameI18n) const
{
	QString objw = nameI18n.toUpper();

	for (auto* asterism : asterisms)
	{
		QString objwcap = asterism->nameI18.toUpper();
		if (objwcap == objw) return asterism;
	}
	return Q_NULLPTR;
}

StelObjectP AsterismMgr::searchByName(const QString& name) const
{
	QString objw = name.toUpper();
	for (auto* asterism : asterisms)
	{
		QString objwcap = asterism->englishName.toUpper();
		if (objwcap == objw) return asterism;

		objwcap = asterism->abbreviation.toUpper();
		if (objwcap == objw) return asterism;
	}
	return Q_NULLPTR;
}

QStringList AsterismMgr::listAllObjects(bool inEnglish) const
{
	QStringList result;
	if (inEnglish)
	{
		for (auto* asterism : asterisms)
		{
			if (asterism->isAsterism())
				result << asterism->getEnglishName();
		}
	}
	else
	{
		for (auto* asterism : asterisms)
		{
			if (asterism->isAsterism())
				result << asterism->getNameI18n();
		}
	}
	return result;
}

StelObjectP AsterismMgr::searchByID(const QString &id) const
{
	for (auto* asterism : asterisms)
	{
		if (asterism->getID() == id) return asterism;
	}
	return Q_NULLPTR;
}

QString AsterismMgr::getStelObjectType() const
{
	return Asterism::ASTERISM_TYPE;
}

void AsterismMgr::setFlagIsolateAsterismSelected(const bool isolate)
{
	if (isolateAsterismSelected != isolate)
	{
		isolateAsterismSelected = isolate;

		// when turning off isolated selection mode, clear existing isolated selections.
		if (!isolateAsterismSelected)
		{
			for (auto* asterism : asterisms)
			{
				asterism->setFlagLines(getFlagLines());
				asterism->setFlagLabels(getFlagLabels());
			}
		}
		emit isolateAsterismSelectedChanged(isolate);
	}
}

bool AsterismMgr::getFlagIsolateAsterismSelected(void) const
{
	return isolateAsterismSelected;
}

void AsterismMgr::setSelectedAsterism(Asterism *a)
{
	// update states for other asterisms to fade them out
	if (a != Q_NULLPTR)
	{
		selected.push_back(a);

		if (getFlagIsolateAsterismSelected())
		{
			// Propagate current settings to newly selected asterism
			a->setFlagLines(getFlagLines());
			a->setFlagLabels(getFlagLabels());

			for (auto* asterism : asterisms)
			{
				bool match = false;
				for (auto* selected_asterisms : selected)
				{
					if (asterism == selected_asterisms)
					{
						match=true; // this is a selected asterism
						break;
					}
				}

				if(!match)
				{
					// Not selected asterism
					asterism->setFlagLines(false);
					asterism->setFlagLabels(false);
				}
			}
		}
		else
		{
			for (auto* asterism : asterisms)
			{
				asterism->setFlagLines(false);
				asterism->setFlagLabels(false);
			}

			// Propagate current settings to newly selected asterism
			a->setFlagLines(getFlagLines());
			a->setFlagLabels(getFlagLabels());
		}
	}
	else
	{
		if (selected.empty()) return;

		// Otherwise apply standard flags to all asterisms
		for (auto* asterism : asterisms)
		{
			asterism->setFlagLines(getFlagLines());
			asterism->setFlagLabels(getFlagLabels());
		}

		// And remove all selections
		selected.clear();
	}
}

//! Remove a asterism from the selected asterism list
void AsterismMgr::unsetSelectedAsterism(Asterism *a)
{
	if (a != Q_NULLPTR)
	{
		for (auto iter = selected.begin(); iter != selected.end();)
		{
			if( (*iter)->getEnglishName().toLower() == a->getEnglishName().toLower() )
				iter = selected.erase(iter);
			else
				++iter;
		}

		// If no longer any selection, restore all flags on all asterisms
		if (selected.empty())
		{
			// Otherwise apply standard flags to all asterisms
			for (auto* asterism : asterisms)
			{
				asterism->setFlagLines(getFlagLines());
				asterism->setFlagLabels(getFlagLabels());
			}
		}
		else if(isolateAsterismSelected)
		{
			// No longer selected asterism
			a->setFlagLines(false);
			a->setFlagLabels(false);
		}
	}
}

void AsterismMgr::selectAsterism(const QString &englishName)
{
	if (!getFlagIsolateAsterismSelected())
		setFlagIsolateAsterismSelected(true); // Enable isolated selection

	bool found = false;
	for (auto* asterism : asterisms)
	{
		if (asterism->getEnglishName().toLower()==englishName.toLower())
		{
			setSelectedAsterism(asterism);
			found = true;
			break;
		}
	}
	if (!found)
		qDebug() << "The asterism" << englishName << "is not found";
}

void AsterismMgr::deselectAsterism(const QString &englishName)
{
	if (!getFlagIsolateAsterismSelected())
		setFlagIsolateAsterismSelected(true); // Enable isolated selection

	bool found = false;
	for (auto* asterism : asterisms)
	{
		if (asterism->getEnglishName().toLower()==englishName.toLower())
		{
			unsetSelectedAsterism(asterism);
			found = true;
			break;
		}
	}

	if (selected.size()==0 && found)
	{
		// Let's remove the selection for all asterisms if the list of selected asterisms is empty
		for (auto* asterism : asterisms)
		{
			asterism->setFlagLines(false);
			asterism->setFlagLabels(false);
		}
	}

	if (!found)
		qDebug() << "The asterism" << englishName << "is not found";
}

void AsterismMgr::deselectAsterisms(void)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	if (getFlagIsolateAsterismSelected())
	{
		// The list of selected asterisms is empty, but...
		if (selected.size()==0)
		{
			// ...let's unselect all asterisms for guarantee
			for (auto* asterism : asterisms)
			{
				asterism->setFlagLines(false);
				asterism->setFlagLabels(false);
			}
		}

		// If any asterism is selected at the moment, then let's do not touch to it!
		if (omgr->getWasSelected() && !selected.empty())
			selected.pop_back();

		// Let's hide all previously selected asterisms
		for (auto* asterism : selected)
		{
			asterism->setFlagLines(false);
			asterism->setFlagLabels(false);
		}
	}
	else
	{
		const QList<StelObjectP> newSelectedConst = omgr->getSelectedObject("Asterism");
		if (!newSelectedConst.empty())
			omgr->unSelect();
	}
	selected.clear();
}

void AsterismMgr::selectAllAsterisms()
{
	for (auto* asterism : asterisms)
		setSelectedAsterism(asterism);
}

void AsterismMgr::selectedObjectChange(StelModule::StelModuleSelectAction action)
{
	StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	Q_ASSERT(omgr);
	const QList<StelObjectP> newSelected = omgr->getSelectedObject();
	if (newSelected.empty())
		return;

	const QList<StelObjectP> newSelectedAsterisms = omgr->getSelectedObject("Asterism");
	if (!newSelectedAsterisms.empty())
	{
		// If removing this selection
		if(action == StelModule::RemoveFromSelection)
			unsetSelectedAsterism(static_cast<Asterism *>(newSelectedAsterisms[0].data()));
		else // Add asterism to selected list (do not select a star, just the constellation)
			setSelectedAsterism(static_cast<Asterism *>(newSelectedAsterisms[0].data()));
	}
}

void AsterismMgr::switchSelectionMode()
{
	bool state = getFlagIsolateAsterismSelected();
	setFlagIsolateAsterismSelected(!state);
	if (!state)
		deselectAsterisms();
}
