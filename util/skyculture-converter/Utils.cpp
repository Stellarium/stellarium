#include "Utils.hpp"
#include <QDebug>
#include <QStringList>

std::vector<int> parseReferences(const QString& inStr)
{
	if(inStr.isEmpty()) return {};
	const auto strings = inStr.split(",");
	std::vector<int> refs;
	for(const auto& string : strings)
	{
		bool ok = false;
		const auto ref = string.toInt(&ok);
		if(!ok) 
		{
			qWarning() << "Failed to parse reference number" << string << "in" << inStr;
			continue;
		}
		refs.push_back(ref);
	}
	return refs;
}

QString formatReferences(const std::vector<int>& refs)
{
	QString out;
	for(const int ref : refs)
		out += QString("%1,").arg(ref);
	if(!out.isEmpty()) out.chop(1); // remove trailing comma
	return out;
}

QString jsonEscape(const QString& string)
{
	QString out;
	for(const QChar c : string)
	{
		const int u = uint16_t(c.unicode());
		if(u == '\\')
			out += "\\\\";
		else if(u == '\n')
			out += "\\n";
		else if(u == '"')
			out += "\\\"";
		else if(u < 0x20)
			out += QString("\\u%1").arg(u, 4, 16, QLatin1Char('0'));
		else
			out += c;
	}
	return out;
}
