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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELFADER_HPP
#define STELFADER_HPP

#include <QtGlobal>

//! @class StelFader
//! Manages a (usually smooth) transition between two states (typically ON/OFF) in function of a counter
//! It used for various purpose like smooth transitions
class StelFader
{
public:
	// Create and initialise
	StelFader(bool initialState) : state(initialState) {;}
	virtual ~StelFader() {;}
	//! Increments the internal counter of deltaMs milliseconds
	virtual void update(int deltaMs) = 0;
	//! Gets current value (between 0 and 1)
	virtual float getInterstate() const = 0;
	// Switchors can be used just as bools
	virtual StelFader& operator=(bool s) = 0;
	bool operator==(bool s) const {return state==s;}
	operator bool() const {return state;}
	virtual void setDuration(int) {;}
	virtual float getDuration() const = 0;
protected:
	float getTargetValue() const {return state ? 1.f : 0.f;}
	float getStartValue() const {return state ? 0.f : 1.f;}
	bool state;
};

//! @class BooleanFader
//! Implementation of StelFader which behaves like a normal boolean, i.e. no transition between on and off.
class BooleanFader : public StelFader
{
public:
	// Create and initialise
	BooleanFader(bool initialState=false) : StelFader(initialState) {;}
	~BooleanFader() {;}
	// Increments the internal counter of deltaTime ticks
	void update(int deltaMs) {Q_UNUSED(deltaMs);}
	// Gets current switch state
	float getInterstate() const {return state ? 1.f : 0.f;}
	// Switchors can be used just as bools
	StelFader& operator=(bool s) {state=s; return *this;}
	virtual float getDuration() const {return 0.f;}
};

//! @class LinearFader
//! Implementation of StelFader which implements a linear transition.
//! Please note that state is updated instantaneously, so if you need to draw something fading in
//! and out, you need to check the interstate value (!=0) to know to draw when on AND during transitions.
class LinearFader : public StelFader
{
public:
	// Create and initialise to default
	LinearFader(int _duration=1000, bool initialState=false)
		: StelFader(initialState), counter(_duration)
	{
		duration = _duration;
		interstate = getTargetValue();
	}

	~LinearFader() {;}

	// Increments the internal counter of deltaMs milliseconds
	void update(int deltaMs)
	{
		if (!isTransiting()) return; // We are not in transition
		counter+=deltaMs;
		if (counter>=duration)
		{
			// Transition is over
			interstate = getTargetValue();
		}
		else
		{
			interstate = getStartValue() + (getTargetValue() - getStartValue()) * counter/duration;
		}
	}

	// Get current switch state
	float getInterstate() const { return interstate;}

	// StelFaders can be used just as bools
	StelFader& operator=(bool s)
	{
		if (isTransiting()) {
			// if same end state, no changes
			if(s == state) return *this;

			// otherwise need to reverse course
			state = s;
			counter = duration - counter;
		} else {
			if (state == s) return *this;  // no change

			// set up and begin transit
			state = s;
			counter=0;
		}
		return *this;
	}

	void setDuration(int _duration)
	{
		if (counter >= duration)
			counter = _duration;
		duration = _duration;
	}
	virtual float getDuration() const {return duration;}

private:
	bool isTransiting() const { return counter < duration; }
	int duration;
	int counter;
	float interstate;
};


class ParabolicFader : public LinearFader
{
public:
	// Create and initialise to default
	ParabolicFader(int _duration=1000, bool initialState=false)
		: LinearFader(_duration, initialState)
	{
	}

	// Get current switch state
	float getInterstate(void) const { return LinearFader::getInterstate() * LinearFader::getInterstate();}
};

#endif // STELFADER_HPP
