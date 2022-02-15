/* -*-  Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 *  File:  RngStream.h for multiple streams of Random Numbers
 *  Copyright (C) 2001  Pierre L'Ecuyer (lecuyer@iro.umontreal.ca)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 *  02110-1301 USA
 *
 *  Linking this random number generator statically or dynamically with
 *  other modules is making a combined work based on this random number
 *  generator.  Thus, the terms and conditions of the GNU General Public
 *  License cover the whole combination.
 *
 *  In addition, as a special exception, the copyright holders of this
 *  random number generator give you permission to combine this random
 *  number generator program with free software programs or libraries
 *  that are released under the GNU LGPL and with code included in the
 *  standard release of ns-2 under the Apache 2.0 license or under
 *  otherwise-compatible licenses with advertising requirements (or
 *  modified versions of such code, with unchanged license).  You may
 *  copy and distribute such a system following the terms of the GNU GPL
 *  for this random number generator and the licenses of the other code
 *  concerned, provided  that you include the source code of that other
 *  code when and as the GNU GPL requires distribution of source code.
 *
 *  Note that people who make modified versions of this random number
 *  generator are not obligated to grant this special exception for
 *  their modified versions; it is their choice whether to do so.
 *  The GNU General Public License gives permission to release a
 *  modified  version without this exception; this exception also makes
 *  it possible to release a modified version which carries forward
 *  this exception.
 *
 *  //Incorporated into rng.h and modified to maintain backward
 *  //compatibility with ns-2.1b8.  Users can use their current scripts
 *  //unmodified with the new RNG.  To get the same results as with the
 *  //previous RNG, define OLD_RNG when compiling (e.g.,
 *  //make -D OLD_RNG).
 *  // - Michele Weigle, University of North Carolina
 *  // (mcweigle@cs.unc.edu)  October 10, 2001
 * 
 * "@(#) $Header: /cvsroot/nsnam/ns-2/tools/rng.h,v 1.26 2006/02/02 18:19:44 mweigle Exp $ (LBL)";
 */

/* new random number generator */

#ifndef _rng_h_
#define _rng_h_

#include <math.h>
#include <stdlib.h>                     // for atoi

//#include "config.h"

#ifndef MAXINT
#define MAXINT  2147483647      // XX [for now]
#endif

/*
 * Use class RNG in real programs.
 */
class RNG 
{

public:
        RNG (unsigned int runId);

        /*
         * Added for new RNG
         */
        static void set_package_seed (const unsigned long seed[6]); 
        /*
          Sets the initial seed s 0 of the package to the six integers in the
          vector seed. The first 3 integers in the seed must all be less than
          m 1 = 4294967087, and not all 0; and the last 3 integers must all be
          less than m 2 = 4294944443, and not all 0. If this method is not
          called, the default initial seed is (12345, 12345, 12345, 12345,
          12345, 12345).
        */

        void reset_start_stream (); 
        /*
          Reinitializes the stream to its initial state: C g and B g are set
          to I g
        */

        void reset_start_substream (); 
        /*
          Reinitializes the stream to the beginning of its current substream:
          C g is set to B g.
        */

        void reset_next_substream (); 
        /*
          Reinitializes the stream to the beginning of its next substream: N g
          is computed, and C g and B g are set to N g .
        */

        void set_antithetic (bool a); 
        /*
          If a = true, the stream will start generating antithetic variates,
          i.e., 1 - U instead of U,until this method is called again with a =
          false.
        */

        void increased_precis (bool incp); 
        /*
          After calling this method with incp = true, each call to the
          generator (direct or indirect) for this stream will return a uniform
          random number with more bits of resolution (53 bits if machine
          follows IEEE 754 standard) instead of 32 bits, and will advance the
          state of the stream by 2 steps instead of 1. More precisely, if s is
          a stream of the class RngStream, in the non� antithetic case, the
          instruction ``u = s.RandU01()'' will be equivalent to ``u =
          (s.RandU01() + s.RandU01() * fact) % 1.0'' where the constant fact
          is equal to 2 -24 . This also applies when calling RandU01
          indirectly (e.g., via RandInt, etc.). By default, or if this method
          is called again with incp = false, each to call RandU01 for this
          stream advances the state by 1 step and returns a number with 32
          bits of resolution.
        */

