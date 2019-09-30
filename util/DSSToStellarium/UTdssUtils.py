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

import dssUtils
import unittest

class TestDssUtils(unittest.TestCase):

    def setUp(self):
      pass

    def test_pointProjection(self):
        wcs = dssUtils.DssWcs("S032")
        pixPos = [500, 1253]
        raDecPos = wcs.pixelToRaDec(pixPos)
        self.assertAlmostEqual(pixPos[0], wcs.raDecToPixel(raDecPos)[0])
        self.assertAlmostEqual(pixPos[1], wcs.raDecToPixel(raDecPos)[1])

if __name__ == '__main__':
    unittest.main()
