# STELLARIUM INSTALLATION INSTRUCTIONS

## BINARY INSTALLATION

Most users will prefer using precompiled binary packages:

- **WINDOWS USERS:**
run _setup.exe_ and follow the instructions.

- **MACOSX USERS:**
run _stellarium.dmg_

- **LINUX USERS:**
Look for the binary package matching your distribution.

## Special instructions for COMPILATION on Linux and other unix-likes:

See the wiki:
http://stellarium.org/wiki/index.php/Compilation_on_Linux

## Special instructions for COMPILATION on MACOSX with Xcode

See the wiki:
http://stellarium.org/wiki/index.php/Compilation_on_Macosx

## Special instructions for COMPILATION on WINDOWS (XP) with MinGW

See the wiki:
http://stellarium.org/wiki/index.php/Windows_Build_Instructions


## COMPILATION (and modification) from the SVN sources

You can get the latest SVN snapshot (from sourceforge). However with this SVN
version no correct behaviour is guaranteed. It is mainly intended for use by
developers.
You can browse the SVN tree from

http://stellarium.svn.sourceforge.net/viewvc/stellarium/

For build instructions see these pages on the Stellarium wiki:

http://stellarium.org/wiki/index.php/Compilation_on_Linux
http://stellarium.org/wiki/index.php/Windows_Build_Instructions
http://stellarium.org/wiki/index.php/Compilation_on_Macosx

You can have a look at the src/ directory where you will find the source
files.  Edit whatever you want in it and when your new great feature is done
you will need to share it with the community of Stellarium developers.

At this point, an official developer just have to type svn commit to update
the repository version. But as you are not an official developer (yet!) you
will need to create a patch file which will contain all the changes you did
on the source code.  

```
cd directory-you-want-to-diff
svn diff > mypatch.diff
```

You can now submit mypatch.diff on the sourceforge Stellarium page in the
patches section (http://sourceforge.net/tracker/?group_id=48857&atid=454375)
with a clear comment on what is the patch doing.

A project member will then have a look at it and decide whether the patch is
accepted or rejected for integration into Stellarium.

You might also want to subscribe to the stellarium-pubdevel mailing list to
keep up with the latest discussions about Stellarium development.  You
can subscribe here:

https://lists.sourceforge.net/lists/listinfo/stellarium-pubdevel
