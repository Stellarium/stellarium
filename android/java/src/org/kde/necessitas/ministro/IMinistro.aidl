/*
    Copyright (c) 2011, BogDan Vatra <bog_dan_ro@yahoo.com>

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


package org.kde.necessitas.ministro;

import org.kde.necessitas.ministro.IMinistroCallback;

interface IMinistro
{
/**
* Check/download required libs to run the application
*
* param callback  - interface used by Minsitro service to notify the client when the loader is ready
* param parameters
*            parameters fields:
*                 * Key Name                   Key type         Explanations
*                   "required.modules"         StringArray      Required modules by your application
*                   "application.title"        String           Application name, used to show more informations to user
*                   "qt.provider"              String           Qt libs provider, currently only "necessitas" is supported.
*                   "minimum.ministro.api"     Integer          Minimum Ministro API level, used to check if Ministro service compatible with your application. Current API Level is 1 !
*                   "minimum.qt.version"       Integer          Minimim Qt version (e.g. 0x040800, which means Qt 4.8.0, check http://doc.trolltech.com/4.8/qtglobal.html#QT_VERSION)!
*/
    void requestLoader(in IMinistroCallback callback, in Bundle parameters);
}
