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

#ifndef BSPMANAGER_HPP
#define BSPMANAGER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "BSPReader.hpp"

//! @class BSPManager
//! Keeps track of the SPK kernels (*.bsp) found in one directory (for Stellarium: the "ephemBSP" folder of the
//! user data directory), opens them on demand and keeps them open for reuse.
//!
//! The directory is scanned at construction time by the owner (call rescan()) and again whenever rescan() is
//! called (e.g. from a "Refresh" button). There is no folder watcher.
//!
//! First use: TT-TDB (NAIF pseudo-body 1000000001) taken from a user-selectable kernel (default de431t.bsp,
//! which covers the full BC10000..AD10000 range). All times are JD in TDB, as in the kernels themselves.
//!
//! The class deliberately knows nothing about StelApp/StelCore so that it can be unit tested on its own.
//! Not thread safe.
class BSPManager : public QObject
{
	Q_OBJECT
public:
	//! What is known about one *.bsp file after scanning its header.
	struct KernelInfo
	{
		QString fileName;            //!< file name without path, e.g. "de431t.bsp"
		QString filePath;            //!< absolute path
		qint64 sizeBytes = 0;
		bool valid = false;          //!< readable DAF/SPK file with at least one supported segment
		int segmentCount = 0;        //!< number of supported (type 2/3) segments
		double jdBegin = 0.;         //!< earliest start of any segment, JD(TDB)
		double jdEnd = 0.;           //!< latest end of any segment, JD(TDB)
		bool hasTTminusTDB = false;  //!< contains the TT-TDB pseudo-body
		double ttJdBegin = 0.;       //!< coverage of TT-TDB, JD(TDB)
		double ttJdEnd = 0.;
	};

	//! NAIF ids of the TT-TDB pseudo-body
	static constexpr int TTMTDB_TARGET = BSPReader::TT_MINUS_TDB_TARGET;
	static constexpr int TTMTDB_CENTER = 1000000000;

	//! Name of the kernel used for TT-TDB unless another one is selected
	static QString defaultTTminusTDBKernel() { return QStringLiteral("de431t.bsp"); }
	//! Value of the TT-TDB kernel setting meaning "use no kernel" (the caller then falls back on something else).
	static QString noTTminusTDBKernel() { return QStringLiteral("none"); }

	//! @param directory folder holding the kernels (need not exist). rescan() is NOT called here.
	explicit BSPManager(const QString& directory, QObject* parent = nullptr);
	~BSPManager() override;

	QString directory() const { return dirPath; }
	//! Change the folder and rescan it.
	void setDirectory(const QString& directory);

	//! All *.bsp files found by the last rescan(), sorted by file name (case-insensitive). Unreadable files are
	//! included with valid == false so that the GUI can say so.
	const QVector<KernelInfo>& kernels() const { return kernelList; }
	//! File names of the valid kernels, or only those containing TT-TDB.
	QStringList kernelFileNames(bool onlyWithTTminusTDB = false) const;
	//! @return nullptr if no such kernel was found (file names are compared case-insensitively)
	const KernelInfo* kernelInfo(const QString& fileName) const;

	//! Reader for a kernel, opened on first use and kept open until closeAll()/rescan().
	//! @return nullptr if the kernel is unknown, invalid or cannot be opened.
	BSPReader* reader(const QString& fileName);

	//! Convenience: BSPReader::compute() on the named kernel (units as in BSPReader).
	bool compute(const QString& fileName, double jdTDB, int target, int center, double p[3], double v[3]);

	//! One text line per segment of the kernel (target/center, time span, record layout); for logs and for
	//! checking against other software.
	QStringList dumpSegments(const QString& fileName);

	// ---- TT-TDB ----
	QString ttMinusTDBKernel() const { return ttKernel; }
	//! True if the user chose not to use any kernel for TT-TDB (setting "none"). ttMinusTDB() then fails silently.
	bool ttMinusTDBKernelIsNone() const { return ttKernel == noTTminusTDBKernel(); }
	//! Select the kernel providing TT-TDB. Any file name is accepted; use ttMinusTDBAvailable() to find out
	//! whether it can actually serve a given date. An empty name or noTTminusTDBKernel() selects no kernel.
	void setTTminusTDBKernel(const QString& fileName);
	//! True if the selected kernel exists, is valid, contains TT-TDB and covers jdTDB.
	bool ttMinusTDBAvailable(double jdTDB) const;
	//! TT-TDB in seconds at jdTDB; optionally its rate of change (s/s).
	//! @return false if the selected kernel cannot provide it.
	bool ttMinusTDB(double jdTDB, double& seconds, double* rate = nullptr);
	//! JD(TT) = JD(TDB) + (TT-TDB)/86400
	bool jdTTfromTDB(double jdTDB, double& jdTT);

public slots:
	//! (Re)scan the directory. Closes all open readers.
	//! @return number of *.bsp files found (valid or not)
	int rescan();
	//! Close all open kernel files (they are reopened when needed).
	void closeAll();

signals:
	//! Emitted after every rescan(); refresh any list of kernels shown in the GUI.
	void kernelsChanged();
	//! A problem worth telling the user about (each distinct problem is reported once).
	void errorOccurred(const QString& message);

private:
	int indexOfKernel(const QString& fileName) const;
	void updateTTIndex();
	void warnOnce(const QString& key, const QString& message);

	QString dirPath;
	QVector<KernelInfo> kernelList;
	QVector<BSPReader*> readers; //!< parallel to kernelList; nullptr until opened
	QString ttKernel;
	int ttIndex = -1;            //!< index of ttKernel in kernelList, -1 if absent/invalid/without TT-TDB
	bool ttWarned = false;        //!< the "TT-TDB kernel unavailable" problem was already reported
	QStringList warned;
};

#endif // BSPMANAGER_HPP
