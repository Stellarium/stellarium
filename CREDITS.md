## LICENSE
```
   Copyright (C) 2004-2017 Fabien Chereau et al.

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
	4.1 All graphics are copyrighted by the Stellarium's Team (GPL) except
	    the ones mentioned below :
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
	4.5b The Sun, Proteus and Iapetus maps and rings of Uranus 
	     are from Celestia (http://shatters.net/celestia/)
	     under the GNU General Purpose License, version 2 or any later
	     version:
	     - The Proteus map is from Phil Stooke.
	     - Iapetus map is from dr. Fridger Schrempp (t00fri).
	4.5c The Amalthea, Ariel, Charon, Europa, Ganymede, Gaspra, Haumea, 
	    Hyperion, Ida, Io, Janus, Mercury, Neptune, Oberon, Phoebe, Pluto, 
	    Saturn, Sedna, Tethys, Umbriel, Vesta, Venus, Mimas, Rhea, Bianca,
	    Ceres, Miranda, Titania, Triton, Uranus, Dysnomia, Eris, Epimetheus
            and comet maps are processed by Oleg Pluton a.k.a Helleformer
	    License: Creative Commons Attribution 4.0 International
	4.5d Callisto map is created by John van Vliet from PDS
	     data and modified by RVS. License: cc-by-sa.
	4.5e Dione and Enceladus maps are created by NASA (CICLOPS team) 
	     from Cassini data, colored by RVS. Public domain.
	4.5f All other planet maps from David Seal's site:
	     http://maps.jpl.nasa.gov/   see license in section 10.2
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
	4.8 M31, and the Pleiades pictures come from Herm Perez :
	    http://home.att.net/~hermperez/default.htm
	    License: "Feel free to use these images, if you use them in a
	    commercial setting please attribute the source."
	4.9 Images of M8, M33, NGC253, NGC1499, NGC2244 
	    from Jean-Pierre Bousquet
	4.10 Images of M1, M15, M16, M27, M42, M57, M97, NGC6946 from Stephane
	     Dumont
	4.11 Images of M17, M44, NGC856, NGC884 from Maxime Spano
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
	4.15 Images of NGC281, NGC5139, NGC6543, NGC6960, NGC7023, NGC7317,
	     NGC7319, NGC7320
	     from Andrey Kuznetsov, Kepler Observatory
	     http://kepler-observatorium.ru
	     License: Creative Commons Attribution 3.0 Unported
	4.16 Images of NGC891, NGC1333, NGC2903, NGC3185, NGC3187, NGC3189,
	     NGC3190, NGC3193, NGC3718, NGC3729, NGC4490, NGC5981, NGC5982,
	     NGC5985, NGC7129 
	     from Oleg Bryzgalov 
	     http://olegbr.astroclub.kiev.ua/
	     License: Creative Commons Attribution 3.0 Unported
	4.17 Image of eta Carinae 
	     from Georg Zotti
	     http://www.waa.at/
	     License: Creative Commons Attribution 3.0 Unported
	4.18 Images of IC1805, IC1848, NGC6888
	     from Steve Tuttle
	     http://www.stuttle1.com/
	4.19 Images of IC4628, M20, M21, M47, NGC2467, IC2948, NGC3324, NGC3293,
	     NGC7590, RCW158
	     from Trevor Gerdes
	     http://www.sarcasmogerdes.com/
	4.20 Images of IC2118, NGC1532
		from users of Ice In Space
		http://www.iceinspace.com.au/
	4.21 Image of IC5146 from James A Weier
	4.22 Images of SMC, LMC (Magellanic Clouds) and rho Oph from Albert Van
	     Donkelaar
	4.23 Images of NGC55, NGC300, NGC1365, NGC3628, NGC4945, NGC5128,
	     NGC6726, NGC6744, NGC6752, NGC6822, NGC7293, NGC2070
	     from Philip Montgomery
	     http://www.kenthurst.bigpondhosting.com/
	4.24 The Vesta and Ceres map was taken from USGS website
	     https://astrogeology.usgs.gov/
	     and colored by RVS. License: public domain.
	4.25 Images of NGC7318, NGC7331, M3, M13, M51, M63, M64, M74, M78, M81,
	     M82, M96, M101, M105, Barnard 22, Barnard 142, Barnard 173, IC405,
	     IC443, NGC1514, NGC1961, NGC2371, NGC2403, NGC246, NGC2841,
	     NGC3310, NGC3938, NGC4559, NGC7008, NGC7380, NGC7479, NGC7635,
	     Sh2-101
	     from Peter Vasey, Plover Hill Observatory
	     http://www.madpc.co.uk/~peterv/
	4.26 Image of IC434 from Marc Aragnou
	4.27 Image of solar corona from eclipse 2008-08-01 by Georg Zotti
	4.28 Images of IC59, IC63, IC410, NGC2359, Sadr region from Carole Pope
	     https://sites.google.com/site/caroleastroimaging/
	4.29 Images of NGC3690, NGC5257, NGC6050, IC883, UGC8335 and UGC9618
	     from NASA, ESA, the Hubble Heritage (STScI/AURA)-ESA/Hubble
	     Collaboration
	     License: public domain; http://hubblesite.org/copyright/
	4.30 Images of NGC40, NGC4631, NGC4627, NGC4656, NGC4657 from Steven
	     Bellavia
	4.31 Images of Barnard Loop, IC342 from Sun Shuwei
	     License: public domain
	4.32 Images of M77, Sh2-264, Sh2-308 from Wang Lingyi
	     License: public domain
	4.33 Images of IC10 from Lowell Observatory
	     http://www2.lowell.edu/
	     License: public domain
	4.34 Images of IC2177, NGC6334, NGC6357, Fornax Cluster from ESO/Digitized
	     Sky Survey 2
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.35 Images of NGC3603 from ESO/La Silla Observatory
	     http://eso.org/public/
	     License: Creative Commons Attribution 4.0 International 
	4.36 Images of NGC4244 from Ole Nielsen
	     License: Creative Commons Attribution-Share Alike 2.5 Generic 
	4.37 Images of NGC7000 from NASA
	     License: public domain
	4.38 Images of IC1396 from Giuseppe Donatiello
	     License: Creative Commons CC0 1.0 Universal Public Domain Dedication
	4.39 Images of Sh2-155 from Hewholooks
	     https://commons.wikimedia.org/wiki/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.40 Images of NGC4565 from Ken Crawford
	     http://www.imagingdeepsky.com/
	     License: This work is free and may be used by anyone for any purpose.
	     If you wish to use this content, you do not need to request permission
	     as long as you follow any licensing requirements mentioned on this page.
	4.41 Images of Coma Cluster from Caelum Observatory
	     http://www.caelumobservatory.com/
	     License: Creative Commons Attribution-Share Alike 3.0 Unported
	4.42 Images of NGC1316 from ESO
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
