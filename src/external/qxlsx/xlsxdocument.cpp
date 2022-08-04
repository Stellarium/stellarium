// xlsxdocument.cpp

#include <QtGlobal>
#include <QFile>
#include <QPointF>
#include <QBuffer>
#include <QDir>
#include <QTemporaryFile>
#include <QFile>
#include <QSharedPointer>
#include <QDebug>

#include "xlsxdocument.h"
#include "xlsxdocument_p.h"
#include "xlsxworkbook.h"
#include "xlsxworksheet.h"
#include "xlsxcontenttypes_p.h"
#include "xlsxrelationships_p.h"
#include "xlsxstyles_p.h"
#include "xlsxtheme_p.h"
#include "xlsxdocpropsapp_p.h"
#include "xlsxdocpropscore_p.h"
#include "xlsxsharedstrings_p.h"
#include "xlsxutility_p.h"
#include "xlsxworkbook_p.h"
#include "xlsxdrawing_p.h"
#include "xlsxmediafile_p.h"
#include "xlsxchart.h"
#include "xlsxzipreader_p.h"
#include "xlsxzipwriter_p.h"

/*
	From Wikipedia: The Open Packaging Conventions (OPC) is a
	container-file technology initially created by Microsoft to store
	a combination of XML and non-XML files that together form a single
	entity such as an Open XML Paper Specification (OpenXPS)
	document. http://en.wikipedia.org/wiki/Open_Packaging_Conventions.

	At its simplest an Excel XLSX file contains the following elements:

		 ____ [Content_Types].xml
		|
		|____ docProps
		| |____ app.xml
		| |____ core.xml
		|
		|____ xl
		| |____ workbook.xml
		| |____ worksheets
		| | |____ sheet1.xml
		| |
		| |____ styles.xml
		| |
		| |____ theme
		| | |____ theme1.xml
		| |
		| |_____rels
		| |____ workbook.xml.rels
		|
		|_____rels
		  |____ .rels

	The Packager class coordinates the classes that represent the
	elements of the package and writes them into the XLSX file.
*/

QT_BEGIN_NAMESPACE_XLSX

