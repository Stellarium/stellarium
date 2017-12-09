/*******************************************************************************
 NexDome
 Copyright(c) 2017 Rozeware Development Ltd. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include "nex_dome.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#include <memory>

#include <indicom.h>

#include <connectionplugins/connectionserial.h>

// We declare an auto pointer to nexDome.
std::unique_ptr<NexDome> nexDome(new NexDome());

#define DOME_SPEED      2.0             /* 2 degrees per second, constant */
#define SHUTTER_TIMER   5.0             /* Shutter closes/open in 5 seconds */

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
        nexDome->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
        nexDome->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
        nexDome->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
        nexDome->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
  INDI_UNUSED(dev);
  INDI_UNUSED(name);
  INDI_UNUSED(sizes);
  INDI_UNUSED(blobsizes);
  INDI_UNUSED(blobs);
  INDI_UNUSED(formats);
  INDI_UNUSED(names);
  INDI_UNUSED(n);
}

void ISSnoopDevice (XMLEle *root)
{
    nexDome->ISSnoopDevice(root);
}

NexDome::NexDome()
{
   TimeSinceUpdate=0;
   InMotion=false;
   Calibrating=false;
   BatteryMain=0;
   BatteryShutter=0;
   HomeError=0;
   HomeAz=-1;
   StepsPerDomeTurn=0;
   StoredPark=-1;
   ShutterState=-1;
   //LastShutterState=-1;
   MotorPower=-1;
   ShutterPosition=0;
   DomeReversed=-1;
   HaveFirmwareVersion=false;

   SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_ABS_MOVE | DOME_CAN_PARK | DOME_HAS_SHUTTER);
   //SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_ABS_MOVE | DOME_CAN_PARK );

}

/************************************************************************************
 *
* ***********************************************************************************/
bool NexDome::initProperties()
{
    INDI::Dome::initProperties();

    SetParkDataType(PARK_AZ);

    //  now fix the park data display
    IUFillNumber(&ParkPositionN[AXIS_AZ],"PARK_AZ","AZ Degrees","%5.1f",0.0,360.0,0.0,0);
    //  lets fix the display for absolute position
    IUFillNumber(&DomeAbsPosN[0],"DOME_ABSOLUTE_POSITION","Degrees","%5.1f",0.0,360.0,1.0,0.0);
    //IUFillNumberVector(&ParkPositionNP,ParkPositionN,1,getDeviceName(),

    IUFillSwitch(&HomeS[0],"Home","",ISS_OFF);
    IUFillSwitchVector(&HomeSP,HomeS,1,getDeviceName(),"DOME_HOME","Home",MAIN_CONTROL_TAB,IP_RW,ISR_ATMOST1,0,IPS_IDLE);
    IUFillSwitch(&CalibrateS[0],"Calibrate","",ISS_OFF);
    IUFillSwitchVector(&CalibrateSP,CalibrateS,1,getDeviceName(),"DOME_CALIBRATE","Calibrate",SITE_TAB,IP_RW,ISR_ATMOST1,0,IPS_IDLE);
    IUFillNumber(&SyncPositionN[0],"SYNC_AZ","AZ Degrees","%5.1f",0.0,360,0.0,0);
    IUFillNumberVector(&SyncPositionNP,SyncPositionN,1,getDeviceName(),"DOME_SYNC","Sync",SITE_TAB,IP_RW,60,IPS_IDLE);
    IUFillNumber(&HomePositionN[0],"HOME_POSITON","degrees","%5.1f",0.0,360.0,0.0,0);
    IUFillNumberVector(&HomePositionNP,HomePositionN,1,getDeviceName(),"HOME_POS","Home Az",SITE_TAB,IP_RO,60,IPS_IDLE);
    IUFillNumber(&BatteryLevelN[0],"BATTERY_ROTATOR","Rotator","%5.2f",0.0,16.0,0.0,0);
    IUFillNumber(&BatteryLevelN[1],"BATTERY_SHUTTER","Shutter","%5.2f",0.0,16.0,0.0,0);
    IUFillNumberVector(&BatteryLevelNP,BatteryLevelN,2,getDeviceName(),"BATTERY","Battery Level",SITE_TAB,IP_RO,60,IPS_IDLE);
    IUFillText(&FirmwareVersionT[0],"FIRMWARE_VERSION","Version","");
    IUFillTextVector(&FirmwareVersionTP,FirmwareVersionT,1,getDeviceName(),"FIRMWARE","Firmware",SITE_TAB,IP_RO,60,IPS_IDLE);
    IUFillSwitch(&ReversedS[0],"Disable","",ISS_OFF);
    IUFillSwitch(&ReversedS[1],"Enable","",ISS_OFF);
    IUFillSwitchVector(&ReversedSP,ReversedS,2,getDeviceName(),"DOME_REVERSED","Reversed",SITE_TAB,IP_RW,ISR_ATMOST1,0,IPS_IDLE);

    serialConnection->setDefaultBaudRate(Connection::Serial::B_9600);

    //addAuxControls();
    return true;
}

