/*
 * Solar System editor plug-in for Stellarium
 *
 * Copyright (C) 2010 Bogdan Marinov
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

#include "SolarSystemEditor.hpp"

#include "ManualImportWindow.hpp"
#include "ui_manualImportWindow.h"

#include <QColor>
#include <QColorDialog>
#include <QFileDialog>
#include <QImageReader>

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelMainView.hpp"
//#include "StelTranslator.hpp"


ManualImportWindow::ManualImportWindow(): StelDialog("SolarSystemEditorManualImport")
{
	ui = new Ui_manualImportWindow();
	ssoManager = GETSTELMODULE(SolarSystemEditor);
}

ManualImportWindow::~ManualImportWindow()
{
	delete ui;
}

void ManualImportWindow::createDialogContent()
{
	ui->setupUi(dialog);

	//Signals
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),
	        this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->LocationBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(ui->lineEditColor, SIGNAL(textChanged(QString)), this, SLOT(parseColorString(QString)));
	connect(ui->pushButtonSelectColor, SIGNAL(clicked()), this, SLOT(selectColor()));

	connect(ui->pushButtonSelectTexture, SIGNAL(clicked()), this, SLOT(selectPlanetTextureFile()));
	connect(ui->pushButtonSelectRingTexture, SIGNAL(clicked()), this, SLOT(selectRingTextureFile()));

        ui->labelLongitudeOfTheAscendingNode->setText(QString("Longitude of the ascending node %1:").arg(QChar(0x03A9)));//Capital omega
        ui->radioButtonArgumentOfPeriapsis->setText(QString("Argument of periapsis %1:").arg(QChar(0x3C9)));//Lowercase omega
        ui->radioButtonLongitudeOfPeriapsis->setText(QString("Longitude of periapsis %1:").arg(QChar(0x3D6)));

	//TODO: Move to "set defaults" function
	ui->lineEditColor->setText("1.0, 1.0, 1.0");
	ui->lineEditTexture->setText("nomap.png");
	ui->lineEditRingTexture->setText("saturn_rings_radial.png");
}

void ManualImportWindow::retranslate()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

void ManualImportWindow::selectColor()
{
    QColor color = QColorDialog::getColor(objectColor,&StelMainView::getInstance());
	objectColor =  color;
	ui->lineEditColor->setText(QString("%1, %2, %3").arg(color.redF()).arg(color.greenF()).arg(color.blueF()));
	setColorButtonColor(color);
}

void ManualImportWindow::parseColorString(QString colorCode)
{
	QStringList colorComponents = colorCode.split(QChar(','));
	int count = colorComponents.count();
	if (count < 3 || count > 4)
		return;

	bool ok;
	double red = colorComponents.at(0).toDouble(&ok);
	if (!ok || red < 0.0 || red > 1.0)
		return;
	double green = colorComponents.at(1).toDouble(&ok);
	if (!ok || green < 0.0 || green > 1.0)
		return;
	double blue = colorComponents.at(2).toDouble(&ok);
	if (!ok || blue < 0.0 || blue > 1.0)
		return;

	QColor color;
	color.setRedF(red);
	color.setGreenF(green);
	color.setBlueF(blue);

	if (count == 4)
	{
		double alpha = colorComponents.at(3).toDouble(&ok);
		if (!ok || alpha < 0.0 || alpha > 1.0)
			return;
		color.setAlphaF(alpha);
	}

	objectColor = color;
	setColorButtonColor(color);
}

void ManualImportWindow::setColorButtonColor(QColor newColor)
{
	qDebug() << "setColorButtonColor()";
	QPixmap pixmap(16, 16);
	pixmap.fill(newColor);
	ui->pushButtonSelectColor->setIcon(QIcon(pixmap));
}

void ManualImportWindow::toggleCometOrbit(bool)
{
	//
}

void ManualImportWindow::toggleEllipticOrbit(bool)
{
	//
}

void ManualImportWindow::toggleObjectSpecificOrbit(bool)
{
	//
}

void ManualImportWindow::toggleMeanMotionOrPeriod(bool)
{
	//
}

void ManualImportWindow::selectPlanetTextureFile()
{
	selectTextureFile(ui->lineEditTexture);
}

void ManualImportWindow::selectRingTextureFile()
{
	selectTextureFile(ui->lineEditRingTexture);
}

void ManualImportWindow::selectTextureFile(QLineEdit * filePathLineEdit)
{
	//Find out the parent directory of the last selected file.
	//Open the textures directory if no file have been selected.
	QString texturesDirectoryPath;
	QString currentFileName = filePathLineEdit->text();
	if (currentFileName.isEmpty())
	{
		texturesDirectoryPath = StelFileMgr::findFile("textures", StelFileMgr::Directory);
		if (texturesDirectoryPath.isEmpty())
			return;
	}
	else
	{
		QString currentFilePath = StelFileMgr::findFile("textures/" + currentFileName, StelFileMgr::File);
		if (currentFilePath.isEmpty())
		{
			filePathLineEdit->clear();
			return;
		}
		QFileInfo currentFileInfo(currentFilePath);
		texturesDirectoryPath = currentFileInfo.canonicalPath();
	}

	//Select an existing file
	QStringList supportedFormats;
	for (auto format : QImageReader::supportedImageFormats())
	{
		supportedFormats.append(QString("*.%1").arg(QString(format)));//It's a wee bit long...
	}
	QString fileFilter = QString("Texture files (%1)").arg(supportedFormats.join(" "));
	QString newFilePath = QFileDialog::getOpenFileName(Q_NULLPTR, QString(), texturesDirectoryPath, fileFilter);

	//Is the file in one of the two "textures" directories?
	if (newFilePath.isEmpty())
		return;
	QFileInfo newFileInfo(newFilePath);
	QDir newFileParentDirectory = newFileInfo.dir();
	if (newFileParentDirectory.dirName() != "textures")
		return;
	QDir installedTexturesDirectory(StelFileMgr::getInstallationDir() + "/textures");
	QDir userTexturesDirectory(StelFileMgr::getUserDir() + "/textures");
	if (newFileParentDirectory != installedTexturesDirectory && newFileParentDirectory != userTexturesDirectory)
		return;

	if (verifyTextureFile(newFileInfo.canonicalFilePath()))
		filePathLineEdit->setText(newFileInfo.fileName());
}

bool ManualImportWindow::verifyTextureFile(QString filePath)
{
	//TODO: Absolute path? File exists?

	QPixmap texture(filePath);

	if (texture.isNull())
	{
		qDebug() << "File doesn't exist or is not an accepted texure format:"
				<< filePath;
		return false;
	}

	if (!verifyPowerOfTwo(texture.height()))
	{
		qDebug() << "Invalid texure height:" << texture.height()
				<< "for file" << filePath;
		return false;
	}
	if (!verifyPowerOfTwo(texture.width()))
	{
		qDebug() << "Invalid texture width:" << texture.width()
				<< "for file" << filePath;
		return false;
	}

	return true;
}

bool ManualImportWindow::verifyPowerOfTwo(int value)
{
	if (value > 0 && (value & (value-1)) == 0)
		return true;
	else
		return false;
}
