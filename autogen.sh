#!/bin/sh

SETUP_GETTEXT=./setup-gettext

($SETUP_GETTEXT --gettext-tool) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have gettext installed to compile Stellarium";
	echo;
	exit;
}

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

# Backup the po/ChangeLog. This should prevent the annoying
# gettext ChangeLog modifications.

cp -p po/ChangeLog po/ChangeLog.save

echo "Running gettextize, please ignore non-fatal messages...."
$SETUP_GETTEXT

# Restore the po/ChangeLog file.
mv po/ChangeLog.save po/ChangeLog


aclocal $ACLOCAL_FLAGS || exit;
autoheader || exit;
automake --add-missing --copy;
autoconf || exit;
automake || exit;
./configure $@

