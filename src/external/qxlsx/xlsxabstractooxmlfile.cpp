// xlsxabstractooxmlfile.cpp

#include <QtGlobal>
#include <QBuffer>
#include <QByteArray>

#include "xlsxabstractooxmlfile.h"
#include "xlsxabstractooxmlfile_p.h"

QT_BEGIN_NAMESPACE_XLSX

AbstractOOXmlFilePrivate::AbstractOOXmlFilePrivate(AbstractOOXmlFile *q, AbstractOOXmlFile::CreateFlag flag=AbstractOOXmlFile::F_NewFromScratch)
    : relationships(new Relationships), flag(flag), q_ptr(q)
{

}

AbstractOOXmlFilePrivate::~AbstractOOXmlFilePrivate()
{

}

/*!
 * \internal
 *
 * \class AbstractOOXmlFile
 *
 * Base class of all the ooxml part file.
 */

AbstractOOXmlFile::AbstractOOXmlFile(CreateFlag flag)
    :d_ptr(new AbstractOOXmlFilePrivate(this, flag))
{
}

AbstractOOXmlFile::AbstractOOXmlFile(AbstractOOXmlFilePrivate *d)
    :d_ptr(d)
{

}

AbstractOOXmlFile::~AbstractOOXmlFile()
{
    if (d_ptr->relationships)
        delete d_ptr->relationships;
    delete d_ptr;
}

QByteArray AbstractOOXmlFile::saveToXmlData() const
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    saveToXmlFile(&buffer);

    return data;
}

bool AbstractOOXmlFile::loadFromXmlData(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    return loadFromXmlFile(&buffer);
}

/*!
 * \internal
 */
void AbstractOOXmlFile::setFilePath(const QString path)
{
    Q_D(AbstractOOXmlFile);
    d->filePathInPackage = path;
}

/*!
 * \internal
 */
QString AbstractOOXmlFile::filePath() const
{
    Q_D(const AbstractOOXmlFile);
    return d->filePathInPackage;
}


/*!
 * \internal
 */
Relationships *AbstractOOXmlFile::relationships() const
{
    Q_D(const AbstractOOXmlFile);
    return d->relationships;
}


QT_END_NAMESPACE_XLSX
