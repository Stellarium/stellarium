// Define the SIP wrapper to the stelMain library.

%Module stelapp 0


class StelAppQt4
{

%TypeHeaderCode
#include <StelAppQt4.hpp>
%End

public:
	StelAppQt4();
%MethodCode
		char* args[] = {"stellarium"};
		sipCpp = new StelAppQt4(1, args);
%End
			
	void startMainLoop();
	
private:	
	StelAppQt4(const StelAppQt4&);
};

