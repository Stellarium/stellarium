/*
 * Copyright (C) 2012 Sibi Antony (Phonon/QT4)
 * Copyright (C) 2015 Georg Zotti (Reactivate for QT5)
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

#include "StelVideoMgr.hpp"
#include "StelMainView.hpp"
#include <QDebug>
#include <QDir>
#ifdef ENABLE_MEDIA
	//#include <QVideoWidget> // Adapt to that when we finally switch to QtQuick2!
	#include <QGraphicsVideoItem>
	#include <QMediaPlayer>
#endif


StelVideoMgr::StelVideoMgr()
{
}

#ifdef ENABLE_MEDIA
StelVideoMgr::~StelVideoMgr()
{
	foreach(QString id, videoObjects.keys())
	{
		dropVideo(id);
	}
}

void StelVideoMgr::loadVideo(const QString& filename, const QString& id, const float x, const float y, const bool show, const float alpha)
{
	if (videoObjects.contains(id))
	{
		qWarning() << "[StelVideoMgr] Video object with ID" << id << "already exists, dropping it";
		dropVideo(id);
	}

	videoObjects[id] = new VideoPlayer;
	videoObjects[id]->player = new QMediaPlayer();
	videoObjects[id]->videoItem= new QGraphicsVideoItem();

	connect(videoObjects[id]->player, SIGNAL(audioAvailableChanged(bool)), this, SLOT(handleAudioAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(bufferStatusChanged(int)), this, SLOT(handleBufferStatusChanged(int)));
	connect(videoObjects[id]->player, SIGNAL(currentMediaChanged(QMediaContent)), this, SLOT(handleCurrentMediaChanged(QMediaContent)));
	connect(videoObjects[id]->player, SIGNAL(durationChanged(qint64)), this, SLOT(handleDurationChanged(qint64)));
	connect(videoObjects[id]->player, SIGNAL(error(QMediaPlayer::Error)), this, SLOT(handleError(QMediaPlayer::Error)));
	connect(videoObjects[id]->player, SIGNAL(mediaChanged(QMediaContent)), this, SLOT(handleMediaChanged(QMediaContent)));
	connect(videoObjects[id]->player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)), this, SLOT(handleMediaStatusChanged(QMediaPlayer::MediaStatus))); // debug-log messages
	connect(videoObjects[id]->player, SIGNAL(mutedChanged(bool)), this, SLOT(handleMutedChanged(bool)));
	//connect(videoObjects[id]->player, SIGNAL(networkConfigurationChanged(QNetworkConfiguration)), this, SLOT(handleNetworkConfigurationChanged(QNetworkConfiguration)));
	connect(videoObjects[id]->player, SIGNAL(playbackRateChanged(qreal)), this, SLOT(handlePlaybackRateChanged(qreal)));
	connect(videoObjects[id]->player, SIGNAL(positionChanged(qint64)), this, SLOT(handlePositionChanged(qint64)));
	connect(videoObjects[id]->player, SIGNAL(seekableChanged(bool)), this, SLOT(handleSeekableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handleStateChanged(QMediaPlayer::State)));
	connect(videoObjects[id]->player, SIGNAL(videoAvailableChanged(bool)), this, SLOT(handleVideoAvailableChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(volumeChanged(int)), this, SLOT(handleVolumeChanged(int)));

	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(bool)), this, SLOT(handleAvailabilityChanged(bool)));
	connect(videoObjects[id]->player, SIGNAL(availabilityChanged(QMultimedia::AvailabilityStatus)), this, SLOT(handleAvailabilityChanged(QMultimedia::AvailabilityStatus)));
	connect(videoObjects[id]->player, SIGNAL(metaDataAvailableChanged(bool)), this, SLOT(handleMetaDataAvailableChanged(bool)));
	// I don't connect this for now: I see this is triggered, but no details delivered by the next signal?
	//connect(videoObjects[id]->player, SIGNAL(metaDataChanged()), this, SLOT(handleMetaDataChanged()));
	connect(videoObjects[id]->player, SIGNAL(metaDataChanged(QString,QVariant)), this, SLOT(handleMetaDataChanged(QString,QVariant)));
	connect(videoObjects[id]->player, SIGNAL(notifyIntervalChanged(int)), this, SLOT(handleNotifyIntervalChanged(int)));

	// we give the player the id as property to allow better tracking of log messages or to access their videoObjects.
	videoObjects[id]->player->setProperty("Stel_id", id);

	// It seems we need an absolute pathname here.
	qDebug() << "Trying to load" << filename;
	QMediaContent content(QUrl::fromLocalFile(QFileInfo(filename).absoluteFilePath()));
	qDebug() << "Loaded " << content.canonicalUrl();
	// On Windows, this reveals we have nothing loaded!
	qDebug() << "MediaContent canonical resources:";
	qDebug() << "\tData size:     " << content.canonicalResource().dataSize();
	qDebug() << "\tMIME Type:     " << content.canonicalResource().mimeType();
	qDebug() << "\tVideo codec:   " << content.canonicalResource().videoCodec();
	qDebug() << "\tResolution:    " << content.canonicalResource().resolution();
	qDebug() << "\tVideo bitrate: " << content.canonicalResource().videoBitRate();
	qDebug() << "\tAudio codec:   " << content.canonicalResource().audioCodec();
	qDebug() << "\tChannel Count: " << content.canonicalResource().channelCount();
	qDebug() << "\tAudio bitrate: " << content.canonicalResource().audioBitRate();
	qDebug() << "\tSample rate:   " << content.canonicalResource().sampleRate();
	qDebug() << "\tlanguage:      " << content.canonicalResource().language();
	videoObjects[id]->player->setMedia(content);
	videoObjects[id]->player->setVideoOutput(videoObjects[id]->videoItem);
	// Maybe ask only player?
	qDebug() << "Media Resources queried from player:";
	qDebug() << "\tSTATUS:        " << videoObjects[id]->player->mediaStatus();
	qDebug() << "\tFile:          " << videoObjects[id]->player->currentMedia().canonicalUrl();
	qDebug() << "\tData size:     " << videoObjects[id]->player->currentMedia().canonicalResource().dataSize();
	qDebug() << "\tMIME Type:     " << videoObjects[id]->player->currentMedia().canonicalResource().mimeType();
	qDebug() << "\tVideo codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().videoCodec();
	qDebug() << "\tResolution:    " << videoObjects[id]->player->currentMedia().canonicalResource().resolution();
	qDebug() << "\tVideo bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().videoBitRate();
	qDebug() << "\tAudio codec:   " << videoObjects[id]->player->currentMedia().canonicalResource().audioCodec();
	qDebug() << "\tChannel Count: " << videoObjects[id]->player->currentMedia().canonicalResource().channelCount();
	qDebug() << "\tAudio bitrate: " << videoObjects[id]->player->currentMedia().canonicalResource().audioBitRate();
	qDebug() << "\tSample rate:   " << videoObjects[id]->player->currentMedia().canonicalResource().sampleRate();
	qDebug() << "\tlanguage:      " << videoObjects[id]->player->currentMedia().canonicalResource().language();

	StelMainView::getInstance().scene()->addItem(videoObjects[id]->videoItem);
	// TODO: Apparently we must wait until status=loaded. (?)

	videoObjects[id]->videoItem->setPos(x, y);
	//videoObjects[id]->videoItem->setSize(videoObjects[id]->player->media().resources()[0].resolution());
	videoObjects[id]->videoItem->setSize(content.resources().at(0).resolution());
	// DEBUG show a box. I had vieo playing in a tiny box already, once..!
	videoObjects[id]->videoItem->setSize(QSizeF(350, 350));

	videoObjects[id]->videoItem->setOpacity(alpha);
	videoObjects[id]->videoItem->setVisible(show);
	qDebug() << "Loaded video" << id << "for pos " << x << "/" << y << "Size" << videoObjects[id]->videoItem->size();

}

void StelVideoMgr::playVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			// if already playing, stop and play from the start
			if (videoObjects[id]->player->state() == QMediaPlayer::PlayingState)
			{
				videoObjects[id]->player->stop();
			}

			// otherwise just play it, or resume playing paused video.
			qDebug() << "StelVideoMgr::playVideo(): playing " << id;
			videoObjects[id]->player->play();
		}
	}
	else qDebug() << "StelVideoMgr::playVideo()" << id << ": no such video";
}

void StelVideoMgr::pauseVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->pause();
		}
	}
	else qDebug() << "StelVideoMgr::pauseVideo()" << id << ": no such video";
}

void StelVideoMgr::stopVideo(const QString& id)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->stop();
		}
	}
	else qDebug() << "StelVideoMgr::stopVideo()" << id << ": no such video";
}

void StelVideoMgr::seekVideo(const QString& id, const qint64 ms)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL) 
		{
			if (videoObjects[id]->player->isSeekable())
			{
				videoObjects[id]->player->setPosition(ms);
				// Seek capability depends on the backend used.
			}
			else
			{
				qDebug() << "[StelVideoMgr] Cannot seek media source.";
			}
		}
	}
	else qDebug() << "StelVideoMgr::seekVideo()" << id << ": no such video";
}

void StelVideoMgr::dropVideo(const QString& id)
{
	if (!videoObjects.contains(id))
		return;
	if (videoObjects[id]->player!=NULL)
	{
		videoObjects[id]->player->stop();
		StelMainView::getInstance().scene()->removeItem(videoObjects[id]->videoItem);
		delete videoObjects[id]->player;
		delete videoObjects[id]->videoItem;
		delete videoObjects[id];
		videoObjects.remove(id); 
	}
}

void StelVideoMgr::setVideoXY(const QString& id, const float x, const float y)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setPos(x, y);
		}
	}
	else qDebug() << "StelVideoMgr::setVideoXY()" << id << ": no such video";
}

void StelVideoMgr::setVideoAlpha(const QString& id, const float alpha)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setOpacity(alpha);
		}
	}
	else qDebug() << "StelVideoMgr::setVideoAlpha()" << id << ": no such video";
}

void StelVideoMgr::resizeVideo(const QString& id, float w, float h)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			QSize videoSize=videoObjects[id]->player->currentMedia().resources()[1].resolution();
			float aspectRatio=videoSize.width()/videoSize.height();
			// TODO: Repair next lines, linker does not find function.
			if (w!=-1.0f && h!=-1.0f)
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::IgnoreAspectRatio);
			else
			{
				videoObjects[id]->videoItem->setAspectRatioMode(Qt::KeepAspectRatio);
				if (w==-1.0f && h==-1.0f)
				{
					h=videoSize.height();
					w=videoSize.width();
				}
				else if (h==-1.0f)
					h=w/aspectRatio;
				else if (w==-1.0f)
					w=h*aspectRatio;
			}
			videoObjects[id]->videoItem->setSize(QSizeF(w, h));
		}
	}
	else qDebug() << "StelVideoMgr::resizeVideo()" << id << ": no such video";
}

void StelVideoMgr::showVideo(const QString& id, const bool show)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->videoItem!=NULL)
		{
			videoObjects[id]->videoItem->setVisible(show);
		}
	}
	else qDebug() << "StelVideoMgr::showVideo()" << id << ": no such video";
}


void StelVideoMgr::mute(const QString& id, bool mute)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->setMuted(mute);
		}
	}
	else qDebug() << "StelVideoMgr::mute()" << id << ": no such video";
}

void StelVideoMgr::setVolume(const QString& id, int newVolume)
{
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			videoObjects[id]->player->setVolume(newVolume);
		}
	}
	else qDebug() << "StelVideoMgr::setVolume()" << id << ": no such video";
}

int StelVideoMgr::getVolume(const QString& id)
{
	int volume=-1;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			volume=videoObjects[id]->player->volume();
		}
	}
	else qDebug() << "StelVideoMgr::getVolume()" << id << ": no such video";
	return volume;
}

bool StelVideoMgr::isPlaying(const QString& id)
{
	int playing=false;
	if (videoObjects.contains(id))
	{
		if (videoObjects[id]->player!=NULL)
		{
			playing= (videoObjects[id]->player->state() == QMediaPlayer::PlayingState );
		}
	}
	else qDebug() << "StelVideoMgr::isPlaying()" << id << ": no such video";
	return playing;
}


/*
 * Signal handlers for all signals of QMediaPlayer. Usually for now this only writes a message to logfile.
 */
