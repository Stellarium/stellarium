/**************************************************
this is the second version of release ASI Camera ASIs
any question feel free contact us:sam.wen@zwoptical.com

here is the suggested procedure to operate the camera.

--> ASIGetNumOfConnectedCameras
----> ASIGetCameraProperty for each camera

--> ASIOpenCamera
--> ASIInitCamera
--> ASIGetNumOfControls
----> ASIGetControlCaps for each contronl and set or get value from them

--> ASISetROIFormat

--> ASIStartVideoCapture

//this is recommended to do in another thread
while(1)
{
	ASIGetVideoData
	...
}


***************************************************/
#ifndef ASICAMERA2_H
#define ASICAMERA2_H




#ifdef _WINDOWS
	#define ASICAMERA_API __declspec(dllexport)
#else
	#define ASICAMERA_API 
#endif

#define ASICAMERA_ID_MAX 128

typedef enum ASI_BAYER_PATTERN{
	ASI_BAYER_RG=0,
	ASI_BAYER_BG,
	ASI_BAYER_GR,
	ASI_BAYER_GB
}ASI_BAYER_PATTERN;

typedef enum ASI_IMG_TYPE{ //Supported Video Format 
	ASI_IMG_RAW8 = 0,
	ASI_IMG_RGB24,
	ASI_IMG_RAW16,
	ASI_IMG_Y8,
	ASI_IMG_END = -1

}ASI_IMG_TYPE;

typedef enum ASI_GUIDE_DIRECTION{ //Guider Direction
	ASI_GUIDE_NORTH=0,
	ASI_GUIDE_SOUTH,
	ASI_GUIDE_EAST,
	ASI_GUIDE_WEST
}ASI_GUIDE_DIRECTION;



typedef enum ASI_FLIP_STATUS {
	ASI_FLIP_NONE = 0,//: original
	ASI_FLIP_HORIZ,//: horizontal flip
	ASI_FLIP_VERT,// vertical flip
	ASI_FLIP_BOTH,//:both horizontal and vertical flip

}ASI_FLIP_STATUS;

typedef enum ASI_ERROR_CODE{ //ASI ERROR CODE
	ASI_SUCCESS=0,
	ASI_ERROR_INVALID_INDEX, //no camera connected or index value out of boundary
	ASI_ERROR_INVALID_ID, //invalid ID
	ASI_ERROR_INVALID_CONTROL_TYPE, //invalid control type
	ASI_ERROR_CAMERA_CLOSED, //camera didn't open
	ASI_ERROR_CAMERA_REMOVED, //failed to find the camera, maybe the camera has been removed
	ASI_ERROR_INVALID_PATH, //cannot find the path of the file
	ASI_ERROR_INVALID_FILEFORMAT, 
	ASI_ERROR_INVALID_SIZE, //wrong video format size
	ASI_ERROR_INVALID_IMGTYPE, //unsupported image formate
	ASI_ERROR_OUTOF_BOUNDARY, //the startpos is out of boundary
	ASI_ERROR_TIMEOUT, //timeout
	ASI_ERROR_INVALID_SEQUENCE,//stop capture first
	ASI_ERROR_BUFFER_TOO_SMALL, //buffer size is not big enough
	ASI_ERROR_VIDEO_MODE_ACTIVE,
	ASI_ERROR_EXPOSURE_IN_PROGRESS,
	ASI_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range
	ASI_ERROR_END
}ASI_ERROR_CODE;

typedef enum ASI_BOOL{
	ASI_FALSE =0,
	ASI_TRUE
}ASI_BOOL;

