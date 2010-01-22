/*
 * Copyright (C) 2009 Timothy Reaves
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _LIMITINGDOUBLEVALIDATOR_HPP_
#define _LIMITINGDOUBLEVALIDATOR_HPP_

#include <QObject>
#include <QDoubleValidator>

class LimitingDoubleValidator : public QDoubleValidator {
	Q_OBJECT
	
public:
	LimitingDoubleValidator(double bottom, double top, int decimals, QObject *parent = 0);
	virtual ~LimitingDoubleValidator();
	virtual void fixup(QString & input ) const;
	
};

#endif // _LIMITINGDOUBLEVALIDATOR_HPP_