void StelVideoMgr::handleAudioAvailableChanged(bool available)
{
	qDebug() << "QMediaplayer: " << this->sender()->property("Stel_id").toString() << ": Audio is now available:" << available;
}
void StelVideoMgr::handleBufferStatusChanged(int percentFilled)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Buffer filled (%):" << percentFilled;
}

void StelVideoMgr::handleCurrentMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Current media changed:" << media.canonicalUrl();
}
void StelVideoMgr::handleDurationChanged(qint64 duration)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": Duration changed to:" << duration;
}
void StelVideoMgr::handleError(QMediaPlayer::Error error)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  error:" << error;
}
void StelVideoMgr::handleMediaChanged(const QMediaContent & media)
{
	qDebug() << "QMediaPlayer: " << QObject::sender()->property("Stel_id").toString() << ": Media changed:" << media.canonicalUrl();
}
void StelVideoMgr::handleMediaStatusChanged(QMediaPlayer::MediaStatus status) // debug-log messages
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  status changed:" << status;
}
void StelVideoMgr::handleMutedChanged(bool muted)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  mute changed:" << muted;
}

//void StelVideoMgr::handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration)
//{
//	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  network configuration changed:" << configuration;
//}
void StelVideoMgr::handlePlaybackRateChanged(qreal rate)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ": playback rate changed to:" << rate;
}