bool NexDome::SetupParms()
{

    DomeAbsPosN[0].value = 0;


    IDSetNumber(&DomeAbsPosNP, NULL);
    IDSetNumber(&DomeParamNP, NULL);

    if (InitPark())
    {
        // If loading parking data is successful, we just set the default parking values.
        SetAxis1ParkDefault(90);
    }
    else
    {
        // Otherwise, we set all parking data to default in case no parking data is found.
        SetAxis1Park(90);
        SetAxis1ParkDefault(90);
    }

    return true;
}

bool NexDome::Handshake()
{
    //SetTimer(10);
    return true;
}

/*

bool NexDome::Connect()
{
    bool rc;

    rc = INDI::Dome::Connect();

    if (rc)
    {
    	fprintf(stderr,"Dome is connected fd is %d\n",PortFD);
    	// set a short timer so we read it right away
    	SetTimer(1000);
    } else {
	fprintf(stderr,"Call to connect returned false\n");
	return false;
    }



    return true;

    int rc;

    rc=tty_connect(PortT[0].text,9600,8,0,1,&PortFD);
    if(rc != TTY_OK) {
      fprintf(stderr,"Failed to connect to NexDome\n");
      return false;
    }
    fprintf(stderr,"Dome is connected\n");
    // set a short timer so we read it right away
    SetTimer(10);
    return true;

}
*/

NexDome::~NexDome()
{

}

const char * NexDome::getDefaultName()
{
        return (char *)"NexDome";
}

bool NexDome::updateProperties()
{
    INDI::Dome::updateProperties();

    if (isConnected())
    {
        defineSwitch(&HomeSP);
        defineSwitch(&CalibrateSP);
        defineNumber(&SyncPositionNP);
        defineNumber(&HomePositionNP);
        defineNumber(&BatteryLevelNP);
        defineText(&FirmwareVersionTP);
        defineSwitch(&ReversedSP);
        SetupParms();
	SetTimer(100);
    } else {
        deleteProperty(HomeSP.name);
        deleteProperty(CalibrateSP.name);
	deleteProperty(SyncPositionNP.name);
	deleteProperty(HomePositionNP.name);
	deleteProperty(BatteryLevelNP.name);
	deleteProperty(FirmwareVersionTP.name);
	deleteProperty(ReversedSP.name);
    }

    return true;
}

