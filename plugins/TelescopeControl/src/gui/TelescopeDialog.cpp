/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file)
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

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "StelTranslator.hpp"
#include "TelescopeControl.hpp"
#include "TelescopeConfigurationDialog.hpp"
#include "TelescopeDialog.hpp"
#include "ui_telescopeDialog.h"
#include "StelGui.hpp"
#include <QDebug>
#include <QFrame>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItem>

using namespace TelescopeControlGlobals;


TelescopeDialog::TelescopeDialog()
	: StelDialog("TelescopeControl")
	, telescopeCount(0)
	, configuredSlot(0)
	, configuredTelescopeIsNew(false)
{
	telescopeStatus[0] = StatusNA;
	telescopeType[0] = ConnectionNA;

	ui = new Ui_telescopeDialogForm;

	//TODO: Fix this - it's in the same plugin
	telescopeManager = GETSTELMODULE(TelescopeControl);
	telescopeListModel = new QStandardItemModel(0, ColumnCount);

	//TODO: This shouldn't be a hash...
	statusString[StatusNA] = QString(N_("N/A"));
	statusString[StatusStarting] = QString(N_("Starting"));
	statusString[StatusConnecting] = QString(N_("Connecting"));
	statusString[StatusConnected] = QString(N_("Connected"));
	statusString[StatusDisconnected] = QString(N_("Disconnected"));
	statusString[StatusStopped] = QString(N_("Stopped"));
}

TelescopeDialog::~TelescopeDialog()
{	
	delete ui;
	
	delete telescopeListModel;
}

void TelescopeDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutText();
		setHeaderNames();
		updateWarningTexts();
		
		//Retranslate type strings
		for (int i = 0; i < telescopeListModel->rowCount(); i++)
		{
			QStandardItem* item = telescopeListModel->item(i, ColumnType);
			QString original = item->data(Qt::UserRole).toString();
			QModelIndex index = telescopeListModel->index(i, ColumnType);
			telescopeListModel->setData(index, q_(original), Qt::DisplayRole);
		}
	}
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	// Kinetic scrolling
	kineticScrollingList << ui->telescopeTreeView << ui->textBrowserHelp << ui->textBrowserAbout;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	//Connect: sender, signal, receiver, method
	//Page: Telescopes
	connect(ui->pushButtonChangeStatus, SIGNAL(clicked()), this, SLOT(buttonChangeStatusPressed()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()), this, SLOT(buttonConfigurePressed()));
	connect(ui->pushButtonAdd, SIGNAL(clicked()), this, SLOT(buttonAddPressed()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()), this, SLOT(buttonRemovePressed()));
	
	connect(ui->telescopeTreeView, SIGNAL(clicked (const QModelIndex &)), this, SLOT(selectTelecope(const QModelIndex &)));
	//connect(ui->telescopeTreeView, SIGNAL(activated (const QModelIndex &)), this, SLOT(configureTelescope(const QModelIndex &)));
	
	//Page: Options:
	connect(ui->checkBoxReticles, SIGNAL(clicked(bool)),
	        telescopeManager, SLOT(setFlagTelescopeReticles(bool)));
	connect(ui->checkBoxLabels, SIGNAL(clicked(bool)),
	        telescopeManager, SLOT(setFlagTelescopeLabels(bool)));
	connect(ui->checkBoxCircles, SIGNAL(clicked(bool)),
	        telescopeManager, SLOT(setFlagTelescopeCircles(bool)));
	
	connect(ui->checkBoxEnableLogs, SIGNAL(toggled(bool)), telescopeManager, SLOT(setFlagUseTelescopeServerLogs(bool)));
	
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->labelExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->lineEditExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), ui->pushButtonPickExecutablesDirectory, SLOT(setEnabled(bool)));
	connect(ui->checkBoxUseExecutables, SIGNAL(toggled(bool)), this, SLOT(checkBoxUseExecutablesToggled(bool)));
	
	connect(ui->pushButtonPickExecutablesDirectory, SIGNAL(clicked()), this, SLOT(buttonBrowseServerDirectoryPressed()));
	
	//In other dialogs:
	connect(&configurationDialog, SIGNAL(changesDiscarded()), this, SLOT(discardChanges()));
	connect(&configurationDialog, SIGNAL(changesSaved(QString, ConnectionType)), this, SLOT(saveChanges(QString, ConnectionType)));
	
	//Initialize the style
	updateStyle();
	
	//Initializing the list of telescopes
	telescopeListModel->setColumnCount(ColumnCount);
	setHeaderNames();
	
	ui->telescopeTreeView->setModel(telescopeListModel);
	ui->telescopeTreeView->header()->setSectionsMovable(false);
	ui->telescopeTreeView->header()->setSectionResizeMode(ColumnSlot, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setStretchLastSection(true);
	
	//Populating the list
	//Cycle the slots
	for (int slotNumber = MIN_SLOT_NUMBER; slotNumber < SLOT_NUMBER_LIMIT; slotNumber++)
	{
		//Slot #
		//int slotNumber = (i+1)%SLOT_COUNT;//Making sure slot 0 is last
		
		//Make sure that this is initialized for all slots
		telescopeStatus[slotNumber] = StatusNA;
		
		//Read the telescope properties
		QString name;
		ConnectionType connectionType;
		QString equinox;
		QString host;
		int portTCP;
		int delay;
		bool connectAtStartup;
		QList<double> circles;
		QString serverName;
		QString portSerial;
		QString rts2Url;
		QString rts2Username;
		QString rts2Password;
		int rts2Refresh;
		QString ascomDeviceId;
		bool ascomUseDeviceEqCoordType;

		if(!telescopeManager->getTelescopeAtSlot(slotNumber, connectionType, name, equinox, host, portTCP, delay, connectAtStartup, circles, serverName, portSerial, rts2Url, rts2Username, rts2Password, rts2Refresh, ascomDeviceId, ascomUseDeviceEqCoordType))
			continue;
		
		//Determine the server type
		telescopeType[slotNumber] = connectionType;
		
		//Determine the telescope's status
		if (telescopeManager->isConnectedClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnected;
		}
		else
		{
			//TODO: Fix this!
			//At startup everything exists and attempts to connect
			telescopeStatus[slotNumber] = StatusConnecting;
		}
		
		addModelRow(slotNumber, connectionType, telescopeStatus[slotNumber], name);
		
		//After everything is done, count this as loaded
		telescopeCount++;
	}
	
	//Finished populating the table, let's sort it by slot number
	//ui->telescopeTreeView->setSortingEnabled(true);//Set in the .ui file
	ui->telescopeTreeView->sortByColumn(ColumnSlot, Qt::AscendingOrder);
	//(Works even when the table is empty)
	//(Makes redundant the delay of 0 above)
	
	//TODO: Reuse code.
	if(telescopeCount > 0)
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->header()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
	}
	else
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->pushButtonAdd->setFocus();
	}
	updateWarningTexts();
	
	if(telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	
	//Checkboxes
	ui->checkBoxReticles->setChecked(telescopeManager->getFlagTelescopeReticles());
	ui->checkBoxLabels->setChecked(telescopeManager->getFlagTelescopeLabels());
	ui->checkBoxCircles->setChecked(telescopeManager->getFlagTelescopeCircles());
	ui->checkBoxEnableLogs->setChecked(telescopeManager->getFlagUseTelescopeServerLogs());
	
	//Telescope server directory
	ui->checkBoxUseExecutables->setChecked(telescopeManager->getFlagUseServerExecutables());
	ui->lineEditExecutablesDirectory->setText(telescopeManager->getServerExecutablesDirectoryPath());
	
	//About page
	setAboutText();
	
	//Everything must be initialized by now, start the updateTimer
	//TODO: Find if it's possible to run it only when the dialog is visible
	QTimer* updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateTelescopeStates()));
	updateTimer->start(200);
}

