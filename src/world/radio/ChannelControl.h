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

#ifndef CHANNELCONTROL_H
#define CHANNELCONTROL_H

#include <vector>
#include <list>
#include <set>

#include <inet/common/INETDefs.h>
#include <inet/common/geometry/common/Coord.h>

#include "world/radio/IChannelControl.h"

namespace simu5g {

using namespace omnetpp;

// Forward declarations
class AirFrame;

#define TRANSMISSION_PURGE_INTERVAL    1.0

/**
 * Keeps track of radios/NICs, their positions and channels;
 * also caches neighbor info (which other Radios are within
 * interference distance).
 */
struct IChannelControl::RadioEntry {
    opp_component_ptr<cModule> radioModule;  // the module that registered this radio interface
    cGate *radioInGate = nullptr;  // gate on host module used to receive airframes
    int channel;
    inet::Coord pos; // cached radio position

    struct Compare {
        bool operator()(const RadioRef& lhs, const RadioRef& rhs) const {
            ASSERT(lhs != nullptr);
            ASSERT(rhs != nullptr);
            return lhs->radioModule->getId() < rhs->radioModule->getId();
        }
    };
    // we cache neighbors set in a std::vector, because std::set iteration is slow;
    // std::vector is created and updated on demand
    std::set<RadioRef, Compare> neighbors; // cached neighbor list
    std::vector<RadioRef> neighborList;
    bool isNeighborListValid;
    bool isActive;
};

/**
 * Monitors which radios are "in range". Supports multiple channels.
 *
 * @ingroup channelControl
 * @see ChannelAccess
 */
class ChannelControl : public cSimpleModule, public IChannelControl
{
  protected:
    typedef std::list<RadioEntry> RadioList;
    typedef std::vector<RadioRef> RadioRefVector;

    RadioList radios;

    /** keeps track of ongoing transmissions; this is needed when a radio
     * switches to another channel (then it needs to know whether the target channel
     * is empty or busy)
     */
    typedef std::vector<TransmissionList> ChannelTransmissionLists;
    ChannelTransmissionLists transmissions; // indexed by channel number (size=numChannels)

    /** used to clear the transmission list from time to time */
    simtime_t lastOngoingTransmissionsUpdate;

    friend std::ostream& operator<<(std::ostream&, const RadioEntry&);
    friend std::ostream& operator<<(std::ostream&, const TransmissionList&);

    /** Set debugging for the basic module*/
    bool coreDebug;

    /** the maximum interference distance in the network.*/
    double maxInterferenceDistance;

    /** the number of controlled channels */
    int numChannels;

  protected:
    virtual void updateConnections(RadioRef h);

    /** Calculate interference distance*/
    virtual double calcInterfDist();

    /** Reads init parameters and calculates a maximum interference distance*/
    void initialize() override;

    /** Throws away expired transmissions. */
    virtual void purgeOngoingTransmissions();

    /** Validate the channel identifier */
    virtual void checkChannel(int channel);

    /** Get the list of modules in range of the given host */
    virtual const RadioRefVector& getNeighbors(RadioRef h);

    /** Notifies the channel control with an ongoing transmission */
    virtual void addOngoingTransmission(RadioRef h, AirFrame *frame);

    /** Returns the "handle" of a previously registered radio. The pointer to the registering (radio) module must be provided */
    virtual RadioRef lookupRadio(cModule *radioModule);

  public:
    ~ChannelControl() override;

    /** Registers the given radio. If radioInGate==NULL, the "radioIn" gate is assumed */
    RadioRef registerRadio(cModule *radioModule, cGate *radioInGate = nullptr) override;

    /** Unregisters the given radio */
    void unregisterRadio(RadioRef r) override;

    /** Returns the host module that contains the given radio */
    cModule *getRadioModule(RadioRef r) const override { return r->radioModule; }

    /** Returns the input gate of the host for receiving AirFrames */
    cGate *getRadioGate(RadioRef r) const override { return r->radioInGate; }

    /** Returns the channel the given radio listens on */
    int getRadioChannel(RadioRef r) const override { return r->channel; }

    /** To be called when the host moved; updates proximity info */
    void setRadioPosition(RadioRef r, const inet::Coord& pos) override;

    /** Called when host switches channel */
    void setRadioChannel(RadioRef r, int channel) override;

    /** Returns the number of radio channels (frequencies) simulated */
    int getNumChannels() override { return numChannels; }

    /** Provides a list of transmissions currently on the air */
    const TransmissionList& getOngoingTransmissions(int channel) override;

    /** Called from ChannelAccess, to transmit a frame to the radios in range, on the frame's channel */
    void sendToChannel(RadioRef srcRadio, AirFrame *airFrame) override;

    /** Returns the maximum interference distance*/
    double getInterferenceRange(RadioRef r) override { return maxInterferenceDistance; }

    /** Disable the reception in the reference module */
    void disableReception(RadioRef r) override { r->isActive = false; };

    /** Enable the reception in the reference module */
    void enableReception(RadioRef r) override { r->isActive = true; };

    /** Returns propagation speed of the signal in meters/sec */
    double getPropagationSpeed() override { return SPEED_OF_LIGHT; }
};

} //namespace

#endif

