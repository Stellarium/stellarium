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
#include "StelApp.hpp"
#include <QDebug>

#ifdef ENABLE_SPEECH

#include <random>
#include <chrono>
#include <QVoice>
#include "StelLocaleMgr.hpp"
#include "StelTranslator.hpp"
#endif

Q_LOGGING_CATEGORY(Speech,"stel.Speech", QtDebugMsg) // TODO: Before merge, demote to QtInfoMsg


StelSpeechMgr::StelSpeechMgr(): StelModule()
#ifdef ENABLE_SPEECH
      , m_speech(nullptr)
#endif
{
	setObjectName("StelSpeechMgr");

	// Start without engine, then set the right one in init()
#ifndef ENABLE_SPEECH
	qCInfo(Speech) << "Text to Speech requires Qt6.4 or higher";
#endif
}

StelSpeechMgr::~StelSpeechMgr()
{
#ifdef ENABLE_SPEECH
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
	initNarrationFlagsFromConfig(conf);
	m_rate  =qBound(-1.0, conf->value("speech/rate",   0.0).toDouble(), 1.0);
	m_pitch =qBound(-1.0, conf->value("speech/pitch",  0.0).toDouble(), 1.0);
	m_volume=qBound( 0.0, conf->value("speech/volume", 0.5).toDouble(), 1.0);

#ifdef ENABLE_SPEECH
	QStringList availableEngines = QTextToSpeech::availableEngines();
	availableEngines.removeOne("mock"); // remove unhelpful dummy
	// we are only running when enabled(), i.e. at least the default engine is available.
	// If not, just keep the module disabled.
	if (availableEngines.isEmpty())
	{
		qCWarning(Speech) << "No Speech engine found. Disabling StelSpeechMgr.";
		return;
	}
	Q_ASSERT(!availableEngines.isEmpty());

	qCDebug(Speech) << "StelSpeechMgr:::init(): available engine names:" << availableEngines.join(", ");
	// Problem: The sequence of engines is arbitrary and changes from call to call.
	// On Windows, there are winrt and sapi. Make sure winrt (usually better) is first.
#ifdef Q_OS_WIN
	availableEngines.sort();
	std::reverse(availableEngines.begin(), availableEngines.end());
#endif
	// TODO decide what to do on various Linux flavours (any preferrable engine?)

	// Only change from default engine if the configured is available.
	const QString candEngine=conf->value("speech/engine", availableEngines.constFirst()).toString();
	if (availableEngines.contains(candEngine))
		setEngine(candEngine);
	else if (!availableEngines.isEmpty())
		setEngine(availableEngines.constFirst());
	else
	{
		qCWarning(Speech) << "Cannot Initialize Text to Speech";
		Q_ASSERT(0); // is there a chance to ever come here?
	}
#endif
}

bool StelSpeechMgr::enabled() const
{
#ifdef ENABLE_SPEECH
	return (m_speech && m_speech->state()!=QTextToSpeech::State::Error);
#else
	return false;
#endif
}

#ifdef ENABLE_SPEECH

QTextToSpeech::State StelSpeechMgr::getState() const
{
	if (m_speech)
		return m_speech->state();
	return QTextToSpeech::State::Error;
}
#endif

void StelSpeechMgr::say(const QString &narration) const
{
	qCDebug(Speech) << "StelSpeechMgr::say(): " << narration;
#ifdef ENABLE_SPEECH
	if (enabled() && m_speech->voice().name().length()>0)
		m_speech->say(narration);
	else
		qCCritical(Speech) << "Available Engines:" << QTextToSpeech::availableEngines();
#endif
}

void StelSpeechMgr::stop() const
{
	qCDebug(Speech) << "StelSpeechMgr::stop() ";
#ifdef ENABLE_SPEECH
	if (enabled() && m_speech->voice().name().length()>0)
	{
		m_speech->stop(QTextToSpeech::BoundaryHint::Word);

		// Easter egg... Allow occasional argument :-)
		unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::default_random_engine generator(seed);
		std::uniform_int_distribution<int> distribution(0,15);
		int mood = distribution(generator);

		const QStringList answers={
			qc_("Why? You asked for it!",     "Object narration"),
			qc_("OK, I shut up.",             "Object narration"),
			qc_("Come on!",                   "Object narration"),
			qc_("Naaah!",                     "Object narration"),
			qc_("Sir Yes Sir, Silence, Sir!", "Object narration")
		};
		if (mood<answers.length())
		{
			m_speech->say(answers.at(mood)); // Just testing ... ;-)
		}
	}
#endif
}

void StelSpeechMgr::setRate(double rate)
{
	m_rate=rate;
	StelApp::immediateSave("speech/rate", rate);
#ifdef ENABLE_SPEECH
	if (m_speech)
		m_speech->setRate(rate);
#endif
	emit rateChanged(rate);
	qCDebug(Speech) << "set rate" << rate;
}

