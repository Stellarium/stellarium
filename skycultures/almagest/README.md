# Almagest

## Technical notes

In these notes we will provide technical details on how the data used by the Almagest sky culture has been compiled.  For the user–facing documentation, see the `description.en.utf8` file in this directory.

### The almstars directory

Files included in the `almstars` directory come from Ernie Wright's
[Visualization of the Almagest Catalog.](http://www.etwright.org/astro/almagest.html)

The `cat1.dat` file is used as the source of Almagest descriptions of the stars in English language.  In addition to star descriptions, this file contains BSC (HR) numbers where available and modern designations such as "1 alp UMi" for Polaris or "NGC 2632 M44" for the Beehive (Praesepe) Cluster.

### The make_names.py script

The `make_names.py` takes the `cat1.dat` file and generates `star_names.fab` and `dso_names.fab` files that can be used by Stellarium.  Since `star_names.fab` requires HIP rather than HR numbers, the required cross-catalogue data is extracted from the `cross-id.dat` file.  The DSOs are identified by their modern designations and the necesary translations are hard-coded in the script. (There are only three such objects in the Almagest.)