namespace xlsxDocumentCpp {
	std::string copyTag(const std::string &sFrom, const std::string &sTo, const std::string &tag) {
		const std::string tagToFindStart = "<" + tag;
		const std::string tagToFindEnd = "</" + tag;
		const std::string tagEnd = "</" + tag + ">";

		// search all occurrences of tag in 'sFrom'
		std::string sFromData = "";
		size_t startIndex = 0;
		while (true) {
			std::size_t startPos = sFrom.find(tagToFindStart, startIndex);
			if (startPos != std::string::npos) {
				std::size_t endPos = sFrom.find(tagToFindEnd, startPos);
				std::string tagEndTmp = tagEnd;
				if (endPos == std::string::npos) {	// second try to find the ending, maybe it is "/>" 
					endPos = sFrom.find("/>", startPos);
					tagEndTmp = "/>";
				}
				if (endPos != std::string::npos) {
					sFromData += sFrom.substr(startPos, endPos - startPos) + tagEndTmp;
					startIndex = endPos + strlen(tagEndTmp.c_str());
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}

		std::string sOut = sTo; // copy 'sTo' in the output string

		if (!sFromData.empty()) { // tag found in 'from'?
								  // search all occurrences of tag in 'sOut' and delete them
			int firstPosTag = -1;
			while (true) {
				std::size_t startPos = sOut.find(tagToFindStart);
				if (startPos != std::string::npos) {
					std::size_t endPos = sOut.find(tagToFindEnd);
					std::string tagEndTmp = tagEnd;
					if (endPos == std::string::npos) {	// second try to find the ending, maybe it is "/>" 
						endPos = sOut.find("/>", startPos);
						tagEndTmp = "/>";
					}
					if (endPos != std::string::npos) {
						if (firstPosTag < 0)
							firstPosTag = startPos;
						std::string stringBefore = sOut.substr(0, startPos);
						endPos += strlen(tagEndTmp.c_str());
						std::string stringAfter = sOut.substr(endPos, strlen(sOut.c_str()) - endPos);
						sOut = stringBefore + stringAfter;
					}
					else {
						break;
					}
				}
				else {
					break;
				}
			}

			if (firstPosTag == -1) {
				// tag not found in 'sTo' file
				// try to find a default pos using standard tags
				std::vector<std::string> defaultPos{ "</styleSheet>", "<pageMargins", "</workbook>" };
				for (unsigned int i = 0; i < defaultPos.size(); ++i) {
					std::size_t iDefaultPos = sOut.find(defaultPos[i]);
					if (iDefaultPos != std::string::npos) {
						firstPosTag = iDefaultPos;
						break;
					}
				}
			}

			// add the tag extracted from 'sFrom' in 'sOut'
			// add in the position of the first tag found in 'sOut' ('firstPosTag')
			if (firstPosTag >= 0) {
				std::string stringBefore = sOut.substr(0, firstPosTag);
				std::string stringAfter = sOut.substr(firstPosTag, strlen(sOut.c_str()) - firstPosTag);
				sOut = stringBefore + sFromData + stringAfter;
			}
		}

		return sOut;
	}
}

DocumentPrivate::DocumentPrivate(Document *p) :
	q_ptr(p), defaultPackageName(QStringLiteral("Book1.xlsx")),
	isLoad(false)
{
}

void DocumentPrivate::init()
{
	if (contentTypes.isNull())
		contentTypes = QSharedPointer<ContentTypes>(new ContentTypes(ContentTypes::F_NewFromScratch));

	if (workbook.isNull())
		workbook = QSharedPointer<Workbook>(new Workbook(Workbook::F_NewFromScratch));
}

bool DocumentPrivate::loadPackage(QIODevice *device)
{
	Q_Q(Document);
	ZipReader zipReader(device);
	QStringList filePaths = zipReader.filePaths();

	//Load the Content_Types file
	if (!filePaths.contains(QLatin1String("[Content_Types].xml")))
		return false;
	contentTypes = QSharedPointer<ContentTypes>(new ContentTypes(ContentTypes::F_LoadFromExists));
	contentTypes->loadFromXmlData(zipReader.fileData(QStringLiteral("[Content_Types].xml")));

	//Load root rels file
	if (!filePaths.contains(QLatin1String("_rels/.rels")))
		return false;
	Relationships rootRels;
	rootRels.loadFromXmlData(zipReader.fileData(QStringLiteral("_rels/.rels")));

	//load core property
	QList<XlsxRelationship> rels_core = rootRels.packageRelationships(QStringLiteral("/metadata/core-properties"));
	if (!rels_core.isEmpty()) {
		//Get the core property file name if it exists.
		//In normal case, this should be "docProps/core.xml"
		QString docPropsCore_Name = rels_core[0].target;

		DocPropsCore props(DocPropsCore::F_LoadFromExists);
		props.loadFromXmlData(zipReader.fileData(docPropsCore_Name));
        const auto propNames = props.propertyNames();
        for (const QString &name : propNames)
			q->setDocumentProperty(name, props.property(name));
	}

	//load app property
	QList<XlsxRelationship> rels_app = rootRels.documentRelationships(QStringLiteral("/extended-properties"));
	if (!rels_app.isEmpty()) {
		//Get the app property file name if it exists.
		//In normal case, this should be "docProps/app.xml"
		QString docPropsApp_Name = rels_app[0].target;

		DocPropsApp props(DocPropsApp::F_LoadFromExists);
		props.loadFromXmlData(zipReader.fileData(docPropsApp_Name));
        const auto propNames = props.propertyNames();
        for (const QString &name : propNames)
			q->setDocumentProperty(name, props.property(name));
	}

	//load workbook now, Get the workbook file path from the root rels file
	//In normal case, this should be "xl/workbook.xml"
	workbook = QSharedPointer<Workbook>(new Workbook(Workbook::F_LoadFromExists));
	QList<XlsxRelationship> rels_xl = rootRels.documentRelationships(QStringLiteral("/officeDocument"));
	if (rels_xl.isEmpty())
		return false;
    const QString xlworkbook_Path = rels_xl[0].target;
    const QString xlworkbook_Dir = *( splitPath(xlworkbook_Path).begin() );
    const QString relFilePath = getRelFilePath(xlworkbook_Path);

    workbook->relationships()->loadFromXmlData( zipReader.fileData(relFilePath) );
	workbook->setFilePath(xlworkbook_Path);
	workbook->loadFromXmlData(zipReader.fileData(xlworkbook_Path));

	//load styles
	QList<XlsxRelationship> rels_styles = workbook->relationships()->documentRelationships(QStringLiteral("/styles"));
	if (!rels_styles.isEmpty()) {
		//In normal case this should be styles.xml which in xl
		QString name = rels_styles[0].target;

        // dev34
        QString path;
        if ( xlworkbook_Dir == QLatin1String(".") ) // root
        {
            path = name;
        }
        else
        {
            path = xlworkbook_Dir + QLatin1String("/") + name;
        }

		QSharedPointer<Styles> styles (new Styles(Styles::F_LoadFromExists));
		styles->loadFromXmlData(zipReader.fileData(path));
		workbook->d_func()->styles = styles;
	}

	//load sharedStrings
	QList<XlsxRelationship> rels_sharedStrings = workbook->relationships()->documentRelationships(QStringLiteral("/sharedStrings"));
	if (!rels_sharedStrings.isEmpty()) {
		//In normal case this should be sharedStrings.xml which in xl
		QString name = rels_sharedStrings[0].target;
		QString path = xlworkbook_Dir + QLatin1String("/") + name;
		workbook->d_func()->sharedStrings->loadFromXmlData(zipReader.fileData(path));
	}

	//load theme
	QList<XlsxRelationship> rels_theme = workbook->relationships()->documentRelationships(QStringLiteral("/theme"));
	if (!rels_theme.isEmpty()) {
		//In normal case this should be theme/theme1.xml which in xl
		QString name = rels_theme[0].target;
		QString path = xlworkbook_Dir + QLatin1String("/") + name;
		workbook->theme()->loadFromXmlData(zipReader.fileData(path));
	}

	//load sheets
	for (int i=0; i<workbook->sheetCount(); ++i) {
		AbstractSheet *sheet = workbook->sheet(i);
        QString strFilePath = sheet->filePath();
        QString rel_path = getRelFilePath(strFilePath);
		//If the .rel file exists, load it.
		if (zipReader.filePaths().contains(rel_path))
			sheet->relationships()->loadFromXmlData(zipReader.fileData(rel_path));
		sheet->loadFromXmlData(zipReader.fileData(sheet->filePath()));
	}

	//load external links
	for (int i=0; i<workbook->d_func()->externalLinks.count(); ++i) {
		SimpleOOXmlFile *link = workbook->d_func()->externalLinks[i].data();
		QString rel_path = getRelFilePath(link->filePath());
		//If the .rel file exists, load it.
		if (zipReader.filePaths().contains(rel_path))
			link->relationships()->loadFromXmlData(zipReader.fileData(rel_path));
		link->loadFromXmlData(zipReader.fileData(link->filePath()));
	}

	//load drawings
	for (int i=0; i<workbook->drawings().size(); ++i) {
		Drawing *drawing = workbook->drawings()[i];
		QString rel_path = getRelFilePath(drawing->filePath());
		if (zipReader.filePaths().contains(rel_path))
			drawing->relationships()->loadFromXmlData(zipReader.fileData(rel_path));
		drawing->loadFromXmlData(zipReader.fileData(drawing->filePath()));
	}

	//load charts
	QList<QSharedPointer<Chart> > chartFileToLoad = workbook->chartFiles();
	for (int i=0; i<chartFileToLoad.size(); ++i) {
		QSharedPointer<Chart> cf = chartFileToLoad[i];
		cf->loadFromXmlData(zipReader.fileData(cf->filePath()));
	}

	//load media files
	QList<QSharedPointer<MediaFile> > mediaFileToLoad = workbook->mediaFiles();
	for (int i=0; i<mediaFileToLoad.size(); ++i) {
		QSharedPointer<MediaFile> mf = mediaFileToLoad[i];
		const QString path = mf->fileName();
		const QString suffix = path.mid(path.lastIndexOf(QLatin1Char('.'))+1);
		mf->set(zipReader.fileData(path), suffix);
	}

	isLoad = true; 
	return true;
}

bool DocumentPrivate::savePackage(QIODevice *device) const
{
	Q_Q(const Document);

	ZipWriter zipWriter(device);
	if (zipWriter.error())
		return false;

	contentTypes->clearOverrides();

	DocPropsApp docPropsApp(DocPropsApp::F_NewFromScratch);
	DocPropsCore docPropsCore(DocPropsCore::F_NewFromScratch);

	// save worksheet xml files
	QList<QSharedPointer<AbstractSheet> > worksheets = workbook->getSheetsByTypes(AbstractSheet::ST_WorkSheet);
	if (!worksheets.isEmpty())
		docPropsApp.addHeadingPair(QStringLiteral("Worksheets"), worksheets.size());

    for (int i = 0 ; i < worksheets.size(); ++i)
    {
		QSharedPointer<AbstractSheet> sheet = worksheets[i];
        contentTypes->addWorksheetName(QStringLiteral("sheet%1").arg(i+1));
		docPropsApp.addPartTitle(sheet->sheetName());

        zipWriter.addFile(QStringLiteral("xl/worksheets/sheet%1.xml").arg(i+1), sheet->saveToXmlData());

		Relationships *rel = sheet->relationships();
		if (!rel->isEmpty())
            zipWriter.addFile(QStringLiteral("xl/worksheets/_rels/sheet%1.xml.rels").arg(i+1), rel->saveToXmlData());
	}

	//save chartsheet xml files
	QList<QSharedPointer<AbstractSheet> > chartsheets = workbook->getSheetsByTypes(AbstractSheet::ST_ChartSheet);
	if (!chartsheets.isEmpty())
        docPropsApp.addHeadingPair(QStringLiteral("Chartsheets"), chartsheets.size());
    for (int i=0; i<chartsheets.size(); ++i)
    {
		QSharedPointer<AbstractSheet> sheet = chartsheets[i];
        contentTypes->addWorksheetName(QStringLiteral("sheet%1").arg(i+1));
		docPropsApp.addPartTitle(sheet->sheetName());

        zipWriter.addFile(QStringLiteral("xl/chartsheets/sheet%1.xml").arg(i+1), sheet->saveToXmlData());
		Relationships *rel = sheet->relationships();
		if (!rel->isEmpty())
            zipWriter.addFile(QStringLiteral("xl/chartsheets/_rels/sheet%1.xml.rels").arg(i+1), rel->saveToXmlData());
	}

	// save external links xml files
    for (int i=0; i<workbook->d_func()->externalLinks.count(); ++i)
    {
		SimpleOOXmlFile *link = workbook->d_func()->externalLinks[i].data();
        contentTypes->addExternalLinkName(QStringLiteral("externalLink%1").arg(i+1));

        zipWriter.addFile(QStringLiteral("xl/externalLinks/externalLink%1.xml").arg(i+1), link->saveToXmlData());
		Relationships *rel = link->relationships();
		if (!rel->isEmpty())
            zipWriter.addFile(QStringLiteral("xl/externalLinks/_rels/externalLink%1.xml.rels").arg(i+1), rel->saveToXmlData());
	}

	// save workbook xml file
	contentTypes->addWorkbook();
	zipWriter.addFile(QStringLiteral("xl/workbook.xml"), workbook->saveToXmlData());
	zipWriter.addFile(QStringLiteral("xl/_rels/workbook.xml.rels"), workbook->relationships()->saveToXmlData());

	// save drawing xml files
    for (int i=0; i<workbook->drawings().size(); ++i)
    {
        contentTypes->addDrawingName(QStringLiteral("drawing%1").arg(i+1));

		Drawing *drawing = workbook->drawings()[i];
        zipWriter.addFile(QStringLiteral("xl/drawings/drawing%1.xml").arg(i+1), drawing->saveToXmlData());
		if (!drawing->relationships()->isEmpty())
            zipWriter.addFile(QStringLiteral("xl/drawings/_rels/drawing%1.xml.rels").arg(i+1), drawing->relationships()->saveToXmlData());
	}

	// save docProps app/core xml file
    const auto docPropNames = q->documentPropertyNames();
    for (const QString &name : docPropNames) {
		docPropsApp.setProperty(name, q->documentProperty(name));
		docPropsCore.setProperty(name, q->documentProperty(name));
	}
	contentTypes->addDocPropApp();
	contentTypes->addDocPropCore();
	zipWriter.addFile(QStringLiteral("docProps/app.xml"), docPropsApp.saveToXmlData());
	zipWriter.addFile(QStringLiteral("docProps/core.xml"), docPropsCore.saveToXmlData());

	// save sharedStrings xml file
	if (!workbook->sharedStrings()->isEmpty()) {
		contentTypes->addSharedString();
		zipWriter.addFile(QStringLiteral("xl/sharedStrings.xml"), workbook->sharedStrings()->saveToXmlData());
	}

    // save calc chain [dev16]
    contentTypes->addCalcChain();
    zipWriter.addFile(QStringLiteral("xl/calcChain.xml"), workbook->styles()->saveToXmlData());

	// save styles xml file
	contentTypes->addStyles();
	zipWriter.addFile(QStringLiteral("xl/styles.xml"), workbook->styles()->saveToXmlData());

	// save theme xml file
	contentTypes->addTheme();
	zipWriter.addFile(QStringLiteral("xl/theme/theme1.xml"), workbook->theme()->saveToXmlData());

	// save chart xml files
    for (int i=0; i<workbook->chartFiles().size(); ++i)
    {
        contentTypes->addChartName(QStringLiteral("chart%1").arg(i+1));
		QSharedPointer<Chart> cf = workbook->chartFiles()[i];
        zipWriter.addFile(QStringLiteral("xl/charts/chart%1.xml").arg(i+1), cf->saveToXmlData());
	}

	// save image files
    for (int i=0; i<workbook->mediaFiles().size(); ++i)
    {
		QSharedPointer<MediaFile> mf = workbook->mediaFiles()[i];
		if (!mf->mimeType().isEmpty())
			contentTypes->addDefault(mf->suffix(), mf->mimeType());

        zipWriter.addFile(QStringLiteral("xl/media/image%1.%2").arg(i+1).arg(mf->suffix()), mf->contents());
	}

	// save root .rels xml file
	Relationships rootrels;
	rootrels.addDocumentRelationship(QStringLiteral("/officeDocument"), QStringLiteral("xl/workbook.xml"));
	rootrels.addPackageRelationship(QStringLiteral("/metadata/core-properties"), QStringLiteral("docProps/core.xml"));
	rootrels.addDocumentRelationship(QStringLiteral("/extended-properties"), QStringLiteral("docProps/app.xml"));
	zipWriter.addFile(QStringLiteral("_rels/.rels"), rootrels.saveToXmlData());

	// save content types xml file
	zipWriter.addFile(QStringLiteral("[Content_Types].xml"), contentTypes->saveToXmlData());

	zipWriter.close();
	return true;
}

bool DocumentPrivate::copyStyle(const QString &from, const QString &to)
{
	// create a temp file because the zip writer cannot modify already existing zips
	QTemporaryFile tempFile;
	tempFile.open();
	tempFile.close();
	QString temFilePath = QFileInfo(tempFile).absoluteFilePath();

	ZipWriter temporalZip(temFilePath);

	ZipReader zipReader(from);
	QStringList filePaths = zipReader.filePaths();

    QSharedPointer<ZipReader> toReader = QSharedPointer<ZipReader>(new ZipReader(to));

	QStringList toFilePaths = toReader->filePaths();

	// copy all files from "to" zip except those related to style
	for (int i = 0; i < toFilePaths.size(); i++) {
        if (toFilePaths[i].contains(QLatin1String("xl/styles"))) {
			if (filePaths.contains(toFilePaths[i])) {	// style file exist in 'from' as well
				// modify style file
                std::string fromData = QString::fromUtf8(zipReader.fileData(toFilePaths[i])).toStdString();
                std::string toData = QString::fromUtf8(toReader->fileData(toFilePaths[i])).toStdString();
				// copy default theme style from 'from' to 'to'
				toData = xlsxDocumentCpp::copyTag(fromData, toData, "dxfs");
				temporalZip.addFile(toFilePaths.at(i), QString::fromUtf8(toData.c_str()).toUtf8());

				continue;
			}
		}

        if (toFilePaths[i].contains(QLatin1String("xl/workbook"))) {
			if (filePaths.contains(toFilePaths[i])) {	// workbook file exist in 'from' as well
				// modify workbook file
                std::string fromData = QString::fromUtf8(zipReader.fileData(toFilePaths[i])).toStdString();
                std::string toData = QString::fromUtf8(toReader->fileData(toFilePaths[i])).toStdString();
				// copy default theme style from 'from' to 'to'
				toData = xlsxDocumentCpp::copyTag(fromData, toData, "workbookPr");
				temporalZip.addFile(toFilePaths.at(i), QString::fromUtf8(toData.c_str()).toUtf8());
				continue;
			}
		}

        if (toFilePaths[i].contains(QLatin1String("xl/worksheets/sheet"))) {
			if (filePaths.contains(toFilePaths[i])) {	// sheet file exist in 'from' as well
				// modify sheet file
                std::string fromData = QString::fromUtf8(zipReader.fileData(toFilePaths[i])).toStdString();
                std::string toData = QString::fromUtf8(toReader->fileData(toFilePaths[i])).toStdString();
				// copy "conditionalFormatting" from 'from' to 'to'
				toData = xlsxDocumentCpp::copyTag(fromData, toData, "conditionalFormatting");
				temporalZip.addFile(toFilePaths.at(i), QString::fromUtf8(toData.c_str()).toUtf8());
				continue;
			}
		}

		QByteArray data = toReader->fileData(toFilePaths.at(i));
		temporalZip.addFile(toFilePaths.at(i), data);
	}

	temporalZip.close();

    toReader.clear();

	tempFile.close();

	QFile::remove(to);
	tempFile.copy(to);

	return true;
}

/*!
  \class Document
  \inmodule QtXlsx
  \brief The Document class provides a API that is used to handle the contents of .xlsx files.

*/

/*!
 * Creates a new empty xlsx document.
 * The \a parent argument is passed to QObject's constructor.
 */
Document::Document(QObject *parent) :
	QObject(parent), d_ptr(new DocumentPrivate(this))
{
	d_ptr->init();
}

/*!
 * \overload
 * Try to open an existing xlsx document named \a name.
 * The \a parent argument is passed to QObject's constructor.
 */
Document::Document(const QString &name, 
					QObject *parent) :
	QObject(parent), 
	d_ptr(new DocumentPrivate(this))
{
	d_ptr->packageName = name; 

	if (QFile::exists(name)) 
	{
		QFile xlsx(name);
		if (xlsx.open(QFile::ReadOnly))
		{
			if (! d_ptr->loadPackage(&xlsx))
			{
				// NOTICE: failed to load package 
			}
		}
	}

	d_ptr->init();
}

/*!
 * \overload
 * Try to open an existing xlsx document from \a device.
 * The \a parent argument is passed to QObject's constructor.
 */
Document::Document(QIODevice *device, QObject *parent) :
	QObject(parent), d_ptr(new DocumentPrivate(this))
{
	if (device && device->isReadable())
	{
		if (!d_ptr->loadPackage(device))
		{
			// NOTICE: failed to load package 
		}
	}
	d_ptr->init();
}

/*!
	\overload

	Write \a value to cell \a row_column with the given \a format.
 */
bool Document::write(const CellReference &row_column, const QVariant &value, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->write(row_column, value, format);
	return false;
}

/*!
 * Write \a value to cell (\a row, \a col) with the \a format.
 * Returns true on success.
 */
bool Document::write(int row, int col, const QVariant &value, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->write(row, col, value, format);
	return false;
}

/*!
	\overload
	Returns the contents of the cell \a cell.

	\sa cellAt()
*/
QVariant Document::read(const CellReference &cell) const
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->read(cell);
	return QVariant();
}

/*!
	Returns the contents of the cell (\a row, \a col).

	\sa cellAt()
 */
QVariant Document::read(int row, int col) const
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->read(row, col);
	return QVariant();
}

