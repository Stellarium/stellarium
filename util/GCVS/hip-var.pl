#!/usr/bin/perl

$GCVS 	= "./iii.dat";		# GCVS
$HIPV   = "./varcat-hip.dat"; 	# Hipparcos
$TYCV	= "./varcat-tyc.dat"; 	# Tycho 2

open (HV, ">$HIPV");
#open (TV, ">$TYCV");
open (GV, "$GCVS");
while (<GV>) {
    $rawstring = $_;
    $maxmagstr = substr($rawstring,48,8);
    $maxmag = $maxmagstr+0;
    $maxmagstr =~ s/[ ]+//gi;
    $yearstr   = substr($rawstring,100,4);
    $year = $yearstr+0;
    if ($maxmag<=12.5 && length($maxmagstr)!=0 && $year==0) {
	print HV $rawstring;
    }
#    if ($maxmag>9.0 && $maxmag<=12.5 && $year==0) {
#	print TV $rawstring;
#    }
}
close GV;
close HV;
#close TV;