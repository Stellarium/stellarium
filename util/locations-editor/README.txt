Stellarium Location List Editor
===============================

Building
--------

Use Qt Creator to open the .pro project file. After building, run "clean" to
avoid temporary files showing up as unversioned files in Bazaar.

Alternatively, from the command line, run:
 qmake locations-editor.pro
 make
 make clean
 ./locations-editor

Usage
-----

If you run it from the /utils directory, it will be able to sniff out and load
the checkout's base_locations.txt file in the /data directory.

The "Also save as binary" option is automatically checked if a project list is loaded and unchecked when a user list is loaded.

When saving a list that contains duplicates in binary form, items with duplicateIDs get conflated with the last item with the same ID, the same as in the
binary saving function in StelLocationMgr.

TODO
----

It has been developed and used only on Linux, so:
Test on Windows.
Test on Mac OS.

Better handling of duplicates.
Better replications of the current format of base_locations.txt
Verification of entered values. Item delegates.

In the long term: Support for adding time zone information.
