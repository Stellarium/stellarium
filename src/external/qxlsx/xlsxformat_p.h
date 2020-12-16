/****************************************************************************
** Copyright (c) 2013-2014 Debao Zhang <hello@debao.me>
** All right reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
** NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
** LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
** OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
** WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
****************************************************************************/
#ifndef XLSXFORMAT_P_H
#define XLSXFORMAT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Xlsx API.  It exists for the convenience
// of the Qt Xlsx.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "xlsxformat.h"
#include <QSharedData>
#include <QMap>
#include <QSet>

namespace QXlsx {

class FormatPrivate : public QSharedData
{
public:
    enum FormatType
    {
        FT_Invalid = 0,
        FT_NumFmt = 0x01,
        FT_Font = 0x02,
        FT_Alignment = 0x04,
        FT_Border = 0x08,
        FT_Fill = 0x10,
        FT_Protection = 0x20
    };

    enum Property {
        P_STARTID,

        //numFmt
        P_NumFmt_Id,
        P_NumFmt_FormatCode,

        //font
        P_Font_STARTID,
        P_Font_Size = P_Font_STARTID,
        P_Font_Italic,
        P_Font_StrikeOut,
        P_Font_Color,
        P_Font_Bold,
        P_Font_Script,
        P_Font_Underline,
        P_Font_Outline,
        P_Font_Shadow,
        P_Font_Name,
        P_Font_Family,
        P_Font_Charset,
        P_Font_Scheme,
        P_Font_Condense,
        P_Font_Extend,
        P_Font_ENDID,

        //border
        P_Border_STARTID,
        P_Border_LeftStyle = P_Border_STARTID,
        P_Border_RightStyle,
        P_Border_TopStyle,
        P_Border_BottomStyle,
        P_Border_DiagonalStyle,
        P_Border_LeftColor,
        P_Border_RightColor,
        P_Border_TopColor,
        P_Border_BottomColor,
        P_Border_DiagonalColor,
        P_Border_DiagonalType,
        P_Border_ENDID,

        //fill
        P_Fill_STARTID,
        P_Fill_Pattern = P_Fill_STARTID,
        P_Fill_BgColor,
        P_Fill_FgColor,
        P_Fill_ENDID,

        //alignment
        P_Alignment_STARTID,
        P_Alignment_AlignH = P_Alignment_STARTID,
        P_Alignment_AlignV,
        P_Alignment_Wrap,
        P_Alignment_Rotation,
        P_Alignment_Indent,
        P_Alignment_ShinkToFit,
        P_Alignment_ENDID,

        //protection
        P_Protection_Locked,
        P_Protection_Hidden,

        P_ENDID
    };

    FormatPrivate();
    FormatPrivate(const FormatPrivate &other);
    ~FormatPrivate();

    bool dirty; //The key re-generation is need.
    QByteArray formatKey;

    bool font_dirty;
    bool font_index_valid;
    QByteArray font_key;
    int font_index;

    bool fill_dirty;
    bool fill_index_valid;
    QByteArray fill_key;
    int fill_index;

    bool border_dirty;
    bool border_index_valid;
    QByteArray border_key;
    int border_index;

    int xf_index;
    bool xf_indexValid;

    bool is_dxf_fomat;
    int dxf_index;
    bool dxf_indexValid;

    int theme;

    QMap<int, QVariant> properties;
};

}

#endif // XLSXFORMAT_P_H
