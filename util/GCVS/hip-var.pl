#!/usr/bin/perl

$GCVS 	= "/home/aw/varstar/iii.dat";		# GCVS
$HIPV   = "/home/aw/varstar/varcat-hip.dat"; 	# Hipparcos
$TYCV	= "/home/aw/varstar/varcat-tyc.dat"; 	# Tycho 2

open (HV, ">$HIPV");
open (TV, ">$TYCV");
open (GV, "$GCVS");
while (<GV>) {
    $rawstring = $_;
    $maxmagstr = substr($rawstring,48,8);
    $maxmag = $maxmagstr+0;
    $maxmagstr =~ s/[ ]+//gi;
    $yearstr   = substr($rawstring,87,4);
    $year = $yearstr+0;
    if ($maxmag<=8.0 && length($maxmagstr)!=0 && $year==0) {
	print HV $rawstring;
    }
    if ($maxmag>8.0 && $maxmag<=11.5 && $year==0) {
	print TV $rawstring;
    }
}
close GV;
close HV;
close TV;