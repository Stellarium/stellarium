#!/usr/bin/perl
#
# read a module.ini file from the early version of the plugin, which
# contain satellite data, and output a json formatted version of the
# satellite data.
#
# re-direct the output into the satelites.json file, and then
# edit the module.ini file to remove all sections except the
# [module] section.
#

use strict;
use warnings;

my %data;
my $sec = "";

while(<>)
{
	s/^\s*//;
	s/\s*$//;
	if (/^\[([^\]]+)\]$/)
	{
		$sec = $1;
		next;
	}

	if (/^(\w+)\s*=\s*(.+)$/ && $sec ne "" && $sec ne "module")
	{
		$data{$sec}{$1} = $2;
	}
}

my $indent=0;
my $groupdata="";
jo("{\n");
$indent++;
jo('"shortName": "satellite orbital data",' . "\n");
jo('"creator": "ini2json.pl",' . "\n");
jo('"hintColor": [0.0,0.5,0.5],' . "\n");
jo('"satellites":' . "\n");
jo("{\n");
$indent++;
jogroup();
foreach my $designation (sort keys %data)
{
	jogroup(",");
	jo("\"$designation\": \n");
	jo("{\n");
	$indent++;
	# keys which are simple string values
	foreach my $textkey (qw(description tle1 tle2)) {
		if (defined($data{$designation}{$textkey})) {
			jo('"' . $textkey . '": "' . $data{$designation}{$textkey} . '",' . "\n");
		}
	}

	# visible is bool so it doesn't need quotes...
	if (defined($data{$designation}{'visible'})) {
		jo('"visible": ' . $data{$designation}{'visible'} . ",\n");
	}

	# groups are an array of strings
	if (defined($data{$designation}{'groups'})) {
		my @groups = map {$_="\"$_\""} split(/\s*,\s*/, $data{$designation}{'groups'});
		jo('"groups": [' . join(",", @groups) . "],\n");
	}

	# hint_colors are an array of floats
	if (defined($data{$designation}{'hint_color'})) {
		my @groups = split(/\s*,\s*/, $data{$designation}{'hint_color'});
		jo('"hintColor": [' . join(",", @groups) . "],\n");
	}

	my @comms;
	foreach my $k (keys %{$data{$designation}})
	{
		if ($k =~ /^comm\d+$/)
		{
			@comms = (@comms, $data{$designation}{$k});
		}
	}

	if ($#comms >= 0)
	{
		jo("\"comms\":\n");
		jo("[\n");
		$indent++;
		foreach my $comm (@comms)
		{
			my @commparts = split(/\s*\|\s*/, $comm);
			jo("{\n");
			$indent++;
                        jo('"frequency": ' . $commparts[0] . ",\n");
			if (defined($commparts[1])) { if ($commparts[1] ne "") {
				jo('"modulation": "' . ($commparts[1] || "") . "\",\n");
			}}
			if (defined($commparts[2])) { if ($commparts[2] ne "") {
			jo('"description": "' . ($commparts[2] || "") . "\",\n");
			}}
			jogroup();
			$indent--;
			jo("},\n");
		}
		jogroup();
		$indent--;
		jo("],\n");
	}
	jogroup();
	$indent--;
	jo("},\n");
}
jogroup();
$indent--;
jo("}\n");
$indent--;
jo("}\n");
jogroup();
print "\n";

sub jo {
	$groupdata .= "\t" x $indent . $_[0];
}

sub jogroup {
	if (!defined($_[0])) { $groupdata =~ s/,$//; }
	print $groupdata;
	$groupdata = "";
}

#!/usr/bin/perl
#
# read a module.ini file from the early version of the plugin, which
# contain satellite data, and output a json formatted version of the
# satellite data.
#
# re-direct the output into the satelites.json file, and then
# edit the module.ini file to remove all sections except the
# [module] section.
#

use strict;
use warnings;

my %data;
my $sec = "";

while(<>)
{
	s/^\s*//;
	s/\s*$//;
	if (/^\[([^\]]+)\]$/)
	{
		$sec = $1;
		next;
	}

	if (/^(\w+)\s*=\s*(.+)$/ && $sec ne "" && $sec ne "module")
	{
		$data{$sec}{$1} = $2;
	}
}

my $indent=0;
my $groupdata="";
jo("{\n");
$indent++;
jo('"shortName": "satellite orbital data",' . "\n");
jo('"creator": "ini2json.pl",' . "\n");
jo('"hintColor": [0.0,0.5,0.5],' . "\n");
jo('"satellites":' . "\n");
jo("{\n");
$indent++;
jogroup();
foreach my $designation (sort keys %data)
{
	jogroup(",");
	jo("\"$designation\": \n");
	jo("{\n");
	$indent++;
	# keys which are simple string values
	foreach my $textkey (qw(description tle1 tle2)) {
		if (defined($data{$designation}{$textkey})) {
			jo('"' . $textkey . '": "' . $data{$designation}{$textkey} . '",' . "\n");
		}
	}

	# visible is bool so it doesn't need quotes...
	if (defined($data{$designation}{'visible'})) {
		jo('"visible": ' . $data{$designation}{'visible'} . ",\n");
	}

	# groups are an array of strings
	if (defined($data{$designation}{'groups'})) {
		my @groups = map {$_="\"$_\""} split(/\s*,\s*/, $data{$designation}{'groups'});
		jo('"groups": [' . join(",", @groups) . "],\n");
	}

	# hint_colors are an array of floats
	if (defined($data{$designation}{'hint_color'})) {
		my @groups = split(/\s*,\s*/, $data{$designation}{'hint_color'});
		jo('"hintColor": [' . join(",", @groups) . "],\n");
	}

	my @comms;
	foreach my $k (keys %{$data{$designation}})
	{
		if ($k =~ /^comm\d+$/)
		{
			@comms = (@comms, $data{$designation}{$k});
		}
	}

	if ($#comms >= 0)
	{
		jo("\"comms\":\n");
		jo("[\n");
		$indent++;
		foreach my $comm (@comms)
		{
			my @commparts = split(/\s*\|\s*/, $comm);
			jo("{\n");
			$indent++;
                        jo('"frequency": ' . $commparts[0] . ",\n");
			if (defined($commparts[1])) { if ($commparts[1] ne "") {
				jo('"modulation": "' . ($commparts[1] || "") . "\",\n");
			}}
			if (defined($commparts[2])) { if ($commparts[2] ne "") {
			jo('"description": "' . ($commparts[2] || "") . "\",\n");
			}}
			jogroup();
			$indent--;
			jo("},\n");
		}
		jogroup();
		$indent--;
		jo("],\n");
	}
	jogroup();
	$indent--;
	jo("},\n");
}
jogroup();
$indent--;
jo("}\n");
$indent--;
jo("}\n");
jogroup();
print "\n";

sub jo {
	$groupdata .= "\t" x $indent . $_[0];
}

sub jogroup {
	if (!defined($_[0])) { $groupdata =~ s/,$//; }
	print $groupdata;
	$groupdata = "";
}