void TelescopeDialog::setAboutText()
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	//TODO: Expand
	QString aboutPage = "<html><head></head><body>";
	aboutPage += "<h2>" + q_("Telescope Control plug-in") + "</h2><table width=\"90%\">";
	aboutPage += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + TELESCOPE_CONTROL_PLUGIN_VERSION + "</td></tr>";
	aboutPage += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + TELESCOPE_CONTROL_PLUGIN_LICENSE + "</td></tr>";
	aboutPage += "<tr><td rowspan=6><strong>" + q_("Authors") + "</strong></td><td>Johannes Gajdosik</td></td>";
	aboutPage += "<tr><td>Bogdan Marinov &lt;bogdan.marinov84@gmail.com&gt; (" + q_("Plug-in and GUI programming") + ")</td></tr>";
	aboutPage += "<tr><td>Gion Kunz &lt;gion.kunz@gmail.com&gt; (" + q_("ASCOM Telescope Client") + ")</td></tr>";
	aboutPage += "<tr><td>Petr Kubánek (" + q_("RTS2 support") + ")</td></tr>";
	aboutPage += "<tr><td>Alessandro Siniscalchi &lt;asiniscalchi@gmail.com&gt; (" + q_("INDI Telescope Client") + ")</td></tr>";
	aboutPage += "<tr><td rowspan=3><strong>" + q_("Contributors") + ":</strong></td><td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td></tr>";
	aboutPage += "<tr><td>Michael Heinz</td></tr>";
	aboutPage += "<tr><td>Alexandros Kosiaris</td></tr>";
	aboutPage += "</table>";

	aboutPage += "<p>" + q_("This plug-in is based on and reuses a lot of code under the GNU General Public License:") + "</p><ul>";
	aboutPage += "<li>" + q_("the Telescope, TelescopeDummy, TelescopeTcp and TelescopeMgr classes in Stellarium's code (the client side of Stellarium's original telescope control feature);") + "</li>";
	aboutPage += "<li>" + q_("the telescope server core code (licensed under the LGPL)") + "</li>";
	aboutPage += "<li>" + q_("the TelescopeServerLx200 telescope server core code (originally licensed under the LGPL)");
	aboutPage += "<br/>" + q_("Author of all of the above - the client, the server core, and the LX200 server, along with the Stellarium telescope control network protocol (over TCP/IP), is <b>Johannes Gajdosik</b>.") + "</li>";
	aboutPage += "<li>" + q_("the TelescopeServerNexStar telescope server core code (originally licensed under the LGPL, based on TelescopeServerLx200) by <b>Michael Heinz</b>.") + "</li>";
	aboutPage += "<li>" + q_("INDI by <b>Alessandro Siniscalchi</b>.") + "</li></ul>";

	aboutPage += "<h3>" + q_("Links") + "</h3>";
	aboutPage += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Telescope Control plug-in") + "</p>";
	aboutPage += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	aboutPage += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	aboutPage += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	aboutPage += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Telescope_Control_plug\">\\1</a>") + "</li>";
	aboutPage += "</ul></p></body></html>";
	
	QString helpPage = "<html><head></head><body>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += "<p>" + q_("A more complete and up-to-date documentation for this plug-in can be found on the {Telescope Control} page in the Stellarium Wiki.").replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Telescope_Control_plug-in\">\\1</a>") + "</p>";
	helpPage += "<h3><a name=\"top\" />" + q_("Contents") + "</h3><ul>";
	helpPage += "<li><a href=\"#Abilities_and_limitations\">" + q_("Abilities and limitations") + "</a></li>";
	helpPage += "<li><a href=\"#originalfeature\">" + q_("The original telescope control feature") + "</a></li>";
	helpPage += "<li><a href=\"#usingthisplugin\">" + q_("Using this plug-in") + "</a></li>";
	helpPage += "<li><a href=\"#mainwindow\">" + q_("Main window ('Telescopes')") + "</a></li>";
	helpPage += "<li><a href=\"#configwindow\">" + q_("Telescope configuration window") + "</a><ul>";
	helpPage += "<li><a href=\"#connection_type\">" + q_("Connection type") + "</a></li>";
	helpPage += "<li><a href=\"#telescope_properties\">" + q_("Telescope properties") + "</a></li>";
	helpPage += "<li><a href=\"#device_settings\">" + q_("Device settings") + "</a></li>";
	helpPage += "<li><a href=\"#connection_settings\">" + q_("Connection settings") + "</a></li>";
	helpPage += "<li><a href=\"#fovcircles\">" + q_("Field of view indicators") + "</a></li></ul></li>";
	helpPage += "<li><a href=\"#slew_to\">" + q_("'Slew telescope to' window") + "</a></li>";
	helpPage += "<li><a href=\"#commands\">" + q_("Telescope commands") + "</a></li>";
	helpPage += "<li><a href=\"#devices\">" + q_("Supported devices") + "</a></li>";
	helpPage += "<li><a href=\"#virtual_telescope\">" + q_("Virtual telescope") + "</a></li></ul>";

	helpPage += "<h3><a name=\"Abilities_and_limitations\" />" + q_("Abilities and limitations") + "</h3>";
	helpPage += "<p>" + q_("This plug-in allows Stellarium to send only '<b>slew</b>' ('go to') commands to the device and to receive its current position. It cannot issue any other commands, so users should be aware of the possibility for mount collisions and similar situations. (To abort a slew, you can start another one to a safe position.)") + "</p>";
	helpPage += "<p>" + q_("As of the current version, this plug-in doesn't allow satellite tracking, and is not very suitable for lunar or planetary observations.") + "</p>";
	helpPage += "<p><span style=\"color: red; font-weight: bolder;\">" + q_("WARNING: Stellarium CANNOT prevent your telescope from being pointed at the Sun.") + "</span></p><ul>";
	helpPage += "<li>" + q_("Never point your telescope at the Sun without a proper solar filter installed. The powerful light amplified by the telescope WILL cause irreversible damage to your eyes and/or your equipment.") + "</li>";
	helpPage += "<li>" + q_("Even if you don't do it deliberately, a slew during daylight hours may cause your telescope to point at the sun on its way to the given destination, so it is strongly recommended to avoid using the telescope control feature before sunset without appropriate protection.") + "</li></ul>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"originalfeature\" />" + q_("The original telescope control feature") + "</h3>";
	helpPage += "<p>" + q_("As of Stellarium 0.10.5, the original telescope control feature has been removed. There is no longer a way to control a telescope with Stellarium without this plug-in.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"usingthisplugin\" />" + q_("Using this plug-in") + "</h3>";
	helpPage += "<p>" + q_("Here are two general ways to control a device with this plug-in, depending on the situation:") + "</p><ul>";
	helpPage += "<li><b>" + q_("DIRECT CONNECTION") + "</b>: ";

	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("A {device supported by the plug-in} is connected with a cable to the computer running Stellarium;").replace(a_rx, "<a href=\"#devices\">\\1</a>");
	helpPage += "</li>";
	helpPage += "<li><b>" + q_("INDIRECT CONNECTION") + "</b>: <ul>";
	helpPage += "<li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("A device is connected to the same computer but it is driven by a {stand-alone telescope server program}").replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Telescope_Control_%28client-server%29\">\\1</a>") + " ";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("or a {third-party application} <b>that can 'talk' to Stellarium</b>;").replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Telescope_Control#Third_party_applications\">\\1</a>");
	helpPage += "</li>";
	helpPage += "<li>" + q_("A device is connected to a remote computer and the software that drives it can 'talk' to Stellarium <i>over the network</i>; this software can be either one of Stellarium's stand-alone telescope servers, or a third party application.") + "</li></ul></li></ul>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += "<p>" + q_("Most older telescopes use cables that connect to a {serial port} (RS-232), the newer ones use USB (Universal Serial Bus).").replace(a_rx, "<a href=\"http://meta.wikimedia.org/wiki/wikipedia:en:serial_port\">\\1</a>")
		 + " " + q_("On Linux and Mac OS X both cases are handled identically by the plug-in. On Windows, a USB connection may require a 'virtual serial port' software, if it is not supplied with the cable or the telescope.")
		 + " " + q_("Such a software creates a virtual ('fake') COM port that corresponds to the real USB port so it can be used by the plug-in.")
		 + " " + q_("On all three platforms, if the computer has no 'classic' serial ports and the telescope can connect only to a serial port, a serial-to-USB (RS-232-to-USB) adapter may be necessary.") + "</p>";
	helpPage += "<p>" + q_("Telescope set-up (setting geographical coordinates, performing alignment, etc.) should be done before connecting the telescope to Stellarium.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"mainwindow\" />" + q_("Main window ('Telescopes')") + "</h3>";
	helpPage += "<p>" + q_("The plug-in's main window can be opened:") + "</p><ul>";
	helpPage += "<li>" + q_("By pressing the 'configure' button for the plug-in in the 'Plugins' tab of Stellarium's Configuration window (opened by pressing <b>F2</b> or the respective button in the left toolbar).") + "</li>";
	helpPage += "<li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("By pressing the 'Configure telescopes...' button in the {'Slew to' window} (opened by pressing <b>Ctrl+0</b> or the respective button on the bottom toolbar).").replace(a_rx, "<a href=\"#slew_to\">\\1</a>");
	helpPage += "</li></ul>";
	helpPage += "<p>" + q_("The <b>Telescopes</b> tab displays a list of the telescope connections that have been set up:") + "</p><ul>";
	helpPage += "<li>" + q_("The number (<b>#</b>) column shows the number used to control this telescope. For example, for telescope #2, the shortcut is Ctrl+2.") + "</li>";
	helpPage += "<li>" + q_("The <b>Status</b> column indicates if this connection is currently active or not. Unfortunately, there are some cases in which 'Connected' is displayed when no working connection exists.") + "</li>";
	helpPage += "<li>" + q_("The <b>Type</b> field indicates what kind of connection is this:") + "</li><ul>";
	helpPage += "<li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>virtual</b> means a {virtual telescope};").replace(a_rx, "<a href=\"#virtual_telescope\">\\1</a>");
	helpPage += "</li>";
	helpPage += "<li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>local, Stellarium</b> means a DIRECT connection to the telescope (see {above});").replace(a_rx, "<a href=\"#usingthisplugin\">\\1</a>");
	helpPage += "</li>";
	helpPage += "<li>" + q_("<b>local, external</b> means an INDIRECT connection to a program running on the same computer;") + "</li>";
	helpPage += "<li>" + q_("<b>remote, unknown</b> means an INDIRECT connection over a network to a remote machine.") + "</li></ul></li></ul>";
	helpPage += "<p>" + q_("To set up a new telescope connection, press the <b>Add</b> button. To modify the configuration of an existing connection, select it in the list and press the <b>Configure</b> button. In both cases, a telescope connection configuration window will open.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"configwindow\" />" + q_("Telescope configuration window") + "</h3>";

	helpPage += "<h4><a name=\"connection_type\" />" + q_("Connection type") + "</h4>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("The topmost field represents the choice between the two types of connections (see {above}):").replace(a_rx, "<a href=\"#usingthisplugin\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p><b>" + q_("Telescope controlled by:") + "</b></p><ul>";
	helpPage += "<li>" + q_("<b>Stellarium, directly through a serial port</b> is the DIRECT case") + "</li>";
	helpPage += "<li>" + q_("<b>External software or a remote computer</b> is the INDIRECT case") + "</li>";
	helpPage += "<li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>Nothing, just simulate one (a moving reticle)</b> is a {virtual telescope} (no connection)").replace(a_rx, "<a href=\"#virtual_telescope\">\\1</a>");
	helpPage += "</li></ul>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h4><a name=\"telescope_properties\" />" + q_("Telescope properties") + "</h4>";
	helpPage += "<p>" + q_("<b>Name</b> is the label that will be displayed on the screen next to the telescope reticle.") + "</p>";
	helpPage += "<p>" + q_("<b>Connection delay</b>: If the movement of the telescope reticle on the screen is uneven, you can try increasing or decreasing this value.") + "</p>";
	helpPage += "<p>" + q_("<b>Coordinate system</b>: Some Celestron telescopes have had their firmware updated and now interpret the coordinates they receive as coordinates that use the equinox of the date (EOD, also known as JNow), making necessary this override.") + "</p>";
	helpPage += "<p>" + q_("<b>Start/connect at startup</b>: Check this option if you want Stellarium to attempt to connect to the telescope immediately after it starts.")
		 + " " + q_("Otherwise, to start the telescope, you need to open the main window, select that telescope and press the 'Start/Connect' button.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h4><a name=\"device_settings\" />" + q_("Device settings") + "</h4>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("This section is active only for DIRECT connections (see {above}).").replace(a_rx, "<a href=\"#usingthisplugin\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>" + q_("<b>Serial port</b> sets the serial port used by the telescope.") + "</p>";
	helpPage += "<p>" + q_("There is a pop-up box that suggests some default values:") + "</p><ul>";
	helpPage += "<li>" + q_("On Windows, serial ports COM1 to COM10;") + "</li>";
	helpPage += "<li>" + q_("On Linux, serial ports /dev/ttyS0 to /dev/ttyS3 and USB ports /dev/ttyUSB0 to /dev/ttyUSB3;") + "</li>";
	helpPage += "<li>" + q_("On Mac OS X, the list is empty as it names its ports in a peculiar way.") + "</li></ul>";
	helpPage += "<p>" + q_("If you are using an USB cable, the default serial port of your telescope most probably is not in the list of suggestions.") + "</p>";
	helpPage += "<p>" + q_("To list all valid serial port names in Mac OS X, open a terminal and type:") + "<br /><samp>ls /dev/*</samp></p>";
	helpPage += "<p>" + q_("This will list all devices, the full name of your serial port should be somewhere in the list (for example, '/dev/cu.usbserial-FTDFZVMK').") + "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>Device model</b>: see {Supported devices} below.").replace(a_rx, "<a href=\"#devices\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h4><a name=\"connection_settings\" />" + q_("Connection settings") + "</h4>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("Both fields here refer to communication over a network ({TCP/IP}).").replace(a_rx, "<a href=\"http://meta.wikimedia.org/wiki/wikipedia:en:TCP/IP\">\\1</a>") + " ";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("Doing something with them is necessary only for INDIRECT connections (see {above}).").replace(a_rx, "<a href=\"#usingthisplugin\">\\1</a>") + " ";
	helpPage += "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>Host</b> can be either a host name or an {IPv4} address such as '127.0.0.1'. The default value of 'localhost' means 'this computer'.").replace(a_rx, "<a href=\"http://meta.wikimedia.org/wiki/wikipedia:en:IPv4\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>" + q_("<b>Port</b> refers to the TCP port used for communication. The default value depends on the telescope number and ranges between 10001 and 10009.") + "</p>";
	helpPage += "<p>" + q_("Both values are ignored for DIRECT connections.") + "</p>";
	helpPage += "<p>" + q_("For INDIRECT connections, modifying the default host name value makes sense only if you are attempting a remote connection over a network.")
		 + " " + q_("In this case, it should be the name or IP address of the computer that runs a program that runs the telescope.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h4><a name=\"fovcircles\" />" + q_("Field of view indicators") + "</h4>";
	helpPage += "<p>" + q_("A series of circles representing different fields of view can be added around the telescope marker. This is a relic from the times before the <strong>Oculars</strong> plug-in existed.") + "</p>";
	helpPage += "<p>" + q_("In the telescope configuration window, click on 'User Interface Settings'.")
		 + " " + q_("Mark the 'Use field of view indicators' option, then enter a list of values separated with commas in the field below.")
		 + " " + q_("The values are interpreted as degrees of arc.") + "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("This can be used in combination with a {virtual telescope} to display a moving reticle with the Telrad circles.").replace(a_rx, "<a href=\"#virtual_telescope\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"slew_to\" />" + q_("'Slew telescope to' window") + "</h3>";
	helpPage += "<p>" + q_("The 'Slew telescope to' window can be opened by pressing <b>Ctrl+0</b> or the respective button in the bottom toolbar.") + "</p>";
	helpPage += "<p>" + q_("It contains two fields for entering celestial coordinates, selectors for the preferred format (Hours-Minutes-Seconds, Degrees-Minutes-Seconds, or Decimal degrees), a drop-down list and two buttons.") + "</p>";
	helpPage += "<p>" + q_("The drop-down list contains the names of the currently connected devices.") + " ";
	helpPage += q_("If no devices are connected, it will remain empty, and the 'Slew' button will be disabled.") + "</p>";
	helpPage += "<p>" + q_("Pressing the <b>Slew</b> button slews the selected device to the selected set of coordinates.") + " ";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("See the section about {keyboard commands} below for other ways of controlling the device.").replace(a_rx, "<a href=\"#commands\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("Pressing the <b>Configure telescopes...</b> button opens the {main window} of the plug-in.").replace(a_rx, "<a href=\"#mainwindow\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>" + q_("<b>TIP:</b> Inside the 'Slew' window, underlined letters indicate that pressing 'Alt + underlined letter' can be used instead of clicking.") + " ";
	helpPage +=  q_("For example, pressing <b>Alt+S</b> is equivalent to clicking the 'Slew' button, pressing <b>Alt+E</b> switches to decimal degree format, etc.") + "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"commands\" />" + q_("Sending commands") + "</h3>";
	helpPage += "<p>" + q_("Once a telescope is successfully started/connected, Stellarium displays a telescope reticle labelled with the telescope's name on its current position in the sky.")
		 + " " + q_("The reticle is an object like every other in Stellarium - it can be selected with the mouse, it can be tracked and it appears as an object in the 'Search' window.") + "</p>";
	helpPage += "<p>" + q_("<b>To point a device to an object:</b> Select an object (e.g. a star) and press the number of the device while holding down the <b>Ctrl</b> key.")
		 + " (" + q_("For example, Ctrl+1 for telescope #1.") + ") "
		 + q_("This will move the telescope to the selected object.") + "</p>";
	helpPage += "<p>" + q_("<b>To point a device to the center of the view:</b> Press the number of the device while holding down the <b>Alt</b> key.")
		 + " (" + q_("For example, Alt+1 for telescope #1.") + ") "
		 + q_("This will slew the device to the point in the center of the current view.")
		 + " (" + q_("If you move the view after issuing the command, the target won't change unless you issue another command.") + ")</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("<b>To point a device to a given set of coordinates:</b> Use the {'Slew to' window} (press <b>Ctrl+0</b>).").replace(a_rx, "<a href=\"#slew_to\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"devices\" />" + q_("Supported devices") + "</h3>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("All devices listed in the {'Device model' list} are convenience definitions using one of the two built-in interfaces: the Meade LX200 (the Meade Autostar controller) interface and the Celestron NexStar interface.").replace(a_rx, "<a href=\"#device_settings\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>" + q_("The device list contains the following:") + "</p><dl>";
	helpPage += "<dt><b>Celestron NexStar (compatible)</b></dt><dd>" + q_("Any device using the NexStar interface.") + "</dd>";
	helpPage += "<dt><b>Losmandy G-11</b></dt><dd>" + q_("A computerized telescope mount made by Losmandy (Meade LX-200/Autostar interface).") + "</dd>";
	helpPage += "<dt><b>Meade Autostar compatible</b></dt><dd>" + q_("Any device using the LX-200/Autostar interface.") + "</dd>";
	helpPage += "<dt><b>Meade ETX-70 (#494 Autostar, #506 CCS)</b></dt><dd>" + q_("The Meade ETX-70 telescope with the #494 Autostar controller and the #506 Connector Cable Set.") + " ";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("According to the tester, it is a bit slow, so its default setting of %1'Connection delay'%2 is 1.5 seconds instead of 0.5 seconds.").replace(a_rx, "<a href=\"#telescope_properties\">\\1</a>");
	helpPage += "</dd>";
	helpPage += "<dt><b>Meade LX200 (compatible)</b></dt><dd>" + q_("Any device using the LX-200/Autostar interface.") + "</dd>";
	helpPage += "<dt><b>Sky-Watcher SynScan AZ mount</b></dt><dd>" + q_("The Sky-Watcher SynScan AZ GoTo mount is used in a number of telescopes.") + "</dd>";
	helpPage += "<dt><b>Sky-Watcher SynScan (version 3 or later)</b></dt><dd>" + q_("<b>SynScan</b> is also the name of the hand controller used in other Sky-Watcher GoTo mounts, and it seems that any mount that uses a SynScan controller version 3.0 or greater is supported by the plug-in, as it uses the NexStar protocol.") + "</dd>";
	helpPage += "<dt><b>Wildcard Innovations Argo Navis (Meade mode)</b></dt><dd>" + q_("Argo Navis is a 'Digital Telescope Computer' by Wildcard Innovations.")
		 + " " + q_("It is an advanced digital setting circle that turns an ordinary telescope (for example, a dobsonian) into a 'Push To' telescope (a telescope that uses a computer to find targets and human power to move the telescope itself).")
		 + " " + q_("Just don't forget to set it to Meade compatibility mode and set the baud rate to 9600B")
		 + "<sup><a href=\"http://www.iceinspace.com.au/forum/showpost.php?p=554948&amp;postcount=18\">1</a></sup>.</dd></dl>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "<h3><a name=\"virtual_telescope\" />" + q_("Virtual telescope") + "</h3>";
	helpPage += "<p>" + q_("If you want to test this plug-in without an actual device connected to the computer, choose <b>Nothing, just simulate one (a moving reticle)</b> in the <b>Telescope controlled by:</b> field. It will show a telescope reticle that will react in the same way as the reticle of a real telescope controlled by the plug-in.") + "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("See the section above about {field of view indicators} for a possible practical application (emulating 'Telrad' circles).").replace(a_rx, "<a href=\"#fovcircles\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	helpPage += q_("This feature is equivalent to the 'Dummy' type of telescope supported by {Stellarium's original telescope control feature}.").replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Telescope_Control_%28client-server%29\">\\1</a>");
	helpPage += "</p>";
	helpPage += "<p><a href=\"#top\"><small>[" + q_("Back to top") + "]</small></a></p>";

	helpPage += "</body></html>";
	
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		ui->textBrowserAbout->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
		ui->textBrowserHelp->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}
	ui->textBrowserAbout->setHtml(aboutPage);
	ui->textBrowserHelp->setHtml(helpPage);
}

void TelescopeDialog::setHeaderNames()
{
	QStringList headerStrings;
	// TRANSLATORS: Symbol for "number"
	headerStrings << q_("#");
	//headerStrings << "Start";
	headerStrings << q_("Status");
	headerStrings << q_("Type");
	headerStrings << q_("Name");
	telescopeListModel->setHorizontalHeaderLabels(headerStrings);
}

void TelescopeDialog::updateWarningTexts()
{
	QString text;
	if (telescopeCount > 0)
	{
#ifdef Q_OS_MAC
		QString modifierName = "Command";
#else
		QString modifierName = "Ctrl";
#endif
		
		text = QString(q_("To slew a connected telescope to an object (for example, a star), select that object, then hold down the %1 key and press the key with that telescope's number. To slew it to the center of the current view, hold down the Alt key and press the key with that telescope's number.")).arg(modifierName);
	}
	else
	{
		if (telescopeManager->getDeviceModels().isEmpty())
		{
			// TRANSLATORS: Currently, it is very unlikely if not impossible to actually see this text. :)
			text = q_("No device model descriptions are available. Stellarium will not be able to control a telescope on its own, but it is still possible to do it through an external application or to connect to a remote host.");
		}
		else
		{
			// TRANSLATORS: The translated name of the Add button is automatically inserted.
			text = QString(q_("Press the \"%1\" button to set up a new telescope connection.")).arg(ui->pushButtonAdd->text());
		}
	}
	
	ui->labelWarning->setText(text);
}

QString TelescopeDialog::getTypeLabel(ConnectionType type)
{
	QString typeLabel;
	switch (type)
	{
		case ConnectionInternal:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("local, Stellarium");
			break;
		case ConnectionLocal:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("local, external");
			break;
		case ConnectionRemote:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("remote, unknown");
			break;
		case ConnectionVirtual:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("virtual");
			break;
		case ConnectionRTS2:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("remote, RTS2");
			break;
		case ConnectionINDI:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("remote, INDI/INDIGO");
			break;
		case ConnectionASCOM:
			// TRANSLATORS: Telescope connection type
			typeLabel = N_("local, ASCOM");
			break;
		default:
			;
	}
	return typeLabel;
}

void TelescopeDialog::addModelRow(int number,
                                  ConnectionType type,
                                  TelescopeStatus status,
                                  const QString& name)
{
	Q_ASSERT(telescopeListModel);
	
	QStandardItem* tempItem = Q_NULLPTR;
	int lastRow = telescopeListModel->rowCount();
	// Number
	tempItem = new QStandardItem(QString::number(number));
	tempItem->setEditable(false);
	telescopeListModel->setItem(lastRow, ColumnSlot, tempItem);
	
	// Checkbox
	//TODO: This is not updated, because it was commented out
	//tempItem = new QStandardItem;
	//tempItem->setEditable(false);
	//tempItem->setCheckable(true);
	//tempItem->setCheckState(Qt::Checked);
	//tempItem->setData("If checked, this telescope will start when Stellarium is started", Qt::ToolTipRole);
	//telescopeListModel->setItem(lastRow, ColumnStartup, tempItem);//Start-up checkbox
	
	//Status
	tempItem = new QStandardItem(q_(statusString[status]));
	tempItem->setEditable(false);
	telescopeListModel->setItem(lastRow, ColumnStatus, tempItem);
	
	//Type
	QString typeLabel = getTypeLabel(type);
	tempItem = new QStandardItem(q_(typeLabel));
	tempItem->setEditable(false);
	tempItem->setData(typeLabel, Qt::UserRole);
	telescopeListModel->setItem(lastRow, ColumnType, tempItem);
	
	//Name
	tempItem = new QStandardItem(name);
	tempItem->setEditable(false);
	telescopeListModel->setItem(lastRow, ColumnName, tempItem);
}

void TelescopeDialog::updateModelRow(int rowNumber,
                                     ConnectionType type,
                                     TelescopeStatus status,
                                     const QString& name)
{
	Q_ASSERT(telescopeListModel);
	if (rowNumber > telescopeListModel->rowCount())
		return;
	
	//The slot number doesn't need to be updated. :)
	//Status
	QString statusLabel = q_(statusString[status]);
	QModelIndex index = telescopeListModel->index(rowNumber, ColumnStatus);
	telescopeListModel->setData(index, statusLabel, Qt::DisplayRole);
	
	//Type
	QString typeLabel = getTypeLabel(type);
	index = telescopeListModel->index(rowNumber, ColumnType);
	telescopeListModel->setData(index, typeLabel, Qt::UserRole);
	telescopeListModel->setData(index, q_(typeLabel), Qt::DisplayRole);
	
	//Name
	index = telescopeListModel->index(rowNumber, ColumnName);
	telescopeListModel->setData(index, name, Qt::DisplayRole);
}


void TelescopeDialog::selectTelecope(const QModelIndex & index)
{
	//Extract selected item index
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(index.row(),0) ).toInt();
	updateStatusButtonForSlot(selectedSlot);

	//In all cases
	ui->pushButtonRemove->setEnabled(true);
}

void TelescopeDialog::configureTelescope(const QModelIndex & currentIndex)
{
	configuredTelescopeIsNew = false;
	configuredSlot = telescopeListModel->data( telescopeListModel->index(currentIndex.row(), ColumnSlot) ).toInt();
	
	//Stop the telescope first if necessary
	if(telescopeType[configuredSlot] != ConnectionInternal && telescopeStatus[configuredSlot] != StatusDisconnected)
	{
		if(telescopeManager->stopTelescopeAtSlot(configuredSlot)) //Act as "Disconnect"
				telescopeStatus[configuredSlot] = StatusDisconnected;
		else
			return;
	}
	else if(telescopeStatus[configuredSlot] != StatusStopped)
	{
		if(telescopeManager->stopTelescopeAtSlot(configuredSlot)) //Act as "Stop"
					telescopeStatus[configuredSlot] = StatusStopped;
	}
	//Update the status in the list
	int curRow = ui->telescopeTreeView->currentIndex().row();
	QModelIndex curIndex = telescopeListModel->index(curRow, ColumnStatus);
	QString string = q_(statusString[telescopeStatus[configuredSlot]]);
	telescopeListModel->setData(curIndex, string, Qt::DisplayRole);
	
	setVisible(false);
	configurationDialog.setVisible(true); //This should be called first to actually create the dialog content
	
	configurationDialog.initExistingTelescopeConfiguration(configuredSlot);
}

void TelescopeDialog::buttonChangeStatusPressed()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
	
	//TODO: As most of these are asynchronous actions, it looks like that there should be a queue...
	
	if(telescopeType[selectedSlot] != ConnectionInternal) 
	{
		//Can't be launched by Stellarium -> can't be stopped by Stellarium
		//Can be only connected/disconnected
		if(telescopeStatus[selectedSlot] == StatusDisconnected)
		{
			if(telescopeManager->startTelescopeAtSlot(selectedSlot)) //Act as "Connect"
				telescopeStatus[selectedSlot] = StatusConnecting;
		}
		else
		{
			if(telescopeManager->stopTelescopeAtSlot(selectedSlot)) //Act as "Disconnect"
				telescopeStatus[selectedSlot] = StatusDisconnected;
		}
	}
	else
	{
		switch(telescopeStatus[selectedSlot]) //Why the switch?
		{
			case StatusNA:
			case StatusStopped:
			{
				if(telescopeManager->startTelescopeAtSlot(selectedSlot)) //Act as "Start"
					telescopeStatus[selectedSlot] = StatusConnecting;
			}
			break;
			case StatusConnecting:
			case StatusConnected:
			{
				if(telescopeManager->stopTelescopeAtSlot(selectedSlot)) //Act as "Stop"
					telescopeStatus[selectedSlot] = StatusStopped;
			}
			break;
			default:
				break;
		}
	}
	
	//Update the status in the list
	int curRow = ui->telescopeTreeView->currentIndex().row();
	QModelIndex curIndex = telescopeListModel->index(curRow, ColumnStatus);
	QString string = q_(statusString[telescopeStatus[selectedSlot]]);
	telescopeListModel->setData(curIndex, string, Qt::DisplayRole);
}