void StelVideoMgr::handlePositionChanged(qint64 position)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  position changed to (ms):" << position;
}
void StelVideoMgr::handleSeekableChanged(bool seekable)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  seekable changed to:" << seekable;
}
void StelVideoMgr::handleStateChanged(QMediaPlayer::State state)
{
	QString senderId=QObject::sender()->property("Stel_id").toString();
	qDebug() << "QMediaplayer: " << senderId << ":  state changed to:" << state;
	qDebug() << "Media Resources from player:";
	qDebug() << "\tMedia Status:  " << videoObjects[senderId]->player->mediaStatus();
	qDebug() << "\tFile:          " << videoObjects[senderId]->player->currentMedia().canonicalUrl();
	qDebug() << "\tData size:     " << videoObjects[senderId]->player->currentMedia().canonicalResource().dataSize();
	qDebug() << "\tMIME Type:     " << videoObjects[senderId]->player->currentMedia().canonicalResource().mimeType();
	qDebug() << "\tVideo codec:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().videoCodec();
	qDebug() << "\tResolution:    " << videoObjects[senderId]->player->currentMedia().canonicalResource().resolution();
	qDebug() << "\tVideo bitrate: " << videoObjects[senderId]->player->currentMedia().canonicalResource().videoBitRate();
	qDebug() << "\tAudio codec:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().audioCodec();
	qDebug() << "\tChannel Count: " << videoObjects[senderId]->player->currentMedia().canonicalResource().channelCount();
	qDebug() << "\tAudio bitrate: " << videoObjects[senderId]->player->currentMedia().canonicalResource().audioBitRate();
	qDebug() << "\tSample rate:   " << videoObjects[senderId]->player->currentMedia().canonicalResource().sampleRate();
	qDebug() << "\tlanguage:      " << videoObjects[senderId]->player->currentMedia().canonicalResource().language();

}
void StelVideoMgr::handleVideoAvailableChanged(bool videoAvailable)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Video available:" << videoAvailable;
}
void StelVideoMgr::handleVolumeChanged(int volume)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  volume changed to:" << volume;
}

