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

#include "nodes/mec/MEPlatform/MEWarningAlertService/MEWarningAlertService.h"
#include "inet/common/packet/Packet_m.h"

Define_Module(MEWarningAlertService);

void MEWarningAlertService::initialize(int stage)
{
    EV << "MEWarningAlertService::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    // setting the MEWarningAlertService gate sizes
    int maxMEApps = 0;
    cModule* mePlatform = getParentModule();
    if(mePlatform != NULL){
        cModule* meHost = mePlatform->getParentModule();
        if(meHost->hasPar("maxMEApps"))
            maxMEApps = meHost->par("maxMEApps");
        else
            throw cRuntimeError("MEWarningAlertService::initialize - \tFATAL! Error when getting meHost.maxMEApps parameter!");

        this->setGateSize("meAppOut", maxMEApps);
        this->setGateSize("meAppIn", maxMEApps);
    }
    else{
        EV << "MEWarningAlertService::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("MEWarningAlertService::initialize - \tFATAL! Error when getting getParentModule()");
    }

    //retrieving parameters
    dangerEdgeA = inet::Coord(par("dangerEdgeAx").doubleValue(), par("dangerEdgeAy").doubleValue(), par("dangerEdgeAz").doubleValue());
    dangerEdgeB = inet::Coord(par("dangerEdgeBx").doubleValue(), par("dangerEdgeBy").doubleValue(), par("dangerEdgeBz").doubleValue());
    dangerEdgeC = inet::Coord(par("dangerEdgeCx").doubleValue(), par("dangerEdgeCy").doubleValue(), par("dangerEdgeCz").doubleValue());
    dangerEdgeD = inet::Coord(par("dangerEdgeDx").doubleValue(), par("dangerEdgeDy").doubleValue(), par("dangerEdgeDz").doubleValue());

    //drawing the Danger Area
    cPolygonFigure *polygon = new cPolygonFigure("polygon");
    std::vector<cFigure::Point> points;
    points.push_back(cFigure::Point(dangerEdgeA.x, dangerEdgeA.y));
    points.push_back(cFigure::Point(dangerEdgeB.x, dangerEdgeB.y));
    points.push_back(cFigure::Point(dangerEdgeC.x, dangerEdgeC.y));
    points.push_back(cFigure::Point(dangerEdgeD.x, dangerEdgeD.y));
    polygon->setPoints(points);
    polygon->setLineColor(cFigure::RED);
    polygon->setLineWidth(2);
    getSimulation()->getSystemModule()->getCanvas()->addFigure(polygon);
}

void MEWarningAlertService::handleMessage(cMessage *msg)
{
    EV << "MEWarningAlertService::handleMessage - \n";

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto alert = packet->peekAtFront<WarningAlertPacket>();
    if(!strcmp(alert->getType(), INFO_UEAPP))
        handleInfoUEWarningAlertApp(msg);

    delete msg;
}

void MEWarningAlertService::handleInfoUEWarningAlertApp(cMessage* msg){

    inet::Packet* packet = check_and_cast<inet::Packet*>(msg);
    auto alert = packet->peekAtFront<WarningAlertPacket>();

    EV << "MEWarningAlertService::handleInfoUEWarningAlertApp - Received " << alert->getType() << " type WarningAlertPacket from " << alert->getSourceAddress() << endl;

    inet::Coord uePosition(alert->getPositionX(), alert->getPositionY(), alert->getPositionZ());

    inet::Packet* respPacket = new inet::Packet("WarningAlertPacketInfo");
    auto respAlert = inet::makeShared<WarningAlertPacket>();
    respAlert->setType(INFO_MEAPP);

    if(isInQuadrilateral(uePosition, dangerEdgeA, dangerEdgeB, dangerEdgeC, dangerEdgeD)){

        respAlert->setDanger(true);
        respPacket->insertAtBack(alert);
        send(respPacket, "meAppOut", msg->getArrivalGate()->getIndex());

        EV << "MEWarningAlertService::handleInfoUEWarningAlertApp - "<< respAlert->getSourceAddress() << " is in Danger Area! Sending the " << INFO_MEAPP << " type WarningAlertPacket with danger == TRUE!" << endl;
    }
    else{

        respAlert->setDanger(false);
        respPacket->insertAtBack(alert);
        send(respPacket, "meAppOut", msg->getArrivalGate()->getIndex());

        EV << "MEWarningAlertService::handleInfoUEWarningAlertApp - "<< respAlert->getSourceAddress() << " is not in Danger Area! Sending the " << INFO_MEAPP << " type WarningAlertPacket with danger == FALSE!" << endl;
    }
}

bool MEWarningAlertService::isInTriangle(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C){

      //considering all points relative to A
      inet::Coord v0 = B-A;   // B w.r.t A
      inet::Coord v1 = C-A;   // C w.r.t A
      inet::Coord v2 = P-A;   // P w.r.t A

      // writing v2 = u*v0 + v*v1 (linear combination in the Plane defined by vectors v0 and v1)
      // and finding u and v (scalar)

      double den = ((v0*v0) * (v1*v1)) - ((v0*v1) * (v1*v0));

      double u = ( ((v1*v1) * (v2*v0)) - ((v1*v0) * (v2*v1)) ) / den;
      double v = ( ((v0*v0) * (v2*v1)) - ((v0*v1) * (v2*v0)) ) / den;

      // checking if coefficients u and v are constrained in [0-1], that means inside the triangle ABC
      if(u>=0 && v>=0 && u+v<=1)
      {
          //EV << "inside!";
          return true;
      }
      else{
          //EV << "outside!";
          return false;
      }
}

bool MEWarningAlertService::isInQuadrilateral(inet::Coord P, inet::Coord A, inet::Coord B, inet::Coord C, inet::Coord D)
{
      return isInTriangle(P, A, B, C) || isInTriangle(P, A, C, D);
}
