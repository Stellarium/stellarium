This is the Binaries directory of the SPOUT_LIBRARY part of the Spout SDK 
retrieved 2016-12-08 from https://github.com/leadedge/Spout2. We need this, 
together with the import header defined at \src\core\external\SpoutLibrary.h 
because the Spout source code is compilable only with MSVC, so we use a 
C/COM-style interface to access a precompiled .dll, this *should* work with all 
Windows compilers. 

More information: http://spout.zeal.co/ 
