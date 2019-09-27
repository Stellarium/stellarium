#!/usr/bin/env python3
# vim: syntax=python ts=4 sts=4 sw=4 expandtab

# Reads the Sky & Telescope official star names document (.docx)
# given as the first argument, and returns star_names.fab output

import docx
import sys
import re
import percache
from astroquery.vizier import Vizier
from collections import defaultdict

# livesync=True so that even if we ctrl-c out of
# the program, any previously cached values will
# be present for future invocations
cache = percache.Cache('.hip_cache_stars', livesync=True)
@cache
def get_hip(name):
    """
    Given a star's Bayer designation, queries
    VizieR and attempts to locate a Hipparcos
    star ID at the location.

    Returns an integer HIP ID if found, or None otherwise

    Maintains a .hip_cache_stars file to speed up lookups;
    you can delete the .hip_cache_stars file to perform
    fresh lookups.
    """
    # Search the Hipparcos catalog, and only return results that include
    # a HIP (Hipparcos) column, sorting the results by magnitude.  The
    # top result is almost certainly the star we want.
    v = Vizier(catalog='I/239/hip_main', columns=["HIP", "+Vmag"])

    try:
        result = v.query_object(name)
    except EOFError:
        # if we get no results we might get an EOFError
        return None
    try:
        table = result['I/239/hip_main']
    except TypeError:
        # A TypeError means that the results didn't include anything from
        # the I/239/hip_main catalog.  The "in" operator doesn't seem to
        # work with Table objects.
        return None
    else:
        return table['HIP'][0]

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: {} SnT_star_names.docx".format(sys.argv[0]), file=sys.stderr) # lgtm [py/syntax-error]
        sys.exit(1)

    doc = docx.Document(sys.argv[1])

    # Get the list of stars from the Word document.  The stars
    # are easy to find because they're in their own paragraphs,
    # with a tab separating the star name and its bayer designation.
    # Some star names have footnotes; the regex looks for numbers,
    # commas, and spaces after the star name, and doesn't include them
    # in the star name.
    stars = defaultdict(dict)
    star_re = re.compile('^\s*([^,0-9]+)[ 0-9,]*\t(.*)$')
    for p in doc.paragraphs:
        m = re.match(star_re, p.text)
        if m:
            # There is a list of standard vs non-standard star names in the document
            # that match the regex, so we check for the non-standard names and skip them.
            # The standard names appear later in the document.
            if m.group(2) in ('Alnair', 'Almaak', 'Alphekka', 'Alnath', 'Etamin', 'Mirphak', 'Phad', 'Rigil Kent', 'Shedir', 'Nonstandard'):
                continue
            # There are two lists of star names, sorted differently.  We just
            # use the first list and skip the second one.
            if m.group(1) in stars:
                continue
            stars[m.group(1)] = { }
            stars[m.group(1)]['bayer'] = m.group(2)
            print("{} -> {} ({})".format(p.text.encode('utf-8'), m.group(1), m.group(2)), file=sys.stderr)

    # Now get the HIP designator for each star
    try:
        for name in sorted(stars):
            print("{} ({})".format(name, stars[name]['bayer']), file=sys.stderr)
            for n in (stars[name]['bayer'], name,):
                print(" * {}: ".format(n), end='', file=sys.stderr)
                hip = get_hip(n)
                if hip:
                    stars[name]['hip'] = hip
                    print(str(hip), file=sys.stderr)
                    break
                else:
                    print("FAILED", file=sys.stderr)
    except KeyboardInterrupt:
        print("KeyboardInterrupt: ending processing and printing out current data", file=sys.stderr)

    # Add additional notable stars that are in the Pocket Sky Atlas
    stars["Hind's Crimson Star"]['hip'] = 23203
    stars["Vela X-1"]['hip'] = 44368
    stars["Lalande 21185"]['hip'] = 54035
    stars["Groombridge 1830"]['hip'] = 57939
    stars["Rmk 14"]['hip'] = 59654
    stars["V1002"]['hip'] = 69995
    stars["H N 28"]['hip'] = 73184
    stars["T = Blaze Star"]['hip'] = 78322
    stars["Barnard's Star"]['hip'] = 87937
    stars["Lacaille 8760"]['hip'] = 105090
    stars["Herschel's Garnet Star"]['hip'] = 107259
    stars["Krueger 60"]['hip'] = 110893
    stars["Bradley 3077"]['hip'] = 114622
    stars["Proxima Centauri"]['hip'] = 70890
    stars["Alula Borealis"]['hip'] = 55219

    # Manually enter some stars that don't return results in VizieR
    stars["Alula Australis"]['hip'] = 55203
    stars["Arkab"]['hip'] = 95241

    # Clean up the data to ensure all stars have HIP designations
    for name in [ *stars.keys() ]:
        if 'hip' not in stars[name] or not stars[name]['hip']:
            print('warning: {} has no HIP designation'.format(name), file=sys.stderr)
            del(stars[name])

    # Print out the star_names.fab formatted data
    for name in sorted(stars, key=lambda k: stars[k]['hip']):
        print('{:>6}|_("{}")'.format(stars[name]['hip'], name))
