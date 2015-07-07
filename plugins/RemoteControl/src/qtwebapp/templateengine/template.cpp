/**
  @file
  @author Stefan Frings
*/

#include "template.h"
#include <QFileInfo>
#include <QRegularExpression>

Template::Template(QString source, QString sourceName)
    : QString(source) {
    this->sourceName=sourceName;
    this->warnings=false;
}

Template::Template(QFile& file, QTextCodec* textCodec) {
    this->warnings=false;
    sourceName=QFileInfo(file.fileName()).baseName();
    if (!file.isOpen()) {
        file.open(QFile::ReadOnly | QFile::Text);
    }
    QByteArray data=file.readAll();
    file.close();
    if (data.size()==0 || file.error()) {
	qCritical("Template: cannot read from %s, %s",qPrintable(sourceName),qPrintable(file.errorString()));
    }
    else
    {
	    if(textCodec)
		    append(textCodec->toUnicode(data));
	    else
		    append(fromUtf8(data));
    }
}


int Template::setVariable(QString name, QString value) {
    int count=0;
    QString variable="{"+name+"}";
    int start=indexOf(variable);
    while (start>=0) {
        replace(start, variable.length(), value);
        count++;
        start=indexOf(variable,start+value.length());
    }
    if (count==0 && warnings) {
        qWarning("Template: missing variable %s in %s",qPrintable(variable),qPrintable(sourceName));
    }
    return count;
}

int Template::setCondition(QString name, bool value) {
    int count=0;
    QString startTag=QString("{if %1}").arg(name);
    QString elseTag=QString("{else %1}").arg(name);
    QString endTag=QString("{end %1}").arg(name);
    // search for if-else-end
    int start=indexOf(startTag);
    while (start>=0) {
        int end=indexOf(endTag,start+startTag.length());
        if (end>=0) {
            count++;
            int ellse=indexOf(elseTag,start+startTag.length());
            if (ellse>start && ellse<end) { // there is an else part
                if (value==true) {
                    QString truePart=mid(start+startTag.length(), ellse-start-startTag.length());
                    replace(start, end-start+endTag.length(), truePart);
                }
                else { // value==false
                    QString falsePart=mid(ellse+elseTag.length(), end-ellse-elseTag.length());
                    replace(start, end-start+endTag.length(), falsePart);
                }
            }
            else if (value==true) { // and no else part
                QString truePart=mid(start+startTag.length(), end-start-startTag.length());
                replace(start, end-start+endTag.length(), truePart);
            }
            else { // value==false and no else part
                replace(start, end-start+endTag.length(), "");
            }
            start=indexOf(startTag,start);
        }
        else {
            qWarning("Template: missing condition end %s in %s",qPrintable(endTag),qPrintable(sourceName));
        }
    }
    // search for ifnot-else-end
    QString startTag2="{ifnot "+name+"}";
    start=indexOf(startTag2);
    while (start>=0) {
        int end=indexOf(endTag,start+startTag2.length());
        if (end>=0) {
            count++;
            int ellse=indexOf(elseTag,start+startTag2.length());
            if (ellse>start && ellse<end) { // there is an else part
                if (value==false) {
                    QString falsePart=mid(start+startTag2.length(), ellse-start-startTag2.length());
                    replace(start, end-start+endTag.length(), falsePart);
                }
                else { // value==true
                    QString truePart=mid(ellse+elseTag.length(), end-ellse-elseTag.length());
                    replace(start, end-start+endTag.length(), truePart);
                }
            }
            else if (value==false) { // and no else part
                QString falsePart=mid(start+startTag2.length(), end-start-startTag2.length());
                replace(start, end-start+endTag.length(), falsePart);
            }
            else { // value==true and no else part
                replace(start, end-start+endTag.length(), "");
            }
            start=indexOf(startTag2,start);
        }
        else {
            qWarning("Template: missing condition end %s in %s",qPrintable(endTag),qPrintable(sourceName));
        }
    }
    if (count==0 && warnings) {
        qWarning("Template: missing condition %s or %s in %s",qPrintable(startTag),qPrintable(startTag2),qPrintable(sourceName));
    }
    return count;
}

