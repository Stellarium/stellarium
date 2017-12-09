#!/bin/env python3

import asyncio
import signal
import socket
import sys
from socket import SOL_SOCKET, SO_BROADCAST, SO_REUSEADDR
from nse_telescope import NexStarScope, repr_angle

import curses

telescope=None

async def broadcast(sport=2000, dport=55555, host='255.255.255.255', seconds_to_sleep=5):
    sck = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sck.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    sck.setsockopt(SOL_SOCKET, SO_BROADCAST, 1)
    # Fake msg. The app does not care for the payload
    msg = 110*b'X'
    sck.bind(('',sport))
    telescope.print_msg('Broadcasting to port {0}'.format(dport))
    telescope.print_msg('sleeping for: {0} seconds'.format(seconds_to_sleep))
    while True :
        bn = sck.sendto(msg,(host,dport))
        await asyncio.sleep(seconds_to_sleep)
    telescope.print_msg('Stopping broadcast')

async def timer(seconds_to_sleep=1,telescope=None):
    from time import time
    t=time()
    while True :
        await asyncio.sleep(seconds_to_sleep)
        if telescope : 
            telescope.tick(time()-t)
        t=time()

async def handle_port2000(reader, writer):
    '''
    This function handles initial communication with the WiFly module and
    delegates the real job of simulating the scope to the NexStarScope class.
    It also handles all the dirty details of actual communication.
    '''
    
    # The WiFly module is initially in the transparent mode and just passes
    # the data to the serial connection. Unless the '$$$' sequence is detected.
    # Then it switches to the command mode until the exit command is issued.
    # The $$$ should be guarded by the 1s silence.
    transparent=True
    retry = 5
    global telescope
    # Endless comm loop.
    connected=False
    while True :
        data = await reader.read(1024)
        if not data :
            writer.close()
            telescope.print_msg('Connection closed. Closing server.')
            return
        elif not connected :
            telescope.print_msg('App from {0} connected.'.format(writer.get_extra_info('peername')))
            connected=True
        retry = 5
        addr = writer.get_extra_info('peername')
        #print("-> Scope received %r from %r." % (data, addr))
        resp = b''
        if transparent :
            if data[:3]==b'$$$' :
                # Enter command mode
                transparent = False
                telescope.print_msg('App from {0} connected.'.format(addr))
                resp = b'CMD\r\n'
            else :
                # pass it on to the scope for handling
                resp = telescope.handle_msg(data)
        else :
            # We are in command mode detect exit and get out.
            # Otherwise just echo what we got and ack.
            message = b''
            try :
                message = data.decode('ascii').strip()
            except UnicodeError :
                # The data is invalid ascii - ignore it
                pass
            if message == 'exit' :
                # get out of the command mode
                transparent = True
                resp = data + b'\r\nEXIT\r\n'
            else :
                resp = data + b'\r\nAOK\r\n<2.40-CEL> '
        if resp :
            #print("<- Server sending: %r" % resp )
            writer.write(resp)
            await writer.drain()

#def signal_handler(signal, frame):  
#    loop.stop()
#    sys.exit(0)

#signal.signal(signal.SIGINT, signal_handler)

def to_be(n, size):
    b=bytearray(size)
    i=size-1
    while i >= 0:
        b[i] = n % 256
        n = n >> 8
        i -= 1
    return b
def from_be(b):
    n=0
    for i in range(len(b)):
        n = (n << 8) + b[i]
    return n
def to_le(n, size):
    b=bytearray(size)
    i=0
    while i < size:
        b[i] = n % 256
        n = n >> 8
        i += 1
    return b
def from_le(b):
    n=0
    for i in range(len(b)-1, -1, -1):
        n = (n << 8) + b[i]
    return n


