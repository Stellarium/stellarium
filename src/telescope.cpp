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
#include <math.h>

#ifdef WIN32
  #include <windows.h> // GetSystemTimeAsFileTime
  #include <winsock2.h>
  #define ERRNO WSAGetLastError()
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
  #define ERRNO errno
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
  Vec3d desired_pos;
};


class TelescopeTcp : public Telescope {
public:
  TelescopeTcp(const string &name,const string &params);
  ~TelescopeTcp(void) {hangup();}
private:
  bool isConnected(void) const
    {return (!IS_INVALID_SOCKET(fd) && !wait_for_connection_establishment);}
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
  long long int next_connection_attempt;
  char read_buff[96];
  char *read_buff_end;
  char write_buff[96];
  char *write_buff_end;
};

Telescope *Telescope::create(const string &url) {
  string::size_type i = url.find(':');
  if (i == string::npos) {
    cerr << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", ':' missing" << endl;
    return 0;
  }
  const string name = url.substr(0,i);
  if (i+2 > url.length()) {
    cerr << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", too short" << endl;
    return 0;
  }
  string::size_type j = url.find(':',i+1);
  if (j == string::npos) {
    cerr << "Telescope::create(" << url << "): bad url: \"" << url
         << "\", 2nd ':' missing" << endl;
    return 0;
  }
  const string type = url.substr(i+1,j-i-1);
  if (j+2 > url.length()) {
    cerr << "Telescope::create(" << url << "): bad url: \"" << url
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
    cerr << "Telescope::create(" << url << "): unknown telescop type \""
         << type << '"' << endl;
  }
  if (rval && !rval->isInitialized()) {
    delete rval;
    rval = 0;
  }
  return rval;
}


Telescope::Telescope(const string &name) {
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



TelescopeTcp::TelescopeTcp(const string &name,const string &params)
             :Telescope(name),fd(INVALID_SOCKET) {
  hangup();
  address.sin_port = htons(0);
  string::size_type i = params.find(':');
  if (i == string::npos) {
    cerr << "TelescopeTcp::TelescopeTcp(" << name << ',' << params << "): "
            "bad params: ':' missing" << endl;
    return;
  }
  const string host = params.substr(0,i);
  if (i+2 > params.length()) {
    cerr << "Telescope::create(" << params << "): bad params: too short"
         << endl;
    return;
  }
  int port;
  if (1!=sscanf(params.c_str()+(i+1),"%d",&port) || port<=0 || port>0xFFFF) {
    cerr << "Telescope::create(" << params << "): bad port" << endl;
    return;
  }
  struct hostent *hep = gethostbyname(host.c_str());
  if (hep == 0) {
    cerr << "Telescope::create(" << params << "): unknown host" << endl;
    return;
  }
  if (hep->h_length != 4) {
    cerr << "Telescope::create(" << params << "): only IPv4 implemented"
         << endl;
    return;
  }
  memset(&address,0,sizeof(struct sockaddr_in));
  memcpy(&(address.sin_addr),hep->h_addr,4);
  address.sin_port = htons(port);
  address.sin_family = AF_INET;
  next_connection_attempt = -0x8000000000000000LL;
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
    if (write_buff_end-write_buff+12 < (int)sizeof(write_buff)) {
      const double ra = atan2(j2000_pos[1],j2000_pos[0]);
      const double dec = atan2(j2000_pos[2],
              sqrt(j2000_pos[0]*j2000_pos[0]+j2000_pos[1]*j2000_pos[1]));
      unsigned int ra_int = (unsigned int)floor(0.5 + ra*(0x8000000/M_PI));
      int dec_int = (int)floor(0.5 + dec*(0x8000000/M_PI));
//      cout << "TelescopeTcp::telescopeGoto: "
//              "queuing packet: "
//           << (12*ra/M_PI) << ',' << (180*dec/M_PI)
//           << ";  " << ra_int << ',' << dec_int << endl;
        // length of packet:
      *write_buff_end++ = 12;
      *write_buff_end++ = 0;
        // type of packet:
      *write_buff_end++ = 0;
      *write_buff_end++ = 0;
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
      cerr << "TelescopeTcp::telescopeGoto: "
              "communication is too slow, I will ignore this command" << endl;
    }
  }
}