void TelescopeDialog::buttonConfigurePressed()
{
	if(ui->telescopeTreeView->currentIndex().isValid())
		configureTelescope(ui->telescopeTreeView->currentIndex());
}

void TelescopeDialog::buttonAddPressed()
{
	if(telescopeCount >= SLOT_COUNT)
		return;
	
	configuredTelescopeIsNew = true;
	
	//Find the first unoccupied slot (there is at least one)
	for (configuredSlot = MIN_SLOT_NUMBER; configuredSlot < SLOT_NUMBER_LIMIT; configuredSlot++)
	{
		//configuredSlot = (i+1)%SLOT_COUNT;
		if(telescopeStatus[configuredSlot] == StatusNA)
			break;
	}
	
	setVisible(false);
	configurationDialog.setVisible(true); //This should be called first to actually create the dialog content
	configurationDialog.initNewTelescopeConfiguration(configuredSlot);
}

void TelescopeDialog::buttonRemovePressed()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected slot
	int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(),0) ).toInt();
	
	//Stop the telescope if necessary and remove it
	if(telescopeManager->stopTelescopeAtSlot(selectedSlot))
	{
		//TODO: Update status?
		if(!telescopeManager->removeTelescopeAtSlot(selectedSlot))
		{
			//TODO: Add debug
			return;
		}
	}
	else
	{
		//TODO: Add debug
		return;
	}
	
	//Save the changes to file
	telescopeManager->saveTelescopes();
	
	telescopeStatus[selectedSlot] = StatusNA;
	telescopeCount -= 1;
	
