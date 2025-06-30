//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef COMPONENTCARRIER_H_
#define COMPONENTCARRIER_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class Binder;

/*
 * This module acts as a descriptor for one Component Carrier
 */
class ComponentCarrier : public cSimpleModule
{
  protected:
    // Reference to Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // Carrier Frequency
    double carrierFrequency_;

    // Number of bands for this carrier
    unsigned int numBands_;

    // Numerology used for this carrier
    unsigned int numerologyIndex_;

    // Index of the slot format for TDD (-1 stands for FDD)
//    int tddSlotFormatIndex_;

    bool useTdd_;
    unsigned int tddNumSymbolsDl_;
    unsigned int tddNumSymbolsUl_;

  public:

    void initialize() override;

    /*
     * Returns the carrier frequency
     */
    double getCarrierFrequency() { return carrierFrequency_; }

    /*
     * Returns the number of logical bands
     */
    unsigned int getNumBands() { return numBands_; }

    /*
     * Returns the numerology index
     */
    unsigned int getNumerologyIndex() { return numerologyIndex_; }

    bool isTddEnabled() { return useTdd_; }
    unsigned int getTddNumSymbolsDl() { return tddNumSymbolsDl_; }
    unsigned int getTddNumSymbolsUl() { return tddNumSymbolsUl_; }

};

} //namespace

#endif

