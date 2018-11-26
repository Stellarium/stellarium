//--------------------------------------------------------------------
//
// QXlsx
// MIT License
// https://github.com/j2doll/QXlsx
//
// QtXlsx
// https://github.com/dbzhang800/QtXlsxWriter
// http://qtxlsx.debao.me/
// MIT License

#ifndef QXLSX_XLSXDOCUMENT_H
#define QXLSX_XLSXDOCUMENT_H

#include "xlsxglobal.h"
#include "xlsxformat.h"
#include "xlsxworksheet.h"

#include <QtGlobal>
#include <QObject>
#include <QVariant>
#include <QIODevice>
class QImage;

QT_BEGIN_NAMESPACE_XLSX

class Workbook;
class Cell;
class CellRange;
class DataValidation;
class ConditionalFormatting;
class Chart;
class CellReference;

class DocumentPrivate;
class Document : public QObject
{
	Q_OBJECT
	Q_DECLARE_PRIVATE(Document)

public:
	explicit Document(QObject *parent = NULL);
	Document(const QString& xlsxName, QObject* parent = NULL);
	Document(QIODevice* device, QObject* parent = NULL);
	virtual ~Document();

	bool write(const CellReference &cell, const QVariant &value, const Format &format=Format());
	bool write(int row, int col, const QVariant &value, const Format &format=Format());
	
	QVariant read(const CellReference &cell) const;
	QVariant read(int row, int col) const;
	
	bool insertImage(int row, int col, const QImage &image);
	
	Chart *insertChart(int row, int col, const QSize &size);
	
	bool mergeCells(const CellRange &range, const Format &format=Format());
	bool unmergeCells(const CellRange &range);

	bool setColumnWidth(const CellRange &range, double width);
	bool setColumnFormat(const CellRange &range, const Format &format);
	bool setColumnHidden(const CellRange &range, bool hidden);
	bool setColumnWidth(int column, double width);
	bool setColumnFormat(int column, const Format &format);
	bool setColumnHidden(int column, bool hidden);
	bool setColumnWidth(int colFirst, int colLast, double width);
	bool setColumnFormat(int colFirst, int colLast, const Format &format);
	bool setColumnHidden(int colFirst, int colLast, bool hidden);
	
	double columnWidth(int column);
	Format columnFormat(int column);
	bool isColumnHidden(int column);

	bool setRowHeight(int row, double height);
	bool setRowFormat(int row, const Format &format);
	bool setRowHidden(int row, bool hidden);
	bool setRowHeight(int rowFirst, int rowLast, double height);
	bool setRowFormat(int rowFirst, int rowLast, const Format &format);
	bool setRowHidden(int rowFirst, int rowLast, bool hidden);

	double rowHeight(int row);
	Format rowFormat(int row);
	bool isRowHidden(int row);

	bool groupRows(int rowFirst, int rowLast, bool collapsed = true);
	bool groupColumns(int colFirst, int colLast, bool collapsed = true);
	
	bool addDataValidation(const DataValidation &validation);
	bool addConditionalFormatting(const ConditionalFormatting &cf);

	Cell *cellAt(const CellReference &cell) const;
	Cell *cellAt(int row, int col) const;

	bool defineName(const QString &name, const QString &formula, const QString &comment=QString(), const QString &scope=QString());

	CellRange dimension() const;

	QString documentProperty(const QString &name) const;
	void setDocumentProperty(const QString &name, const QString &property);
	QStringList documentPropertyNames() const;

	QStringList sheetNames() const;
	bool addSheet(const QString &name = QString(), AbstractSheet::SheetType type = AbstractSheet::ST_WorkSheet);
	bool insertSheet(int index, const QString &name = QString(), AbstractSheet::SheetType type = AbstractSheet::ST_WorkSheet);
	bool selectSheet(const QString &name);
	bool renameSheet(const QString &oldName, const QString &newName);
	bool copySheet(const QString &srcName, const QString &distName = QString());
	bool moveSheet(const QString &srcName, int distIndex);
	bool deleteSheet(const QString &name);

	Workbook *workbook() const;
	AbstractSheet *sheet(const QString &sheetName) const;
	AbstractSheet *currentSheet() const;
	Worksheet *currentWorksheet() const;

	bool save() const;
	bool saveAs(const QString &xlsXname) const;
	bool saveAs(QIODevice *device) const;

	bool isLoadPackage() const; 

	bool changeimage(int filenoinmidea,QString newfile); // add by liufeijin20181025

private:
	Q_DISABLE_COPY(Document)
	DocumentPrivate * const d_ptr;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXDOCUMENT_H
