 ## ### ### #   #   ### ###  #  # # # #
 #   #  ##  #   #   ### ##   #  # # ###
##   #  ### ### ### # # # #  #  ### # #

By Fabien Chéreau : stellarium@free.fr
Info : http://stellarium.free.fr

Stellarium comes with ABSOLUTELY NO WARRANTY.  See the gpl.txt file for
details.

-----------------------------------------------------------------
REQUIREMENT
-----------------------------------------------------------------
Windows or linux
A 3d openGL acceleration card and a good CPU.

-----------------------------------------------------------------
QUICK START
-----------------------------------------------------------------
Navigation :
Use the direction keys to move the point of view.
Use page up and page down keys to zoom in and out.
Use left mouse button to select an object, right button to select
no object and middle mouse button or SPACE to center on the 
selected object.
Zoomming on nebulaes or planets is very interesting.... (use P to find planets)
You can follow the earth rotation by pressing the key T, 
this is usefull when you are doing a deep zoom.
Press the H key for more help.

-----------------------------------------------------------------
CONFIGURATION
-----------------------------------------------------------------
Use the files config/config.txt and config/location.txt to change the video settings and other 
parameters.

Speed configuration :
If the FPS (number of Frame Per Second) is too low (<3), you probably have 
no 3D acceleration. Delete the file "config.txt" and rename the file 
"config_no_3D_acceleration.txt" in "config.txt". It should be a bit faster,
but with less options.


------------------------------------------------------------------
INSTALLATION INSTUCTIONS
------------------------------------------------------------------
WINDOWS USERS : 
unzip stellarium.zip then just execute stellarium.exe!

TO INSTALL ON LINUX :
Compile with gcc/g++.
You need openGL, Glu, Glut and glpng libraries.
See in glpng.zip for the glpng library.

To use the Makefile, you have to change the GL, GLU and glut 
libraries locations if needed. (add -Ldir in the FLAG section with dir=directory of the 
path where you have those libraries)
Then :

go in the "src" directory and type :

$make
$cd..
$./Stellarium

Notes : some distributions need to include -lXmu to link properly

------------------------------------------------------------------
THANKS
------------------------------------------------------------------
    Brad Schaefer for his sky rendering algorithm.
    Sylvain Ferey for his optimisation of the grids drawing.
    Ben Wyatt (ben@wyatt100.freeserve.co.uk) for the great Glpng library.
    Jean-François Tremblay for his porting on MacOSX.
	Vincent Caron for his parser bugfix and Linux compatibility bugfixes.
	Nick Porcino for his planet function.
    Tangui Morlier for his help on Linux System.
	Bill Gray (projectpluto.com) and
	Mark Huss (mark@mhuss.com) for all the astro libraries.
	Nate Miller (nkmiller@calpoly.edu) for his excellent vector librarie.
    Chris Laurel (claurel@shatters.net) who makes Celestia.
	Yuuki Ninomiya (gm@debian.or.jp) for the parsecfg file.

------------------------------------------------------------------
NOTE
------------------------------------------------------------------
This program is free, but if you have nothing to do with your 
money :) , just send it to :

	Club Astronomie de l'INSA de Lyon
	20, Av. Albert Einstein
	69621 Villeurbanne, FRANCE


------------------------------------------------------------------
LICENSE
------------------------------------------------------------------
   Copyright (C) 2002 Fabien Chéreau chereau@free.fr

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   See gpl.txt for more information regarding the GNU General Public License.
