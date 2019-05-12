#!/usr/bin/python

# This file is part of Stellarium. Stellarium is free software: you can
# redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, version 2.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Copyright Fabien Chereau <fabien.chereau@gmail.com>

import sys
from pathlib import Path

from PIL import Image

if __name__ == '__main__':
    outDirectory = Path('./results')
    level = 11
    imSize = 256

    print(f"Generate upper levels tile in {outDirectory}")

    if not outDirectory.exists():
        print(f"Output directory {outDirectory} doesn't exist. It should be there and contain tiles for level {level}.")
        sys.exit(-1)

    level = level - 1
    while level >= 0:
        print("Start level %s" % level)
        outDirectory.joinpath(level).mkdir(exist_ok=True)
        allTiles = []
        for filename in os.listdir("%s/%d" % (outDirectory, level + 1)):
            basename, extension = filename.split('.')
            if basename.endswith("partial"):
                tmp, partial = basename.split('-')
                if not os.path.exists("%s/%d/%s" % (outDirectory, level + 1, tmp + '.' + extension)):
                    cmd = "mv %s/%d/%s %s/%d/%s" % (
                        outDirectory, level + 1, filename, outDirectory, level + 1, tmp + '.' + extension)
                    os.system(cmd)
                basename = tmp
            xystr = basename.split('_')
            allTiles.append((int(xystr[0]), int(xystr[1])))

        toGenerate = {}
        for tile in allTiles:
            key = (tile[0] / 2, tile[1] / 2)
            if key not in toGenerate:
                toGenerate[key] = []
            toGenerate[key].append(tile)

        i = 0
        for (x, y) in toGenerate:
            i = i + 1
            if i % 1000 == 0:
                print("%d/%d" % (i, len(toGenerate)))
            resImg = Image.new("RGB", (imSize, imSize))
            tile = toGenerate[(x, y)]
            if (x * 2, y * 2) in tile:
                im = Image.open("%s/%d/%d_%d.jpg" % (outDirectory, level + 1, x * 2, y * 2)).resize(
                    (imSize / 2, imSize / 2), Image.NEAREST)
                resImg.paste(im, (0, 0))
            if (x * 2 + 1, y * 2) in tile:
                im = Image.open("%s/%d/%d_%d.jpg" % (outDirectory, level + 1, x * 2 + 1, y * 2)).resize(
                    (imSize / 2, imSize / 2), Image.NEAREST)
                resImg.paste(im, (imSize / 2, 0))
            if (x * 2, y * 2 + 1) in tile:
                im = Image.open("%s/%d/%d_%d.jpg" % (outDirectory, level + 1, x * 2, y * 2 + 1)).resize(
                    (imSize / 2, imSize / 2), Image.NEAREST)
                resImg.paste(im, (0, imSize / 2))
            if ((x * 2 + 1, y * 2 + 1)) in tile:
                im = Image.open("%s/%d/%d_%d.jpg" % (outDirectory, level + 1, x * 2 + 1, y * 2 + 1)).resize(
                    (imSize / 2, imSize / 2), Image.NEAREST)
                resImg.paste(im, (imSize / 2, imSize / 2))
            resImg.save("%s/%d/%d_%d.jpg" % (outDirectory, level, x, y), quality=90)

        level = level - 1
