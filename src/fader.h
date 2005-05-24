/*
 * Stellarium
 * Copyright (C) 2005 Fabien Chéreau
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

class fader
{
public:
	// Create and initialise
	fader(bool _state, float _min_value=0.f, float _max_value=1.f) : state(_state), min_value(_min_value), max_value(_max_value) {;}
    virtual ~fader() {;}
	// Increments the internal counter of delta_time ticks
	virtual void update(int delta_ticks) = 0;
	// Gets current switch state
	virtual float get_interstate(void) const = 0;
	virtual float get_interstate_percentage(void) const = 0;
	// Switchors can be used just as bools
	virtual fader& operator=(bool s) = 0;
	bool operator==(bool s) const {return state==s;}
	operator bool() const {return state;}
	virtual void set_duration(int _duration) {;}
	void set_min_value(float _min) {min_value = _min;}
	void set_max_value(float _max) {max_value = _max;}
protected:
	bool state;
	float min_value, max_value;
};

class boolean_fader : public fader
{
public:
	// Create and initialise
	boolean_fader(bool _state=false, float _min_value=0.f, float _max_value=1.f) : fader(_state, _min_value, _max_value) {;}
    ~boolean_fader() {;}
	// Increments the internal counter of delta_time ticks
	void update(int delta_ticks) {;}
	// Gets current switch state
	float get_interstate(void) const {return state ? max_value : min_value;}
	float get_interstate_percentage(void) const {return state ? 100.f : 0.f;}
	// Switchors can be used just as bools
	fader& operator=(bool s) {state=s; return *this;}
protected:
};

// Please note that state is updated instantaneously, so if you need to draw something fading in
// and out, you need to check the interstate value (!=0) to know to draw when on AND during transitions
class linear_fader : public fader
{
public:
	// Create and initialise to default
	linear_fader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: fader(_state, _min_value, _max_value)
	{
		is_transiting = false;
		duration = _duration;
		interstate = state ? max_value : min_value;
	}
	
    ~linear_fader() {;}
	
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
	float get_interstate(void) const { return interstate;}
	float get_interstate_percentage(void) const {return 100.f * (interstate-min_value)/(max_value-min_value);}
	
	// Faders can be used just as bools
	fader& operator=(bool s)
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
	void set_min_value(float _min) {min_value = _min;}
	void set_max_value(float _max) {max_value = _max;}
	
protected:
	bool is_transiting;
	int duration;
	float start_value, target_value;
	int counter;
	float interstate;
};


// Please note that state is updated instantaneously, so if you need to draw something fading in
// and out, you need to check the interstate value (!=0) to know to draw when on AND during transitions
class parabolic_fader : public fader
{
public:
	// Create and initialise to default
	parabolic_fader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: fader(_state, _min_value, _max_value)
	{
		is_transiting = false;
		duration = _duration;
		interstate = state ? max_value : min_value;
	}
	
    ~parabolic_fader() {;}
	
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
	float get_interstate(void) const { return interstate;}
	float get_interstate_percentage(void) const {return 100.f * (interstate-min_value)/(max_value-min_value);}
	
	// Faders can be used just as bools
	fader& operator=(bool s)
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
	void set_min_value(float _min) {min_value = _min;}
	void set_max_value(float _max) {max_value = _max;}
	
protected:
	bool is_transiting;
	int duration;
	float start_value, target_value;
	int counter;
	float interstate;
};


/* better idea but not working...
class parabolic_fader : public linear_fader
{
public:
	parabolic_fader(int _duration=1000, float _min_value=0.f, float _max_value=1.f, bool _state=false) 
		: linear_fader(_duration, _min_value, _max_value, _state)
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