void StelVideoMgr::handleAvailabilityChanged(bool available)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  available:" << available;
}

void StelVideoMgr::handleAvailabilityChanged(QMultimedia::AvailabilityStatus availability){
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  availability:" << availability;
}

void StelVideoMgr::handleMetaDataAvailableChanged(bool available){
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Metadata available:" << available;
}

void StelVideoMgr::handleMetaDataChanged(){
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Metadata changed.";
}

void StelVideoMgr::handleMetaDataChanged(const QString & key, const QVariant & value){
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Metadata change:" << key << "=>" << value;
}

void StelVideoMgr::handleNotifyIntervalChanged(int milliseconds)
{
	qDebug() << "QMediaplayer: " << QObject::sender()->property("Stel_id").toString() << ":  Notify interval changed to:" << milliseconds;
}


#else 
void StelVideoMgr::loadVideo(const QString& filename, const QString& id, float x, float y, bool show, float alpha)
{
	qWarning() << "[StelVideoMgr] This build of Stellarium does not support video - cannot load video" << QDir::toNativeSeparators(filename) << id << x << y << show << alpha;
}
StelVideoMgr::~StelVideoMgr() {;}
void StelVideoMgr::playVideo(const QString&) {;}
void StelVideoMgr::pauseVideo(const QString&) {;}
void StelVideoMgr::stopVideo(const QString&) {;}
void StelVideoMgr::dropVideo(const QString&) {;}
void StelVideoMgr::seekVideo(const QString&, qint64) {;}
void StelVideoMgr::setVideoXY(const QString&, float, float) {;}
void StelVideoMgr::setVideoAlpha(const QString&, float) {;}
void StelVideoMgr::resizeVideo(const QString&, float, float) {;}
void StelVideoMgr::showVideo(const QString&, bool) {;}
// New functions for 0.14
int StelVideoMgr::getDuration(const QString&){return -1;}
QSize StelVideoMgr::getResolution(const QString&){return QSize(0,0);}
int StelVideoMgr::getWidth(const QString&){return -1;}
int StelVideoMgr::getHeight(const QString&){return -1;}
void StelVideoMgr::mute(const QString&, bool){;}
void StelVideoMgr::setVolume(const QString&, int){;}
int StelVideoMgr::getVolume(const QString&){return -1;}

