/*! \mainpage libapogee Documentation
 \section over Overview
 libapogee contains the C++ interfaces to Apogee Imaging Systems cameras and filterwheels.

*/

/*! \page exceptionHandling  Exception Handling
 \section se Exceptions
 When an error is detected libapogee logs the error, in the Windows Event Log or syslog, 
 and then throws a std::runtime_exception.  The exception's what() string contains a : delimited
 string with the following format, <em>libapogee:location:Error Message:ErrorType</em>.  
 Here is an example of a critical errror  <em>libapogee:.\\win\\GenTwoWinUSB.cpp(307):VND_APOGEE_STATUS 
 failed with error 22:1</em>. The ErrorType vaule matches the Apg::ErrorType enum.  The 
 following control flow diagram shows how to handle various error types.
  
 \image html ExceptionHandling_small.png
 
 \section ece Critical Errors
Critical errors occur when there is a breakdown of communications between the host PC and the
camera.  When these error occur it is recommended to close the connection to the camera which
will attempt to clear the IO errors.  The calling application should then rescan the IO bus for the 
camera.  If the camera is detected, then proceed with opening the camera and initializing it just
like on start up.

 \section ese Serious Errors
 Almost all serious error conditions can be resolved by calling ApogeeCam::Reset, 
 which resets the camera's imaging pipeline.  After calling this function the calling
 program should reinitalize the camera (ApogeeCam::Init) and then wait for 
 the camera to enter the flushing state (Apg::Status_Flushing).  Upon entering the 
 flushing state, the camera is then ready to resume imaging operations.
  
*/

/*! \page hwTrig Hardware Triggering
\section Overview
\par
The Alta and Ascent camera systems allow for the use of an external, 
hardware trigger/signal to begin an exposure. The trigger signal arrives 
through the camera I/O port the pins and use of which are defined in 
another section of this document. This section provides additional detail 
on the properties for enabling or disabling different types of exposure triggers.

\par
Previous versions of the driver and firmware used the CameraMode 
property to control hardware triggering, by setting this property to either 
Apg::CameraMode_ExternalShutter or Apg::CameraMode_ExternalTrigger. 
Trigger operations are now controlled by properties that are set when using 
the camera in a specific mode. The following short table shows the trigger 
properties and the corresponding camera modes for which they are valid.

<TABLE>
<TR>
<TH> Mode </TH>
<TH> Type </TH>
<TH> Normal </TH>
<TH> TDI </TH>
<TH> Kinetics </TH>
</TR>
<TR>
<TD> Apg::TriggerMode_ExternalShutter </TD>
<TD> Apg::TriggerType_Each or Apg::TriggerType_Group </TD>
<TD> Yes </TD>
<TD> No </TD>
<TD> No </TD>
</TR>
<TR>
<TD> Apg::TriggerMode_ExternalReadoutIo  </TD>
<TD> Apg::TriggerType_Each or Apg::TriggerType_Group </TD>
<TD> Yes </TD>
<TD> No </TD>
<TD> No </TD>
</TR>
<TR>
<TD> Apg::TriggerMode_Normal </TD>
<TD> Apg::TriggerType_Each or Apg::TriggerType_Group </TD>
<TD> Yes </TD>
<TD> No </TD>
<TD> No </TD>
</TR>
<TR>
<TD> Apg::TriggerMode_TdiKinetics </TD>
<TD> Apg::TriggerType_Each or Apg::TriggerType_Group </TD>
<TD> No </TD>
<TD> Yes </TD>
<TD> Yes </TD>
</TR>
</TABLE>

\par
The ExternalShutter property is straightforward. When used, this signal 
(which is assigned a different I/O pin than the usual trigger start signal) 
controls the length of the exposure. It may be used in conjunction with the 
ExternalIoReadout property, to control when digitization begins. These two 
properties are designed to be used with single exposures.

\par
The Each/Group trigger properties are designed to give the greatest flexibility 
and number of options to users, for each corresponding camera mode.

\section nmt Normal Mode Triggers
\par
The following chart details how the Each/Group type for the Apg::TriggerMode_Normal 
mode are interpreted when ImageCount equals one (single exposure) and when ImageCount is greater 
than one (using the camera's internal sequence engine).
<TABLE>
<TR>
<TH> Apg::TriggerType_Each State </TH>
<TH> Apg::TriggerType_Group State </TH>
<TH> ImageCount = 1 </TH>
<TH> ImageCount > 1 </TH>
</TR>

<TR>
<TD> FALSE </TD>
<TD> FALSE </TD>
<TD> Software initiated single exposure. No hardware trigger enabled. </TD>
<TD> Software initiated sequenced exposure. No hardware trigger enabled. </TD>
</TR>

<TR>
<TD> FALSE </TD>
<TD> TRUE </TD>
<TD> Hardware trigger is used to begin the single exposure. </TD>
<TD> Hardware trigger is used to begin the sequenced exposure. One trigger kicks off the entire series of images. </TD>
</TR>

<TR>
<TD> TRUE </TD>
<TD> FALSE </TD>
<TD> Not a valid/usable option, and will have no impact. Because ImageCount is one, the camera control firmware should ignore the Each setting. </TD>
<TD> The first image of the sequence is begun by software control. Each subsequent image in the sequence will be initiated when its corresponding hardware trigger arrives. </TD>
</TR>

<TR>
<TD> TRUE </TD>
<TD> TRUE </TD>
<TD> Hardware trigger is used to begin the single exposure. Because ImageCount is one, the camera control firmware should ignore the Each setting. </TD>
<TD> The first image, as well as all subsequent images, of the sequence will be initiated by a corresponding hardware trigger. </TD>
</TR>
</TABLE>

\section tkt TDI-Kinetrics Triggers
\par 
The following chart details how the Each/Group types are interpreted for the Apg::TriggerMode_TdiKinetics. 
TDI operation presumes multiple rows, are in effect a sequence of normal images.
Kinetics operation presumes multiple sections, are in effect a sequence of normal images.
<TABLE>
<TR>
<TH> Apg::TriggerType_Each State </TH>
<TH> Apg::TriggerType_Group State </TH>
<TH> Description </TH>
</TR>

<TR>
<TD> FALSE </TD>
<TD> FALSE </TD>
<TD> Software initiated imaging No hardware trigger enabled. </TD>
</TR>

<TR>
<TD> FALSE </TD>
<TD> TRUE </TD>
<TD> A single hardware trigger is used to begin the entire TDI or Kinetics image. </TD>
</TR>

<TR>
<TD> TRUE </TD>
<TD> FALSE </TD>
<TD> The first row/section of the TDI/Kinetics image is begun by software control. Each subsequent row/section in the TDI/Kinetics image will be initiated when its corresponding hardware trigger arrives. </TD>
</TR>

<TR>
<TD> TRUE </TD>
<TD> TRUE </TD>
<TD> The first and subsequent rows/sections of the TDI/Kinetics image will be initiated by a corresponding hardware trigger. </TD>
</TR>
</TABLE>

\section tcu Control and Usage

*/
