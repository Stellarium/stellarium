/*
Copyright (c) 2026 Sylvain Simard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

BSPManager: finds, opens and serves NAIF/JPL SPK kernels (*.bsp)
*/

#include "BSPManager.hpp"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

BSPManager::BSPManager(const QString& directory, QObject* parent)
	: QObject(parent)
	, dirPath(directory)
	, ttKernel(defaultTTminusTDBKernel())
{
}

BSPManager::~BSPManager()
{
	closeAll();
}

void BSPManager::setDirectory(const QString& directory)
{
	dirPath = directory;
	rescan();
}

void BSPManager::closeAll()
{
	for (int i = 0; i < readers.size(); ++i)
	{
		delete readers[i];
		readers[i] = nullptr;
	}
}

int BSPManager::rescan()
{
	closeAll();
	kernelList.clear();
	readers.clear();
	warned.clear();

	const QDir dir(dirPath);
	if (dir.exists())
	{
		const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable);
		for (const QFileInfo& fi : files)
		{
			if (!fi.fileName().endsWith(QStringLiteral(".bsp"), Qt::CaseInsensitive))
				continue;

			KernelInfo ki;
			ki.fileName = fi.fileName();
			ki.filePath = fi.absoluteFilePath();
			ki.sizeBytes = fi.size();

			// Only the DAF headers and segment summaries are read here, even for multi-GB kernels
			BSPReader r(ki.filePath);
			if (r.open())
			{
				for (const BSPReader::Segment& s : r.segments())
				{
					if (ki.segmentCount == 0)
					{
						ki.jdBegin = s.jdBegin;
						ki.jdEnd = s.jdEnd;
					}
					else
					{
						ki.jdBegin = qMin(ki.jdBegin, s.jdBegin);
						ki.jdEnd = qMax(ki.jdEnd, s.jdEnd);
					}
					++ki.segmentCount;

					if (s.target == TTMTDB_TARGET && s.center == TTMTDB_CENTER)
					{
						if (!ki.hasTTminusTDB)
						{
							ki.ttJdBegin = s.jdBegin;
							ki.ttJdEnd = s.jdEnd;
						}
						else
						{
							ki.ttJdBegin = qMin(ki.ttJdBegin, s.jdBegin);
							ki.ttJdEnd = qMax(ki.ttJdEnd, s.jdEnd);
						}
						ki.hasTTminusTDB = true;
					}
				}
				ki.valid = (ki.segmentCount > 0);
				r.close();
			}
			kernelList.append(ki);
		}
	}

	std::sort(kernelList.begin(), kernelList.end(), [](const KernelInfo& a, const KernelInfo& b)
	{
		return a.fileName.toLower() < b.fileName.toLower();
	});
	readers = QVector<BSPReader*>(kernelList.size(), nullptr);
	ttWarned = false;
	updateTTIndex();

	emit kernelsChanged();
	return kernelList.size();
}

QStringList BSPManager::kernelFileNames(bool onlyWithTTminusTDB) const
{
	QStringList names;
	for (const KernelInfo& k : kernelList)
		if (k.valid && (!onlyWithTTminusTDB || k.hasTTminusTDB))
			names << k.fileName;
	return names;
}

int BSPManager::indexOfKernel(const QString& fileName) const
{
	const QString key = fileName.toLower();
	for (int i = 0; i < kernelList.size(); ++i)
		if (kernelList.at(i).fileName.toLower() == key)
			return i;
	return -1;
}

const BSPManager::KernelInfo* BSPManager::kernelInfo(const QString& fileName) const
{
	const int i = indexOfKernel(fileName);
	return (i < 0) ? nullptr : &kernelList.at(i);
}

void BSPManager::warnOnce(const QString& key, const QString& message)
{
	if (warned.contains(key))
		return;
	warned << key;
	qWarning().noquote() << message;
	emit errorOccurred(message);
}

BSPReader* BSPManager::reader(const QString& fileName)
{
	const int i = indexOfKernel(fileName);
	if (i < 0 || !kernelList.at(i).valid)
		return nullptr;
	if (readers.at(i) && readers.at(i)->isOpen())
		return readers.at(i);

	delete readers[i];
	readers[i] = new BSPReader(kernelList.at(i).filePath);
	if (!readers.at(i)->open())
	{
		delete readers[i];
		readers[i] = nullptr;
		warnOnce(QStringLiteral("open:") + fileName, QStringLiteral("BSPManager: cannot open kernel %1").arg(fileName));
		return nullptr;
	}
	return readers.at(i);
}

bool BSPManager::compute(const QString& fileName, double jdTDB, int target, int center, double p[3], double v[3])
{
	BSPReader* r = reader(fileName);
	return r && r->compute(jdTDB, target, center, p, v);
}

QStringList BSPManager::dumpSegments(const QString& fileName)
{
	QStringList lines;
	const BSPReader* r = reader(fileName);
	if (!r)
		return lines;
	for (const BSPReader::Segment& s : r->segments())
	{
		lines << QStringLiteral("%1 / %2  type %3  JD %4 .. %5  (%6 records of %7 d, %8 coefficients)")
			 .arg(s.target).arg(s.center).arg(s.type)
			 .arg(s.jdBegin, 0, 'f', 3).arg(s.jdEnd, 0, 'f', 3)
			 .arg(s.nRecords).arg(s.daysPerRecord, 0, 'g', 12).arg(s.nCoeff);
	}
	return lines;
}

void BSPManager::updateTTIndex()
{
	ttIndex = indexOfKernel(ttKernel);
	if (ttIndex >= 0 && !(kernelList.at(ttIndex).valid && kernelList.at(ttIndex).hasTTminusTDB))
		ttIndex = -1;
}

void BSPManager::setTTminusTDBKernel(const QString& fileName)
{
	const QString name = fileName.trimmed();
	ttKernel = (name.isEmpty() || name.toLower() == noTTminusTDBKernel()) ? noTTminusTDBKernel() : name;
	ttWarned = false;
	updateTTIndex();
}

bool BSPManager::ttMinusTDBAvailable(double jdTDB) const
{
	if (ttIndex < 0)
		return false;
	const KernelInfo& k = kernelList.at(ttIndex);
	return jdTDB >= k.ttJdBegin && jdTDB <= k.ttJdEnd;
}

bool BSPManager::ttMinusTDB(double jdTDB, double& seconds, double* rate)
{
	if (ttIndex < 0)
	{
		// This can be called every time the Delta-T is computed: build the message only once.
		// Nothing to report if the user deliberately chose no kernel.
		if (!ttWarned && !ttMinusTDBKernelIsNone())
		{
			ttWarned = true;
			warnOnce(QStringLiteral("tt:") + ttKernel,
				 QStringLiteral("BSPManager: kernel %1 is not available or does not contain TT-TDB").arg(ttKernel));
		}
		return false;
	}
	BSPReader* r = reader(kernelList.at(ttIndex).fileName);
	double p[3], v[3];
	if (!r || !r->compute(jdTDB, TTMTDB_TARGET, TTMTDB_CENTER, p, v))
		return false;
	seconds = p[0];
	if (rate)
		*rate = v[0];
	return true;
}

bool BSPManager::jdTTfromTDB(double jdTDB, double& jdTT)
{
	double secs;
	if (!ttMinusTDB(jdTDB, secs))
		return false;
	jdTT = jdTDB + secs / 86400.0;
	return true;
}