typedef struct _ASI_CAMERA_INFO
{
	char Name[64]; //the name of the camera, you can display this to the UI
	int CameraID; //this is used to control everything of the camera in other functions
	long MaxHeight; //the max height of the camera
	long MaxWidth;	//the max width of the camera

	ASI_BOOL IsColorCam; 
	ASI_BAYER_PATTERN BayerPattern;

	int SupportedBins[16]; //1 means bin1 which is supported by every camera, 2 means bin 2 etc.. 0 is the end of supported binning method
	ASI_IMG_TYPE SupportedVideoFormat[8]; //this array will content with the support output format type.IMG_END is the end of supported video format

	double PixelSize; //the pixel size of the camera, unit is um. such like 5.6um
	ASI_BOOL MechanicalShutter;
	ASI_BOOL ST4Port;
	ASI_BOOL IsCoolerCam;
	ASI_BOOL IsUSB3Host;
	ASI_BOOL IsUSB3Camera;
	float ElecPerADU;

	char Unused[24];
} ASI_CAMERA_INFO;

typedef enum ASI_CONTROL_TYPE{ //Control type//
	ASI_GAIN = 0,
	ASI_EXPOSURE,
	ASI_GAMMA,
	ASI_WB_R,
	ASI_WB_B,
	ASI_BRIGHTNESS,
	ASI_BANDWIDTHOVERLOAD,	
	ASI_OVERCLOCK,
	ASI_TEMPERATURE,// return 10*temperature
	ASI_FLIP,
	ASI_AUTO_MAX_GAIN,
	ASI_AUTO_MAX_EXP,//micro second
	ASI_AUTO_MAX_BRIGHTNESS,
	ASI_HARDWARE_BIN,
	ASI_HIGH_SPEED_MODE,
	ASI_COOLER_POWER_PERC,
	ASI_TARGET_TEMP,// not need *10
	ASI_COOLER_ON,
	ASI_MONO_BIN,//lead to less grid at software bin mode for color camera
	ASI_FAN_ON,
	ASI_PATTERN_ADJUST,
	ASI_ANTI_DEW_HEATER,

}ASI_CONTROL_TYPE;

typedef struct _ASI_CONTROL_CAPS
{
	char Name[64]; //the name of the Control like Exposure, Gain etc..
	char Description[128]; //description of this control
	long MaxValue;
	long MinValue;
	long DefaultValue;
	ASI_BOOL IsAutoSupported; //support auto set 1, don't support 0
	ASI_BOOL IsWritable; //some control like temperature can only be read by some cameras 
	ASI_CONTROL_TYPE ControlType;//this is used to get value and set value of the control
	char Unused[32];
} ASI_CONTROL_CAPS;

typedef enum ASI_EXPOSURE_STATUS {
	ASI_EXP_IDLE = 0,//: idle states, you can start exposure now
	ASI_EXP_WORKING,//: exposing
	ASI_EXP_SUCCESS,// exposure finished and waiting for download
	ASI_EXP_FAILED,//:exposure failed, you need to start exposure again

}ASI_EXPOSURE_STATUS;

typedef struct _ASI_ID{
	unsigned char id[8];
}ASI_ID;

#ifndef __cplusplus
#define ASI_CONTROL_TYPE int
#define ASI_BOOL int
#define ASI_ERROR_CODE int
#define ASI_FLIP_STATUS int
#define ASI_IMG_TYPE int
#define ASI_GUIDE_DIRECTION int
#define ASI_BAYER_PATTERN int
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
Descriptions£º 
this should be the first API to be called
get number of connected ASI cameras,

Paras£º 

return£ºnumber of connected ASI cameras. 1 means 1 camera connected.
***************************************************************************/
ASICAMERA_API  int ASIGetNumOfConnectedCameras(); 

/***************************************************************************
Descriptions:
get the product ID of each supported camera, at first set pPIDs as 0 and get length and then malloc a buffer to contain the PIDs

Paras:
int* pPIDs: pointer to array of PIDs

Return: length of the array.
***************************************************************************/
ASICAMERA_API int ASIGetProductIDs(int* pPIDs);

