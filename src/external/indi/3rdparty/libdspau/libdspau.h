/*
 *   libDSPAU - a digital signal processing library for astronomy usage
 *   Copyright (C) 2017  Ilia Platone <info@iliaplatone.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DSPAU_H
#define _DSPAU_H

#ifdef  __cplusplus
extern "C" {
#endif
#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT extern 
#endif
enum dspau_conversiontype {
	magnitude = 0,
	magnitude_dbv = 1,
	magnitude_rooted = 2,
	magnitude_squared = 3,
	phase_degrees = 4,
	phase_radians = 5,
};

/**
* @brief Create a spectrum from a double array of values
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param dims the number of dimensions of the input stream (input).
* @param sizes array with the lengths of each dimension of the input stream (input).
* @param conversion the output magnitude dspau_conversiontype type.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_spectrum(double* in, double* out, int dims, int *sizes, int conversion);

/**
* @brief A square law filter
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_squarelawfilter(double* in, double* out, int len);

/**
* @brief A low pass filter
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param samplingfrequency the sampling frequency of the input stream.
* @param frequency the cutoff frequency of the filter.
* @param q the cutoff slope.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_lowpassfilter(double* in, double* out, int len, double samplingfrequency, double frequency, double q);

/**
* @brief A high pass filter
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param samplingfrequency the sampling frequency of the input stream.
* @param frequency the cutoff frequency of the filter.
* @param q the cutoff slope.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_highpassfilter(double* in, double* out, int len, double samplingfrequency, double frequency, double q);

/**
* @brief A band pass filter
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param samplingfrequency the sampling frequency of the input stream.
* @param frequency the center frequency of the filter.
* @param q the cutoff slope.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_bandpassfilter(double* in, double* out, int len, double samplingfrequency, double frequency, double q);

/**
* @brief A band reject filter
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param samplingfrequency the sampling frequency of the input stream.
* @param frequency the center frequency of the filter.
* @param q the cutoff slope.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_bandrejectfilter(double* in, double* out, int len, double samplingfrequency, double frequency, double q);

/**
* @brief An auto-correlator
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream (input/output).
* @param skip skip n values at beginning (resulting len will be reduced).
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_autocorrelate(double* in, double* out, int *len, int skip);

/**
* @brief A cross-correlator
* @param x the first input stream. (input)
* @param y the second input stream. (input)
* @param len the length of the input streams.
* @return the resulting correlation degree.
* Return the correlation degree.
*/
double dspau_crosscorrelate(double* x, double* y, int len);

/**
* @brief A band-pass auto-correlator (Warning: high memory usage!)
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream. (input/output)
* @param skip skip n values at beginning (resulting len will be reduced).
* @param Q the slope of the band-pass filters.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_bandpasscorrelate(double* in, double* out, int *len, int skip, double Q);

/**
* @brief Gets minimum, mid, and maximum values of the input stream
* @param in the input stream. (input)
* @param len the length of the input stream.
* @param min the minimum value (output).
* @param max the maximum value (output).
* @return the mid value (max - min) / 2 + min.
* Return mid if success.
*/
double dspau_minmidmax(double* in, int len, double *min, double *max);

/**
* @brief A mean calculator
* @param in the input stream. (input)
* @param len the length of the input stream.
* @return the mean value of the stream.
* Return mean if success.
*/
double dspau_mean(double* in, int len);

/**
* @brief Subtract mean from stream
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_removemean(double* in, double* out, int len);

/**
* @brief Stretch minimum and maximum values of the input stream
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param min the desired minimum value.
* @param max the desired maximum value.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_stretch(double* in, double* out, int len, double min, double max);

/**
* @brief Normalize the input stream to the minimum and maximum values
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.

* @param min the clamping minimum value.
* @param max the clamping maximum value.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.

* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_normalize(double* in, double* out, int len, double min, double max);

/**
* @brief Subtract elements of one stream from another's
* @param in1 the input stream to be subtracted. (input)
* @param in2 the input stream with subtraction values. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_sub(double* in1, double* in2, double* out, int len);

/**
* @brief Sum elements of one stream to another's
* @param in1 the first input stream. (input)
* @param in2 the second input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_sum(double* in1, double* in2, double* out, int len);

/**
* @brief Divide elements of one stream to another's
* @param in1 the Numerators input stream. (input)
* @param in2 the Denominators input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_div(double* in1, double* in2, double* out, int len);

/**
* @brief Multiply elements of one stream to another's
* @param in1 the first input stream. (input)
* @param in2 the second input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_mul(double* in1, double* in2, double* out, int len);

/**
* @brief Subtract a value from elements of the input stream
* @param in the Numerators input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param val the value to be subtracted.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_sub1(double* in, double* out, int len, double val);

/**
* @brief Sum elements of the input stream to a value
* @param in the first input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param val the value used for this operation.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_sum1(double* in, double* out, int len, double val);

/**
* @brief Divide elements of the input stream to a value
* @param in the Numerators input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param val the denominator.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_div1(double* in, double* out, int len, double val);

/**
* @brief Multiply elements of the input stream to a value
* @param in the first input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param val the value used for this operation.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_mul1(double* in, double* out, int len, double val);

/**
* @brief Median elements of the inut stream
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @param size the length of the median.
* @param median the location of the median value.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_median(double* in, double* out, int len, int size, int median);

/**
* @brief Convert an 8bit unsigned array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_u8todouble(unsigned char* in, double* out, int len);

/**
* @brief Convert a 16bit unsigned array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_u16todouble(unsigned short int* in, double* out, int len);

/**
* @brief Convert a 32bit unsigned array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_u32todouble(unsigned int* in, double* out, int len);

/**
* @brief Convert a 64bit unsigned array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_u64todouble(unsigned long int* in, double* out, int len);

/**
* @brief Convert an 8bit signed array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_s8todouble(signed char* in, double* out, int len);

/**
* @brief Convert a 16bit signed array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_s16todouble(signed short int* in, double* out, int len);

/**
* @brief Convert a 32bit signed array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_s32todouble(signed int* in, double* out, int len);

/**
* @brief Convert a 64bit signed array into a double array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_s64todouble(signed long int* in, double* out, int len);

/**
* @brief Convert a double array into a 8bit unsigned array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletou8(double* in, unsigned char* out, int len);

/**
* @brief Convert a double array into a 16bit unsigned array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletou16(double* in, unsigned short int* out, int len);

/**
* @brief Convert a double array into a 32bit unsigned array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletou32(double* in, unsigned int* out, int len);

/**
* @brief Convert a double array into a 64bit unsigned array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletou64(double* in, unsigned long int* out, int len);

/**
* @brief Convert a double array into an 8bit signed array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletos8(double* in, signed char* out, int len);

/**
* @brief Convert a double array into a 16bit signed array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletos16(double* in, signed short int* out, int len);

/**
* @brief Convert a double array into a 32bit signed array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletos32(double* in, signed int* out, int len);

/**
* @brief Convert a double array into a 64bit signed array
* @param in the input stream. (input)
* @param out the output stream. (output)
* @param len the length of the input stream.
* @return the output stream if successfull elaboration. NULL if an
* error is encountered.
* Return 0 if success.
* Return -1 if any error occurs.
*/
DLL_EXPORT int dspau_doubletos64(double* in, signed long int* out, int len);

#ifdef  __cplusplus
}
#endif

#endif //_DSPAU_H
