/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "telescope.h"
#include "stel_utility.h"

#include <sstream>

#include <math.h>

#ifdef WIN32
  #include <windows.h> // GetSystemTimeAsFileTime
  #include <winsock2.h>
  #define ERRNO WSAGetLastError()
  #define STRERROR(x) x
  #undef EAGAIN
  #define EAGAIN WSAEWOULDBLOCK
  #undef EINTR
  #define EINTR WSAEINTR
  #undef EINPROGRESS
  #define EINPROGRESS WSAEINPROGRESS
  static u_long ioctlsocket_arg = 1;
  #define SET_NONBLOCKING_MODE(s) ioctlsocket(s,FIONBIO,&ioctlsocket_arg)
  #define SOCKLEN_T int
  #define close closesocket
  #define IS_INVALID_SOCKET(fd) (fd==INVALID_SOCKET)
#else
  #include <netdb.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <string.h> // strerror
  #define ERRNO errno
  #define STRERROR(x) strerror(x)
  #define SET_NONBLOCKING_MODE(s) fcntl(s,F_SETFL,O_NONBLOCK)
  #define SOCKLEN_T socklen_t
  #define SOCKET int
  #define IS_INVALID_SOCKET(fd) (fd<0)
  #define INVALID_SOCKET (-1)
#endif

class TelescopeDummy : public Telescope {
public:
  TelescopeDummy(const string &name,const string &params) : Telescope(name) {
    desired_pos[0] = XYZ[0] = 1.0;
    desired_pos[1] = XYZ[1] = 0.0;
    desired_pos[2] = XYZ[2] = 0.0;
  }
private:
  bool isConnected(void) const {return true;}
  bool hasKnownPosition(void) const {return true;}
  Vec3d getObsJ2000Pos(const Navigator *nav=0) const {return XYZ;}
  void prepareSelectFds(fd_set&,fd_set&,int&) {
    XYZ = XYZ*31.0+desired_pos;
    const double lq = XYZ.lengthSquared();
    if (lq > 0.0) XYZ *= (1.0/sqrt(lq));
    else XYZ = desired_pos;
  }
  void telescopeGoto(const Vec3d &j2000_pos) {
    desired_pos = j2000_pos;
    desired_pos.normalize();
  }
  Vec3d XYZ; // j2000 position
  Vec3d desired_pos;
};


class TelescopeTcp : public Telescope {
public:
  TelescopeTcp(const string &name,const string &params);
  ~TelescopeTcp(void) {hangup();}
private:
  bool isConnected(void) const
    {return (!IS_INVALID_SOCKET(fd) && !wait_for_connection_establishment);}
  Vec3d getObsJ2000Pos(const Navigator *nav=0) const;
  void prepareSelectFds(fd_set &read_fds,fd_set &write_fds,int &fd_max);
  void handleSelectFds(const fd_set &read_fds,const fd_set &write_fds);
  void telescopeGoto(const Vec3d &j2000_pos);
  bool isInitialized(void) const {return (ntohs(address.sin_port)!=0);}
  void performReading(void);
  void performWriting(void);
private:
  void hangup(void);
  struct sockaddr_in address;
  SOCKET fd;
  bool wait_for_connection_establishment;
  long long int end_of_timeout;
  char read_buff[120];
  char *read_buff_end;
  char write_buff[120];
  char *write_buff_end;
  long long int time_delay;
  struct Position {
    long long int server_micros;
    long long int client_micros;
    unsigned int ra_int;
    int dec_int;
    int status;
  };
  Position positions[16];
  Position *position_pointer;
  Position *const end_position;
  virtual bool hasKnownPosition(void) const
    {return (position_pointer->client_micros!=0x7FFFFFFFFFFFFFFFLL);}
};

