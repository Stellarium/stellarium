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

#ifndef STELSPEECHMGR_HPP
#define STELSPEECHMGR_HPP

#include "StelModule.hpp"
#include "StelObject.hpp"
#include <QObject>
#include <QSettings>
#include <QMap>
#include <QString>
#if (QT_VERSION>=QT_VERSION_CHECK(6,6,0)) && defined(ENABLE_MEDIA)
#include <QAudioOutput>
#include <QTextToSpeech>
#endif

class QMediaPlayer;
class QVoice;

//! @class StelSpeechMgr
//! Provides artificial voice output for a narration on sky objects.
//!
//! Sometimes, in the dark you may want voice output over reading small text on the screen.
//! Calling a "narrate" action (default: Shift+R) for the selected object will provide some information.
//!
//! Parallel to the InfoString settings which decide what information items to show,
//! we use a similar structure to decide what data elements are being narrated.
//!
//! @note: This functionality requires Qt6.6 and higher or else just emits the narration to logfile.


class StelSpeechMgr : public StelModule
{
	Q_OBJECT
#if (QT_VERSION>=QT_VERSION_CHECK(6,6,0)) && defined(ENABLE_MEDIA)

	Q_PROPERTY(double rate                READ getRate     WRITE setRate     NOTIFY rateChanged)
	Q_PROPERTY(double pitch               READ getPitch    WRITE setPitch    NOTIFY pitchChanged)
	Q_PROPERTY(double volume              READ getVolume   WRITE setVolume   NOTIFY volumeChanged)
	Q_PROPERTY(QTextToSpeech::State state READ getState                      NOTIFY stateChanged)   // ???
	//Q_PROPERTY(QString language           READ getLanguage WRITE setLanguage NOTIFY languageChanged) // Given the state of language support, it may be easier to have separate voice languages.
	//Q_PROPERTY(QLocale locale             READ getLocale   WRITE setLocale   NOTIFY localeChanged)
	//Q_PROPERTY(QString engine             READ getEngine   WRITE setEngine   NOTIFY engineChanged)
	//Q_PROPERTY(QString voice              READ getVoice    WRITE setVoice    NOTIFY voiceChanged)
#endif


public:
	StelSpeechMgr();
	~StelSpeechMgr() override;
	void init() override;

	//! Retrieve the currently active flags which information bits to narrate
	const StelObject::InfoStringGroup& getNarrationTextFilters() const;

	//! Set the currently active flags which information bits to narrate
	void setNarrationTextFilters(const StelObject::InfoStringGroup &flags);

signals:
	void rateChanged(double);
	void pitchChanged(double);
	void volumeChanged(double);
#if (QT_VERSION>=QT_VERSION_CHECK(6,6,0)) && defined(ENABLE_MEDIA)
	void stateChanged(QTextToSpeech::State state);
#endif

public slots:
	//! Emit an audible synthesized narration
	void say(const QString &narration) const;
	//! Stops replay of narration.
	void stop();

	//! global info, always false if Qt<6.6 and if no engine was initialized
	bool enabled() const;

	//! set replay rate (-1...+1)
	void setRate(double newRate);
	double getRate(){return m_rate;}

	//! set pitch (-1...+1)
	void setPitch(double newPitch);
	double getPitch(){return m_pitch;}

	//! set volume (0...+1)
	void setVolume(double newVolume);
	double getVolume(){return m_volume;}

#if (QT_VERSION>=QT_VERSION_CHECK(6,6,0)) && defined(ENABLE_MEDIA)
	//! return the state of the Speech engine, or QTextToSpeech::State::Error
	QTextToSpeech::State getState();

	//! set the engine to be used. Some OS like Windows have more than one.
	//! @arg engine must be one of those returned by QTextToSpeech::availableEngines.
	//! @return True on success, setting one working engine (the one reported by the engineChanged() signal).
	//!         False means something is broken and no engine is available.
	//! emits engineChanged().
	bool setEngine(QString &engine){return m_speech->setEngine(engine);}

#endif

	//void stateChanged(QTextToSpeech::State state);
	//void engineSelected(int index);
	//void languageSelected(int language);
	//void voiceSelected(int index);

	//void localeChanged(const QLocale &locale);


private:
	//void onEngineReady();

#if (QT_VERSION>=QT_VERSION_CHECK(6,6,0)) && defined(ENABLE_MEDIA)

	QTextToSpeech *m_speech = nullptr;
	QList<QVoice> m_voices;
	QList<QLocale> m_locales;
#endif

	double m_rate;   // -1...1
	double m_pitch;  // -1...1
	double m_volume; // 0...1
	StelObject::InfoStringGroup flags; //! Selection of info bits to be narrated
};

#endif // STELSPEECHMGR_HPP