/***************************************************************************
Descriptions£º
get the property of the connected cameras, you can do this without open the camera.
here is the sample code:

int iNumofConnectCameras = ASIGetNumOfConnectedCameras();
ASI_CAMERA_INFO **ppASICameraInfo = (ASI_CAMERA_INFO **)malloc(sizeof(ASI_CAMERA_INFO *)*iNumofConnectCameras);
for(int i = 0; i < iNumofConnectCameras; i++)
{
ppASICameraInfo[i] = (ASI_CAMERA_INFO *)malloc(sizeof(ASI_CAMERA_INFO ));
ASIGetCameraProperty(ppASICameraInfo[i], i);
}
				
Paras£º		
	ASI_CAMERA_INFO *pASICameraInfo: Pointer to structure containing the property of camera
									user need to malloc the buffer
	int iCameraIndex: 0 means the first connect camera, 1 means the second connect camera

return£º
	ASI_SUCCESS: Operation is successful
	ASI_ERROR_INVALID_INDEX  :no camera connected or index value out of boundary

***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIGetCameraProperty(ASI_CAMERA_INFO *pASICameraInfo, int iCameraIndex);

/***************************************************************************
Descriptions£º
	open the camera before any operation to the camera, this will not affect the camera which is capturing
	All APIs below need to open the camera at first.

Paras£º		
	int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return£º
ASI_SUCCESS: Operation is successful
ASI_ERROR_INVALID_ID  : no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_CAMERA_REMOVED: failed to find the camera, maybe camera has been removed

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIOpenCamera(int iCameraID);

/***************************************************************************
Descriptions

	Initialise the camera after open, this function may take some while, this will affect the camera which is capturing

Paras£º		
	int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIInitCamera(int iCameraID);

/***************************************************************************
Descriptions£º
you need to close the camera to free all the resource


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return£º
ASI_SUCCESS :it will return success even the camera already closed
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASICloseCamera(int iCameraID);




/***************************************************************************
Descriptions£º
Get number of controls available for this camera. the camera need be opened at first.



Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int * piNumberOfControls: pointer to an int to save the number of controls

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIGetNumOfControls(int iCameraID, int * piNumberOfControls);


/***************************************************************************
Descriptions£º
Get controls property available for this camera. the camera need be opened at first.
user need to malloc and maintain the buffer.



Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int iControlIndex: index of control, NOT control type
ASI_CONTROL_CAPS * pControlCaps: Pointer to structure containing the property of the control
user need to malloc the buffer

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIGetControlCaps(int iCameraID, int iControlIndex, ASI_CONTROL_CAPS * pControlCaps);

/***************************************************************************
Descriptions£º
Get controls property value and auto value
note:the value of the temperature is the float value * 10 to convert it to long type, control name is "Temperature"
because long is the only type for control(except cooler's target temperature, because it is an integer)

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int ControlType: this is get from control property use the API ASIGetControlCaps
long *plValue: pointer to the value you want to save the value get from control
ASI_BOOL *pbAuto: pointer to the ASI_BOOL type

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_INVALID_CONTROL_TYPE, //invalid Control type
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIGetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long *plValue, ASI_BOOL *pbAuto);

/***************************************************************************
Descriptions£º
Set controls property value and auto value
it will return success and set the max value or min value if the value is beyond the boundary


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int ControlType: this is get from control property use the API ASIGetControlCaps
long lValue: the value set to the control
ASI_BOOL bAuto: set the control auto

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_INVALID_CONTROL_TYPE, //invalid Control type
ASI_ERROR_GENERAL_ERROR,//general error, eg: value is out of valid range; operate to camera hareware failed
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASISetControlValue(int  iCameraID, ASI_CONTROL_TYPE  ControlType, long lValue, ASI_BOOL bAuto);

 

/***************************************************************************
Descriptions£º
set the ROI area before capture.
you must stop capture before call it.
the width and height is the value after binning.
ie. you need to set width to 640 and height to 480 if you want to run at 640X480@BIN2
ASI120's data size must be times of 1024 which means width*height%1024=0
Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int iWidth,  the width of the ROI area. Make sure iWidth%8 = 0. 
int iHeight,  the height of the ROI area. Make sure iHeight%2 = 0, 
further, for USB2.0 camera ASI120, please make sure that iWidth*iHeight%1024=0. 
int iBin,   binning method. bin1=1, bin2=2
ASI_IMG_TYPE Img_type: the output format you want 

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_INVALID_SIZE, //wrong video format size
ASI_ERROR_INVALID_IMGTYPE, //unsupported image format, make sure iWidth and iHeight and binning is set correct
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASISetROIFormat(int iCameraID, int iWidth, int iHeight,  int iBin, ASI_IMG_TYPE Img_type); 


/***************************************************************************
Descriptions£º
Get the current ROI area setting .

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int *piWidth,  pointer to the width of the ROI area   
int *piHeight, pointer to the height of the ROI area.
int *piBin,   pointer to binning method. bin1=1, bin2=2
ASI_IMG_TYPE *pImg_type: pointer to the output format

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetROIFormat(int iCameraID, int *piWidth, int *piHeight,  int *piBin, ASI_IMG_TYPE *pImg_type); 


/***************************************************************************
Descriptions£º
Set the start position of the ROI area.
you can call this API to move the ROI area when video is streaming
the camera will set the ROI area to the center of the full image as default
at bin2 or bin3 mode, the position is relative to the image after binning


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int iStartX, pointer to the start X
int iStartY  pointer to the start Y

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_OUTOF_BOUNDARY: the start x and start y make the image out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASISetStartPos(int iCameraID, int iStartX, int iStartY); 

/***************************************************************************
Descriptions£º
Get the start position of current ROI area .

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int *piStartX, pointer to the start X
int *piStartY  pointer to the start Y

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetStartPos(int iCameraID, int *piStartX, int *piStartY); 

/***************************************************************************
Descriptions£º
Get the droped frames .
drop frames happen when USB is traffic or harddisk write speed is slow
it will reset to 0 after stop capture

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
int *piDropFrames pointer to drop frames

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetDroppedFrames(int iCameraID,int *piDropFrames); 

/***************************************************************************
Descriptions£º
provide a dark file's path to the function and enable dark subtract
this is used when there is hot pixel or need to do long exposure
you'd better make this dark file from the  "dark subtract" funtion 
of the "video capture filter" directshow page.
the dark file's size should be the same of camera's max width and height 
and should be RGB8 raw format.it will on even you changed the ROI setting
it only correct the hot pixels if out put isn't 16bit.

it will be remembered in registry. so "Dark subtract" is on next time if you close your app.


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
char *pcBMPPath: the path to the bmp dark file. 
return£º
ASI_SUCCESS : Operation is successful
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_PATH, //cannot find the path of the file
ASI_ERROR_INVALID_FILEFORMAT, //the dark file's size should be the same of camera's max width and height  

***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIEnableDarkSubtract(int iCameraID, char *pcBMPPath);

/***************************************************************************
Descriptions£º
Disable the dark subtract function.
you'd better call it at start if you don't want to use it.
because dark subtract function is remembered on windows platform


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_CAMERA_CLOSED : camera didn't open
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIDisableDarkSubtract(int iCameraID);

/***************************************************************************
Descriptions£º
Start video capture
then you can get the data from the API ASIGetVideoData


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return:
ASI_SUCCESS : Operation is successful, it will return success if already started
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_EXPOSURE_IN_PROGRESS: snap mode is working, you need to stop snap first
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIStartVideoCapture(int iCameraID);

/***************************************************************************
Descriptions£º
Stop video capture


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty

return:
ASI_SUCCESS : Operation is successful, it will return success if already stopped
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIStopVideoCapture(int iCameraID);

/***************************************************************************
Descriptions£º
get data from the video buffer.the buffer is very small 
you need to call this API as fast as possible, otherwise frame will be discarded
so the best way is maintain one buffer loop and call this API in a loop
please make sure the buffer size is biger enough to hold one image
otherwise the this API will crash


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
unsigned char* pBuffer, caller need to malloc the buffer, make sure the size is big enough
		the size in byte:
		8bit mono:width*height
		16bit mono:width*height*2
		RGB24:width*height*3

int iWaitms, this API will block and wait iWaitms to get one image. the unit is ms
		-1 means wait forever. this value is recommend set to exposure*2+500ms

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_TIMEOUT: no image get and timeout
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetVideoData(int iCameraID, unsigned char* pBuffer, long lBuffSize, int iWaitms);


/***************************************************************************
Descriptions£º
PulseGuide of the ST4 port on. this function only work on the module which have ST4 port


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_GUIDE_DIRECTION direction the direction of guider

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIPulseGuideOn(int iCameraID, ASI_GUIDE_DIRECTION direction);

/***************************************************************************
Descriptions£º
PulseGuide of the ST4 port off. this function only work on the module which have ST4 port
make sure where is ASIPulseGuideOn and there is ASIPulseGuideOff

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_GUIDE_DIRECTION direction the direction of guider

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIPulseGuideOff(int iCameraID, ASI_GUIDE_DIRECTION direction);


/***************************************************************************
Descriptions£º
Start camera exposure. the following 4 API is usually used when long exposure required
start exposure  and check the exposure status then get the data


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_BOOL bIsDark: means dark frame if there is mechanical shutter on the camera. otherwise useless

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_VIDEO_MODE_ACTIVE: video mode is working, you need to stop video capture first
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE  ASIStartExposure(int iCameraID, ASI_BOOL bIsDark);

/***************************************************************************
Descriptions£º
to cancel the long exposure which is on.


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty


return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE  ASIStopExposure(int iCameraID);

/***************************************************************************
Descriptions£º
to get the exposure status, work with ASIStartExposure.
you can read the data if get ASI_EXP_SUCCESS. or have to restart exposure again
if get ASI_EXP_FAILED

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_EXPOSURE_STATUS *pExpStatus: the exposure status


return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary

***************************************************************************/

