#!/usr/bin/perl
#
# Program to take a set of header files and generate DLL export definitions

# Special exports to ignore for this platform

while ( ($file = shift(@ARGV)) ) {
	if ( ! defined(open(FILE, $file)) ) {
		warn "Couldn't open $file: $!\n";
		next;
	}
	$printed_header = 0;
	$file =~ s,.*/,,;
	while (<FILE>) {
		if ( / DECLSPEC.* SDLCALL ([^\s\(]+)/ ) {
			if ( not $exclude{$1} ) {
				print "\t$1\n";
			}
		}
	}
	close(FILE);
}

# Special exports to include for this platform
print "\tSDL_putenv\n";
print "\tSDL_getenv\n";
print "\tSDL_qsort\n";
print "\tSDL_revcpy\n";
print "\tSDL_strlcpy\n";
print "\tSDL_strlcat\n";
print "\tSDL_strdup\n";
print "\tSDL_strrev\n";
print "\tSDL_strupr\n";
print "\tSDL_strlwr\n";
print "\tSDL_ltoa\n";
print "\tSDL_ultoa\n";
print "\tSDL_strcasecmp\n";
print "\tSDL_strncasecmp\n";
print "\tSDL_snprintf\n";
print "\tSDL_vsnprintf\n";
print "\tSDL_iconv\n";
print "\tSDL_iconv_string\n";
print "\tSDL_InitQuickDraw\n";