Telescope *Telescope::create(const string &url) {
  string::size_type i = url.find(':');
  if (i == string::npos) {
    cout << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", ':' missing" << endl;
    return 0;
  }
  const string name = url.substr(0,i);
  if (i+2 > url.length()) {
    cout << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", too short" << endl;
    return 0;
  }
  string::size_type j = url.find(':',i+1);
  if (j == string::npos) {
    cout << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", 2nd ':' missing" << endl;
    return 0;
  }
  const string type = url.substr(i+1,j-i-1);
  if (j+2 > url.length()) {
    cout << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", too short" << endl;
    return 0;
  }
  const string params = url.substr(j+1);
  cout << "Telescope::create(" << url << "): trying to create telescope \""
       << name << "\" of type \"" << type << "\" with parameters \""
       << params << '"' << endl;
  Telescope *rval = 0;
  if (type == "Dummy") {
    rval = new TelescopeDummy(name,params);
  } else if (type == "TCP") {
    rval = new TelescopeTcp(name,params);
  } else {
    cout << "Telescope::create(" << url << "): unknown telescop type \""
         << type << '"' << endl;
  }
  if (rval && !rval->isInitialized()) {
    delete rval;
    rval = 0;
  }
  return rval;
}


Telescope::Telescope(const string &name) : name(name) {
  std::wostringstream oss;
  oss << name.c_str();
  nameI18n = oss.str();
}

wstring Telescope::getInfoString(const Navigator *nav) const {
  const Vec3d equatorial_pos = get_earth_equ_pos(nav);
  double ra,dec;
  rect_to_sphe(&ra,&dec,equatorial_pos);
  std::wostringstream oss;
  oss << nameI18n << endl
      << "RA/DE: " << StelUtility::printAngleHMS(ra)
      << "/" << StelUtility::printAngleDMS(dec) << endl;
  return oss.str();
}

wstring Telescope::getShortInfoString(const Navigator*) const {
  return nameI18n;
}



long long int GetNow(void) {
#ifdef WIN32
  FILETIME file_time;
  GetSystemTimeAsFileTime(&file_time);
  return (*((__int64*)(&file_time))/10) - 86400000000LL*134774;
#else
  struct timeval tv;
  gettimeofday(&tv,0);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
#endif
}

TelescopeTcp::TelescopeTcp(const string &name,const string &params)
             :Telescope(name),fd(INVALID_SOCKET),
              end_position(positions+(sizeof(positions)/sizeof(positions[0]))) {
  hangup();
  address.sin_port = htons(0);
  string::size_type i = params.find(':');
  if (i == string::npos) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad params: ':' missing" << endl;
    return;
  }
  const string host = params.substr(0,i);
  if (i+2 > params.length()) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad params: too short"
         << endl;
    return;
  }
  string::size_type j = params.find(':',i+1);
  if (j == string::npos) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad params: ':' missing" << endl;
    return;
  }
  int port;
  if (1!=sscanf(params.substr(i+1,j-i-1).c_str(),"%d",&port) ||
      port<=0 || port>0xFFFF) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad port" << endl;
    return;
  }
  if (1!=sscanf(params.substr(j+1).c_str(),"%Ld",&time_delay) ||
      time_delay<=0 || time_delay>10000000) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad time_delay" << endl;
    return;
  }
  struct hostent *hep = gethostbyname(host.c_str());
  if (hep == 0) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "unknown host" << endl;
    return;
  }
  if (hep->h_length != 4) {
    cout << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "only IPv4 implemented"
         << endl;
    return;
  }
  memset(&address,0,sizeof(struct sockaddr_in));
  memcpy(&(address.sin_addr),hep->h_addr,4);
  address.sin_port = htons(port);
  address.sin_family = AF_INET;
  end_of_timeout = -0x8000000000000000LL;

  for (position_pointer = positions;
       position_pointer < end_position;
       position_pointer++) {
    position_pointer->server_micros = 0x7FFFFFFFFFFFFFFFLL;
    position_pointer->client_micros = 0x7FFFFFFFFFFFFFFFLL;
    position_pointer->ra_int = 0;
    position_pointer->dec_int = 0;
    position_pointer->status = 0;
  }
  position_pointer = positions;
}

void TelescopeTcp::hangup(void) {
  if (!IS_INVALID_SOCKET(fd)) {
    close(fd);
    fd = INVALID_SOCKET;
  }
  read_buff_end = read_buff;
  write_buff_end = write_buff;
  wait_for_connection_establishment = false;
}

