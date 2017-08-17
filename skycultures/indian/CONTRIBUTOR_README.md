# Intro
Welcome and thanks for contributing! For general information, please see description.en.utf8 .

# Updating constellations_boundaries.dat
Since constellations_boundaries.data does not allow comments, we place relevant comments here.

## Format
```
N RA_1 DE_1 RA_2 DE_2 ... RA_N DE_N 2 CON1 CON2
  where
  N number of corners
  RA_n, DE_n right ascension and declination (degrees) of the corners in J2000 coordinates.
  2 CON1 CON2 legacy data. They indicated “border between 2 constellations, CON1, CON1” but
  are now only required to keep the format.
```

## naksatra-junctions
Conversion and coordinate lookup:
-  http://simbad.u-strasbg.fr/simbad/sim-id?Ident=Beta+Arietis
-  https://ned.ipac.caltech.edu/forms/calculator.html

The ecliptic poles are (as of epoch 1 January 2000) at:
 - (North) right ascension 18h 0m 0.0s (exact), declination +66° 33′ 38.55″ (270 66.82)
 - (South) right ascension 6h 0m 0.0s (exact), declination −66° 33′ 38.55″  (90 -66.82)

