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

#include <QtGlobal>
#ifdef Q_OS_WIN

#ifndef OLE_HPP
#define OLE_HPP

#include <Ole2.h>
#include <QDebug>

static VARIANT g_NULL_VARIANT;
#define NULL_VARIANT (g_NULL_VARIANT)
typedef VOID(EASY_OLE_EVH)(LPVOID lpData);

BOOL OleInit(DWORD dwInitType);
HRESULT OleCreateInstance(LPCOLESTR lpszProgID, IDispatch** ppIDispatch);

HRESULT OleMethodCall(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs = 0,
  VARIANT lpArg1 = NULL_VARIANT, VARIANT lpArg2 = NULL_VARIANT, VARIANT lpArg3 = NULL_VARIANT,
  VARIANT lpArg4 = NULL_VARIANT, VARIANT lpArg5 = NULL_VARIANT);

HRESULT OlePropertyGet(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs = 0,
  VARIANT lpArg1 = NULL_VARIANT, VARIANT lpArg2 = NULL_VARIANT, VARIANT lpArg3 = NULL_VARIANT,
  VARIANT lpArg4 = NULL_VARIANT, VARIANT lpArg5 = NULL_VARIANT);

HRESULT OlePropertyPut(IDispatch* pIDispatch, VARIANT* pvResult, LPOLESTR pOleName, INT cArgs = 0,
  VARIANT lpArg1 = NULL_VARIANT, VARIANT lpArg2 = NULL_VARIANT, VARIANT lpArg3 = NULL_VARIANT,
  VARIANT lpArg4 = NULL_VARIANT, VARIANT lpArg5 = NULL_VARIANT);

VARIANT OleStringToVariant(LPOLESTR pString);
VARIANT OleIntToVariant(INT i);
VARIANT OleDoubleToVariant(double d);
VARIANT OleBoolToVariant(BOOL b);

#endif // OLE_HPP
#endif // Q_OS_WIN