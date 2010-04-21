#!/usr/bin/perl -n -a
#
# Quick sanity check for constellationship.fab files
#


$c = $F[0];
$n = $F[1]; 
@F = @F[2..$#F];

print "\n";
for($i=0; $i<$n; $i++) { 
	$a=$F[0]; 
	$b=$F[1]; 
	@F=@F[2..$#F]; 
	if ($a == $b) {
		$diag = "ERROR - same star at start and end of line";
	}
	else {
		$diag = "OK";
	}
	printf "%-3s %3d %-10s -> %-10s %s\n", $c, $i, $a, $b, $diag; 
}
