
#include "StelMacosxDirs.hpp"
#include <Carbon/Carbon.h>

/*
  Make a utf-8 encoded string as an array of char in outCString, using
  Core Foundation calls on the Macosx.  This function found several places
  on the net summer of 2007.
*/
OSStatus CreateCStringUTF8(CFStringRef inString, char* &outCString)
{
	CFIndex maxBytesNeeded =
		CFStringGetMaximumSizeForEncoding(
			CFStringGetLength(inString),
			kCFStringEncodingUTF8);
	CFIndex bufferSize = 1 + maxBytesNeeded;

	outCString = (char*) malloc(bufferSize);

	OSStatus err = (outCString == nil ? (OSStatus)memFullErr : (OSStatus)noErr);

	if (err == noErr)
	{
		if (!CFStringGetCString(inString, outCString, bufferSize,
		                        kCFStringEncodingUTF8))
		{
			free(outCString);
			outCString = nil;
			err = coreFoundationUnknownErr;
		}
	}

	return err;
}

QString StelMacosxDirs::getApplicationDirectory()
{
	FSRef appBundleRef;
	ProcessSerialNumber psn = {0, kCurrentProcess};

	if (GetProcessBundleLocation(&psn, &appBundleRef) == noErr)
	{
		CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &appBundleRef);
		CFStringRef cfString = NULL;
		char * cstr;

		if (url != NULL)
		{
			cfString = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
			CFRelease(url);
			if (CreateCStringUTF8(cfString, cstr) == noErr)
			{
				QString res(cstr);
				free(cstr);
				return res;
			}
		}
	}
	return QString();
}

QString StelMacosxDirs::getApplicationResourcesDirectory()
{
	return StelMacosxDirs::getApplicationDirectory().append("/Contents/Resources");
}