/*!
 * Insert an \a image to current active worksheet at the position \a row, \a column
 * Returns ture if success.
 */
int Document::insertImage(int row, int column, const QImage &image)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->insertImage(row, column, image);

    return 0;
}

bool Document::getImage(int imageIndex, QImage& img)
{
    if (Worksheet *sheet = currentWorksheet())
        return sheet->getImage(imageIndex, img);

    return  false;
}

bool Document::getImage(int row, int col, QImage &img)
{
    if (Worksheet *sheet = currentWorksheet())
        return sheet->getImage(row, col, img);

    return  false;
}

uint Document::getImageCount()
{
    if (Worksheet *sheet = currentWorksheet())
        return sheet->getImageCount();

    return 0;
}


/*!
 * Creates an chart with the given \a size and insert it to the current
 * active worksheet at the position \a row, \a col.
 * The chart will be returned.
 */
Chart *Document::insertChart(int row, int col, const QSize &size)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->insertChart(row, col, size);
	return 0;
}

/*!
  Merge a \a range of cells. The first cell should contain the data and the others should
  be blank. All cells will be applied the same style if a valid \a format is given.
  Returns true on success.

  \note All cells except the top-left one will be cleared.
 */
bool Document::mergeCells(const CellRange &range, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->mergeCells(range, format);
	return false;
}

