// -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2005,2006,2007 INRIA
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
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
// Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
//

namespace simu5g;

enum ModulationClass {
    // Modulation class unknown or unspecified. A WifiMode with this WifiModulationClass has not been properly initialized.
    MOD_CLASS_UNKNOWN = 0;
    // Infrared (IR) (Clause 16)
    MOD_CLASS_IR = 1;
    // Frequency-hopping spread spectrum (FHSS) PHY (Clause 14)
    MOD_CLASS_FHSS = 2;
    // DSSS PHY (Clause 15) and HR/DSSS PHY (Clause 18)
    MOD_CLASS_DSSS = 3;
    // ERP-PBCC PHY (19.6)
    MOD_CLASS_ERP_PBCC = 4;
    // DSSS-OFDM PHY (19.7)
    MOD_CLASS_DSSS_OFDM = 5;
    // ERP-OFDM PHY (19.5)
    MOD_CLASS_ERP_OFDM = 6;
    // OFDM PHY (Clause 17)
    MOD_CLASS_OFDM = 7;
    // HT PHY (Clause 20)
    MOD_CLASS_HT = 8;
};

//
// This enumeration defines the various convolutional coding rates
// used for the OFDM transmission modes in the IEEE 802.11
// standard. DSSS (for example) rates which do not have an explicit
// coding stage in their generation should have this parameter set to
// WIFI_CODE_RATE_UNDEFINED.
//
enum CodeRate {
    // No explicit coding (e.g., DSSS rates)
    CODE_RATE_UNDEFINED = 0;
    // Rate 3/4
    CODE_RATE_3_4 = 1;
    // Rate 2/3
    CODE_RATE_2_3 = 2;
    // Rate 1/2
    CODE_RATE_1_2 = 3;
    // Rate 5/6
    CODE_RATE_5_6 = 4;
};

//
// \brief Represents a single transmission mode
//
// A WifiMode is implemented by a single integer which is used
// to lookup in a global array the characteristics of the
// associated transmission mode. It is thus extremely cheap to
// keep a WifiMode variable around.
//
class ModulationType
{
    bool isMandatory = false @getter(getIsMandatory);
    uint32_t bandwidth = 0;
    uint32_t dataRate = 0;
    /// MANDATORY it is necessary to set the dataRate before the codeRate
    CodeRate codeRate = CODE_RATE_UNDEFINED;
    uint32_t phyRate = 0;
    uint8_t constellationSize = 0;
    ModulationClass modulationClass = MOD_CLASS_UNKNOWN;
    uint32_t frequency = 2400;
}

cplusplus(ModulationType) {{
  public:
    bool operator==(const ModulationType& b) const
    {
        return this->isMandatory == b.isMandatory &&
               this->bandwidth == b.bandwidth &&
               this->codeRate == b.codeRate &&
               this->dataRate == b.dataRate &&
               this->phyRate == b.phyRate &&
               this->constellationSize == b.constellationSize &&
               this->modulationClass == b.modulationClass &&
               this->frequency == b.frequency;
    }
}}

cplusplus(ModulationType::setCodeRate) {{
    switch (codeRate) {
        case CODE_RATE_5_6:
            phyRate = dataRate * 6 / 5;
            break;
        case CODE_RATE_3_4:
            phyRate = dataRate * 4 / 3;
            break;
        case CODE_RATE_2_3:
            phyRate = dataRate * 3 / 2;
            break;
        case CODE_RATE_1_2:
            phyRate = dataRate * 2 / 1;
            break;
        case CODE_RATE_UNDEFINED:
        default:
            phyRate = dataRate;
            break;
    }
}}