def handle_stellarium_cmd(tel, d):
    import time
    p=0
    while p < len(d)-2:
        psize=from_le(d[p:p+2]) 
        if (psize > len(d) - p):
            break
        ptype=from_le(d[p+2:p+4])
        if ptype == 0:
            micros=from_le(d[p+4:p+12])
            if abs((micros/1000000.0) - int(time.time())) > 60.0:
                tel.print_msg('Client clock differs for more than one minute: '+str(int(micros/1000000.0))+'/'+str(int(time.time())))
            targetraint=from_le(d[p+12:p+16])
            targetdecint=from_le(d[p+16:p+20])
            if (targetdecint > (4294967296 / 2)):
                targetdecint = - (4294967296 - targetdecint)
            targetra=(targetraint * 24.0) / 4294967296.0
            targetdec=(targetdecint * 360.0) / 4294967296.0
            tel.print_msg('GoTo {} {}'.format(repr_angle(targetra/360),
                                                repr_angle(targetdec/360)))
            p+=psize
        else:
            # No such cmd
            tel.print_msg('Stellarium: unknown command ({})'.format(ptype))
            p+=psize
    return p

def make_stellarium_status(tel,obs):
    import math
    import time
    import ephem
    from math import pi
    
    alt=tel.alt
    azm=tel.azm
    obs.date=ephem.now()
    rajnow, decjnow=obs.radec_of(azm*2*pi, alt*2*pi)
    rajnow/=2*pi
    decjnow/=2*pi
    status=0
    msg=bytearray(24)
    msg[0:2]=to_le(24, 2)
    msg[2:4]=to_le(0, 2)
    tstamp=int(time.time())
    msg[4:12]=to_le(tstamp, 8)
    msg[12:16]=to_le(int(math.floor(rajnow * 4294967296.0)), 4)
    msg[16:20]=to_le(int(math.floor(decjnow * 4294967296.0)), 4)
    msg[20:24]=to_le(status, 4)
    return msg
    

connections = []

async def report_scope_pos(sleep=0.1, scope=None, obs=None):
    while True:
        await asyncio.sleep(sleep)
        for tr in connections:
            tr.write(make_stellarium_status(scope,obs))


class StellariumServer(asyncio.Protocol):

    def __init__(self, *arg, **kwarg):
        import ephem
        global telescope
        
        self.obs = ephem.Observer()
        self.obs.lon, self.obs.lat = '20:02', '50:05'
        self.telescope=telescope
        asyncio.Protocol.__init__(self,*arg,**kwarg)

    def connection_made(self, transport):
        peername = transport.get_extra_info('peername')
        if self.telescope is not None:
            self.telescope.print_msg('Stellarium from {}\n'.format(peername))
        self.transport = transport
        connections.append(transport)
        
    def connection_lost(self, exc):
        try:
            connections.remove(self.transport)
            self.telescope.print_msg('Stellarium connection closed\n')
        except ValueError:
            pass

    def data_received(self,data):
        if self.telescope is not None:
            handle_stellarium_cmd(telescope,data)
    
def main(stdscr):
    import ephem
    
    global telescope 

    obs = ephem.Observer()
    obs.lon, obs.lat = '20:02', '50:05'

    if len(sys.argv) >1 and sys.argv[1]=='t':
        telescope = NexStarScope(stdscr=None)
    else :
        telescope = NexStarScope(stdscr=stdscr)

    loop = asyncio.get_event_loop()
    
    scope = loop.run_until_complete(
                asyncio.start_server(handle_port2000, host='', port=2000))
    
    stell = loop.run_until_complete(
                loop.create_server(StellariumServer,host='',port=10001))
    
    telescope.print_msg('NSE simulator strted on {}.'.format(scope.sockets[0].getsockname()))
    telescope.print_msg('Hit CTRL-C to stop.')
    
    asyncio.ensure_future(broadcast())
    asyncio.ensure_future(timer(0.1,telescope))
    asyncio.ensure_future(report_scope_pos(0.1,telescope,obs))

    try :
        loop.run_forever()
    except KeyboardInterrupt :
        pass
    telescope.print_msg('Simulator shutting down')
    scope.close()
    loop.run_until_complete(scope.wait_closed())
    stell.close()
    loop.run_until_complete(stell.wait_closed())

    #loop.run_until_complete(asyncio.wait([broadcast(), timer(0.2), scope]))
    loop.close()

curses.wrapper(main)
