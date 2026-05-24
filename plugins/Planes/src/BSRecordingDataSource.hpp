/*
 * Copyright (C) 2013 Felix Zeltner
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

#ifndef BSRECORDINGDATASOURCE_HPP
#define BSRECORDINGDATASOURCE_HPP

#include "FlightDataSource.hpp"
#include "StelProgressController.hpp"

Q_DECLARE_METATYPE(QList<FlightP>)


class BSParser : public QObject
{
	Q_OBJECT
public:
	BSParser() {}
	~BSParser() {}

public slots:
	void loadFile(QString filename);

signals:
	void done(QList<FlightP> flights);
	void progressChanged(qint64 done, qint64 total);

private:
	QList<FlightP> *results;
};


class BSRecordingDataSource : public FlightDataSource
{
	Q_OBJECT
public:
	BSRecordingDataSource();
	~BSRecordingDataSource();

	QList<FlightP> *getRelevantFlights();
	void updateRelevantFlights(double jd, double rate);
	void init();
	void deinit();

	void loadFile(QString filename);

public slots:
	void processResult(QList<FlightP> result);
	void setProgress(qint64 done, qint64 total);

signals:
	void parseFileRequested(QString filename);

private:
	bool workerRunning;
	QList<FlightP> flights; //<! Holds all flights parsed from the file
	QList<FlightP> relevantFlights; //<! Holds the relevant flights
	QThread *workerThread;
	BSParser *worker;
	StelProgressController *progressBar;
};

#endif // BSRECORDINGDATASOURCE_HPP
