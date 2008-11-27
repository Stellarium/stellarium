#include "TextEntryDateTimeValidator.hpp"
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QDebug>
#include <vector>

using namespace std;

//! represents a valid, complete date string.
static QString finalRe("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[10])[T ]([01][0-9]|2[0123]):([012345][0-9]):([012345][0-9])");

TextEntryDateTimeValidator::TextEntryDateTimeValidator(QObject *parent) :
		QValidator(parent),
		final(finalRe),
		badEpoch("0*1582-10-([56789]|0[56789]|1[01234])[T ]([01][0-9]|2[0123]):([012345][0-9]):([01][0-9]|2[0123])"),
		editingYear("(-?[0-9]*)-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[10])[T ]([01][0-9]|2[0123]):([012345][0-9]):([012345][0-9])"),
		editingMonth("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-([0-9]{0,2})-(0[1-9]|[12][0-9]|3[10])[T ]([01][0-9]|2[0123]):([012345][0-9]):([012345][0-9])"),
		editingDay("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-(0[1-9]|1[0-2])-([0-9]{0,2})[T ]([01][0-9]|2[0123]):([012345][0-9]):([012345][0-9])"),
		editingHour("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[10])[T ]([0-9]{0,2}):([012345][0-9]):([012345][0-9])"),
		editingMinute("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[10])[T ]([01][0-9]|2[0123]):([0-9]{0,2}):([012345][0-9])"),
		editingSecond("(-0*[1-9][0-9]{0,5}|0+|0*[1-9][0-9]{0,5})-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[10])[T ]([01][0-9]|2[0123]):([012345][0-9]):([0-9]{0,2})")
{}

QValidator::State TextEntryDateTimeValidator::validate(QString & input, int & pos) const
{
	if (final.exactMatch(input))
	{
		return Acceptable;
	}

	// gap between Julian and Gregorian calendars.
	if (badEpoch.exactMatch(input))
	{
		return Invalid;
	}

	// user still editing probably
	if (editingYear.exactMatch(input) ||
	        editingMonth.exactMatch(input) ||
	        editingDay.exactMatch(input) ||
	        editingHour.exactMatch(input) ||
	        editingMinute.exactMatch(input) ||
	        editingSecond.exactMatch(input))
	{
		return Intermediate;
	}

	// all other cases
	return Invalid;
}

vector<int> TextEntryDateTimeValidator::getIntsFromISO8601String(const QString & dt)
{
	QRegExp final(finalRe);
	vector<int> retval;
	if (final.exactMatch(dt))
	{
		QStringList number_strings = final.capturedTexts();
		bool ok;
		int v;
		for (int i = 1; i < number_strings.size(); i++)
		{
			qWarning() << ":: at capture " << i << " got a " << number_strings[i];
			ok = true;
			v = number_strings[i].toInt(&ok, 10);
			qWarning() << "  :: and it was a " << v << " " << ok;
			if (ok)
			{
				retval.push_back(v);
			}
			else
			{
				retval.clear();
				qWarning() << "TextEntryDateTimeValidator::getIntsFromISO8601String: input string failed to be an exact date at capture " << i << ", returning nothing: " << dt;
				break;
			}
		}
	}
	else
	{
		qWarning() << "TextEntryDateTimeValidator::getIntsFromISO8601String: input string failed to be an exact date, returning nothing: " << dt;
	}
	return retval;
}