void StelSpeechMgr::setPitch(double pitch)
{
	m_pitch=pitch;
	StelApp::immediateSave("speech/pitch", pitch);
#ifdef ENABLE_SPEECH
	if (m_speech)
		m_speech->setPitch(pitch);
#endif
	emit pitchChanged(pitch);
	qCDebug(Speech) << "set Pitch" << pitch;
}

void StelSpeechMgr::setVolume(double volume)
{
	m_volume=volume;
	StelApp::immediateSave("speech/volume", volume);
#ifdef ENABLE_SPEECH
	if (m_speech)
		m_speech->setVolume(volume);
#endif
	emit volumeChanged(volume);
	qCDebug(Speech) << "set Volume" << volume;
}

#ifdef ENABLE_SPEECH

void StelSpeechMgr::setEngine(const QString &engineName)
{
    if (m_speech)
	    delete m_speech;
    m_speech = new QTextToSpeech(engineName, this);

    // some engines initialize asynchronously
    if (m_speech->state() == QTextToSpeech::Ready) {
	onEngineReady();
    } else {
	connect(m_speech, &QTextToSpeech::stateChanged, this, &StelSpeechMgr::onEngineReady,
		Qt::SingleShotConnection);
    }
}

QString StelSpeechMgr::getEngine() const
{
	if (m_speech)
		return m_speech->engine();
	else
		return QString();
}

// Re-apply settings and re-configure current/configured language.
void StelSpeechMgr::onEngineReady()
{
	if (m_speech->state() != QTextToSpeech::Ready) {
		qCWarning(Speech) << "Speech engine not ready but" << m_speech->state();
		qCWarning(Speech) << "This should actually never happen and leaves Speech output disabled.";
		Q_ASSERT(0);
		return;
	}
	StelApp::immediateSave("speech/engine", m_speech->engine());

	m_speech->setRate(m_rate);
	m_speech->setPitch(m_pitch);
	m_speech->setVolume(m_volume);

	qCDebug(Speech) << "StelSpeechMgr::onEngineReady(): current engine name" << m_speech->engine();

	// To check available locales, we act as if language was changed.
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &StelSpeechMgr::onAppLanguageChanged, static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::UniqueConnection));
	onAppLanguageChanged();
	emit speechReady();
}

// Allows use on Qt6.4!
QVoice StelSpeechMgr::findVoice(QString &name) const
{
	if (enabled() && !m_voices.isEmpty())
	{
		for (const QVoice &v: m_voices)
			if (v.name() == name)
				return v;
		qCWarning(Speech) << "Voice" << name << "not available. Using" << m_voices.constFirst().name();
		return m_voices.constFirst();
	}
	qCWarning(Speech) << "Voice" << name << "not available. Indeed, no voice available. Returning empty.";
	return QVoice();
}

void StelSpeechMgr::setVoice(QString &name)
{
	// Note: 6.10 is a placeholder. Testing whether 6.4 is enough on e.g. Ubuntu 24.04.
#if (QT_VERSION>=QT_VERSION_CHECK(6,10,0))
	if (name.isEmpty())
		qCWarning(Speech) << "Voice name empty. Falling back to" << m_voices.first().name();

	QList<QVoice>voices=m_speech->findVoices(name);
	if (!voices.isEmpty())
		m_speech->setVoice(voices.first());
	else
	{
		// Should not happen with our GUI setup. Just fall back to some voice.
		qCWarning(Speech) << "Voice" << name << "not found. Falling back to" << m_voices.first().name();
		m_speech->setVoice(m_voices.first());
		Q_ASSERT(0);
	}
#else
	m_speech->setVoice(findVoice(name));
#endif
}

QStringList StelSpeechMgr::getAvailableVoiceNames() const
{
	QStringList voiceNames;
	for (const QVoice &v: m_voices)
		voiceNames.append(v.name());
	return voiceNames;
}


