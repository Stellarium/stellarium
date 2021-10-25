#!/usr/bin/perl

#
# Tool for extraction the names of sky cultures for translation
#
# Copyright (C) 2021 Alexander Wolf
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

$SRC	= "../../skycultures/CMakeLists.txt";
$OUT	= "../../skycultures/translations_skycultures.fab";

open(ALL, "<:encoding(utf8)", "$SRC");
@srcData = <ALL>;
close ALL;

@SC = (); # sky culturees
for($i=0;$i<scalar(@srcData);$i++)
{
	$str = $srcData[$i];
	# if (substr($str, 0, 1) eq '#') { next; }
	if ($str =~ /ADD_SUBDIRECTORY\(\s*([A-Za-z\_\-]+)\s*\)/)
	{
		push(@SC, $1);
	}
}

open(OUT, ">:encoding(utf8)", "$OUT");
print OUT "#\n# This file contains names of sky cultures.\n";
print OUT "# It is not meant to be compiled but just parsed by gettext.\n#\n\n";
print OUT "#\n# Constellation cultures\n#\n";
for($i=0;$i<scalar(@SC);$i++)
{
	$str = $SC[$i];
	
	$INFOSRC = "../../skycultures/".$str."/info.ini";
	open(INFO, "<:encoding(utf8)", "$INFOSRC");
	@info = <INFO>;
	close INFO;
	$name = "";
	for($j=0;$j<scalar(@info);$j++)
	{
		if ($info[$j] =~ m/name\s*=(.+)/)
		{
			$name = $1;
			$name =~ s/\r//g;
			$name =~ s/\n//g;
			$name =~ s/^\s+|\s+$//g;
		}
	}
	
	if ($name ne '')
	{
		print OUT "# TRANSLATORS: Name of the sky culture\n";
		print OUT "qc_(\"".$name."\",\"sky culture\")\n";
	}
}
print OUT "\n";
close OUT;
