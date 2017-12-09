/*
 * Copyright © 2008, Roland Roberts
 *
 */

#include "DsiTypes.h"

namespace DSI
{
/* */
template <>
NamedEnum<int, DSI::DeviceCommand>::instances_list NamedEnum<int, DSI::DeviceCommand>::s_instances =
    std::set<const NamedEnum<int, DSI::DeviceCommand> *, DSI::DeviceCommand::NamedEnum_Ptr_LessThan>();

const DeviceCommand DeviceCommand::PING("PING", 0x00);
const DeviceCommand DeviceCommand::RESET("RESET", 0x01);
const DeviceCommand DeviceCommand::ABORT("ABORT", 0x02);
const DeviceCommand DeviceCommand::TRIGGER("TRIGGER", 0x03);
const DeviceCommand DeviceCommand::CLEAR_TS("CLEAR_TS", 0x04);
const DeviceCommand DeviceCommand::GET_VERSION("GET_VERSION", 0x14);
const DeviceCommand DeviceCommand::GET_STATUS("GET_STATUS", 0x15);
const DeviceCommand DeviceCommand::GET_TIMESTAMP("GET_TIMESTAMP", 0x16);
const DeviceCommand DeviceCommand::GET_EEPROM_LENGTH("GET_EEPROM_LENGTH", 0x1e);
const DeviceCommand DeviceCommand::GET_EEPROM_BYTE("GET_EEPROM_BYTE", 0x1f);
const DeviceCommand DeviceCommand::SET_EEPROM_BYTE("SET_EEPROM_BYTE", 0x20);
const DeviceCommand DeviceCommand::GET_GAIN("GET_GAIN", 0x32);
const DeviceCommand DeviceCommand::SET_GAIN("SET_GAIN", 0x33);
const DeviceCommand DeviceCommand::GET_OFFSET("GET_OFFSET", 0x34);
const DeviceCommand DeviceCommand::SET_OFFSET("SET_OFFSET", 0x35);
const DeviceCommand DeviceCommand::GET_EXP_TIME("GET_EXP_TIME", 0x36);
const DeviceCommand DeviceCommand::SET_EXP_TIME("SET_EXP_TIME", 0x37);
const DeviceCommand DeviceCommand::GET_EXP_MODE("GET_EXP_MODE", 0x38);
const DeviceCommand DeviceCommand::SET_EXP_MODE("SET_EXP_MODE", 0x39);
const DeviceCommand DeviceCommand::GET_VDD_MODE("GET_VDD_MODE", 0x3a);
const DeviceCommand DeviceCommand::SET_VDD_MODE("SET_VDD_MODE", 0x3b);
const DeviceCommand DeviceCommand::GET_FLUSH_MODE("GET_FLUSH_MODE", 0x3c);
const DeviceCommand DeviceCommand::SET_FLUSH_MODE("SET_FLUSH_MODE", 0x3d);
const DeviceCommand DeviceCommand::GET_CLEAN_MODE("GET_CLEAN_MODE", 0x3e);
const DeviceCommand DeviceCommand::SET_CLEAN_MODE("SET_CLEAN_MODE", 0x3f);
const DeviceCommand DeviceCommand::GET_READOUT_SPD("GET_READOUT_SPD", 0x40);
const DeviceCommand DeviceCommand::SET_READOUT_SPD("SET_READOUT_SPD", 0x41);
const DeviceCommand DeviceCommand::GET_READOUT_MODE("GET_READOUT_MODE", 0x42);
const DeviceCommand DeviceCommand::SET_READOUT_MODE("SET_READOUT_MODE", 0x43);
const DeviceCommand DeviceCommand::GET_NORM_READOUT_DELAY("GET_NORM_READOUT_DELAY", 0x44);
const DeviceCommand DeviceCommand::SET_NORM_READOUT_DELAY("SET_NORM_READOUT_DELAY", 0x45);
const DeviceCommand DeviceCommand::GET_ROW_COUNT_ODD("GET_ROW_COUNT_ODD", 0x46);
const DeviceCommand DeviceCommand::SET_ROW_COUNT_ODD("SET_ROW_COUNT_ODD", 0x47);
const DeviceCommand DeviceCommand::GET_ROW_COUNT_EVEN("GET_ROW_COUNT_EVEN", 0x48);
const DeviceCommand DeviceCommand::SET_ROW_COUNT_EVEN("SET_ROW_COUNT_EVEN", 0x49);
const DeviceCommand DeviceCommand::GET_TEMP("GET_TEMP", 0x4a);
const DeviceCommand DeviceCommand::GET_EXP_TIMER_COUNT("GET_EXP_TIMER_COUNT", 0x4b);
const DeviceCommand DeviceCommand::PS_ON("PS_ON", 0x64);
const DeviceCommand DeviceCommand::PS_OFF("PS_OFF", 0x65);
const DeviceCommand DeviceCommand::CCD_VDD_ON("CCD_VDD_ON", 0x66);
const DeviceCommand DeviceCommand::CCD_VDD_OFF("CCD_VDD_OFF", 0x67);
const DeviceCommand DeviceCommand::AD_READ("AD_READ", 0x68);
const DeviceCommand DeviceCommand::AD_WRITE("AD_WRITE", 0x69);
const DeviceCommand DeviceCommand::TEST_PATTERN("TEST_PATTERN", 0x6a);
const DeviceCommand DeviceCommand::GET_DEBUG_VALUE("GET_DEBUG_VALUE", 0x6b);
const DeviceCommand DeviceCommand::GET_EEPROM_VIDPID("GET_EEPROM_VIDPID", 0x6c);
const DeviceCommand DeviceCommand::SET_EEPROM_VIDPID("SET_EEPROM_VIDPID", 0x6d);
const DeviceCommand DeviceCommand::ERASE_EEPROM("ERASE_EEPROM", 0x6e);

/* XXX: I am not sure that we need this*/
template <>
NamedEnum<int, DevicePipeId>::instances_list NamedEnum<int, DSI::DevicePipeId>::s_instances =
    std::set<const NamedEnum<int, DSI::DevicePipeId> *, DSI::DevicePipeId::NamedEnum_Ptr_LessThan>();
const DevicePipeId DevicePipeId::REQUEST("REQUEST", 0);
const DevicePipeId DevicePipeId::RESPONSE("RESPONSE", 1);
const DevicePipeId DevicePipeId::IMAGE("IMAGE", 2);

/* */
template <>
NamedEnum<int, DeviceResponse>::instances_list NamedEnum<int, DSI::DeviceResponse>::s_instances =
    std::set<const NamedEnum<int, DSI::DeviceResponse> *, DSI::DeviceResponse::NamedEnum_Ptr_LessThan>();
const DeviceResponse DeviceResponse::ACK("ACK", 0x06);
const DeviceResponse DeviceResponse::NACK("NACK", 0x15);

/* */
template <>
NamedEnum<int, DSI::UsbSpeed>::instances_list NamedEnum<int, DSI::UsbSpeed>::s_instances =
    std::set<const NamedEnum<int, DSI::UsbSpeed> *, DSI::UsbSpeed::NamedEnum_Ptr_LessThan>();
const UsbSpeed UsbSpeed::FULL("FULL", 0);
const UsbSpeed UsbSpeed::HIGH("HIGH", 1);

/* DSI A-D Register */
template <>
NamedEnum<int, AdRegister>::instances_list NamedEnum<int, DSI::AdRegister>::s_instances =
    std::set<const NamedEnum<int, DSI::AdRegister> *, DSI::AdRegister::NamedEnum_Ptr_LessThan>();
const AdRegister AdRegister::CONFIGURATION("CONFIGURATION", 0);
const AdRegister AdRegister::MUX_CONFIG("MUX_CONFIG", 1);
const AdRegister AdRegister::RED_PGA("RED_PGA", 2);
const AdRegister AdRegister::GREEN_PGA("GREEN_PGA", 3);
const AdRegister AdRegister::BLUE_PGA("BLUE_PGA", 4);
const AdRegister AdRegister::RED_OFFSET("RED_OFFSET", 5);
const AdRegister AdRegister::GREEN_OFFSET("GREEN_OFFSET", 6);
const AdRegister AdRegister::BLUE_OFFSET("BLUE_OFFSET", 7);

/*  */
template <>
NamedEnum<int, CleanMode>::instances_list NamedEnum<int, DSI::CleanMode>::s_instances =
    std::set<const NamedEnum<int, DSI::CleanMode> *, DSI::CleanMode::NamedEnum_Ptr_LessThan>();
const CleanMode CleanMode::ENABLED("ENABLED", 0);
const CleanMode CleanMode::DISABLED("DISABLED", 1);

/* */
template <>
NamedEnum<int, DesampleMethod>::instances_list NamedEnum<int, DSI::DesampleMethod>::s_instances =
    std::set<const NamedEnum<int, DSI::DesampleMethod> *, DSI::DesampleMethod::NamedEnum_Ptr_LessThan>();
const DesampleMethod DesampleMethod::HIGH_BYTE("HIGH_BYTE", 0);
const DesampleMethod DesampleMethod::LOW_BYTE("LOW_BYTE", 1);

/* */
template <>
NamedEnum<int, CopyRasters>::instances_list NamedEnum<int, DSI::CopyRasters>::s_instances =
    std::set<const NamedEnum<int, DSI::CopyRasters> *, DSI::CopyRasters::NamedEnum_Ptr_LessThan>();
const CopyRasters CopyRasters::ALL("ALL", 0);
const CopyRasters CopyRasters::EVEN("EVEN", 1);
const CopyRasters CopyRasters::ODD("ODD", 2);

/* */
template <>
NamedEnum<int, ExposureMode>::instances_list NamedEnum<int, DSI::ExposureMode>::s_instances =
    std::set<const NamedEnum<int, DSI::ExposureMode> *, DSI::ExposureMode::NamedEnum_Ptr_LessThan>();
const ExposureMode ExposureMode::NORMAL("NORMAL", 1);
const ExposureMode ExposureMode::BIN2X2("BIN2X2", 2);

/* */
template <>
NamedEnum<int, FlushMode>::instances_list NamedEnum<int, DSI::FlushMode>::s_instances =
    std::set<const NamedEnum<int, DSI::FlushMode> *, DSI::FlushMode::NamedEnum_Ptr_LessThan>();
const FlushMode FlushMode::CONTINUOUS("CONTINUOUS", 0);
const FlushMode FlushMode::BEFORE_EXPOSURE("BEFORE_EXPOSURE", 1);
const FlushMode FlushMode::NEVER("NEVER", 2);

/* */
template <>
NamedEnum<int, ReadoutMode>::instances_list NamedEnum<int, DSI::ReadoutMode>::s_instances =
    std::set<const NamedEnum<int, DSI::ReadoutMode> *, DSI::ReadoutMode::NamedEnum_Ptr_LessThan>();
const ReadoutMode ReadoutMode::DUAL("DUAL", 0);
const ReadoutMode ReadoutMode::SINGLE("SINGLE", 1);
const ReadoutMode ReadoutMode::ODD("ODD", 2);
const ReadoutMode ReadoutMode::EVEN("EVEN", 3);

/* */
template <>
NamedEnum<int, ReadoutSpeed>::instances_list NamedEnum<int, DSI::ReadoutSpeed>::s_instances =
    std::set<const NamedEnum<int, DSI::ReadoutSpeed> *, DSI::ReadoutSpeed::NamedEnum_Ptr_LessThan>();
const ReadoutSpeed ReadoutSpeed::NORMAL("NORMAL", 0);
const ReadoutSpeed ReadoutSpeed::HIGH("HIGH", 1);

/* */
template <>
NamedEnum<int, VddMode>::instances_list NamedEnum<int, DSI::VddMode>::s_instances =
    std::set<const NamedEnum<int, DSI::VddMode> *, DSI::VddMode::NamedEnum_Ptr_LessThan>();
const VddMode VddMode::AUTO("AUTO", 0);
const VddMode VddMode::ON("ON", 1);
const VddMode VddMode::OFF("OFF", 2);
};
