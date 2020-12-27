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

#include <QHostInfo>
#include "RemoteSync.hpp"
#include "RemoteSyncDialog.hpp"
#include "ui_remoteSyncDialog.h"

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModule.hpp"
#include "StelModuleMgr.hpp"
#include "StelPropertyMgr.hpp"

RemoteSyncDialog::RemoteSyncDialog()
	: StelDialog("RemoteSync")
	, rs(Q_NULLPTR)
{
	ui = new Ui_remoteSyncDialog();
}

RemoteSyncDialog::~RemoteSyncDialog()
{
	delete ui; ui=Q_NULLPTR;
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

	// Kinetic scrolling
	kineticScrollingList << ui->aboutTextBrowser;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	ui->pushButtonSelectProperties->setText(QChar(0x2192));
	ui->pushButtonDeselectProperties->setText(QChar(0x2190));

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	connect(rs, SIGNAL(stateChanged(RemoteSync::SyncState)), this, SLOT(updateState()));
	updateState();

	connect(rs, SIGNAL(errorOccurred(QString)), this, SLOT(printErrorMessage(QString)));

	ui->clientServerHostEdit->setText(rs->getClientServerHost());
	connect(ui->clientServerHostEdit, SIGNAL(textChanged(QString)), rs, SLOT(setClientServerHost(QString)));
	ui->clientServerPortSpinBox->setValue(rs->getClientServerPort());
	connect(ui->clientServerPortSpinBox, SIGNAL(valueChanged(int)), rs, SLOT(setClientServerPort(int)));

	ui->serverPortSpinBox->setValue(rs->getServerPort());
	connect(ui->serverPortSpinBox, SIGNAL(valueChanged(int)), rs, SLOT(setServerPort(int)));

	ui->comboBoxClientServerQuits->setModel(ui->comboBoxClientConnectionLost->model());
	ui->comboBoxClientConnectionLost->setCurrentIndex(rs->getConnectionLostBehavior());
	ui->comboBoxClientServerQuits->setCurrentIndex(rs->getQuitBehavior());
	connect(ui->comboBoxClientConnectionLost, SIGNAL(activated(int)), this, SLOT(setConnectionLostBehavior(int)));
	connect(rs, &RemoteSync::connectionLostBehaviorChanged, ui->comboBoxClientConnectionLost, &QComboBox::setCurrentIndex);
	connect(ui->comboBoxClientServerQuits, SIGNAL(activated(int)), this, SLOT(setQuitBehavior(int)));
	connect(rs, &RemoteSync::quitBehaviorChanged, ui->comboBoxClientServerQuits, &QComboBox::setCurrentIndex);

	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionTime, SyncClient::SyncTime);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionLocation, SyncClient::SyncLocation);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionSelection, SyncClient::SyncSelection);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionStelProperty, SyncClient::SyncStelProperty);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionView, SyncClient::SyncView);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxOptionFov, SyncClient::SyncFov);
	ui->buttonGroupSyncOptions->setId(ui->checkBoxExcludeGUIProps, SyncClient::SkipGUIProps);
	updateCheckboxesFromSyncOptions();
	connect(rs, SIGNAL(clientSyncOptionsChanged(SyncClient::SyncOptions)), this, SLOT(updateCheckboxesFromSyncOptions()));
	connect(ui->buttonGroupSyncOptions, SIGNAL(buttonToggled(int,bool)), this, SLOT(checkboxToggled(int,bool)));

	connect(ui->saveSettingsButton, SIGNAL(clicked()), rs, SLOT(saveSettings()));	
	connect(ui->restoreDefaultsButton, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	populateExclusionLists();
	connect(ui->pushButtonSelectProperties, SIGNAL(clicked()), this, SLOT(addPropertiesForExclusion()));
	connect(ui->pushButtonDeselectProperties, SIGNAL(clicked()), this, SLOT(removePropertiesForExclusion()));

	setAboutHtml();
}

void RemoteSyncDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qCDebug(remoteSync) << "restore defaults...";
		rs->restoreDefaultSettings();
	}
	else
		qCDebug(remoteSync) << "restore defaults is canceled...";
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
	disconnect(ui->serverButton, SIGNAL(clicked(bool)), Q_NULLPTR, Q_NULLPTR);
	disconnect(ui->clientButton, SIGNAL(clicked(bool)), Q_NULLPTR, Q_NULLPTR);
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
		updateIPlabel(false);
	}
	else if (state == RemoteSync::SERVER)
	{
		ui->serverButton->setText(q_("Stop server"));
		ui->serverControls->setEnabled(false);
		connect(ui->serverButton, SIGNAL(clicked(bool)), rs, SLOT(stopServer()));
		ui->clientGroupBox->setEnabled(false);

		ui->statusLabel->setText(QString(q_("Running as server on port %1")).arg(rs->getServerPort()));
		updateIPlabel(true);
	}
	else
	{
		connect(ui->clientButton, SIGNAL(clicked(bool)), rs, SLOT(disconnectFromServer()));

		ui->serverGroupBox->setEnabled(false);
		ui->clientGroupBox->setEnabled(true);
		ui->clientControls->setEnabled(false);
		updateIPlabel(false);

		if(state == RemoteSync::CLIENT_CONNECTING)
		{
			ui->clientButton->setText(q_("Cancel connecting"));
			ui->statusLabel->setText(QString(q_("Connecting to %1: %2...")).arg(rs->getClientServerHost()).arg(rs->getClientServerPort()));
		}
		else if (state == RemoteSync::CLIENT_WAIT_RECONNECT)
		{
			ui->clientButton->setText(q_("Cancel connecting"));
			ui->statusLabel->setText(QString(q_("Retrying connection to %1: %2...")).arg(rs->getClientServerHost()).arg(rs->getClientServerPort()));
		}
		else if (state == RemoteSync::CLIENT_CLOSING)
		{
			ui->clientGroupBox->setEnabled(false);

			ui->clientButton->setText(q_("Disconnecting..."));
			ui->statusLabel->setText(q_("Disconnecting..."));
		}
		else if (state == RemoteSync::CLIENT)
		{
			ui->clientButton->setText(q_("Disconnect from server"));
			ui->statusLabel->setText(QString(q_("Connected to %1: %2")).arg(rs->getClientServerHost()).arg(rs->getClientServerPort()));
		}
	}
}

