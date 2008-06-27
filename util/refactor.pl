#!/usr/bin/perl  

use warnings;
use strict;
use Getopt::Long;
use Term::ANSIColor;

my $flg_verbose = 0;
my $flg_yes = 0;
GetOptions(
	"verbose" => \$flg_verbose,
	"yes"     => \$flg_yes,
	"help"    => sub { lusage(0); },
) || usage(1);

# we expect three or more parameters (search pattern, replacement, file(s))
if ($#ARGV<2) { usage(2);}

if ($ARGV[1] eq ".") {
	# if the replacement is just ".", guess what the replacement is 
	# from the search pattern by transforming it to camelCase from 
	# lc_underscore_style
	my $s = shift || usage(1);
	my $r = autoProcess($s);
	shift;
	refactorString($s, $r, @ARGV);
}
else {
	# or if the second argument is not ".", use it as the replacemnt
	my $s = shift || usage(1);
	my $r = shift || usage(1);
	refactorString($s, $r, @ARGV);
}

exit(0);

# dump usage message and exit.  message is maded from POD tags in this file
sub usage {
	system "pod2usage < $0"; 
	exit $_[0] || 0;
}

# longer usage message
sub lusage {
	system "pod2usage -verbose 1 < $0"; 
	exit $_[0] || 0;
}

# guess a replacement string from the search pattern
# e.g. variable_name -> variableName
sub autoProcess {
	my $s = shift;
	my $w = $s;
	$w =~ s/_([a-z])/uc($1)/ge;
	$w =~ s/_//g;
	return $w;
}

# do all the work
# args: search-pattern; replacement-string; file; [file]; ...
sub refactorString {
	# sanity checks on parameters
	my $s = shift || die "empty search string not good\n";
	my $r = shift || die "empty replacement string not good\n";
	my @allFileList = @_;

	if ($s eq "") { die "empty string not good\n"; }

	if ($s eq $r) {
		die "re-factored string is the same as the original $s\n";
	}

	if (!$flg_yes) {
		print "Will replace $s with $r:\n";
	}
	my %patchFileList;
	my $textRed = color 'red';
	my $textReset = color 'reset';

	# read all the files and build a list of the ones which contain the
	# search pattern.  the list is the keys in the hash %patchFileList
	# also print previews of the replacements, with the text to be
	# replaced in red.
	foreach my $f (@allFileList) {
		if ($f =~ /~$/) { warn "SKIPPING backup file: $f"; next; }
		if ( ! open(F, "<$f") ) { 
			warn "cannot open $f : SKIPPING\n";
		}
		else {
			while(<F>) {
				if (s/($s)/$textRed$1$textReset/g) { 
					$patchFileList{$f} = 1; 
					if (!$flg_yes || $flg_verbose) {
						print "$f\[$.\]: $_";
					}
				}
			}
			close(F);
		}
	}

	# prompt user if they want to do the replacement
	my $do_it = $flg_yes;
	if ($flg_yes) { 
		$do_it = 1;
	}
	else {
		print "\nAccept changes (y/n)> ";
		my $res = <STDIN>;
		chomp $res;
		if (lc($res) eq "y") {
			$do_it = 1;
		}
	}

	if ($do_it)
	{
		# iterate over the files identified in the first scan, doing the
		# replacement.  The original files will be renamed with a ~ suffix
		# and the original named file will be re-created with the changes.
		my $updatedLines = 0;
		my $updatedFiles = 0;
		foreach my $f (keys %patchFileList) {
			my $orig = "$f" . "~";
			rename "$f", "$orig"  || die "cannot rename file $f to $orig : $!\n";
			open(ORIG, "<$orig") || die "cannot open $orig for reading : $!\n";
			open(NEWF, ">$f")  || die "cannot open $f for writing : $!\n";
			while(<ORIG>)
			{
				if (s/$s/$r/g) { $updatedLines ++; }
				print NEWF $_;
			}
			close(ORIG);
			close(NEWF);
			$updatedFiles++;
			if ($flg_verbose) {
				print "patched $f (backup in $orig)\n";
			}
		}

		if ($flg_verbose) {
			print "Done updated $updatedLines lines in $updatedFiles files\n";
		}
	}
	elsif ($flg_verbose) {
		print "not patching\n";
	}
	exit;
}

__END__


=head1 NAME 

refactor - replace patterns in bunch of files with a nice-ish preview

=head1 SYNOPSIS

refactor [options] pattern {replacement|.} file [file] ...

=head1 DESCRIPTION

Replaces Perl-RE I<pattern> with I<replacement> in all specified files.  First the
program prints out what the replacement is, and then lists lines in files where
a replacement will be made, highlighing in red the text which will be replaced.

If I<replacement> is a period ("."), the replacement will be guesses at according to
Stellarium's variable coding standards (e.g. camel_case => camelCase).

=head1 OPTIONS

=over

=item B<--help>

Print the command line syntax an option details.

=item B<--verbose>

Print extra information about what is going on.

=item B<--yes>

Don't prompt for confirmation - just do it.

=head1 ENVIRONMENT

N/A

=head1 FILES

When refactor is run, all modified files are backed up into a file with the same name
as the original with a ~ suffix.  refactor will not opereate on files with this name.

=head1 LICENSE

refactor is released under the GNU GPL (version 2, June 1991).  A
copy of the license should have been provided in the distribution of
the software in a file called "LICENSE".  If you can't find this, then
try here: http://www.gnu.org/copyleft/gpl.html

=head1 AUTHOR

Matthew Gates <matthew@porpoisehead.net>

http://porpoisehead.net/

=head1 CHANGELOG

=over

=item Date:2008-06-26 Created, Author MNG

Original version.

=back

=head1 BUGS

Please report bugs to the author.

=head1 SEE ALSO

sed, grep and friends

=cut