//Update the interface to reflect the changes:
	
	//Make sure that the header section keeps it size
	if(telescopeCount == 0)
		ui->telescopeTreeView->header()->setSectionResizeMode(ColumnType, QHeaderView::Interactive);
	
	//Remove the telescope from the table/tree
	telescopeListModel->removeRow(ui->telescopeTreeView->currentIndex().row());
	
	//If there are less than the maximal number of telescopes now, new ones can be added
	if(telescopeCount < SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(true);
	
	//If there are no telescopes left, disable some buttons
	if(telescopeCount == 0)
	{
		//TODO: Fix the phantom text of the Status button (reuse code?)
		//IDEA: Vsible/invisible instead of enabled/disabled?
		//The other buttons expand to take the place (delete spacers)
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
	}
	else
	{
		ui->telescopeTreeView->setCurrentIndex(telescopeListModel->index(0,0));
	}
	updateWarningTexts();
}

void TelescopeDialog::saveChanges(QString name, ConnectionType type)
{
	//Save the changes to file
	telescopeManager->saveTelescopes();
	
	//Type and server properties
	telescopeType[configuredSlot] = type;
	switch (type)
	{
		case ConnectionVirtual:
			telescopeStatus[configuredSlot] = StatusStopped;
			break;

		case ConnectionInternal:
			if(configuredTelescopeIsNew)
				telescopeStatus[configuredSlot] = StatusStopped;//TODO: Is there a point? Isn't it better to force the status update method?
			break;

		case ConnectionLocal:
			telescopeStatus[configuredSlot] = StatusDisconnected;
			break;

		case ConnectionRemote:
		default:
			telescopeStatus[configuredSlot] = StatusDisconnected;
	}
	
	//Update the model/list
	TelescopeStatus status = telescopeStatus[configuredSlot];
	if(configuredTelescopeIsNew)
	{
		addModelRow(configuredSlot, type, status, name);
		telescopeCount++;
	}
	else
	{
		int currentRow = ui->telescopeTreeView->currentIndex().row();
		updateModelRow(currentRow, type, status, name);
	}
	//Sort the updated table by slot number
	ui->telescopeTreeView->sortByColumn(ColumnSlot, Qt::AscendingOrder);
	
	//Can't add more telescopes if they have reached the maximum number
	if (telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	
	//
	if (telescopeCount == 0)
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->telescopeTreeView->header()->setSectionResizeMode(ColumnType, QHeaderView::Interactive);
	}
	else
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->setCurrentIndex(telescopeListModel->index(0,0));
		ui->pushButtonConfigure->setEnabled(true);
		ui->pushButtonRemove->setEnabled(true);
		ui->telescopeTreeView->header()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
	}
	updateWarningTexts();
	
	configuredTelescopeIsNew = false;
	configurationDialog.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
}

