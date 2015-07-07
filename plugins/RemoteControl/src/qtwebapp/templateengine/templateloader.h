/**
  @file
  @author Stefan Frings
*/

#ifndef TEMPLATELOADER_H
#define TEMPLATELOADER_H

#include <QString>
#include <QSettings>
#include <QTextCodec>
#include <QMutex>
#include "templateglobal.h"
#include "template.h"

/**
  Loads localized versions of template files. If the caller requests a file with the
  name "index" and the suffix is ".tpl" and the requested locale is "de_DE, de, en-US",
  then files are searched in the following order:

  - index-de_DE.tpl
  - index-de.tpl
  - index-en_US.tpl  
  - index-en.tpl
  - index.tpl

  The following settings are required:
  <code><pre>
  path=../templates
  suffix=.tpl
  encoding=UTF-8
  </pre></code>
  The path is relative to the directory of the config file. In case of windows, if the
  settings are in the registry, the path is relative to the current working directory.
  @see TemplateCache
*/

class DECLSPEC TemplateLoader : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(TemplateLoader)
public:

    /**
      Constructor.
      @param settings configurations settings
      @param parent parent object
    */
    TemplateLoader(QSettings* settings, QObject* parent=0);

    /** Destructor */
    virtual ~TemplateLoader();

    /**
      Get a template for a given locale.
      This method is thread safe.
      @param templateName base name of the template file, without suffix and without locale
      @param locales Requested locale(s), e.g. "de_DE, en_EN". Strings in the format of
      the HTTP header Accept-Locale may be used. Badly formatted parts in the string are silently
      ignored.
      @return If the template cannot be loaded, an error message is logged and an empty template is returned.
    */
    Template getTemplate(QString templateName, QString locales=QString());

protected:

    /**
      Try to get a file from cache or filesystem.
      @param localizedName Name of the template with locale to find
      @return The template document, or empty string if not found
    */
    virtual QString tryFile(QString localizedName);

    /** Directory where the templates are searched */
    QString templatePath;

    /** Suffix to the filenames */
    QString fileNameSuffix;

    /** Codec for decoding the files */
    QTextCodec* textCodec;

    /** Used to synchronize threads */
    QMutex mutex;
};

#endif // TEMPLATELOADER_H
