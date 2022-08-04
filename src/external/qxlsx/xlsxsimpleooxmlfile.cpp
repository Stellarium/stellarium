// xlsxsimpleooxmlfile.cpp

#include "xlsxsimpleooxmlfile_p.h"

#include <QtGlobal>
#include <QIODevice>

QT_BEGIN_NAMESPACE_XLSX

SimpleOOXmlFile::SimpleOOXmlFile(CreateFlag flag)
    :AbstractOOXmlFile(flag)
{
}

void SimpleOOXmlFile::saveToXmlFile(QIODevice *device) const
{
    device->write(xmlData);
}

QByteArray SimpleOOXmlFile::saveToXmlData() const
{
    return xmlData;
}

bool SimpleOOXmlFile::loadFromXmlData(const QByteArray &data)
{
    xmlData = data;
    return true;
}

bool SimpleOOXmlFile::loadFromXmlFile(QIODevice *device)
{
    xmlData = device->readAll();
    return true;
}

QT_END_NAMESPACE_XLSX