void TelescopeDialog::discardChanges()
{
	configurationDialog.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
	
	if (telescopeCount >= SLOT_COUNT)
		ui->pushButtonAdd->setEnabled(false);
	if (telescopeCount == 0)
		ui->pushButtonRemove->setEnabled(false);
	
	configuredTelescopeIsNew = false;
}

void TelescopeDialog::updateTelescopeStates()
{
	if(telescopeCount == 0)
		return;
	
	int slotNumber = -1;
	for (int i=0; i<(telescopeListModel->rowCount()); i++)
	{
		slotNumber = telescopeListModel->data( telescopeListModel->index(i, ColumnSlot) ).toInt();
		//TODO: Check if these cover all possibilites
		if (telescopeManager->isConnectedClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnected;
		}
		else if(telescopeManager->isExistingClientAtSlot(slotNumber))
		{
			telescopeStatus[slotNumber] = StatusConnecting;
		}
		else
		{
			if(telescopeType[slotNumber] == ConnectionInternal)
				telescopeStatus[slotNumber] = StatusStopped;
			else
				telescopeStatus[slotNumber] = StatusDisconnected;
		}
		
		//Update the status in the list
		QModelIndex index = telescopeListModel->index(i, ColumnStatus);
		QString statusStr = q_(statusString[telescopeStatus[slotNumber]]);
		telescopeListModel->setData(index, statusStr, Qt::DisplayRole);
	}
	
	if(ui->telescopeTreeView->currentIndex().isValid())
	{
		int selectedSlot = telescopeListModel->data( telescopeListModel->index(ui->telescopeTreeView->currentIndex().row(), ColumnSlot) ).toInt();
		//Update the ChangeStatus button
		updateStatusButtonForSlot(selectedSlot);
	}
}

