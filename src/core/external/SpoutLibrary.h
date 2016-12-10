//
//	SpoutLibrary.dll
//
//	Spout SDK dll compatible with any C++ compiler
//

#include <windows.h>
#include <GL/GL.h>

#define SPOUTLIBRARY_EXPORTS // defined for this DLL. The application imports rather than exports

#ifdef SPOUTLIBRARY_EXPORTS
#define SPOUTAPI __declspec(dllexport)
#else
#define SPOUTAPI __declspec(dllimport)
#endif

////////////////////////////////////////////////////////////////////////////////
//
// COM-Like abstract interface.
// This interface doesn't require __declspec(dllexport/dllimport) specifier.
// Method calls are dispatched via virtual table.
// Any C++ compiler can use it.
// Instances are obtained via factory function.
//
struct SPOUTLIBRARY
{
	// Sender
	virtual bool CreateSender(const char *Sendername, unsigned int width, unsigned int height, DWORD dwFormat = 0) = 0;
	virtual void ReleaseSender(DWORD dwMsec = 0) = 0;
	virtual bool UpdateSender(const char* Sendername, unsigned int width, unsigned int height) = 0;
	virtual bool SendTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, bool bInvert = true, GLuint HostFBO = 0) = 0;
	virtual bool SendImage(const unsigned char* pixels, unsigned int width, unsigned int height, GLenum glFormat = GL_RGBA, bool bInvert=false) = 0;

	// Receiver
	virtual bool CreateReceiver(char* Sendername, unsigned int &width, unsigned int &height, bool bUseActive = false) = 0;
	virtual void ReleaseReceiver() = 0;
	virtual bool ReceiveTexture(char* Sendername, unsigned int &width, unsigned int &height, GLuint TextureID = 0, GLuint TextureTarget = 0, bool bInvert = false, GLuint HostFBO = 0) = 0;
	virtual bool ReceiveImage(char* Sendername, unsigned int &width, unsigned int &height, unsigned char* pixels, GLenum glFormat = GL_RGBA, bool bInvert = false, GLuint HostFBO=0) = 0;
	virtual bool CheckReceiver(char* Sendername, unsigned int &width, unsigned int &height, bool &bConnected) = 0;
	virtual bool GetImageSize(char* sendername, unsigned int &width, unsigned int &height, bool &bMemoryMode) = 0;	

	virtual bool BindSharedTexture() = 0;
	virtual bool UnBindSharedTexture() = 0;
	
	virtual bool DrawSharedTexture(float max_x = 1.0, float max_y = 1.0, float aspect = 1.0, bool bInvert = true) = 0;
	virtual bool DrawToSharedTexture(GLuint TextureID, GLuint TextureTarget, unsigned int width, unsigned int height, float max_x = 1.0, float max_y = 1.0, float aspect = 1.0, bool bInvert = false, GLuint HostFBO = 0) = 0;

	virtual int  GetSenderCount() = 0;
	virtual bool GetSenderName(int index, char* sendername, int MaxSize = 256) = 0;
	virtual bool GetSenderInfo(const char* sendername, unsigned int &width, unsigned int &height, HANDLE &dxShareHandle, DWORD &dwFormat) = 0;
	virtual bool GetActiveSender(char* Sendername) = 0;
	virtual bool SetActiveSender(const char* Sendername) = 0;
	
	// Utilities
	virtual bool SetDX9(bool bDX9 = true) = 0; // User request to use DirectX 9 (default is DirectX 11)
	virtual bool GetDX9() = 0; // Return the flag that has been set
	virtual bool SetMemoryShareMode(bool bMem = true) = 0;
	virtual bool GetMemoryShareMode() = 0;
	virtual int  GetMaxSenders() = 0; // Get maximum senders allowed
	virtual void SetMaxSenders(int maxSenders) = 0; // Set maximum senders allowed
	virtual bool GetHostPath(const char *sendername, char *hostpath, int maxchars) = 0; // The path of the host that produced the sender
	virtual int  GetVerticalSync() = 0;
	virtual bool SetVerticalSync(bool bSync = true) = 0;
	virtual bool SelectSenderPanel(const char* message = NULL) = 0;

	// Access to globals
	virtual bool GetSpoutSenderName(char * sendername, int maxchars) = 0; // get the global sender name
	virtual bool IsSpoutInitialized() = 0; // has the class been initialized
	
	// Adapter functions
	virtual int  GetNumAdapters() = 0; // Get the number of graphics adapters in the system
	virtual bool GetAdapterName(int index, char *adaptername, int maxchars) = 0; // Get an adapter name
	virtual bool SetAdapter(int index = 0) = 0; // Set required graphics adapter for output
	virtual int  GetAdapter() = 0; // Get the SpoutDirectX global adapter index
	
	// Library release function
    virtual void Release() = 0;

};


// Handle type. In C++ language the interface type is used.
typedef SPOUTLIBRARY* SPOUTHANDLE;

// Factory function that creates instances of the SPOUT object.
extern "C" SPOUTAPI SPOUTHANDLE WINAPI GetSpout(VOID);


////////////////////////////////////////////////////////////////////////////////
