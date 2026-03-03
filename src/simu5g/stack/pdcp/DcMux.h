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

#ifndef _PDCP_DC_MUX_H_
#define _PDCP_DC_MUX_H_

#include <map>
#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/pdcp/PdcpTxEntityBase.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class LowerMux;

class DcMux : public cSimpleModule
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    MacNodeId nodeId_;

    LowerMux *lowerMux_ = nullptr;

    cGate *dcManagerInGate_ = nullptr;

    typedef std::map<DrbKey, PdcpTxEntityBase *> PdcpBypassTxEntities;
    PdcpBypassTxEntities bypassTxEntities_;

  public:
    PdcpTxEntityBase *lookupBypassTxEntity(DrbKey id);
    void registerBypassTxEntity(DrbKey id, PdcpTxEntityBase *txEnt);
    void unregisterBypassTxEntity(DrbKey id);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
