// xlsxchart_p.h

#ifndef QXLSX_CHART_P_H
#define QXLSX_CHART_P_H

#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QVector>
#include <QMap>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "xlsxabstractooxmlfile_p.h"
#include "xlsxchart.h"

QT_BEGIN_NAMESPACE_XLSX

class XlsxSeries
{
public:
    //At present, we care about number cell ranges only!
    QString numberDataSource_numRef; // yval, val
    QString axDataSource_numRef; // xval, cat
    QString headerH_numRef;
    QString headerV_numRef;
    bool    swapHeader = false;
};

class XlsxAxis
{
public:
    enum Type { T_None = (-1), T_Cat, T_Val, T_Date, T_Ser };
    enum AxisPos { None = (-1), Left, Right, Top, Bottom };
public:
    XlsxAxis(){}

    XlsxAxis( Type t,
              XlsxAxis::AxisPos p,
              int id,
              int crossId,
              QString axisTitle = QString())
    {
        type = t;
        axisPos = p;
        axisId = id;
        crossAx = crossId;

        if ( !axisTitle.isEmpty() )
        {
            axisNames[ p ] = axisTitle;
        }
    }

public:
    Type type;
    XlsxAxis::AxisPos axisPos;
    int axisId;
    int crossAx;
    QMap< XlsxAxis::AxisPos, QString > axisNames;
};

class ChartPrivate : public AbstractOOXmlFilePrivate
{
    Q_DECLARE_PUBLIC(Chart)

public:
    ChartPrivate(Chart *q, Chart::CreateFlag flag);
    ~ChartPrivate();

public:
    bool loadXmlChart(QXmlStreamReader &reader);
    bool loadXmlPlotArea(QXmlStreamReader &reader);
protected:
    bool loadXmlPlotAreaElement(QXmlStreamReader &reader);
public:
    bool loadXmlXxxChart(QXmlStreamReader &reader);
    bool loadXmlSer(QXmlStreamReader &reader);
    QString loadXmlNumRef(QXmlStreamReader &reader);
    QString loadXmlStrRef(QXmlStreamReader &reader);
    bool loadXmlChartTitle(QXmlStreamReader &reader);
    bool loadXmlChartLegend(QXmlStreamReader &reader);
protected:
    bool loadXmlChartTitleTx(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRich(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRichP(QXmlStreamReader &reader);
    bool loadXmlChartTitleTxRichP_R(QXmlStreamReader &reader);
protected:
    bool loadXmlAxisCatAx(QXmlStreamReader &reader);
    bool loadXmlAxisDateAx(QXmlStreamReader &reader);
    bool loadXmlAxisSerAx(QXmlStreamReader &reader);
    bool loadXmlAxisValAx(QXmlStreamReader &reader);
    bool loadXmlAxisEG_AxShared(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Scaling(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Overlay(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P_pPr(QXmlStreamReader &reader, XlsxAxis* axis);
    bool loadXmlAxisEG_AxShared_Title_Tx_Rich_P_R(QXmlStreamReader &reader, XlsxAxis* axis);

    QString readSubTree(QXmlStreamReader &reader);

public:
    void saveXmlChart(QXmlStreamWriter &writer) const;
    void saveXmlChartTitle(QXmlStreamWriter &writer) const;
    void saveXmlPieChart(QXmlStreamWriter &writer) const;
    void saveXmlBarChart(QXmlStreamWriter &writer) const;
    void saveXmlLineChart(QXmlStreamWriter &writer) const;
    void saveXmlScatterChart(QXmlStreamWriter &writer) const;
    void saveXmlAreaChart(QXmlStreamWriter &writer) const;
    void saveXmlDoughnutChart(QXmlStreamWriter &writer) const;
    void saveXmlSer(QXmlStreamWriter &writer, XlsxSeries *ser, int id) const;
    void saveXmlAxis(QXmlStreamWriter &writer) const;
    void saveXmlChartLegend(QXmlStreamWriter &writer) const;

protected:
    void saveXmlAxisCatAx(QXmlStreamWriter &writer, XlsxAxis* axis) const;
    void saveXmlAxisDateAx(QXmlStreamWriter &writer, XlsxAxis* axis) const;
    void saveXmlAxisSerAx(QXmlStreamWriter &writer, XlsxAxis* axis) const;
    void saveXmlAxisValAx(QXmlStreamWriter &writer, XlsxAxis* axis) const;

    void saveXmlAxisEG_AxShared(QXmlStreamWriter &writer, XlsxAxis* axis) const;
    void saveXmlAxisEG_AxShared_Title(QXmlStreamWriter &writer, XlsxAxis* axis) const;
    QString GetAxisPosString( XlsxAxis::AxisPos axisPos ) const;
    QString GetAxisName(XlsxAxis* ptrXlsxAxis) const;

public:
    Chart::ChartType chartType;
    QList< QSharedPointer<XlsxSeries> > seriesList;
    QList< QSharedPointer<XlsxAxis> > axisList;
    QMap< XlsxAxis::AxisPos, QString > axisNames;
    QString chartTitle;
    AbstractSheet* sheet;
    Chart::ChartAxisPos legendPos;
    bool legendOverlay;
    bool majorGridlinesEnabled;
    bool minorGridlinesEnabled;

    QString layout;             // only for storing a readed file
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_CHART_P_H