void TelescopeDialog::updateStatusButtonForSlot(int selectedSlot)
{
	if(telescopeType[selectedSlot] != ConnectionInternal)
	{
		//Can't be launched by Stellarium => can't be stopped by Stellarium
		//Can be only connected/disconnected
		if(telescopeStatus[selectedSlot] == StatusDisconnected)
		{
			setStatusButtonToConnect();
			ui->pushButtonChangeStatus->setEnabled(true);
		}
		else
		{
			setStatusButtonToDisconnect();
			ui->pushButtonChangeStatus->setEnabled(true);
		}
	}
	else
	{
		switch(telescopeStatus[selectedSlot])
		{
			case StatusNA:
			case StatusStopped:
				setStatusButtonToStart();
				ui->pushButtonChangeStatus->setEnabled(true);
				break;
			case StatusConnected:
			case StatusConnecting:
				setStatusButtonToStop();
				ui->pushButtonChangeStatus->setEnabled(true);
				break;
			default:
				setStatusButtonToStart();
				ui->pushButtonChangeStatus->setEnabled(false);
				ui->pushButtonConfigure->setEnabled(false);
				ui->pushButtonRemove->setEnabled(false);
				break;
		}
	}
}

void TelescopeDialog::setStatusButtonToStart()
{
	ui->pushButtonChangeStatus->setText(q_("Start"));
	ui->pushButtonChangeStatus->setIcon(QIcon(":/graphicGui/uibtStart.png"));
        ui->pushButtonChangeStatus->setToolTip(q_("Start the selected local telescope"));
}

