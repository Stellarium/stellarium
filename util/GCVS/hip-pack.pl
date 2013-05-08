#!/usr/bin/perl

$SC = "/home/aw/varstar/vcat-hip.dat"; 	# Source
$OC = "/home/aw/varstar/vcat-hip-repack.dat"; 	# Destination

open (OC, ">$OC");
open (SC, "$SC");
while (<SC>) {
    $rawstring = $_;
    $hipstr = substr($rawstring,0,6);
    $designationstr = substr($rawstring,15,9);
    $vclassstr = substr($rawstring,44,9);
    $maxmagstr = substr($rawstring,55,7);
    $ampflagstr = substr($rawstring,65,1);
    $minmagstr = substr($rawstring,66,6);
    $flagstr = substr($rawstring,78,2);
    $epochstr = substr($rawstring,81,10);
    $periodstr = substr($rawstring,100,16);
    $mmstr = substr($rawstring,121,2);
    $sclassstr = substr($rawstring,127,16);

    $ampflag = 0;
    if ($ampflagstr eq '(') {
	$ampflag = 1;
    }
    if ($ampflagstr eq '<') {
	$ampflag = 2;
    }
    
    if ($flagstr eq 'V' || $flagstr eq 'V ') {
	$flag = 0;
    }
    if ($flagstr eq 'Hp') {
	$flag = 1;
    }
    if ($flagstr eq 'B' || $flagstr eq 'B ') {
	$flag = 2;
    }
    if ($flagstr eq 'U' || $flagstr eq 'U ') {
	$flag = 3;
    }
    if ($flagstr eq 'p' || $flagstr eq 'p ') {
	$flag = 4;
    }
    if ($flagstr eq 'K' || $flagstr eq 'K ') {
	$flag = 5;
    }
    if ($flagstr eq 'Y' || $flagstr eq 'Y ') {
	$flag = 6;
    }

    
    print OC $hipstr."|".$designationstr."|".$vclassstr."|".$maxmagstr."|".$ampflag."|".$minmagstr."|".$flag."|".$epochstr."|".$periodstr."|".$mmstr."|".$sclassstr."\n";
}
close SC;
close OC;
