/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QUrl>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>
#include <cmath>
#include <limits>

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "StelLocaleMgr.hpp"
#include "GreatRedSpotDialog.hpp"

#include "ui_greatRedSpotDialog.h"

GreatRedSpotDialog::GreatRedSpotDialog() : StelDialog("GreatRedSpot")
{
	ui = new Ui_GreatRedSpotDialogForm;
}

GreatRedSpotDialog::~GreatRedSpotDialog()
{
	if (updateReply)
	{
		updateReply->disconnect(this);
		updateReply->abort();
		updateReply->deleteLater();
	}
	delete ui;
	ui=Q_NULLPTR;
}

void GreatRedSpotDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateRecentGrsMeasurementLink();
		setUpdateError(updateError, updateErrorDetails);
	}
}

void GreatRedSpotDialog::createDialogContent()
{
	ui->setupUi(dialog);

	// Accept link-label presses so the graphics proxy delivers the release that activates the link.
	const auto link = ui->recentGrsMeasurementLabel;
	link->setTextInteractionFlags(link->textInteractionFlags() | Qt::TextSelectableByMouse);

	updateRecentGrsMeasurementLink();
	ui->updateErrorLabel->hide();
	
	//Signals and slots
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &GreatRedSpotDialog::retranslate);
	connect(ui->titleBar, &TitleBar::closeClicked, this, &StelDialog::close);
	connect(ui->titleBar, &TitleBar::movedTo, this, &GreatRedSpotDialog::handleMovedTo);

	SolarSystem* ss = GETSTELMODULE(SolarSystem);
	connectIntProperty(ui->longitudeSpinBox, "SolarSystem.grsLongitude");
	connectDoubleProperty(ui->driftDoubleSpinBox, "SolarSystem.grsDrift");

	const StelLocaleMgr& locmgr = StelApp::getInstance().getLocaleMgr();
	QString fmt = QString("%1 hh:mm").arg(locmgr.getQtDateFormatStr());
	ui->jdDateTimeEdit->setDisplayFormat(fmt);
	ui->jdDateTimeEdit->setTimeSpec(Qt::UTC);
	ui->jdDateTimeEdit->setDateTime(StelUtils::jdToQDateTime(ss->getGrsJD(), Qt::UTC));
	connect(ui->jdDateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &GreatRedSpotDialog::setGrsJD);

	connect(ui->updateGrsPushButton, &QPushButton::clicked, this, &GreatRedSpotDialog::updateGrsParameters);
}

void GreatRedSpotDialog::setGrsJD(QDateTime dt)
{
	GETSTELMODULE(SolarSystem)->setGrsJD(StelUtils::qDateTimeToJd(dt));
}

void GreatRedSpotDialog::updateRecentGrsMeasurementLink()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	const QString url = conf->value("astro/grs_measurements_url", "https://jupos.hier-im-netz.de/rGrs.htm").toString();
	ui->recentGrsMeasurementLabel->setText(QString("<a href=\"%1\">%2</a>")
		.arg(url.toHtmlEscaped(), q_("View recent GRS measurements").toHtmlEscaped()));
}

void GreatRedSpotDialog::setUpdateError(UpdateError error, const QString& details)
{
	updateError = error;
	updateErrorDetails = details;
	QString text;
	switch (error)
	{
		case UpdateError::None:
			break;
		case UpdateError::UnsupportedDate:
			text = q_("The simulation date is not supported by this dialog. No request was sent.");
			break;
		case UpdateError::Network:
			text = q_("Could not contact the GRS service: %1").arg(details);
			break;
		case UpdateError::Timeout:
			text = q_("The GRS update timed out. Please try again.");
			break;
		case UpdateError::Service:
			text = q_("The GRS service could not provide data for the requested date: %1").arg(details);
			break;
		case UpdateError::InvalidData:
			text = q_("The GRS service returned invalid or incomplete parameters: %1")
				.arg(details);
			break;
	}
	ui->updateErrorLabel->setText(text);
	ui->updateErrorLabel->setVisible(error != UpdateError::None);
}

