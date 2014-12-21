#!/usr/bin/python
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

