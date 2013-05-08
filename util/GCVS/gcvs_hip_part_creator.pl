#!/usr/bin/perl

$SC = "./vcat-hip.dat"; 	# Source
$OC = "./gcvs_hip_part.dat"; 	# Destination
$Hdr = "./header.txt";

open(HDR, "$Hdr");
@bf = <HDR>;
close HDR;

open (OC, ">$OC");
for ($i=0;$i<scalar(@bf);$i++) {
    print OC $bf[$i];
}

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
    
    $hipstr =~ s/(\s+)//gi;
    $designationstr =~ s/(\s+)/ /gi;
    $vclassstr =~ s/(\s+)//gi;
    $maxmagstr =~ s/(\s+)//gi;
    $minmagstr =~ s/(\s+)//gi;
    $epochstr =~ s/(\s+)//gi;
    $periodstr =~ s/(\s+)//gi;
    $mmstr =~ s/(\s+)//gi;
    $sclassstr =~ s/(\s+)//gi;
    
    $designationstr =~ s/alf/α/;
    $designationstr =~ s/bet/β/;
    $designationstr =~ s/gam/γ/;
    $designationstr =~ s/del/δ/;
    $designationstr =~ s/eps/ε/;
    $designationstr =~ s/zet/ζ/;
    $designationstr =~ s/eta/η/;
    $designationstr =~ s/the/θ/;
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
    $designationstr =~ s/khi/χ/;
    $designationstr =~ s/psi/ψ/;
    $designationstr =~ s/ome/ω/;

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

    
    print OC $hipstr."\t".$designationstr."\t".$vclassstr."\t".$maxmagstr."\t".$ampflag."\t".$minmagstr."\t".$flag."\t".$epochstr."\t".$periodstr."\t".$mmstr."\t".$sclassstr."\n";
}
close SC;
close OC;
