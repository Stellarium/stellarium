// xlsxformat_p.h
#ifndef XLSXFORMAT_P_H
#define XLSXFORMAT_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxformat.h"

QT_BEGIN_NAMESPACE_XLSX

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


QT_END_NAMESPACE_XLSX

#endif
