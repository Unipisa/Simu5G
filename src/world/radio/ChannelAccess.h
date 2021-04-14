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

// Forward declarations
class AirFrame;

//
// Base class for the PHY layer
//
class ChannelAccess : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
    IChannelControl* cc;  // Pointer to the ChannelControl module
    IChannelControl::RadioRef myRadioRef;  // Identifies this radio in the ChannelControl module
    cModule *hostModule;    // the host that contains this radio model
    inet::Coord radioPos;  // the physical position of the radio (derived from display string or from mobility models)
    bool positionUpdateArrived;

  public:
    ChannelAccess() : cc(nullptr), myRadioRef(nullptr), hostModule(nullptr) {}
    virtual ~ChannelAccess();

    /**
     * @brief Called by the signalling mechanism to inform of changes.
     *
     * ChannelAccess is subscribed to position changes.
     */
    virtual void receiveSignal(omnetpp::cComponent *source, omnetpp::simsignal_t signalID, omnetpp::cObject *obj, omnetpp::cObject *) override;

    /** Finds the channelControl module in the network */
    IChannelControl *getChannelControl();

  protected:
    /** Sends a message to all radios in range */
    virtual void sendToChannel(AirFrame *msg);

    virtual omnetpp::cPar& getChannelControlPar(const char *parName) { return dynamic_cast<omnetpp::cModule *>(cc)->par(parName); }
    const inet::Coord& getRadioPosition() const { return radioPos; }
    omnetpp::cModule *getHostModule() const { return hostModule; }

    /** Register with ChannelControl and subscribe to hostPos*/
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::INITSTAGE_PHYSICAL_LAYER + 1; }
};

#endif
