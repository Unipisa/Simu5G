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

#include "world/radio/ChannelAccess.h"
#include <inet/mobility/contract/IMobility.h>
#include <inet/common/ModuleAccess.h>

using namespace omnetpp;

static int parseInt(const char *s, int defaultValue)
{
    if (!s || !*s)
        return defaultValue;

    char *endptr;
    int value = strtol(s, &endptr, 10);
    return *endptr == '\0' ? value : defaultValue;
}

// the destructor unregister the radio module
ChannelAccess::~ChannelAccess()
{
    if (cc && myRadioRef)
    {
        // check if channel control exist
        IChannelControl *cc = dynamic_cast<IChannelControl *>(getSimulation()->getModuleByPath("channelControl"));
        if (cc)
             cc->unregisterRadio(myRadioRef);
        myRadioRef = nullptr;
    }
}
/**
 * Upon initialization ChannelAccess registers the nic parent module
 * to have all its connections handled by ChannelControl
 */
void ChannelAccess::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        cc = getChannelControl();
        hostModule = inet::findContainingNode(this);
        myRadioRef = nullptr;

        positionUpdateArrived = false;

        // subscribe to the correct mobility module

        if (hostModule->findSubmodule("mobility") != -1)
        {
            // register to get a notification when position changes
            hostModule->subscribe(inet::IMobility::mobilityStateChangedSignal, this);
        }
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        if (!positionUpdateArrived && hostModule->isSubscribed(inet::IMobility::mobilityStateChangedSignal, this))
        {
            // ...else, get the initial position from the display string
            radioPos.x = parseInt(hostModule->getDisplayString().getTagArg("p", 0), -1);
            radioPos.y = parseInt(hostModule->getDisplayString().getTagArg("p", 1), -1);

            if (radioPos.x == -1 || radioPos.y == -1)
                error("The coordinates of '%s' host are invalid. Please set coordinates in "
                        "'@display' attribute, or configure Mobility for this host.",
                        hostModule->getFullPath().c_str());

            const char *s = hostModule->getDisplayString().getTagArg("p", 2);
            if (s && *s)
                error("The coordinates of '%s' host are invalid. Please remove automatic arrangement"
                        " (3rd argument of 'p' tag)"
                        " from '@display' attribute, or configure Mobility for this host.",
                        hostModule->getFullPath().c_str());
        }
        myRadioRef = cc->registerRadio(this);
        cc->setRadioPosition(myRadioRef, radioPos);
    }
}

IChannelControl *ChannelAccess::getChannelControl()
{
    IChannelControl *cc = dynamic_cast<IChannelControl *>(getSimulation()->getModuleByPath("channelControl"));
    if (!cc)
        throw cRuntimeError("Could not find ChannelControl module with name 'channelControl' in the toplevel network.");
    return cc;
}

/**
 * This function has to be called whenever a packet is supposed to be
 * sent to the channel.
 *
 * This function really sends the message away, so if you still want
 * to work with it you should send a duplicate!
 */
void ChannelAccess::sendToChannel(AirFrame *msg)
{
    EV << "sendToChannel: sending to gates\n";

    // delegate it to ChannelControl
    cc->sendToChannel(myRadioRef, msg);
}

void ChannelAccess::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *)
{
    if (signalID == inet::IMobility::mobilityStateChangedSignal)
    {
        inet::IMobility *mobility = check_and_cast<inet::IMobility*>(obj);
        radioPos = mobility->getCurrentPosition();
        positionUpdateArrived = true;

        if (myRadioRef)
            cc->setRadioPosition(myRadioRef, radioPos);
    }
}
