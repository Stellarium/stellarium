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
#ifndef QXLSX_XLSXDATAVALIDATION_H
#define QXLSX_XLSXDATAVALIDATION_H

#include "xlsxglobal.h"
#include <QSharedDataPointer>
#include <QString>
#include <QList>

class QXmlStreamReader;
class QXmlStreamWriter;

QT_BEGIN_NAMESPACE_XLSX

class Worksheet;
class CellRange;
class CellReference;

class DataValidationPrivate;
class   DataValidation
{
public:
    enum ValidationType
    {
        None,
        Whole,
        Decimal,
        List,
        Date,
        Time,
        TextLength,
        Custom
    };

    enum ValidationOperator
    {
        Between,
        NotBetween,
        Equal,
        NotEqual,
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual
    };

    enum ErrorStyle
    {
        Stop,
        Warning,
        Information
    };

    DataValidation();
    DataValidation(ValidationType type, ValidationOperator op=Between, const QString &formula1=QString()
            , const QString &formula2=QString(), bool allowBlank=false);
    DataValidation(const DataValidation &other);
    ~DataValidation();

    ValidationType validationType() const;
    ValidationOperator validationOperator() const;
    ErrorStyle errorStyle() const;
    QString formula1() const;
    QString formula2() const;
    bool allowBlank() const;
    QString errorMessage() const;
    QString errorMessageTitle() const;
    QString promptMessage() const;
    QString promptMessageTitle() const;
    bool isPromptMessageVisible() const;
    bool isErrorMessageVisible() const;
    QList<CellRange> ranges() const;

    void setValidationType(ValidationType type);
    void setValidationOperator(ValidationOperator op);
    void setErrorStyle(ErrorStyle es);
    void setFormula1(const QString &formula);
    void setFormula2(const QString &formula);
    void setErrorMessage(const QString &error, const QString &title=QString());
    void setPromptMessage(const QString &prompt, const QString &title=QString());
    void setAllowBlank(bool enable);
    void setPromptMessageVisible(bool visible);
    void setErrorMessageVisible(bool visible);

    void addCell(const CellReference &cell);
    void addCell(int row, int col);
    void addRange(int firstRow, int firstCol, int lastRow, int lastCol);
    void addRange(const CellRange &range);

    DataValidation &operator=(const DataValidation &other);

    bool saveToXml(QXmlStreamWriter &writer) const;
    static DataValidation loadFromXml(QXmlStreamReader &reader);
private:
    QSharedDataPointer<DataValidationPrivate> d;
};

QT_END_NAMESPACE_XLSX

#endif // QXLSX_XLSXDATAVALIDATION_H
