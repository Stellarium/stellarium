/*
 * Stellarium
 * Copyright (C) 2012 Ferdinand Majerech
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

#ifndef _STELRENDERERSTATISTICS_HPP_
#define _STELRENDERERSTATISTICS_HPP_


//! Enumerates options of what should be done with a statistic 
//! when the StelRendererStatistics swap() function is callse (i.e. at the end of frame)
enum StatisticSwapMode
{
	StatisticSwapMode_SetToZero,
	StatisticSwapMode_DoNothing
};

//! Stores and provides access to statistics about a StelRenderer backend.
//!
//! This acts as a map of stings (const char*) and doubles.
//!
//! Two sets of statistics are stored: current (this frame, currently recorded)
//! and previous (previous frame, currently readable.)
//!
//! To read statistics (from previous frame), they can be iterated like this:
//!
//! @code
//! const char* name, double value;
//! while(statistics.getNext(name, value))
//! {
//! 	//Do something with name and value
//! }
//! @endcode
//!
//! To record statistics, the [] operator is used, modifying the current frame's
//! statistics. The index is an integer previously returned by the addStatistic() 
//! member function:
//!
//! @code
//! statistics[statisticID] += 1.0;
//! @endcode
//!
//! The resetIteration() member fuction can be used to reset iteration to start.
class StelRendererStatistics
{
friend class StelQGLRenderer;
public:
	//! Construct an empty StelRendererStatistics object.
	StelRendererStatistics()
		: iterationIndex(0)
		, statisticCount(0)
	{
		memset(statistics,         '\0', MAX_STATISTICS * sizeof(double));
		memset(previousStatistics, '\0', MAX_STATISTICS * sizeof(double));
		memset(statisticNames,     '\0', MAX_STATISTICS * sizeof(const char*));
		memset(swapModes,          '\0', MAX_STATISTICS * sizeof(StatisticSwapMode));
	}

	//! Get next statistic name and value.
	//!
	//! @param name  Name of the statistic will be stored here (only a pointer to it).
	//! @param value Value of the statistic will be stored here.
	//!
	//! @return true if we got a name/value, false if we're done iterating and 
	//!         name and value were not set.
	bool getNext(const char*& name, double& value)
	{
		Q_ASSERT_X(iterationIndex <= statisticCount, Q_FUNC_INFO,
		           "Statistics iteration index out of bounds");
		if(iterationIndex == statisticCount)
		{
			return false;
		}

		name  = statisticNames[iterationIndex];
		value = previousStatistics[iterationIndex];
		++iterationIndex;

		return true;
	}

	//! Reset iteration to the beginning.
	void resetIteration()
	{
		iterationIndex = 0;
	}

	//! Access statistic with specified index to modify it.
	//!
	//! Statistics are accessed by integer indices.
	//! A clean way to do this is to use an enum with name corresponding
	//! to the name of the statistic.
	double& operator [] (int index)
	{
		Q_ASSERT_X(index >= 0 && index < statisticCount, Q_FUNC_INFO,
		           "Index of a statistic out of range");
		return statistics[index];
	}

	//! Add a statistic with specified name.
	//!
	//! Also returns the index of the statistic. Index will be 0 for the 
	//! first statistic added, 1 for the second and so on.
	//!
	//! @param name     Name of the statistic. This MUST exist as long as the statistics
	//!                 exist. The best way to ensure this is to use a string literal.
	//! @param swapMode Specifies what to do when when the swap() function is called 
	//!                 (at the end of a frame). Used for statistics that are recorded 
	//!                 separately each frame and need to be zeroed out.
	//! @return Index of the statistic to use with the [] operator.
	int addStatistic
		(const char* name, const StatisticSwapMode swapMode = StatisticSwapMode_DoNothing)
	{
		Q_ASSERT_X(statisticCount < MAX_STATISTICS, Q_FUNC_INFO,
		           "Can't add any more statistics (at most 48 are supported)");
		swapModes[statisticCount]    = swapMode;
		statisticNames[statisticCount] = name;
		++statisticCount;
		return statisticCount - 1;
	}

	//! Called at the end of frame - changes current statistics to previous statistics.
	//!
	//! Also handles StatisticSwapMode logic, e.g. zeroing out statistics
	//! that have StatisticSwapMode_SetToZero.
	void swap()
	{
		memcpy(previousStatistics, statistics, MAX_STATISTICS * sizeof(double));
		for(int s = 0; s < statisticCount; ++s)
		{
			if(swapModes[s] == StatisticSwapMode_SetToZero)
			{
				statistics[s] = 0.0f;
			}
			else if(!swapModes[s] == StatisticSwapMode_DoNothing)
			{
				Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown statistic swap mode");
			}
		}
	}

private:
	//! Current index in backend. Used in iteration when getting statistics.
	int iterationIndex;

	//! Maximum number of statistics.
	//!
	//! Can be increased when needed but note that size of this object, 
	//! which is copied once per frame is determined 
	//! by four static-size arrays with this size.
	static const int MAX_STATISTICS = 40;

	//! Statistic values. Fixed-size array is used to allow simple copying 
	//! without allocation overhead.
	//!
	//! This is where statistics are modified.
	double statistics[MAX_STATISTICS];

	//! Statistics from the previous frame, saved at the last swap() call.
	//!
	//! These are read by iteration.
	double previousStatistics[MAX_STATISTICS];

	//! Names of the statistics. 
	//!
	//! We only store pointers - names are stored outside (generally as 
	//! global constants - literals)
	//! For index i, statisticNames[i] is the name of statistics[i]
	//!
	//! Note that the names are only used for output - not for indexing.
	const char* statisticNames[MAX_STATISTICS];

	//! Swap modes of each statistic.
	StatisticSwapMode swapModes[MAX_STATISTICS];

	//! Number of statistic values used.
	int statisticCount;
};

#endif // _STELRENDERERSTATISTICS_HPP_