bool NexDome::ISNewSwitch(const char *dev,const char *name,ISState *states,char *names[], int n)
{
    if(strcmp(dev,getDeviceName())==0) {
      if(!strcmp(name,HomeSP.name)) {
        if(AtHome) {
          //fprintf(stderr,"Already at home\n");
          HomeSP.s=IPS_OK;
          IDSetSwitch(&HomeSP,"Already at home");
        } else {
          //fprintf(stderr,"Homing the dome\n");
          if(!MotorPower) {
            HomeSP.s=IPS_ALERT;
            IDSetSwitch(&HomeSP,"Cannot home without motor power");
          } else {
            HomeSP.s=IPS_BUSY;
            WriteString("h\n");
            IDSetSwitch(&HomeSP,"Dome finding home");
          }
        }
        return true;
      }
      if(!strcmp(name,CalibrateSP.name)) {
        if(AtHome) {
          //fprintf(stderr,"Calibrating\n");
          CalibrateSP.s=IPS_BUSY;
          WriteString("c\n");
          Calibrating=true;
	  time(&CalStartTime);
          HomeAz=-1;
          IDSetSwitch(&CalibrateSP,"Dome is Calibrating");
        } else {
          //fprintf(stderr,"Cannot calibrate unless at home\n");
          IDSetSwitch(&CalibrateSP,"Cannot calibrate unless dome is at home position");
        }
        return true;
      }
      if(!strcmp(name,ReversedSP.name)) {
         if(states[0]==ISS_OFF) {
            ReversedSP.s=IPS_OK;
	    ReversedS[0].s=ISS_OFF;
	    ReversedS[1].s=ISS_ON;
            WriteString("y 1\n");
	    IDSetSwitch(&ReversedSP,"Dome is reversed");
         } else {
            ReversedSP.s=IPS_IDLE;
            ReversedS[0].s=ISS_ON;
            ReversedS[1].s=ISS_OFF;
            WriteString("y 0\n");
	    IDSetSwitch(&ReversedSP,"Dome is not reversed");
         }
      }
    }
    return INDI::Dome::ISNewSwitch(dev,name,states,names,n);
}

bool NexDome::ISNewNumber(const char *dev,const char *name,double values[],char *names[],int n)
{
    if(strcmp(dev,getDeviceName())==0) {
      if(!strcmp(name,SyncPositionNP.name)) {
        float p;
	char buf[30];
        p=values[0];
	SyncPositionN[0].value=p;
	SyncPositionNP.s=IPS_BUSY;
	IDSetNumber(&SyncPositionNP,NULL);
        //fprintf(stderr,"Got a sync at %3.0f\n",p);
	sprintf(buf,"s %4.1f\n",p);
	WriteString(buf);
        //  tell main routines to get the new home position
        //HomeAz=-1;
        return true;
      }
    }
    return INDI::Dome::ISNewNumber(dev,name,values,names,n);
}

bool NexDome::Disconnect()
{
    fprintf(stderr,"Disconnect Port\n");
    tty_disconnect(PortFD);
    return true;
}

int NexDome::ClearSerialBuffers()
{
  int bytesRead;
  char str[35];

  if(isConnected()==false) return 0;

  //  clear anything already in the buffer
  while(tty_read(PortFD,str,30,0,&bytesRead)==TTY_OK) {
    fprintf(stderr,"Clearing %d bytes\n",bytesRead);
  }
  return 0;
}

/* read a string till lf or cr */
int NexDome::ReadString(char *buf,int size)
{
  //char *pt;
  int num;
  int bytesRead;
  char a;
  int rc;

  num=0;
  buf[num]=0;
  //fprintf(stderr,"first read\n");
  rc=tty_read(PortFD,&a,1,2,&bytesRead);
  while(rc==TTY_OK) {
    //fprintf(stderr,"read returns %d %c\n",rc,a);
    if((a=='\n')||(a=='\r')) return num;
    buf[num]=a;
    num++;
    buf[num]=0;
    if(num==size) return num;
    //fprintf(stderr,"second read\n");
    rc=tty_read(PortFD,&a,1,1,&bytesRead);
  }
  if(rc != TTY_OK) {
    //fprintf(stderr,"Readstring returns error %d\n",rc);
    return -1;
  }
  return num;
}