ASICAMERA_API ASI_ERROR_CODE  ASIGetExpStatus(int iCameraID, ASI_EXPOSURE_STATUS *pExpStatus);

/***************************************************************************
Descriptions£º
get data after exposure.
please make sure the buffer size is biger enough to hold one image
otherwise the this API will crash


Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
unsigned char* pBuffer, caller need to malloc the buffer, make sure the size is big enough
the size in byte:
8bit mono:width*height
16bit mono:width*height*2
RGB24:width*height*3


return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
ASI_ERROR_TIMEOUT: no image get and timeout
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetDataAfterExp(int iCameraID, unsigned char* pBuffer, long lBuffSize);

/***************************************************************************
Descriptions£º
get camera id stored in flash, only available for USB3.0 camera

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_ID* pID: pointer to ID

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASIGetID(int iCameraID, ASI_ID* pID);

/***************************************************************************
Descriptions£º
write camera id to flash, only available for USB3.0 camera

Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
ASI_ID ID: ID

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API  ASI_ERROR_CODE ASISetID(int iCameraID, ASI_ID ID);

/***************************************************************************
Descriptions£º
get pre-setting parameter
Paras£º		
int CameraID: this is get from the camera property use the API ASIGetCameraProperty
Offset_HighestDR: offset at highest dynamic range, 
Offset_UnityGain: offset at unity gain
int *Gain_LowestRN, *Offset_LowestRN: gain and offset at lowest read noise

return:
ASI_SUCCESS : Operation is successful
ASI_ERROR_CAMERA_CLOSED : camera didn't open
ASI_ERROR_INVALID_ID  :no camera of this ID is connected or ID value is out of boundary
***************************************************************************/
ASICAMERA_API ASI_ERROR_CODE ASIGetGainOffset(int iCameraID, int *Offset_HighestDR, int *Offset_UnityGain, int *Gain_LowestRN, int *Offset_LowestRN);


#ifdef __cplusplus
}
#endif

#endif
