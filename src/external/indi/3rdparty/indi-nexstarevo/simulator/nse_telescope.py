#!/bin/env python3

import struct
import sys
from math import pi

import curses
from collections import deque
import binascii

# ID tables
targets={'ANY':0x00,
         'MB' :0x01,
         'HC' :0x04,
         'UKN1':0x05,
         'HC+':0x0d,
         'AZM':0x10,
         'ALT':0x11,
         'APP':0x20,
         'GPS':0xb0,
         'UKN2': 0xb4,
         'WiFi':0xb5,
         'BAT':0xb6,
         'CHG':0xb7,
         'LIGHT':0xbf
        }
trg_names={value:key for key, value in targets.items()}
        
control={
         'HC' :0x04,
         'HC+':0x0d,
         'APP':0x20,
        }

commands={
          'MC_GET_POSITION':0x01,
          'MC_GOTO_FAST':0x02,
          'MC_SET_POSITION':0x04,
          'MC_GET_???':0x05,
          'MC_SET_POS_GUIDERATE':0x06,
          'MC_SET_NEG_GUIDERATE':0x07,
          'MC_LEVEL_START':0x0b,
          'MC_SET_POS_BACKLASH':0x10,
          'MC_SET_NEG_BACKLASH':0x11,
          'MC_SLEW_DONE':0x13,
          'MC_GOTO_SLOW':0x17,
          'MC_AT_INDEX':0x18,
          'MC_SEEK_INDEX':0x19,
          'MC_SET_MAXRATE':0x20,
          'MC_GET_MAXRATE':0x21,
          'MC_ENABLE_MAXRATE':0x22,
          'MC_MAXRATE_ENABLED':0x23,
          'MC_MOVE_POS':0x24,
          'MC_MOVE_NEG':0x25,
          'MC_ENABLE_CORDWRAP':0x38,
          'MC_DISABLE_CORDWRAP':0x39,
          'MC_SET_CORDWRAP_POS':0x3a,
          'MC_POLL_CORDWRAP':0x3b,
          'MC_GET_CORDWRAP_POS':0x3c,
          'MC_GET_POS_BACKLASH':0x40,
          'MC_GET_NEG_BACKLASH':0x41,
          'MC_GET_AUTOGUIDE_RATE':0x47,
          'MC_GET_APPROACH':0xfc,
          'MC_SET_APPROACH':0xfd,
          'GET_VER':0xfe,
         }
cmd_names={value:key for key, value in commands.items()}

ACK_CMDS=[0x02,0x04,0x06,0x24,]

MC_ALT=0x11
MC_AZM=0x10

trg_cmds = {
    'BAT': {
        0x10:'GET_VOLTAGE',
        0x18:'GET_SET_CURRENT',
    },
    'CHG': {
        0x10: 'GET_SET_MODE',
    },
    'LIGHT': {
        0x10:'GET_SET_LEVEL',
    },
}

RATES = {
    0 : 0.0,
    1 : 1/(360*60),
    2 : 2/(360*60),
    3 : 5/(360*60),
    4 : 15/(360*60),
    5 : 30/(360*60),
    6 : 1/360,
    7 : 2/360,
    8 : 5/360,
    9 : 10/360
}

def print_command(cmd):
    if cmd[2] in (0x10, 0x20):
        try :
            return 'Command: %s->%s [%s] len:%d: data:%r' % (
                        trg_names[cmd[1]], 
                        trg_names[cmd[2]], 
                        cmd_names[cmd[3]], cmd[0], cmd[4:-1])
        except KeyError :
            pass
    try :
        return 'Command: %s->%s [%02x] len:%d: data:%r' % (
                    trg_names[cmd[1]], 
                    trg_names[cmd[2]], 
                    cmd[3], cmd[0], cmd[4:-1])
    except KeyError :
        pass
        
    return 'Command: %02x->%02x [%02x] len:%d: data:%r' % (
                cmd[1], cmd[2], cmd[3], cmd[0], cmd[4:-1])
            

def decode_command(cmd):
    return (cmd[3], cmd[1], cmd[2], cmd[0], cmd[4:-1], cmd[-1])

def split_cmds(data):
    # split the data to commands
    # the initial byte b'\03b' is removed from commands
    return [cmd for cmd in data.split(b';') if len(cmd)]

