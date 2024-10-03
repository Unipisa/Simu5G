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

#ifndef CHANNEL_ACCESS_H
#define CHANNEL_ACCESS_H

#include <list>
#include <limits>

#include <omnetpp.h>
#include <inet/common/INETDefs.h>

#include "world/radio/IChannelControl.h"
#include "common/features.h"

namespace simu5g {

using namespace omnetpp;

// Forward declarations
class AirFrame;

//
// Base class for the PHY layer
//
class ChannelAccess : public cSimpleModule, public cListener
{
  protected:
    opp_component_ptr<IChannelControl> cc = nullptr;  // Pointer to the ChannelControl module
    IChannelControl::RadioRef myRadioRef = nullptr;  // Identifies this radio in the ChannelControl module
    opp_component_ptr<cModule> hostModule;    // the host that contains this radio model
    inet::Coord radioPos;  // the physical position of the radio (derived from display string or from mobility models)
    bool positionUpdateArrived;

  public:
    ~ChannelAccess() override;

    /**
     * @brief Called by the signaling mechanism to inform of changes.
     *
     * ChannelAccess is subscribed to position changes.
     */
    void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *) override;

    /** Finds the channelControl module in the network */
    IChannelControl *getChannelControl();

    /**
     * @brief Called when a mobilityStateChanged signal is received.
     *
     * Makes the PHY layer emit statistics related to the serving cell
     */
    virtual void emitMobilityStats() {}

  protected:
    /** Sends a message to all radios in range */
    virtual void sendToChannel(AirFrame *msg);

    virtual cPar& getChannelControlPar(const char *parName) { return check_and_cast<cModule *>(cc.get())->par(parName); }
    const inet::Coord& getRadioPosition() const { return radioPos; }
    cModule *getHostModule() const { return hostModule; }

    /** Register with ChannelControl and subscribe to hostPos*/
    void initialize(int stage) override;
    int numInitStages() const override { return inet::INITSTAGE_PHYSICAL_LAYER + 1; }
};

} //namespace

#endif