void GreatRedSpotDialog::updateGrsParameters()
{
	if (updateReply)
		return;

	setUpdateError(UpdateError::None);
	// Capture UT once: the simulation may continue running while the request is in flight.
	const double jd = StelApp::getInstance().getCore()->getJD();
	// Check the range before converting: jdToQDateTime() asserts for dates such as year zero.
	const double minimumJD = StelUtils::qDateTimeToJd(ui->jdDateTimeEdit->minimumDateTime());
	const double maximumJD = StelUtils::qDateTimeToJd(ui->jdDateTimeEdit->maximumDateTime());
	if (!std::isfinite(jd) || jd < minimumJD || jd > maximumJD)
	{
		setUpdateError(UpdateError::UnsupportedDate);
		return;
	}
	const QDateTime epoch = StelUtils::jdToQDateTime(jd, Qt::UTC);

	QUrl url("https://ssp.imcce.fr/webservices/vo-tools/jgrs/");
	QUrlQuery query;
	query.addQueryItem("q", "ephemeris");
	query.addQueryItem("-ep", QString::number(jd - 365.25, 'f', 8));
	// Measure the change over the preceding Julian year. Intermediate samples unwrap the longitude
	// and keep the sampling step below the service's 100-day limit.
	query.addQueryItem("-nbd", "5");
	query.addQueryItem("-step", "91.3125d");
	query.addQueryItem("-mime", "json");
	query.addQueryItem("-output", "--jd");
	query.addQueryItem("-from", "Stellarium");
	url.setQuery(query);
	QNetworkRequest request(url);
	request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
	request.setRawHeader("User-Agent", StelUtils::getUserAgentString().toUtf8());

	ui->updateGrsPushButton->setEnabled(false);
	updateTimedOut = false;
	const auto reply = StelApp::getInstance().getNetworkAccessManager()->get(request);
	updateReply = reply;
	const auto timer = new QTimer(reply);
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, this, [this, reply]{
		updateTimedOut = true;
		reply->abort();
	});
	connect(reply, &QNetworkReply::finished, this, [this, reply, jd, epoch, timer]{
		timer->stop();
		updateReply.clear();
		reply->deleteLater();
		ui->updateGrsPushButton->setEnabled(true);
		if (updateTimedOut)
		{
			setUpdateError(UpdateError::Timeout);
			return;
		}

		QJsonParseError parseError;
		const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
		const QJsonObject response = document.object();
		// IMCCE may return an error document together with an HTTP error status.
		if ((response.contains("flag") && response.value("flag").toInt() != 1) ||
		    !response.value("message").toString().isEmpty())
		{
			setUpdateError(UpdateError::Service, response.value("message").toString());
			return;
		}
		if (reply->error() != QNetworkReply::NoError)
		{
			setUpdateError(UpdateError::Network, reply->errorString());
			return;
		}
		if (parseError.error != QJsonParseError::NoError)
		{
			setUpdateError(UpdateError::InvalidData,
			               q_("Could not parse the response as JSON: %1").arg(parseError.errorString()));
			return;
		}
		if (!document.isObject())
		{
			setUpdateError(UpdateError::InvalidData, q_("Expected a JSON object."));
			return;
		}
		if (!response.value("data").isArray())
		{
			setUpdateError(UpdateError::InvalidData,
			               q_("The response does not contain an ephemeris data array."));
			return;
		}

		const QJsonArray data = response.value("data").toArray();
		if (data.size() != 5)
		{
			setUpdateError(UpdateError::InvalidData,
			               q_("Expected 5 ephemeris samples, received %1.").arg(data.size()));
			return;
		}
		const QJsonObject first = data.at(0).toObject();
		constexpr double invalid = std::numeric_limits<double>::quiet_NaN();
		const double longitude = data.last().toObject().value("GRS long").toDouble(invalid);
		double previousLongitude = first.value("GRS long").toDouble(invalid);
		double drift = 0.0;
		// Five samples give four 91.3125-day intervals, within the API's 100-day step limit.
		// Sum the shortest signed changes between adjacent samples to track 0/360 crossings
		// throughout the year, rather than choosing a single shortest arc between the endpoints.
		// This assumes less than 180 degrees of motion per interval. The result is the net annual
		// change; the intermediate samples track wrapping, rather than contributing to a fit.
		for (int i = 0; i < data.size(); ++i)
		{
			const QJsonObject sample = data.at(i).toObject();
			const double sampleJD = sample.value("Date").toDouble(invalid);
			const double sampleLongitude = sample.value("GRS long").toDouble(invalid);
			if (!std::isfinite(sampleJD))
			{
				setUpdateError(UpdateError::InvalidData,
				               q_("Sample %1 has a missing or invalid date.").arg(i + 1));
				return;
			}
			const double expectedJD = jd - 365.25 + i * 91.3125;
			if (std::abs(sampleJD - expectedJD) > 1e-6)
			{
				setUpdateError(UpdateError::InvalidData,
				               q_("Sample %1 has Julian date %2; expected %3.")
				                   .arg(i + 1).arg(sampleJD, 0, 'f', 8).arg(expectedJD, 0, 'f', 8));
				return;
			}
			if (!std::isfinite(sampleLongitude))
			{
				setUpdateError(UpdateError::InvalidData,
				               q_("Sample %1 has a missing or invalid GRS longitude.").arg(i + 1));
				return;
			}
			if (sampleLongitude < 0.0 || sampleLongitude >= 360.0)
			{
				setUpdateError(UpdateError::InvalidData,
				               q_("Sample %1 has GRS longitude %2 degrees, outside [0, 360).")
				                   .arg(i + 1).arg(sampleLongitude));
				return;
			}
			drift += std::remainder(sampleLongitude - previousLongitude, 360.0);
			previousLongitude = sampleLongitude;
		}
		if (!std::isfinite(drift) || drift < ui->driftDoubleSpinBox->minimum() ||
		    drift > ui->driftDoubleSpinBox->maximum())
		{
			setUpdateError(UpdateError::InvalidData,
			               q_("Computed annual drift is %1 degrees/year; the supported range is %2 to %3.")
			                   .arg(drift).arg(ui->driftDoubleSpinBox->minimum())
			                   .arg(ui->driftDoubleSpinBox->maximum()));
			return;
		}

		// All parameters have been validated; apply the settings.
		ui->longitudeSpinBox->setValue(qRound(longitude) % 360);
		ui->driftDoubleSpinBox->setValue(drift);
		ui->jdDateTimeEdit->setDateTime(epoch);
	});
	timer->start(30000);
}
