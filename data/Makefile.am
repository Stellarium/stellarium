man_MANS = stellarium.1
pkgdatadir = $(datadir)/@PACKAGE@/data
pkgdata_DATA = icon.bmp stellarium.ico 	script_load_config script_set_time_zone \
	script_set_locale script_save_config 	landscapes.ini ssystem.ini courierfont.txt \
	spacefont.txt messier.fab name.fab 	skycultures.fab files_format.txt zone.tab \
	hipparcos.fab name.txt skylanguages.fab 	cities.fab arial.txt DejaVuSans.ttf \
	cadwell.dat sharpless.dat ngc2000.dat double_txt.dat variable_txt.dat
EXTRA_DIST = $(pkgdata_DATA) $(man_MANS)
SUBDIRS = sky_cultures scripts