void TelescopeTcp::telescopeGoto(const Vec3d &j2000_pos) {
  if (isConnected()) {
    if (write_buff_end-write_buff+20 < (int)sizeof(write_buff)) {
      const double ra = atan2(j2000_pos[1],j2000_pos[0]);
      const double dec = atan2(j2000_pos[2],
              sqrt(j2000_pos[0]*j2000_pos[0]+j2000_pos[1]*j2000_pos[1]));
      unsigned int ra_int = (unsigned int)floor(
                               0.5 +  ra*(((unsigned int)0x80000000)/M_PI));
      int dec_int = (int)floor(0.5 + dec*(((unsigned int)0x80000000)/M_PI));
//      cout << "TelescopeTcp(" << name << ")::telescopeGoto: "
//              "queuing packet: "
//           << (12*ra/M_PI) << ',' << (180*dec/M_PI)
//           << ";  " << ra_int << ',' << dec_int << endl;
        // length of packet:
      *write_buff_end++ = 20;
      *write_buff_end++ = 0;
        // type of packet:
      *write_buff_end++ = 0;
      *write_buff_end++ = 0;
        // client_micros:
      long long int now = GetNow();
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;now>>=8;
      *write_buff_end++ = now;
        // ra:
      *write_buff_end++ = ra_int;ra_int>>=8;
      *write_buff_end++ = ra_int;ra_int>>=8;
      *write_buff_end++ = ra_int;ra_int>>=8;
      *write_buff_end++ = ra_int;
        // dec:
      *write_buff_end++ = dec_int;dec_int>>=8;
      *write_buff_end++ = dec_int;dec_int>>=8;
      *write_buff_end++ = dec_int;dec_int>>=8;
      *write_buff_end++ = dec_int;
    } else {
      cout << "TelescopeTcp(" << name << ")::telescopeGoto: "
              "communication is too slow, I will ignore this command" << endl;
    }
  }
}

void TelescopeTcp::performWriting(void) {
  const int to_write = write_buff_end - write_buff;
  const int rc = send(fd,write_buff,to_write,0);
  if (rc < 0) {
    if (ERRNO != EINTR && ERRNO != EAGAIN) {
      cout << "TelescopeTcp(" << name << ")::performWriting: "
              "send failed: " << STRERROR(ERRNO) << endl;
      hangup();
    }
  } else if (rc > 0) {
    if (rc >= to_write) {
        // everything written
      write_buff_end = write_buff;
    } else {
        // partly written
      memmove(write_buff,write_buff+rc,to_write-rc);
      write_buff_end -= rc;
    }
  }
}