/*!
  Unmerge the cells in the \a range.
  Returns true on success.
*/
bool Document::unmergeCells(const CellRange &range)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->unmergeCells(range);
	return false;
}

/*!
  Sets width in characters of columns with the given \a range and \a width.
  Returns true on success.
 */
bool Document::setColumnWidth(const CellRange &range, double width)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnWidth(range, width);
	return false;
}

/*!
  Sets format property of columns with the gien \a range and \a format.
  Returns true on success.
 */
bool Document::setColumnFormat(const CellRange &range, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnFormat(range, format);
	return false;
}

/*!
  Sets hidden property of columns \a range to \a hidden. Columns are 1-indexed.
  Hidden columns are not visible.
  Returns true on success.
 */
bool Document::setColumnHidden(const CellRange &range, bool hidden)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnWidth(range, hidden);
	return false;
}

/*!
  Sets width in characters \a column to \a width. Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnWidth(int column, double width)
{
	return setColumnWidth(column,column,width);
}

/*!
  Sets format property \a column to \a format. Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnFormat(int column, const Format &format)
{
	return setColumnFormat(column,column,format);
}

/*!
  Sets hidden property of a \a column. Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnHidden(int column, bool hidden)
{
	return setColumnHidden(column,column,hidden);
}

/*!
  Sets width in characters for columns [\a colFirst, \a colLast]. Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnWidth(int colFirst, int colLast, double width)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnWidth(colFirst, colLast, width);
	return false;
}

/*!
  Sets format property of columns [\a colFirst, \a colLast] to \a format.
  Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnFormat(int colFirst, int colLast, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnFormat(colFirst, colLast, format);
	return false;
}


/*!
  Sets hidden property of columns [\a colFirst, \a colLast] to \a hidden.
  Columns are 1-indexed.
  Returns true on success.
 */