// Find and configure voices for current app language
// This should be called when either app language or speech engine changes.
void StelSpeechMgr::onAppLanguageChanged()
{
	qCDebug(Speech) << "StelSpeechMgr: configuring Speech language";
	const QString appLanguage=StelApp::getInstance().getLocaleMgr().getAppLanguage();
	qCDebug(Speech) << "StelSpeechMgr: App Language is" << appLanguage;
	QList<QLocale>locales=m_speech->availableLocales();
	qCDebug(Speech) << "StelSpeechMgr: Available Locales:" << locales;
	qCDebug(Speech) << "StelSpeechMgr: current locale name" << m_speech->locale().name();
	qCDebug(Speech) << "Now checking and potentially re-setting locale";
	if (locales.contains(QLocale(appLanguage)))
		m_speech->setLocale(QLocale(appLanguage));
	else
		m_speech->setLocale(QLocale::English);

	qCDebug(Speech) << "StelSpeechMgr: current locale name" << m_speech->locale().name();

	// TODO: AT THIS POINT AVAILABLE VOICES MAY HAVE CHANGED. Emit a signal for GUI changes?

	m_voices=m_speech->availableVoices();

	qCDebug(Speech) << "StelSpeechMgr: available voices for app language" << appLanguage;
	QStringList voiceNames;
	for (const QVoice &v: std::as_const(m_voices))
	{
		voiceNames.append(v.name());
		qCDebug(Speech) << "    " << v.name() << "age:" << v.age() << v.ageName(v.age())  << "gender:" << v.gender() << v.genderName(v.gender()) << "locale:" << v.locale();
	}

	QSettings *conf=StelApp::getInstance().getSettings();
	QString storedVoice=conf->value("speech/voice").toString();

	if (voiceNames.contains(storedVoice))
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,10,0))
		QList<QVoice> voices=m_speech->findVoices(storedVoice);
		m_speech->setVoice(voices.first());
#else
		m_speech->setVoice(findVoice(storedVoice));
#endif
	}
	else
		m_speech->setVoice(m_voices.constFirst());


	qCDebug(Speech) << "StelSpeechMgr: current voice name"  << m_speech->voice().name();
	qCDebug(Speech) << "StelSpeechMgr: with params pitch:" << m_speech->pitch()
			<< "rate:" << m_speech->rate() << "volume:" << m_speech->volume();

	emit languageChanged();
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


void StelSpeechMgr::initNarrationFlagsFromConfig(QSettings *conf)
{
	StelObject::InfoStringGroup flags = StelObject::InfoStringGroup(StelObject::None);

	conf->beginGroup("custom_selected_info");
	if (conf->value("flag_narration_name", false).toBool())
		flags |= StelObject::Name;
	if (conf->value("flag_narration_catalognumber", false).toBool())
		flags |= StelObject::CatalogNumber;
	if (conf->value("flag_narration_magnitude", false).toBool())
		flags |= StelObject::Magnitude;
	if (conf->value("flag_narration_absolutemagnitude", false).toBool())
		flags |= StelObject::AbsoluteMagnitude;
	if (conf->value("flag_narration_radecj2000", false).toBool())
		flags |= StelObject::RaDecJ2000;
	if (conf->value("flag_narration_radecofdate", false).toBool())
		flags |= StelObject::RaDecOfDate;
	if (conf->value("flag_narration_hourangle", false).toBool())
		flags |= StelObject::HourAngle;
	if (conf->value("flag_narration_altaz", false).toBool())
		flags |= StelObject::AltAzi;
	if (conf->value("flag_narration_elongation", false).toBool())
		flags |= StelObject::Elongation;
	if (conf->value("flag_narration_distance", false).toBool())
		flags |= StelObject::Distance;
	if (conf->value("flag_narration_velocity", false).toBool())
		flags |= StelObject::Velocity;
	if (conf->value("flag_narration_propermotion", false).toBool())
		flags |= StelObject::ProperMotion;
	if (conf->value("flag_narration_size", false).toBool())
		flags |= StelObject::Size;
	if (conf->value("flag_narration_extra", false).toBool())
		flags |= StelObject::Extra;
	if (conf->value("flag_narration_type", false).toBool())
		flags |= StelObject::ObjectType;
	if (conf->value("flag_narration_galcoord", false).toBool())
		flags |= StelObject::GalacticCoord;
	if (conf->value("flag_narration_supergalcoord", false).toBool())
		flags |= StelObject::SupergalacticCoord;
	if (conf->value("flag_narration_othercoord", false).toBool())
		flags |= StelObject::OtherCoord;
	if (conf->value("flag_narration_eclcoordofdate", false).toBool())
		flags |= StelObject::EclipticCoordOfDate;
	if (conf->value("flag_narration_eclcoordj2000", false).toBool())
		flags |= StelObject::EclipticCoordJ2000;
	if (conf->value("flag_narration_constellation", false).toBool())
		flags |= StelObject::IAUConstellation;
	if (conf->value("flag_narration_cultural_constellation", false).toBool())
		flags |= StelObject::CulturalConstellation;
	if (conf->value("flag_narration_sidereal_time", false).toBool())
		flags |= StelObject::SiderealTime;
	if (conf->value("flag_narration_rts_time", false).toBool())
		flags |= StelObject::RTSTime;
	if (conf->value("flag_narration_solar_lunar", false).toBool())
	    flags |= StelObject::SolarLunarPosition;
	conf->endGroup();

	setNarrationTextFilters(flags);
}
