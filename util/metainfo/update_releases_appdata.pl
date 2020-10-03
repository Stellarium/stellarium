#!/usr/bin/perl
#
# This is tool to extract change log data and update *.appdata.xml file
#
# Copyright (c) 2020 Alexander Wolf
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
use XML::DOM;

my $CHLF	= "../../ChangeLog";
my $XML		= "../../data/org.stellarium.Stellarium.appdata.xml";

open(CHL, "<$CHLF");
my @changelog = <CHL>;
close CHL;

my %releases = ();
# parse changelog and extract dates of releases with their versions
for(my $i=0; $i<scalar(@changelog); $i++)
{
    my $line = $changelog[$i];
    if ($line =~ m/^([0-9.]+)\s+\[([0-9\-]+)\]/)
    {
	$releases{$1} = $2;
    }
}

my $parser = new XML::DOM::Parser;
my $doc = $parser->parsefile($XML);
my $releasesListElement = $doc->getElementsByTagName('releases'); 
my $node = $releasesListElement->item(0);
# remove all items
for my $id ($node->getChildNodes)
{
    $node->removeChild($id);
}

foreach my $version (reverse sort { $releases{$a} <=> $releases{$b} or $a cmp $b } keys %releases)
{
    my $date = $releases{$version};
    my $newElement = $doc->createElement('release');
    $newElement->setAttribute("date", $date);
    $newElement->setAttribute("version", $version);
    $node->appendChild($newElement);
    #$node->addText("\n");
}

$doc->printToFile($XML);
$doc->dispose;
