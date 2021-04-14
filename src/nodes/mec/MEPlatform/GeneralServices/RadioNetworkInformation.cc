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

#include "nodes/mec/MEPlatform/GeneralServices/RadioNetworkInformation.h"
#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/mac/layer/LteMacEnbD2D.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/phy/layer/LtePhyBase.h"

Define_Module(RadioNetworkInformation);

void RadioNetworkInformation::initialize(int stage)
{
    EV << "RadioNetworkInformation::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage!=inet::INITSTAGE_APPLICATION_LAYER)
        return;

    mePlatform = getParentModule();
    if(mePlatform != NULL){
        meHost = mePlatform->getParentModule();
    }
    else{
        EV << "RadioNetworkInformation::initialize - ERROR getting mePlatform cModule!" << endl;
        throw cRuntimeError("RadioNetworkInformation::initialize - \tFATAL! Error when getting getParentModule()");
    }

    //getting the list of eNB connected from parameter
    if(meHost->hasPar("eNBList") && strcmp(meHost->par("eNBList").stringValue(), "")){

        char* token = strtok ( (char*) meHost->par("eNBList").stringValue(), " ");            // split by spaces

        while (token != NULL)
        {
            EV <<"RadioNetworkInformation::initialize - eNB (from par): "<< token << endl;

            enb.push_back(token);
            token = strtok (NULL, " ");
        }
    }
    //getting the list of eNB connected from gate connections
    else
    {
        int nENB = meHost->gateSize("pppENB");

        for(int i=0; i<nENB; i++){
            char* eNBName = (char*) meHost->gate("pppENB$o", i)->getPathEndGate()->getOwnerModule()->getParentModule()->getParentModule()->getFullName();
            enb.push_back(eNBName);
            EV <<"RadioNetworkInformation::initialize - eNB (from gate): "<< eNBName << endl;
        }
    }

    binder_ = getBinder();
}

void RadioNetworkInformation::handleMessage(cMessage *msg)
{
}

double RadioNetworkInformation::getUETxPower(std::string ue){

    cModule* module = getSimulation()->getModuleByPath(ue.c_str());

    if(module != NULL)
    {
        int carOmnetID = module->getId();

        std::vector<UeInfo*>::const_iterator it = binder_->getUeList()->begin();
        std::vector<UeInfo*>::const_iterator et = binder_->getUeList()->end();

        for (;it!=et;++it)
            if((*it)->ue->getId() == carOmnetID)
                return (*it)->phy->getTxPwr(D2D);
    }
    return -1;
}

Cqi RadioNetworkInformation::getUEcqi(std::string ue){

    cModule* module = getSimulation()->getModuleByPath(ue.c_str());

    if(module != NULL)
    {
        int carOmnetID = module->getId();

        std::vector<UeInfo*>::const_iterator it = binder_->getUeList()->begin();
        std::vector<UeInfo*>::const_iterator et = binder_->getUeList()->end();

        for (;it!=et;++it)
        {
            if((*it)->ue->getId() == carOmnetID)
            {
                cModule* enb =  getSimulation()->getModule(getBinder()->getOmnetId((*it)->cellId));
                if(enb != NULL)
                {
                    LteMacBase* mac = check_and_cast<LteMacBase*> (enb->getSubmodule("cellularNic")->getSubmodule("mac"));
                    if(mac->isD2DCapable())
                    {
                        LteMacEnbD2D* macD2D = check_and_cast<LteMacEnbD2D*> (mac);
                        CellInfo* cellInfo = check_and_cast<CellInfo*> (enb->getSubmodule("cellInfo"));
                        double carrierFrequency = cellInfo->getPrimaryCarrierFrequency(); // FIXME select the appropriate carrier

                        std::vector<Cqi> cqiVector = macD2D->getAmc()->computeTxParams((*it)->id, D2D, carrierFrequency).readCqiVector();
                        if(!cqiVector.empty()){

                            EV << "RadioNetworkInformation::getUEcqi - cqi: " << ue << ": " << cqiVector.at(0) << endl;
                            return cqiVector.at(0);
                        }
                        else{
                            //maybe it is not necessary (pre-configured parameters are automatically loaded!)
                            EV << "RadioNetworkInformation::getUEcqi - pre-configured cqi: " << ue << ": " << macD2D->getPreconfiguredTxParams()->readCqiVector().at(0) << endl;
                            return macD2D->getPreconfiguredTxParams()->readCqiVector().at(0);
                        }
                    }
                }
            }
        }
    }

    return -1;
}
