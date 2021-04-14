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


#ifndef __MEWARNINGALERTSERVICE_H_
#define __MEWARNINGALERTSERVICE_H_

//WarningAlertPacket
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"

#include "inet/common/geometry/common/Coord.h"

using namespace omnetpp;

/*
 * see MEWarningAlertService.ned
 */
class MEWarningAlertService : public cSimpleModule
{
    inet::Coord dangerEdgeA, dangerEdgeB, dangerEdgeC, dangerEdgeD;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        void handleInfoUEWarningAlertApp(cMessage* pkt);

        //utilities to check if the ue is within the danger area
        bool isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C);
        bool isInQuadrilateral(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D);
};

#endif
