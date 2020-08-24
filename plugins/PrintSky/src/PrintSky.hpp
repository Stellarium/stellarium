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
#include "StelStyle.hpp"
#include "PrintSkyDialog.hpp"

//class PrintSky
class PrintSky : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool invertColors        READ getInvertColors        WRITE setInvertColors        NOTIFY invertColorsChanged)
	Q_PROPERTY(bool scaleToFit          READ getScaleToFit          WRITE setScaleToFit          NOTIFY scaleToFitChanged)
	Q_PROPERTY(bool printData           READ getPrintData           WRITE setPrintData           NOTIFY printDataChanged)
	Q_PROPERTY(bool flagPrintObjectInfo READ getFlagPrintObjectInfo WRITE setFlagPrintObjectInfo NOTIFY flagPrintObjectInfoChanged)
	Q_PROPERTY(bool printSSEphemerides  READ getPrintSSEphemerides  WRITE setPrintSSEphemerides  NOTIFY printSSEphemeridesChanged)
	Q_PROPERTY(bool flagLimitMagnitude  READ getFlagLimitMagnitude  WRITE setFlagLimitMagnitude  NOTIFY flagLimitMagnitudeChanged)
	Q_PROPERTY(double limitMagnitude    READ getLimitMagnitude      WRITE setLimitMagnitude      NOTIFY limitMagnitudeChanged)
	Q_PROPERTY(bool orientationPortrait READ getOrientationPortrait WRITE setOrientationPortrait NOTIFY orientationChanged)

public:
	 PrintSky();
	virtual ~PrintSky() Q_DECL_OVERRIDE;


	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() Q_DECL_OVERRIDE;
	virtual bool configureGui(bool show=true) Q_DECL_OVERRIDE;
	virtual void update(double) Q_DECL_OVERRIDE {}
	virtual void draw(StelCore* core) Q_DECL_OVERRIDE;
	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

signals:
	void invertColorsChanged(bool state);
	void scaleToFitChanged(bool state);
	void orientationChanged(bool state);
	void printDataChanged(bool state);
	void flagPrintObjectInfoChanged(bool state);
	void printSSEphemeridesChanged(bool state);
	void flagLimitMagnitudeChanged(bool state);
	void limitMagnitudeChanged(double mag);

public slots:

	// Show dialog printing enabling preview and print buttons
	//void initPrintingSky();

	void setInvertColors(bool state);
	void setScaleToFit(bool state);
	void setOrientationPortrait(bool state);
	void setPrintData(bool state);
	void setFlagPrintObjectInfo(bool state);
	void setPrintSSEphemerides(bool state);
	void setFlagLimitMagnitude(bool state);
	void setLimitMagnitude(double mag);

	bool getInvertColors() const { return invertColors;}
	bool getScaleToFit() const { return scaleToFit;}
	bool getOrientationPortrait() const { return orientationPortrait;}
	bool getPrintData() const { return printData;}
	bool getFlagPrintObjectInfo() const { return flagPrintObjectInfo;}
	bool getPrintSSEphemerides() const { return printSSEphemerides;}
	bool getFlagLimitMagnitude() const {return flagLimitMagnitude;}
	double getLimitMagnitude() const {return limitMagnitude;}


private slots:

private:
	//Printing options
	bool invertColors, scaleToFit, printData, printSSEphemerides, orientationPortrait; // true for Portrait paper, false for Landscape
	bool flagPrintObjectInfo;  // print data for selected object
	bool flagLimitMagnitude;   // limit objects in epmeris list to objects brighter than...
	double limitMagnitude;     // limit magnitude for the ephemeris list

	PrintSkyDialog *printskyDialog;
	QSettings* conf;
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
