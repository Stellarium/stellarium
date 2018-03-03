#!/usr/bin/perl

#
# Tool for parse a iso3166.tab file for extraction of countries names for translation
#
# Copyright (C) 2016, 2017 Alexander Wolf
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

use Text::CSV;
use utf8;

$csvparser = Text::CSV->new();

$SRC	= "../data/iso3166.tab";
$TMPL	= "./translations_countries.tmpl";
$CSV	= "./countries.csv";
$OUT	= "../src/translations_countries.h";

open(ALL, "<:encoding(utf8)", "$SRC");
@srcData = <ALL>;
close ALL;

open(TMPL, "<:encoding(utf8)", "$TMPL");
@tmpl = <TMPL>;
close TMPL;

open(CSV, "<:encoding(utf8)", "$CSV");
@csvdata = <CSV>;
close CSV;

%wikilink = ();
%comment  = ();
for($i=1;$i<scalar(@csvdata);$i++)
{
	$status = $csvparser->parse($csvdata[$i]);
	@csvpdata = $csvparser->fields();
	$wlink = $csvpdata[1];
	$atxt  = $csvpdata[2];
	$isoc  = $csvpdata[3];
	$wikilink{$isoc} = $wlink;
	$comment{$isoc}  = $atxt;
}

open(OUT, ">:encoding(utf8)", "$OUT");
for($i=0;$i<scalar(@tmpl);$i++)
{
	if ($tmpl[$i] =~ m/LIST_OF_COUNTRIES/gi)
	{
		for($j=0;$j<scalar(@srcData);$j++)
		{
			$str = $srcData[$j];
			chop($str);
			if ($str !~ m/#/)
			{
				@item = split(/\t/, $str);
				$iso     = $item[0];
				$country = $item[1];
				$country =~ s/&/and/gi;
				if ($wikilink{$iso} ne '') {
					print OUT "\t\t// TRANSLATORS: ".$wikilink{$iso}."\n";
				} else { print "[".$iso."]\n"; }
				print OUT "\t\tN_(\"".$country."\");\n";
			}
		}
	}
	else
	{
		print OUT $tmpl[$i];
	}
}
close OUT;
