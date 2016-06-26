#!/usr/bin/perl -w

#
# Copyright (C) 2013 Alexander Wolf
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 
# 02110-1335, USA.

# v.1
# This tool read original Hipparcos catalog (J1991.25) + Hipparcos from VizieR (J2000.0)
# and update coordinates in original catalog.
# v.2
# Get Hipparcos 2 from VizieR (cat. 2007 - I/311) and improve original catalog.
#
# All catalogs can be found here: http://astro.altspu.ru/~aw/stellarium/hipparcos/
#

# Original Hipparcos catalog (1997; J1991.25)
$HIPmain	= "./hip_main.dat";
$HIPcom		= "./h_dm_com.dat";

# Hipparcos from VizieR (2013; J2000.0)
$HIPmainV	= "./hip_main_j2000.dat";
$HIPmainVa	= "./hip_main_j2000a.dat";
$HIPcomV	= "./h_dm_com_j2000.dat";

# Hipparcos 2 from VizieR (2013)
$HIP2main	= "./hip2_main.dat";

# Result
$HIPmainR	= "./hip_main_r.dat";
$HIPcomR	= "./h_dm_com_r.dat";

# Hipparcos main catalog
open(oHIP, "<$HIPmain");
@catalog = <oHIP>;
close oHIP;

open(cHIP, "<$HIPmainV");
@catalogV = <cHIP>;
close cHIP;

open(vHIP, "<$HIPmainVa");
@catalogA = <vHIP>;
close vHIP;

open(tHIP, "<$HIP2main");
@catalogT = <tHIP>;
close tHIP;

for($i=0;$i<scalar(@catalogT);$i++) 
{
	@fixt = split('\|',$catalogT[$i]);
	$id = $fixt[0]+0;
	$Plx[$id] = $fixt[1]; 	# Plx from H2
	$pmRA[$id] = $fixt[2];	# pmRA from H2
	$pmDE[$id] = $fixt[3];	# pmDE from H2
	$Hpmag[$id] = $fixt[4]; # Hpmag from H2
	$BV[$id] = $fixt[5];	# B-V from H2
}

open(rHIP, ">$HIPmainR");
for($i=0;$i<scalar(@catalog);$i++) 
{
	@cat = split('\|',$catalog[$i]);
	@fix = split('\|',$catalogV[$i]);
	@fixa = split('\|',$catalogA[$i]);

	if ($cat[8] ne '            ' && $cat[9] ne '            ')
	{
		if ($fixa[0] ne '            ' && $fixa[1] ne '            ')
		{
			$cat[3] = substr($fixa[0],0,11); # RA
			$cat[4] = substr($fixa[1],0,11); # DE
		}
		$cat[8] = $fix[0]."00";
		$cat[9] = $fix[1]."00";
	}
	$id = $cat[1]+0;
	if ($Plx[$id] ne '') {
		$cat[11] = $Plx[$id]; # Plx from H2
	}
	if ($pmRA[$id] ne '') {
		$cat[12] = $pmRA[$id]; # pmRA from H2
	}
	if ($pmDE[$id] ne '') {
		$cat[13] = $pmDE[$id]; # pmDE from H2
	}
	if ($Hpmag[$id] ne '') {
		$cat[44] = $Hpmag[$id]; # Hpmag from H2
	}
	if ($BV[$id] ne '') {
		$cat[37] = $BV[$id]; # B-V from H2
	}
	print rHIP join('|', @cat);
}
close rHIP;

# Hipparcos binary stars catalog
open(oHIP, "<$HIPcom");
@catalog = <oHIP>;
close oHIP;

open(cHIP, "<$HIPcomV");
@catalogV = <cHIP>;
close cHIP;

open(rHIP, ">$HIPcomR");
for($i=0;$i<scalar(@catalog);$i++) 
{
	@cat = split('\|',$catalog[$i]);
	@fix = split('\|',$catalogV[$i]);
	$cat[20] = $fix[0]."00"; # RA
	$cat[21] = $fix[1]."00"; # DE
	print rHIP join('|', @cat);
}
close rHIP;
