#!/usr/bin/perl

use LWP::UserAgent;

$GCVS   = "./varcat-hip.dat"; 	# GCVS part
$HIPV	= "./vcat-hip.dat";
$fpart 	= "http://simbad.u-strasbg.fr/simbad/sim-id?Ident=";
$lpart 	= "&NbIdent=1&Radius=2&Radius.unit=arcmin&submit=submit+id";

$ua = LWP::UserAgent->new(
			    keep_alive=>1,
			    timeout=>180
);

$ua->agent("Opera/9.80 (X11; Linux i686; U; ru) Presto/2.9.168 Version/11.50");

open (OUT, ">$HIPV");
open (GV, "$GCVS");
while (<GV>) {
    $rawstring = $_;
    $designation = substr($rawstring,8,9);
    $designation =~ s/[ ]{1,}/+/gi;
    
    $URL = $fpart.$designation.$lpart;
    
    $request = HTTP::Request->new('GET', $URL);
    $responce = $ua->request($request);
    $content = $responce->content;
    
    $content =~ m/>HIP<\/A> ([0-9]+)/gi;
    $hipn = $1;

    $len = 6-length($hipn);
    if ($len<6) {
	$add = "";
        for ($i=0;$i<$len;$i++) {
	    $add .= " ";
        }
	print OUT $hipn.$add."|".$rawstring;
    }
}
close GV;
close OUT;