// New functions for QT5
void StelVideoMgr::handleAudioAvailableChanged(bool available){;}
void StelVideoMgr::handleBufferStatusChanged(int percentFilled){;}
void StelVideoMgr::handleCurrentMediaChanged(const QMediaContent & media){;}
void StelVideoMgr::handleDurationChanged(qint64 duration){;}
void StelVideoMgr::handleError(QMediaPlayer::Error error){;}
void StelVideoMgr::handleMediaChanged(const QMediaContent & media){;}
void StelVideoMgr::handleMediaStatusChanged(QMediaPlayer::MediaStatus status){;}
void StelVideoMgr::handleMutedChanged(bool muted){;}
void StelVideoMgr::handleNetworkConfigurationChanged(const QNetworkConfiguration & configuration){;}
void StelVideoMgr::handlePlaybackRateChanged(qreal){;}
void StelVideoMgr::handlePositionChanged(qint64){;}
void StelVideoMgr::handleSeekableChanged(bool){;}
void StelVideoMgr::handleStateChanged(QMediaPlayer::State){;}
void StelVideoMgr::handleVideoAvailableChanged(bool){;}
void StelVideoMgr::handleVolumeChanged(int){;}
void StelVideoMgr::handleAvailabilityChanged(bool){;}
void StelVideoMgr::handleAvailabilityChanged(QMultimedia::AvailabilityStatus){;}
void StelVideoMgr::handleMetaDataAvailableChanged(bool){;}
void StelVideoMgr::handleMetaDataChanged(){;}
void StelVideoMgr::handleMetaDataChanged(const QString&, const QVariant&){;}
void StelVideoMgr::handleNotifyIntervalChanged(int){;}

#endif // ENABLE_MEDIA


