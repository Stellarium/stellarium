#!/usr/bin/perl

#
# Tool for extraction the list of contributors from git log
#
# Copyright (C) 2022 Alexander Wolf
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

$TMPL	= "./contributors.tmpl";
$OUT	= "../../src/gui/ContributorsList.hpp";
$ppl    = 5;
$prefix = "        ";

@contributors = ();

# bots and synonyms
@exclude_list = ('Alexander Wolf', 'Launchpad Translations', 'Marcos CARDINOT', 'treaves', 'gzotti',
		 'whitesource-bolt-for-github\[bot\]', 'Александр Вольф', 'fossabot', 'toshaevil',
		 't4saha', 'jess', 'pjw', 'wlaun', 'S', 'transifex-integration\[bot\]', 'codesee-maps\[bot\]');
# replacements for nicks
%replacements = ('holgerno'=>'Holger Nießner', 'Hans'=>'Hans Lambermont', 'rossmitchell'=>'Ross Mitchell',
		 'seescho'=>'Edgar Scholz', 'martinber'=>'Martin Bernardi', 'daniel.adastra'=>'Daniel Adastra',
		 'arya-s'=>'Andrei Borza', 'alex'=>'Alexander Duytschaever','GunChleoc'=>'Fòram na Gàidhlig',
		 'miroslavbroz'=>'Miroslav Broz', 'espie'=>'Marc Espie', 'rich'=>'Pavel Klimenko',
		 'Snow Sailor'=>'Nick Kanel');

open(GITSL, "git shortlog -sn |") or die "Can't execute git shortlog: $!";
while (<GITSL>) {
    if (/^\s*\d+\t+(.+)$/) {
		$data = $1;
		# removing
		for($i=0;$i<scalar(@exclude_list);$i++) {
			$data =~ s/^$exclude_list[$i]$//;
		}
		# replacing
		foreach $k (keys %replacements){
			$data =~ s/^$k$/$replacements{$k}/;
		}
		if ($data ne '') { push @contributors, $data; }
    }
}
close GITSL;

open(TMPL, "<$TMPL");
@tmpl = <TMPL>;
close TMPL;

open(OUT, ">$OUT");
for($i=0;$i<scalar(@tmpl);$i++)
{
	if ($tmpl[$i] =~ m/LIST_OF_CONTRIBUTORS/gi)
	{
		$cs = scalar(@contributors);
		$k = 0;
		if ($cs>0) {
			for($j=0;$j<($cs-1);$j++) {
				if ($k==0) { print OUT $prefix; }
				print OUT "\"".$contributors[$j]."\", ";
				if ($k==($ppl-1)) { print OUT "\n"; $k = 0; } else { $k++; }
			}
			if ($k>=$ppl || $k==0) { print OUT $prefix; }
			print OUT "\"".$contributors[$cs-1]."\"\n";
		}
	}
	else
	{
		print OUT $tmpl[$i];
	}
}
close OUT;