def make_checksum(data):
    return ((~sum([c for c in bytes(data)]) + 1) ) & 0xFF


def ack_cmd(cmd):
    c,f,t,l,d,cs=decode_command(cmd)
    rsp=b''
    if c in ACK_CMDS :
        rsp=b';\x03'+bytes((t,f,c))
        rsp+=bytes((make_checksum(rsp[1:]),))
    return rsp

def f2dms(f):
    '''
    Convert fraction of the full rotation to DMS triple (degrees).
    '''
    s= 1 if f>0 else -1
    d=360*abs(f)
    dd=int(d)
    mm=int((d-dd)*60)
    ss=(d-dd-mm/60)*3600
    return dd,mm,ss

def parse_pos(d):
    '''
    Parse first three bytes into the DMS string
    '''
    if len(d)>=3 :
        pos=struct.unpack('!i',b'\x00'+d[:3])[0]/2**24
        return u'%03d°%02d\'%04.1f"' % f2dms(pos)
    else :
        return u''

def repr_pos(alt,azm):
    return u'(%03d°%02d\'%04.1f", %03d°%02d\'%04.1f")' % (f2dms(alt) + f2dms(azm))

def repr_angle(a):
    return u'%03d°%02d\'%04.1f"' % f2dms(a)


def unpack_int3(d):
    return struct.unpack('!i',b'\x00'+d[:3])[0]/2**24

def pack_int3(f):
    return struct.pack('!i',int(f*(2**24)))[1:]
    
def unpack_int2(d):
    return struct.unpack('!i',b'\x00\x00'+d[:2])[0]

def pack_int2(v):
    return struct.pack('!i',int(v))[-2:]
    


