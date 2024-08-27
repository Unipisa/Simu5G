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

#include "stack/phy/das/DasFilter.h"

namespace simu5g {

using namespace omnetpp;

DasFilter::DasFilter(LtePhyBase *ltePhy, Binder *binder,
        RemoteAntennaSet *ruSet, double rssiThreshold) : ruSet_(ruSet), rssiThreshold_(rssiThreshold), binder_(binder), ltePhy_(ltePhy)
{
}


void DasFilter::setMasterRuSet(MacNodeId masterId)
{
    cModule *module = getSimulation()->getModule(binder_->getOmnetId(masterId));
    if (getNodeTypeById(masterId) == ENODEB || getNodeTypeById(masterId) == GNODEB) {
        das_ = check_and_cast<LtePhyEnb *>(module->getSubmodule("cellularNic")->getSubmodule("phy"))->getDasFilter();
        ruSet_ = das_->getRemoteAntennaSet();
    }
    else {
        ruSet_ = nullptr;
    }

    // Clear structures used with old master on handover
    reportingSet_.clear();
}

double DasFilter::receiveBroadcast(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    EV << "DAS Filter: Received Broadcast\n";
    EV << "DAS Filter: ReportingSet now contains:\n";
    reportingSet_.clear();

    double rssiEnb = 0;
    for (unsigned int i = 0; i < ruSet_->getAntennaSetSize(); i++) {
        // equal bitrate mapping
        std::vector<double> rssiV;
        LteChannelModel *channelModel = ltePhy_->getChannelModel();
        if (channelModel == nullptr)
            throw cRuntimeError("DasFilter::receiveBroadcast - channel model is a null pointer. Abort.");
        else
            rssiV = channelModel->getSINR(frame, lteInfo);
        double rssi = 0;
        for (const auto& value : rssiV)
            rssi += value;
        rssi /= rssiV.size();
        //EV << "Sender Position: (" << senderPos.getX() << "," << senderPos.getY() << ")\n";
        //EV << "My Position: (" << myPos.getX() << "," << myPos.getY() << ")\n";

        EV << "RU" << i << " RSSI: " << rssi;
        if (rssi > rssiThreshold_) {
            EV << " is associated";
            reportingSet_.insert((Remote)i);
        }
        EV << "\n";
        if (i == 0)
            rssiEnb = rssi;
    }

    return rssiEnb;
}

RemoteSet DasFilter::getReportingSet()
{
    return reportingSet_;
}

RemoteAntennaSet *DasFilter::getRemoteAntennaSet() const
{
    return ruSet_;
}

double DasFilter::getAntennaTxPower(int i)
{
    return ruSet_->getAntennaTxPower(i);
}

inet::Coord DasFilter::getAntennaCoord(int i)
{
    return ruSet_->getAntennaCoord(i);
}

std::ostream& operator<<(std::ostream& stream, const DasFilter *das)
{
    stream << das->getRemoteAntennaSet() << endl;
    return stream;
}

} //namespace

