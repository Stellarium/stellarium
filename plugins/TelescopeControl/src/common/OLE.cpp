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
#ifdef Q_OS_WIN
#include <Ole2.h>
#include <Windows.h>
#include <QDebug>

// Thanks: http://www.codeproject.com/Articles/34998/MS-Office-OLE-Automation-Using-C
static HRESULT OleInternalDispatch(
  INT nType, VARIANT* pvResult, IDispatch* pIDispatch, LPOLESTR pOleName, INT cArgs...)
{
	va_list vMarker;
	HRESULT hResult;
	DISPPARAMS dp = {nullptr, nullptr, 0, 0};
	DISPID dispNamedId = DISPID_PROPERTYPUT;
	DISPID dispId;
	VARIANT* pArgs = nullptr;
	EXCEPINFO execpInfo;
	UINT puArgErr = 0;

	if (!pIDispatch) return E_FAIL;

	hResult = pIDispatch->GetIDsOfNames(IID_NULL, &pOleName, 1, LOCALE_USER_DEFAULT, &dispId);
	if (FAILED(hResult))
	{
		qCWarning(Telescopes).nospace().noquote() << "OleInvoke: Failed to get DispId. hResult="
							  << hResult << " (0x" << QString::number(hResult, 16) << ").";
		return hResult;
	}

	va_start(vMarker, cArgs);

	pArgs = new VARIANT[cArgs + 1];

	// Populate pArgs in reverse order as expected by OLE
	for (INT i = (cArgs - 1); i >= 0; i--)
	{
		pArgs[i] = va_arg(vMarker, VARIANT);
	}

	dp.cArgs = cArgs;
	dp.rgvarg = pArgs;

	if (nType & DISPATCH_PROPERTYPUT)
	{
		dp.cNamedArgs = 1;
		dp.rgdispidNamedArgs = &dispNamedId;
	}

	ZeroMemory(&execpInfo, sizeof(execpInfo));

	if (pvResult) VariantInit(pvResult);

	hResult = pIDispatch->Invoke(
	  dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, nType, &dp, pvResult, &execpInfo, &puArgErr);

	if (FAILED(hResult))
	{
		qCWarning(Telescopes).nospace().noquote() << "OleInvoke: Exception [scode: " << execpInfo.scode << " (0x"
							  << QString::number(unsigned(execpInfo.scode), 16) << ") wcode: "
							  << execpInfo.wCode << " (0x" << QString::number(unsigned(execpInfo.wCode), 16)
							  << ") puArgErr: " << puArgErr << " (0x" << QString::number(puArgErr, 16) << ")] GLE: " << GetLastError();

		SysFreeString(execpInfo.bstrDescription);
		SysFreeString(execpInfo.bstrHelpFile);
		SysFreeString(execpInfo.bstrSource);
	}

	va_end(vMarker);
	delete[] pArgs;

	return hResult;
}

HRESULT OleMethodCall(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs,
  VARIANT lpArg1, VARIANT lpArg2, VARIANT lpArg3, VARIANT lpArg4, VARIANT lpArg5)
{
	return OleInternalDispatch(
	  DISPATCH_METHOD, pvResult, pIDispatch, pOleName, cArgs, lpArg1, lpArg2, lpArg3, lpArg4, lpArg5);
}

HRESULT OlePropertyGet(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs,
  VARIANT lpArg1, VARIANT lpArg2, VARIANT lpArg3, VARIANT lpArg4, VARIANT lpArg5)
{
	return OleInternalDispatch(
	  DISPATCH_PROPERTYGET, pvResult, pIDispatch, pOleName, cArgs, lpArg1, lpArg2, lpArg3, lpArg4, lpArg5);
}

HRESULT OlePropertyPut(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs,
  VARIANT lpArg1, VARIANT lpArg2, VARIANT lpArg3, VARIANT lpArg4, VARIANT lpArg5)
{
	return OleInternalDispatch(
	  DISPATCH_PROPERTYPUT, pvResult, pIDispatch, pOleName, cArgs, lpArg1, lpArg2, lpArg3, lpArg4, lpArg5);
}

