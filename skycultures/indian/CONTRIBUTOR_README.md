# Intro
Welcome and thanks for contributing! For general information, please see description.en.utf8 .

# Updating constellations_boundaries.dat
Since constellations_boundaries.data does not (prior to 0.16.1) allow comments, we place relevant comments here.

## Format
```
N RA_1 DE_1 RA_2 DE_2 ... RA_N DE_N 2 CON1 CON2
  where
  N number of corners
  RA_n, DE_n right ascension and declination (degrees) of the corners in J2000 coordinates.
  2 CON1 CON2 indicates “border between 2 constellations, CON1, CON1”.
```
PS: The above description is corrected from the user guide, following [this discussion](https://bugs.launchpad.net/stellarium/+bug/1710443/comments/22).

## naksatra-junctions
Conversion and coordinate lookup:
-  http://simbad.u-strasbg.fr/simbad/sim-id?Ident=Beta+Arietis
-  https://ned.ipac.caltech.edu/forms/calculator.html

The ecliptic poles are (as of epoch 1 January 2000) at:
 - (North) right ascension 18h 0m 0.0s (exact), declination +66° 33′ 38.55″ (270 66.82)
 - (South) right ascension 6h 0m 0.0s (exact), declination −66° 33′ 38.55″  (90 -66.82)

