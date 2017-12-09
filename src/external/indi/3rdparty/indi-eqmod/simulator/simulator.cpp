
#include "simulator.h"

#include <string.h>

EQModSimulator::EQModSimulator(INDI::Telescope *t)
{
    telescope = t;
}

void EQModSimulator::Connect()
{
    ISwitch *sw           = IUFindOnSwitch(SimModeSP);
    sksim                 = new SkywatcherSimulator();
    if (!strcmp(sw->name, "SIM_EQ6"))
    {
        sksim->setupVersion("020300");
        sksim->setupRA(180, 47, 12, 200, 64, 2);
        sksim->setupDE(180, 47, 12, 200, 64, 2);
    }
    else
    {
        if (!strcmp(sw->name, "SIM_HEQ5"))
        {
            sksim->setupVersion("020301");
            sksim->setupRA(135, 47, 9, 200, 64, 2);
            sksim->setupDE(135, 47, 9, 200, 64, 2);
        }
        else
        {
            if (!strcmp(sw->name, "SIM_NEQ5"))
            {
                sksim->setupVersion("020302");
                sksim->setupRA(144, 44, 9, 200, 32, 2);
                sksim->setupDE(144, 44, 9, 200, 32, 2);
            }
            else
            {
                if (!strcmp(sw->name, "SIM_NEQ3"))
                {
                    sksim->setupVersion("020303");
                    sksim->setupRA(130, 55, 10, 200, 32, 2);
                    sksim->setupDE(130, 55, 10, 200, 32, 2);
                }
                else
                {
                    if (!strcmp(sw->name, "SIM_GEEHALEL"))
                    {
                        sksim->setupVersion("0203F0");
                        sksim->setupRA(144, 60, 15, 400, 8, 1);
                        sksim->setupDE(144, 60, 10, 400, 8, 1);
                    }
                    else
                    {
                        if (!strcmp(sw->name, "SIM_CUSTOM"))
                        {
                            double teeth, num, den, steps, microsteps, highspeed;
                            ISwitch *hssw = IUFindOnSwitch(SimHighSpeedSP);
                            sksim->setupVersion(IUFindText(SimMCVersionTP, "SIM_MCPHRASE")->text);
                            teeth      = IUFindNumber(SimWormNP, "RA_TEETH")->value;
                            num        = IUFindNumber(SimRatioNP, "RA_RATIO_NUM")->value;
                            den        = IUFindNumber(SimRatioNP, "RA_RATIO_DEN")->value;
                            steps      = IUFindNumber(SimMotorNP, "RA_MOTOR_STEPS")->value;
                            microsteps = IUFindNumber(SimMotorNP, "RA_MOTOR_USTEPS")->value;
                            highspeed  = 1;
                            if (!strcmp(hssw->name, "SIM_HALFSTEP"))
                                highspeed = 2;
                            sksim->setupRA(teeth, num, den, steps, microsteps, highspeed);

                            teeth      = IUFindNumber(SimWormNP, "DE_TEETH")->value;
                            num        = IUFindNumber(SimRatioNP, "DE_RATIO_NUM")->value;
                            den        = IUFindNumber(SimRatioNP, "DE_RATIO_DEN")->value;
                            steps      = IUFindNumber(SimMotorNP, "DE_MOTOR_STEPS")->value;
                            microsteps = IUFindNumber(SimMotorNP, "DE_MOTOR_USTEPS")->value;
                            sksim->setupDE(teeth, num, den, steps, microsteps, highspeed);
                        }
                    }
                }
            }
        }
    }
}

void EQModSimulator::receive_cmd(const char *cmd, int *received)
{
    // *received=0;
    if (sksim)
        sksim->process_command(cmd, received);
}

void EQModSimulator::send_reply(char *buf, int *sent)
{
    if (sksim)
        sksim->get_reply(buf, sent);
    //strncpy(buf,"=\r", 2);
    //*sent=2;
}

bool EQModSimulator::initProperties()
{
    telescope->buildSkeleton("indi_eqmod_simulator_sk.xml");

    SimWormNP      = telescope->getNumber("SIMULATORWORM");
    SimRatioNP     = telescope->getNumber("SIMULATORRATIO");
    SimMotorNP     = telescope->getNumber("SIMULATORMOTOR");
    SimModeSP      = telescope->getSwitch("SIMULATORMODE");
    SimHighSpeedSP = telescope->getSwitch("SIMULATORHIGHSPEED");
    SimMCVersionTP = telescope->getText("SIMULATORMCVERSION");

    return true;
}

bool EQModSimulator::updateProperties(bool enable)
{
    if (enable)
    {
        telescope->defineSwitch(SimModeSP);
        telescope->defineNumber(SimWormNP);
        telescope->defineNumber(SimRatioNP);
        telescope->defineNumber(SimMotorNP);
        telescope->defineSwitch(SimHighSpeedSP);
        telescope->defineText(SimMCVersionTP);

        defined = true;
        /*
      AlignDataFileTP=telescope->getText("ALIGNDATAFILE");
      AlignDataBP=telescope->getBLOB("ALIGNDATA");
      */
    }
    else if (defined)
    {
        telescope->deleteProperty(SimModeSP->name);
        telescope->deleteProperty(SimWormNP->name);
        telescope->deleteProperty(SimRatioNP->name);
        telescope->deleteProperty(SimMotorNP->name);
        telescope->deleteProperty(SimHighSpeedSP->name);
        telescope->deleteProperty(SimMCVersionTP->name);
    }

    return true;
}

bool EQModSimulator::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        INumberVectorProperty *nvp = telescope->getNumber(name);
        if ((nvp != SimWormNP) && (nvp != SimRatioNP) & (nvp != SimMotorNP))
            return false;
        if (telescope->isConnected())
        {
            DEBUGDEVICE(telescope->getDeviceName(), INDI::Logger::DBG_WARNING,
                        "Can not change simulation settings when mount is already connected");
            return false;
        }

        nvp->s = IPS_OK;
        IUUpdateNumber(nvp, values, names, n);
        IDSetNumber(nvp, NULL);
        return true;
    }
    return false;
}

bool EQModSimulator::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    //  first check if it's for our device

    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        ISwitchVectorProperty *svp = telescope->getSwitch(name);
        if ((svp != SimModeSP) && (svp != SimHighSpeedSP))
            return false;
        if (telescope->isConnected())
        {
            DEBUGDEVICE(telescope->getDeviceName(), INDI::Logger::DBG_WARNING,
                        "Can not change simulation settings when mount is already connected");
            return false;
        }

        svp->s = IPS_OK;
        IUUpdateSwitch(svp, states, names, n);
        IDSetSwitch(svp, NULL);
        return true;
    }
    return false;
}

bool EQModSimulator::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //  first check if it's for our device

    if (strcmp(dev, telescope->getDeviceName()) == 0)
    {
        ITextVectorProperty *tvp = telescope->getText(name);
        if (tvp)
        {
            if ((tvp != SimMCVersionTP))
                return false;
            if (telescope->isConnected())
            {
                DEBUGDEVICE(telescope->getDeviceName(), INDI::Logger::DBG_WARNING,
                            "Can not change simulation settings when mount is already connected");
                return false;
            }

            tvp->s = IPS_OK;
            IUUpdateText(tvp, texts, names, n);
            IDSetText(tvp, NULL);
            return true;
        }
    }
    return false;
}