void TelescopeTcp::performReading(void) {
  const int to_read = read_buff + sizeof(read_buff) - read_buff_end;
  const int rc = recv(fd,read_buff_end,to_read,0);
  if (rc < 0) {
    if (ERRNO != EINTR && ERRNO != EAGAIN) {
      cout << "TelescopeTcp(" << name << ")::performReading: "
              "recv failed: " << STRERROR(ERRNO) << endl;
      hangup();
    }
  } else if (rc == 0) {
    cout << "TelescopeTcp(" << name << ")::performReading: "
            "server has closed the connection" << endl;
    hangup();
  } else {
    read_buff_end += rc;
    char *p = read_buff;
    while (read_buff_end-p >= 2) {
      const int size = (int)(                ((unsigned char)(p[0])) |
                              (((unsigned int)(unsigned char)(p[1])) << 8) );
      if (size > (int)sizeof(read_buff) || size < 4) {
        cout << "TelescopeTcp(" << name << ")::performReading: "
                "bad packet size: " << size << endl;
        hangup();
        return;
      }
      if (size > read_buff_end-p) {
          // wait for complete packet
        break;
      }
      const int type = (int)(                ((unsigned char)(p[2])) |
                              (((unsigned int)(unsigned char)(p[3])) << 8) );
        // dispatch:
      switch (type) {
        case 0: {
          if (size < 24) {
            cout << "TelescopeTcp(" << name << ")::performReading: "
                    "type 0: bad packet size: " << size << endl;
            hangup();
            return;
          }
          const long long int server_micros = (long long int)
                 (  ((unsigned long long int)(unsigned char)(p[ 4])) |
                   (((unsigned long long int)(unsigned char)(p[ 5])) <<  8) |
                   (((unsigned long long int)(unsigned char)(p[ 6])) << 16) |
                   (((unsigned long long int)(unsigned char)(p[ 7])) << 24) |
                   (((unsigned long long int)(unsigned char)(p[ 8])) << 32) |
                   (((unsigned long long int)(unsigned char)(p[ 9])) << 40) |
                   (((unsigned long long int)(unsigned char)(p[10])) << 48) |
                   (((unsigned long long int)(unsigned char)(p[11])) << 56) );
          const unsigned int ra_int =
                    ((unsigned int)(unsigned char)(p[12])) |
                   (((unsigned int)(unsigned char)(p[13])) <<  8) |
                   (((unsigned int)(unsigned char)(p[14])) << 16) |
                   (((unsigned int)(unsigned char)(p[15])) << 24);
          const int dec_int =
            (int)(  ((unsigned int)(unsigned char)(p[16])) |
                   (((unsigned int)(unsigned char)(p[17])) <<  8) |
                   (((unsigned int)(unsigned char)(p[18])) << 16) |
                   (((unsigned int)(unsigned char)(p[19])) << 24) );
          const int status =
            (int)(  ((unsigned int)(unsigned char)(p[20])) |
                   (((unsigned int)(unsigned char)(p[21])) <<  8) |
                   (((unsigned int)(unsigned char)(p[22])) << 16) |
                   (((unsigned int)(unsigned char)(p[23])) << 24) );

          position_pointer++;
          if (position_pointer >= end_position) position_pointer = positions;
          position_pointer->server_micros = server_micros;
          position_pointer->client_micros = GetNow();
          position_pointer->ra_int = ra_int;
          position_pointer->dec_int = dec_int;
          position_pointer->status = status;
          cout << "TelescopeTcp(" << name << ")::performReading: "
//                  "Client Time: " << position_pointer->client_micros
               << ", ra: " << position_pointer->ra_int
               << ", dec: " << position_pointer->dec_int
               << endl;
        } break;
        default:
          cout << "TelescopeTcp(" << name << ")::performReading: "
                  "ignoring unknown packet, type: " << type << endl;
          break;
      }
      p += size;
    }
    if (p >= read_buff_end) {
        // everything handled
      read_buff_end = read_buff;
    } else {
        // partly handled
      memmove(read_buff,p,read_buff_end-p);
      read_buff_end -= (p-read_buff);
    }
  }
}

Vec3d TelescopeTcp::getObsJ2000Pos(const Navigator*) const {
  if (position_pointer->client_micros == 0x7FFFFFFFFFFFFFFFLL) {
    return Vec3d(0,0,0);
  }
  const long long int now = GetNow() - time_delay;
  const Position *p = position_pointer;
  do {
    const Position *pp = p;
    if (pp == positions) pp = end_position;
    pp--;
    if (pp->client_micros == 0x7FFFFFFFFFFFFFFFLL) break;
    if (pp->client_micros <= now && now <= p->client_micros) {
      const double h = (M_PI/(unsigned int)0x80000000)
                     / (p->client_micros - pp->client_micros);
      const long long int f0 = (now - pp->client_micros);
      const long long int f1 = (p->client_micros - now);

      double ra;
      if (pp->ra_int <= p->ra_int) {
        if (p->ra_int - pp->ra_int <= (unsigned int)0x80000000) {
          ra = h*(f1*pp->ra_int  + f0*p->ra_int);
        } else {
          ra = h*(f1*(0*0x100000000LL + pp->ra_int)  + f0*p->ra_int);
        }
      } else {
        if (pp->ra_int - p->ra_int <= (unsigned int)0x80000000) {
          ra = h*(f1*pp->ra_int  + f0*p->ra_int);
        } else {
          ra = h*(f1*pp->ra_int  + f0*(0*0x100000000LL + p->ra_int));
        }
      }
      const double dec = h*(f1*pp->dec_int + f0*p->dec_int);
//      cout << "TelescopeTcp(" << name << ")::getObsJ2000Pos: "
//              "Time: " << now
//           << ", ra: " << ra*(((unsigned int)0x80000000)/M_PI)
//           << ", dec: " << dec*(((unsigned int)0x80000000)/M_PI)
//           << endl;
      const double cdec = cos(dec);
      return Vec3d(cos(ra)*cdec,sin(ra)*cdec,sin(dec));
    }
    p = pp;
  } while (p != position_pointer);
  const double h = (M_PI/(unsigned int)0x80000000);
  const double ra  = h*position_pointer->ra_int;
  const double dec = h*position_pointer->dec_int;
  const double cdec = cos(dec);
  return Vec3d(cos(ra)*cdec,sin(ra)*cdec,sin(dec));
}

