/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010,2011,2012,2013 TELEMATICS LAB, Politecnico di Bari
 *
 * This file is part of LTE-Sim
 *
 * LTE-Sim is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation;
 *
 * LTE-Sim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LTE-Sim; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Francesco Capozzi <f.capozzi@poliba.it>
 */

#ifndef BLERVSSINR_15CQI_TU_H_
#define BLERVSSINR_15CQI_TU_H_

#include <iostream>

namespace simu5g {

static double BLER_15_CQI_TU[15][16] = {
    {
        1, 1, 0.996, 0.992, 0.968, 0.88, 0.76, 0.564, 0.364, 0.22, 0.084, 0.044, 0.008, 0, 0.004, 0,
    },
    {
        1, 1, 1, 0.996, 0.968, 0.928, 0.84, 0.68, 0.42, 0.284, 0.12, 0.076, 0.02, 0.008, 0, 0,
    },
    {
        1, 1, 1, 0.992, 0.956, 0.924, 0.752, 0.588, 0.436, 0.3, 0.156, 0.048, 0.032, 0, 0, 0,
    },
    {
        1, 1, 1, 0.988, 0.984, 0.9, 0.784, 0.596, 0.432, 0.248, 0.112, 0.072, 0.008, 0.02, 0, 0,
    },
    {
        1, 1, 0.988, 0.996, 0.924, 0.844, 0.724, 0.46, 0.356, 0.208, 0.076, 0.02, 0.008, 0, 0, 0,
    },
    {
        1, 1, 0.996, 0.984, 0.936, 0.868, 0.792, 0.66, 0.424, 0.304, 0.164, 0.064, 0.036, 0.016, 0.004, 0,
    },
    {
        1, 1, 1, 0.996, 0.98, 0.928, 0.836, 0.672, 0.544, 0.304, 0.156, 0.08, 0.036, 0.008, 0.004, 0,
    },
    {
        1, 1, 1, 0.988, 0.98, 0.936, 0.836, 0.676, 0.476, 0.344, 0.18, 0.088, 0.028, 0.012, 0, 0,
    },
    {
        1, 1, 1, 0.992, 0.968, 0.944, 0.832, 0.668, 0.58, 0.404, 0.288, 0.164, 0.084, 0.016, 0.012, 0,
    },
    {
        1, 1, 1, 0.992, 0.992, 0.96, 0.876, 0.728, 0.552, 0.376, 0.216, 0.132, 0.036, 0.024, 0.008, 0.0,
    },
    {
        1, 1, 1, 1, 0.992, 0.976, 0.916, 0.876, 0.716, 0.664, 0.396, 0.3, 0.16, 0.076, 0.06, 0.02,
    },
    {
        1, 1, 1, 1, 1, 0.996, 0.972, 0.908, 0.812, 0.696, 0.504, 0.312, 0.24, 0.18, 0.088, 0.008,
    },
    {
        1, 1, 1, 0.98, 0.952, 0.88, 0.804, 0.68, 0.516, 0.368, 0.188, 0.14, 0.068, 0.052, 0.028, 0.02,
    },
    {
        0.98, 0.968, 0.944, 0.912, 0.8, 0.736, 0.572, 0.42, 0.332, 0.26, 0.16, 0.144, 0.068, 0.036, 0.02, 0.004,
    },
    {
        0.964, 0.932, 0.848, 0.78, 0.668, 0.632, 0.488, 0.452, 0.372, 0.296, 0.164, 0.124, 0.072, 0.048, 0.032, 0.02,
    },

};

static double SINR_15_CQI_TU[15][16] = {
    {
        -14.5, -13.5, -12.5, -11.5, -10.5, -9.5, -8.5, -7.5, -6.5, -5.5, -4.5, -3.5, -2.5, -1.5, -0.5, 0.5,
    },
    {
        -13, -12, -11, -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2,
    },
    {
        -10.5, -9.5, -8.5, -7.5, -6.5, -5.5, -4.5, -3.5, -2.5, -1.5, -0.5, 0.5, 1.5, 2.5, 3.5, 4.5,
    },
    {
        -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7,
    },
    {
        -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    },
    {
        -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    },
    {
        -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    },
    {
        0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5,
    },
    {
        2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5,
    },
    {
        4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    },
    {
        6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    },
    {
        7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5, 19.5, 20.5, 21.5, 22.5,
    },
    {
        12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    },
    {
        17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
    },
    {
        23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    },

};

static double GetBLER_TU(double SINR, int MCS)
{
    int index = -1;
    double BLER = 0.0;
    int CQI = MCS;
    double R = 0.0;

    if (SINR <= SINR_15_CQI_TU[CQI - 1][0]) {
        BLER = 1.0;
    }
    else if (SINR >= SINR_15_CQI_TU[CQI - 1][15]) {
        BLER = 0.0;
    }
    else {
        for (int i = 0; i < 15; i++) {
            if (SINR >= SINR_15_CQI_TU[CQI - 1][i] && SINR < SINR_15_CQI_TU[CQI - 1][i + 1]) {
                index = i;
                //std::cout << SINR_15_CQI_TU [CQI-1] [i] << " - " << SINR_15_CQI_TU [CQI-1] [i+1] << std::endl;
            }
        }
    }

    if (index != -1) {
        R = (SINR - SINR_15_CQI_TU[CQI - 1][index]) / (SINR_15_CQI_TU[CQI - 1][index + 1] - SINR_15_CQI_TU[CQI - 1][index]);

        BLER = BLER_15_CQI_TU[CQI - 1][index] + R * (BLER_15_CQI_TU[CQI - 1][index + 1] - BLER_15_CQI_TU[CQI - 1][index]);
    }

    if (BLER >= 0.1) {
#ifdef BLER_DEBUG
        std::cout << "SINR " << SINR << " "
                  << "CQI " << CQI << " "
                  << "SINRprec " << SINR_15_CQI_TU[CQI - 1][index] << " "
                  << "SINRsucc " << SINR_15_CQI_TU[CQI - 1][index + 1] << " "
                  << "BLERprec " << BLER_15_CQI_TU[CQI - 1][index] << " "
                  << "BLERsucc " << BLER_15_CQI_TU[CQI - 1][index + 1] << " "
                  << "R " << R << " "
                  << "BLER " << BLER << " "
                  << std::endl;
#endif
    }

    return BLER;
}

} //namespace

#endif /* BLERVSSINR_15CQI_TU_H_ */

