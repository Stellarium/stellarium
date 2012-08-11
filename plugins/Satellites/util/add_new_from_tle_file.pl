#!/usr/bin/perl

# This is a helper script of download_tle_find_new.sh
# It accepts a TLE list, checks if the satellites in it already are listed in
# satellites.json, and if not, outputs appropriately formatted JSON entries.
# The new satellites are added to the groups passed as parameters.

#my $groups = "\"scientific\", \"weather\", \"non-operational\"";
map { s/^/"/; s/$/"/; } @ARGV;
my $groups = join(", ", @ARGV);

my %ignore;

open(SAT, "<satellites.json") || die "Cannot open existing satellites.json file";
while(<SAT>) {
	chomp;
	if (/^\s*\"([^"]+)":\s*$/) {
		$ignore{$1} = 1;
	}
}

my(%h);

my $count = 0;

while(<STDIN>) {
	chomp;
	s/\s+$//;
	$_ =~ /^2\s(\d+)\s/;
	$h{'id'} = $1; 
	if (/^1/) { $h{'tle1'} = $_; }
	elsif (/^2/) {
		$h{'tle2'} = $_;
		out(%h);
		foreach my $k (keys %h) {delete $h{$k};}
	}
	else { 
		s/\s*\[[^\]]+\]\s*$//; 
		$h{'name'} = $_; 
	}
}

print STDERR "Added " . $count . " new in [" . $groups . "]\n";

sub out {
	if (defined($ignore{$h{'id'}})) { 
		print STDERR "Ignoring " . $h{'id'} . "/" . $h{'name'} . "\n";
		return;
	}

	printf "\t\t\"%s\":\n", $h{'id'};
	print  "\t\t{\n";
	print  "\t\t\t\"groups\": [$groups],\n";
	print  "\t\t\t\"orbitVisible\": false,\n";
	printf "\t\t\t\"name\": \"%s\",\n", $h{'name'};
	printf "\t\t\t\"tle1\": \"%s\",\n", $h{'tle1'};
	printf "\t\t\t\"tle2\": \"%s\",\n", $h{'tle2'};
	print  "\t\t\t\"visible\": false\n";
	print  "\t\t},\n";

	$count++;
}