class NexStarScope:
    
    __mcfw_ver=(7,11,5100//256,5100%256)
    __hcfw_ver=(5,28,5300//256,5300%256)
    
    trg=('MB', 'HC', 'UKN1', 'HC+', 'AZM', 'ALT', 'APP', 
            'GPS', 'WiFi', 'BAT', 'CHG', 'LIGHT')
    
    def __init__(self, ALT=0.0, AZM=0.0, tui=True, stdscr=None):
        self.tui=tui
        self.alt=ALT
        self.azm=AZM
        self.trg_alt=self.alt
        self.trg_azm=self.azm
        self.alt_rate=0
        self.azm_rate=0
        self.alt_approach=0
        self.azm_approach=0
        self.last_cmd=''
        self.slewing=False
        self.guiding=False
        self.goto=False
        self.alt_guiderate=0.0
        self.azm_guiderate=0.0
        self.alt_maxrate=4000
        self.azm_maxrate=4000
        self.use_maxrate=False
        self.cmd_log=deque(maxlen=30)
        self.msg_log=deque(maxlen=10)
        self.bat_current=2468
        self.bat_voltage=12345678
        self.lt_logo=64
        self.lt_tray=128
        self.lt_wifi=255
        self.charge=False
        self._other_handlers = {
            0x10: NexStarScope.cmd_0x10,
            0x18: NexStarScope.cmd_0x18,
            0xfe: NexStarScope.send_ack,
        }
        self._mc_handlers = {
          0x01 : NexStarScope.get_position,
          0x02 : NexStarScope.goto_fast,
          0x04 : NexStarScope.set_position,
          0x05 : NexStarScope.cmd_0x05,
          0x06 : NexStarScope.set_pos_guiderate,
          0x07 : NexStarScope.set_neg_guiderate,
          0x0b : NexStarScope.level_start,
          0x10 : NexStarScope.set_pos_backlash,
          0x11 : NexStarScope.set_neg_backlash,
          0x13 : NexStarScope.slew_done,
          0x17 : NexStarScope.goto_slow,
          0x18 : NexStarScope.at_index,
          0x19 : NexStarScope.seek_index,
          0x20 : NexStarScope.set_maxrate,
          0x21 : NexStarScope.get_maxrate,
          0x22 : NexStarScope.enable_maxrate,
          0x23 : NexStarScope.maxrate_enabled,
          0x24 : NexStarScope.move_pos,
          0x25 : NexStarScope.move_neg,
          0x38 : NexStarScope.send_ack,
          0x39 : NexStarScope.send_ack,
          0x3a : NexStarScope.set_cordwrap_pos,
          0x40 : NexStarScope.get_pos_backlash,
          0x41 : NexStarScope.get_neg_backlash,
          0x47 : NexStarScope.get_autoguide_rate,
          0xfc : NexStarScope.get_approach,
          0xfd : NexStarScope.set_approach,
          0xfe : NexStarScope.fw_version,
        }
        if tui : self.init_dsp(stdscr)

    def send_ack(self, data, snd, rcv):
        return b''

    def set_maxrate(self, data, snd, rcv):
        if rcv==0x10 :
            self.alt_maxrate=unpack_int2(data)
        else :
            self.azm_maxrate=unpack_int2(data)
        return b''

    def get_maxrate(self, data, snd, rcv):
        if len(data) == 0 :
            #return pack_int2(self.alt_maxrate) + pack_int2(self.azm_maxrate)
            return bytes.fromhex('0fa01194')
        else :
            return b''

    def enable_maxrate(self, data, snd, rcv):
        self.use_maxrate=bool(data[0])
        return b''

    def maxrate_enabled(self, data, snd, rcv):
        if self.use_maxrate :
            return b'\x01'
        else :
            return b'\x00'

    def cmd_0x10(self, data, snd, rcv):
        if rcv == 0xbf : # LIGHT
            if len(data)==2 : # Set level
                if data[0]==0 :
                    self.lt_tray=data[1]
                elif data[0]==1 :
                    self.lt_logo=data[1]
                else :
                    self.lt_wifi=data[1]
                return b''
            elif len(data)==1: # Get level
                if data[0]==0 :
                    return bytes([int(self.lt_tray%256)])
                elif data[0]==1 :
                    return bytes([int(self.lt_logo%256)])
                else :
                    return bytes([int(self.lt_wifi%256)])
            else :
                return b''
        elif rcv == 0xb7 : # CHG
            if len(data):
                self.charge = bool(data[0])
                return b''
            else :
                return bytes([int(self.charge)])
        elif rcv == 0xb6 : # BAT
            self.bat_voltage*=0.99
            v=struct.pack('!i',int(self.bat_voltage))
            return bytes.fromhex('0102') + v
        else :
            return b''

    def cmd_0x18(self, data, snd, rcv):
        if rcv == 0xb6 : # BAT
            if len(data):
                i=data[0]*256+data[1]
                i=min(5000,i)
                i=max(2000,i)
                self.bat_current=i
            return struct.pack('!i',int(self.bat_current))[-2:]


    def get_position(self, data, snd, rcv):
        try :
            trg = trg_names[rcv]
        except KeyError :
            trg = '???'
        if trg == 'ALT':
            return pack_int3(self.alt)
        else :
            return pack_int3(self.azm)

    def goto_fast(self, data, snd, rcv):
        self.last_cmd='GOTO_FAST'
        self.slewing=True
        self.goto=True
        self.guiding=False
        self.alt_guiderate=0
        self.azm_guiderate=0
        if rcv==MC_ALT :
            r=self.alt_maxrate/(360e3)
        else :
            r=self.azm_maxrate/(360e3)
        a=unpack_int3(data)
        if trg_names[rcv] == 'ALT':
            self.trg_alt=a
            if a-self.alt < 0 :
                r = -r
            self.alt_rate = r
        else :
            self.trg_azm=a%1.0
            if self.trg_azm - self.azm < 0 :
                r = -r
            if abs(self.trg_azm - self.azm) > 0.5 :
                r = -r
            self.azm_rate = r
        return b''

    def set_position(self,data, snd, rcv):
        return b''

    def cmd_0x05(self, data, snd, rcv):
        return bytes.fromhex('1685')

    def set_pos_guiderate(self, data, snd, rcv):
        # The 1.1 factor is experimental to fit the actual hardware
        a=1.1*(2**24/1000/360/60/60)*unpack_int3(data) # (transform to rot/sec)
        self.guiding = a>0
        if trg_names[rcv] == 'ALT':
            self.alt_guiderate=a
        else :
            self.azm_guiderate=a
        return b''

    def set_neg_guiderate(self, data, snd, rcv):
        # The 1.1 factor is experimental to fit the actual hardware
        a=1.1*(2**24/1000/360/60/60)*unpack_int3(data) # (transform to rot/sec)
        self.guiding = a>0
        if trg_names[rcv] == 'ALT':
            self.alt_guiderate=-a
        else :
            self.azm_guiderate=-a
        return b''

    def level_start(self, data, snd, rcv):
        return b''

    def set_pos_backlash(self, data, snd, rcv):
        return b''

    def set_neg_backlash(self, data, snd, rcv):
        return b''

    def goto_slow(self, data, snd, rcv):
        self.last_cmd='GOTO_SLOW'
        self.slewing=True
        self.goto=True
        self.guiding=False
        self.alt_guiderate=0
        self.azm_guiderate=0
        r=0.2/360
        a=unpack_int3(data)
        if trg_names[rcv] == 'ALT':
            self.trg_alt=a
            if self.alt < a :
                self.alt_rate = r
            else :
                self.alt_rate = -r
        else :
            self.trg_azm=a%1.0
            f = 1 if abs(self.azm - self.trg_azm)<0.5 else -1 
            if self.azm < self.trg_azm :
                self.azm_rate = f*r
            else :
                self.azm_rate = -f*r
        return b''

    def slew_done(self, data, snd, rcv):
        if rcv == MC_ALT :
            return b'\x00' if self.alt_rate else b'\xff'
        if rcv == MC_AZM :
            return b'\x00' if self.azm_rate else b'\xff'

    def at_index(self, data, snd, rcv):
        return b'\x00'

    def seek_index(self, data, snd, rcv):
        return b''

    def move_pos(self, data, snd, rcv):
        self.last_cmd='MOVE_POS'
        self.slewing=True
        self.goto=False
        r=RATES[int(data[0])]
        if trg_names[rcv] == 'ALT':
            self.alt_rate = r
        else :
            self.azm_rate = r
        return b''

    def move_neg(self, data, snd, rcv):
        self.last_cmd='MOVE_NEG'
        self.slewing=True
        self.goto=False
        r=RATES[int(data[0])]
        if trg_names[rcv] == 'ALT':
            self.alt_rate = -r
        else :
            self.azm_rate = -r
        return b''

    def set_cordwrap_pos(self, data, snd, rcv):
        self.cordwrap_pos=struct.unpack('!i',b'\x00'+data[:3])[0]
        return b''

    def get_pos_backlash(self, data, snd, rcv):
        return b'\x00'

    def get_neg_backlash(self, data, snd, rcv):
        return b'\x00'

    def get_autoguide_rate(self, data, snd, rcv):
        return b'\xf0'

    def get_approach(self, data, snd, rcv):
        try :
            trg = trg_names[rcv]
        except KeyError :
            trg = '???'
        if trg == 'ALT':
            return bytes((self.alt_approach,))
        else :
            return bytes((self.azm_approach,))
            
    def set_approach(self, data, snd, rcv):
        try :
            trg = trg_names[rcv]
        except KeyError :
            trg = '???'
        if trg == 'ALT':
            self.alt_approach=data[0]
        else :
            self.azm_approach=data[0]
        return b''

    def fw_version(self, data, snd, rcv):
        try :
            trg = trg_names[rcv]
        except KeyError :
            trg = '???'
        if trg in ('ALT','AZM'):
            return bytes(NexStarScope.__mcfw_ver)
        elif trg in ('HC', 'HC+'):
            return bytes(NexStarScope.__hcfw_ver)
        else :
            return b''

    def init_dsp(self,stdscr):
        self.scr=stdscr
        if stdscr :
            self.cmd_log_w=curses.newwin(self.cmd_log.maxlen+2,60,0,50)
            self.state_w=curses.newwin(1,80,0,0)
            self.state_w.border()
            self.pos_w=curses.newwin(4,25,1,0)
            self.pos_w.border()
            self.trg_w=curses.newwin(4,25,1,25)
            self.trg_w.border()
            self.rate_w=curses.newwin(4,25,5,0)
            self.guide_w=curses.newwin(4,25,5,25)
            self.other_w=curses.newwin(8,50,9,0)
            self.msg_w=curses.newwin(self.msg_log.maxlen+2,50,17,0)
            stdscr.refresh()

    def update_dsp(self):
        if self.scr :
            mode = 'Idle'
            if self.guiding : mode = 'Guiding'
            if self.slewing : mode = 'Slewing'
            self.state_w.clear()
            self.state_w.addstr(0,1,'State: %8s' % mode)
            self.state_w.refresh()
            self.pos_w.clear()
            self.pos_w.border()
            self.pos_w.addstr(0,1,'Position:')
            self.pos_w.addstr(1,3,'Alt: ' + repr_angle(self.alt))
            self.pos_w.addstr(2,3,'Azm: ' + repr_angle(self.azm))
            self.pos_w.refresh()
            self.trg_w.clear()
            self.trg_w.border()
            self.trg_w.addstr(0,1,'Target:')
            self.trg_w.addstr(1,3,'Alt: ' + repr_angle(self.trg_alt))
            self.trg_w.addstr(2,3,'Azm: ' + repr_angle(self.trg_azm))
            self.trg_w.refresh()
            self.rate_w.clear()
            self.rate_w.border()
            self.rate_w.addstr(0,1,'Move rate:')
            self.rate_w.addstr(1,3,'Alt: %+8.4f °/s' % (self.alt_rate*360))
            self.rate_w.addstr(2,3,'Azm: %+8.4f °/s' % (self.azm_rate*360))
            self.rate_w.refresh()
            self.guide_w.clear()
            self.guide_w.border()
            self.guide_w.addstr(0,1,'Guide rate:')
            self.guide_w.addstr(1,3,'Alt: %+8.4f "/s' % (self.alt_guiderate*360*60*60))
            self.guide_w.addstr(2,3,'Azm: %+8.4f "/s' % (self.azm_guiderate*360*60*60))
            self.guide_w.refresh()
            self.other_w.clear()
            self.other_w.border()
            self.other_w.addstr(0,1,'Other:')
            self.other_w.addstr(1,3,'BAT: %9.6f V' % (self.bat_voltage/1e6))
            self.other_w.addstr(2,3,'CHG: %5.3f A' % (self.bat_current/1e3))
            self.other_w.addstr(3,3,'LIGHTS: Logo: %d  Tray: %d  WiFi: %d' % (self.lt_logo, self.lt_tray, self.lt_wifi))
            self.other_w.addstr(4,3,'Charge: %s' % ('On' if self.charge else 'Auto'))
            if self.use_maxrate : 
                self.other_w.addstr(5,1,'*')
            self.other_w.addstr(5,3,'Max rate ALT:%.2f  AZM:%.2f' % (self.alt_maxrate/1e3, self.azm_maxrate/1e3))
            self.other_w.refresh()
            self.cmd_log_w.clear()
            self.cmd_log_w.border()
            self.cmd_log_w.addstr(0,1,'Commands log')
            for n,cmd in enumerate(self.cmd_log) :
                self.cmd_log_w.addstr(n+1,1,cmd)
            self.cmd_log_w.refresh()
            self.msg_w.border()
            self.msg_w.addstr(0,1,'Msg:')
            for n,cmd in enumerate(self.msg_log) :
                self.msg_w.addstr(n+1,1,cmd)
            self.msg_w.refresh()

    def show_status(self):
        if self.tui and self.scr:
            self.update_dsp()
        else :
            mode = 'Idle'
            if self.guiding : mode = 'Guiding'
            if self.slewing : mode = 'Slewing'
            print('\r%8s: %s -> %s S(%+5.2f %+5.2f) G(%+.4f %+.4f) CMD: %10s' % (
                    mode, repr_pos(self.alt, self.azm), 
                    repr_pos(self.trg_alt, self.trg_azm),
                    self.alt_rate*360, self.azm_rate*360, # deg/sec
                    self.alt_guiderate*360*60*60, self.azm_guiderate*360*60*60, # arcsec/sec
                    self.last_cmd), end='')
            sys.stdout.flush()

    def tick(self, interval):
        eps=1e-6 # 0.1" precission
        maxrate = 5/360
        if self.last_cmd=='GOTO_FAST' :
            eps*=100

        self.alt += (self.alt_rate + self.alt_guiderate)*interval
        self.azm += (self.azm_rate + self.azm_guiderate)*interval
        if self.slewing and self.goto:
            # AZM
            r=self.trg_azm - self.azm
            if abs(r)>0.5 :
                r = -r
            s=1 if r>0 else -1
            mr=min(maxrate,abs(self.azm_rate))
            if mr*interval > abs(r) :
                mr/=2
            self.azm_rate=s*min(mr,abs(r))
            self.azm_rate=s*mr
                
            # ALT
            r=self.trg_alt - self.alt
            s=1 if r>0 else -1
            mr=min(maxrate,abs(self.alt_rate))
            if mr*interval > abs(r) :
                mr/=2
            self.alt_rate=s*min(mr,abs(r))
            self.alt_rate=s*mr
            
        if abs(self.azm_rate) < eps and abs(self.alt_rate) < eps :
            self.slewing=False
            self.goto=False
        if abs(self.azm_rate) < eps :
            self.azm_rate=0
        if abs(self.alt_rate) < eps :
            self.alt_rate=0
        self.show_status()

    @property
    def alt(self):
        return self.__alt
        
    @alt.setter
    def alt(self,ALT):
        self.__alt=ALT
        # Altitude movement limits
        #self.__alt = min(self.__alt,0.23)
        #self.__alt = max(self.__alt,-0.03)
        
    @property
    def azm(self):
        return self.__azm
    
    @azm.setter
    def azm(self,AZM):
        self.__azm=AZM % 1.0
            
    @property
    def trg_alt(self):
        return self.__trg_alt
        
    @trg_alt.setter
    def trg_alt(self,ALT):
        self.__trg_alt=ALT
        # Altitude movement limits
        #self.__trg_alt = min(self.__trg_alt,0.24)
        #self.__trg_alt = max(self.__trg_alt,-0.01)
        
    @property
    def trg_azm(self):
        return self.__trg_azm
    
    @trg_azm.setter
    def trg_azm(self,AZM):
        self.__trg_azm=AZM % 1.0

    def handle_cmd(self, cmd):
        #print("-> Scope received %s." % (print_command(cmd)))
        try :
            c,f,t,l,d,s=decode_command(cmd)
        except IndexError :
            self.print_msg("Malformed command: %r" % (cmd,))
            return b''
        resp=b''
        if make_checksum(cmd[:-1]) != s :
            self.print_msg("Wrong checksum. Ignoring.")
        else :
            resp = b';' + cmd
            if t in (0x10, 0x11):
                handlers=self._mc_handlers
                try :
                    s=('%s[%s] ' % (trg_names[t], cmd_names[c])) + ''.join('%02x' % b for b in d)
                except KeyError :
                    s=('MC[%02x]' % c) + ' ' + ''.join('%02x' % b for b in d)
            else :
                handlers=self._other_handlers
                try :
                    s=('%s[%s]' % (trg_names[t],trg_cmds[trg_names[t]][c])) + ' ' + ''.join('%02x' % b for b in d)
                except KeyError :
                    s=('%02x[%02x]' % (t,c)) + ' ' + ''.join('%02x' % b for b in d)
            sr=b''
            if c in handlers :
                r = handlers[c](self,d,f,t)
                sr = r
                r = bytes((len(r)+3,t,f,c)) + r
                resp += b';' + r + bytes((make_checksum(r),))
                #print('Response: %r' % resp)
            else :
                #print('Scope got unknown command %02x' % c)
                #print("-> Scope received %s." % (print_command(cmd)))
                s = '** ' + s
                pass

            if 'MC_GET_POSITION' in s :
                return resp

            s += '-> ' + ''.join('%02x' % x for x in sr)

            if self.cmd_log :
                if s != self.cmd_log[-1] :
                    self.cmd_log.append(s)
            else :
                self.cmd_log.append(s)

        return resp

    def print_msg(self, msg):
        if self.msg_log :
            if msg != self.msg_log[-1] :
                self.msg_log.append(msg)
        else :
            self.msg_log.append(msg)



    def handle_msg(self, msg):
        '''
        React to message. Get AUX command(s) in the message and react to it. 
        Return a message simulating real AUX scope response.
        '''
        return b''.join([self.handle_cmd(cmd) for cmd in split_cmds(msg)])
            



