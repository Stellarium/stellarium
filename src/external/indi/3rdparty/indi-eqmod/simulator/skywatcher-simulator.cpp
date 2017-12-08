
#include "skywatcher-simulator.h"

#include <indidevapi.h>

#include <string.h>

void SkywatcherSimulator::send_byte(unsigned char c)
{
    reply[replyindex++] = c;
}

void SkywatcherSimulator::send_string(const char *s)
{
    while (*s)
    {
        send_byte(*s++);
    }
}

char SkywatcherSimulator::hexa[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

void SkywatcherSimulator::send_u24(unsigned int n)
{
    send_byte(hexa[(n & 0xF0) >> 4]);
    send_byte(hexa[(n & 0x0F)]);
    send_byte(hexa[(n & 0xF000) >> 12]);
    send_byte(hexa[(n & 0x0F00) >> 8]);
    send_byte(hexa[(n & 0xF00000) >> 20]);
    send_byte(hexa[(n & 0x0F0000) >> 16]);
}

void SkywatcherSimulator::send_u12(unsigned int n)
{
    send_byte(hexa[(n & 0xF0) >> 4]);
    send_byte(hexa[(n & 0x0F)]);
    send_byte(hexa[(n & 0xF00) >> 8]);
}

void SkywatcherSimulator::send_u8(unsigned char n)
{
    send_byte(hexa[(n & 0xF0) >> 4]);
    send_byte(hexa[(n & 0x0F)]);
}

unsigned int SkywatcherSimulator::get_u8(const char *cmd)
{
    unsigned int res = 0;
    res |= HEX(cmd[3]);
    res <<= 4;
    res |= HEX(cmd[4]);
    read += 2;
    return res;
}

unsigned int SkywatcherSimulator::get_u24(const char *cmd)
{
    unsigned int res = 0;
    res              = HEX(cmd[7]);
    res <<= 4;
    res |= HEX(cmd[8]);
    res <<= 4;
    res |= HEX(cmd[5]);
    res <<= 4;
    res |= HEX(cmd[6]);
    res <<= 4;
    res |= HEX(cmd[3]);
    res <<= 4;
    res |= HEX(cmd[4]);
    read += 6;
    return res;
}

void SkywatcherSimulator::setupVersion(const char *mcversion)
{
    version = mcversion;
}
void SkywatcherSimulator::setupRA(unsigned int nb_teeth, unsigned int gear_ratio_num, unsigned int gear_ratio_den,
                                  unsigned int nb_steps, unsigned int nb_microsteps, unsigned int highspeed)
{
    ra_nb_teeth        = nb_teeth;
    ra_gear_ratio_num  = gear_ratio_num;
    ra_gear_ratio_den  = gear_ratio_den;
    ra_nb_steps        = nb_steps;
    ra_microsteps      = nb_microsteps;
    ra_highspeed_ratio = ra_microsteps / highspeed;

    ra_steps_360  = (ra_nb_teeth * ra_gear_ratio_num * ra_nb_steps * ra_microsteps) / ra_gear_ratio_den;
    ra_steps_worm = (ra_gear_ratio_num * ra_nb_steps * ra_microsteps) / ra_gear_ratio_den;

    ra_position       = 0x800000;
    ra_microstep      = 0x00;
    ra_pwmindex       = ra_microsteps - 1;
    ra_wormperiod     = 0x256;
    ra_target         = 0x000001;
    ra_target_current = 0x000000;
    ra_target_slow    = 400;
    ra_breaks         = 400;

    ra_status = 0X0010; // lowspeed, forward, slew mode, stopped
    gettimeofday(&lastraTime, NULL);
    //IDLog("Simulator setupRA %d %d\n", ra_steps_360, ra_steps_worm);
}
void SkywatcherSimulator::setupDE(unsigned int nb_teeth, unsigned int gear_ratio_num, unsigned int gear_ratio_den,
                                  unsigned int nb_steps, unsigned int nb_microsteps, unsigned int highspeed)
{
    de_nb_teeth        = nb_teeth;
    de_gear_ratio_num  = gear_ratio_num;
    de_gear_ratio_den  = gear_ratio_den;
    de_nb_steps        = nb_steps;
    de_microsteps      = nb_microsteps;
    de_highspeed_ratio = de_microsteps / highspeed;

    de_steps_360  = (de_nb_teeth * de_gear_ratio_num * de_nb_steps * de_microsteps) / de_gear_ratio_den;
    de_steps_worm = (de_gear_ratio_num * de_nb_steps * de_microsteps) / de_gear_ratio_den;

    de_position       = 0x800000;
    de_microstep      = 0x00;
    de_pwmindex       = de_microsteps - 1;
    de_wormperiod     = 0x256;
    de_target         = 0x000001;
    de_target_current = 0x000000;
    de_target_slow    = 400;
    de_breaks         = 400;

    de_status = 0X0010; // lowspeed, forward, slew mode, stopped
                        //IDLog("Simulator setupDE %d %d\n", de_steps_360, de_steps_worm);
}

void SkywatcherSimulator::compute_timer_ra(unsigned int wormperiod)
{
    unsigned long n = (wormperiod * MUL_RA);
    n += ((wormperiod * REM_RA) / ra_steps_worm);
    ra_period = n;
    //IDLog("Simulator RA Worm period = %d, Microstep timer period = %d µs\n", wormperiod, ra_period);
}

void SkywatcherSimulator::compute_timer_de(unsigned int wormperiod)
{
    unsigned long n = (wormperiod * MUL_DE);
    n += ((wormperiod * REM_DE) / de_steps_worm);
    de_period = n;
    //IDLog("Simulator DE Worm period = %d, Microstep timer period = %d µs\n", wormperiod, de_period);
}

void SkywatcherSimulator::compute_ra_position()
{
    struct timeval raTime, resTime;
    gettimeofday(&raTime, NULL);
    timersub(&raTime, &lastraTime, &resTime);
    if (GETMOTORPROPERTY(ra_status, RUNNING))
    {
        unsigned int stepmul = (GETMOTORPROPERTY(ra_status, HIGHSPEED)) ? ra_highspeed_ratio : 1;
        unsigned int deltastep;
        deltastep = ((((resTime.tv_sec * MICROSECONDS) + resTime.tv_usec) * stepmul) / ra_period);
        if (!(GETMOTORPROPERTY(ra_status, SLEWMODE)))
        { // GOTO
            if ((GETMOTORPROPERTY(ra_status, HIGHSPEED)) &&
                (ra_target_current + deltastep >= ra_target - ra_target_slow))
            {
                struct timeval hstime, lstime;
                //hstime: time moving @ highspeed
                hstime.tv_sec =
                    ((ra_target - ra_target_slow - ra_target_current) * ra_period) / (MICROSECONDS * stepmul);
                hstime.tv_usec =
                    (((ra_target - ra_target_slow - ra_target_current) * ra_period) / stepmul) % MICROSECONDS;
                timersub(&resTime, &hstime, &lstime);
                UNSETMOTORPROPERTY(ra_status, HIGHSPEED); //switch to low speed
                deltastep = (ra_target - ra_target_slow - ra_target_current) +
                            ((((lstime.tv_sec * MICROSECONDS) + lstime.tv_usec)) / ra_period);
            }

            if (ra_target_current + deltastep >= ra_target)
            {
                deltastep = ra_target - ra_target_current;
                SETMOTORPROPERTY(ra_status, SLEWMODE);
                ra_pause();
            }
            else
                ra_target_current += deltastep;
        }
        if (GETMOTORPROPERTY(ra_status, BACKWARD)) //BACKWARD
            ra_position = ra_position - deltastep;
        else
            ra_position = ra_position + deltastep;
    }
    lastraTime = raTime;
}

void SkywatcherSimulator::compute_de_position()
{
    struct timeval deTime, resTime;
    gettimeofday(&deTime, NULL);
    timersub(&deTime, &lastdeTime, &resTime);
    if (GETMOTORPROPERTY(de_status, RUNNING))
    {
        unsigned int stepmul = (GETMOTORPROPERTY(de_status, HIGHSPEED)) ? de_highspeed_ratio : 1;
        unsigned int deltastep;
        deltastep = ((((resTime.tv_sec * MICROSECONDS) + resTime.tv_usec) * stepmul) / de_period);
        if (!(GETMOTORPROPERTY(de_status, SLEWMODE)))
        { // GOTO
            if ((GETMOTORPROPERTY(de_status, HIGHSPEED)) &&
                (de_target_current + deltastep >= de_target - de_target_slow))
            {
                struct timeval hstime, lstime;
                //hstime: time moving @ highspeed
                hstime.tv_sec =
                    ((de_target - de_target_slow - de_target_current) * de_period) / (MICROSECONDS * stepmul);
                hstime.tv_usec =
                    (((de_target - de_target_slow - de_target_current) * de_period) / stepmul) % MICROSECONDS;
                timersub(&resTime, &hstime, &lstime);
                UNSETMOTORPROPERTY(de_status, HIGHSPEED); //switch to low speed
                deltastep = (de_target - de_target_slow - de_target_current) +
                            ((((lstime.tv_sec * MICROSECONDS) + lstime.tv_usec)) / de_period);
            }
            if (de_target_current + deltastep >= de_target)
            {
                deltastep = de_target - de_target_current;
                SETMOTORPROPERTY(de_status, SLEWMODE);
                de_pause();
            }
            else
                de_target_current += deltastep;
        }
        if (GETMOTORPROPERTY(de_status, BACKWARD)) //BACKWARD
            de_position = de_position - deltastep;
        else
            de_position = de_position + deltastep;
    }
    lastdeTime = deTime;
}

void SkywatcherSimulator::ra_resume()
{
    gettimeofday(&lastraTime, NULL);
    compute_timer_ra(ra_wormperiod);
    //GOTO
    if (!(GETMOTORPROPERTY(ra_status, SLEWMODE)))
        ra_target_current = 0;

    SETMOTORPROPERTY(ra_status, RUNNING);
}

void SkywatcherSimulator::de_resume()
{
    gettimeofday(&lastdeTime, NULL);
    compute_timer_de(de_wormperiod);
    //GOTO
    if (!(GETMOTORPROPERTY(de_status, SLEWMODE)))
        de_target_current = 0;

    SETMOTORPROPERTY(de_status, RUNNING);
}

void SkywatcherSimulator::ra_pause()
{
    UNSETMOTORPROPERTY(ra_status, RUNNING);
}

void SkywatcherSimulator::ra_stop()
{
    UNSETMOTORPROPERTY(ra_status, RUNNING);
}

void SkywatcherSimulator::de_pause()
{
    UNSETMOTORPROPERTY(de_status, RUNNING);
}

void SkywatcherSimulator::de_stop()
{
    UNSETMOTORPROPERTY(de_status, RUNNING);
}

void SkywatcherSimulator::process_command(const char *cmd, int *received)
{
    replyindex = 0;
    read       = 1;
    if (cmd[0] != ':')
    {
        send_byte('!');
        goto next_cmd;
    }
    switch (cmd[1])
    {
        /** Syntrek Protocol **/
        case 'e': // Get firmware version
            if (cmd[2] != '1')
            {
                goto cant_do;
            }
            else
            {
                send_byte('=');
                send_string(version);
            }
            break;
        case 'a': // Get number of microsteps per revolution
            if (cmd[2] == '1')
            {
                send_byte('=');
                send_u24(ra_steps_360);
            }
            else if (cmd[2] == '2')
            {
                send_byte('=');
                send_u24(de_steps_360);
            }
            else
                goto cant_do;
            break;
        case 'b': // Get number of microsteps per revolution
            if (cmd[2] == '1')
            {
                send_byte('=');
                send_u24(ra_steps_worm);
            }
            else if (cmd[2] == '2')
            {
                send_byte('=');
                send_u24(de_steps_worm);
            }
            else
                goto cant_do;
            break;
        case 'D': // Get Worm period
            if (cmd[2] == '1')
            {
                send_byte('=');
                send_u24(ra_wormperiod);
            }
            else if (cmd[2] == '2')
            {
                send_byte('=');
                send_u24(de_wormperiod);
            }
            else
                goto cant_do;
            break;
        case 'f': // Get motor status
            if (cmd[2] == '1')
            {
                send_byte('=');
                send_u12(ra_status);
            }
            else if (cmd[2] == '2')
            {
                send_byte('=');
                send_u12(de_status);
            }
            else
                goto cant_do;
            break;
        case 'j': // Get encoder values
            if (cmd[2] == '1')
            {
                compute_ra_position();
                send_byte('=');
                send_u24(ra_position);
            }
            else if (cmd[2] == '2')
            {
                compute_de_position();
                send_byte('=');
                send_u24(de_position);
            }
            else
                goto cant_do;
            break;
        case 'g': // get high speed ratio
            if (cmd[2] == '1')
            {
                send_byte('=');
                send_u24(ra_highspeed_ratio);
            }
            else if (cmd[2] == '2')
            {
                send_byte('=');
                send_u24(de_highspeed_ratio);
            }
            else
                goto cant_do;
            break;
        case 'E': // Set encoder values
            if (cmd[2] == '1')
            {
                ra_position = get_u24(cmd);
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_position = get_u24(cmd);
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'F': // Initialize & activate motors
            if ((cmd[2] != '1') && (cmd[2] != '2') && (cmd[2] != '3'))
                goto cant_do;
            //init_pwm();
            //init_timers();
            if ((cmd[2] == '1') || (cmd[2] == '3'))
            {
                //SETRAPWMA(pwm_table[ra_pwmindex]); SETRAPWMB(pwm_table[MICROSTEPS - 1 - ra_pwmindex]);
                //SETRAPHIA(ra_microstep); SETRAPHIB(ra_microstep);
                SETMOTORPROPERTY(ra_status, INITIALIZED);
                //RAENABLE = 1;
            }
            if ((cmd[2] == '2') || (cmd[2] == '3'))
            {
                //SETDEPWMA(pwm_table[de_pwmindex]); SETDEPWMB(pwm_table[MICROSTEPS - 1 - de_pwmindex]);
                //SETDEPHIA(de_microstep); SETDEPHIB(de_microstep);
                SETMOTORPROPERTY(de_status, INITIALIZED);
                //DEENABLE = 1;
            }
            send_byte('=');
            break;

        case 'J': // Start motor
            if (cmd[2] == '1')
            {
                ra_resume();
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_resume();
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'K': // Stop motor
            if (cmd[2] == '1')
            {
                ra_pause();
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_pause();
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'L': // Instant Stop motor
            if (cmd[2] == '1')
            {
                ra_stop();
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_stop();
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'I': // Set Speed
            if (cmd[2] == '1')
            {
                ra_wormperiod = get_u24(cmd);
                if (GETMOTORPROPERTY(ra_status, RUNNING))
                    compute_timer_ra(ra_wormperiod);
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_wormperiod = get_u24(cmd);
                if (GETMOTORPROPERTY(de_status, RUNNING))
                    compute_timer_de(de_wormperiod);
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'G': // Set Mode/Direction
            if ((cmd[2] != '1') && (cmd[2] != '2'))
                goto cant_do;
            if (cmd[2] == '1')
            {
                unsigned char modedir = get_u8(cmd);
                if (modedir & 0x0F)
                    SETMOTORPROPERTY(ra_status, BACKWARD);
                else
                    UNSETMOTORPROPERTY(ra_status, BACKWARD);
                modedir = (modedir >> 4);
                if (modedir == 0)
                {
                    UNSETMOTORPROPERTY(ra_status, SLEWMODE);
                    SETMOTORPROPERTY(ra_status, HIGHSPEED);
                }
                else if (modedir == 1)
                {
                    SETMOTORPROPERTY(ra_status, SLEWMODE);
                    UNSETMOTORPROPERTY(ra_status, HIGHSPEED);
                }
                else if (modedir == 2)
                {
                    UNSETMOTORPROPERTY(ra_status, SLEWMODE);
                    UNSETMOTORPROPERTY(ra_status, HIGHSPEED);
                }
                else if (modedir == 3)
                {
                    SETMOTORPROPERTY(ra_status, SLEWMODE);
                    SETMOTORPROPERTY(ra_status, HIGHSPEED);
                }
            }
            if (cmd[2] == '2')
            {
                unsigned char modedir = get_u8(cmd);
                if (modedir & 0x0F)
                    SETMOTORPROPERTY(de_status, BACKWARD);
                else
                    UNSETMOTORPROPERTY(de_status, BACKWARD);
                modedir = (modedir >> 4);
                if (modedir == 0)
                {
                    UNSETMOTORPROPERTY(de_status, SLEWMODE);
                    SETMOTORPROPERTY(de_status, HIGHSPEED);
                }
                else if (modedir == 1)
                {
                    SETMOTORPROPERTY(de_status, SLEWMODE);
                    UNSETMOTORPROPERTY(de_status, HIGHSPEED);
                }
                else if (modedir == 2)
                {
                    UNSETMOTORPROPERTY(de_status, SLEWMODE);
                    UNSETMOTORPROPERTY(de_status, HIGHSPEED);
                }
                else if (modedir == 3)
                {
                    SETMOTORPROPERTY(de_status, SLEWMODE);
                    SETMOTORPROPERTY(de_status, HIGHSPEED);
                }
            }
            send_byte('=');
            break;
        case 'H': // Set Goto Target
            if (cmd[2] == '1')
            {
                ra_target = get_u24(cmd);
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_target = get_u24(cmd);
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'M': // Set Goto BreakSteps
            if (cmd[2] == '1')
            {
                ra_target_slow = get_u24(cmd);
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_target_slow = get_u24(cmd);
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'U': // Set BreakSteps
            if (cmd[2] == '1')
            {
                ra_breaks = get_u24(cmd);
                send_byte('=');
            }
            else if (cmd[2] == '2')
            {
                de_breaks = get_u24(cmd);
                send_byte('=');
            }
            else
                goto cant_do;
            break;
        case 'P': // Set ST4 guide Rate
            send_byte('=');
            break;
        default:
            goto cant_do;
    }
    read += 3; // 2 + '\r'
    goto next_cmd;
cant_do:
    send_byte('!'); // Can't execute command
next_cmd:
    send_byte('\x0d');
    reply[replyindex] = '\0';
    *received         = read;
}

void SkywatcherSimulator::get_reply(char *buf, int *len)
{
    strncpy(buf, reply, replyindex + 1);
    *len = replyindex;
}
