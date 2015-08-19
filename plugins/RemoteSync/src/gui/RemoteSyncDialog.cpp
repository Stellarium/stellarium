/*
 * Stellarium Remote Sync plugin
 * Copyright (C) 2015 Florian Schaukowitsch
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

#include "RemoteSync.hpp"
#include "RemoteSyncDialog.hpp"
#include "ui_remoteSyncDialog.h"

#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"

RemoteSyncDialog::RemoteSyncDialog()
	: rs(NULL)
{
	ui = new Ui_remoteSyncDialog();
}

RemoteSyncDialog::~RemoteSyncDialog()
{
	delete ui;
}

void RemoteSyncDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void RemoteSyncDialog::createDialogContent()
{
	rs = GETSTELMODULE(RemoteSync);
	ui->setupUi(dialog);

#ifdef Q_OS_WIN
	//Kinetic scrolling for tablet pc and pc
	QList<QWidget *> addscroll;
	addscroll << ui->aboutTextBrowser;
	installKineticScrolling(addscroll);
#endif

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	connect(rs, SIGNAL(stateChanged(RemoteSync::SyncState)), this, SLOT(updateState()));
	updateState();

	connect(rs, SIGNAL(errorOccurred(QString)), this, SLOT(printErrorMessage(QString)));

	ui->clientServerHostEdit->setText(rs->getClientServerHost());
	connect(ui->clientServerHostEdit, SIGNAL(textChanged(QString)), rs, SLOT(setClientServerHost(QString)));
	ui->clientServerPortSpinBox->setValue(rs->getClientServerPort());
	connect(ui->clientServerPortSpinBox, SIGNAL(valueChanged(int)), rs, SLOT(setClientServerPort(int)));

	ui->serverPortSpinBox->setValue(rs->getServerPort());
	connect(ui->serverPortSpinBox, SIGNAL(valueChanged(int)), rs, SLOT(setServerPort(int)));

	connect(ui->saveSettingsButton, SIGNAL(clicked()), rs, SLOT(saveSettings()));
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), rs, SLOT(restoreDefaultSettings()));

	setAboutHtml();
}

void RemoteSyncDialog::printErrorMessage(const QString error)
{
	ui->statusLabel->setText(QString(q_("ERROR: %1")).arg(error));
	ui->statusLabel->setStyleSheet("color: Red;");
}

void RemoteSyncDialog::updateState()
{
	RemoteSync::SyncState state = rs->getState();

	//disconnect the click signals from whatever is connected
	disconnect(ui->serverButton, SIGNAL(clicked(bool)), NULL, NULL);
	disconnect(ui->clientButton, SIGNAL(clicked(bool)), NULL, NULL);
	ui->statusLabel->setStyleSheet("");

	if(state == RemoteSync::IDLE)
	{
		ui->serverGroupBox->setEnabled(true);
		ui->serverControls->setEnabled(true);
		ui->serverButton->setText(q_("Start server"));
		connect(ui->serverButton, SIGNAL(clicked(bool)), rs, SLOT(startServer()));

		ui->clientGroupBox->setEnabled(true);
		ui->clientControls->setEnabled(true);
		ui->clientButton->setText(q_("Connect to server"));
		connect(ui->clientButton, SIGNAL(clicked(bool)), rs, SLOT(connectToServer()));

		ui->statusLabel->setText(q_("Not running"));
	}
	else if (state == RemoteSync::SERVER)
	{
		ui->serverButton->setText("Stop server");
		ui->serverControls->setEnabled(false);
		connect(ui->serverButton, SIGNAL(clicked(bool)), rs, SLOT(stopServer()));
		ui->clientGroupBox->setEnabled(false);

		ui->statusLabel->setText(QString(q_("Running as server on port %1")).arg(rs->getServerPort()));
	}
	else if(state == RemoteSync::CLIENT_CONNECTING)
	{
		ui->serverGroupBox->setEnabled(false);
		ui->clientGroupBox->setEnabled(false);

		ui->clientButton->setText(q_("Connecting..."));
		ui->statusLabel->setText(QString(q_("Connecting to %1:%2...")).arg(rs->getClientServerHost()).arg(rs->getClientServerPort()));
	}
	else if (state == RemoteSync::CLIENT)
	{
		ui->serverGroupBox->setEnabled(false);
		ui->clientGroupBox->setEnabled(true);
		ui->clientControls->setEnabled(false);

		ui->clientButton->setText(q_("Disconnect from server"));
		connect(ui->clientButton, SIGNAL(clicked(bool)), rs, SLOT(disconnectFromServer()));

		ui->statusLabel->setText(QString(q_("Connected to %1:%2")).arg(rs->getClientServerHost()).arg(rs->getClientServerPort()));
	}
}

void RemoteSyncDialog::setAboutHtml(void)
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Remote Sync Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + REMOTESYNC_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Florian Schaukowitsch</td></tr>";
	html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td>Georg Zotti</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Remote Control plugin provides state synchronization for multiple Stellarium instances running in a network.") + "</p>";
	// TODO Add longer instructions?

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Remote Sync plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/RemoteSync_plugin\">").arg("</a>") + "</li>";
	html += "</ul></p></body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=NULL)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}
