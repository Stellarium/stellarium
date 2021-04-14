## LICENSE
```
   Copyright (C) 2004-2019 Fabien Chereau et al.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

   See the COPYING file for more information regarding the GNU General
   Public License.
```

## Full Reference & Credits
```
1. Technical Articles
	1.1 The tone reproductor class
		The class mainly performs a fast implementation of the algorithm
		from the paper [1], with more accurate values from [2]. The blue
		shift formula is taken from [3] and combined with the Scotopic
		vision formula from [4].
		[1] "Tone Reproduction for Realistic Images", Tumblin and
		    Rushmeier, IEEE Computer Graphics & Application, November
		    1993
		[2] "Tone Reproduction and Physically Based Spectral Rendering",
		    Devlin, Chalmers, Wilkie and Purgathofer in EUROGRAPHICS
		    2002
		[3] "Night Rendering", H. Wann Jensen, S. Premoze, P. Shirley,
		    W.B. Thompson, J.A. Ferwerda, M.M. Stark
		[4] "A Visibility Matching Tone Reproduction Operator for High
		    Dynamic Range Scenes", G.W. Larson, H. Rushmeier, C. Piatko
	1.2 The skylight class
		The class is a fast implementation of the algorithm from the
		article "A Practical Analytic Model for Daylight" by A. J.
		Preetham, Peter Shirley and Brian Smits.
	1.3 The skybright class
		The class is a fast reimplementation of the VISLIMIT.BAS basic
		source code from Brad Schaefer's article on pages 57-60,  May
		1998 _Sky & Telescope_,	"To the Visual Limits". The basic
		sources are available on the Sky and Telescope web site.
		(code "offered as-is and without support.")
	1.4 The Delta-T calculations
		For implementation of calculation routines for Delta-T we used 
		the following sources:
		[1] Delta-T webpage by Rob van Gent: 
		    http://www.staff.science.uu.nl/~gent0113/deltat/deltat.htm
		[2] "Five Millennium Canon of Solar Eclipses", Espenak and Meeus
		    http://eclipse.gsfc.nasa.gov/SEhelp/deltatpoly2004.html
		[3] "On the system of astronomical constants", Clemence, G. M.,
		    Astronomical Journal, Vol. 53, p. 169
		    http://adsabs.harvard.edu/abs/1948AJ.....53..169C
		[4] "The Rotation of the Earth, and the Secular Accelerations of
		    the Sun, Moon and Planets",
		    Spencer Jones, Monthly Notices of the Royal Astronomical 
		    Society, 99 (1939), 541-558
		    http://adsabs.harvard.edu/abs/1939MNRAS..99..541S
		[5] "Polynomial approximations for the correction delta T E.T.-
		    U.T. in the period 1800-1975",
		    Schmadel, L. D.; Zech, G., Acta Astronomica, vol. 29, no. 1,
		    1979, p. 101-104.
		    http://adsabs.harvard.edu/abs/1979AcA....29..101S
		[6] "ELP 2000-85 and the dynamic time-universal time relation",
		    Borkowski, K. M.,
		    Astronomy and Astrophysics (ISSN 0004-6361), vol. 205, no. 
		    1-2, Oct. 1988, p. L8-L10.
		    http://adsabs.harvard.edu/abs/1988A&A...205L...8B
		[7] "Empirical Transformations from U.T. to E.T. for the Period
		    1800-1988", 
		    Schmadel, L. D.; Zech, G., Astronomische Nachrichten 309, 
		    219-221
		    http://adsabs.harvard.edu/abs/1988AN....309..219S
		[8] "Historical values of the Earth's clock error DeltaT and the
		    calculation of eclipses",
		    Morrison, L. V.; Stephenson, F. R., Journal for the History
		    of Astronomy (ISSN 0021-8286), 
		    Vol. 35, Part 3, No. 120, p. 327 - 336 (2004)
		    http://adsabs.harvard.edu/abs/2004JHA....35..327M
		[9] "Addendum: Historical values of the Earth's clock error", 
		    Morrison, L. V.; Stephenson, F. R.,
		    Journal for the History of Astronomy (ISSN 0021-8286), Vol.
		    36, Part 3, No. 124, p. 339 (2005)
		    http://adsabs.harvard.edu/abs/2005JHA....36..339M
		[10] "Polynomial approximations to Delta T, 1620-2000 AD", 
		    Meeus, J.; Simons, L.,
		    Journal of the British Astronomical Association, vol.110,
		    no.6, 323
		    http://adsabs.harvard.edu/abs/2000JBAA..110..323M
		[11] "Einstein's Theory of Relativity Confirmed by Ancient Solar
		    Eclipses", Henriksson G.,
		    http://adsabs.harvard.edu/abs/2009ASPC..409..166H
		[12] "Canon of Solar Eclipses" by Mucke & Meeus (1983)
		[13] "The accelerations of the earth and moon from early
		     astronomical observations", 
		     Muller P. M., Stephenson F. R.,
		     http://adsabs.harvard.edu/abs/1975grhe.conf..459M
		[14] "Pre-Telescopic Astronomical Observations", Stephenson F.R.,
		    http://adsabs.harvard.edu/abs/1978tfer.conf....5S
		[15] "Long-term changes in the rotation of the earth - 700 B.C.
		     to A.D. 1980", 
		     Stephenson F. R., Morrison L. V., 
		     Philosophical Transactions, Series A (ISSN 0080-4614), vol.
		     313, no. 1524, Nov. 27, 1984, p. 47-70.
		     http://adsabs.harvard.edu/abs/1984RSPTA.313...47S
		[16] "Long-Term Fluctuations in the Earth's Rotation: 700 BC to
		    AD 1990",
		    Stephenson F. R., Morrison L. V., 
		    Philosophical Transactions: Physical Sciences and 
		    Engineering, Volume 351, Issue 1695, pp. 165-202
		    http://adsabs.harvard.edu/abs/1995RSPTA.351..165S
		[17] "Historical Eclipses and Earth's Rotation" by F. R. 
		    Stephenson (1997)
		    http://ebooks.cambridge.org/ebook.jsf?bid=CBO9780511525186
		[18] "Astronomical Algorithms" by J. Meeus (2nd ed., 1998)
		[19] "Astronomy on the Personal Computer" by O. Montenbruck & 
		     T. Pfleger (4nd ed., 2000)
		[20] "Calendrical Calculations" by E. M. Reingold & 
		     N. Dershowitz (2nd ed., 2001)
		[21] DeltaT webpage by V. Reijs: 
		     http://www.iol.ie/~geniet/eng/DeltaTeval.htm
	1.5 An accurate long-time precession model compatible with P03:
		J. Vondrak, N. Capitaine, P. Wallace: New precession expressions,
		valid for long time intervals.
		Astronomy&Astrophysics 534, A22 (2011); 
		DOI: 10.1051/0004-6361/201117274
	1.6 Nutation: 
		Dennis D. McCarthy and Brian J. Lizum: An Abridged Model of the
		Precession-Nutation of the Celestial Pole.
		Celestial Mechanics and Dynamical Astronomy 85: 37-49, 2003.
		This model provides accuracy better than 1 milli-arcsecond in the
		time 1995-2050. It is applied for years 1500..2500 only.

2. Included source code
	2.1 Some computation of the sideral time (sidereal_time.h/c) and pluto
	    orbit contains code from the libnova library (LGPL) by Liam Girdwood.
	2.2 The orbit.cpp/h and solve.h files are directly borrowed from
	    Celestia (Chris Laurel). (GPL license)
	2.3 Several implementations of IMCCE theories for planet and satellite
	    movement by Johannes Gajdosik (MIT-style license,
	    see the corresponding files for the license text)
	2.4 The tesselation algorithms were originally extracted from the glues 
	    library version 1.4 Mike Gorchak <mike@malva.ua> (SGI FREE SOFTWARE
	    LICENSE B).
	2.5 OBJ loader in the Scenery3D plugin based on glObjViewer (c) 2007 
	    dhpoware
	2.6 Parts of the code to work with DE430 and DE431 data files have been 
	    taken from Project Pluto (GPL license).
	2.7 The SpoutLibrary.dll and header from the SpoutSDK version 2.005 
	    available at http://spout.zeal.co (BSD license).

3. Data
	3.1 The Hipparcos star catalog
	    From ESA (European Space Agency) and the Hipparcos mission.
	    ref. ESA, 1997, The Hipparcos and Tycho Catalogues, ESA SP-1200
	    http://cdsweb.u-strasbg.fr/ftp/cats/I/239
	3.2 The solar system data mainly comes from IMCCE and partly from
	    Celestia.
	3.3 Polynesian constellations are based on diagrams from the Polynesian
	    Voyaging Society
	3.4 Chinese constellations are based on diagrams from the Hong Kong
	    Space Museum
	3.5 Egyptian constellations are based on the work of Juan Antonio
	    Belmonte, Instituto de Astrofisica de Canarias
	3.6 The Tycho-2 Catalogue of the 2.5 Million Brightest Stars
	    Hog E., Fabricius C., Makarov V.V., Urban S., Corbin T.,
	    Wycoff G., Bastian U., Schwekendiek P., Wicenec A.
	    <Astron. Astrophys. 355, L27 (2000)>
	    http://cdsweb.u-strasbg.fr/ftp/cats/I/259
	3.7 Naval Observatory Merged Astrometric Dataset (NOMAD) version 1
	    http://www.nofs.navy.mil/nomad
	    Norbert Zacharias writes:
	    "There are no fees, both UCAC and NOMAD are freely available
	    with the only requirement that the source of the data (U.S.
	    Naval Observatory) and original product name need to be provided
	    with any distribution, as well as a description about any
	    changes made to the data, if at all."
	    The changes made to the data are:
	    -) try to compute visual magnitude and color from the b,v,r
	       values
	    -) compute nr_of_measurements = the number of valid b,v,r values
	    -) throw away or keep stars (depending on magnitude,
	       nr_of_measurements, combination of flags, tycho2 number)
	    -) add all stars from Hipparcos (incl. component solutions), and
	       tycho2+1st supplement
	    -) reorganize the stars in several brigthness levels and
	       triangular zones according to position and magnitude
	    The programs that are used to generate the star files are called
	    "MakeCombinedCatalogue", "ParseHip", "ParseNomad", and can be
	    found in the util subdirectory in source code. The position,
	    magnitudes, and proper motions of the stars coming from NOMAD
	    are unchanged, except for a possible loss of precision,
	    especially in magnitude. When there is no v-magnitude, it is
	    estimated from r or b magnitude.  When there is no b- or v-
	    magnitude, the color B-V is estimated from the other magnitudes.
	    Also proper motions of faint stars are neglected at all.
	3.8 Stellarium's Catalog of Variable Stars based on General Catalog of
	    Variable Stars (GCVS) version 2013Apr.
	    http://www.sai.msu.su/gcvs/gcvs/
	    Samus N.N., Durlevich O.V., Kazarovets E V., Kireeva N.N., 
	    Pastukhova E.N., Zharova A.V., et al., General Catalogue of Variable
	    Stars (Samus+ 2007-2012)
	    http://cdsarc.u-strasbg.fr/viz-bin/Cat?cat=B%2Fgcvs&
	3.9 Consolidated DSO catalog was created from various data:
	    [1]  NGC/IC data taken from SIMBAD Astronomical Database
	         http://simbad.u-strasbg.fr
	    [2]  Distance to NGC/IC data taken from NED (NASA/IPAC EXTRAGALACTIC
	         DATABASE) 
	         http://ned.ipac.caltech.edu
	    [3]  Catalogue of HII Regions (Sharpless, 1959) taken from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/20
	    [4]  H-α emission regions in Southern Milky Way (Rodgers+, 1960)
	         taken from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/216
	    [5]  Catalogue of Reflection Nebulae (Van den Bergh, 1966) taken
	         from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/21
	    [6]  Lynds' Catalogue of Dark Nebulae (LDN) (Lynds, 1962) taken
	         from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/7A
	    [7]  Lynds' Catalogue of Bright Nebulae (Lynds, 1965) taken from
	         VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/9
	    [8]  Catalog of bright diffuse Galactic nebulae (Cederblad, 1946)
	         taken from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/231
	    [9]  Barnard's Catalogue of 349 Dark Objects in the Sky (Barnard,
	         1927) taken from VizieR
	         http://vizier.u-strasbg.fr/viz-bin/VizieR?-source=VII/220A
	    [10] A Catalogue of Star Clusters shown on Franklin-Adams Chart
	         Plates (Melotte, 1915) taken from NASA ADS
	         http://adsabs.harvard.edu/abs/1915MmRAS..60..175M
	    [11] On Structural Properties of Open Galactic Clusters and their
	         Spatial Distribution. Catalog of Open Galactic Clusters.
	         (Collinder, 1931) taken from NASA ADS
	         http://adsabs.harvard.edu/abs/1931AnLun...2....1C
	    [12] The Collinder Catalog of Open Star Clusters. An Observer’s
	         Checklist.
	         Edited by Thomas Watson. Taken from CloudyNights
	         http://www.cloudynights.com/page/articles/cat/articles/the-collinder-catalog-updated-r2467
	3.10 Cross-identification of objects in consolidated DSO catalog was
	     maked with:
	    [1]  SIMBAD Astronomical Database
	         http://simbad.u-strasbg.fr
	    [2]  Merged catalogue of reflection nebulae (Magakian, 2003)
	         http://vizier.u-strasbg.fr/viz-bin/VizieR-3?-source=J/A+A/399/141
	    [3]  Messier Catalogue was taken from Wikipedia (includes
	         morphological classification and distances)
	         https://en.wikipedia.org/wiki/List_of_Messier_objects
	    [4]  Caldwell Catalogue was taken from Wikipedia (includes
	         morphological classification and distances)
	         https://en.wikipedia.org/wiki/Caldwell_catalogue
	3.11 Morphological classification and magnitudes (partially) for Melotte
	     catalogue was taken from DeepSkyPedia
	     http://deepskypedia.com/wiki/List:Melotte


4. Graphics
	4.1 All graphics are copyrighted by the Stellarium's Team (GNU GPLv2 or later) 
	    except the ones mentioned below :
	4.2 The "earthmap" texture was created by NASA (Reto Stockli, NASA Earth
	    Observatory) using data from the MODIS instrument aboard the
	    Terra satellite (Public Domain). See chapter 10.1 for full
	    credits.
	4.4 Moon texture map was combined from maps by USGS Astrogeology Research
	    Program,
	    http://astrogeology.usgs.gov (Public Domain, DFSG-free) and by Lunar
	    surface textures from Celestia, based on Clementine data (Public
	    Domain).
	4.5a Jupiter map created by James Hastings-Trew from Cassini data. "The 
	     maps are free to download and use as source material or resource
	     in artwork or rendering (CGI or real time)."
	4.5b The Sun and Iapetus maps and rings of Uranus 
	     are from Celestia (http://shatters.net/celestia/)
	     under the GNU General Purpose License, version 2 or any later
	     version:
	     - Iapetus map is from dr. Fridger Schrempp (t00fri).
	4.5c The Amalthea, Europa, Ganymede, Gaspra,Mercury, Uranus, Neptune, Mimas,
	     Bianca, Epimetheus, Ida, Vesta, Hyperion, Io, Janus, Phoebe, Saturn, 
	     Tethys, Venus, Ceres, Uranus, Epimetheus, Deimos, Proteus and comet
             maps are processed by Oleg Pluton a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5d The Titania, Umbriel, Pluto, Charon, Sedna and 2007 OR10 maps are created
	     by Kexitt and postprocessed by Oleg Pluton a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5e The Ariel, Haumea, Miranda, Oberon and Nereid maps are created by 
	     Snowfall and postprocessed by Oleg Pluton a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5f The Eris and Dysnomia maps are created by MrSpace43 and postprocessed by 
	     Oleg Pluton a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5g The Rhea map are created by FarGetaNik and postprocessed by Oleg Pluton 
	     a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5i The Titan (Clouds) map are created by Magenta Meteorite and postprocessed
	     by Oleg Pluton a.k.a Helleformer
	     License: Creative Commons Attribution 4.0 International
	4.5k Callisto map is created by John van Vliet from PDS
	     data and modified by RVS. License: cc-by-sa.
	4.5l Dione and Enceladus maps are created by NASA (CICLOPS team) 
	     from Cassini data, colored by RVS. Public domain.
	4.5m All other planet maps from David Seal's site:
	     http://maps.jpl.nasa.gov/   see license in section 10.2
	4.5n Bennu map is created by NASA from OSIRIS-REx spacecraft data. Public domain.
	     Full size mosaic: https://www.asteroidmission.org/bennu_global_mosaic/
	4.6 The fullsky milky way panorama is created by Axel Mellinger,
	    University of Potsdam, Germany. Further information and more
	    pictures available from
	    http://home.arcor-online.de/axel.mellinger/
	    License: permission given to "Modify and redistribute this image
	    if proper credit to the original image is given."
	4.7 All messiers nebula pictures except those mentioned below from the
	    Grasslands Observatory : "Images courtesy of Tim Hunter and
	    James McGaha, Grasslands Observatory at http://www.3towers.com."
	    License: permission given to "use the image freely" (including
	    right to modify and redistribute) "as long as it is credited."
	4.8 M31 pictures come from LEE ang HG731GZ
	    License: Creative Commons Attribution 3.0 Unported
	4.9 Images of NGC4526, NGC6544, NGC6553, Sh2-261
	    from Yang Kai
	    License: public domain 
	4.10 Images of M1, M27, M57, M97, NGC6946 from Stephane
	     Dumont
	4.11 Images of NGC856, NGC884 from Maxime Spano
	4.12 Constellation art, GUI buttons, logo created by Johan Meuris
	     (Jomejome) (jomejome at users.sourceforge.net)
	     http://www.johanmeuris.eu/
	     License: released under the Free Art License
	     (http://artlibre.org/licence.php/lalgb.html)
	     Icon created by Johan Meuris
	     License: Creative Commons Attribution-ShareAlike 3.0 Unported
	4.13 The "earth-clouds" texture includes imagery owned by NASA.
	     See NASA's Visible Earth project at http://visibleearth.nasa.gov/
	     License: 1. The imagery is free of licensing fees
		      2. NASA requires that they be provided a credit as the 
		         owners of the imagery
	     The cloud texturing was taken from Celestia (GPL),
	     http://www.shatters.net/celestia/
	4.14 The folder icon derived from the Tango Desktop Project, used under
	     the terms of the Creative Commons Attribution Share-Alike
	     license.
	4.15 Images of NGC7317, NGC7319, NGC7320
	     from Andrey Kuznetsov, Kepler Observatory
	     http://kepler-observatorium.ru
	     License: Creative Commons Attribution 3.0 Unported
	4.16 Images of NGC2903, NGC3185, NGC3187, NGC3189,
	     NGC3190, NGC3193, NGC3718, NGC3729, NGC5981, NGC5982,
	     NGC5985
	     from Oleg Bryzgalov 
	     http://olegbr.astroclub.kiev.ua/
	     License: Creative Commons Attribution 3.0 Unported
	4.17 Image of eta Carinae 
	     from Harel Boren
	     http://www.pbase.com/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.18 Images of IC1805
	     from Aldeabaran66
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.19 Images of M21, M47, NGC3324, NGC7590, RCW158
	     from Trevor Gerdes
	     http://www.sarcasmogerdes.com/
	4.20 Images of NGC1532
	     from users of Ice In Space
	     http://www.iceinspace.com.au/
	4.21 Images of NGC3293
	     from ESO/G. Beccari
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International
	4.22 Images of SMC (Magellanic Clouds) from Albert Van
	     Donkelaar
	4.23 Images of NGC1365, NGC6726
	     from Philip Montgomery
	     http://www.kenthurst.bigpondhosting.com/
	4.24 The Vesta and Ceres map was taken from USGS website
	     https://astrogeology.usgs.gov/
	     and colored by RVS. License: public domain.
	4.25 Images of M15, M24, M51, M58, M63, M76,
	     M81, M82, M96, M101, M105, M106, IC405, IC443, IC1727, NGC246, 
	     NGC457, NGC467, NGC470, NGC474, NGC488, NGC672, NGC691, NGC1514, 
	     NGC1961, NGC2174, NGC2371, NGC2392, NGC2403, NGC2655, NGC2685, 
	     NGC2805, NGC2814, NGC2820, NGC2841, NGC3166, NGC3310, NGC3344, 
	     NGC3359, NGC3504, NGC3512, NGC3521, NGC3628, NGC3938, NGC4151, 
	     NGC4535, NGC4559, NGC4631, NGC4656, NGC4657, NGC5033, NGC7008, 
	     NGC7318, NGC7331, NGC7479, NGC7635, NGC7640, NGC7789, 
	     PGC1803573, Barnard 142, Barnard 173, Sh2-101, 
	     LDN1235, Sadr region (Gamma Cygni), Jones-Emberson 1, Medusa
	     from Peter Vasey, Plover Hill Observatory
	     http://www.madpc.co.uk/~peterv/
	4.26 Image of IC3568 from Howard Bond (Space Telescope Science Institute), Robin Ciardullo (Pennsylvania State University) and NASA
	     License: public domain
	4.27 Image of solar corona from eclipse 2008-08-01 by Georg Zotti
	4.28 Images of IC410, NGC2359 from Carole Pope
	     https://sites.google.com/site/caroleastroimaging/
	4.29 Images of NGC2261, NGC2818, NGC2936, NGC3314, NGC3690, NGC3918,
	     NGC4038-4039, NGC5257, NGC5307, NGC6027, NGC6050, NGC6369, NGC6826, NGC7742, IC883,
	     IC4406, PGC2248, UGC1810, UGC8335, UGC9618, Red Rectangle and Calabash Nebula
	     from NASA, ESA, the Hubble Heritage (STScI/AURA)-ESA/Hubble
	     Collaboration
	     License: public domain; http://hubblesite.org/copyright/
	4.30 Image of NGC40 from Steven Bellavia
	4.31 Images of NGC1579, NGC2467, NGC6590, Barnard Loop, IC342, SH2-297 from Sun Shuwei
	     License: public domain
	4.32 Images of M77, NGC3109, NGC3180, NGC4449, NGC5474, NGC6231, NGC6397, Sh2-264, Sh2-308, LDN1622
	     from Wang Lingyi
	     License: public domain
	4.33 Images of PGC6830, PGC29653, UGC5373 from Lowell Observatory
	     http://www2.lowell.edu/
	     License: public domain
	4.34 Images of M59, M60, M89, M90, IC2177, IC2220, IC2631, NGC1433,
	     NGC2442, NGC3572, PGC10074,
	     RCW32, RCW38, RCW49, Fornax Cluster, Virgo Cluster
	     from ESO/Digitized Sky Survey 2
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.35 Images of NGC3603 from ESO/La Silla Observatory
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.36 Images of M49, NGC147, NGC404, NGC4228, NGC4244
	     from Ole Nielsen
	     License: Creative Commons Attribution-Share Alike 2.5 Generic 
	4.37 Images of NGC2808, NGC7023, Hercules Cluster from NASA
	     License: public domain
	4.38 Images of M44, IC1396 from Giuseppe Donatiello
	     License: Creative Commons CC0 1.0 Universal Public Domain Dedication
	4.39 Images of M4, M12, M14, M19, M20, M22, M55, M56, M62, M88, M92, M108, 
	     M109, IC5146, NGC225, NGC281, NGC663, NGC891, NGC1931, NGC6823, NGC7814
	     from Hewholooks
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.40 Images of NGC4565 from Ken Crawford
	     http://www.imagingdeepsky.com/
	     License: This work is free and may be used by anyone for any purpose.
	     If you wish to use this content, you do not need to request permission
	     as long as you follow any licensing requirements mentioned on this page.
	4.41 Images of M61, M64, M65, M66, M91, M99, M100, NGC613, NGC772, NGC918, NGC1042, NGC1360, NGC1398,
	     NGC1501, NGC1535, NGC1555, NGC2158, NGC2282, NGC2346, NGC2362, NGC2440, NGC2683, NGC2775, NGC3132,
	     NGC3242, NGC3486, NGC3750, NGC4170, NGC4216, NGC4361, NGC4395, NGC4414,,NGC4450, NGC4676, NGC4725,
	     NGC5216, NGC5248, NGC5426, NGC5466, NGC6302, NGC6522, NGC6543, NGC6563, NGC6781, NGC6894, NGC7000,
	     NGC7354, NGC7822, SH2-136, UGC3697, Abell31, Abell33, Abell39, Cassiopeia A, LBN438, Wild's Triplet
	     from Adam Block/Mount Lemmon SkyCenter/University of Arizona
	     http://www.caelumobservatory.com/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.42 Images of IC1295, NGC134, NGC92, NGC55, NGC300, NGC908, NGC936, NGC1232, NGC1313, NGC1316,
	     NGC1792, NGC1978, NGC2207, NGC2736, NGC3199, NGC3699, NGC3766, NGC3532, NGC4945, NGC5128, NGC5189,
	     NGC6362, NGC6537, NGC6752, NGC6769, NGC6822, NGC7793
	     from ESO
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International
 	4.43 Images of M33, M42, NGC253, NGC5566, LBN782, the Pleiades from HG731GZ
	     License: Creative Commons Attribution 3.0 Unported
	4.44 Images of M8, M17, M94, M104, IC434, IC2944, IC4628, NGC2264, NGC6334, NGC6357,
	     NGC6726, NGC7293, SMC (Hydrogen Alpha)
	     from Dylan O'Donnell
	     http://deography.com/
	     License: public domain
	4.45 Images of NGC247
	     No machine-readable source provided. Own work assumed (based on copyright claims).
	     No machine-readable author provided. Fany Toporenko assumed (based on copyright claims).
	     License: Permission is granted to copy, distribute and/or modify this document
	     under the terms of the GNU Free Documentation License, Version 1.2 or any later
	     version published by the Free Software Foundation; with no Invariant Sections,
	     no Front-Cover Texts, and no Back-Cover Texts. A copy of the license is
	     included in the section entitled GNU Free Documentation License.
	4.46 Images of IC1613 
	     from Philos2000
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.47 Images of M16, IC4592, IC4601, NGC4236, NGC6914, Perseus Cluster
	     from Sun Gang
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.48 Images of NGC4651, NGC5906
	     from R. Jay GaBany
	     https://www.cosmotography.com/images/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.49 Images of NGC7662
	     from Gianluca.pollastri
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.50 Images of M2, M3, M5, M13, M10, M28, M30, M53, M54, M69, M71, M75, M78, M85, M102, M107,
	     IC10, PGC143, UGC12613
	     from Starhopper
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.51 Images of M95, NGC520, NGC660, NGC925, NGC1300, NGC4490, NGC6888, NGC7129, NGC7497
	     from Jschulman555
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution 3.0 Unported
	4.52 Images of M98
	     from Clh288
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 2.5 Generic
	4.53 Images of NGC6818
	     from Robert Rubin (NASA/ESA Ames Research Center), Reginald Dufour and Matt Browning (Rice University), Patrick Harrington (University of Maryland), and NASA/ESA
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.54 Images of NGC6188
	     from Ivan Bok
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution 3.0 Unported
	4.55 Images of NGC6188
	     from Friendlystar
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution 3.0 Unported
	4.56 Images of NGC1023
	     from Fryns Fryns
	     https://commons.wikimedia.org/wiki/
	     License:  public domain
	4.57 Images of NGC1269, NGC3195
	     from Zhu Ying
	     License:  Creative Commons Attribution 3.0 Unported
	4.58 Images of IC2118, NGC1097, NGC1499
	     from Zhao Jingna
	     License:  Creative Commons Attribution 3.0 Unported
	4.59 Images of NGC1491
	     from Chuck Ayoub
	     https://commons.wikimedia.org/wiki/
	     License:  Creative Commons Attribution-Share Alike 4.0 International
	4.60 Images of IC2169, NGC2244, NGC3077, SH2-155, SH2-263, UGC10822, Barnard 22, Barnard207,
	     Abell85
	     from Keesscherer
	     https://commons.wikimedia.org/wiki/
	     License:  Creative Commons Attribution-Share Alike 4.0 International
	4.61 Images of NGC3115
	     from Mi Lan
	     License:  Creative Commons Attribution 3.0 Unported
	4.62 Images of NGC3621
	     from ESO and Joe DePasquale
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.63 Images of NGC2547, Vela supernova remnant
	     from ESO/J.Perez
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.64 Images of M83
	     from TRAPPIST/E. Jehin/ESO
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.65 Images of NGC288
	     from Sunfishtommy
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.66 Images of IC1848, NGC1055
	     from Jeffjnet
	     http://www.iceinspace.com.au/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.67 Images of NGC2867, NGC6884
	     from Howard Bond (ST ScI) and NASA/ESA
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International 
	4.68 Images of NGC4567
	     from Klaus Hohmann
	     https://commons.wikimedia.org/wiki
	     License: Creative Commons Attribution-Share Alike 3.0 Germany
	4.69 Images of NGC5286, RCW53
	     from Kong Fanxi
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.70 Images of NGC5897
	     from San Esteban
	     http://www.astrosurf.com
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.71 Images of NGC6388
	     from ESO, F. Ferraro (University of Bologna)
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.72 Images of NGC7380, SH2-80, SH2-106
	     from Cristina Cellini
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution 4.0 International
	4.73 Images of NGC6744
	     from Zhuokai Liu, Jiang Yuhang
	     License: Creative Commons Attribution 4.0 International 
	4.74 Images of NGC2170, SH2-1, SH2-240
	     from Rogelio Bernal Andreo
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.75 Images of VDB 152
	     from Hubble Space Telescope
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.76 Images of M72, NGC6905
	     from Tom Wildoner
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.77 Images of NGC2623
	     from NASA, ESA and A. Evans (Stony Brook University, New York, University of Virginia & National Radio Astronomy Observatory, Charlottesville, USA)
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.78 Images of IC4634, NGC3808, NGC5882, NGC6210, NGC6572, NGC6741
	     from ESA/Hubble & NASA
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.79 Images of NGC6240, PGC33423
	     from NASA, ESA, the Hubble Heritage (STScI/AURA)-ESA/Hubble Collaboration, and A. Evans (University of Virginia, Charlottesville/NRAO/Stony Brook University)
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.80 Images of UGC5470
	     from Friendlystar
	     https://ko.wikipedia.org/
	     License: Creative Commons Attribution 3.0 Unported
	4.81 Images of UGC6253, UGC9749
	     from Giuseppe Donatiello
	     https://en.wikipedia.org/
	     License: public domain
	4.82 Images of UGC10214
	     from NASA, Holland Ford (JHU), the ACS Science Team and ESA
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.83 Images of NGC6503, NGC7714, Hydra Cluster
	     from NASA, ESA, Digitized Sky Survey 2
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.84 Images of NGC2477
	     from Guillermo Abramson
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution 3.0 Unported
	4.85 Images of NGC4755
	     from ESO/Y. Beletsky
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.86 Images of NGC7538
	     from J. Aleu
	     https://commons.wikimedia.org/wiki/
	     License: public domain
	4.87 Images of IC2395, IC4756, NGC3114, NGC6025, NGC6087, NGC6124, NGC6633
	     from Roberto Mura
	     https://en.wikipedia.org/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.88 Images of NGC362
	     from ESO/VISTA VMC
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.89 Images of NGC2997
	     from Adam Block/ChileScope
	     http://www.caelumobservatory.com/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.90 Images of M74, NGC1788, NGC5053, Sh2-132, Coma Cluster, Leo Cluster, barnard150, LDN673
	     from Bart Delsaert
	     https://delsaert.com/
	     License: Creative Commons Attribution 3.0 Unported
	4.91 Images of NGC1333
	     from AAE-Agrupacio Astronomica d'Eivissa
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 4.0 International
	4.92 Images of NGC5139
	     from Jose Mtanous
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.93 Images of NGC6960
	     from Jose Mtanous
	     https://commons.wikimedia.org/wiki/
	     License: public domain
	4.94 Images of NGC6445
	     from The Pan-STARRS1 Surveys (PS1) and the PS1 public science archive have been made possible through contributions by the Institute for Astronomy, the University of Hawaii, the Pan-STARRS Project Office, the Max-Planck Society and its participating institutes, the Max Planck Institute for Astronomy, Heidelberg and the Max Planck Institute for Extraterrestrial Physics, Garching, The Johns Hopkins University, Durham University, the University of Edinburgh, the Queen's University Belfast, the Harvard-Smithsonian Center for Astrophysics, the Las Cumbres Observatory Global Telescope Network Incorporated, the National Central University of Taiwan, the Space Telescope Science Institute, the National Aeronautics and Space Administration under Grant No. NNX08AR22G issued through the Planetary Science Division of the NASA Science Mission Directorate, the National Science Foundation Grant No. AST-1238877, the University of Maryland, Eotvos Lorand University (ELTE), the Los Alamos National Laboratory, and the Gordon and Betty Moore Foundation.
	     https://ps1images.stsci.edu/cgi-bin/ps1cutouts
	     License: public domain
	4.95 Images of LMC (Magellanic Clouds)
	     from ESO/R. Gendler and Sun Shuwei
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International. This work is a derivative of "Map of the Large Magellanic Cloud" and "The entire Large Magellanic Cloud with annotations" by ESO/R. Gendler, used under Creative Commons Attribution 4.0 International. This work is licensed under Creative Commons Attribution 4.0 International by Sun Shuwei.
	4.96 Images of rho Oph
	     from ESO/S. Guisard
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.97 Images of IC59, IC63
	     from Zhang Ruiping
	     License: Creative Commons Attribution 4.0 International 
	4.98 Images of IC423
	     from Astrowicht
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.99 Images of IC418, IC4593, NGC5315, NGC6751
	     from NASA/ESA and The Hubble Heritage Team (STScI/AURA)
	     https://www.spacetelescope.org/
	     License: Creative Commons Attribution 4.0 International
	4.100 Images of NGC7009 from ESO/J. Walsh
	      http://eso.org/public/
	      License: Creative Commons Attribution 4.0 International
	4.101 Images of NGC7027 from Judy Schmidt
	      https://commons.wikimedia.org/wiki/
	      License: public domain 
	4.102 Images of NGC6309, NGC6891 from Fabian RRRR
	      https://commons.wikimedia.org/wiki/
	      License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.103 Images of NGC2022 from ESA/Hubble & NASA, R. Wade
	      https://www.spacetelescope.org/
	      License: Creative Commons Attribution 4.0 International
	4.104 Images of NGC654 from Antonio F. Sanchez
	      https://commons.wikimedia.org/wiki/
	      License: Creative Commons Attribution-Share Alike 4.0 International
	4.105 Images of NGC6496
	      from Mohamad Abbas
	      https://commons.wikimedia.org/wiki/
	      License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.106 Images of PGC3589
	      from ESA/Hubble, Digitized Sky Survey 2
	      https://www.spacetelescope.org/
	      License: Creative Commons Attribution 4.0 International
	4.107 Images of NGC1672
	      from Davide De Martin (ESA/Hubble), the ESA/ESO/NASA Photoshop FITS Liberator & Digitized Sky Survey 2
	      https://www.spacetelescope.org/
	      License: Creative Commons Attribution 4.0 International
	4.108 Images of NGC4298
	      from NASA, ESA, Digitized Sky Survey 2; Acknowledgement: Davide De Martin
	      https://www.spacetelescope.org/
	      License: Creative Commons Attribution 4.0 International
	4.109 Images of NGC1502
	      from Kamil Pecinovsky
	      https://commons.wikimedia.org/wiki/
	      License: Creative Commons Attribution-Share Alike 4.0 International
	4.110 Images of NGC752
	      from Tayson82
	      https://commons.wikimedia.org/wiki/
	      License: Creative Commons Attribution-Share Alike 4.0 International
	4.111 Images of Sh2-170
	      from Wang Wei
	      License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.112 Images of NGC2613
	      from ESO/IDA/Danish 1.5 m/R. Gendler, J.-E. Ovaldsen, C. Thone and C. Feron
	      http://eso.org/public/
	      License: Creative Commons Attribution 4.0 International
```

