#!/usr/bin/env python3

from astroquery.vizier import Vizier
from astropy.coordinates import SkyCoord, Angle
from astropy import units as u
import sys
import re
import percache
import os
from collections import defaultdict

def snt_data():
    """
    Reads data from the STDIN and returns a generator that
    returns a dict with each field parsed out.
    """
    #                              mag           ra          npd             bayer              sup       weight    cons
    data_regex = re.compile(r'([0-9\. -]{5}) ([0-9\. ]{8}) ([0-9\. ]{8}) ([A-Za-z0-9 -]{3})([a-zA-Z0-9 ])([0-9])([a-zA-Z]{3})')
    for line in sys.stdin:
        line = line.rstrip('\n\r')
        m = re.match(data_regex, line)

        if m:
            # The S&T data has "Erj" as the continuation of Eridanus
            # after pi.  This is because the S&T data has a gap around
            # pi, since it lies slightly within Cetus.  S&T's own line
            # drawing software requires that one of the last four characters
            # change to signify a new line is to be started, rather than
            # continuing from the previous point.  So they had to create
            # a "fake" constellation to make their line drawing software
            # start a new line after pi.  Hence 'Erj'.
            constellation = m.group(7)
            if constellation == 'Erj':
                constellation = 'Eri'

            yield {
                "mag": float(m.group(1).strip()),
                "ra": round(float(m.group(2).strip()), 5),
                "npd": round(float(m.group(3).strip()), 4),
                "dec": round(90 - float(m.group(3).strip()), 4),
                "bayer": m.group(4).strip(),
                "superscript": None if m.group(5) == " " else m.group(5),
                "weight": int(m.group(6)),
                "constellation": constellation,
            }
        else:
            if not line.startswith('#'):
                print("WARNING: No match: {}".format(line), file=sys.stderr) # lgtm [py/syntax-error]

# livesync=True so that even if we ctrl-c out of
# the program, any previously cached values will
# be present for future invocations
cache = percache.Cache('.hip_cache', livesync=True)
@cache
def get_hip(ra, dec, mag):
    """
    Given an RA (in hours and decimals), and Dec (in
    degrees and decimals), and a magnitude (in
    visual magnitudes), queries VizieR and attempts
    to locate a Hipparcos star ID at the location.

    Returns an integer HIP ID if found, or None otherwise

    Maintains a .hip_cache file to speed up lookups;
    you can delete the .hip_cache file to perform
    fresh lookups.
    """
    coord = SkyCoord(ra=Angle("{} hours".format(ra)),
                     dec=Angle("{} degree".format(dec)),
                     obstime="J2000.0")

    # Search the Hipparcos catalog, and only return results that include
    # a HIP (Hipparcos) column, sorting the results by magnitude.  The
    # top result is almost certainly the star we want.
    v = Vizier(catalog='I/239/hip_main', columns=["HIP", "+Vmag"])
    # Constrain the search to stars within 1 Vmag of our target
    v.query_constraints(Vmag="{}..{}".format(mag - 0.5, mag + 0.5))

    # Start with a targeted search, which returns more quickly from the
    # API. If that fails to find a star, query a 3 degree diameter circle
    # around the ra/dec.  This is because Sky & Telescope has a convention
    # of stopping their constellation lines a degree or so away from the
    # star, if that star isn't actually part of the constellation
    # (example: Alpheratz, which is part of the Pegasus figure, but the
    # star is in Andromeda)
    for radius in (0.05, 1.5):
        result = v.query_region(coord, radius=radius*u.deg)
        try:
            table = result['I/239/hip_main']
        except TypeError:
            # A TypeError means that the results didn't include anything from
            # the I/239/hip_main catalog.  The "in" operator doesn't seem to
            # work with Table objects.
            continue
        else:
            return table['HIP'][0]
    return None

if __name__ == '__main__':
    exitval = 0
    constellationship = defaultdict(list)
    current_line = None
    previous_line = None
    current_hip = None
    previous_hip = None
    try:
        for vertex in snt_data():
            # The line weight and constellation, taken together, indicate
            # one continuous line.  If this value changes, we start a new line.
            previous_line = current_line
            current_line = "{weight}{constellation}".format(**vertex)

            print("{bayer} {constellation} [ra={ra}, dec={dec}, mag={mag}]...".format(**vertex), file=sys.stderr, end='', flush=True)
            previous_hip = current_hip
            # The S&T data is ambiguous for the location of the o2 (31) Cyg vertex,
            # so we skip get_hip and assign it manually.
            if vertex['constellation'] == 'Cyg' and vertex['bayer'] == '31-':
                current_hip = 99848
            elif vertex['constellation'] == 'Pup' and vertex['bayer'] == 'alf':
                # Issue #1438: Ensure line from nu Pup to Canopus ends at Canopus
                current_hip = 30438
            elif vertex['constellation'] == 'UMa' and vertex['bayer'] == 'xi':
                # Issue #1414: Ensure line from nu UMa to xi UMa actually ends at xi UMa
                current_hip = 55203
            else:
                current_hip = get_hip(ra=vertex['ra'], dec=vertex['dec'], mag=vertex['mag'])
            if not current_hip:
                raise ValueError("Unable to locate HIP for {bayer} {constellation} vertex [ra={ra}, dec={dec}, mag={mag}]".format(**vertex))
            print("HIP {}".format(current_hip), file=sys.stderr, flush=True)

            # Append a new line tuple to the constellation if we are still
            # on the same line, and the previous vertex is defined.
            if previous_line == current_line and previous_hip and current_hip:
                constellationship[vertex['constellation']].append( (previous_hip, current_hip,) )
    except KeyboardInterrupt:
        # If the user hits ctrl+c during processing, don't just abort; continue
        # on to output the constellationship data that was already gathered,
        # then exit non-zero.
        exitval = 1
        print("Caught KeyboardInterrupt", file=sys.stderr)

    # Special case for Mensa and Microscopium constellations
    # Use alpha and beta of each constellation to generate the lines
    constellationship['Men'].append( (29271, 23467,) )
    constellationship['Mic'].append( (102831, 102989,) )

    print("Generating constellationship data...", file=sys.stderr)
    for constellation in sorted(constellationship.keys()):
        # The format of constellationship.fab is:
        # <constellation name> <number of lines> startHIP endHIP startHIP endHIP ...
        print("{} {} {}".format(constellation, len(constellationship[constellation]), " ".join([ "{} {}".format(i[0], i[1]) for i in constellationship[constellation] ])))

    sys.exit(exitval)
