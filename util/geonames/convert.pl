#!/usr/bin/perl

#
# Tool for create a base_locations.txt file for Stellarium
#
# Copyright (C) 2013 Alexander Wolf
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

$SRC	= "./cities15000.txt";
$CODE	= "./admin1CodesASCII.txt";
$HDR	= "./base_locations.header";
$ADX	= "./base_locations.appendix";
$OUT	= "./base_locations.txt";

open(ALL, "<$SRC");
@allData = <ALL>;
close ALL;

open(BLH, "<$HDR");
@header = <BLH>;
close BLH;

open(APX, "<$ADX");
@appdx = <APX>;
close APX;

open(CC, "<$CODE");
@code = <CC>;
close CC;

%geo = ();
for ($i=0;$i<scalar(@code);$i++)
{
	@gc = split(/\t/, $code[$i]);
	$geo{$gc[0]} = $gc[2];
}

open(OUT, ">$OUT");
for($i=0;$i<scalar(@header);$i++)
{
	print OUT $header[$i];
}
for($i=0;$i<scalar(@allData);$i++)
{
	@item = split(/\t/, $allData[$i]);
	$latn = $item[4]+0;
	$lonn = $item[5]+0;
	if ($latn>=0) { $lat = $latn."N"; } else { $lat = abs($latn)."S"; }
	if ($lonn>=0) { $lon = $lonn."E"; } else { $lon = abs($lonn)."W"; }
	if ($item[7] eq "PPLC") {
		$type = "C";
	} elsif ($item[7] eq "PPLA") {
		$type = "R";
	} else {
		$type = "N";
	}
	$population = $item[14]/1000;
	if ($population >= 1000)
	{
		$pollution = 9;
	} elsif ($population >= 500) {
		$pollution = 8;
	} elsif ($population >= 100) {
		$pollution = 7;
	} elsif ($population >= 50) {
		$pollution = 6;
	} elsif ($population >= 10) {
		$pollution = 5;
	} elsif ($population >= 5) {
		$pollution = 4;
	} else {
		$pollution = '';
	}
	$country = $item[8];
	$country =~ tr/[A-Z]+/[a-z]/;
	#print OUT join("\t", $item[2], $geo{$item[8].".".$item[10]}, $country, $type, $population, $lat, $lon, $item[16], $pollution, $item[17], "", "")."\n";
	print OUT join("\t", $item[2], $geo{$item[8].".".$item[10]}, $country, $type, $population, $lat, $lon, $item[16], $pollution, $item[17])."\n";
}
for($i=0;$i<scalar(@appdx);$i++)
{
	print OUT $appdx[$i];
}

close OUT;
