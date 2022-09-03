/**
  @file
  @author Stefan Frings
*/

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include <QString>
#include <QIODevice>
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
#include <QStringConverter>
#else
#include <QTextCodec>
#endif
#include <QFile>
#include <QString>
#include "templateglobal.h"

/**
 * @ingroup qtWebApp
 * Interface for Template::translate calls.
*/
class ITemplateTranslationProvider
{
public:
	/** Implement this method to provide a translated string for a given key */
	virtual QString getTranslation(const QString& key) = 0;
	virtual ~ITemplateTranslationProvider(){}
};


/**
 Enhanced version of QString for template processing. Templates
 are usually loaded from files, but may also be loaded from
 prepared Strings.
 Example template file:
 <p><code><pre>
 Hello {username}, how are you?

 {if locked}
     Your account is locked.
 {else locked}
     Welcome on our system.
 {end locked}

 The following users are on-line:
     Username       Time
 {loop user}
     {user.name}    {user.time}
 {end user}
 </pre></code></p>
 <p>
 Example code to fill this template:
 <p><code><pre>
 Template t(QFile("test.tpl"),QTextCode::codecForName("UTF-8"));
 t.setVariable("username", "Stefan");
 t.setCondition("locked",false);
 t.loop("user",2);
 t.setVariable("user0.name,"Markus");
 t.setVariable("user0.time,"8:30");
 t.setVariable("user1.name,"Roland");
 t.setVariable("user1.time,"8:45");
 </pre></code></p>
 <p>
 The code example above shows how variable within loops are numbered.
 Counting starts with 0. Loops can be nested, for example:
 <p><code><pre>
 &lt;table&gt;
 {loop row}
     &lt;tr&gt;
     {loop row.column}
         &lt;td&gt;{row.column.value}&lt;/td&gt;
     {end row.column}
     &lt;/tr&gt;
 {end row}
 &lt;/table&gt;
 </pre></code></p>
 <p>
 Example code to fill this nested loop with 3 rows and 4 columns:
 <p><code><pre>
 t.loop("row",3);

 t.loop("row0.column",4);
 t.setVariable("row0.column0.value","a");
 t.setVariable("row0.column1.value","b");
 t.setVariable("row0.column2.value","c");
 t.setVariable("row0.column3.value","d");

 t.loop("row1.column",4);
 t.setVariable("row1.column0.value","e");
 t.setVariable("row1.column1.value","f");
 t.setVariable("row1.column2.value","g");
 t.setVariable("row1.column3.value","h");

 t.loop("row2.column",4);
 t.setVariable("row2.column0.value","i");
 t.setVariable("row2.column1.value","j");
 t.setVariable("row2.column2.value","k");
 t.setVariable("row2.column3.value","l");
 </pre></code></p>
 @see TemplateLoader
 @see TemplateCache
*/

class DECLSPEC Template : public QString {
public:
    /**
      Constructor that reads the template from a string.
      @param source The template source text
      @param sourceName Name of the source file, used for logging
    */
    Template(QString source, QString sourceName);

    /**
      Constructor that reads the template from a file. Note that this class does not
      cache template files by itself, so using this constructor is only recommended
      to be used on local filesystem.
      @param file File that provides the source text
      @param textCodec Encoding of the source. If null, UTF-8 is assumed.
      @param encoding Encoding of the source. (Qt6 version)
      @see TemplateLoader
      @see TemplateCache
      @note Qt6 upgrade by Georg Zotti for Stellarium
    */

#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
    Template(QFile& file, QStringConverter::Encoding encoding=QStringConverter::Utf8);
#else
    Template(QFile& file, QTextCodec* textCodec = Q_NULLPTR);
#endif

    virtual ~Template() { }

    /**
      Replace a variable by the given value.
      Affects tags with the syntax

      - {name}

      After settings the
      value of a variable, the variable does not exist anymore,
      it it cannot be changed multiple times.
      @param name name of the variable
      @param value new value
      @return The count of variables that have been processed
    */
    int setVariable(QString name, QString value);

    /**
      Set a condition. This affects tags with the syntax

      - {if name}...{end name}
      - {if name}...{else name}...{end name}
      - {ifnot name}...{end name}
      - {ifnot name}...{else name}...{end name}

     @param name Name of the condition
     @param value Value of the condition
     @return The count of conditions that have been processed
    */
    int setCondition(QString name, bool value);

    /**
     Set number of repetitions of a loop.
     This affects tags with the syntax

     - {loop name}...{end name}
     - {loop name}...{else name}...{end name}

     @param name Name of the loop
     @param repetitions The number of repetitions
     @return The number of loops that have been processed
    */
    int loop(QString name, int repetitions);

    /**
     Enable warnings for missing tags
     @param enable Warnings are enabled, if true
    */
    void enableWarnings(bool enable=true);

    /**
      Translates all translation tags using the given translation provider.
      The translation tags have a PHP-like format to be able to be parsed using gettext/xgettext.
      Examples: <?= tr("this is an example") ?>, <?= tr("You have %1 new messages", "20") ?>
    */
    void translate(ITemplateTranslationProvider& provider);

private:
    /** Name of the source file */
    QString sourceName;

    /** Enables warnings, if true */
    bool warnings;
};

#endif // TEMPLATE_H
