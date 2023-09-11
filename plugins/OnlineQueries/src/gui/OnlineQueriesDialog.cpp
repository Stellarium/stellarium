/*
 * Stellarium OnlineQueries Plug-in
 *
 * Copyright (C) 2020-21 Georg Zotti
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

#include "OnlineQueriesDialog.hpp"
#include "OnlineQueries.hpp"
#include "StelWebEngineView.hpp"

#include <QPushButton>
#include <QDesktopServices>
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

OnlineQueriesDialog::OnlineQueriesDialog(QObject* parent) :
	StelDialogSeparate("OnlineQueries", parent),
	plugin(nullptr),
	view(nullptr)
{
	setObjectName("OnlineQueriesDialog");
	ui = new Ui_onlineQueriesDialogForm;
}

OnlineQueriesDialog::~OnlineQueriesDialog()
{
	delete ui;
	if (view) delete view;
}

void OnlineQueriesDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void OnlineQueriesDialog::createDialogContent()
{
	plugin = GETSTELMODULE(OnlineQueries);
	Q_ASSERT(plugin);

	//load UI from form file
	ui->setupUi(dialog);
	// Given possible unavailability of QtWebEngine on some platforms,
	// we must add this dynamically here. In addition, there are platforms where QtWebEngine appears to be available,
	// but fails to initialize properly at runtime. We therefore can only add this as QWidget.
	// If manually disabled, we use a QTextBrowser.
	if (plugin->webEngineDisabled())
	{
		view = new QTextBrowser();
	}
	else
	{
		view = new StelWebEngineView();
	}
	Q_ASSERT(view);
	view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	view->setMinimumSize(0, 180);
	ui->verticalLayout->addWidget(view);

	//hook up retranslate event
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	//connect UI events
	connect(ui->closeStelWindow, &QPushButton::clicked, plugin, [=]{ plugin->setEnabled(false);});
	connect(ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));

	// Kinetic scrolling and style sheet for output
	kineticScrollingList << view;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}
	setAboutHtml();

	connect(ui->wikipediaPushButton,    SIGNAL(clicked()), plugin, SLOT(queryWikipedia()));
	connect(ui->aavsoPushButton,        SIGNAL(clicked()), plugin, SLOT(queryAAVSO()));
	connect(ui->gcvsPushButton,         SIGNAL(clicked()), plugin, SLOT(queryGCVS()));
	// Unfortunately, Ancient-Skies did not resurface so far. Keep this here, maybe re-activate ~2025.
	//connect(ui->ancientSkiesPushButton, SIGNAL(clicked()), plugin, SLOT(queryAncientSkies()));
	ui->ancientSkiesPushButton->hide();
	// set custom tab buttons to hostnames, or deactivate unconfigured buttons
	if (!plugin->getCustomUrl1().isEmpty())
	{
		ui->custom1PushButton->setText(QUrl(plugin->getCustomUrl1()).host());
		connect(ui->custom1PushButton, SIGNAL(clicked()), plugin, SLOT(queryCustomSite1()));
	}
	else {
		ui->custom1PushButton->setText(qc_("(Custom 1)", "GUI label"));
		ui->custom1PushButton->setEnabled(false);
	}
	if (!plugin->getCustomUrl2().isEmpty())
	{
		ui->custom2PushButton->setText(QUrl(plugin->getCustomUrl2()).host());
		connect(ui->custom2PushButton, SIGNAL(clicked()), plugin, SLOT(queryCustomSite2()));
	}
	else {
		ui->custom2PushButton->setText(qc_("(Custom 2)", "GUI label"));
		ui->custom2PushButton->setEnabled(false);
	}
	if (!plugin->getCustomUrl3().isEmpty())
	{
		ui->custom3PushButton->setText(QUrl(plugin->getCustomUrl3()).host());
		connect(ui->custom3PushButton, SIGNAL(clicked()), plugin, SLOT(queryCustomSite3()));
	}
	else {
		ui->custom3PushButton->setText(qc_("(Custom 3)", "GUI label"));
		ui->custom3PushButton->setEnabled(false);
	}

#ifdef WITH_QTWEBENGINE
	if (plugin->webEngineDisabled())
	{
		ui->backPushButton->hide();
		ui->forwardPushButton->hide();
	}
	else
	{
		connect(ui->backPushButton,    &QPushButton::clicked, this, [=]{static_cast<StelWebEngineView*>(view)->triggerPageAction(QWebEnginePage::Back);});
		connect(ui->forwardPushButton, &QPushButton::clicked, this, [=]{static_cast<StelWebEngineView*>(view)->triggerPageAction(QWebEnginePage::Forward);});
	}
#else
	ui->backPushButton->hide();
	ui->forwardPushButton->hide();
#endif
}

void OnlineQueriesDialog::setOutputHtml(QString html) const
{
	if (plugin->webEngineDisabled())
	{
		static_cast<QTextBrowser*>(view)->setHtml(html);
	}
	else
	{
		static_cast<StelWebEngineView*>(view)->setHtml(html);
	}
}

void OnlineQueriesDialog::setOutputUrl(QUrl url) const
{
#ifdef WITH_QTWEBENGINE
	if (plugin->webEngineDisabled())
	{
		QDesktopServices::openUrl(url);
		static_cast<QTextBrowser*>(view)->setHtml(QString("<p>" + qc_("Opened %1 in your web browser", "OnlineQueries") + "</p>").arg(url.host()));
	}
	else
	{
		static_cast<StelWebEngineView*>(view)->setUrl(url);
	}
#else
	QDesktopServices::openUrl(url);
	static_cast<StelWebEngineView*>(view)->setHtml(QString("<p>" + qc_("Opened %1 in your web browser", "OnlineQueries") + "</p>").arg(url.host()));
#endif
}

void OnlineQueriesDialog::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("OnlineQueries Plug-in") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ONLINEQUERIES_PLUGIN_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("License") + ":</strong></td><td>" + ONLINEQUERIES_PLUGIN_LICENSE + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Georg Zotti</td></tr>";
	//html += "<tr><td><strong>" + q_("Contributors") + ":</strong></td><td> List with br separators </td></tr>";
	html += "</table>";

	html += "<p>" + q_("The OnlineQueries plugin provides an interface to various online sources for astronomical information.") + "</p>";
	html += "<ul><li>" + q_("Wikipedia, the free online encyclopedia") + "</li>";
	html += "<li>" + q_("AAVSO, the International Variable Star Index of the American Association for Variable Star Observers") + "</li>";
	html += "<li>" + q_("GCVS, the General Catalogue of Variable Stars of the Sternberg Astronomical Institute and the Institute of Astronomy of the Russian Academy of Sciences in Moscow") + "</li>";
	// Unfortunately, the website this plugin was made for has not come up again. Maybe later, though.
	//html += "<li>" + q_("Ancient-Skies, a private project which collects information about star names and their mythologies") + "</li>";
	html += "<li>" + q_("3 custom websites of your choice") + "</li>";
	html += "</ul>";
	html += "<p>" + q_("Regardless of the current program language, the result is always presented in English or the language of the respective website.") + "</p>";

	html += "<h3>" + q_("Publications") + "</h3>";
	html += "<p>"  + q_("If you use this plugin in your publications, please cite:") + "</p>";
	html += "<ul>";
	html += "<li>" + QString("Georg Zotti, Susanne M. Hoffmann, Doris Vickers, Rüdiger Schultz, Alexander Wolf: Revisiting Star Names: Stellarium and the Ancient Skies Database. "
				 "In: P. Maglova & Alexey Stoev (eds.). Cultural Astronomy & Ancient Skywatching. Proc. SEAC2021, Plovdiv 2023.").toHtmlEscaped() + "</li>";
	html += "</ul>";

	html += StelApp::getInstance().getModuleMgr().getStandardSupportLinksInfo("OnlineQueries plugin");
	html += "</body></html>";

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if(gui!=nullptr)
	{
		QString htmlStyleSheet(gui->getStelStyle().htmlStyleSheet);
		ui->aboutTextBrowser->document()->setDefaultStyleSheet(htmlStyleSheet);
	}
	ui->aboutTextBrowser->setHtml(html);
}
