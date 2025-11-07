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
#include "StelLocaleMgr.hpp"
#endif

StelSpeechMgr::StelSpeechMgr()
{
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	m_speech=new QTextToSpeech(static_cast<QObject*>(this));
#ifdef ENABLE_NLS
	m_speech->setLocale(QLocale(StelApp::getInstance().getLocaleMgr().getAppLanguage()));
#else
	m_speech->setLocale(QLocale::English);
#endif
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
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
		m_speech->setRate(m_rate);
		m_speech->setPitch(m_pitch);
		m_speech->setVolume(m_volume);

		qDebug() << "StelSpeechMgr: App Language is"     << StelApp::getInstance().getLocaleMgr().getAppLanguage();
		qDebug() << "StelSpeechMgr: default engine name" << m_speech->engine();
		qDebug() << "StelSpeechMgr: default locale name" << m_speech->locale().name();
		qDebug() << "StelSpeechMgr: default voice name"  << m_speech->voice().name();

		connect(m_speech, &QTextToSpeech::engineChanged, this, [=](){
			qDebug() << "StelSpeechMgr:engineChanged() lambda";
			m_locales=m_speech->availableLocales();
			QString appLanguage=StelApp::getInstance().getLocaleMgr().getAppLanguage();
			qDebug() << "   StelSpeechMgr: App Language is" << appLanguage;
			if (m_locales.contains(QLocale(appLanguage)))
				m_speech->setLocale(QLocale(appLanguage));
			else
				m_speech->setLocale(QLocale::English);
			m_voices=m_speech->availableVoices();
		});

		QStringList availableEngines=m_speech->availableEngines();
		availableEngines.removeOne("mock"); // unhelpful dummy
		// we are only running when enabled(), i.e. at least the default engine is available.
		Q_ASSERT(!availableEngines.isEmpty());
		qDebug() << "StelSpeechMgr: available engine names:" << availableEngines.join(", ");
		// Problem: The sequence of engines is arbitrary and changes from call to call.
		// On Windows, there are winrt and sapi. Make sure winrt (usually better) is first.
#ifdef Q_OS_WIN
		availableEngines.sort();
		std::reverse(availableEngines.begin(), availableEngines.end());
#endif
		// TODO decide what to do on various Linux flavours (any preferrable engine?)

		// Only change from default engine if the configured is available.
		QString candEngine=conf->value("speech/engine", availableEngines.constFirst()).toString();
		if (availableEngines.contains(candEngine))
			m_speech->setEngine(candEngine);
		// This does not immediately trigger the connected lambda above. (Only processed in outer event loop...)
		QString appLanguage=StelApp::getInstance().getLocaleMgr().getAppLanguage();
		qDebug() << "StelSpeechMgr: App Language is" << appLanguage;
		m_locales=m_speech->availableLocales();
		if (m_locales.contains(QLocale(appLanguage)))
			m_speech->setLocale(QLocale(appLanguage));
		else
			m_speech->setLocale(QLocale::English);
		m_voices=m_speech->availableVoices();

		qDebug() << "StelSpeechMgr: current engine name" << m_speech->engine();
		qDebug() << "StelSpeechMgr: current locale name" << m_speech->locale().name();

		qDebug() << "StelSpeechMgr: available voices after these changes:";
		for (const QVoice &v: std::as_const(m_voices))
		{
			qDebug() << "    " << v.name() << "age:" << v.age() << "gender:" << v.gender() << "language:" <<  v.language() << "locale:" << v.locale();
		}

		QList<QVoice> voices=m_speech->findVoices(conf->value("speech/voice", m_voices.first().name()).toString());
		m_speech->setVoice(voices.constFirst());
		qDebug() << "StelSpeechMgr: current voice name"  << m_speech->voice().name();



#endif

	}
	else
		qWarning() << "Cannot Initialize Text to Speech";
}

bool StelSpeechMgr::enabled() const
{
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	return (m_speech && m_speech->state()!=QTextToSpeech::State::Error);
#else
	return false;
#endif
}

#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
QTextToSpeech::State StelSpeechMgr::getState()
{
	if (m_speech)
		return m_speech->state();
	return QTextToSpeech::State::Error;
}
#endif

void StelSpeechMgr::say(const QString &narration) const
{
	qDebug() << "StelSpeechMgr: reading: " << narration;
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	if (enabled())
		m_speech->say(narration);
#endif
}

void StelSpeechMgr::stop()
{
	qDebug() << "StelSpeechMgr: stop() ";
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	if (enabled())
	{
		m_speech->stop(QTextToSpeech::BoundaryHint::Word);
		m_speech->say(qc_("Why? You asked for it!", "Object narration")); // Just testing ... ;-)
	}
#endif
}

void StelSpeechMgr::setRate(double rate)
{
	m_rate=rate;
	StelApp::immediateSave("speech/rate", rate);
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	if (m_speech)
		m_speech->setRate(rate);
#endif
	emit rateChanged(rate);
}

void StelSpeechMgr::setPitch(double pitch)
{
	m_pitch=pitch;
	StelApp::immediateSave("speech/pitch", pitch);
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	if (m_speech)
		m_speech->setPitch(pitch);
#endif
	emit pitchChanged(pitch);
}

void StelSpeechMgr::setVolume(double volume)
{
	m_volume=volume;
	StelApp::immediateSave("speech/volume", volume);
#if defined(ENABLE_MEDIA) && (QT_VERSION>=QT_VERSION_CHECK(6,6,0))
	if (m_speech)
		m_speech->setVolume(volume);
#endif
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

//! Retrieve the currently active flags which information bits to narrate
const StelObject::InfoStringGroup& StelSpeechMgr::getNarrationTextFilters() const
{
	return flags;
}

//! Set the currently active flags which information bits to narrate
void StelSpeechMgr::setNarrationTextFilters(const StelObject::InfoStringGroup &flags)
{
	this->flags=flags;
}