void TelescopeDialog::setStatusButtonToStop()
{
	ui->pushButtonChangeStatus->setText(q_("Stop"));
	ui->pushButtonChangeStatus->setIcon(QIcon(":/graphicGui/uibtStop.png"));
        ui->pushButtonChangeStatus->setToolTip(q_("Stop the selected local telescope"));
}

void TelescopeDialog::setStatusButtonToConnect()
{
        ui->pushButtonChangeStatus->setText(q_("Connect"));
	ui->pushButtonChangeStatus->setIcon(QIcon(":/graphicGui/uibtStart.png"));
        ui->pushButtonChangeStatus->setToolTip(q_("Connect to the selected telescope"));
}

void TelescopeDialog::setStatusButtonToDisconnect()
{
        ui->pushButtonChangeStatus->setText(q_("Disconnect"));
	ui->pushButtonChangeStatus->setIcon(QIcon(":/graphicGui/uibtStop.png"));
        ui->pushButtonChangeStatus->setToolTip(q_("Disconnect from the selected telescope"));
}

void TelescopeDialog::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		if (gui)
			ui->textBrowserAbout->document()->setDefaultStyleSheet(gui->getStelStyle().htmlStyleSheet);
	}
}

void TelescopeDialog::checkBoxUseExecutablesToggled(bool useExecutables)
{
	telescopeManager->setFlagUseServerExecutables(useExecutables);
}

void TelescopeDialog::buttonBrowseServerDirectoryPressed()
{
	QString newPath = QFileDialog::getExistingDirectory (Q_NULLPTR, QString(q_("Select a directory")), telescopeManager->getServerExecutablesDirectoryPath());
	//TODO: Validation? Directory exists and contains servers?
	if(!newPath.isEmpty())
	{
		ui->lineEditExecutablesDirectory->setText(newPath);
		telescopeManager->setServerExecutablesDirectoryPath(newPath);
		telescopeManager->setFlagUseServerExecutables(true);
	}
}
