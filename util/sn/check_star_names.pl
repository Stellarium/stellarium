#!/usr/bin/perl

#
# Tool for check the names of stars
#
# Copyright (C) 2022 Alexander V. Wolf
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

use utf8;
use open qw( :encoding(UTF-8) :std );

$IAU = "./IAU-CSN.txt";
$SND = "../../skycultures/modern/star_names.fab";
$OUT = "./report.txt";
$FAB = "./star_names.fab";

$sources = "6,12";
$starnames = "";
open(SN, "<$SND");
while(<SN>) {
    if (/^\s*(\d+)\|\_\(\"(.+)\"\)/) {
        $hip  = $1;
        $name = $2;
        $starnames .= $name."-".$hip.";";
    }
}
close SN;
$starnames =~ s/\s+//gi;

%wgsn = ();
open(IAU, "<$IAU");
@IAUDATA = <IAU>;
close IAU;

for($i=0;$i<scalar(@IAUDATA);$i++) {
    $data = $IAUDATA[$i];

    if (substr($data, 0, 1) eq '#' || substr($data, 0, 1) eq '$' || substr($data, 0, 1) eq '\r' || substr($data, 0, 1) eq '\n' || substr($data, 0, 1) eq '') { next; }

    # Name/ASCII
    $sname = substr($data, 0, 17);
    # Name/Diacritics
    $sname = substr($data, 18, 17);
    $sname =~ s/\s{2,}//gi;
    $ship  = substr($data, 89, 7);
    $ship  =~ s/\s+//gi;
    $wgsn{$sname} = $ship;
}

open(OUT, ">$OUT");
open(FAB, ">$FAB");
@keys = sort { lc $a cmp lc $b } keys %wgsn;
foreach $k (@keys) {
    if ($wgsn{$k} ne '_') {
        $check = $k."-".$wgsn{$k};
        $check =~ s/\s+//gi;
        if ($starnames =~ /$check/) {
            $rep = "OK";
            $draft = 0;
        } else {
            $rep = "--- MISSING ---";
            $draft = 1;
        }
        printf OUT "%6d %-20s : %-15s\n", $wgsn{$k}, $k, $rep;
        if ($draft==1) {
            $hip = sprintf("%6d", $wgsn{$k});
            print FAB "$hip|_(\"$k\") $sources\n";
        }
    }
}
close FAB;
close OUT;