VARIANT OleStringToVariant(LPOLESTR pString)
{
	VARIANT v;

	VariantInit(&v);
	V_VT(&v) = VT_BSTR;
	V_BSTR(&v) = SysAllocString(pString);

	return v;
}

VARIANT OleIntToVariant(INT i)
{
	VARIANT v;

	VariantInit(&v);
	V_VT(&v) = VT_I4;
	V_I4(&v) = i;

	return v;
}

VARIANT OleDoubleToVariant(double d)
{
	VARIANT v;

	VariantInit(&v);
	V_VT(&v) = VT_R8;
	V_R8(&v) = d;

	return v;
}

VARIANT OleBoolToVariant(BOOL b)
{
	VARIANT v;

	VariantInit(&v);
	V_VT(&v) = VT_BOOL;
	V_BOOL(&v) = b == TRUE ? -1 : 0;

	return v;
}

QString variantToQstring(VARIANT &var)
{
	VARIANT varDest;
	VariantInit(&varDest);
	QPair<QString, QString>out;

	// Attempt to convert to string type (VT_BSTR)
	if (SUCCEEDED(VariantChangeType(&varDest, &var, 0, VT_BSTR))) {
		out.first="VT_BSTR";
		out.second=QString::fromUtf16(reinterpret_cast<const ushort*>(varDest.bstrVal));
		VariantClear(&varDest); // Clean up
	} else {
		out.first="Not";
		out.second="understood";
		qCWarning(Telescopes) << "Unsupported variant type: " << var.vt;
	}
	return QString("%1: %2").arg(out.first, out.second);
}

int variantToInt(VARIANT &var)
{
	VARIANT varDest;
	VariantInit(&varDest);
	int out;

	// Attempt to convert to string type (VT_BSTR)
	if (SUCCEEDED(VariantChangeType(&varDest, &var, 0, VT_I4))){
		out=varDest.intVal;
		VariantClear(&varDest); // Clean up
	} else {
		qCWarning(Telescopes) << "Unsupported variant type: " << var.vt;
	}
	return out;
}

double variantToDouble(VARIANT &var)
{
	VARIANT varDest;
	VariantInit(&varDest);
	double out;

	// Attempt to convert to string type (VT_BSTR)
	if (SUCCEEDED(VariantChangeType(&varDest, &var, 0, VT_R8))){
		out=varDest.dblVal;
		VariantClear(&varDest); // Clean up
	} else {
		qCWarning(Telescopes) << "Unsupported variant type: " << var.vt;
	}
	return out;
}

bool variantToBool(VARIANT &var)
{
	VARIANT varDest;
	VariantInit(&varDest);
	bool out;

	// Attempt to convert to string type (VT_BSTR)
	if (SUCCEEDED(VariantChangeType(&varDest, &var, 0, VT_BOOL))){
		out = ( varDest.boolVal == 0 ? false : true );
		VariantClear(&varDest); // Clean up
	} else {
		qCWarning(Telescopes) << "Unsupported variant type: " << var.vt;
	}
	return out;
}

VOID OleReleaseInstance(IDispatch* pIDispatch)
{
	pIDispatch->Release();
}

HRESULT OleCreateInstance(LPCOLESTR lpszProgID, IDispatch** ppIDispatch)
{
	HRESULT hResult;
	CLSID clsid;
	LPVOID p;

	hResult = CLSIDFromProgID(lpszProgID, &clsid);
	if (FAILED(hResult)) return hResult;

	hResult = CoCreateInstance(
	  static_cast<REFCLSID>(clsid), nullptr, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, static_cast<REFCLSID>(IID_IDispatch), &p);

	if (FAILED(hResult)) return hResult;

	*ppIDispatch = static_cast<IDispatch*>(p);

	return hResult;
}

BOOL OleInit(DWORD dwInitType)
{
	if (!dwInitType) dwInitType = COINIT_MULTITHREADED;

	HRESULT hResult = CoInitializeEx(nullptr, dwInitType);
	if (FAILED(hResult))
	{
		return FALSE;
	}

	return TRUE;
}

#endif // Q_OS_WIN
