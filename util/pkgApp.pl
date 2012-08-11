#!/usr/bin/perl

use strict;
use Cwd;

my $inmt = "/usr/bin/install_name_tool";
my $ch_inmt = qq{$inmt -change \%s \%s \%s};
my $id_inmt = qq{$inmt -id \%s \%s};

my $otool = qq{/usr/bin/otool -L \%s};

my $appdir = shift(@ARGV);
chdir $appdir;
my $main_executable = shift(@ARGV);
my $frameworks_dir = shift(@ARGV);
my $current_arch = `/usr/bin/uname -m`;
chomp($current_arch);

if ( ! -e "$frameworks_dir/$current_arch" ) {
    `mkdir -p $frameworks_dir/$current_arch`;
}

&recurse( $main_executable, $frameworks_dir, $current_arch );

sub recurse {
    my($main_executable, $frameworks_dir, $current_arch) = @_;

    my $cmd1 = sprintf($otool, $main_executable);
    my($app, @names) = `$cmd1`;

    my $name;
  NAME_LOOP:
    foreach $name ( @names ) {
	chomp($name);
	$name =~ s,^\s*,,;
	$name =~ s,\s*\(compa.+$,,;

	## we've already dealt with this name on this executable, ie, itself
	if ( index($name, $main_executable) >= 0 ||
	     index($main_executable, $name) >= 0 ) {
	    next NAME_LOOP;
	}

	## leave sys libraries alone and dont include them
	if ( $name =~ m,^(/System/Library|/usr/lib|\@executable_name), &&
	     $name !~ m,^(/usr/lib/libiconv), && $name !~ m,^(/usr/lib/libintl),){ 
	    next NAME_LOOP; 
	}

	## a rooted Framework
	if ( $name =~ m,([^/]+\.framework)/(\S+)$, ) {
	    my $fwname = $1;
	    my $binary = $2;

	    my $absname = &locateFramework($fwname);
	    my $arch = &architecture("$absname/$binary");

	    if ( $arch eq $current_arch || $arch eq 'fat' ) {
		my $relPath = "\@executable_path/../Frameworks/$fwname/$binary";
		my $fwPath = "$frameworks_dir/$fwname/$binary";

		my $not_existed = 1;
		if ( ! -e $fwPath ) {
		    my $c = "cp -RP -p $absname $frameworks_dir/$fwname";
		    `$c`;
		} else {
		    $not_existed = 0;
		}

		my $c = sprintf($id_inmt, $relPath, $fwPath);
		`$c`;
		$c = sprintf($ch_inmt, $name, $relPath, $main_executable);
		`$c`;

		if ( $not_existed ) {
		    &recurse($fwPath, $frameworks_dir, $current_arch);
		}
	    } else {
		warn qq{$0: [1] for $main_executable: what to do about $absname being $arch!!!!!\n};
	    }
	    next NAME_LOOP;
	}

	## an unrooted Framework
	if ( $name =~ m,^([^/]+\.framework)/(\S+)$, ) {
	    my $fwname = $1;
	    my $binary = $2;

	    my $absname = &locateFramework($fwname);
	    my $arch = &architecture("$absname/$binary");

	    if ( $arch eq $current_arch || $arch eq 'fat' ) {
		my $relPath = "\@executable_path/../Frameworks/$fwname/$binary";
		my $fwPath = "$frameworks_dir/$fwname/$binary";

		my $not_existed = 1;
		if ( ! -e $fwPath ) {
		    my $c = "cp -RP -p $absname $frameworks_dir/$fwname";
		    `$c`;
		} else {
		    $not_existed = 0;
		}

		my $c = sprintf($id_inmt, $relPath, $fwPath);
		`$c`;
		$c = sprintf($ch_inmt, $name, $relPath, $main_executable);
		`$c`;

		if ( $not_existed ) {
		    &recurse($fwPath, $frameworks_dir, $current_arch);
		}
	    } else {
		warn qq{$0: [1] for $main_executable: what to do about $absname being $arch!!!!!\n};
	    }
	    next NAME_LOOP;
	}

	## a rooted dylib
	if ( $name =~ m,^/.+?([^/]+\.dylib)$, ) {
	    my $basename = $1;
	    my $absname = $name;

	    my $arch = &architecture($absname);

	    if ( $arch eq $current_arch || $arch eq 'fat' ) {
		my $relPath = "\@executable_path/../Frameworks/$current_arch/$basename";
		my $fwPath = "$frameworks_dir/$current_arch/$basename";

		my $not_existed = 1;
		if ( ! -e $fwPath ) {
		    my $c = "cp -P $absname $frameworks_dir/$current_arch";
		    `$c`;
		} else {
		    $not_existed = 0;
		}

		my $c = sprintf($id_inmt, $relPath, $fwPath);
		`$c`;
		$c = sprintf($ch_inmt, $absname, $relPath, $main_executable);
		`$c`;

		if ( $not_existed ) {
		    &recurse($fwPath, $frameworks_dir, $current_arch);
		}
	    } else {
		warn qq{$0: [2] for $main_executable: what to do about $name being $arch!!!\n};
	    }
	    next NAME_LOOP;
	}

	## an unrooted dylib; we only have one of these for Stellarium
	## right now, libstelMain.dylib, and its a special case.
	if ( $name =~ m,libstelMain.dylib, ) {

	    if ( $name =~ m/\@executable/ ) {
		# we've been here already
		next NAME_LOOP;
	    }

	    my $basename = $name;
	    my $absname = "MacOS/" . $name;

	    my $arch = &architecture($absname);

	    if ( $arch eq $current_arch || $arch eq 'fat') {
		my $relPath = "\@executable_path/$basename";

		my $c = sprintf($id_inmt, $relPath, $absname);
		`$c`;
		$c = sprintf($ch_inmt, $name, $relPath, $main_executable);
		`$c`;

		&recurse($absname, $frameworks_dir, $current_arch);
	    } else {
		warn qq{$0: [4] for $main_executable: what to do about $name being '$arch'!!!\n};
	    }
	    next NAME_LOOP;
	}
	

	## something else?
	warn qq{$0: [3] for $main_executable: what to do with $name?!?!?\n};
    }
}


sub architecture {
    my $file = shift;
    my(@output) = `/usr/bin/file $file`;

    if ( grep(m/^${file}: symbolic link to ([^\']+)/, @output) ) {
	my($r, @junk) = grep(m/^${file}: symbolic link to ([^\']+)/, @output);
	$r =~ m,^(.+)/([^/]+.dylib): symbolic link to ([^\']+),;
	my $root = $1;
	my $base = $2;
	my $target = $3;

	if ( $target =~ m,^/, ) {
	    return &architecture("$target");
	} else {
	    return &architecture("$root/$target");
	}
    }

    my $retval = undef;
    # file with 2 architectures

    if ( grep(m/(universal binary|fat file) with [0-9] architectures/i, @output)) {
	$retval =  'fat';
    }
    elsif ( grep(m/ppc/, @output) ) {
	$retval =  'ppc';
    }
    elsif ( grep(m/i386/, @output) ) {
	$retval =  'i386';
    }
    elsif ( grep(m/x86_64/, @output) ) {
	$retval =  'x86_64';
    }
    # warn qq{$0: $file isa '$retval' arch\n};
    return $retval;
}

sub locateFramework {
    my $fname = shift;
    my $lib;
    foreach $lib ( '~/Library/Frameworks', '/Library/Frameworks', '/usr/local/Trolltech/Qt-4.7.1/lib' ) {
	if ( -e "$lib/$fname" ) {
	    return "$lib/$fname";
	}
    }
    warn qq{$0: couldnt find $fname!!!!\n};
    return undef;
}
