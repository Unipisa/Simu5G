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

simsignal_t CellularReceiver::rcvdSinrDlSignal_ = registerSignal("rcvdSinrDl");
simsignal_t CellularReceiver::rcvdSinrUlSignal_ = registerSignal("rcvdSinrUl");
simsignal_t CellularReceiver::measuredSinrDlSignal_ = registerSignal("measuredSinrDl");
simsignal_t CellularReceiver::measuredSinrUlSignal_ = registerSignal("measuredSinrUl");

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

void CellularReceiver::setCarrierFrequencies(const std::vector<GHz>& carrierFrequencies)
{
    Enter_Method_Silent();
    ASSERT(carrierSignals_.empty());
    if (carrierFrequencies.size() < 2)
        return;
    for (GHz carrierFrequency : carrierFrequencies) {
        CarrierSignals& signals = carrierSignals_[carrierFrequency];
        signals.rcvdSinrDl = addCarrierStatistic("rcvdSinrDl", carrierFrequency);
        signals.rcvdSinrUl = addCarrierStatistic("rcvdSinrUl", carrierFrequency);
        signals.measuredSinrDl = addCarrierStatistic("measuredSinrDl", carrierFrequency);
        signals.measuredSinrUl = addCarrierStatistic("measuredSinrUl", carrierFrequency);
    }
}

simsignal_t CellularReceiver::addCarrierStatistic(const char *statisticName, GHz carrierFrequency)
{
    // in MHz, so that the usual frequencies (3.5GHz) put no dot in the name; a
    // dot would split the name where the ini file addresses the statistic
    std::string name = opp_stringf("%s-%gMHz", statisticName, inet::units::values::MHz(carrierFrequency).get());
    std::replace(name.begin(), name.end(), '.', '_');
    cProperty *statisticTemplate = getProperties()->get("statisticTemplate", statisticName);
    if (statisticTemplate == nullptr)
        throw cRuntimeError("No @statisticTemplate[%s] on %s", statisticName, getNedTypeName());
    simsignal_t signal = registerSignal(name.c_str());
    getEnvir()->addResultRecorders(this, signal, name.c_str(), statisticTemplate);
    return signal;
}

const CellularReceiver::CarrierSignals *CellularReceiver::findCarrierSignals(GHz carrierFrequency) const
{
    if (carrierSignals_.empty())
        return nullptr;
    auto it = carrierSignals_.find(carrierFrequency);
    if (it == carrierSignals_.end())
        throw cRuntimeError("%s: no statistics for carrier %g GHz, which the radio does not serve", getFullPath().c_str(), carrierFrequency.get());
    return &it->second;
}

void CellularReceiver::emitOnCarrier(simsignal_t signal, double value)
{
    // The recorders of a per-carrier statistic are attached to its signal
    // directly (cEnvir::addResultRecorders() with a signal), and OMNeT++ puts
    // no warm-up filter in front of those, as it does for a @statistic's. The
    // warm-up period is therefore honoured here, as that filter would.
    if (simTime() >= getSimulation()->getWarmupPeriod())
        emit(signal, value);
}

void CellularReceiver::emitRcvdSinr(Direction dir, GHz carrierFrequency, double sinr)
{
    Enter_Method_Silent();
    ASSERT(dir == DL || dir == UL);
    emit(dir == DL ? rcvdSinrDlSignal_ : rcvdSinrUlSignal_, sinr);
    if (const CarrierSignals *signals = findCarrierSignals(carrierFrequency))
        emitOnCarrier(dir == DL ? signals->rcvdSinrDl : signals->rcvdSinrUl, sinr);
}

void CellularReceiver::emitMeasuredSinr(Direction dir, GHz carrierFrequency, double sinr)
{
    Enter_Method_Silent();
    ASSERT(dir == DL || dir == UL);
    emit(dir == DL ? measuredSinrDlSignal_ : measuredSinrUlSignal_, sinr);
    if (const CarrierSignals *signals = findCarrierSignals(carrierFrequency))
        emitOnCarrier(dir == DL ? signals->measuredSinrDl : signals->measuredSinrUl, sinr);
}

} // namespace simu5g