void TelescopeTcp::prepareSelectFds(fd_set &read_fds,fd_set &write_fds,
                                    int &fd_max) {
  if (IS_INVALID_SOCKET(fd)) {
      // try reconnecting
    const long long int now = GetNow();
    if (now < end_of_timeout) return;
    end_of_timeout = now + 5000000;
    fd = socket(AF_INET,SOCK_STREAM,0);
    if (IS_INVALID_SOCKET(fd)) {
      cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
              "socket() failed: " << STRERROR(ERRNO) << endl;
      return;
    }
    if (SET_NONBLOCKING_MODE(fd) != 0) {
      cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
              "could not set nonblocking mode: " << STRERROR(ERRNO) << endl;
      hangup();
      return;
    }
    if (connect(fd,(struct sockaddr*)(&address),sizeof(address)) != 0) {
      if (ERRNO != EINPROGRESS && ERRNO != EAGAIN) {
        cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
                "connect() failed: " << STRERROR(ERRNO) << endl;
        hangup();
        return;
      }
      wait_for_connection_establishment = true;
//      cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
//              "waiting for connection establishment" << endl;
    } else {
      wait_for_connection_establishment = false;
      cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
              "connection established" << endl;
        // connection established, wait for next call of prepareSelectFds
    }
  } else {
      // socked is already connected
    if (fd_max < (int)fd) fd_max = (int)fd;
    if (wait_for_connection_establishment) {
      const long long int now = GetNow();
      if (now > end_of_timeout) {
        end_of_timeout = now + 1000000;
        cout << "TelescopeTcp(" << name << ")::prepareSelectFds: "
                "connect timeout" << endl;
        hangup();
        return;
      }
      FD_SET(fd,&write_fds);
    } else {
      if (write_buff_end > write_buff) FD_SET(fd,&write_fds);
      FD_SET(fd,&read_fds);
    }
  }
}

void TelescopeTcp::handleSelectFds(const fd_set &read_fds,
                                   const fd_set &write_fds) {
  if (!IS_INVALID_SOCKET(fd)) {
    if (wait_for_connection_establishment) {
      if (FD_ISSET(fd,&write_fds)) {
        wait_for_connection_establishment = false;
        int err = 0;
        SOCKLEN_T length = sizeof(err);
        if (getsockopt(fd,SOL_SOCKET,SO_ERROR,(char*)(&err),&length) != 0) {
          cout << "TelescopeTcp(" << name << ")::handleSelectFds: "
                  "getsockopt failed" << endl;
          hangup();
        } else {
          if (err != 0) {
            cout << "TelescopeTcp(" << name << ")::handleSelectFds: "
                    "connect failed: " << STRERROR(err) << endl;
            hangup();
          } else {
            cout << "TelescopeTcp(" << name << ")::handleSelectFds: "
                    "connection established" << endl;
          }
        }
      }
    } else { // connection already established
      if (FD_ISSET(fd,&write_fds)) {
        performWriting();
      }
      if (!IS_INVALID_SOCKET(fd) && FD_ISSET(fd,&read_fds)) {
        performReading();
      }
    }
  }
}



