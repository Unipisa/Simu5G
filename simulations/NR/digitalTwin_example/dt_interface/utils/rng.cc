/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 *  File: RngStream.cc for multiple streams of Random Numbers 
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
 *  //Incorporated into rng.cc and modified to maintain backward
 *  //compatibility with ns-2.1b8.  Users can use their current scripts
 *  //unmodified with the new RNG.  To get the same results as with the
 *  //previous RNG, define OLD_RNG when compiling (e.g.,
 *  //make -D OLD_RNG).
 *  // - Michele Weigle, University of North Carolina
 *  // (mcweigle@cs.unc.edu)  October 10, 2001
 */

#ifndef lint
static const char rcsid[] =
"@(#) $Header: /cvsroot/nsnam/ns-2/tools/rng.cc,v 1.31 2006/02/21 15:20:20 mahrenho Exp $ (LBL)";
#endif

/* new random number generator */

#include <sys/time.h>			// for gettimeofday
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include "rng.h"

RNG* RNG::default_ = NULL;

double
RNG::normal(double avg, double std)
{
	static int parity = 0;
	static double nextresult;
	double sam1, sam2, rad;
   
	if (std == 0) return avg;
	if (parity == 0) {
		sam1 = 2*uniform() - 1;
		sam2 = 2*uniform() - 1;
		while ((rad = sam1*sam1 + sam2*sam2) >= 1) {
			sam1 = 2*uniform() - 1;
			sam2 = 2*uniform() - 1;
		}
		rad = sqrt((-2*log(rad))/rad);
		nextresult = sam2 * rad;
		parity = 1;
		return (sam1 * rad * std + avg);
	}
	else {
		parity = 0;
		return (nextresult * std + avg);
	}
}

using namespace std; 
namespace 
{ 
	const double m1 = 4294967087.0; 
	const double m2 = 4294944443.0; 
	const double norm = 1.0 / (m1 + 1.0); 
	const double a12 = 1403580.0; 
	const double a13n = 810728.0; 
	const double a21 = 527612.0; 
	const double a23n = 1370589.0; 
	const double two17 = 131072.0; 
	const double two53 = 9007199254740992.0; 
	const double fact = 5.9604644775390625e-8; /* 1 / 2^24 */ 

	// The following are the transition matrices of the two MRG 
	// components (in matrix form), raised to the powers -1, 1, 
	// 2^76, and 2^127, resp. 

	const double InvA1[3][3] = { // Inverse of A1p0 
		{ 184888585.0, 0.0, 1945170933.0 }, 
		{ 1.0, 0.0, 0.0 }, 
		{ 0.0, 1.0, 0.0 } 
	}; 

	const double InvA2[3][3] = { // Inverse of A2p0 
		{ 0.0, 360363334.0, 4225571728.0 }, 
		{ 1.0, 0.0, 0.0 }, 
		{ 0.0, 1.0, 0.0 } 
	}; 

	const double A1p0[3][3] = { 
		{ 0.0, 1.0, 0.0 }, 
		{ 0.0, 0.0, 1.0 }, 
		{ -810728.0, 1403580.0, 0.0 } 
	}; 

	const double A2p0[3][3] = { 
		{ 0.0, 1.0, 0.0 }, 
		{ 0.0, 0.0, 1.0 }, 
		{ -1370589.0, 0.0, 527612.0 } 
	}; 

	const double A1p76[3][3] = { 
		{ 82758667.0, 1871391091.0, 4127413238.0 }, 
		{ 3672831523.0, 69195019.0, 1871391091.0 }, 
		{ 3672091415.0, 3528743235.0, 69195019.0 } 
	}; 

	const double A2p76[3][3] = { 
		{ 1511326704.0, 3759209742.0, 1610795712.0 }, 
		{ 4292754251.0, 1511326704.0, 3889917532.0 }, 
		{ 3859662829.0, 4292754251.0, 3708466080.0 } 
	}; 

