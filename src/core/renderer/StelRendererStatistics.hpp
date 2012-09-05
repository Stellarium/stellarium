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

//! Renderer statistics backend.
//! 
//! Provides storage and functions to set statistics.
//! This class is designed to behave like a map but to have minimum overhead.
//!
//! When a statistic is added (specifying a name), it is assigned an index
//! which can be used to access it. To make this sane an enum with names 
//! corresponding to statistic names can be used 
//! to access the statistics.
//!
//! For example of this see the QGLRendererStatistics enum in StelQGLRenderer.hpp 
//! and the StelQGLRenderer::initStatistics() function.
//!
//!
//! @note This is an internal function of the Renderer subsystem and should not be used elsewhere.
struct RendererStatisticsBackend
{
	//! Maximum number of statistics.
	//!
	//! Can be increased when needed but note that size of this object, 
	//! which is copied a few times (2 currently) per frame is determined 
	//! by two static-size arrays with this size.
	static const int MAX_STATISTICS = 32;

	//! Statistic values. Fixed-size array is used to allow simple copying 
	//! without allocation overhead.
	double statistics[MAX_STATISTICS];

	//! Names of the statistics. 
	//!
	//! We only store pointers - names are stored outside (generally as 
	//! global constants - literals)
	//! For index i, statisticNames[i] is the name of statistics[i]
	//!
	//! Note that the names are only used for output - not for indexing.
	const char* statisticNames[MAX_STATISTICS];

	//! Number of statistic values used.
	int statisticCount;

	//! Construct a RendererStatisticsBackend.
	RendererStatisticsBackend()
		: statisticCount(0)
	{
		memset(statistics, '\0', MAX_STATISTICS * sizeof(double));
		memset(statisticNames, '\0', MAX_STATISTICS * sizeof(const char*));
	}

	//! Access statistic with specified index (e.g. to modify it).
	//!
	//! Statistics are accessed by integer indices.
	//! A clean way to do this is to use an enum with name corresponding
	//! to the name of the statistic.
	double& operator [] (int index)
	{
		Q_ASSERT_X(index >= 0 && index < statisticCount, Q_FUNC_INFO,
		           "Index of a statistic out of range ");
		return statistics[index];
	}

	//! Add a statistic with specified name.
	//!
	//! Also returns the index of the statistic. Index will be 0 for the 
	//! first statistic added, 1 for the second and so on.
	//!
	//! @note ALL statistics must be added before any statistics are set 
	//!       (i.e. at renderer initialization).
	//!
	//! @param name Name of the statistic. This MUST exist as long as the statistics
	//!             exist. The best way to ensure this is to use a string literal.
	//! @return Index of the statistic to use with the [] operator.
	int addStatistic(const char* name)
	{
		Q_ASSERT_X(statisticCount < MAX_STATISTICS, Q_FUNC_INFO,
		           "Adding too many statistics (at most 32 are supported)");
		statisticNames[statisticCount++] = name;
		return statisticCount - 1;
	}
};

//! Stores and provides access to statistics about a StelRenderer backend.
//!
//! This acts as a map of stings (const char*) and doubles.
//!
//! It can be iterated like this:
//!
//! @code
//! const char* name, double value;
//! while(statistics.getNext(name, value))
//! {
//! 	//Do something with name and value
//! }
//! @endcode
//!
//! The reset() member fuction can be used to reset iteration to start.
class StelRendererStatistics
{
friend class StelQGLRenderer;
public:
	//! Construct an empty StelRendererStatistics object.
	StelRendererStatistics()
		: iterationIndex(0)
	{
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
		if(iterationIndex == backend.statisticCount)
		{
			return false;
		}

		name  = backend.statisticNames[iterationIndex];
		value = backend.statistics[iterationIndex];
		++iterationIndex;

		return true;
	}

	//! Reset iteration to the beginning.
	void resetIteration()
	{
		iterationIndex = 0;
	}

private:
	//! Stores the actual statistics.
	RendererStatisticsBackend backend;

	//! Current index in backend. Used in iteration when getting statistics.
	int iterationIndex;

	//! Used by StelRenderer implementations (friends of StelRendererStatistics) to write statistics
	RendererStatisticsBackend& getBackend() {return backend;}
};

#endif // _STELRENDERERSTATISTICS_HPP_