## Appendix
```
1 Full credits for image 4.2
	Author: Reto Stockli, NASA Earth Observatory,
		rstockli (at) climate.gsfc.nasa.gov
	Address of correspondence:
		Reto Stockli
		ETH/IAC (NFS Klima) & NASA/GSFC Code 913 (SSAI)
		University Irchel
		Building 25 Room J53
		Winterthurerstrasse 190
		8057 Zurich, Switzerland
	Phone:  +41 (0)1 635 5209
	Fax:    +41 (0)1 362 5197
	Email:  rstockli (at) climate.gsfc.nasa.gov
	http://earthobservatory.nasa.gov
	http://www.iac.ethz.ch/staff/stockli
	Supervisors:
		Fritz Hasler and David Herring, NASA/Goddard Space Flight Center
	Funding:
		This project was realized under the SSAI subcontract 2101-01-027
		(NAS5-01070)

	License :
		"Any and all materials published on the Earth Observatory are
		freely available for re-publication or re-use, except where
		copyright is indicated."

2 License for the JPL planets images
(http://www.jpl.nasa.gov/images/policy/index.cfm)

    Unless otherwise noted, images and video on JPL public web sites (public
    sites ending with a jpl.nasa.gov address) may be used for any purpose
    without prior permission, subject to the special cases noted below.
    Publishers who wish to have authorization may print this page and retain
    it for their records; JPL does not issue image permissions on an image
    by image basis.  By electing to download the material from this web site
    the user agrees:
    1. that Caltech makes no representations or warranties with respect to
       ownership of copyrights in the images, and does not represent others
       who may claim to be authors or owners of copyright of any of the
       images, and makes no warranties as to the quality of the images.
       Caltech shall not be responsible for any loss or expenses resulting
       from the use of the images, and you release and hold Caltech harmless
       from all liability arising from such use.
    2. to use a credit line in connection with images. Unless otherwise
       noted in the caption information for an image, the credit line should
       be "Courtesy NASA/JPL-Caltech."
    3. that the endorsement of any product or service by Caltech, JPL or
       NASA must not be claimed or implied.
    Special Cases:
    * Prior written approval must be obtained to use the NASA insignia logo
      (the blue "meatball" insignia), the NASA logotype (the red "worm"
      logo) and the NASA seal. These images may not be used by persons who
      are not NASA employees or on products (including Web pages) that are
      not NASA sponsored. In addition, no image may be used to explicitly
      or implicitly suggest endorsement by NASA, JPL or Caltech of
      commercial goods or services. Requests to use NASA logos may be
      directed to Bert Ulrich, Public Services Division, NASA Headquarters,
      Code POS, Washington, DC 20546, telephone (202) 358-1713, fax (202)
      358-4331, email bert.ulrich@hq.nasa.gov.
    * Prior written approval must be obtained to use the JPL logo (stylized
      JPL letters in red or other colors). Requests to use the JPL logo may
      be directed to the Television/Imaging Team Leader, Media Relations
      Office, Mail Stop 186-120, Jet Propulsion Laboratory, Pasadena CA
      91109, telephone (818) 354-5011, fax (818) 354-4537.
    * If an image includes an identifiable person, using the image for
      commercial purposes may infringe that person's right of privacy or
      publicity, and permission should be obtained from the person. NASA
      and JPL generally do not permit likenesses of current employees to
      appear on commercial products. For more information, consult the NASA
      and JPL points of contact listed above.
    * JPL/Caltech contractors and vendors who wish to use JPL images in
      advertising or public relation materials should direct requests to the
      Television/Imaging Team Leader, Media Relations Office, Mail Stop
      186-120, Jet Propulsion Laboratory, Pasadena CA 91109, telephone
      (818) 354-5011, fax (818) 354-4537.
    * Some image and video materials on JPL public web sites are owned by
      organizations other than JPL or NASA. These owners have agreed to
      make their images and video available for journalistic, educational
      and personal uses, but restrictions are placed on commercial uses.
      To obtain permission for commercial use, contact the copyright owner
      listed in each image caption.  Ownership of images and video by
      parties other than JPL and NASA is noted in the caption material
      with each image.
```
