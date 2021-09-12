// xlsxdatetype.cpp

#include <QtGlobal>

#include "xlsxglobal.h"
#include "xlsxutility_p.h"
#include "xlsxdatetype.h"

QT_BEGIN_NAMESPACE_XLSX

DateType::DateType()
{
}

/*
DateType::DateType(bool is1904)
{
    isSet = false;
}

DateType::DateType(double d, bool is1904)
{
    // TODO: check date

    // int iVaue = (int) d;
    // double surplus = d - double(iVaue);

    dValue = d;
    is1904Type = is1904;
    isSet = true;
}

DateType::DateType(QDateTime qdt, bool is1904)
{
    double ret = datetimeToNumber( qdt, is1904 );
    dValue = ret;
    is1904Type = is1904;
    isSet = true;
}

DateType::DateType(QDate qd, bool is1904)
{

    is1904Type = is1904;
    isSet = true;
}

DateType::DateType(QTime qt, bool is1904)
{
    double ret = timeToNumber( qt );
    dValue = ret;
    is1904Type = is1904;
    isSet = true;
}

// enum currentDateType { DateAndTimeType, OnlyDateType, OnlyTimeType };

DateType::currentDateType DateType::getType()
{

}

bool DateType::getValue(QDateTime* pQdt)
{

}


bool DateType::getValue(QDate* pQd)
{

}

bool DateType::getValue(QTime* pQt)
{

}

bool DateType::getValue(double* pD)
{

}
*/

QT_END_NAMESPACE_XLSX
