/*
 * Copyright (C) 2019 Gion Kunz <gion.kunz@gmail.com>
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


#include "OLE.hpp"
#include "TelescopeControl.hpp"
#include "ASCOMSupport.hpp"


#ifdef Q_OS_WIN

#include <comdef.h>
#include <QRegularExpression>

#endif // Q_OS_WIN

bool ASCOMSupport::isASCOMSupported()
{
	#ifdef Q_OS_WIN

	int majorVersion = getASCOMMajorVersion();

	// Check ASCOM Platform version to be 6 or greater. We apparently need 7 for Qt6!
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	if (majorVersion >= 7)
#else
	if (majorVersion >= 6)
#endif
	{
		return true;
	}

	return false;

	#else // Q_OS_WIN
	return false;
	#endif // Q_OS_WIN
}

int ASCOMSupport::getASCOMMajorVersion()
{
	#ifdef Q_OS_WIN

	VARIANT v1;
	HRESULT hResult;
	BOOL initResult;
	IDispatch* utilDispatch;

	initResult = OleInit(COINIT_APARTMENTTHREADED);
        hResult = OleCreateInstance(L"ASCOM.Utilities.Util", &utilDispatch);

	// OLE Problem
	if (FAILED(hResult) || !initResult) {
                qCCritical(Telescopes).nospace().noquote() << "ASCOM getASCOMMajorVersion(): initResult = " << initResult
                                                           << ", hResult = 0x" << QString::number(unsigned(hResult), 16)
                                                           << " = " << QString::number((hResult), 10);
                qCCritical(Telescopes) << "Problem with ASCOM. Presumably not installed properly?";
		return false;
	}

	hResult = OlePropertyGet(utilDispatch, &v1, const_cast<wchar_t*>(LPlatformVersion));
        qCInfo(Telescopes) << "ASCOM platformversion returns" << hResult;
	QString version = QString::fromStdWString(v1.bstrVal);
        QString majorVersion;

	static const QRegularExpression versionRx("^([^\\.]*)\\.([^\\.]*)$");
	QRegularExpressionMatch versionMatch=versionRx.match(version);
	if (versionMatch.hasMatch())
	{
		majorVersion = versionMatch.captured(1).trimmed();
		return majorVersion.toInt();
	}

	return 0;

	#else // Q_OS_WIN
	return 0;
	#endif // Q_OS_WIN
}

const wchar_t* ASCOMSupport::LPlatformVersion = L"PlatformVersion";
