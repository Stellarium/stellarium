# Almagest

## Technical notes

In these notes we will provide technical details on how the data used by the Almagest sky culture has been compiled.  For the user–facing documentation, see the `description.en.utf8` file in this directory.

### The almstars directory

Files included in the `almstars` directory come from Ernie Wright's
[Visualization of the Almagest Catalog.](http://www.etwright.org/astro/almagest.html)

The `cat1.dat` file is used as the source of Almagest descriptions of the stars in English language.  In addition to star descriptions, this file contains BSC (HR) numbers where available and modern designations such as "1 alp UMi" for Polaris or "NGC 2632 M44" for the Beehive (Praesepe) Cluster.

#### Catalog File Format

(Description by Ernie Wright.)

The three Almagest catalog files (`cat1.dat`, `cat2.dat`, `cat3.dat`) are 7-bit ASCII text files with Windows (0Dh 0Ah) line endings. They can be examined in Wordpad (turn off word wrap) or any other text editor. They can also be loaded into most spreadsheet programs by telling them that the data is in fixed-length fields delimited by spaces.

The first line identifies the file as an Almagest Stars catalog and names the catalog:

```text
   Almagest Catalog:  catalog-name
```

This is followed by 1028 variable-length lines. Each line is a star record containing the following fields.

```text
   Bytes  Format  Units    Explanation
   ---------------------------------------------------------
    1- 4  I4               [1-1028] Baily number
    6- 8  A3               constellation abbreviation
   10-11  I2               [1-45] index within constellation
   13-15  I3      deg      longitude degrees
   17-18  I2      arcmin   longitude minutes
   20     A1               latitude minus
   21-22  I2      deg      latitude degrees
   24-25  I2      arcmin   latitude minutes
   27-29  F3.1             magnitude
   31-34  I4               [1-9110] HR number
   36-47  A12              modern Bayer/Flamsteed ID
   49-    A?               description
```

The first line identifies the Almagest star by both Baily number (on the left) and the ordinal within the constellation. The former, as in modern catalogs, is a running ordinal, attributed to Francis Baily, that uniquely numbers each entry in the Almagest catalog. It is listed by Peters and Knobel, and it is the only Almagest star identification in Grasshoff's Appendix B. Toomer, however, follows Manitius in numbering only within each constellation, so you need both numbers when consulting the sources of the catalogs.

The second line is Ptolemy's description of the star's location within the figure of the constellation. Occasionally this also includes remarks about relative brightness or color. The description is given as it appears in the catalog source. The Manitius catalog's descriptions are in German, those from Peters/Knobel are in Latin, and those from Toomer are in English.

The line following the description contains the modern identity of the star and the longitude, latitude, and magnitude as given in the Almagest. (These values do not reflect the amount that the Almagest stars have been shifted, but they do include the corrections entered on the Set Epoch dialog.) The modern identification comprises the HR (Harvard Revised) number in the Yale Bright Star Catalog, the Flamsteed number, the Bayer letter, and the constellation.

The second part of the data lists the corresponding Yale catalog information for stars within one degree of the clicked position.

### The make_names.py script

The `make_names.py` script parses an "almstars" catalogue file and generates `star_names.fab` and `dso_names.fab` files that can be used by Stellarium.  There are four catalogue files included in the `almstars/` directory and the script uses `cat1.dat`.  If you would like to experiment with a different catalogue - change the value of the
`CAT_FILE` variable at the top of the script and run the script as follows:

```sh
$ python3.9 make_names.py
Almagest Catalog:  Toomer/Grasshoff
  found 1022 unique stars and 16 proper names
52 nu1 Boo (HR 5763) is in Boo, Her.
112 bet Tau (HR 1791) is in Aur, Tau.
24 alp PsA (HR 8728) is in Aqr, PsA.
```

Since `star_names.fab` requires HIP rather than HR numbers, the required cross-catalogue data is extracted from the `cross-id.dat` file.  The DSOs are identified by their modern designations and the necessary translations are hard-coded in the script. (There are only three such objects in the Almagest.)

In addition, information required to generate proper names and location of the stars relative to constellation figures that is present in the primary source but not reflected in the computer readable catalogues, is manually
coded in the script.  See the `PROPER_NAMES` and `OUTSIDE` dictionaries.
