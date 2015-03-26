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

$SC = "./vcat-hip.dat"; 	# Source
$OC = "./gcvs_hip_part.dat"; 	# Destination

open (OC, ">$OC");
open (SC, "$SC");
while (<SC>) {
    $rawstring = $_;
    $hipstr = substr($rawstring,0,6);
    $designationstr = substr($rawstring,15,9);
    $vclassstr = substr($rawstring,44,9);
    $maxmagstr = substr($rawstring,55,7);
    $ampflagstr = substr($rawstring,65,1);
    $min1magstr = substr($rawstring,66,6);
    $min2magstr = substr($rawstring,79,6);
    $flagstr = substr($rawstring,91,2);
    $epochstr = substr($rawstring,94,10);
    $periodstr = substr($rawstring,114,16);
    $mmstr = substr($rawstring,134,2);
    $sclassstr = substr($rawstring,140,16);
    
    $hipstr =~ s/(\s+)//gi;
    $designationstr =~ s/(\s+)/ /gi;
    $vclassstr =~ s/(\s+)//gi;
    $maxmagstr =~ s/(\s+)//gi;
    $min1magstr =~ s/(\s+)//gi;
    $min2magstr =~ s/(\s+)//gi;
    $epochstr =~ s/(\s+)//gi;
    $periodstr =~ s/(\s+)//gi;
    $mmstr =~ s/(\s+)//gi;
    $sclassstr =~ s/(\s+)//gi;
    $flagstr =~ s/(\s+)//gi; 
    
    $designationstr =~ s/alf/α/;
    $designationstr =~ s/bet/β/;
    $designationstr =~ s/gam/γ/;
    $designationstr =~ s/del/δ/;
    $designationstr =~ s/eps/ε/;
    $designationstr =~ s/zet/ζ/;
    $designationstr =~ s/eta/η/;
    $designationstr =~ s/the/θ/;
    $designationstr =~ s/tet/θ/; # typo
    $designationstr =~ s/iot/ι/;
    $designationstr =~ s/kap/κ/;
    $designationstr =~ s/lam/λ/;
    $designationstr =~ s/mu./μ/;
    $designationstr =~ s/nu./ν/;
    $designationstr =~ s/xi./ξ/;
    $designationstr =~ s/omi/ο/;
    $designationstr =~ s/pi./π/;
    $designationstr =~ s/rho/ρ/;
    $designationstr =~ s/sig/σ/;
    $designationstr =~ s/tau/τ/;
    $designationstr =~ s/ups/υ/;
    $designationstr =~ s/phi/φ/;
    $designationstr =~ s/ksi/ξ/;
    $designationstr =~ s/khi/χ/;
    $designationstr =~ s/psi/ψ/;
    $designationstr =~ s/ome/ω/;
    $designationstr =~ s/V0/V/; # remove leading zero

    $ampflag = 0;
    if ($ampflagstr eq '(') {
	$ampflag = 1;
    }
    if ($ampflagstr eq '<') {
	$ampflag = 2;
    }
    if ($ampflagstr eq '>') {
	$ampflag = 3;
    }
    
    print OC $hipstr."\t".$designationstr."\t".$vclassstr."\t".$maxmagstr."\t".$ampflag."\t".$min1magstr."\t".$min2magstr."\t".$flagstr."\t".$epochstr."\t".$periodstr."\t".$mmstr."\t".$sclassstr."\n";
}
close SC;
close OC;
