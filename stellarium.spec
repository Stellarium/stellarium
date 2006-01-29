Name: stellarium
Version: 0.8.0
Release: 1
Summary: Astronomical Sky Simulator
Copyright: GPL
Group: Applications/Scientific
Source: %{name}-%{version}.tar.gz
URL: http://stellarium.sourceforge.net
Packager: Kipp Cannon
# The following dependancies are the names of the appropriate packages on a
# RedHat (8.0) system.  This may not fit nicely into other distributions.
Requires: glibc
Requires: libgcc
Requires: libstdc
Requires: SDL
Requires: XFree86-libs
Requires: XFree86-Mesa-libGL
Requires: XFree86-Mesa-libGLU
BuildRequires: fileutils
BuildRequires: glibc-devel
BuildRequires: libstdc-devel
BuildRequires: SDL-devel
BuildRequires: XFree86-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-root
# Package is not, really, relocatable (uses a compiled-in path to look for
# data files).
#Prefix: %{_prefix}
%description
Stellarium is a free software available for Windows, Linux/Unix and MacOSX.
It renders 3D photo-realistic skies in real time.  With stellarium, you
really see what you can see with your eyes, binoculars or a small
telescope.


%prep
%setup -q


%build
%configure
%{__make}


%install
%makeinstall


%clean
[ ${RPM_BUILD_ROOT} != "/" ] && rm -Rf ${RPM_BUILD_ROOT}
rm -Rf ${RPM_BUILD_DIR}/%{name}-%{version}


%files
%defattr(-,root,root)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%{_bindir}/*
%{_datadir}/%{name}
%{_datadir}/applications/*
%{_datadir}/pixmaps/*
