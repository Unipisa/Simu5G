//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/radio/CellularReceiver.h"

#include "simu5g/stack/phy/radio/BlerCurveErrorModel.h"

namespace simu5g {

Define_Module(CellularReceiver);

void CellularReceiver::initialize()
{
    noiseFigure_ = par("noiseFigure");
    cableLoss_ = par("cableLoss");
}

BlerCurveErrorModel *CellularReceiver::getErrorModel() const
{
    return check_and_cast<BlerCurveErrorModel *>(getSubmodule("errorModel"));
}

bool CellularReceiver::decide(double packetErrorRate)
{
    Enter_Method("decide");
    double randomSample = uniform(0.0, 1.0);
    bool received = randomSample > packetErrorRate;
    EV << "CellularReceiver: packet error rate " << packetErrorRate << ", random sample " << randomSample
       << " -> " << (received ? "received" : "lost") << endl;
    return received;
}

} // namespace simu5g
