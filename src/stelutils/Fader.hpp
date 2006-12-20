/*
 * Stellarium
 * Copyright (C) 2005 Fabien Chereau
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

#ifndef _FADER_H_
#define _FADER_H_

#include <cstdio>
#include <cfloat>

// Class which manages a (usually smooth) transition between two states (typically ON/OFF) in function of a counter
// It used for various purpose like smooth transitions between 

class Fader
{
public:
	// Create and initialise
	Fader(bool _state, float _min_value=0.f, float _max_value=1.f) : state(_state), min_value(_min_value), max_value(_max_value) {;}
    virtual ~Fader() {;}
	// Increments the internal counter of delta_time ticks
	virtual void update(int delta_ticks) = 0;
	// Gets current switch state
	virtual float getInterstate(void) const = 0;
	virtual float get_interstate_percentage(void) const = 0;
	// Switchors can be used just as bools
	virtual Fader& operator=(bool s) = 0;
	bool operator==(bool s) const {return state==s;}
	operator bool() const {return state;}
	virtual void set_duration(int _duration) {;}
	virtual float get_duration(void) = 0;
	virtual void set_min_value(float _min) {min_value = _min;}
	virtual void set_max_value(float _max) {max_value = _max;}
	float get_min_value(void) {return min_value;}
	float get_max_value(void) {return max_value;}
protected:
	bool state;
	float min_value, max_value;
};

class BooleanFader : public Fader
{
public:
	// Create and initialise
	BooleanFader(bool _state=false, float _min_value=0.f, float _max_value=1.f) : Fader(_state, _min_value, _max_value) {;}
    ~BooleanFader() {;}
	// Increments the internal counter of delta_time ticks
	void update(int delta_ticks) {;}
	// Gets current switch state
	float getInterstate(void) const {return state ? max_value : min_value;}
	float get_interstate_percentage(void) const {return state ? 100.f : 0.f;}
	// Switchors can be used just as bools
	Fader& operator=(bool s) {state=s; return *this;}
	virtual float get_duration(void) {return 0.f;}
protected:
};

// Please note that state is updated instantaneously, so if you need to draw something fading in
// and out, you need to check the interstate value (!=0) to know to draw when on AND during transitions
class LinearFader : public Fader
{
public:
	// Create and initialise to default
	LinearFader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: Fader(_state, _min_value, _max_value)
	{
		is_transiting = false;
		duration = _duration;
		interstate = state ? max_value : min_value;
	}
	
    ~LinearFader() {;}
	
	// Increments the internal counter of delta_time ticks
	void update(int delta_ticks)
	{
		if (!is_transiting) return; // We are not in transition
		counter+=delta_ticks;
		if (counter>=duration)
		{
			// Transition is over
			is_transiting = false;
			interstate = target_value;
			// state = (target_value==max_value) ? true : false;
		}
		else
		{
			interstate = start_value + (target_value - start_value) * counter/duration;
		}

		//		printf("Counter %d  interstate %f\n", counter, interstate);
	}
	
	// Get current switch state
	float getInterstate(void) const { return interstate;}
	float get_interstate_percentage(void) const {return 100.f * (interstate-min_value)/(max_value-min_value);}
	
	// Faders can be used just as bools
	Fader& operator=(bool s)
	{

		if(is_transiting) {
			// if same end state, no changes
			if(s == state) return *this;
			
			// otherwise need to reverse course
			state = s;
			counter = duration - counter;
			float temp = start_value;
			start_value = target_value;
			target_value = temp;
			
		} else {

			if(state == s) return *this;  // no change

			// set up and begin transit
			state = s;
			start_value = s ? min_value : max_value;
			target_value = s ? max_value : min_value;
			counter=0;
			is_transiting = true;
		}
		return *this;
	}
	
	void set_duration(int _duration) {duration = _duration;}
	virtual float get_duration(void) {return duration;}
	void set_max_value(float _max) {
		if(interstate >=  max_value) interstate =_max;
		max_value = _max;
	}

protected:
	bool is_transiting;
	int duration;
	float start_value, target_value;
	int counter;
	float interstate;
};


// Please note that state is updated instantaneously, so if you need to draw something fading in
// and out, you need to check the interstate value (!=0) to know to draw when on AND during transitions
class ParabolicFader : public Fader
{
public:
	// Create and initialise to default
	ParabolicFader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: Fader(_state, _min_value, _max_value)
	{
		is_transiting = false;
		duration = _duration;
		interstate = state ? max_value : min_value;
	}
	
    ~ParabolicFader() {;}
	
	// Increments the internal counter of delta_time ticks
	void update(int delta_ticks)
	{
		if (!is_transiting) return; // We are not in transition
		counter+=delta_ticks;
		if (counter>=duration)
		{
			// Transition is over
			is_transiting = false;
			interstate = target_value;
			// state = (target_value==max_value) ? true : false;
		}
		else
		{
			interstate = start_value + (target_value - start_value) * counter/duration;
			interstate *= interstate;
		}

		// printf("Counter %d  interstate %f\n", counter, interstate);
	}
	
	// Get current switch state
	float getInterstate(void) const { return interstate;}
	float get_interstate_percentage(void) const {return 100.f * (interstate-min_value)/(max_value-min_value);}
	
	// Faders can be used just as bools
	Fader& operator=(bool s)
	{

		if(is_transiting) {
			// if same end state, no changes
			if(s == state) return *this;
			
			// otherwise need to reverse course
			state = s;
			counter = duration - counter;
			float temp = start_value;
			start_value = target_value;
			target_value = temp;
			
		} else {

			if(state == s) return *this;  // no change

			// set up and begin transit
			state = s;
			start_value = s ? min_value : max_value;
			target_value = s ? max_value : min_value;
			counter=0;
			is_transiting = true;
		}
		return *this;
	}
	
	void set_duration(int _duration) {duration = _duration;}
	virtual float get_duration(void) {return duration;}
protected:
	bool is_transiting;
	int duration;
	float start_value, target_value;
	int counter;
	float interstate;
};


/* better idea but not working...
class ParabolicFader : public LinearFader
{
public:
	ParabolicFader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: LinearFader(_duration, _min_value, _max_value, _state)
		{
		}
	
	// Increments the internal counter of delta_time ticks
	void update(int delta_ticks)
	{

		printf("Counter %d  interstate %f\n", counter, interstate);
		if (!is_transiting) return; // We are not in transition
		counter+=delta_ticks;
		if (counter>=duration)
		{
			// Transition is over
			is_transiting = false;
			interstate = target_value;
		}
		else
		{
			interstate = start_value + (target_value - start_value) * counter/duration;
			interstate *= interstate;
		}

		printf("Counter %d  interstate %f\n", counter, interstate);
	}
};
*/

#endif //_FADER_H_
