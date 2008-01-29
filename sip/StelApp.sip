// Define the SIP wrapper to the stelMain library.

%Module pystellarium 0

class StelMainWindow
{

%TypeHeaderCode
#include "StelMainWindow.hpp"
%End

public:
	StelMainWindow();			
	void init()
	%MethodCode
		char* args[] = {"stellarium"};
		sipCpp->init(1, args);
	%End
};

