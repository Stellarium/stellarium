/*
 * Copyright © 2008, Roland Roberts
 *
 */

#ifndef __DsiExceptions_hh
#define __DsiExceptions_hh

#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>

namespace DSI
{
class dsi_exception : public std::runtime_error
{
  public:
    dsi_exception(const std::string &arg) : std::runtime_error(arg){};
};

class bad_command : public dsi_exception
{
  public:
    bad_command(const std::string &arg = "response command does not match request command") : dsi_exception(arg) {}
    bad_command(const std::ostringstream &arg) : dsi_exception(arg.str()) {}
};
class bad_length : public dsi_exception
{
  public:
    bad_length(const std::string &arg = "response length does not match bytes read") : dsi_exception(arg) {}
    bad_length(const std::ostringstream &arg) : dsi_exception(arg.str()) {}
};

class bad_response : public dsi_exception
{
  public:
    bad_response(const std::string &arg = "command response was NACK") : dsi_exception(arg) {}
    bad_response(const std::ostringstream &arg) : dsi_exception(arg.str()) {}
};

class device_read_error : public dsi_exception
{
  public:
    device_read_error(const std::string &arg = "device read error") : dsi_exception(arg) {}
    device_read_error(const std::ostringstream &arg) : dsi_exception(arg.str()) {}
};

class device_write_error : public dsi_exception
{
  public:
    device_write_error(const std::string &arg = "device write error") : dsi_exception(arg) {}
    device_write_error(const std::ostringstream &arg) : dsi_exception(arg.str()) {}
};
};

#endif /* __DsiExceptions_hh */
