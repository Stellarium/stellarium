/*
 * Stellarium
 * Copyright (C) 2016 Florian Schaukowitsch
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

//! @file
//! This is a private header for some StelDialog implementation classes, which require MOC'cing
//! Other classes should not include this header.

#ifndef STELDIALOG_P_HPP
#define STELDIALOG_P_HPP

#include "StelPropertyMgr.hpp"
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QGroupBox>

//! A StelPropertyProxy that works with QAbstractButton widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QAbstractButtonStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QAbstractButtonStelPropertyConnectionHelper(StelProperty* prop,QAbstractButton* button);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QAbstractButton* button;
};

//! A StelPropertyProxy that works with QGroupBox widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QGroupBoxStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QGroupBoxStelPropertyConnectionHelper(StelProperty* prop,QGroupBox* box);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QGroupBox* box;
};

//! A StelPropertyProxy that works with QComboBox widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QComboBoxStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QComboBoxStelPropertyConnectionHelper(StelProperty* prop,QComboBox* combo);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QComboBox* combo;
};

//! A StelPropertyProxy that works with QComboBox widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QComboBoxStelStringPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QComboBoxStelStringPropertyConnectionHelper(StelProperty* prop,QComboBox* combo);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QComboBox* combo;
};

//! A StelPropertyProxy that works with QLineEdit widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QLineEditStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QLineEditStelPropertyConnectionHelper(StelProperty* prop,QLineEdit* edit);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QLineEdit* edit;
};

//! A StelPropertyProxy that works with QSpinBox widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QSpinBoxStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QSpinBoxStelPropertyConnectionHelper(StelProperty* prop,QSpinBox* spin);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QSpinBox* spin;
};


//! A StelPropertyProxy that works with QDoubleSpinBox widgets.
//! When the property changes, the widget's value is updated, while preventing widget signals to be sent.
//! This avoids emitting the valueChanged() signal, which would unnecessarily set the property value again, which may lead to problems.
class QDoubleSpinBoxStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QDoubleSpinBoxStelPropertyConnectionHelper(StelProperty* prop,QDoubleSpinBox* spin);

protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private:
	QDoubleSpinBox* spin;
};

class QSliderStelPropertyConnectionHelper : public StelPropertyProxy
{
	Q_OBJECT
public:
	QSliderStelPropertyConnectionHelper(StelProperty* prop, double minValue, double maxValue, QSlider* slider);
	QSliderStelPropertyConnectionHelper(StelProperty* prop, int minValue, int maxValue, QSlider* slider);
protected slots:
	virtual void onPropertyChanged(const QVariant& value) Q_DECL_OVERRIDE;
private slots:
	void sliderIntValueChanged(int val);
private:
	QSlider* slider;
	double minValue, maxValue,dRange;
};

#endif // STELDIALOG_P_HPP
