#!/usr/bin/perl

#
# Tool for create a GCVS catalog for Stellarium
#
# Copyright (C) 2013 Alexander Wolf
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
