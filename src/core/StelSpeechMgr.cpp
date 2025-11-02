/*
 * Copyright (C) 2025 Georg Zotti
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

#include "StelSpeechMgr.hpp"
#include "StelTranslator.hpp"
#include "StelApp.hpp"
#include <QDebug>

#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))

#include <QTextToSpeech>
#include <QVoice>
#endif

StelSpeechMgr::StelSpeechMgr()
{
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	m_speech=new QTextToSpeech(static_cast<QObject*>(this));
#else
	qWarning() << "Text to Speech requires Qt6.6 or higher";
#endif
}

StelSpeechMgr::~StelSpeechMgr()
{
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	m_voices.clear();
	if (m_speech)
	{
		m_speech->stop(QTextToSpeech::BoundaryHint::Immediate);
		delete m_speech;
		m_speech=nullptr;
	}
#endif
}

void StelSpeechMgr::init()
{
	QSettings *conf=StelApp::getInstance().getSettings();
	m_rate  =qBound(-1.0, conf->value("speech/rate",   0.0).toDouble(), 1.0);
	m_pitch =qBound(-1.0, conf->value("speech/pitch",  0.0).toDouble(), 1.0);
	m_volume=qBound( 0.0, conf->value("speech/volume", 50.0).toDouble(), 1.0);
	if (enabled())
	{
		m_speech->setRate(m_rate);
		m_speech->setPitch(m_pitch);
		m_speech->setVolume(m_volume);

		qDebug() << "StelSpeechMgr: engine name" << m_speech->engine();
		qDebug() << "StelSpeechMgr: locale name" << m_speech->locale().name();
		qDebug() << "StelSpeechMgr: voice name" << m_speech->voice().name();

		m_speech->setLocale(QLocale::English);

		QList<QVoice> voices = m_speech->availableVoices();
		qDebug() << "StelSpeechMgr: available voices:";
		for (const QVoice &v: voices)
		{
			qDebug() << v.name() << "age:" << v.age() << "gender:" << v.gender() << "language:" <<  v.language() << "locale:" << v.locale();
		}


	}
	else
		qWarning() << "Cannot Initialize Text to Speech";
}

bool StelSpeechMgr::enabled() const
{
	return (m_speech && m_speech->state()!=QTextToSpeech::State::Error);
}

QTextToSpeech::State StelSpeechMgr::getState()
{
	if (m_speech)
		return m_speech->state();
	return QTextToSpeech::State::Error;
}


void StelSpeechMgr::say(const QString &narration) const
{
	qDebug() << "StelSpeechMgr: reading: " << narration;
	if (enabled())
		m_speech->say(narration);
}

void StelSpeechMgr::stop()
{
	qDebug() << "StelSpeechMgr: stop() ";
	if (enabled())
	{
		m_speech->stop(QTextToSpeech::BoundaryHint::Word);
		m_speech->say(qc_("Why? You asked for it!", "Object narration")); // Just testing ... ;-)
	}
}

void StelSpeechMgr::setRate(double rate)
{
	m_rate=rate;
	StelApp::immediateSave("speech/rate", rate);
	if (m_speech)
		m_speech->setRate(rate);
	emit rateChanged(rate);
}

void StelSpeechMgr::setPitch(double pitch)
{
	m_pitch=pitch;
	StelApp::immediateSave("speech/pitch", pitch);
	if (m_speech)
		m_speech->setPitch(pitch);
	emit pitchChanged(pitch);
}

void StelSpeechMgr::setVolume(double volume)
{
	m_volume=volume;
	StelApp::immediateSave("speech/volume", volume);
	if (m_speech)
		m_speech->setVolume(volume);
	emit volumeChanged(volume);
}

#if 0
void StelSpeechMgr::engineSelected(QString &engineName)
{
    delete m_speech;
    m_speech = engineName == u"default"
	       ? new QTextToSpeech(this)
	       : new QTextToSpeech(engineName, this);

    // some engines initialize asynchronously
    if (m_speech->state() == QTextToSpeech::Ready) {
	onEngineReady();
    } else {
	connect(m_speech, &QTextToSpeech::stateChanged, this, &StelSpeechMgr::onEngineReady,
		Qt::SingleShotConnection);
    }
}

void StelSpeechMgr::selectLocale(QLocale locale)
{
	if (enabled())
		m_speech->setLocale(locale);
}

void StelSpeechMgr::selectVoice(int index)
{
	m_speech->setVoice(m_voices.at(index));
}

void StelSpeechMgr::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui.language->setCurrentIndex(ui.language->findData(localeVariant));

    QSignalBlocker blocker(ui.voice);

    ui.voice->clear();

    m_voices = m_speech->availableVoices();
    QVoice currentVoice = m_speech->voice();
    for (const QVoice &voice : std::as_const(m_voices)) {
	ui.voice->addItem(u"%1 - %2 - %3"_s
			  .arg(voice.name(), QVoice::genderName(voice.gender()),
			       QVoice::ageName(voice.age())));
	if (voice.name() == currentVoice.name())
	    ui.voice->setCurrentIndex(ui.voice->count() - 1);
    }
}
#endif