void TelescopeTcp::performWriting(void) {
  const int to_write = write_buff_end - write_buff;
  const int rc = send(fd,write_buff,to_write,0);
  if (rc < 0) {
    if (ERRNO != EINTR && ERRNO != EAGAIN) {
      cerr << "TelescopeTcp::performWriting: send failed" << endl;
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
      cerr << "TelescopeTcp::performReading: recv failed" << endl;
      hangup();
    }
  } else if (rc == 0) {
    cerr << "TelescopeTcp::performReading: "
            "server has closed the connection" << endl;
    hangup();
  } else {
    read_buff_end += rc;
    char *p = read_buff;
    while (read_buff_end-p >= 2) {
      const int size = (int)(                ((unsigned char)(p[0])) |
                              (((unsigned int)(unsigned char)(p[1])) << 8) );
      if (size > (int)sizeof(read_buff) || size < 4) {
        cerr << "TelescopeTcp::performReading: "
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
          if (size < 12) {
            cerr << "TelescopeTcp::performReading: "
                    "type 0: bad packet size: " << size << endl;
            hangup();
            return;
          }
          const unsigned int ra_int =
                    ((unsigned int)(unsigned char)(p[ 4])) |
                   (((unsigned int)(unsigned char)(p[ 5])) <<  8) |
                   (((unsigned int)(unsigned char)(p[ 6])) << 16) |
                   (((unsigned int)(unsigned char)(p[ 7])) << 24);
          const int dec_int =
            (int)(  ((unsigned int)(unsigned char)(p[ 8])) |
                   (((unsigned int)(unsigned char)(p[ 9])) <<  8) |
                   (((unsigned int)(unsigned char)(p[10])) << 16) |
                   (((unsigned int)(unsigned char)(p[11])) << 24) );
          const double ra = ra_int*(M_PI/0x8000000);
          const double dec = dec_int*(M_PI/0x8000000);
          const double cdec = cos(dec);
          XYZ[0] = cos(ra)*cdec;
          XYZ[1] = sin(ra)*cdec;
          XYZ[2] = sin(dec);
        } break;
        default:
          cout << "TelescopeTcp::performReading: "
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

static long long int GetNow(void) {
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

void TelescopeTcp::prepareSelectFds(fd_set &read_fds,fd_set &write_fds,
                                    int &fd_max) {
  if (IS_INVALID_SOCKET(fd)) {
      // try reconnecting
    const long long int now = GetNow();
    if (now < next_connection_attempt) return;
    next_connection_attempt = now + 5000000;
    fd = socket(AF_INET,SOCK_STREAM,0);
    if (IS_INVALID_SOCKET(fd)) {
      cerr << "TelescopeTcp::prepareSelectFds: socket() failed" << endl;
      return;
    }
    if (SET_NONBLOCKING_MODE(fd) != 0) {
      cerr << "TelescopeTcp::prepareSelectFds: "
              "could not set nonblocking mode" << endl;
      hangup();
      return;
    }
    if (connect(fd,(struct sockaddr*)(&address),sizeof(address)) != 0) {
      if (ERRNO != EINPROGRESS && ERRNO != EAGAIN) {
        cerr << "TelescopeTcp::prepareSelectFds: "
                "connect() failed" << endl;
        hangup();
        return;
      }
      wait_for_connection_establishment = true;
    } else {
      wait_for_connection_establishment = false;
        // connection established, wait for next call of prepareSelectFds
    }
  } else {
      // socked is already connected
    if (fd_max < (int)fd) fd_max = (int)fd;
    if (wait_for_connection_establishment) {
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
          cerr << "TelescopeTcp::handleSelectFds: getsockopt failed" << endl;
          hangup();
        } else {
          if (err != 0) {
//            cerr << "TelescopeTcp::handleSelectFds: connect failed: " << err
//                 << endl;
            hangup();
          } else {
            cout << "TelescopeTcp::handleSelectFds: connection established"
                 << endl;
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



