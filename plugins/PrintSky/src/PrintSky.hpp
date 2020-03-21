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
	Q_PROPERTY(bool useInvertColors     READ getInvertColorsState        WRITE setInvertColorsState        NOTIFY invertColorsChanged)
	Q_PROPERTY(bool scaleToFit          READ getScaleToFitState          WRITE setScaleToFitState          NOTIFY scaleToFitChanged)
	Q_PROPERTY(bool printData           READ getPrintDataState           WRITE setPrintDataState           NOTIFY printDataChanged)
	Q_PROPERTY(bool printSSEphemerides  READ getPrintSSEphemeridesState  WRITE setPrintSSEphemeridesState  NOTIFY printSSEphemeridesChanged)
	Q_PROPERTY(bool orientationPortrait READ getOrientationPortraitState WRITE setOrientationPortraitState NOTIFY orientationChanged)

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

	//! Show dialog printing enabling preview and print buttons
	void initPrintingSky();

	void setInvertColorsState(bool state);
	void setScaleToFitState(bool state);
	void setOrientationPortraitState(bool state);
	void setPrintDataState(bool state);
	void setPrintSSEphemeridesState(bool state);
	bool getInvertColorsState() const { return useInvertColors;}
	bool getScaleToFitState() const { return scaleToFit;}
	bool getOrientationPortraitState() const { return orientationPortrait;}
	bool getPrintDataState() const { return printData;}
	bool getPrintSSEphemeridesState() const { return printSSEphemerides;}


private slots:

private:
	//Printing options
	bool useInvertColors, scaleToFit, printData, printSSEphemerides;
	//QString orientation;
	bool orientationPortrait; // true for Portrait paper, false for Landscape

	PrintSkyDialog *printskyDialog;
};

//#include "fixx11h.h"
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