int NexDome::WriteString(const char *buf)
{
    char ReadBuf[40];
    int bytesWritten;
    int rc;
    char m;

    tty_write(PortFD,buf,strlen(buf),&bytesWritten);
    //tty_write(PortFD,"\n",1,&bytesWritten);

    //  lets process returns
    //  There may be more than one response qued
    //  So continue processing them till we read the response message
    //  for the query we just wrote
    rc=ReadString(ReadBuf,40);
    while(rc >= 0) {
      //  return size zero says dome just sent an empty linefeed
      if(rc > 0) {
        ProcessDomeMessage(ReadBuf);
        //  A message is always sent using lower case
        //  and the response is upper case
        m=buf[0];
        m=toupper(m);
        //  if first character of response is the upper case variant of what
        //  we just sent, we have processed the que
        if(m==ReadBuf[0]) return 0;
      }
      rc=ReadString(ReadBuf,40);
    }
    //  if we get here, there was a tty error
    //  reading from the dome
    return -1;
}

void NexDome::TimerHit()
{
    //fprintf(stderr,"Timer\n");
    int nexttimer=1000;
    if(isConnected()) ReadDomeStatus();
    //else fprintf(stderr,"Timer but not connected\n");
    SetTimer(nexttimer);
}

void NexDome::ProcessDomeMessage(char *buf)
{
  //char Message[100];

  if(buf[0]==0) {
    //  no need to process an empty string
    //  it was just an empty line feed
    return;
  }
  //fprintf(stderr,"Process %s\n",buf);
  //  Dome in motion message
  if(buf[0]=='M') {
    int m;
    m=atoi(&buf[1]);
    //fprintf(stderr,"Motion is %d\n",m);
    if(m==0) {
      //fprintf(stderr,"Dome is stopped\n");
      if(getDomeState()==DOME_PARKING) {
          SetParked(true);
      }
      InMotion=false;
      if(MotorPower) {
        DomeAbsPosNP.s=IPS_OK;
        DomeMotionSP.s=IPS_OK;
      } else {
        DomeAbsPosNP.s=IPS_ALERT;
        DomeMotionSP.s=IPS_ALERT;
      }
      IDSetSwitch(&DomeMotionSP,NULL);
      if(Calibrating) {
	float delta;
	time_t now;
	time(&now);
	delta=difftime(now,CalStartTime);
        //fprintf(stderr,"Calibrating Finished\n");
        //Calibrating=false;
        CalibrateSP.s=IPS_OK;
        IDSetSwitch(&CalibrateSP,"Calibration complete %3.0f seconds",delta);
        //WriteString("t\n");
      }
    } else {
      //fprintf(stderr,"Dome is in motion\n");
      //if(m==3) fprintf(stderr,"Calibrating\n");
      InMotion=true;
      DomeAbsPosNP.s=IPS_BUSY;
      if(HomeSP.s==IPS_OK) {
        HomeSP.s=IPS_IDLE;
        IDSetSwitch(&HomeSP,NULL);
      }
    }
  }
  //  Dome heading
  if(buf[0]=='Q') {
    float h;
    h=atof(&buf[1]);
    DomeAbsPosN[0].value=h;
    IDSetNumber(&DomeAbsPosNP,NULL);
  }
  //  Home sensor
  if(buf[0]=='Z') {
    int t;
    t=atoi(&buf[1]);
    if(t==1) {
      //if(!AtHome) fprintf(stderr,"AtHome\n");
      AtHome=true;
      if(HomeSP.s != IPS_OK) {
        HomeSP.s = IPS_OK;
        IDSetSwitch(&HomeSP,"Dome is at home");
      }
    }
    if(t==0) {
      //fprintf(stderr,"Not at home\n");
      AtHome=false;
      if(HomeSP.s != IPS_IDLE) {
        if(MotorPower) HomeSP.s=IPS_IDLE;
        else HomeSP.s=IPS_ALERT;
        IDSetSwitch(&HomeSP,NULL);
      }
    }
    if(t==-1) {
      if(HomeSP.s != IPS_BUSY) {
        if(MotorPower) HomeSP.s=IPS_BUSY;
        else HomeSP.s=IPS_ALERT;
        IDSetSwitch(&HomeSP,"Dome has not been homed");
      }
    }
  }
  //  Battery Levels
  if(buf[0]=='K') {
    int b1,b2;
    float b3,b4;
    char a;
    sscanf(buf,"%c %d %d",&a,&b1,&b2);
    b3=(float)b1/100;
    b4=(float)b2/100;
    if((BatteryMain != b3)||(BatteryShutter != b4)) {
      BatteryMain=b3;
      BatteryShutter=b4;
      BatteryLevelN[0].value=BatteryMain;
      BatteryLevelN[1].value=BatteryShutter;
      if(BatteryMain > 7) {
        BatteryLevelNP.s=IPS_OK;
        if(!MotorPower) {
          IDSetNumber(&BatteryLevelNP,"Motor is powered");
          //ParkSP.s=IPS_OK;
	  //IDSetSwitch(&ParkSP,NULL);
        }
        MotorPower=true;
      } else {
        //  and it's off now
        if(MotorPower) IDSetNumber(&BatteryLevelNP,"Motor is NOT powered");
        MotorPower=false;
        DomeAbsPosNP.s=IPS_ALERT;
        IDSetNumber(&DomeAbsPosNP,NULL);
 	//ParkSP.s=IPS_ALERT;
	//IDSetSwitch(&ParkSP,NULL);
	HomeSP.s=IPS_ALERT;
	IDSetSwitch(&HomeSP,NULL);
       

        BatteryLevelNP.s=IPS_ALERT;
      } 
      IDSetNumber(&BatteryLevelNP,NULL);
    }
    //fprintf(stderr,"Battery %d %d\n",b1,b2);
  }
  //  Steps per revolution
  if(buf[0]=='T') {
    //int b1;
    StepsPerDomeTurn=atoi(&buf[1]);
    //fprintf(stderr,"****  %d steps per turn ****\n",StepsPerDomeTurn);
    IDSetSwitch(&HomeSP,"Dome has %d steps per revolution",StepsPerDomeTurn);
  }
  //  Pointing error at last home sensor
  if(buf[0]=='O') {
    float b1;
    b1=atof(&buf[1]);
    if(b1 != HomeError) {
      fprintf(stderr,"Home error %4.2f\n",b1);
      HomeError=b1;
    }
  }
  //  Processed a sync
  if(buf[0]=='S') {
    float b1;
    b1=atof(&buf[1]);
    //sprintf(Message,"Dome sync at %4.1f\n",b1);
    SyncPositionNP.s=IPS_OK;
    IDSetNumber(&SyncPositionNP,"Dome sync at %3.0f",b1);
    //  refetch the new home azimuth
    HomeAz=-1;
  }
  if(buf[0]=='I') {
    float b1;
    b1=atof(&buf[1]);
    if(b1 != HomeAz) {
      //fprintf(stderr,"HomeAzimuth %4.1f\n",b1);
      HomePositionN[0].value=b1;
      IDSetNumber(&HomePositionNP,"Home position is %4.1f degrees",b1);
      HomeAz=b1;
    }
  }
  if(buf[0]=='B') {
    float b1;
    b1=atof(&buf[1]);
    if((b1 != ShutterPosition)) {
      ShutterPosition=b1;
      if(b1==90.0) {
        shutterState=SHUTTER_OPENED;
        DomeShutterSP.s=IPS_OK;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is open");
      } else {
        if(b1==-22.5) {
          shutterState=SHUTTER_CLOSED;
          DomeShutterSP.s=IPS_IDLE;
	  DomeShutterS[0].s=ISS_OFF;
	  DomeShutterS[1].s=ISS_OFF;
          IDSetSwitch(&DomeShutterSP,"Shutter is closed");
        }  else {
          shutterState=SHUTTER_UNKNOWN;
          DomeShutterSP.s=IPS_ALERT;
	  DomeShutterS[0].s=ISS_OFF;
	  DomeShutterS[1].s=ISS_OFF;
          IDSetSwitch(&DomeShutterSP,"Shutter Position %4.1f",b1);
        }
      }
    }
  }
  if(buf[0]=='U') {
    int b;
    b=atoi(&buf[1]);
    if(b!=ShutterState) {
      if(b==0) {
        DomeShutterSP.s=IPS_ALERT;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is not connected");
      }
      if(b==1) {
        DomeShutterSP.s=IPS_OK;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is open");
      }
      if(b==2) {
        DomeShutterSP.s=IPS_BUSY;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is opening");
      }
      if(b==3) {
        DomeShutterSP.s=IPS_IDLE;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is closed");
      }
      if(b==4) {
        DomeShutterSP.s=IPS_BUSY;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter is closing");
      }
      if(b==5) {
        DomeShutterSP.s=IPS_ALERT;
	DomeShutterS[0].s=ISS_OFF;
	DomeShutterS[1].s=ISS_OFF;
        IDSetSwitch(&DomeShutterSP,"Shutter state undetermined");
      }

      ShutterState=b;
    }
  }
  if(buf[0]=='Y') {
    DomeReversed=atoi(&buf[1]);
    if(DomeReversed==1) {
      ReversedS[0].s=ISS_OFF;
      ReversedS[1].s=ISS_ON;
      ReversedSP.s=IPS_OK;
      IDSetSwitch(&ReversedSP,nullptr);
    } else {
      ReversedS[0].s=ISS_ON;
      ReversedS[1].s=ISS_OFF;
      ReversedSP.s=IPS_IDLE;
      IDSetSwitch(&ReversedSP,nullptr);
    }
  }
  if(buf[0]=='V') {
    IUSaveText(&FirmwareVersionT[0],&buf[1]);
    IDSetText(&FirmwareVersionTP,nullptr);
    HaveFirmwareVersion=true;

  }
}

