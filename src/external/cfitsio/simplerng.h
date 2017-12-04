/* 
   Simple Random Number Generators
       - getuniform - uniform deviate [0,1]
       - getnorm    - gaussian (normal) deviate (mean=0, stddev=1)
       - getpoisson - poisson deviate for given expected mean lambda

   This code is adapted from SimpleRNG by John D Cook, which is
   provided in the public domain.

   The original C++ code is found here:
   http://www.johndcook.com/cpp_random_number_generation.html

   This code has been modified in the following ways compared to the
   original.
     1. convert to C from C++
     2. keep only uniform, gaussian and poisson deviates
     3. state variables are module static instead of class variables
     4. provide an srand() equivalent to initialize the state
*/

extern void simplerng_setstate(unsigned int u, unsigned int v);
extern void simplerng_getstate(unsigned int *u, unsigned int *v);
extern void simplerng_srand(unsigned int seed);
extern double simplerng_getuniform(void);
extern double simplerng_getnorm(void);
extern int simplerng_getpoisson(double lambda);
extern double simplerng_logfactorial(int n);
