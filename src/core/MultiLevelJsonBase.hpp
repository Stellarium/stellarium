/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _MULTILEVELJSONBASE_HPP_
#define _MULTILEVELJSONBASE_HPP_

#include <QList>
#include <QString>
#include <QVariantMap>
#include <QNetworkReply>

#include "StelSkyLayer.hpp"

class QIODevice;
class StelCore;

//! Abstract base class for managing multi-level tree objects stored in JSON format.
//! The JSON files can be stored on disk or remotely and are loaded into threads.
class MultiLevelJsonBase : public StelSkyLayer
{
	Q_OBJECT

	friend class JsonLoadThread;

public:
	//! Default constructor.
	MultiLevelJsonBase(MultiLevelJsonBase* parent=NULL);

	//! Init the element from a URL.
	//! This method should be called by the constructors of the subclass.
	void initFromUrl(const QString& url);

	//! Init the element from a QVariantMap.
	//! This method should be called by the constructors of the subclass.
	void initFromQVariantMap(const QVariantMap& map);

	//! Destructor.
	~MultiLevelJsonBase();

	//! Return the short name for this image to be used in the loading bar.
	QString getShortName() const {return shortName;}

	//! Return true if an error occured while loading the data.
	bool hasErrorOccured() const {return errorOccured;}

	//! Get the depth level in the tree.
	int getLevel() const {return parent()==NULL ? 0 : (qobject_cast<MultiLevelJsonBase*>(parent()))->getLevel()+1;}

	//! Convert the image informations to a map following the JSON structure.
	//! It can be saved as JSON using the StelJsonParser methods.
	QVariantMap toQVariantMap() const;

	//! Schedule a deletion for all the childs.
	//! It will practically occur after the delay passed as argument to deleteUnusedTiles() has expired.
	void scheduleChildsDeletion();

private slots:
	//! Called when the download for the JSON file terminated.
	void downloadFinished();
	//! Called when the JSON file is loaded.
	void jsonLoadFinished();

protected:
	//! Load the element from a valid QVariantMap.
	//! This method is called after the JSON files are downloaded and parsed into a QVariantMap.
	virtual void loadFromQVariantMap(const QVariantMap& map)=0;

	//! Return true if a deletion is currently scheduled.
	bool isDeletionScheduled() const {return timeWhenDeletionScheduled>0.;}

	//! The very short name for this image set to be used in loading bar.
	QString shortName;

	//! Base URL to prefix to relative URL
	QString baseUrl;

	//! The relative URL passed to the constructor
	QString contructorUrl;

	//! The list of all the created subtiles for this tile
	QList<MultiLevelJsonBase*> subTiles;

	//! Set to true if an error occured with this tile and it should not be displayed
	bool errorOccured;

	void updatePercent(int tot, int numToBeLoaded);

	//! Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
	void deleteUnusedSubTiles();

	//! true if the JSON descriptor file is currently downloading
	bool downloading;

	//! If a deletion was scheduled, cancel it.
	void cancelDeletion();

	//! Load the element information from a JSON file
	static QVariantMap loadFromJSON(QIODevice& input, bool qZcompressed=false, bool gzCompressed=false);

private:
	//! Return the base URL prefixed to relative URL
	QString getBaseUrl() const {return baseUrl;}

	// Used to download remote JSON files if needed
	class QNetworkReply* httpReply;

	// The delay after which a scheduled deletion will occur
	float deletionDelay;

	class JsonLoadThread* loadThread;

	// Time at which deletion was first scheduled
	double timeWhenDeletionScheduled;

	// The temporary map filled in a thread
	QVariantMap temporaryResultMap;

	bool loadingState;
	int lastPercent;

	//! The network manager to use for downloading JSON files
	static class QNetworkAccessManager* networkAccessManager;

	static QNetworkAccessManager& getNetworkAccessManager();
};

#endif // _MULTILEVELJSONBASE_HPP_
