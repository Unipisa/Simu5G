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

#ifndef ICHANNELCONTROL_H
#define ICHANNELCONTROL_H

#include <vector>
#include <list>
#include <set>

#include <inet/common/INETDefs.h>
#include <inet/common/geometry/common/Coord.h>
#include "stack/phy/packet/AirFrame_m.h"

namespace simu5g {

using namespace omnetpp;

// Forward declarations
class AirFrame;

/**
 * Interface to implement for a module that controls radio frequency channel access.
 */
class IChannelControl
{
  protected:
    struct RadioEntry;

  public:
    typedef RadioEntry *RadioRef; // handle for ChannelControl's clients
    typedef std::list<AirFrame *> TransmissionList;

  public:

    /** Registers the given radio. If radioInGate==nullptr, the "radioIn" gate is assumed */
    virtual RadioRef registerRadio(cModule *radioModule, cGate *radioInGate = nullptr) = 0;

    /** Unregisters the given radio */
    virtual void unregisterRadio(RadioRef r) = 0;

    /** Returns the host module that contains the given radio */
    virtual cModule *getRadioModule(RadioRef r) const = 0;

    /** Returns the input gate of the host for receiving AirFrames */
    virtual cGate *getRadioGate(RadioRef r) const = 0;

    /** Returns the channel the given radio listens on */
    virtual int getRadioChannel(RadioRef r) const = 0;

    /** To be called when the host moved; updates proximity info */
    virtual void setRadioPosition(RadioRef r, const inet::Coord& pos) = 0;

    /** Called when host switches channel */
    virtual void setRadioChannel(RadioRef r, int channel) = 0;

    /** Returns the number of radio channels (frequencies) simulated */
    virtual int getNumChannels() = 0;

    /** Provides a list of transmissions currently on the air */
    virtual const TransmissionList& getOngoingTransmissions(int channel) = 0;

    /** Called from ChannelAccess, to transmit a frame to the radios in range, on the frame's channel */
    virtual void sendToChannel(RadioRef srcRadio, AirFrame *airFrame) = 0;

    /** Returns the maximal interference distance */
    virtual double getInterferenceRange(RadioRef r) = 0;

    /** Disable the reception in the reference module */
    virtual void disableReception(RadioRef r) = 0;

    /** Enable the reception in the reference module */
    virtual void enableReception(RadioRef r) = 0;

    /** Returns propagation speed of the signal in meters/sec */
    virtual double getPropagationSpeed() = 0;
};

} //namespace

#endif