bool Document::setColumnHidden(int colFirst, int colLast, bool hidden)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->setColumnHidden(colFirst, colLast, hidden);
	return false;
}

/*!
  Returns width of the \a column in characters of the normal font.
  Columns are 1-indexed.
  Returns true on success.
 */
double Document::columnWidth(int column)
{
	if (Worksheet *sheet = currentWorksheet())
	  return sheet->columnWidth(column);
	return 0.0;
}

/*!
  Returns formatting of the \a column. Columns are 1-indexed.
 */
Format Document::columnFormat(int column)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->columnFormat(column);
	return Format();
}

/*!
  Returns true if \a column is hidden. Columns are 1-indexed.
 */
bool Document::isColumnHidden(int column)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->isColumnHidden(column);
	return false;
}

/*!
  Sets the \a format of the \a row.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Document::setRowFormat(int row, const Format &format)
{
	return setRowFormat(row,row, format);
}

/*!
  Sets the \a format of the rows including and between \a rowFirst and \a rowLast.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Document::setRowFormat(int rowFirst, int rowLast, const Format &format)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->setRowFormat(rowFirst, rowLast, format);
	return false;
}

/*!
  Sets the \a hidden property of the row \a row.
  Rows are 1-indexed. If hidden is true rows will not be visible.

  Returns true if success.
*/
bool Document::setRowHidden(int row, bool hidden)
{
	return setRowHidden(row,row,hidden);
}