	const double A1p127[3][3] = { 
		{ 2427906178.0, 3580155704.0, 949770784.0 }, 
		{ 226153695.0, 1230515664.0, 3580155704.0 }, 
		{ 1988835001.0, 986791581.0, 1230515664.0 } 
	}; 

	const double A2p127[3][3] = { 
		{ 1464411153.0, 277697599.0, 1610723613.0 }, 
		{ 32183930.0, 1464411153.0, 1022607788.0 }, 
		{ 2824425944.0, 32183930.0, 2093834863.0 } 
	}; 

	//------------------------------------------------------------------- 
	// Return (a*s + c) MOD m; a, s, c and m must be < 2^35 
	// 

	double MultModM (double a, double s, double c, double m) 
	{ 
		double v; 
		long a1; 
		v=a*s+c; 

		if (v >= two53 || v <= -two53) { 
			a1 = static_cast<long> (a / two17); a -= a1 * two17; 
			v =a1*s; 
			a1 = static_cast<long> (v / m); v -= a1 * m; 
			v = v * two17 + a * s + c; 
		} 
		a1 = static_cast<long> (v / m); 
		/* in case v < 0)*/ 
		if ((v -= a1 * m) < 0.0) return v += m; else return v; 
	} 

	//------------------------------------------------------------------- 
	// Compute the vector v = A*s MOD m. Assume that -m < s[i] < m. 
	// Works also when v = s. 
	// 
	void MatVecModM (const double A[3][3], const double s[3], double v[3], 
			 double m) 
	{ 
		int i; 
		double x[3]; // Necessary if v = s 
		for (i = 0; i < 3; ++i) { 
			x[i] = MultModM (A[i][0], s[0], 0.0, m); 
			x[i] = MultModM (A[i][1], s[1], x[i], m); 
			x[i] = MultModM (A[i][2], s[2], x[i], m); 
		} 
		for (i = 0; i < 3; ++i) 
			v[i] = x[i]; 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix C = A*B MOD m. Assume that -m < s[i] < m. 
	// Note: works also if A = C or B = C or A = B = C. 
	// 
	void MatMatModM (const double A[3][3], const double B[3][3], 
			 double C[3][3], double m) 
	{ 
		int i, j; 
		double V[3], W[3][3]; 
		for (i = 0; i < 3; ++i) { 
			for (j = 0; j < 3; ++j) 
				V[j] = B[j][i]; 
			MatVecModM (A, V, V, m); 
			for (j = 0; j < 3; ++j) 

				W[j][i] = V[j]; 
		} 
		for (i = 0; i < 3; ++i) 
			for (j = 0; j < 3; ++j) 
				C[i][j] = W[i][j]; 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix B = (A^(2^e) Mod m); works also if A = B. 
	// 
	void MatTwoPowModM (const double A[3][3], double B[3][3], double m, 
			    long e) 
	{ 
		int i, j; 
		/* initialize: B = A */ 
		if (A != B) { 
			for (i = 0; i < 3; ++i) 
				for (j = 0; j < 3; ++j) 
					B[i][j] = A[i][j]; 
		} 
		/* Compute B = A^(2^e) mod m */ 
		for (i = 0; i < e; i++) 
			MatMatModM (B, B, B, m); 
	} 

	//------------------------------------------------------------------- 
	// Compute the matrix B = (A^n Mod m); works even if A = B. 
	// 
	void MatPowModM (const double A[3][3], double B[3][3], double m, 
			 long n) 
	{ 
		int i, j; 
		double W[3][3]; 
		/* initialize: W = A; B = I */ 
		for (i = 0; i < 3; ++i) 
			for (j = 0; j < 3; ++j) { 
				W[i][j] = A[i][j]; 
				B[i][j] = 0.0; 
			} 
		for (j = 0; j < 3; ++j) 
			B[j][j] = 1.0; 
		/* Compute B = A^n mod m using the binary decomposition of n */
		while (n > 0) { 
			if (n % 2) MatMatModM (W, B, B, m); 
			MatMatModM (W, W, W, m); 

			n/=2; 
		} 
	} 

	//-------------------------------------------------------------------- 
	// Check that the seeds are legitimate values. Returns 0 if legal 
	// seeds, -1 otherwise. 
	// 
	int CheckSeed (const unsigned long seed[6]) 
	{ 
		int i; 
		for (i = 0; i < 3; ++i) { 
			if (seed[i] >= m1) { 
				fprintf (stderr, "****************************************\n\n");
				fprintf (stderr, "ERROR: Seed[%d] >= 4294967087, Seed is not set.", i);
				fprintf (stderr, "\n\n****************************************\n\n");
				return (-1); 
			} 
		} 
		for (i = 3; i < 6; ++i) { 
			if (seed[i] >= m2) { 
				fprintf (stderr, "****************************************\n\n");
				fprintf (stderr, "ERROR: Seed[%d] >= 429444443, Seed is not set.", i);
				fprintf (stderr, "\n\n****************************************\n\n");
				return (-1); 
			} 
		} 
		if (seed[0] == 0 && seed[1] == 0 && seed[2] == 0) { 
			fprintf (stderr, "****************************************\n\n");
			fprintf (stderr, "ERROR: First 3 seeds = 0.\n\n");
			fprintf (stderr, "****************************************\n\n");
			return (-1); 
		} 
		if (seed[3] == 0 && seed[4] == 0 && seed[5] == 0) { 
			fprintf (stderr, "****************************************\n\n");
			fprintf (stderr, "ERROR: Last 3 seeds = 0.\n\n");
			fprintf (stderr, "****************************************\n\n");
			return (-1); 
		} 
		return 0; 
	} 
} // end of anonymous namespace 

//------------------------------------------------------------------------- 
// Generate the next random number. 
// 
double RNG::U01 () 
{ 
	long k; 
	double p1, p2, u; 
	/* Component 1 */ 
	p1 = a12 * Cg_[1] - a13n * Cg_[0]; 
	k = static_cast<long> (p1 / m1); 
	p1 -= k * m1; 
	if (p1 < 0.0) p1 += m1; 
	Cg_[0] = Cg_[1]; Cg_[1] = Cg_[2]; Cg_[2] = p1; 
	/* Component 2 */ 
	p2 = a21 * Cg_[5] - a23n * Cg_[3]; 
	k = static_cast<long> (p2 / m2); 
	p2 -= k * m2; 
	if (p2 < 0.0) p2 += m2; 
	Cg_[3] = Cg_[4]; Cg_[4] = Cg_[5]; Cg_[5] = p2; 
	/* Combination */ 
	u = ((p1 > p2) ? (p1 - p2) * norm : (p1 - p2 + m1) * norm); 
	return (anti_ == false) ? u : (1 - u); 
} 

//------------------------------------------------------------------------- 
// Generate the next random number with extended (53 bits) precision. 
// 
double RNG::U01d () 
{ 
	double u; 
	u = U01(); 
	if (anti_) { 
		// Don't forget that U01() returns 1 - u in 
		// the antithetic case 
		u += (U01() - 1.0) * fact; 
		return (u < 0.0) ? u + 1.0 : u; 
	} else { 
		u += U01() * fact; 
		return (u < 1.0) ? u : (u - 1.0); 
	} 
} 

//************************************************************************* 
// Public members of the class start here 
//------------------------------------------------------------------------- 

// The default seed of the package; will be the seed of the first 
// declared RNG, unless set_package_seed is called. 
// 
double RNG::next_seed_[6] = 
{ 
	12345.0, 12345.0, 12345.0, 12345.0, 12345.0, 12345.0 
}; 

//------------------------------------------------------------------------- 
// constructor 
// 
RNG::RNG (unsigned int runId)
{ 
	anti_ = false; 
	inc_prec_ = false; 

	/* Information on a stream. The arrays {Cg_, Bg_, Ig_} contain the
	   current state of the stream, the starting state of the current
	   SubStream, and the starting state of the stream. This stream
	   generates antithetic variates if anti_ = true. It also generates
	   numbers with extended precision (53 bits if machine follows IEEE
	   754 standard) if inc_prec_ = true. next_seed_ will be the seed of
	   the next declared RngStream. */

	for (int i = 0; i < 6; ++i) { 
		Bg_[i] = Cg_[i] = Ig_[i] = next_seed_[i]; 
	} 
	MatVecModM (A1p127, next_seed_, next_seed_, m1); 
	MatVecModM (A2p127, &next_seed_[3], &next_seed_[3], m2); 
	for ( unsigned int i = 0 ; i < runId ; i++ )
		reset_next_substream ();
}


//------------------------------------------------------------------------- 
// Reset Stream to beginning of Stream. 
// 
void RNG::reset_start_stream () 
{ 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i] = Ig_[i]; 
} 

//------------------------------------------------------------------------- 
// Reset Stream to beginning of SubStream. 
// 
void RNG::reset_start_substream () 
{ 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i]; 
} 