void RemoteSyncDialog::setAboutHtml(void)
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Remote Sync Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + REMOTESYNC_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + REMOTESYNC_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td rowspan=2><strong>" + q_("Authors") + ":</strong></td><td>Florian Schaukowitsch</td></tr>";
	html += "<tr><td>Georg Zotti</td></tr>";
	html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td>Alexander Wolf</td></tr>";
	html += "</table>";

	html += "<p>" + q_("The Remote Sync plugin provides state synchronization for multiple Stellarium instances running in a network.") + "</p>";
	html += "<p>" + q_("This can be used, for example, to create multi-screen setups using multiple physical PCs.") + "</p>";
	html += "<p>" + q_("Partial synchronization allows parallel setups of e.g. overview and detail views.") + "</p>";
	html += "<p>" + q_("See manual for detailed description.") + "</p>";
	html += "<p>" + q_("This plugin was developed during ESA SoCiS 2015&amp;2016.") + "</p>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Github website.  Be sure to put \"%1\" in the subject when posting.")).arg("Remote Sync plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/RemoteSync_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";


	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=Q_NULLPTR)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}

void RemoteSyncDialog::updateIPlabel(bool running)
{
	if (running)
	{
		QString localHostName=QHostInfo::localHostName();
		QHostInfo hostInfo = QHostInfo::fromName(localHostName);
		QString ipString("");
		for (auto a : hostInfo.addresses())
		{
			if ((a.protocol() == QAbstractSocket::IPv4Protocol) && a != QHostAddress(QHostAddress::LocalHost))
			{
				ipString += a.toString() + " ";
				continue;
			}
		}
		QString info = QString("%1 %2").arg(q_("Server Name"), localHostName);
		if (!ipString.isEmpty())
			info.append(QString("(IP: %1)").arg(ipString));
		ui->label_serverName->setText(info);
		//ui->label_RemoteRunningState->show();
	}
	else
	{
		ui->label_serverName->setText(q_("Server not active."));
		// Maybe even hide the label?
		//ui->label_RemoteRunningState->hide();
	}
}

void RemoteSyncDialog::updateCheckboxesFromSyncOptions()
{
	SyncClient::SyncOptions options = rs->getClientSyncOptions();

	for (auto* bt : ui->buttonGroupSyncOptions->buttons())
	{
		int id = ui->buttonGroupSyncOptions->id(bt);
		bt->setChecked(options & id);
	}
}

void RemoteSyncDialog::checkboxToggled(int id, bool state)
{
	SyncClient::SyncOptions options = rs->getClientSyncOptions();
	SyncClient::SyncOption enumVal = static_cast<SyncClient::SyncOption>(id);
	//toggle flag
	options = state ? (options|enumVal) : (options&~enumVal);
	rs->setClientSyncOptions(options);
}

void RemoteSyncDialog::populateExclusionLists()
{
	ui->listWidgetAllProperties->clear();
	ui->listWidgetSelectedProperties->clear();

	QStringList excluded=rs->getStelPropFilter();
	excluded.removeOne(""); // Special case
	ui->listWidgetSelectedProperties->addItems(excluded);
	QStringList allProps=StelApp::getInstance().getStelPropertyManager()->getPropertyList();
	for (auto str : excluded)
	{
		allProps.removeOne(str);
	}	
	ui->listWidgetAllProperties->addItems(allProps);

	ui->listWidgetAllProperties->sortItems();
	ui->listWidgetSelectedProperties->sortItems();
}

void RemoteSyncDialog::addPropertiesForExclusion()
{
	QStringList strings;
	if (ui->listWidgetAllProperties->selectedItems().length()>0)
	{
		for (const auto* item : ui->listWidgetAllProperties->selectedItems())
		{
			strings.append(item->text());
		}
		// Now we have 	a stringlist with properties to be added.

		QStringList currentFilter=rs->getStelPropFilter();
		// Add the selected to currentFilter...
		currentFilter=currentFilter+strings;

		// ...and activate new selection
		rs->setStelPropFilter(currentFilter);

		// update lists.
		populateExclusionLists();
	}
}

void RemoteSyncDialog::removePropertiesForExclusion()
{
	QStringList strings;
	if (ui->listWidgetSelectedProperties->selectedItems().length()>0)
	{
		for (const auto* item : ui->listWidgetSelectedProperties->selectedItems())
		{
			strings.append(item->text());
		}
		// Now we have 	a stringlist with properties to be removed.

		QStringList currentFilter=rs->getStelPropFilter();
		// Remove the selected from currentFilter...
		for (auto str : strings)
		{
			currentFilter.removeOne(str);
		}
		// and activate new selection
		rs->setStelPropFilter(currentFilter);

		// update lists.
		populateExclusionLists();
	}
}

void RemoteSyncDialog::setConnectionLostBehavior(int idx)
{
	rs->setConnectionLostBehavior(static_cast<RemoteSync::ClientBehavior>(idx));
}

void RemoteSyncDialog::setQuitBehavior(int idx)
{
	rs->setQuitBehavior(static_cast<RemoteSync::ClientBehavior>(idx));
}
