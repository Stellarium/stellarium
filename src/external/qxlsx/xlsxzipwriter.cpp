// xlsxzipwriter.cpp

#include "xlsxzipwriter_p.h"

#include <QtGlobal>
#include <QDebug>


QT_BEGIN_NAMESPACE_XLSX

ZipWriter::ZipWriter(const QString &filePath)
{
    m_writer = new Stel::QZipWriter(filePath, QIODevice::WriteOnly);
    m_writer->setCompressionPolicy(Stel::QZipWriter::AutoCompress);
}

ZipWriter::ZipWriter(QIODevice *device)
{
    m_writer = new Stel::QZipWriter(device);
    m_writer->setCompressionPolicy(Stel::QZipWriter::AutoCompress);
}

ZipWriter::~ZipWriter()
{
    delete m_writer;
}

bool ZipWriter::error() const
{
    return m_writer->status() != Stel::QZipWriter::NoError;
}

void ZipWriter::addFile(const QString &filePath, QIODevice *device)
{
    m_writer->addFile(filePath, device);
}

void ZipWriter::addFile(const QString &filePath, const QByteArray &data)
{
    m_writer->addFile(filePath, data);
}

void ZipWriter::close()
{
    m_writer->close();
}

QT_END_NAMESPACE_XLSX