/*!
  Sets the \a hidden property of the rows including and between \a rowFirst and \a rowLast.
  Rows are 1-indexed. If hidden is true rows will not be visible.

  Returns true if success.
*/
bool Document::setRowHidden(int rowFirst, int rowLast, bool hidden)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->setRowHidden(rowFirst, rowLast, hidden);
	return false;
}

/*!
  Sets the \a height of the row \a row.
  Row height measured in point size.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Document::setRowHeight(int row, double height)
{
	return setRowHeight(row,row,height);
}

/*!
  Sets the \a height of the rows including and between \a rowFirst and \a rowLast.
  Row height measured in point size.
  Rows are 1-indexed.

  Returns true if success.
*/
bool Document::setRowHeight(int rowFirst, int rowLast, double height)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->setRowHeight(rowFirst, rowLast, height);
	return false;
}

/*!
 Returns height of \a row in points.
*/
double Document::rowHeight(int row)
{
   if (Worksheet *sheet = currentWorksheet())
	  return sheet->rowHeight(row);
	return 0.0;
}

/*!
 Returns format of \a row.
*/
Format Document::rowFormat(int row)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->rowFormat(row);
	 return Format();
}

/*!
 Returns true if \a row is hidden.
*/
bool Document::isRowHidden(int row)
{
	if (Worksheet *sheet = currentWorksheet())
	   return sheet->isRowHidden(row);
	 return false;
}

/*!
   Groups rows from \a rowFirst to \a rowLast with the given \a collapsed.
   Returns false if error occurs.
 */
bool Document::groupRows(int rowFirst, int rowLast, bool collapsed)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->groupRows(rowFirst, rowLast, collapsed);
	return false;
}

/*!
   Groups columns from \a colFirst to \a colLast with the given \a collapsed.
   Returns false if error occurs.
 */
bool Document::groupColumns(int colFirst, int colLast, bool collapsed)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->groupColumns(colFirst, colLast, collapsed);
	return false;
}

/*!
 *  Add a data \a validation rule for current worksheet. Returns true if successful.
 */
bool Document::addDataValidation(const DataValidation &validation)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->addDataValidation(validation);
	return false;
}

/*!
 *  Add a  conditional formatting \a cf for current worksheet. Returns true if successful.
 */
bool Document::addConditionalFormatting(const ConditionalFormatting &cf)
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->addConditionalFormatting(cf);
	return false;
}

/*!
 * \overload
 * Returns the cell at the position \a pos. If there is no cell at
 * the specified position, the function returns 0.
 *
 * \sa read()
 */
Cell *Document::cellAt(const CellReference &pos) const
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->cellAt(pos);
	return 0;
}

/*!
 * Returns the cell at the given \a row and \a col. If there
 * is no cell at the specified position, the function returns 0.
 *
 * \sa read()
 */
Cell *Document::cellAt(int row, int col) const
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->cellAt(row, col);
	return 0;
}

/*!
 * \brief Create a defined name in the workbook with the given \a name, \a formula, \a comment
 *  and \a scope.
 *
 * \param name The defined name.
 * \param formula The cell or range that the defined name refers to.
 * \param scope The name of one worksheet, or empty which means golbal scope.
 * \return Return false if the name invalid.
 */
