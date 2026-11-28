/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#ifndef EOPTESTDATA_HPP
#define EOPTESTDATA_HPP

// (SS) 2026-10-07 Synthetic IERS EOP CSV file contents for testEOPReader and testEOPManager

#include <QDate>
#include <QFile>
#include <QString>
#include <QStringList>

namespace EOPTestData
{
inline QString num(double v, int prec = 6)
{
	return QString::number(v, 'f', prec);
}

//! Long term series (EOP C04): 27 columns
inline QString c04Header()
{
	return QStringLiteral("MJD;Year;Month;Day;Type;x_pole;sigma_x_pole;y_pole;sigma_y_pole;x_rate;sigma_x_rate;y_rate;sigma_y_rate;Type;UT1-UTC;sigma_UT1-UTC;LOD;sigma_LOD;Type;dPsi;sigma_dPsi;dEpsilon;sigma_dEpsilon;dX;sigma_dX;dY;sigma_dY\r\n");
}

//! Standard series (finals): the same 27 columns and 10 Bulletin B columns
inline QString finalsHeader()
{
	QString h = c04Header();
	h.chop(2);
	return h + QStringLiteral(";Type;bulB/x_pole;bulB/y_pole;Type;bulB/UT-UTC;Type;bulB/dPsi;bulB/dEpsilon;bulB/dX;bulB/dY\r\n");
}

inline QStringList dateFields(int mjd, const QString& mjdText)
{
	const QDate d = QDate::fromJulianDay(qint64(mjd) + 2400001);
	return QStringList() << mjdText << QString::number(d.year()) << d.toString("MM") << d.toString("dd");
}

//! C04 row (pole and UT1-UTC in arcsec and s, LOD in s, dPsi dEps dX dY in arcsec). Use "" for no value.
inline QString c04Row(int mjd, double xp, double ut1, double lod,
                      const QString& dpsi, const QString& deps, const QString& dx = QString(), const QString& dy = QString())
{
	QStringList f = dateFields(mjd, QString("%1.00").arg(mjd));
	f << "" << num(xp) << "0.030000" << "0.200000" << "0.030000" << "" << "" << "" << "" << ""
	  << num(ut1, 7) << "0.0020000" << num(lod, 7) << "0.0014000" << ""
	  << dpsi << "0.012000" << deps << "0.002000" << dx << "0.004774" << dy << "0.002000";
	return f.join(';') + "\r\n";
}

//! C04 IAU 1980 file for MJD first..last: xp .10 + .01 i, UT1-UTC .2 - .001 i, LOD .001, dPsi .01 i + dpsiOffset, dEps .006, i = MJD - 60000
inline QString c04File(int first, int last, double dpsiOffset = 0.)
{
	QString s = c04Header();
	for (int m = first; m <= last; ++m)
	{
		const int i = m - 60000;
		s += c04Row(m, 0.10 + 0.01 * i, 0.2 - 0.001 * i, 0.001, num(0.01 * i + dpsiOffset), "0.006000");
	}
	return s;
}

//! C04 IAU 2000 file for MJD first..last: same pole, UT1-UTC and LOD, no dPsi/dEps, dX = 0.0001 i, dY = 0
inline QString c04File2000(int first, int last)
{
	QString s = c04Header();
	for (int m = first; m <= last; ++m)
	{
		const int i = m - 60000;
		s += c04Row(m, 0.10 + 0.01 * i, 0.2 - 0.001 * i, 0.001, "", "", num(0.0001 * i), "0.000000");
	}
	return s;
}

//! Finals row: pole in arcsec, UT1-UTC in s, LOD in msec, dPsi dEps dX dY in marcsec. Use "" for no value.
inline QString finalsRow(int mjd, const QString& poleType, const QString& x, const QString& y,
                         const QString& utType, const QString& ut, const QString& lod,
                         const QString& nutType, const QString& dpsi, const QString& deps,
                         const QString& dx = QString(), const QString& dy = QString())
{
	QStringList f = dateFields(mjd, QString::number(mjd));
	f << poleType << x << "0.010000" << y << "0.010000" << "" << "" << "" << ""
	  << utType << ut << "0.002000" << lod << "0.020000"
	  << nutType << dpsi << "0.5" << deps << "0.3" << dx << "0.2" << dy << "0.2";
	for (int i = 0; i < 10; ++i)
		f << "";
	return f.join(';') + ";\r\n";
}

//! final values of every group (LOD 1.5 msec)
inline QString finalsFinalRow(int mjd, const QString& x, const QString& y, const QString& ut,
                              const QString& dpsi, const QString& deps)
{
	return finalsRow(mjd, "final", x, y, "final", ut, "1.5", "final", dpsi, deps);
}

//! predicted pole and UT1-UTC, no LOD, no nutation
inline QString finalsPredictionRow(int mjd, const QString& x, const QString& y, const QString& ut)
{
	return finalsRow(mjd, "prediction", x, y, "prediction", ut, "", "", "", "");
}

inline bool writeFile(const QString& path, const QString& content)
{
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly))
		return false;
	f.write(content.toUtf8());
	return true;
}
}

#endif // EOPTESTDATA_HPP
