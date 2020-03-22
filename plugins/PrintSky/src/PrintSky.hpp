/*
 * Copyright (C) 2010 Pep Pujols
 * Copyright (C) 2020 Georg Zotti
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

#ifndef PrintSky_H
#define PrintSky_H

#include "StelModule.hpp"
#include <QPrinter>
#include <QPrintPreviewWidget>
#include <QPrintPreviewDialog>
#include "StelStyle.hpp"
#include "PrintSkyDialog.hpp"

//class PrintSky
class PrintSky : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool invertColors        READ getInvertColors        WRITE setInvertColors        NOTIFY invertColorsChanged)
	Q_PROPERTY(bool scaleToFit          READ getScaleToFit          WRITE setScaleToFit          NOTIFY scaleToFitChanged)
	Q_PROPERTY(bool printData           READ getPrintData           WRITE setPrintData           NOTIFY printDataChanged)
	Q_PROPERTY(bool printSSEphemerides  READ getPrintSSEphemerides  WRITE setPrintSSEphemerides  NOTIFY printSSEphemeridesChanged)
	Q_PROPERTY(bool orientationPortrait READ getOrientationPortrait WRITE setOrientationPortrait NOTIFY orientationChanged)

public:
	 PrintSky();
	virtual ~PrintSky();


	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual bool configureGui(bool show=true);
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual void handleKeys(class QKeyEvent* event);
	virtual void handleMouseClicks(class QMouseEvent* event);
	virtual bool handleMouseMoves(int x, int y, Qt::MouseButtons b);
	//virtual void setStelStyle(const QString& style);

signals:
	void invertColorsChanged(bool state);
	void scaleToFitChanged(bool state);
	void orientationChanged(bool state);
	void printDataChanged(bool state);
	void printSSEphemeridesChanged(bool state);

public slots:

	// Show dialog printing enabling preview and print buttons
	//void initPrintingSky();

	void setInvertColors(bool state);
	void setScaleToFit(bool state);
	void setOrientationPortrait(bool state);
	void setPrintData(bool state);
	void setPrintSSEphemerides(bool state);
	bool getInvertColors() const { return invertColors;}
	bool getScaleToFit() const { return scaleToFit;}
	bool getOrientationPortrait() const { return orientationPortrait;}
	bool getPrintData() const { return printData;}
	bool getPrintSSEphemerides() const { return printSSEphemerides;}


private slots:

private:
	//Printing options
	bool invertColors, scaleToFit, printData, printSSEphemerides;
	bool orientationPortrait; // true for Portrait paper, false for Landscape

	PrintSkyDialog *printskyDialog;
};

#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class PrintSkyStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
 };

#endif // PrintSky_H