//------------------------------------------------------------------------- 
// Reset Stream to NextSubStream. 
// 
void RNG::reset_next_substream () 
{ 
	MatVecModM(A1p76, Bg_, Bg_, m1); 
	MatVecModM(A2p76, &Bg_[3], &Bg_[3], m2); 
	for (int i = 0; i < 6; ++i) 
		Cg_[i] = Bg_[i]; 
} 

//------------------------------------------------------------------------- 
void RNG::set_package_seed (const unsigned long seed[6]) 
{ 
	if (CheckSeed (seed)) 
		abort();
	for (int i = 0; i < 6; ++i) 
		next_seed_[i] = seed[i]; 
} 

//------------------------------------------------------------------------- 
// if e > 0, let n = 2^e + c; 
// if e < 0, let n = -2^(-e) + c; 
// if e = 0, let n = c. 
// Jump n steps forward if n > 0, backwards if n < 0. 
// 
void RNG::advance_state (long e, long c) 
{ 
	double B1[3][3], C1[3][3], B2[3][3], C2[3][3]; 
	if (e > 0) { 
		MatTwoPowModM (A1p0, B1, m1, e); 
		MatTwoPowModM (A2p0, B2, m2, e); 
	} else if (e < 0) { 
		MatTwoPowModM (InvA1, B1, m1, -e); 
		MatTwoPowModM (InvA2, B2, m2, -e); 
	} 
	if (c >= 0) { 
		MatPowModM (A1p0, C1, m1, c); 
		MatPowModM (A2p0, C2, m2, c); 
	} else { 
		MatPowModM (InvA1, C1, m1, -c); 
		MatPowModM (InvA2, C2, m2, -c); 
	} 
	if (e) { 
		MatMatModM (B1, C1, C1, m1); 
		MatMatModM (B2, C2, C2, m2); 
	} 
	MatVecModM (C1, Cg_, Cg_, m1); 
	MatVecModM (C2, &Cg_[3], &Cg_[3], m2); 
} 

//------------------------------------------------------------------------- 
void RNG::increased_precis (bool incp) 
{ 
	inc_prec_ = incp; 
} 

//------------------------------------------------------------------------- 
void RNG::set_antithetic (bool a) 
{ 
	anti_ = a; 
} 

//------------------------------------------------------------------------- 
// Generate the next random number. 
// 
double RNG::rand_u01 () 
{ 
	if (inc_prec_) 
		return U01d(); 
	else 
		return U01(); 
} 

//------------------------------------------------------------------------- 
// Generate the next random integer. 
// 
long RNG::rand_int (long low, long high) 
{ 
	//	return (long) low + (long) (((double) (high-low) * drn) + 0.5);
	return ((long) (low + (unsigned long) (((unsigned long) 
						(high-low+1)) * rand_u01())));
} 
