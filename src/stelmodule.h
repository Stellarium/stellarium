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
 
#ifndef STELMODULE_H
#define STELMODULE_H

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <boost/any.hpp>
#include "callbacks.hpp"

#include "stellarium.h"

// Predeclaration
class Projector;
class Navigator;
class ToneReproductor;
class LoadingBar;
class InitParser;

using namespace std;

/**
	@author Fabien Chereau <stellarium@free.fr>
	TODO: Use boost::bind + boost::function instead of callback
*/
class StelModule{
public:
	StelModule() {;}

	virtual ~StelModule() {;}
	
	//! Initialize itself from a configuration (.ini) file
	//! If the initialization takes significant time, the progress should be displayed on the loading bar.
	virtual void init(const InitParser& conf, LoadingBar& lb) = 0;
	
	//! Execute all the openGL drawing functions for this module.
	//! @return the max squared distance in pixels any single object has moved since the previous update.
	virtual double draw(Projector* prj, const Navigator * nav, ToneReproductor* eye) {return 0;}
	
	//! Update the module with respect to the time.
	//! @param delta_time the time increment in second since last call.
	virtual void update(double deltaTime) {;}
	
	//! @brief Update i18n strings from english names according to current global sky and application language.
	//! This method also reload the proper fonts depending on the language.
	//! The translation shall be done using the Translator provided by the StelApp singleton instance.
	virtual void updateI18n() = 0;
		   
	//! @brief Update sky culture, i.e. load data if necessary and translate them to current sky language if needed.
	virtual void updateSkyCulture(LoadingBar& lb) = 0;
	
	//! Get the identifier of the module. Must be unique, it is also the name of the .so (or .dll) file
	//! if the module comes from a shared library
	virtual string getModuleID() const = 0;	
	
	//! Get the version of the module, default is stellarium main version
	virtual string getModuleVersion() const {return VERSION;}
	
	//! Get the name of the module author
	virtual string getAuthorName() {return "Stellarium's Team";}
	
	//! Get the email adress of the module author
	virtual string getAuthorEmail() {return "http://stellarium.org";}
	
	///////////////////////////////////////////////////////////////////////////
	// Properties managment
	
	//! Init itself from a .ini file
	void loadProperties(const string& iniFile);
	
	//! Save itself into a .ini file for subsequent reload
	void saveProperties(const string& iniFile) const;
	
	//! Set a property value from a string: try to cast the value as a bool, int, double, string, Vec3d
	void setPropertyStr(const string& key, const string& value);
	
	//! Get a property value as string: try to cast the value as a bool, int, double, string, Vec3d
	string getPropertyStr(const string& key) const;
	
	//! Set a property value from a defined type
	template<typename ValueType>
	void setProperty(const string& key, ValueType value)
	{
		try
		{
			propertyAccessor& a = getAccessor(key);
			a.set(value);
		}
		catch (boost::bad_any_cast)
		{
			std::cerr << "Error while setting property \"" << key << "\", value="<< value << ",  incorrect type passed." << std::endl;
			assert(0);
		}
	}
	
	//! Get a property value from a defined type
	template<typename ValueType>
	ValueType getProperty(const string& key)
	{
		const propertyAccessor& a = getAccessor(key);
		return a.template get<ValueType>();
	}
	
	//! Get a list containing 3 strings per element : propertyKey, value, comment
	vector<vector<string> > getPropertiesList() const
	{
		vector<vector<string> > result;
		
		map<string, propertyAccessor>::const_iterator iter;
		for (iter=properties.begin(); iter!=properties.end(); ++iter)
		{
			vector<string> line;
			line.push_back((*iter).first);
			line.push_back(getPropertyStr((*iter).first));
			line.push_back((*iter).second.comment);
			result.push_back(line);
		}
		return result;
	}
	
protected:
	//! Add a property to the property list
#define	REGISTER_PROPERTY(type, key, getter, setter, comment) registerProperty<type>( (key) , boost::callback<type>(this, (getter) ), boost::callback<void, type>(this, (setter) ), (comment) )

	//! Add a property to the class
	template<typename ValueType>
	void registerProperty(const string& key, boost::callback<ValueType> getter, boost::callback<void, ValueType> setter, const string& comment = "")
	{
		propertyAccessor myAccessor;
		myAccessor.init(getter, setter);
		myAccessor.comment = comment;
		properties.insert(std::pair<string, propertyAccessor>(key, myAccessor));
	}

private:
	// Enable the access to setter and getter 
	class propertyAccessor
	{
	public:
		template<typename T>
		void init(boost::callback<T> getter, boost::callback<void, T> setter)
		{
			callbackget = boost::any(getter);
			callbackset = boost::any(setter);
		}
		
		//! Call the setter
		template<typename T>
		void set(T value)
		{
			boost::callback<void, T> setter = boost::any_cast<boost::callback<void, T> >(callbackset);
			setter(value);
		}
		
		//! Call the getter
		template<typename T>
		T get() const
		{
			boost::callback<T> getter = boost::any_cast<boost::callback<T> >(callbackget);
			return getter();
		}
		
		//! Description of the property
		string comment;
		
	private:
		boost::any callbackget;
		boost::any callbackset;
	};
	
	//! Get the const accessor matching the given key and fails if doesn't find it
	const propertyAccessor& getAccessor(const string& key) const
	{
		std::map<string, propertyAccessor>::const_iterator iter = properties.find(key);
		if (iter==properties.end())
		{
			std::cerr << "Couldn't find property \"" << key << "\" in module \"" << getModuleID() << "\"." << std::endl;
			assert(0);
		}
		return (*iter).second;
	}
	
	//! Get the accessor matching the given key and fails if doesn't find it
	propertyAccessor& getAccessor(const string& key)
	{
		std::map<string, propertyAccessor>::iterator iter = properties.find(key);
		if (iter==properties.end())
		{
			std::cerr << "Couldn't find property \"" << key << "\" in module \"" << getModuleID() << "\"." << std::endl;
			assert(0);
		}
		return (*iter).second;
	}
	
	//! The list of all the properties and associated accessors
	std::map<string, propertyAccessor> properties;
};

#endif
