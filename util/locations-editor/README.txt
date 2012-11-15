Stellarium Location List Editor
===============================

Locations list policies
-----------------------

The original base_locations.txt seems to contain every or almost every notable
settlement with population >= 15 thousand people. I suggest this number as a
cut-off when proposing new additions to the list.

If you wish to translate an entry, please clone it and edit the new copy. This
way, the original string will be still available. Another posibility is to
aggregate all translations at the end of the file.

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

If you run it from the /utils/locations-editor directory, it will be able to
detect and load the checkout's base_locations.txt file in the /data directory.

If Qt Creator had built the project in a subdirectory (/debug or /release),
set the working directory ("Projects" -> "Run Settings") to the /locations-editor
directory, otherwise the application won't detect base_locations.txt.

The "Also save as binary" option is automatically checked if a project list is loaded and unchecked when a user list is loaded.

When saving a list that contains duplicates in binary form, items with duplicateIDs get conflated with the last item with the same ID, the same as in the
binary saving function in StelLocationMgr.

Due to the way drag-and-drop is implemented, it's possible to move rows from one instance of the app to another,
or to drop rows into a text editor or a spreadsheet.

Known Issues
------------

The .bin.gz file saved by the app is not an actual archive, as it can't be
opened with an archive manager. Nevertheless, Stellarium can read it.

TODO
----

It has been developed and used only on Linux, so:
Test on Mac OS.

Better verification and auto-completion/auto-suggestion of values with
item delegates.

In the long term: Support for adding time zone information.
