/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 
#include "StelModule.hpp"
#include "vecmath.h"
#include "InitParser.hpp"
#include "StelApp.hpp"

/*************************************************************************
 Get the version of the module, default is stellarium main version
*************************************************************************/
string StelModule::getModuleVersion() const
{
	return PACKAGE_VERSION;
}

/*************************************************************************
 Init itself from a .ini file
*************************************************************************/
void StelModule::loadProperties(const string& iniFile)
{
	InitParser conf;
	conf.load(iniFile);
	
	std::cout << "Loading properties for module " << conf.get_str(getModuleID()+":version") << std::endl;
	
	std::map<string, propertyAccessor>::const_iterator iter;
	for(iter=properties.begin();iter!=properties.end();++iter)
	{
		conf.set_str(getModuleID()+":"+(*iter).first, getPropertyStr((*iter).first));
	}
	conf.save(iniFile);	
}

//! Save itself into a .ini file for subsequent reload (don't modify the other entries)
void StelModule::saveProperties(const string& iniFile) const
{
	InitParser conf;
	conf.load(iniFile);
	
	conf.set_str(getModuleID()+":version", getModuleVersion());
	
	std::map<string, propertyAccessor>::const_iterator iter;
	for(iter=properties.begin();iter!=properties.end();++iter)
	{
		conf.set_str(getModuleID()+":"+(*iter).first, getPropertyStr((*iter).first));
	}
	conf.save(iniFile);
}

//! Get a property value as string: try to cast the value as a bool, int, double, string, Vec3d
string StelModule::getPropertyStr(const string& key) const
{
	std::ostringstream oss;
	propertyAccessor p = getAccessor(key);
	try
	{
		if (p.get<bool>())
		{
			oss << "true";
		}
		else
		{
			oss << "false";
		}
	}
	catch (boost::bad_any_cast)
	{
		try
		{
			oss << p.get<int>();
		}
		catch (boost::bad_any_cast)
		{
			try
			{
				oss << p.get<double>();
			}
			catch (boost::bad_any_cast)
			{
				try
				{
					oss << p.get<string>();
				}
				catch (boost::bad_any_cast)
				{
					try
					{
						oss << p.get<Vec3d>();
					}
					catch (boost::bad_any_cast)
					{
						std::cerr << "Error while getting property \"" << key << "\", can't find matching type." << std::endl;
						assert(0);
					}
				}
			}
		}
	}
	return oss.str();
}

//! Set a property value from a string: try to cast the value as a bool, int, double, string, Vec3d
void StelModule::setPropertyStr(const string& key, const string& value)
{
	propertyAccessor p = getAccessor(key);
	try
	{
		p.set<bool>(value=="true");
		if (value!="true" && value!="false")
		{
			std::cerr << "Error while setting property \"" << key << "\" to value \""<< value <<"\", boolean value can be \"true\" or \"false\"." << std::endl;
		}
	}
	catch (boost::bad_any_cast)
	{
		try
		{
			p.set<string>(value);
		}
		catch (boost::bad_any_cast)
		{
			try
			{
				istringstream iss(value);
				int i;
				iss >> i;
				p.set<int>(i);
			}
			catch (boost::bad_any_cast)
			{
				try
				{
					istringstream iss(value);
					double d;
					iss >> d;
					p.set<double>(d);
				}
				catch (boost::bad_any_cast)
				{
					std::cerr << "Error while setting property \"" << key << "\" to value \""<< value <<"\", can't find matching type." << std::endl;
					assert(0);
				}
			}
		}
	}
}
