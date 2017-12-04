/*
 * Copyright © 2008, Roland Roberts
 *
 */

#pragma once

#include "NamedEnum.h"

namespace DSI
{
class DeviceCommand : public NamedEnum<int, DeviceCommand>
{
  protected:
    explicit DeviceCommand(const std::string name, const int &value) : NamedEnum<int, DeviceCommand>(name, value) {}

  public:
    static const DeviceCommand PING;
    static const DeviceCommand RESET;
    static const DeviceCommand ABORT;
    static const DeviceCommand TRIGGER;
    static const DeviceCommand CLEAR_TS;
    static const DeviceCommand GET_VERSION;
    static const DeviceCommand GET_STATUS;
    static const DeviceCommand GET_TIMESTAMP;
    static const DeviceCommand GET_EEPROM_LENGTH;
    static const DeviceCommand GET_EEPROM_BYTE;
    static const DeviceCommand SET_EEPROM_BYTE;
    static const DeviceCommand GET_GAIN;
    static const DeviceCommand SET_GAIN;
    static const DeviceCommand GET_OFFSET;
    static const DeviceCommand SET_OFFSET;
    static const DeviceCommand GET_EXP_TIME;
    static const DeviceCommand SET_EXP_TIME;
    static const DeviceCommand GET_EXP_MODE;
    static const DeviceCommand SET_EXP_MODE;
    static const DeviceCommand GET_VDD_MODE;
    static const DeviceCommand SET_VDD_MODE;
    static const DeviceCommand GET_FLUSH_MODE;
    static const DeviceCommand SET_FLUSH_MODE;
    static const DeviceCommand GET_CLEAN_MODE;
    static const DeviceCommand SET_CLEAN_MODE;
    static const DeviceCommand GET_READOUT_SPD;
    static const DeviceCommand SET_READOUT_SPD;
    static const DeviceCommand GET_READOUT_MODE;
    static const DeviceCommand SET_READOUT_MODE;
    static const DeviceCommand GET_NORM_READOUT_DELAY;
    static const DeviceCommand SET_NORM_READOUT_DELAY;
    static const DeviceCommand GET_ROW_COUNT_ODD;
    static const DeviceCommand SET_ROW_COUNT_ODD;
    static const DeviceCommand GET_ROW_COUNT_EVEN;
    static const DeviceCommand SET_ROW_COUNT_EVEN;
    static const DeviceCommand GET_TEMP;
    static const DeviceCommand GET_EXP_TIMER_COUNT;
    static const DeviceCommand PS_ON;
    static const DeviceCommand PS_OFF;
    static const DeviceCommand CCD_VDD_ON;
    static const DeviceCommand CCD_VDD_OFF;
    static const DeviceCommand AD_READ;
    static const DeviceCommand AD_WRITE;
    static const DeviceCommand TEST_PATTERN;
    static const DeviceCommand GET_DEBUG_VALUE;
    static const DeviceCommand GET_EEPROM_VIDPID;
    static const DeviceCommand SET_EEPROM_VIDPID;
    static const DeviceCommand ERASE_EEPROM;
};

class DevicePipeId : public NamedEnum<int, DevicePipeId>
{
  protected:
    explicit DevicePipeId(std::string name, const int &value) : NamedEnum<int, DevicePipeId>(name, value) {}

  public:
    static const DevicePipeId REQUEST;
    static const DevicePipeId RESPONSE;
    static const DevicePipeId IMAGE;
};

class DeviceResponse : public NamedEnum<int, DeviceResponse>
{
  protected:
    explicit DeviceResponse(const std::string name, const int &value) : NamedEnum<int, DeviceResponse>(name, value) {}

  public:
    static const DeviceResponse ACK;
    static const DeviceResponse NACK;
};

class UsbSpeed : public NamedEnum<int, UsbSpeed>
{
  protected:
    explicit UsbSpeed(std::string name, const int &value) : NamedEnum<int, UsbSpeed>(name, value) {}

  public:
    static const UsbSpeed FULL;
    static const UsbSpeed HIGH;
};

class AdRegister : public NamedEnum<int, AdRegister>
{
  protected:
    explicit AdRegister(const std::string name, const int &value) : NamedEnum<int, AdRegister>(name, value) {}

  public:
    static const AdRegister CONFIGURATION;
    static const AdRegister MUX_CONFIG;
    static const AdRegister RED_PGA;
    static const AdRegister GREEN_PGA;
    static const AdRegister BLUE_PGA;
    static const AdRegister RED_OFFSET;
    static const AdRegister GREEN_OFFSET;
    static const AdRegister BLUE_OFFSET;
};

class CleanMode : public NamedEnum<int, CleanMode>
{
  protected:
    explicit CleanMode(const std::string name, const int &value) : NamedEnum<int, CleanMode>(name, value) {}

  public:
    static const CleanMode ENABLED;
    static const CleanMode DISABLED;
};

class DesampleMethod : public NamedEnum<int, DesampleMethod>
{
  protected:
    explicit DesampleMethod(const std::string name, const int &value) : NamedEnum<int, DesampleMethod>(name, value) {}

  public:
    static const DesampleMethod HIGH_BYTE;
    static const DesampleMethod LOW_BYTE;
};

class CopyRasters : public NamedEnum<int, CopyRasters>
{
  protected:
    explicit CopyRasters(const std::string name, const int &value) : NamedEnum<int, CopyRasters>(name, value) {}

  public:
    static const CopyRasters ALL;
    static const CopyRasters EVEN;
    static const CopyRasters ODD;
};

class ExposureMode : public NamedEnum<int, ExposureMode>
{
  protected:
    explicit ExposureMode(const std::string name, const int &value) : NamedEnum<int, ExposureMode>(name, value) {}

  public:
    static const ExposureMode NORMAL;
    static const ExposureMode BIN2X2;
};

class FlushMode : public NamedEnum<int, FlushMode>
{
  protected:
    explicit FlushMode(std::string name, const int &value) : NamedEnum<int, FlushMode>(name, value) {}

  public:
    static const FlushMode CONTINUOUS;
    static const FlushMode BEFORE_EXPOSURE;
    static const FlushMode NEVER;
};

class ReadoutMode : public NamedEnum<int, ReadoutMode>
{
  protected:
    explicit ReadoutMode(std::string name, const int &value) : NamedEnum<int, ReadoutMode>(name, value) {}

  public:
    static const ReadoutMode DUAL;
    static const ReadoutMode SINGLE;
    static const ReadoutMode ODD;
    static const ReadoutMode EVEN;
};

class ReadoutSpeed : public NamedEnum<int, ReadoutSpeed>
{
  protected:
    explicit ReadoutSpeed(const std::string name, const int &value) : NamedEnum<int, ReadoutSpeed>(name, value) {}

  public:
    static const ReadoutSpeed NORMAL;
    static const ReadoutSpeed HIGH;
};

class VddMode : public NamedEnum<int, VddMode>
{
  protected:
    explicit VddMode(const std::string name, const int &value) : NamedEnum<int, VddMode>(name, value) {}

  public:
    static const VddMode AUTO;
    static const VddMode ON;
    static const VddMode OFF;
};
};
