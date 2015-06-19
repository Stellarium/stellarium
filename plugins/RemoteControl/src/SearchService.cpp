/*
 * Stellarium Remote Control plugin
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

#include "SearchService.hpp"

#include "SearchDialog.hpp"
#include "StelApp.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

SearchService::SearchService(const QByteArray &serviceName, QObject *parent) : AbstractAPIService(serviceName,parent)
{
	//this is run in the main thread
	objMgr = &StelApp::getInstance().getStelObjectMgr();
	useStartOfWords = StelApp::getInstance().getSettings()->value("search/flag_start_words", false).toBool();

	//make sure this object "lives" in the same thread as objMgr
	Q_ASSERT(this->thread() == objMgr->thread());
}

QStringList SearchService::performSearch(const QString &text)
{
	//perform substitution greek text --> symbol
	//use the searchdialog static method for that
	QString greekText = SearchDialog::substituteGreek(text);

	QStringList matches;
	if(greekText != text) {
		matches = objMgr->listMatchingObjectsI18n(text, 3, useStartOfWords);
		matches += objMgr->listMatchingObjects(text, 3, useStartOfWords);
		matches += objMgr->listMatchingObjectsI18n(greekText, (8 - matches.size()), useStartOfWords);
	} else {
		//no greek replaced, saves 1 call
		matches = objMgr->listMatchingObjectsI18n(text, 5, useStartOfWords);
		matches += objMgr->listMatchingObjects(text, 5, useStartOfWords);
	}

	return matches;
}

void SearchService::get(const QByteArray& operation, const QMultiMap<QByteArray, QByteArray> &parameters, HttpResponse &response)
{
	//this is run in an HTTP worker thread

	//make sure the object still "lives" in the main Stel thread, even though we are currently in the
	//HTTP thread
	Q_ASSERT(this->thread() == objMgr->thread());

	if(operation=="find")
	{
		//this may contain greek or other unicode letters
		QString str = QString::fromUtf8(parameters.value("str"));
		str = str.trimmed().toLower();

		if(str.isEmpty())
		{
			writeRequestError("empty search string",response);
			return;
		}

		qDebug()<<"Search string"<<str;

		QStringList results;
		QMetaObject::invokeMethod(this,"performSearch",Qt::BlockingQueuedConnection,
					  Q_RETURN_ARG(QStringList,results),
					  Q_ARG(QString,str));

		//remove duplicates here
		results.removeDuplicates();

		results.sort(Qt::CaseInsensitive);
		// objects with short names should be searched first
		// examples: Moon, Hydra (moon); Jupiter, Ghost of Jupiter
		stringLengthCompare comparator;
		std::sort(results.begin(), results.end(), comparator);

		//return as json
		writeJSON(QJsonDocument(QJsonArray::fromStringList(results)),response);
	}
	else
	{
		//TODO some sort of service description?
		writeRequestError("unsupported operation. GET: search",response);
	}
}

