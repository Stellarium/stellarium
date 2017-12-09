/*******************************************************************************
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

#ifndef NexDome_H
#define NexDome_H

#include <indidome.h>

/*  Some headers we need */
#include <math.h>
#include <sys/time.h>


class NexDome : public INDI::Dome
{

    public:
        NexDome();
        virtual ~NexDome();

	virtual bool ISNewSwitch(const char *dev,const char *name,ISState *states, char *names[],int n);
	virtual bool ISNewNumber(const char *dev,const char *name,double values[],char *names[],int n);
        virtual bool initProperties();
        const char *getDefaultName();
        bool updateProperties();


protected:

	ISwitch HomeS[1];
	ISwitchVectorProperty HomeSP;
	ISwitch CalibrateS[1];
	ISwitchVectorProperty CalibrateSP;
	INumber SyncPositionN[1];
	INumberVectorProperty SyncPositionNP;
	INumberVectorProperty HomePositionNP;
	INumber HomePositionN[1];
	INumber BatteryLevelN[2];
	INumberVectorProperty BatteryLevelNP;
	IText FirmwareVersionT[1];
        ITextVectorProperty FirmwareVersionTP;
	ISwitch ReversedS[2];
	ISwitchVectorProperty ReversedSP;


        //bool Connect();
	bool Handshake();
        bool Disconnect();

        void TimerHit();

        virtual IPState Move(DomeDirection dir, DomeMotionCommand operation);
        //virtual IPState MoveRel(double azDiff);
        virtual IPState MoveAbs(double az);        
        virtual IPState Park();
        virtual IPState UnPark();
        virtual IPState ControlShutter(ShutterOperation operation);
        virtual bool Abort();

        // Parking
        virtual bool SetCurrentPark();
        virtual bool SetDefaultPark();


    private:
        float BatteryMain;
	float BatteryShutter;
        float ShutterPosition;
	bool MotorPower;
	float HomeError;
	float HomeAz;
	float StoredPark;
	time_t CalStartTime;
	int ShutterState;
	int DomeReversed;
        //int LastShutterState;
	int StepsPerDomeTurn;
	int ReadString(char *,int);
        int WriteString(const char *);
        void ProcessDomeMessage(char *);
	void ReadDomeStatus();
	bool SetupParms();
	//int PortFD;
	bool InMotion;
	bool AtHome;
	bool Calibrating;
	int TimeSinceUpdate;
	int ClearSerialBuffers();
	bool HaveFirmwareVersion;

};

#endif