bool Document::defineName(const QString &name, const QString &formula, const QString &comment, const QString &scope)
{
	Q_D(Document);

	return d->workbook->defineName(name, formula, comment, scope);
}

/*!
	Return the range that contains cell data.
 */
CellRange Document::dimension() const
{
	if (Worksheet *sheet = currentWorksheet())
		return sheet->dimension();
	return CellRange();
}

/*!
 * Returns the value of the document's \a key property.
 */
QString Document::documentProperty(const QString &key) const
{
	Q_D(const Document);
    auto it = d->documentProperties.constFind(key);
    if (it != d->documentProperties.constEnd())
        return it.value();
	else
		return QString();
}

/*!
	Set the document properties such as Title, Author etc.

	The method can be used to set the document properties of the Excel
	file created by Qt Xlsx. These properties are visible when you use the
	Office Button -> Prepare -> Properties option in Excel and are also
	available to external applications that read or index windows files.

	The \a property \a key that can be set are:

	\list
	\li title
	\li subject
	\li creator
	\li manager
	\li company
	\li category
	\li keywords
	\li description
	\li status
	\endlist
*/
void Document::setDocumentProperty(const QString &key, const QString &property)
{
	Q_D(Document);
	d->documentProperties[key] = property;
}

/*!
 * Returns the names of all properties that were addedusing setDocumentProperty().
 */
QStringList Document::documentPropertyNames() const
{
	Q_D(const Document);
	return d->documentProperties.keys();
}

/*!
 * Return the internal Workbook object.
 */
Workbook *Document::workbook() const
{
	Q_D(const Document);
	return d->workbook.data();
}

/*!
 * Returns the sheet object named \a sheetName.
 */
AbstractSheet *Document::sheet(const QString &sheetName) const
{
	Q_D(const Document);
	return d->workbook->sheet(sheetNames().indexOf(sheetName));
}

/*!
 * Creates and append an sheet with the given \a name and \a type.
 * Return true if success.
 */
bool Document::addSheet(const QString &name, AbstractSheet::SheetType type)
{
	Q_D(Document);
	return d->workbook->addSheet(name, type);
}

/*!
 * Creates and inserts an document with the given \a name and \a type at the \a index.
 * Returns false if the \a name already used.
 */
bool Document::insertSheet(int index, const QString &name, AbstractSheet::SheetType type)
{
	Q_D(Document);
	return d->workbook->insertSheet(index, name, type);
}

/*!
   Rename the worksheet from \a oldName to \a newName.
   Returns true if the success.
 */
bool Document::renameSheet(const QString &oldName, const QString &newName)
{
	Q_D(Document);
	if (oldName == newName)
		return false;
	return d->workbook->renameSheet(sheetNames().indexOf(oldName), newName);
}

/*!
   Make a copy of the worksheet \a srcName with the new name \a distName.
   Returns true if the success.
 */
bool Document::copySheet(const QString &srcName, const QString &distName)
{
	Q_D(Document);
	if (srcName == distName)
		return false;
	return d->workbook->copySheet(sheetNames().indexOf(srcName), distName);
}

/*!
   Move the worksheet \a srcName to the new pos \a distIndex.
   Returns true if the success.
 */
bool Document::moveSheet(const QString &srcName, int distIndex)
{
	Q_D(Document);
	return d->workbook->moveSheet(sheetNames().indexOf(srcName), distIndex);
}

/*!
   Delete the worksheet \a name.
   Returns true if current sheet was deleted successfully.
 */
bool Document::deleteSheet(const QString &name)
{
	Q_D(Document);
	return d->workbook->deleteSheet(sheetNames().indexOf(name));
}

/*!
 * \brief Return pointer of current sheet.
 */
AbstractSheet *Document::currentSheet() const
{
	Q_D(const Document);

	return d->workbook->activeSheet();
}

/*!
 * \brief Return pointer of current worksheet.
 * If the type of sheet is not AbstractSheet::ST_WorkSheet, then 0 will be returned.
 */
Worksheet *Document::currentWorksheet() const
{
	AbstractSheet *st = currentSheet();
	if (st && st->sheetType() == AbstractSheet::ST_WorkSheet)
		return static_cast<Worksheet *>(st);
	else
		return 0;
}

/*!
 * \brief Set worksheet named \a name to be active sheet.
 * Returns true if success.
 */
bool Document::selectSheet(const QString &name)
{
	Q_D(Document);
	return d->workbook->setActiveSheet(sheetNames().indexOf(name));
}

/*!
 * Returns the names of worksheets contained in current document.
 */
QStringList Document::sheetNames() const
{
	Q_D(const Document);
	return d->workbook->worksheetNames();
}

/*!
 * Save current document to the filesystem. If no name specified when
 * the document constructed, a default name "book1.xlsx" will be used.
 * Returns true if saved successfully.
 */
bool Document::save() const
{
	Q_D(const Document);
	QString name = d->packageName.isEmpty() ? d->defaultPackageName : d->packageName;

	return saveAs(name);
}

/*!
 * Saves the document to the file with the given \a name.
 * Returns true if saved successfully.
 */
bool Document::saveAs(const QString &name) const
{
	QFile file(name);
	if (file.open(QIODevice::WriteOnly))
		return saveAs(&file);
	return false;
}

