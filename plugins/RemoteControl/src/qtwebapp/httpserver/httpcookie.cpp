/**
  @file
  @author Stefan Frings
*/

#include "httpcookie.h"

HttpCookie::HttpCookie()
{
    version=1;
    maxAge=0;
    secure=false;
}

HttpCookie::HttpCookie(const QByteArray name, const QByteArray value, const int maxAge, const QByteArray path, const QByteArray comment, const QByteArray domain, const bool secure)
{
    this->name=name;
    this->value=value;
    this->maxAge=maxAge;
    this->path=path;
    this->comment=comment;
    this->domain=domain;
    this->secure=secure;
    this->version=1;
}

HttpCookie::HttpCookie(const QByteArray source)
{
    version=1;
    maxAge=0;
    secure=false;
    QList<QByteArray> list=splitCSV(source);
    for (auto part : list)
    {
        // Split the part into name and value
        QByteArray name;
        QByteArray value;
        int posi=part.indexOf('=');
        if (posi)
        {
            name=part.left(posi).trimmed();
            value=part.mid(posi+1).trimmed();
        }
        else
        {
            name=part.trimmed();
            value="";
        }

        // Set fields
        if (name=="Comment")
        {
            comment=value;
        }
        else if (name=="Domain")
        {
            domain=value;
        }
        else if (name=="Max-Age")
        {
            maxAge=value.toInt();
        }
        else if (name=="Path")
        {
            path=value;
        }
        else if (name=="Secure")
        {
            secure=true;
        }
        else if (name=="Version")
        {
            version=value.toInt();
        }
        else {
            if (this->name.isEmpty())
            {
                this->name=name;
                this->value=value;
            }
            else
            {
                qWarning("HttpCookie: Ignoring unknown %s=%s",name.data(),value.data());
            }
        }
    }
}

QByteArray HttpCookie::toByteArray() const
{
    QByteArray buffer(name);
    buffer.append('=');
    buffer.append(value);
    if (!comment.isEmpty())
    {
        buffer.append("; Comment=");
        buffer.append(comment);
    }
    if (!domain.isEmpty())
    {
        buffer.append("; Domain=");
        buffer.append(domain);
    }
    if (maxAge!=0)
    {
        buffer.append("; Max-Age=");
        buffer.append(QByteArray::number(maxAge));
    }
    if (!path.isEmpty())
    {
        buffer.append("; Path=");
        buffer.append(path);
    }
    if (secure) {
        buffer.append("; Secure");
    }
    buffer.append("; Version=");
    buffer.append(QByteArray::number(version));
    return buffer;
}

void HttpCookie::setName(const QByteArray name)
{
    this->name=name;
}

void HttpCookie::setValue(const QByteArray value)
{
    this->value=value;
}

void HttpCookie::setComment(const QByteArray comment)
{
    this->comment=comment;
}

void HttpCookie::setDomain(const QByteArray domain)
{
    this->domain=domain;
}

void HttpCookie::setMaxAge(const int maxAge)
{
    this->maxAge=maxAge;
}

void HttpCookie::setPath(const QByteArray path)
{
    this->path=path;
}

void HttpCookie::setSecure(const bool secure)
{
    this->secure=secure;
}

QByteArray HttpCookie::getName() const
{
    return name;
}

QByteArray HttpCookie::getValue() const
{
    return value;
}

QByteArray HttpCookie::getComment() const
{
    return comment;
}

QByteArray HttpCookie::getDomain() const
{
    return domain;
}

int HttpCookie::getMaxAge() const
{
    return maxAge;
}

QByteArray HttpCookie::getPath() const
{
    return path;
}

bool HttpCookie::getSecure() const
{
    return secure;
}

int HttpCookie::getVersion() const
{
    return version;
}

QList<QByteArray> HttpCookie::splitCSV(const QByteArray source)
{
    bool inString=false;
    QList<QByteArray> list;
    QByteArray buffer;
    for (int i=0; i<source.size(); ++i)
    {
        char c=source.at(i);
        if (inString==false)
        {
            if (c=='\"')
            {
                inString=true;
            }
            else if (c==';')
            {
                QByteArray trimmed=buffer.trimmed();
                if (!trimmed.isEmpty())
                {
                    list.append(trimmed);
                }
                buffer.clear();
            }
            else
            {
                buffer.append(c);
            }
        }
        else
        {
            if (c=='\"')
            {
                inString=false;
            }
            else {
                buffer.append(c);
            }
        }
    }
    QByteArray trimmed=buffer.trimmed();
    if (!trimmed.isEmpty())
    {
        list.append(trimmed);
    }
    return list;
}
