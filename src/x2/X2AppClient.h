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

#ifndef __X2APPCLIENT_H_
#define __X2APPCLIENT_H_

#include <inet/applications/sctpapp/SctpClient.h>
#include "common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class SctpAssociation;

/**
 * Implements the X2AppClient simple module. See the NED file for more information.
 */
class X2AppClient : public inet::SctpClient
{
    // reference to the gates
    cGate *x2ManagerOut_ = nullptr;

  protected:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void socketEstablished(inet::SctpSocket *socket, unsigned long int buffer) override;
    void socketDataArrived(inet::SctpSocket *socket, inet::Packet *msg, bool urgent) override;
};

} //namespace

#endif

