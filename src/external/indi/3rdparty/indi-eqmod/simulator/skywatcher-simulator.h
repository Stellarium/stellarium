
#pragma once

#include <sys/time.h>

/* Microstepping */
/* 8 microsteps */
#define MICROSTEP_MASK 0x07
// PWM_MASK = 0x1 << LN(MICROSTEPS)
#define PWM_MASK 0x08
// WINDINGB_MASK = 0x2 << LN(MICROSTEPS)
#define WINDINGB_MASK 0x10
// WINDINGA_MASK = 0x3 << LN(MICROSTEPS)
#define WINDINGA_MASK 0x18

#define SETRAPHIA(microstep) \
    RAPHIA = ((((microstep & WINDINGA_MASK) == 0) || ((microstep & WINDINGA_MASK) == WINDINGA_MASK)) ? 1 : 0)
#define SETRAPHIB(microstep) RAPHIB = ((microstep & WINDINGB_MASK) ? 1 : 0)

#define SETDEPHIA(microstep) \
    DEPHIA = (((microstep & WINDINGA_MASK) == 0) || ((microstep & WINDINGA_MASK) == WINDINGA_MASK)) ? 1 : 0
#define SETDEPHIB(microstep) DEPHIB = (microstep & WINDINGB_MASK) ? 1 : 0

/* Timer period */
#define MICROSECONDS 1000000
#define MUL_RA       (MICROSECONDS / ra_steps_worm)
#define REM_RA       (MICROSECONDS % ra_steps_worm)
#define MUL_DE       (MICROSECONDS / de_steps_worm)
#define REM_DE       (MICROSECONDS % de_steps_worm)

#define SETMOTORPROPERTY(motorstatus, property)   motorstatus |= property
#define UNSETMOTORPROPERTY(motorstatus, property) motorstatus &= ~property
#define GETMOTORPROPERTY(motorstatus, property)   (motorstatus & property)

#define HEX(c) (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

class SkywatcherSimulator
{
  public:
    void setupVersion(const char *mcversion);
    void setupRA(unsigned int nb_teeth, unsigned int gear_ratio_num, unsigned int gear_ratio_den, unsigned int nb_steps,
                 unsigned int nb_microsteps, unsigned int highspeed);
    void setupDE(unsigned int nb_teeth, unsigned int gear_ratio_num, unsigned int gear_ratio_den, unsigned int nb_steps,
                 unsigned int nb_microsteps, unsigned int highspeed);

    void process_command(const char *cmd, int *received);
    void get_reply(char *buf, int *len);

  protected:
  private:
    enum motorstatus
    {
        INITIALIZED = 0x0100,
        RUNNING     = 0x0001,
        SLEWMODE    = 0X0010,
        BACKWARD    = 0x0020,
        HIGHSPEED   = 0x0040
    };

    /* Gears/Worms/Steppers */
    const char *version;
    unsigned int ra_nb_teeth;
    unsigned int de_nb_teeth;
    unsigned int ra_gear_ratio_num;
    unsigned int ra_gear_ratio_den;
    unsigned int de_gear_ratio_num;
    unsigned int de_gear_ratio_den;
    unsigned int ra_nb_steps;
    unsigned int de_nb_steps;
    unsigned int ra_microsteps;
    unsigned int de_microsteps;
    unsigned int ra_highspeed_ratio;
    unsigned int de_highspeed_ratio;

    unsigned int ra_steps_360;
    unsigned int de_steps_360;
    unsigned int ra_steps_worm;
    unsigned int de_steps_worm;

    unsigned int ra_position;
    unsigned char ra_microstep;
    unsigned char ra_pwmindex;
    unsigned int ra_wormperiod;
    unsigned int ra_target;
    unsigned int ra_target_current;
    unsigned int ra_target_slow;
    unsigned int ra_breaks;

    unsigned int de_position;
    unsigned char de_microstep;
    unsigned char de_pwmindex;
    unsigned int de_wormperiod;
    unsigned int de_target;
    unsigned int de_target_current;
    unsigned int de_target_slow;
    unsigned int de_breaks;

    // Motor periods in ns
    unsigned int ra_period;
    unsigned int de_period;

    // Motor status (Skywatcher protocol)
    unsigned int ra_status;
    unsigned int de_status;

    // USART
    char reply[32];
    unsigned char replyindex;
    static char hexa[16];
    void send_byte(unsigned char c);
    void send_string(const char *s);
    void send_u24(unsigned int);
    void send_u12(unsigned int);
    void send_u8(unsigned char);
    unsigned char read;
    unsigned int get_u8(const char *cmd);
    unsigned int get_u24(const char *cmd);

    void compute_timer_ra(unsigned int wormperiod);
    void compute_timer_de(unsigned int wormperiod);
    void compute_ra_position();
    void compute_de_position();
    void ra_resume();
    void de_resume();
    void ra_pause();
    void ra_stop();
    void de_pause();
    void de_stop();

    struct timeval lastraTime;
    struct timeval lastdeTime;
};
