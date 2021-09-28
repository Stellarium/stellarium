=head1 NAME

stellarium - A real-time realistic planetarium

=head1 SYNOPSIS

stellarium [B<OPTIONS>]

=head1 DESCRIPTION

Stellarium is a free GPL software which renders realistic skies in real
time with OpenGL. It is available for Linux/Unix, Windows and MacOSX.
With Stellarium, you really see what you can see with your eyes, 
binoculars or a small telescope.

=head1 OPTIONS

=over

=item B<-v, --version>

Print program name and version and exit.

=item B<-h, --help>

Print a brief synopsis of program options and exit.

=item B<-c, --config-file> I<file>

Use I<file> for the config filename instead of the default F<config.ini>.

=item B<-u, --user-dir> I<dir>

Use I<dir> instead of the default user data directory (F<$HOME/.stellarium/>
on *nix operating systems).

=item B<--verbose>

Even more diagnostic output in logfile (esp. multimedia handling).

=item B<-t, --fix-text>

May fix text rendering problems.

=item B<--scale-gui> I<scale factor>

Scaling the GUI according to scale factor.

=item B<-d, --dump-opengl-details>

Dump information about OpenGL support to logfile. Use this is you have 
graphics problems and want to send a bug report.

=item B<-f, --full-screen> I<yes|no>

With argument I<yes> or I<no> over-rides the full screen setting in the 
config file.  The setting is saved in the config-file and as such will be the
default for subsequent invocations of Stellarium.

=item B<--screenshot-dir> I<dir>

Set the directory into which screenshots will be saved to I<dir>, 
instead of the default (which is $HOME on *nix operating systems).

=item B<--startup-script> I<script>

Specify name of startup script.

=item B<--home-planet> I<planet-name>

Specify observer planet. I<planet-name> is an English name, and should 
refer to an object defined in the F<ssystem.ini> file.

=item B<--altitude> I<alt>

Specify the initial observer altitude, where I<alt> is the altitude in 
meters.

=item B<--longitude> I<lon>

Specify the initial observer longitude, where I<lon> is the longitude.
The format is illustrated by this example: +4d16'12" which refers
to 4 degrees, 16 minutes and 12 arc seconds East.  Westerly longitudes 
should be prefixed with C<->.

=item B<--latitude> I<lat>

Specify the initial observer latitude, where I<lat> is the latitude.
The format is illustrated by this example: +53d58'16.65" which refers
to 53 degrees, 58 minutes and 16.65 arc seconds North.  Southerly 
latitudes should be prefixed with C<->.

=item B<--list-landscapes>

Print a list of landscape names and exit.

=item B<--landscape> I<name>

Start Stellarium using landscape I<name>.
Refer to B<--list-landscapes> for possible names.

=item B<--sky-date> I<date>

Specify sky date in format yyyymmdd.

=item B<--sky-time> I<time>

Specify sky time in format hh:mm:ss.

=item B<--fov> I<fov>

Specify the field of view (I<fov> degrees).

=item B<--projection-type> I<p>

Specify projection type, I<p>.  Permitted values of I<p> are: equalarea, 
stereographic, fisheye, cylinder, mercator, perspective, and orthographic.

=item B<--restore-defaults>

Delete existing config.ini and use defaults.

=back

=head1 RETURN VALUE

=over

=item 0

Completed successfully.

=item not 0

Some sort of error.

=back

=head1 FILES

Note: file locations on non-*nix operating systems (include OSX) may vary.  
Please refer to the Stellarium User Guide for more details, as well
as information on how to customise the Stellarium data files.

=over

=item F<${INSTALL_DATADIR}/>

This is the Installation Data Directory set at compile-time.

=item F<$HOME/.stellarium/>

This is the User Data Directory, which may be over-ridden using command line 
option B<-u>.  It contains the user's settings, extra landscapes, scripts, and can 
also be used to over-ride data files which are provided with the default 
install.

=item F<$HOME/.stellarium/config.ini>

The default main configuration file is F<config.ini>. Refer to B<-c> above to
use a different filename and to B<-u> to use a different User Data Directory.

=item F<$HOME/>

The default screenshot directory. Refer to B<--screenshot-dir> to use a
different path.

=back

=head1 SEE ALSO

celestia(1).

=head1 NOTES

Sources of more information:

=over

=item Websites

Main website: L<http://stellarium.org/>

Wiki: L<http://stellarium.org/wiki/>

Forums: L<http://sourceforge.net/projects/stellarium/forums>

Downloads: L<http://sourceforge.net/projects/stellarium/files/>

Support Requests: L<https://answers.launchpad.net/stellarium>

Bug Tracker: L<https://bugs.launchpad.net/stellarium>

=item The Stellarium User Guide

Visit the downloads page to get a PDF copy of the Stellarium User Guide.

=back

=head1 BUGS

Please report bugs using the bug tracker link in the NOTES section 
of this page.

=head1 AUTHOR

Fabien Chereau, Rob Spearman, Johan Meuris, Matthew Gates, 
Johannes Gajdosik, Nigel Kerr, Andras Mohari, Bogdan Marinov, 
Timothy Reaves, Mike Storm, Diego Marcos, Guillaume Chereau, 
Alexander Wolf, Georg Zotti

x14817

=cut

