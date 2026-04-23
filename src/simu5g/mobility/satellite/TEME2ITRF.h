//
// Copyright (C) 2021 Mario Franke <research@m-franke.net>
//
// Documentation for these modules is at http://sat.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <cmath>
#include "simu5g/mobility/satellite/constants.h"

namespace simu5g {

/*
Helper function to use the same modulo operator as Python for floating point numbers.
*/
double pythonFmod(double a, double b) {
    return a - (b * std::floor(a/b));
}

std::pair<double, double> theta_GMST1982(double jd_ut1, double fraction_ut1=0.0)
{
    /* Return the angle of Greenwich Mean Standard Time 1982 given the JD.

    This angle defines the difference between the idiosyncratic True
    Equator Mean Equinox (TEME) frame of reference used by SGP4 and the
    more standard Pseudo Earth Fixed (PEF) frame of reference.  The UT1
    time should be provided as a Julian date.  Theta is returned in
    radians, and its velocity in radians per day of UT1 time.

    From AIAA 2006-6753 Appendix C.

    inspired from skyfield/sgp4lib.py

    */
    double t = (jd_ut1 - T0 + fraction_ut1) / 36525.0;
    double g = 67310.54841 + (8640184.812866 + (0.093104 + (-6.2e-6) * t) * t) * t;
    double dg = 8640184.812866 + (0.093104 * 2.0 + (-6.2e-6 * 3.0) * t) * t;
    double theta = pythonFmod((pythonFmod(jd_ut1, 1.0) + fraction_ut1 + pythonFmod(g / DAY_S, 1.0)), 1.0) * TAU;
    double theta_dot = (1.0 + dg / (DAY_S * 36525.0)) * TAU;
    return std::make_pair(theta, theta_dot);

}

std::vector<double> multiply_outer(const std::vector<double> &a, const double b)
{
    /* implementation of numpy.multiply.outer
     * see https://numpy.org/doc/stable/reference/generated/numpy.ufunc.outer.html#numpy.ufunc.outer
     * see https://numpy.org/doc/stable/reference/generated/numpy.outer.html
     * especially the Examples section
     */
    assert(a.size() > 0);
    std::vector<double> r(a.size(), 0);
    for (std::vector<double>::size_type i = 0; i < a.size(); i++) {
        r[i] = a[i] * b;
    }
    return r;
}

/* see skyfield/functions.py */
std::vector<std::vector<double>> rot_x(double theta)
{
    double c = cos(theta);
    double s = sin(theta);
    std::vector<std::vector<double>> r{std::vector<double>{1.0, 0.0, 0.0},
                                       std::vector<double>{0.0, c, -1.0 * s},
                                       std::vector<double>{0.0, s, c}};
    return r;
}

/* see skyfield/functions.py */
std::vector<std::vector<double>> rot_y(double theta)
{
    double c = cos(theta);
    double s = sin(theta);
    std::vector<std::vector<double>> r{std::vector<double>{c, 0.0, s},
                                       std::vector<double>{0.0, 1.0, 0.0},
                                       std::vector<double>{-1.0 * s, 0.0, c}};
    return r;
}

/* see skyfield/functions.py */
std::vector<std::vector<double>> rot_z(double theta)
{
    double c = cos(theta);
    double s = sin(theta);
    std::vector<std::vector<double>> r{std::vector<double>{c, -s, 0.0},
                                       std::vector<double>{s, c, 0.0},
                                       std::vector<double>{0.0, 0.0, 1.0}};
    return r;
}

/* computes the scalar/dot product of two vectors */
double dot(const std::vector<double> &a, const std::vector<double> &b)
{
    assert(a.size() == b.size());
    double r = 0;
    for (std::vector<double>::size_type i = 0; i < a.size(); i++) {
        r += a[i] * b[i];
    }
    return r;
}

/* 
 * computes a matrix multiplication a x b
 * Skyfield uses numpy.dot for matrix multiplication although ```matmul``` or ```@``` is preferred
 * see https://numpy.org/doc/stable/reference/generated/numpy.dot.html
 * see skyfield/sgp4lib.py
 */
std::vector<std::vector<double>> dot(const std::vector<std::vector<double>> &a, const std::vector<std::vector<double>> &b)
{
    assert(a[0].size() == b.size());
    std::vector<std::vector<double>> r;
    for (std::vector<double>::size_type i = 0; i < a.size(); i++) {
        std::vector<double> tmp(0, b[0].size());
        r.push_back(tmp);
    }
    for (std::vector<double>::size_type i = 0; i < a.size(); i++) {             // iterate over all rows
        for (std::vector<double>::size_type j = 0; j < b[0].size(); j++) {      // iterate over all columns
            for (std::vector<double>::size_type k = 0; k < a.size(); k++) {
                r[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return r;
}

/* 
 * Matrix times vector: multiply a NxN matrix by a vector 
 * see https://numpy.org/doc/stable/reference/generated/numpy.einsum.html
 * einsum('ij...,j...->i...', M, v)
 * see skyfield/functions.py
 */
std::vector<double> mxv(const std::vector<std::vector<double>> &m, const std::vector<double> &v)
{
    assert(m[0].size() == v.size());
    std::vector<double> r(v.size(), 0);
    for (std::vector<double>::size_type row = 0; row < m.size(); row++) {
        for (std::vector<double>::size_type col = 0; col < v.size(); col++) {
            r[row] += m[row][col] * v[col];
        }
    }
    return r;
}

std::vector<double> v_plus_v(const std::vector<double> &v1, const std::vector<double> &v2)
{
    assert(v1.size() == v2.size());
    std::vector<double> r(v1.size(), 0);
    for (std::vector<double>::size_type row = 0; row < v1.size(); row++) {
            r[row] = v1[row] + v2[row];
    }
    return r;
}


/*
 * implements _cross(a, b) of skyfield/sgp4lib.py
 * I do not understand the python expression
 * my version computers the cross product
 */
std::vector<double> _cross(const std::vector<double> &a, const std::vector<double> &b)
{
    assert(a.size() == b.size());
    std::vector<double> r(a.size(), 0);
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
    return r;
}

std::pair<std::vector<double>, std::vector<double>> TEME_to_ITRF(double jd_ut1, std::vector<double> rTEME, std::vector<double> vTEME, double xp=0.0, double yp=0.0, double fraction_ut1=0.0)
{
    /* Copied from skyfield/sgp4lib.py
     *
     * Deprecated: use the TEME and ITRS frame objects instead.
     */

    std::vector<double> _zero_zero_minus_one{0.0, 0.0, -1.0};
    std::pair<double, double> theta = theta_GMST1982(jd_ut1, fraction_ut1);
    double theta_dot = theta.second;
    std::vector<double> angular_velocity = multiply_outer(_zero_zero_minus_one, theta_dot);

    std::vector<std::vector<double>> R = rot_z(-1.0 * theta.first);

    std::vector<double> rPEF = mxv(R, rTEME);
    std::vector<double> c_product = _cross(angular_velocity, rPEF);
    std::vector<double> vPEF = mxv(R, vTEME);
    vPEF = v_plus_v(vPEF, _cross(angular_velocity, rPEF));

    std::vector<double> rITRF;
    std::vector<double> vITRF;
    if (xp == 0.0 && yp == 0.0) {
        rITRF = rPEF;
        vITRF = vPEF;
    }else{
        std::vector<std::vector<double>> W = dot(rot_x(yp), rot_y(xp));
        rITRF = mxv(W, rPEF);
        vITRF = mxv(W, vPEF);
    }
    return std::make_pair(rITRF, vITRF);
}

} // end namespace
