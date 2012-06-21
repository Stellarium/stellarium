/*
 * Stellarium
 * Copyright (C) 2012 Anton Samoylov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELSHORTCUT_HPP
#define STELSHORTCUT_HPP

#include <QObject>
#include <QMap>
#include <QAction>

class StelShortcut : public QObject
{
	Q_OBJECT
public:
	StelShortcut(const QString& aid, const QString& atext, const QString& akeys,
							 bool acheckable, bool aautoRepeat = true, bool aglobal = false, QGraphicsWidget *parent = NULL);

	~StelShortcut();

	QAction* getAction() const { return action; }

	void setText(const QString& atext);
	void setKeys(const QString& akeys);
	void setCheckable(bool c);
	void setAutoRepeat(bool ar);
	void setGlobal(bool g);

signals:
    
public slots:

private:
	QAction *action;
	QString id;
	QString text;
	QString keys;
	bool checkable;
	bool autoRepeat;
	bool global;

	QList<QKeySequence> splitShortcuts(const QString& shortcuts);
};



class StelShortcutGroup : public QObject
{
	Q_OBJECT
public:
	StelShortcutGroup(QString id);
	QAction* registerAction(const QString& actionId, const QString& text, const QString& keys, bool checkable,
												 bool autoRepeat = true, bool global = false, QGraphicsWidget *parent = false);

	QAction* getAction(const QString &actionId);

signals:

public slots:

private:
	QString id;
	QMap<QString, StelShortcut*> shortcuts;

	StelShortcut* getShortcut(const QString& id);
};

#endif // STELSHORTCUT_HPP