/*!
 * \overload
 * This function writes a document to the given \a device.
 *
 * \warning The \a device will be closed when this function returned.
 */
bool Document::saveAs(QIODevice *device) const
{
	Q_D(const Document);
	return d->savePackage(device);
}

bool Document::isLoadPackage() const
{
	Q_D(const Document);
	return d->isLoad; 
}

bool Document::load() const
{
	return isLoadPackage();
}

bool Document::copyStyle(const QString &from, const QString &to) {
	return DocumentPrivate::copyStyle(from, to);
}

/*!
 * Destroys the document and cleans up.
 */
Document::~Document()
{
	delete d_ptr;
}

//  add by liufeijin 20181025 {{
bool Document::changeimage(int filenoinmidea, QString newfile)
{
	Q_D(const Document);
	QImage newpic;
	
	newpic=QImage(newfile);
	
	QList<QSharedPointer<MediaFile> > mediaFileToLoad = d->workbook->mediaFiles();
	QSharedPointer<MediaFile> mf = mediaFileToLoad[filenoinmidea];
	
	const QString suffix = newfile.mid(newfile.lastIndexOf(QLatin1Char('.'))+1);
	QString mimetypemy;
    if(QString::compare(QLatin1String("jpg"), suffix, Qt::CaseInsensitive)==0)
       mimetypemy=QStringLiteral("image/jpeg");
    if(QString::compare(QLatin1String("bmp"), suffix, Qt::CaseInsensitive)==0)
       mimetypemy=QStringLiteral("image/bmp");
    if(QString::compare(QLatin1String("gif"), suffix, Qt::CaseInsensitive)==0)
       mimetypemy=QStringLiteral("image/gif");
    if(QString::compare(QLatin1String("png"), suffix, Qt::CaseInsensitive)==0)
       mimetypemy=QStringLiteral("image/png");
	
	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.setBuffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	newpic.save(&buffer,suffix.toLocal8Bit().data());
	
	mf->set(ba,suffix,mimetypemy);
	mediaFileToLoad[filenoinmidea]=mf;
	
	return true;
}
// liufeijin }}


/*!
  Returns map of columns with there maximal width
 */
QMap<int, int> Document::getMaximalColumnWidth(int firstRow, int lastRow)
{
    const int defaultPixelSize = 11;    //Default font pixel size of excel?
    int maxRows = -1;
    int maxCols = -1;
    QVector<CellLocation> cellLocation = currentWorksheet()->getFullCells(&maxRows, &maxCols);

    QMap<int, int> colWidth;

    for(int i=0; i < cellLocation.size(); i++)
    {
        int col = cellLocation.at(i).col;
        int row = cellLocation.at(i).row;
        int fs = cellLocation.at(i).cell->format().fontSize();
        if( fs <= 0)
        {
            fs = defaultPixelSize;
        }

//        QString str = cellLocation.at(i).cell.data()->value().toString();
        QString str = read(row, col).toString();

        double w = str.length() * double(fs) / defaultPixelSize + 1; // width not perfect, but works reasonably well

        if( (row >= firstRow) && (row <= lastRow))
        {
            if( w > colWidth.value(col))
            {
                colWidth.insert(col, int(w));
            }
        }
    }

    return colWidth;
}


/*!
  Auto ets width in characters of columns with the given \a range.
  Returns true on success.
 */
bool Document::autosizeColumnWidth(const CellRange &range)
{
    bool erg = false;

    if( !range.isValid())
    {
        return false;
    }

    const QMap<int, int> colWidth = getMaximalColumnWidth(range.firstRow(), range.lastRow());
    auto it = colWidth.constBegin();
    while (it != colWidth.constEnd()) {
        if( (it.key() >= range.firstColumn()) && (it.key() <= range.lastColumn()) )
        {
            erg |= setColumnWidth(it.key(), it.value());
        }
        ++it;
    }

    return erg;
}


/*!
  Auto sets width in characters \a column . Columns are 1-indexed.
  Returns true on success.
 */
bool Document::autosizeColumnWidth(int column)
{
    bool erg = false;

    const QMap<int, int> colWidth = getMaximalColumnWidth();
    auto it = colWidth.constBegin();
    while (it != colWidth.constEnd()) {
        if( it.key() == column)
        {
            erg |= setColumnWidth(it.key(), it.value());
        }
        ++it;
    }

    return erg;
}


/*!
  Auto sets width in characters for columns [\a colFirst, \a colLast]. Columns are 1-indexed.
  Returns true on success.
 */
bool Document::autosizeColumnWidth(int colFirst, int colLast)
{
    Q_UNUSED(colFirst)
    Q_UNUSED(colLast)
    bool erg = false;

    const QMap<int, int> colWidth = getMaximalColumnWidth();
    auto it = colWidth.constBegin();
    while (it != colWidth.constEnd()) {
        if( (it.key() >= colFirst) && (it.key() <= colLast) )
        {
            erg |= setColumnWidth(it.key(), it.value());
        }
        ++it;
    }

    return erg;
}


/*!
  Auto sets width in characters for all columns.
  Returns true on success.
 */
bool Document::autosizeColumnWidth(void)
{
    bool erg = false;

    const QMap<int, int> colWidth = getMaximalColumnWidth();
    auto it = colWidth.constBegin();
    while (it != colWidth.constEnd()) {
        erg |= setColumnWidth(it.key(), it.value());
        ++it;
    }

    return erg;
}


QT_END_NAMESPACE_XLSX
