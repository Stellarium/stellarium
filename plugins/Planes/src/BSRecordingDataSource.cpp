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


#include "BSRecordingDataSource.hpp"
#include "ADS-B.hpp"
#include "StelApp.hpp"

BSRecordingDataSource::BSRecordingDataSource() : FlightDataSource(1), workerRunning(false), workerThread(NULL), worker(NULL), progressBar(NULL)
{
    qRegisterMetaType<QList<FlightP> >();
}

BSRecordingDataSource::~BSRecordingDataSource()
{

}

QList<FlightP> *BSRecordingDataSource::getRelevantFlights()
{
    return &relevantFlights;
}

void BSRecordingDataSource::updateRelevantFlights(double jd, double rate)
{
    if (flights.size() < 50) {
        // For small ammounts of flights, just add all as relevant and don't
        // manage 2 seperate lists
        return;
    }
    relevantFlights.clear();
    double lookahead = fabs(10 * rate);
    if (rate >= 0) {
        //fwd
        foreach(FlightP f, flights) {
            // add flight if it's gonna start soon or has already started
            if ((f->getTimeStart() - jd > 0 && f->getTimeStart() - jd < lookahead) || f->isTimeInRange(jd)) {
                relevantFlights.append(f);
            }
        }
    } else {
        foreach(FlightP f, flights) {
            if ((jd - f->getTimeEnd() > 0 && jd - f->getTimeEnd() < lookahead) || f->isTimeInRange(jd)) {
                relevantFlights.append(f);
            }
        }
    }
    //qDebug() << "Have " << relevantFlights.size() << " relevant flights.";
}

void BSRecordingDataSource::init()
{

}

void BSRecordingDataSource::deinit()
{
    relevantFlights.clear();
    flights.clear();
}

void BSRecordingDataSource::loadFile(QString filename)
{
    workerThread = new QThread();
    worker = new BSParser();
    worker->moveToThread(workerThread);

    worker->connect(this, SIGNAL(parseFile(QString)), SLOT(loadFile(QString)));
    this->connect(worker, SIGNAL(progressUpdate(qint64,qint64)), SLOT(progressUpdate(qint64,qint64)));
    this->connect(worker, SIGNAL(done(QList<FlightP>)), SLOT(workerDone(QList<FlightP>)));

    // clean up
    workerThread->connect(worker, SIGNAL(done(QList<FlightP>)), SLOT(quit()));
    worker->connect(workerThread, SIGNAL(finished()), SLOT(deleteLater()));
    workerThread->connect(workerThread, SIGNAL(finished()), SLOT(deleteLater()));

    workerThread->start();
    if (progressBar == NULL) {
        progressBar = StelApp::getInstance().addProgressBar();
    }
    progressBar->setValue(0);
    progressBar->setRange(0, 100);
    progressBar->setFormat("Parsing BaseStation recording (%v%)");
    workerRunning = true;
    emit parseFile(filename);
}

void BSRecordingDataSource::workerDone(QList<FlightP> result)
{
    flights.clear();
    relevantFlights.clear();
    flights.append(result);
    if (flights.size() < 50) {
        // For small ammounts of flights, just add all as relevant and don't
        // manage 2 seperate lists
        relevantFlights.append(flights);
    }
    workerRunning = false;
    if (progressBar) {
        StelApp::getInstance().removeProgressBar(progressBar);
        progressBar = NULL;
    }
}

void BSRecordingDataSource::progressUpdate(qint64 done, qint64 total)
{
    if (progressBar) {
        progressBar->setValue((100.0 * done) / total);
    }
}


void BSParser::loadFile(QString filename)
{
    QHash<QString, FlightP> flightEvents;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        qint64 fileSize = file.size();
        int lineCount = 0;

        // Out parameters for parseLine()
        QString modeS;
        QString modeSHex;
        QString callsign;
        QString country;
        QString key;
        ADSBFrame frame;
        int invalids = 0;
        while (!file.atEnd()) {
            ++lineCount;
            QString line = file.readLine();
            if (ADSBData::parseLine(line, frame, modeS, modeSHex, callsign, country)) {
                key = modeS + callsign;
                if (flightEvents.contains(key)) {
                    // Flight exists, append
                    flightEvents[key]->appendSingle(frame);
                    flightEvents[key]->setInfo(callsign, country);
                } else {
                    // New flight, create and add to hash
                    QList<ADSBFrame> data;
                    data.append(frame);
                    flightEvents.insert(key, FlightP(new Flight(data, modeS, modeSHex, callsign, country)));
                }
            } else {
                ++invalids;
                //qDebug() << "got invalid line " << line;
            }
            if (lineCount % 1000) {
                emit progressUpdate(file.pos(), fileSize);
            }
        }
        qDebug() << "Parsed " << filename << ", contained " << flightEvents.size() << " flights.\nIgnored " << invalids << " lines.";
    } else {
        qDebug() << "Error opening file";
    }
    file.close();
    emit done(flightEvents.values());
}