void NexDome::ReadDomeStatus()
{
    //int bytesWritten;
    //char str[40];
    int rc;

    if(isConnected() == false) return;  //  No need to reset timer if we are not connected anymore
    //ClearSerialBuffers();

    //fprintf(stderr,"ReadDomeStatus\n");

    rc=WriteString("m\n");
    if(rc < 0) return;
	
    rc=WriteString("q\n");
    if(rc < 0) return;

    if(!InMotion) {
      //fprintf(stderr,"Not in motion\n");
      // Dome is not in motion, lets check a few more things
      // Check for home position
      rc=WriteString("z\n");
      if(rc < 0) return;
      // Check voltage on batteries
      rc=WriteString("k\n");
      if(rc < 0) return;
      //  if we have started a calibration cycle
      //  it has finished since we are not in motion
      if(Calibrating) {
        //  we just calibrated, and are now not moving
        Calibrating=false;
        //  lets get our new value for how many steps to turn the dome 360 degrees
        rc=WriteString("t\n");
        if(rc < 0) return;

      }
      //  lets see how far off we were on last home detect
      rc=WriteString("o\n");
      if(rc < 0) return;
      //  if we have not already fetched this value
      //  fetch it for interest sake
      if(StepsPerDomeTurn==0) {
        rc=WriteString("t\n");
        if(rc < 0) return;
      }
      //  if we have not fetched the home position, fetch it
      if(HomeAz==-1) {
	rc=WriteString("i\n");
        if(rc < 0) return;
      }
      //  get shutter status
      rc=WriteString("u\n");
      if(rc < 0) return;
      //  get shutter position if shutter is talking to us
      if(ShutterState != 0) {
        rc=WriteString("b\n");
        if(rc < 0) return;
      }
      //  if the jog switch is set, unset it
      if((DomeMotionS[0].s == ISS_ON)||(DomeMotionS[1].s==ISS_ON)) {
	DomeMotionS[0].s=ISS_OFF;
	DomeMotionS[1].s=ISS_OFF;
	IDSetSwitch(&DomeMotionSP,nullptr);
      } 
      
    } else {
      //fprintf(stderr,"In Motion\n");
    }

    //  we only need to query this once at startup
    if(DomeReversed==-1) {
      rc=WriteString("y\n");
      if(rc < 0) return;
    }
    if(!HaveFirmwareVersion) {
      rc=WriteString("v\n");
      if(rc < 0) return;
    }

    //  Not all mounts update ra/dec constantly if tracking co-ordinates
    //  This is to ensure our alt/az gets updated even if ra/dec isn't being updated
    //  Once every 10 seconds is more than sufficient
    //  with this added, NexDome will now correctly track telescope simulator
    //  which does not emit new ra/dec co-ords if they are not changing
    if(TimeSinceUpdate++ > 9) {
	TimeSinceUpdate=0;
	UpdateMountCoords();
    }
    return;
}

