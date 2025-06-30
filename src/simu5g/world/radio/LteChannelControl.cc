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
#include <cassert>

#include <inet/common/INETMath.h>

#include "stack/phy/packet/AirFrame_m.h"
#include "world/radio/LteChannelControl.h"

namespace simu5g {

Define_Module(LteChannelControl);

using namespace omnetpp;

#define coreEV    EV << "LteChannelControl: "


/**
 * Calculates maxInterferenceDistance.
 *
 * @ref calcInterfDist
 */
void LteChannelControl::initialize()
{
    coreEV << "initializing LteChannelControl\n";
    ChannelControl::initialize();
}

/**
 * Calculation of the interference distance based on the transmitter
 * power, wavelength, pathloss coefficient and a threshold for the
 * minimal receive power
 *
 * You may want to overwrite this function in order to do your own
 * interference calculation
 *
 * TODO check interference model
 */
double LteChannelControl::calcInterfDist()
{
    double interfDistance;

    // The carrier frequency used
    double carrierFrequency = par("carrierFrequency");
    // Maximum transmission power possible
    double pMax = par("pMax");
    // Signal attenuation threshold
    double sat = par("sat");
    // Path loss coefficient
    double alpha = par("alpha");

    double waveLength = (SPEED_OF_LIGHT / carrierFrequency);
    // Minimum power level to be able to physically receive a signal
    double minReceivePower = pow(10.0, sat / 10.0);

    interfDistance = pow(waveLength * waveLength * pMax / (16.0 * M_PI * M_PI * minReceivePower), 1.0 / alpha);

    coreEV << "max interference distance:" << interfDistance << endl;

    return interfDistance;
}

void LteChannelControl::sendToChannel(RadioRef srcRadio, AirFrame *airFrame)
{
    // NOTE: no Enter_Method()! We pretend this method is part of ChannelAccess

    // Loop through all radios in range
    const RadioRefVector& neighbors = getNeighbors(srcRadio);
    for (auto r : neighbors) {
        coreEV << "sending message to radio\n";
        simtime_t delay = 0.0;
        check_and_cast<cSimpleModule *>(srcRadio->radioModule.get())->sendDirect(airFrame->dup(), delay, airFrame->getDuration(), r->radioInGate);
    }

    // The original frame can be deleted
    delete airFrame;
}

} //namespace

