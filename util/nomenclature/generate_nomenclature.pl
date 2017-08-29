#!/usr/bin/perl
#
# This is tool to extract nomenclature data from the DBF files, which are 
# distributed on the web page https://planetarynames.wr.usgs.gov
#
# Please extract all DBF files from archives into this directory and run the tool.
#
# Copyright (c) 2017 Alexander Wolf
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

use strict;
use DBI;

my @dbfiles;

opendir(DIR, "./") or die "can't open dir: ./";
while(defined(my $file = readdir(DIR))) {
    if ($file =~ /\.dbf$/) { push @dbfiles, $file; }
}

my @header;
open(TMPL, "<./header.tmpl");
@header = <TMPL>;
close TMPL;

for(my $i=0; $i<scalar(@dbfiles); $i++)
{
    my $fileName = $dbfiles[$i];
    my $fabFile  = $fileName;
    $fabFile =~ s/_nomenclature.dbf//gi;
    my $planetName = $fabFile;
    $fabFile = lc $fabFile;
    $fabFile .= ".fab";

    my $dbh = DBI->connect("DBI:XBase:./") or die $DBI::errstr;
    my $sth = $dbh->prepare("SELECT clean_name, diameter, center_lon, center_lat, code, link FROM $fileName") or die $dbh->errstr;
    $sth->execute() or die $sth->errstr;
    open(FAB, ">$fabFile");
    for(my $j=0;$j<scalar(@header);$j++) {
	my $line = $header[$j];
	my $pName   = substr($planetName, 0, 1);
	my $pNameLC = substr($planetName, 1);
	$pNameLC = lc $pNameLC;
	$pName .= $pNameLC;
	$line =~ s/_PLANET_NAME_/$pName/gi;
	$line =~ s/_PLANET_TARGET_/$planetName/gi;
	print FAB $line;
    }
    print FAB "\n";
    
    while(my $arr = $sth->fetchrow_arrayref ) 
    {
	my $id = $arr->[5];
	$id =~ s/http\:\/\/planetarynames\.wr\.usgs\.gov\/Feature\///gi;
	my $latitude  = sprintf "%.6f", $arr->[3];
	my $longitude = sprintf "%.6f", $arr->[2];
	print FAB $id."\t_(\"".$arr->[0]."\")\t".$arr->[4]."\t".$latitude."\t".$longitude."\t".$arr->[1]."\n";
    }
    close FAB;
}