int Template::loop(QString name, int repetitions) {
    Q_ASSERT(repetitions>=0);
    int count=0;
    QString startTag="{loop "+name+"}";
    QString elseTag="{else "+name+"}";
    QString endTag="{end "+name+"}";
    // search for loop-else-end
    int start=indexOf(startTag);
    while (start>=0) {
        int end=indexOf(endTag,start+startTag.length());
        if (end>=0) {
            count++;
            int ellse=indexOf(elseTag,start+startTag.length());
            if (ellse>start && ellse<end) { // there is an else part
                if (repetitions>0) {
                    QString loopPart=mid(start+startTag.length(), ellse-start-startTag.length());
                    QString insertMe;
                    for (int i=0; i<repetitions; ++i) {
                        // number variables, conditions and sub-loop within the loop
                        QString nameNum=name+QString::number(i);
                        QString s=loopPart;
                        s.replace(QString("{%1.").arg(name), QString("{%1.").arg(nameNum));
                        s.replace(QString("{if %1.").arg(name), QString("{if %1.").arg(nameNum));
                        s.replace(QString("{ifnot %1.").arg(name), QString("{ifnot %1.").arg(nameNum));
                        s.replace(QString("{else %1.").arg(name), QString("{else %1.").arg(nameNum));
                        s.replace(QString("{end %1.").arg(name), QString("{end %1.").arg(nameNum));
                        s.replace(QString("{loop %1.").arg(name), QString("{loop %1.").arg(nameNum));
                        insertMe.append(s);
                    }
                    replace(start, end-start+endTag.length(), insertMe);
                }
                else { // repetitions==0
                    QString elsePart=mid(ellse+elseTag.length(), end-ellse-elseTag.length());
                    replace(start, end-start+endTag.length(), elsePart);
                }
            }
            else if (repetitions>0) { // and no else part
                QString loopPart=mid(start+startTag.length(), end-start-startTag.length());
                QString insertMe;
                for (int i=0; i<repetitions; ++i) {
                    // number variables, conditions and sub-loop within the loop
                    QString nameNum=name+QString::number(i);
                    QString s=loopPart;
                    s.replace(QString("{%1.").arg(name), QString("{%1.").arg(nameNum));
                    s.replace(QString("{if %1.").arg(name), QString("{if %1.").arg(nameNum));
                    s.replace(QString("{ifnot %1.").arg(name), QString("{ifnot %1.").arg(nameNum));
                    s.replace(QString("{else %1.").arg(name), QString("{else %1.").arg(nameNum));
                    s.replace(QString("{end %1.").arg(name), QString("{end %1.").arg(nameNum));
                    s.replace(QString("{loop %1.").arg(name), QString("{loop %1.").arg(nameNum));
                    insertMe.append(s);
                }
                replace(start, end-start+endTag.length(), insertMe);
            }
            else { // repetitions==0 and no else part
                replace(start, end-start+endTag.length(), "");
            }
            start=indexOf(startTag,start);
        }
        else {
            qWarning("Template: missing loop end %s in %s",qPrintable(endTag),qPrintable(sourceName));
        }
    }
    if (count==0 && warnings) {
        qWarning("Template: missing loop %s in %s",qPrintable(startTag),qPrintable(sourceName));
    }
    return count;
}

void Template::enableWarnings(bool enable) {
    warnings=enable;
}

void Template::translate(ITemplateTranslationProvider &provider)
{
	const QRegularExpression regexp = QRegularExpression("{trans \"([^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"}");

	int offset = 0;
	QRegularExpressionMatch match;
	do
	{
		match = regexp.match(*this,offset);

		if(match.hasMatch())
		{
			int start = match.capturedStart(0);
			int len = match.capturedLength(0);
			QString key = match.captured(1);

			this->replace(start,len,provider.getTranslation(key));
			offset = start+len;
		}
	}while(match.hasMatch());
}

