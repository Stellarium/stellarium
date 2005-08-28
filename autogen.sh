#!/bin/sh

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile Stellarium";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile Stellarium";
	echo;
	exit;
}

echo "Generating configuration files for Stellarium, please wait...."
echo;

aclocal -I m4 || exit;
autoheader || exit;
automake --add-missing --copy;
autoconf || exit;
automake || exit;

