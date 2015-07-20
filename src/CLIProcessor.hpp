/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef CLIPROCESSOR_HPP
#define CLIPROCESSOR_HPP

#include <QStringList>
#include <QVariant>

class QSettings;

class CLIProcessor
{
public:
	//! Check if a QStringList has a CLI-style option in it (before the first --).
	//! @param argList a list of strings, think argv
	//! @param shortOpt a short-form option string, e.g, "-h"
	//! @param longOpt a long-form option string, e.g. "--help"
	//! @return true if the option exists in args before any element which is "--"
	static bool argsGetOption(const QStringList& argList, QString shortOpt, QString longOpt);

	//! Retrieve the argument to an option from a QStringList.
	//! Given a list of strings, this function will extract the argument of
	//! type T to an option, where the option in an element which matches
	//! either the short or long forms, and the argument to that option
	//! is the following element in the list, e.g. ("--option", "arg").
	//! It is also possible to extract argument to options which are
	//! part of the option element, separated by the "=" character, e.g.
	//! ( "--option=arg" ).
	//! Type conversion is done using the QTextStream class, and as such
	//! possible types which this template function may use are restricted
	//! to those where there is a value operator<<() defined in the
	//! QTextStream class for that type.
	//! The argument list is only processed as far as the first value "--".
	//! If an argument "--" is to be retrieved, it must be apecified using
	//! the "--option=--" form.
	//! @param argList a list of strings, think argv.
	//! @param shortOpt the short form of the option, e.g. "-n".
	//! @param longOpt the long form of the option, e.g. "--number".
	//! @param defaultValue the default value to return if the option was
	//! not found in args.
	//! @exception runtime_error("no_optarg") the expected argument to the
	//! option was not found.
	//! @exception runtime_error("optarg_type") the expected argument to
	//! the option could not be converted.
	//! @return The value of the argument to the specified option which
	//! occurs before the first element with the value "--".  If the option
	//! is not found, defaultValue is returned.
	static QVariant argsGetOptionWithArg(const QStringList& argList, QString shortOpt, QString longOpt, QVariant defaultValue);

	//! Check if a QStringList has a yes/no CLI-style option in it, and
	//! find out the argument to that parameter.
	//! e.g. option --use-foo can have parameter "yes" or "no"
	//! It is also possible for the argument to take values, "1", "0";
	//! "true", "false";
	//! @param argList a list of strings, think argv
	//! @param shortOpt a short-form option string, e.g, "-h"
	//! @param longOpt a long-form option string, e.g. "--help"
	//! @param defaultValue the default value to return if the option was
	//! not found in args.
	//! @exception runtime_error("no_optarg") the expected argument to the
	//! option was not found. The longOpt value is appended in parenthesis.
	//! @exception runtime_error("optarg_type") the expected argument to
	//! the option could not be converted. The longOpt value is appended
	//! in parenthesis.
	//! @return 1 if the argument to the specified opion is "yes", "y",
	//! "true", "on" or 1; 0 if the argument to the specified opion is "no",
	//! "n", "false", "off" or 0; the value of the defaultValue parameter if
	//! the option was not found in the argument list before an element which
	//! has the value "--".
	static int argsGetYesNoOption(const QStringList& argList, QString shortOpt, QString longOpt, int defaultValue);

	//! Processing of command line options which is to be done before config file is read.
	//! This includes the chance to set the configuration file name.  It is to be done
	//! in the sub-class of the StelApp, as the sub-class may want to manage the
	//! argument list, as is the case with the StelMainWindow version.
	static void parseCLIArgsPreConfig(const QStringList& argList);

	//! Processing of command line options which is to be done after the config file is
	//! read.  This gives us the chance to over-ride settings which are in the configuration
	//! file.
	static void parseCLIArgsPostConfig(const QStringList& argList, QSettings* conf);
};

#endif // CLIPROCESSOR_HPP
