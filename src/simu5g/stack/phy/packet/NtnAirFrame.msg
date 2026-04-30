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

import simu5g.stack.phy.packet.LteAirFrame;

cplusplus {{
#include <vector>
}}

namespace simu5g;

//
// Air frame used by transparent NTN paths. It carries the SINR computed on the
// first radio hop so the final receiver can evaluate end-to-end reception.
//
packet NtnAirFrame extends LteAirFrame
{
    double relayHopSinr[];
    bool gatewayReceptionResultValid = false;
    bool gatewayReceptionResult = false;
}

cplusplus(NtnAirFrame) {{
  public:
    bool hasRelayHopSinr() const { return getRelayHopSinrArraySize() > 0; }

    void clearRelayHopSinr() { setRelayHopSinrArraySize(0); }

    void setRelayHopSinrVector(const std::vector<double>& sinr)
    {
        setRelayHopSinrArraySize(sinr.size());
        for (size_t i = 0; i < sinr.size(); i++)
            setRelayHopSinr(i, sinr[i]);
    }

    std::vector<double> getRelayHopSinrVector() const
    {
        std::vector<double> sinr(getRelayHopSinrArraySize());
        for (size_t i = 0; i < sinr.size(); i++)
            sinr[i] = getRelayHopSinr(i);
        return sinr;
    }

    bool hasGatewayReceptionResult() const { return getGatewayReceptionResultValid(); }

    void setGatewayReceptionResultInfo(bool result)
    {
        setGatewayReceptionResult(result);
        setGatewayReceptionResultValid(true);
    }

    void clearGatewayReceptionResult()
    {
        setGatewayReceptionResult(false);
        setGatewayReceptionResultValid(false);
    }
}}