IPState NexDome::MoveAbs(double az)
{
    char buf[30];
    //int rc;

    if(!MotorPower) {
      IDSetNumber(&BatteryLevelNP,"Cannot move dome without motor power");
      return IPS_ALERT;
    }
    //  Just write the string
    //  Our main reader loop will check any returns
    sprintf(buf,"g %3.1f\n",az);
    //fprintf(stderr,"Sending %s",buf);
    WriteString(buf);
    
      DomeAbsPosNP.s=IPS_BUSY;
      IDSetNumber(&DomeAbsPosNP,NULL);

    return IPS_BUSY;

}

IPState NexDome::Move(DomeDirection dir, DomeMotionCommand operation)
{
   double target;

   if (operation == MOTION_START)
    {
        target=DomeAbsPosN[0].value;
        if(dir==DOME_CW) {
	  target+= 5;
	} else {
          target-=5;
	}
	if(target < 0) target+=360;
        if(target >= 360) target-=360;
    }
    else
    {
        target=DomeAbsPosN[0].value;
    }
    MoveAbs(target);

    //IDSetNumber(&DomeAbsPosNP, nullptr);
    return ((operation == MOTION_START) ? IPS_BUSY : IPS_OK);

}

IPState NexDome::Park()
{
    //fprintf(stderr,"Park function\n");
    if(!MotorPower) {
      IDSetNumber(&BatteryLevelNP,"Cannot park with motor unpowered");
      return IPS_ALERT;
    }
    MoveAbs(GetAxis1Park());
    return IPS_BUSY;
}

