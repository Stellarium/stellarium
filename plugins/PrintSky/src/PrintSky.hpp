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
	virtual void setStelStyle(const QString& style);

	//! Returns the module-specific style sheet.
	//! The main StelStyle instance should be passed.
	const StelStyle getModuleStyleSheet(const StelStyle& style);

public slots:

	//! Show dialog printing enabling preview and print buttons
	void initPrintingSky();

private slots:

private:

	//Styles
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;

	//Printing options
	bool useInvertColors, scaleToFit, printData, printSSEphemerides;
	QString orientation;

	PrintSkyDialog *printskyDialog;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class PrintSkyStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
 };

#endif // PrintSky_H
