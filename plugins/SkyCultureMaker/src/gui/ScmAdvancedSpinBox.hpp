/*
 * Sky Culture Maker plug-in for Stellarium
 *
 * Copyright (C) 2025 Moritz Rätz
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCMADVANCEDSPINBOX_HPP
#define SCMADVANCEDSPINBOX_HPP

#include <qspinbox.h>

//! @class ScmAdvancedSpinBox
//! Customized SpinBox that allows for 'better' input (not blocking input when its greater than limit), displays special characters for min / max / arbitrary values and set additional custom min / max.

class ScmAdvancedSpinBox : public QSpinBox
{
	Q_OBJECT
public:
	ScmAdvancedSpinBox(QWidget *parent = nullptr);

	void fixup(QString & input) const override;
	QValidator::State validate(QString &text, int &pos) const override;

	void setDisplayCustomStringForValue(bool display);

	/**
	 * @brief Add a custom string that will be displayed where the SpinBox would normally display a specific (int) value.
	 *
	 * @param value Value at which the string should be displayed.
	 * @param text String that should be displayed.
	 */
	void addCustomStringForValue(int value, QString text);

	/**
	 * @brief Remove a string from the given value.
	 *
	 * @param value Value at which the custom string should be removed.
	 */
	void removeCustomStringForValue(int value);

	/**
	 * @brief Convenience function to add a custom string to the current minimum.
	 *
	 * @param test String that should be displayed at the current min.
	 */
	void setCustomStringForMin(QString text);

	/**
	 * @brief Convenience function to add a custom string to the current maximum.
	 *
	 * @param test String that should be displayed at the current max.
	 */
	void setCustomStringForMax(QString text);

	void setUseCustomMinimum(bool useCustomMin);
	void setUseCustomMaximum(bool useCustomMax);
	void setCustomMinimum(int min);
	void setCustomMaximum(int max);

public slots:
	void setMinimum(int min);
	void setMaximum(int max);

protected:
	QString textFromValue(int value) const override;

private: 
	bool displayCustomStringForValue;
	bool useCustomMinimum;
	bool useCustomMaximum;
	/// Additional custom minimum value that affects setMinimum() when useCustomMinimum is true.
	int customMinimum;
	/// Additional custom maximum value that affects setMaximum() when useCustomMaximum is true.
	int customMaximum;
	/// Contains all strings that are displayed when displayCustomStringForValue is true.
	QHash<int, QString> customStringForValue;
};

#endif // SCMADVANCEDSPINBOX_HPP
