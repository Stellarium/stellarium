/*
 * Stellarium: Meteor Showers Plug-in
 * Copyright (C) 2013-2015 Marcos Cardinot
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

#include <QColorDialog>

#include "MSConfigDialog.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "ui_MSConfigDialog.h"
#include "StelMainView.hpp"
MSConfigDialog::MSConfigDialog(MeteorShowersMgr* mgr)
	: StelDialog("MeteorShowers")
	, m_mgr(mgr)
	, m_ui(new Ui_MSConfigDialog)
{
}

MSConfigDialog::~MSConfigDialog()
{
	delete m_ui;
}

void MSConfigDialog::retranslate()
{
	if (dialog)
	{
		m_ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void MSConfigDialog::createDialogContent()
{
	m_ui->setupUi(dialog);
	m_ui->tabs->setCurrentIndex(0);

	// Kinetic scrolling
	kineticScrollingList << m_ui->about;
	StelGui* gui= dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui)
	{
		enableKineticScrolling(gui->getFlagUseKineticScrolling());
		connect(gui, SIGNAL(flagUseKineticScrollingChanged(bool)), this, SLOT(enableKineticScrolling(bool)));
	}

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(m_ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_ui->TitleBar, SIGNAL(movedTo(QPoint)), this, SLOT(handleMovedTo(QPoint)));
	connect(m_ui->bRestoreDefaults, SIGNAL(clicked()), this, SLOT(restoreDefaults()));

	// General tab
	connect(m_ui->enableAtStartUp, SIGNAL(clicked(bool)), m_mgr, SLOT(setEnableAtStartup(bool)));
	connect(m_ui->showEnableButton, SIGNAL(clicked(bool)), m_mgr, SLOT(setShowEnableButton(bool)));
	connect(m_ui->showSearchButton, SIGNAL(clicked(bool)), m_mgr, SLOT(setShowSearchButton(bool)));

	// Radiant tab
	connect(m_ui->enableMarker, SIGNAL(clicked(bool)), m_mgr, SLOT(setEnableMarker(bool)));
	connect(m_ui->activeRadiantsOnly, SIGNAL(clicked(bool)), m_mgr, SLOT(setActiveRadiantOnly(bool)));
	connect(m_ui->enableLabels, SIGNAL(clicked(bool)), m_mgr, SLOT(setEnableLabels(bool)));
	connect(m_ui->fontSize, SIGNAL(valueChanged(int)), m_mgr, SLOT(setFontSize(int)));

	connect(m_ui->setColorARG, SIGNAL(clicked()), this, SLOT(setColorARG()));
	connect(m_ui->setColorARC, SIGNAL(clicked()), this, SLOT(setColorARC()));
	connect(m_ui->setColorIR, SIGNAL(clicked()), this, SLOT(setColorIR()));

	// Update tab
	connect(m_ui->enableUpdates, SIGNAL(clicked(bool)), m_mgr, SLOT(setEnableAutoUpdates(bool)));
	connect(m_ui->updateFrequency, SIGNAL(valueChanged(int)), m_mgr, SLOT(setUpdateFrequencyHours(int)));
	connect(m_ui->bUpdate, SIGNAL(clicked()), m_mgr, SLOT(updateCatalog()));

	connect(m_ui->enableUpdates, SIGNAL(clicked()), this, SLOT(refreshUpdateTab()));
	connect(m_ui->updateFrequency, SIGNAL(valueChanged(int)), this, SLOT(refreshUpdateTab()));
	connect(m_ui->bUpdate, SIGNAL(clicked()), this, SLOT(refreshUpdateTab()));
	connect(m_mgr, SIGNAL(downloadStatusChanged(DownloadStatus)), this, SLOT(refreshUpdateTab()));

	// About tab
	setAboutHtml();
	if (gui)
	{
		m_ui->about->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}

	init();
}

void MSConfigDialog::restoreDefaults()
{
	if (askConfirmation())
	{
		qDebug() << "[MeteorShower] restore defaults...";
		m_mgr->restoreDefaultSettings();
	}
	else
		qDebug() << "[MeteorShower] restore defaults is canceled...";
}

void MSConfigDialog::init()
{
	// General tab
	m_ui->enableAtStartUp->setChecked(m_mgr->getEnableAtStartup());
	m_ui->showEnableButton->setChecked(m_mgr->getShowEnableButton());
	m_ui->showSearchButton->setChecked(m_mgr->getShowSearchButton());

	// Radiant tab
	m_ui->enableMarker->setChecked(m_mgr->getEnableMarker());
	m_ui->activeRadiantsOnly->setChecked(m_mgr->getActiveRadiantOnly());
	m_ui->enableLabels->setChecked(m_mgr->getEnableLabels());
	m_ui->fontSize->setValue(m_mgr->getFontSize());
	refreshMarkersColor();

	// Update tab
	refreshUpdateTab();
}

void MSConfigDialog::refreshUpdateTab()
{
	m_ui->enableUpdates->setChecked(m_mgr->getEnableAutoUpdates());
	m_ui->updateFrequency->setValue(m_mgr->getUpdateFrequencyHours());
	m_ui->nextUpdate->setDateTime(m_mgr->getNextUpdate());

	m_ui->lastUpdate->setDateTime(m_mgr->getLastUpdate());
	m_ui->bUpdate->setEnabled(true);

	QString msg;
	MeteorShowersMgr::DownloadStatus s = m_mgr->getStatusOfLastUpdate();
	if (s == MeteorShowersMgr::UPDATING)
	{
		msg = q_("Updating...");
		m_ui->bUpdate->setEnabled(false);
	}
	else if (s == MeteorShowersMgr::UPDATED)
	{
		msg = q_("Successfully updated!");
	}
	else if (s == MeteorShowersMgr::FAILED)
	{
		msg = q_("Failed!");
	}
	else
	{
		msg = q_("Outdated!");
	}
	m_ui->status->setText(msg);
}

void MSConfigDialog::refreshMarkersColor()
{
	Vec3f c = m_mgr->getColorARG();
	QColor color(QColor::fromRgbF(c[0], c[1], c[2]));
	m_ui->setColorARG->setStyleSheet("background-color:" + color.name() + ";");

	c = m_mgr->getColorARC();
	color = QColor(QColor::fromRgbF(c[0], c[1], c[2]));
	m_ui->setColorARC->setStyleSheet("background-color:" + color.name() + ";");

	c = m_mgr->getColorIR();
	color = QColor(QColor::fromRgbF(c[0], c[1], c[2]));
	m_ui->setColorIR->setStyleSheet("background-color:" + color.name() + ";");
}

void MSConfigDialog::setColorARG()
{
	Vec3f c = m_mgr->getColorARG();
	QColor color(QColor::fromRgbF(c[0], c[1], c[2]));
    color = QColorDialog::getColor(color,&StelMainView::getInstance());
	if (color.isValid())
	{
		m_ui->setColorARG->setStyleSheet("background-color:" + color.name() + ";");
		m_mgr->setColorARG(Vec3f(color.redF(), color.greenF(), color.blueF()));
	}
}

void MSConfigDialog::setColorARC()
{
	Vec3f c = m_mgr->getColorARC();
	QColor color(QColor::fromRgbF(c[0], c[1], c[2]));
    color = QColorDialog::getColor(color,&StelMainView::getInstance());
	if (color.isValid())
	{
		m_ui->setColorARC->setStyleSheet("background-color:" + color.name() + ";");
		m_mgr->setColorARC(Vec3f(color.redF(), color.greenF(), color.blueF()));
	}
}

void MSConfigDialog::setColorIR()
{
	Vec3f c = m_mgr->getColorIR();
	QColor color(QColor::fromRgbF(c[0], c[1], c[2]));
    color = QColorDialog::getColor(color,&StelMainView::getInstance());
	if (color.isValid())
	{
		m_ui->setColorIR->setStyleSheet("background-color:" + color.name() + ";");
		m_mgr->setColorIR(Vec3f(color.redF(), color.greenF(), color.blueF()));
	}
}

void MSConfigDialog::setAboutHtml()
{
	// Regexp to replace {text} with an HTML link.
	QRegExp a_rx = QRegExp("[{]([^{]*)[}]");

	QString html = "<html><head></head><body>"
	"<h2>" + q_("Meteor Showers Plug-in") + "</h2>"
	"<table width=\"90%\">"
		"<tr width=\"30%\">"
			"<td><strong>" + q_("Version") + ":</strong></td>"
			"<td>" + METEORSHOWERS_PLUGIN_VERSION + "</td>"
		"</tr>"
		"<tr>"
			"<td><strong>" + q_("License") + ":</strong></td>"
			"<td>" + METEORSHOWERS_PLUGIN_LICENSE + "</td>"
		"</tr>"
		"<tr>"
			"<td><strong>" + q_("Author") + ":</strong></td>"
			"<td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td>"
		"</tr>"
		"<tr>"
			"<td><strong>" + q_("Contributors") + ":</strong></td>"
			"<td>Alexander Wolf &lt;alex.v.wolf@gmail.com&gt;</td>"
		"</tr>"
	"</table>"
	"<p>"
	+ q_(
		"This plugin enables you to simulate periodic meteor showers and "
		"to display a marker for each active and inactive radiant."
	) +
	"</p>"
	"<p>"
	+ q_(
		"By a single click on the radiant's marker, you can see all the "
		"details about its position and activity. Most data used on this "
		"plugin comes from the official <a href=\"http://imo.net\">International "
		"Meteor Organization</a> catalog."
	) +
	"</p>"
	+ q_(
	"<p>"
		"It has three types of markers:"
		"<ul>"
			"<li>"
				"<b>Confirmed:</b> "
				"the radiant is active and its data was confirmed."
				" Thus, this is a historical (really occurred in the past) or predicted"
				" meteor shower."
			"</li>"
			"<li>"
				"<b>Generic:</b> "
				"the radiant is active, but its data was not confirmed."
				" It means that this can occur in real life, but that we do not have proper"
				" data about its activity for the current year."
			"</li>"
			"<li>"
				"<b>Inactive:</b> "
				"the radiant is inactive for the current sky date."
			"</li>"
		"</ul>"
	"</p>"
	) +
	"<h3>" + q_("Terms") + "</h3>"
	"<p><b>" + q_("Meteor shower") + "</b>"
		"<br />" +
		q_("A meteor shower is a celestial event in which a number of meteors are observed to "
		"radiate, or originate, from one point in the night sky. These meteors are caused by "
		"streams of cosmic debris called meteoroids entering Earth's atmosphere at extremely "
		"high speeds on parallel trajectories. Most meteors are smaller than a grain of sand, "
		"so almost all of them disintegrate and never hit the Earth's surface. Intense or "
		"unusual meteor showers are known as meteor outbursts and meteor storms, which may "
		"produce greater than 1,000 meteors an hour.") +
	"</p>"
	"<p><b>" + q_("Radiant") + "</b>"
		"<br />" +
		q_("The radiant or apparent radiant of a meteor shower is the point in the sky, from "
		   "which (to a planetary observer) meteors appear to originate. The Perseids, for "
		   "example, are meteors which appear to come from a point within the constellation "
		   "of Perseus.") +
	"</p>"
	"<p>" +
		q_("An observer might see such a meteor anywhere in the sky but the direction of motion, "
		   "when traced back, will point to the radiant. A meteor that does not point back to the "
		   "known radiant for a given shower is known as a sporadic and is not considered part of "
		   "that shower.") +
	"</p>"
	"<p>" + q_("Many showers have a radiant point that changes position during the interval when it "
		   "appears. For example, the radiant point for the Delta Aurigids drifts by more than a "
		   "degree per night.") +
	"</p>"
	"<p><b>" + q_("Zenithal Hourly Rate (ZHR)") + "</b>"
		"<br />" +
		q_("In astronomy, the Zenithal Hourly Rate (ZHR) of a meteor shower is the number of meteors "
		   "a single observer would see in one hour under a clear, dark sky (limiting apparent "
		   "magnitude of 6.5) if the radiant of the shower were at the zenith. The rate that can "
		   "effectively be seen is nearly always lower and decreases the closer the radiant is to "
		   "the horizon.") +
	"</p>"
	"<p><b>" + q_("Population index") + "</b>"
		"<br />" +
		q_("The population index indicates the magnitude distribution of the meteor showers. The "
		   "values below 2.5 correspond to distributions where bright meteors are more frequent "
		   "than average, while values above 3.0 mean that the share of faint meteors is larger "
		   "than usual.") +
	"</p>"
	"<h3>" + q_("Notes") + "</h3>"
	"<p>" + q_("This plugin was initially created as a project of the ESA Summer of Code in Space 2013.") + "</p>"
	"<h3>" + q_("Info") + "</h3>"
	"<p>" + q_("Info about meteor showers you can get here:") + "</p>"
	"<ul>"
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	"<li>" + QString(q_("%1Meteor shower%2 - article in Wikipedia").arg("<a href=\"https://en.wikipedia.org/wiki/Meteor_Showers\">")).arg("</a>") + "</li>"
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	"<li>" + QString(q_("%1International Meteor Organization%2").arg("<a href=\"http://www.imo.net/\">")).arg("</a>") + "</li>"
	"</ul>"
	"<h3>" + q_("Links") + "</h3>"
	"<p>" + QString(q_("Support is provided via the Github website. Be sure to put \"%1\" in the subject when posting.")).arg("Meteor Showers Plugin") + "</p>";
	html += "<p><ul>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you have a question, you can {get an answer here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://groups.google.com/forum/#!forum/stellarium\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("Bug reports and feature requests can be made {here}.").toHtmlEscaped().replace(a_rx, "<a href=\"https://github.com/Stellarium/stellarium/issues\">\\1</a>") + "</li>";
	// TRANSLATORS: The text between braces is the text of an HTML link.
	html += "<li>" + q_("If you want to read full information about this plugin and its history, you can {get info here}.").toHtmlEscaped().replace(a_rx, "<a href=\"http://stellarium.sourceforge.net/wiki/index.php/Meteor_Showers_plugin\">\\1</a>") + "</li>";
	html += "</ul></p></body></html>";

	m_ui->about->setHtml(html);
	// TRANSLATORS: duration
	m_ui->updateFrequency->setSuffix(qc_(" h","time unit"));
}