IPState NexDome::UnPark()
{
    if(!MotorPower) {
      IDSetNumber(&BatteryLevelNP,"Cannot unpark with motor unpowered");
      return IPS_ALERT;
    }
    return IPS_OK;
    //return Dome::ControlShutter(SHUTTER_OPEN);
}

IPState NexDome::ControlShutter(ShutterOperation operation)
{
    int bytesWritten;

    if(ShutterState==0) {
      //  we are not talking to the shutter
      //  so return an error
      return IPS_ALERT;
    }

    if(operation==SHUTTER_OPEN) {
      if(shutterState==SHUTTER_OPENED) return IPS_OK;
      shutterState=SHUTTER_MOVING;
      tty_write_string(PortFD,"d\n",&bytesWritten);
    }
    if(operation==SHUTTER_CLOSE) {
      if(shutterState==SHUTTER_CLOSED) return IPS_OK;
      tty_write_string(PortFD,"e\n",&bytesWritten);
      shutterState=SHUTTER_MOVING;
    }

    return IPS_BUSY;
}

bool NexDome::Abort()
{
    int bytesWritten;
    tty_write_string(PortFD,"a\n",&bytesWritten);
/*
    // If we abort while in the middle of opening/closing shutter, alert.
    if (DomeShutterSP.s == IPS_BUSY)
    {
        DomeShutterSP.s = IPS_ALERT;
        IDSetSwitch(&DomeShutterSP, "Shutter operation aborted. Status: unknown.");
        return false;
    }
*/
    return true;
}

bool NexDome::SetCurrentPark()
{
    SetAxis1Park(DomeAbsPosN[0].value);
    return true;
}

bool NexDome::SetDefaultPark()
{
    // default park position is pointed south
    SetAxis1Park(180);
    return true;
}

