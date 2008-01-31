// Define the SIP wrapper to the stelMain library.

%Module PyStellarium 0

%Import QtGui/QtGuimod.sip
%Import QtOpenGL/QtOpenGLmod.sip

class StelGLWidget : QGLWidget
{
%TypeHeaderCode
	#include "StelGLWidget.hpp"
%End
private:
	StelGLWidget(const StelGLWidget&);
};

class StelMainWindow : QMainWindow
{
%TypeHeaderCode
#include "StelMainWindow.hpp"
%End

public:
	StelMainWindow();			
	void init();
%MethodCode
	char* args[] = {"stellarium"};
	sipCpp->init(1, args);
%End
	StelGLWidget* getOpenGLWin();
private:
	StelMainWindow(const StelMainWindow&);
};

class StelCore
{
%TypeHeaderCode
	#include "StelCore.hpp"
%End

private:
	StelCore(const StelCore&);
};

class StelModule : QObject
{
%TypeHeaderCode
	#include "StelModule.hpp"
%End

public:
	enum StelModuleActionName
	{
		ACTION_DRAW,
		ACTION_UPDATE,
		ACTION_HANDLEMOUSECLICKS,
		ACTION_HANDLEMOUSEMOVES,
		ACTION_HANDLEKEYS
	};
	
	explicit StelModule();
	virtual ~StelModule();
	virtual void init() = 0;
	virtual double draw(StelCore* core) = 0;
	virtual void update(double deltaTime) = 0;
	virtual double getCallOrder(StelModuleActionName actionName) const;
private:
	StelModule(const StelModule&);
};

class StelModuleMgr
{
%TypeHeaderCode
	#include "StelModuleMgr.hpp"
%End

public:
	StelModule* getModule(const QString&);
	void registerModule(StelModule* m /TransferThis/, bool generateCallingLists=false);
private:
	StelModuleMgr(const StelModuleMgr&);
};

class StelApp
{
%TypeHeaderCode
	#include "StelApp.hpp"
%End

public:
	static StelApp& getInstance();
	StelModuleMgr& getModuleMgr();
private:
	StelApp(const StelApp&);
};
