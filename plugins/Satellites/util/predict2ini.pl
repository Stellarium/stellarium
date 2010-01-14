#!/usr/bin/perl
#
# Utility script to take predict TLE file and make a module.ini file from it
#
#

use strict;
use warnings;
use POSIX;

my $flag_diff = 0;
my %extra_data;
my %updated;
my $predict_file = "new.tle";
my $module_ini_file = "../module.ini";

open(P, "<$predict_file") || die "cannot open $predict_file : $!\n";
open(M, "<$module_ini_file") || die "cannot open $module_ini_file : $!\n";

my $header = "[module]\n";
my $section = "";
while(<M>)
{
	if (/^\s*\[([^\]]+)\]/) {
		$section = $1;
		next;
	}

	if ($section eq "module" || $section eq "") {
		$header .= $_;
		next;
	}

	if (/^\s*(\w+)\s*=\s*(.+)\s*/) {
		$extra_data{$section}{$1} = $2;
	}
}
close(M);

my $current_sat = "";
while(<P>)
{
	chomp;
	s/\s*$//;
	if (length($_) < 60) {
		$current_sat = $_;
		$current_sat =~ s/^\s*//;
		$current_sat =~ s/\s*$//;
		$current_sat =~ s/\s*\[[^\]]*\]$//;
		next;
	}

	if ($current_sat ne "") {
		if (defined($extra_data{$current_sat})) {
			if (/^([12]) /) {
				if ($extra_data{$current_sat}{"tle$1"} ne $_) {
					$updated{$current_sat}{"tle$1"} = $extra_data{$current_sat}{"tle$1"};
					$extra_data{$current_sat}{"tle$1"} = $_;
					$extra_data{$current_sat}{"last_updated"} = strftime("%Y-%m-%d %T %z", (localtime(time)) ) 
				}
			}
		}
	}
}
close(P);

print "\nUpdated:\n";
my @done_updates;
foreach my $id (sort keys %updated) {
	my $parts = "";
	if (defined($updated{$id}{"tle1"})) { $parts .= " tle1"; }
	if (defined($updated{$id}{"tle2"})) { $parts .= " tle2"; }

	if ($parts ne "") {
		push @done_updates, "$id: $parts";
	}
}

if ($#done_updates < 0)
{
	print "Nothing needs updating\n";
	exit(0);
}

print "The following updates will be done:\n";
foreach(@done_updates)
{
	print "$_\n";
}

print "\n";

print "Proceed, are you sure you want to overwrite? (y to proceed, anything else to quit)";
print $module_ini_file."\n";
print "y/n? > ";
my $key = getc(STDIN);
if ($key !~ /^[yY]$/) {
	print "ABORTING\n";
	exit(1);
}

print "Over-writing file\n";
open(M, ">$module_ini_file") || die "cannot open $module_ini_file for writing : $!\n";

print M $header;
foreach my $id (sort keys %extra_data)
{
	print M "[$id]\n";
	foreach my $k (sort keys %{$extra_data{$id}}) {
		if (defined($updated{$id}{$k}) && $flag_diff) {
			print M "OLD: " . "$k = " . $updated{$id}{$k} . "\n";
			print M "NEW: " . "$k = " . $extra_data{$id}{$k} . "\n";
		}
		else {
			print M "$k = " . $extra_data{$id}{$k} . "\n";
		}
	}
	print M "\n";
}
close(M);

