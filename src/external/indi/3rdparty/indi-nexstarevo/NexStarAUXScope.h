
#pragma once

#include <vector>
#include <netinet/in.h>

typedef std::vector<unsigned char> buffer;

enum AUXCommands
{
    MC_GET_POSITION      = 0x01,
    MC_GOTO_FAST         = 0x02,
    MC_SET_POSITION      = 0x04,
    MC_SET_POS_GUIDERATE = 0x06,
    MC_SET_NEG_GUIDERATE = 0x07,
    MC_LEVEL_START       = 0x0b,
    MC_SLEW_DONE         = 0x13,
    MC_GOTO_SLOW         = 0x17,
    MC_SEEK_INDEX        = 0x19,
    MC_MOVE_POS          = 0x24,
    MC_MOVE_NEG          = 0x25,
    GET_VER              = 0xfe,
    GPS_GET_LAT          = 0x01,
    GPS_GET_LONG         = 0x02,
    GPS_GET_DATE         = 0x03,
    GPS_GET_YEAR         = 0x04,
    GPS_GET_TIME         = 0x33,
    GPS_TIME_VALID       = 0x36,
    GPS_LINKED           = 0x37
};

enum AUXtargets
{
    ANY   = 0x00,
    MB    = 0x01,
    HC    = 0x04,
    HCP   = 0x0d,
    AZM   = 0x10,
    ALT   = 0x11,
    APP   = 0x20,
    GPS   = 0xb0,
    WiFi  = 0xb5,
    BAT   = 0xb6,
    CHG   = 0xb7,
    LIGHT = 0xbf
};

#define NSEVO_DEFAULT_IP   "1.2.3.4"
#define NSEVO_DEFAULT_PORT 2000

class AUXCommand
{
  public:
    AUXCommand();
    AUXCommand(buffer buf);
    AUXCommand(AUXCommands c, AUXtargets s, AUXtargets d, buffer dat);
    AUXCommand(AUXCommands c, AUXtargets s, AUXtargets d);

    void fillBuf(buffer &buf);
    void parseBuf(buffer buf);
    long getPosition();
    void setPosition(long p);
    void setPosition(double p);
    void setRate(unsigned char r);
    unsigned char checksum(buffer buf);
    void dumpCmd();

    AUXCommands cmd;
    AUXtargets src, dst;
    int len;
    buffer data;
    bool valid;
};

class NexStarAUXScope
{
  public:
    NexStarAUXScope(char const *ip, int port);
    NexStarAUXScope(int port);
    NexStarAUXScope(char const *ip);
    NexStarAUXScope();
    ~NexStarAUXScope();
    bool Abort();
    long GetALT();
    long GetAZ();
    bool slewing();
    bool Slew(AUXtargets trg, int rate);
    bool SlewALT(int rate);
    bool SlewAZ(int rate);
    bool GoToFast(long alt, long az, bool track);
    bool GoToSlow(long alt, long az, bool track);
    bool Track(long altRate, long azRate);
    bool TimerTick(double dt);
    bool UpdateLocation(double lat, double lon, double elev);
    bool Park();
    bool Connect(int PortFD);
    bool Disconnect();

  private:
    static const long STEPS_PER_REVOLUTION = 16777216;
    void initScope(char const *ip, int port);
    void initScope();
    bool detectScope();
    void closeConnection();
    void emulateGPS(AUXCommand &m);
    void readMsgs();
    void processCmd(AUXCommand &cmd);
    void querryStatus();
    bool sendCmd(AUXCommand &c);
    double Lat, Lon, Elv;
    long Alt;
    long Az;
    long AltRate;
    long AzRate;
    long targetAlt;
    long targetAz;
    long slewRate;
    bool tracking;
    bool slewingAlt, slewingAz;
    int sock;
    struct sockaddr_in addr;
    bool simulator = false;
};
