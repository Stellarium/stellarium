// xlsxdatavalidation_p.h

#ifndef XLSXDATAVALIDATION_P_H
#define XLSXDATAVALIDATION_P_H

#include <QtGlobal>
#include <QSharedData>

#include "xlsxdatavalidation.h"

QT_BEGIN_NAMESPACE_XLSX

class    DataValidationPrivate : public QSharedData
{
public:
    DataValidationPrivate();
    DataValidationPrivate(DataValidation::ValidationType type, DataValidation::ValidationOperator op, const QString &formula1, const QString &formula2, bool allowBlank);
    DataValidationPrivate(const DataValidationPrivate &other);
    ~DataValidationPrivate();

    DataValidation::ValidationType validationType;
    DataValidation::ValidationOperator validationOperator;
    DataValidation::ErrorStyle errorStyle;
    bool allowBlank;
    bool isPromptMessageVisible;
    bool isErrorMessageVisible;
    QString formula1;
    QString formula2;
    QString errorMessage;
    QString errorMessageTitle;
    QString promptMessage;
    QString promptMessageTitle;
    QList<CellRange> ranges;
};

QT_END_NAMESPACE_XLSX
#endif // XLSXDATAVALIDATION_P_H
