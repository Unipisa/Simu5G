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

#include "simu5g/stack/NtnFronthaulNic.h"

#include <cmath>

#include "simu5g/stack/phy/packet/LteAirFrame_m.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnFronthaulNic);

void NtnFronthaulNic::initialize()
{
    feederLinkFrequencyOffset_ = GHz(par("feederLinkFrequencyOffset"));
    shiftFrequencyFromFeederLink_ = par("shiftFrequencyFromFeederLink").boolValue();
}

void NtnFronthaulNic::shiftFrequencyBandToServiceLink(cMessage *msg) const
{
    auto *frame = dynamic_cast<LteAirFrame *>(msg);
    if (frame == nullptr)
        return;

    UserControlInfo lteInfo(frame->getAdditionalInfo());
    GHz carrierFrequency = lteInfo.getCarrierFrequency();
    if (std::isnan(carrierFrequency.get()))
        return;

    // Feeder-link frames use the shifted Ka carrier, but the terrestrial gNB/eNB stack
    // remains indexed by the service-link S-band carrier.
    double shiftedFrequency = carrierFrequency.get() - feederLinkFrequencyOffset_.get();
    if (shiftedFrequency <= 0.0)
        throw cRuntimeError("NtnFronthaulNic::shiftFrequencyBandToServiceLink - cannot translate feeder-link carrier %gGHz with offset %gGHz",
                carrierFrequency.get(), feederLinkFrequencyOffset_.get());

    lteInfo.setCarrierFrequency(GHz(shiftedFrequency));
    frame->setAdditionalInfo(lteInfo);
}

void NtnFronthaulNic::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();
    if (arrivalGate->isName("upperLayerIn")) {
        if (shiftFrequencyFromFeederLink_)
            shiftFrequencyBandToServiceLink(msg);
        EV << "NtnFronthaulNic::handleMessage - forwarding packet " << msg->getName() << " to fronthaul link" << endl;
        send(msg, "phys$o");
    }
    else if (arrivalGate->isName("phys$i")) {
        EV << "NtnFronthaulNic::handleMessage - forwarding packet " << msg->getName() << " to upper layer" << endl;
        send(msg, "upperLayerOut");
    }
    else
        throw cRuntimeError("NtnFronthaulNic::handleMessage - unexpected gate %s", arrivalGate->getFullName());
}

} // namespace simu5g