        void advance_state (long e, long c); 
        /*
          Advances the state by n steps (see below for the meaning of n),
          without modifying the states of other streams or the values of B g
          and I g in the current object. If e > 0, then n =2 e + c;if e < 0,
          then n = -2 -e + c;and if e = 0,then n = c. Note:c is allowed to
          take negative values.  We discourage the use of this method.
        */

        double rand_u01 (); 
        /*
          Normally, returns a (pseudo)random number from the uniform
          distribution over the interval (0, 1), after advancing the state by
          one step. The returned number has 32 bits of precision in the sense
          that it is always a multiple of 1/(2 32 - 208). However, if
          IncreasedPrecis(true) has been called for this stream, the state is
          advanced by two steps and the returned number has 53 bits of
          precision.
        */

        long rand_int (long i, long j); 
        /*
          Returns a (pseudo)random number from the discrete uniform distribution
          over the integers {i, i +1,...,j}. Makes one call to RandU01.
        */

        inline double uniform_double() { // range [0.0, 1.0)
                return rand_u01 ();
        }
        double uniform () { return uniform_double (); }

        // these are probably what you want to use
        inline double uniform(double r) 
        { return (r * uniform());}
        inline double uniform(double a, double b)
        { return (a + uniform(b - a)); }
        inline double exponential()
        { return (-log(uniform())); }
        inline double exponential(double r)
        { return (r * exponential());}
        inline int geometric (double p)
        { return 1 + int (log(uniform())/log(1-p)); }
        // See "Wide-area traffic: the failure of poisson modeling", Vern 
        // Paxson and Sally Floyd, IEEE/ACM Transaction on Networking, 3(3),
        // pp. 226-244, June 1995, on characteristics of counting processes 
        // with Pareto interarrivals.
        inline double pareto(double scale, double shape) { 
                // When 1 < shape < 2, its mean is scale**shape, its 
                // variance is infinite.
                return (scale * (1.0/pow(uniform(), 1.0/shape)));
        }
        inline double paretoII(double scale, double shape) { 
                return (scale * ((1.0/pow(uniform(), 1.0/shape)) - 1));
        }
        double normal(double avg, double std);
        inline double lognormal(double avg, double std) { 
                return (exp(normal(avg, std))); 
        }

        inline double rweibull(double shape, double scale) {
                return (pow (-log (uniform()), 1/shape) * scale);
        }
        inline double qweibull(double p, double shape, double scale) {
                return (pow (-log(1-p), 1/shape) * scale);
        }

        inline double laplacian(double mean,double b, double maxValue){
                double u=uniform(-0.5,0.5);
                double sign=fabs(u)/u;
                double res = mean-b*sign*log(1-2*fabs(u));
                if ( res < 0 ) return 0;
                else return (res>maxValue)?maxValue:res;
        }

#define LOG2 0.6931471806
        inline double logit(double x_) {
                return log(x_/(1-x_))/LOG2;
        }
        inline double logitinv(double x_) {
                return pow(2, x_)/(1+pow(2.0, x_));
        }

        /*
          Declarations for random variables used in PackMime
          Definitions are in packmime/packmime_HTTP_rng.cc
        */
        double gammln(double xx);       
        double pnorm(double q);
        double rnorm();
        int rbernoulli(double p);
        double rgamma(double a, double scale);
        double exp_rand();

        //! Generate poisson-distributed numbers (Knuth algorithm)
        unsigned int poisson(double lambda)
        {
                unsigned int k = 0;
                double p = 1;
                double L = exp( -lambda );
                do {
                        k++;
                        double u = uniform();
                        p = p * u;
                } while(p > L);
                return k-1;
        }

protected:   // need to be public?
        double Cg_[6], Bg_[6], Ig_[6]; 
        /*
          Vectors to store the current seed, the beginning of the current block
          (substream) and the beginning of the current stream.
        */

        bool anti_, inc_prec_; 
        /*
          Variables to indicate whether to generate antithetic or increased
          precision random numbers.
        */

        char name_[100]; 
        /*
          String to store the optional name of the current RngStream object. 
        */

        static double next_seed_[6]; 
        /*
          Static vector to store the beginning state of the next RngStream to 
          be created (instantiated).
        */

        double U01 (); 
        /*
          The backbone uniform random number generator. 
        */

        double U01d (); 
        /*
          The backbone uniform random number generator with increased 
          precision. 
        */      
        static RNG* default_;
}; 

#endif /* _rng_h_ */
