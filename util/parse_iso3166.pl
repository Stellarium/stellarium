#!/usr/bin/perl

#
# Tool for parse a iso3166.tab file for extraction of countries names for translation
#
# Copyright (C) 2016 Alexander Wolf
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

$SRC	= "../data/iso3166.tab";
$TMPL	= "./translations_countries.tmpl";
$OUT	= "../src/translations_countries.h";

open(ALL, "<$SRC");
@srcData = <ALL>;
close ALL;

open(TMPL, "<$TMPL");
@tmpl = <TMPL>;
close TMPL;

open(OUT, ">$OUT");
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
				$country = $item[1];
				$country =~ s/&/and/gi;
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
