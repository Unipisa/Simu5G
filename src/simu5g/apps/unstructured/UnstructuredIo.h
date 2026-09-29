//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _UNSTRUCTUREDIO_H_
#define _UNSTRUCTUREDIO_H_

#include <inet/common/packet/Packet.h>

namespace simu5g {

/**
 * The attachment of an application to the cellular NIC of a UE whose session is of
 * type Unstructured, with no transport or network layer in between. See
 * UnstructuredIo.ned.
 */
class UnstructuredIo : public omnetpp::cSimpleModule
{
  protected:
    int interfaceId_ = -1;   // the cellular NIC's interface id, resolved on first use
    int numSent_ = 0;
    int numReceived_ = 0;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
    void finish() override;

    // The interface id of the cellular NIC the payload goes through
    virtual int getInterfaceId();
};

} //namespace

#endif
