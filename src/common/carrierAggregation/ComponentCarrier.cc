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

#include "common/carrierAggregation/ComponentCarrier.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(ComponentCarrier);

void ComponentCarrier::initialize()
{
    binder_.reference(this, "binderModule", true);
    numBands_ = par("numBands");
    carrierFrequency_ = par("carrierFrequency");
    numerologyIndex_ = par("numerologyIndex");
    if (numerologyIndex_ > 4)
        throw cRuntimeError("ComponentCarrier::initialize - numerology index [%d] not valid. It must be in the range between 0-4.", numerologyIndex_);

    useTdd_ = par("useTdd").boolValue();
    if (useTdd_) {
        tddNumSymbolsDl_ = par("tddNumSymbolsDl");
        tddNumSymbolsUl_ = par("tddNumSymbolsUl");
    }

    // Register the carrier to the binder
    binder_->registerCarrier(carrierFrequency_, numBands_, numerologyIndex_, useTdd_, tddNumSymbolsDl_, tddNumSymbolsUl_);
}

} //namespace

