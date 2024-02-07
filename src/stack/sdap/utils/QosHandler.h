//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __SIMU5G_QOSHANDLER_H_
#define __SIMU5G_QOSHANDLER_H_

#include <omnetpp.h>
#include "common/LteCommonEnum_m.h"
#include "common/LteCommon.h"
#include "common/NrCommon.h"
#include "common/LteControlInfo_m.h"
#include "common/binder/GlobalData.h"
using namespace omnetpp;

/**
 * TODO - Generated class
 */
struct QosInfo {
    QosInfo(){};
    QosInfo(Direction dir) {
        this->dir = dir;
    };
    MacNodeId destNodeId;
    MacNodeId senderNodeId;
    inet::Ipv4Address senderAddress;
    inet::Ipv4Address destAddress;
    ApplicationType appType;
    LteTrafficClass trafficClass;
    unsigned short rlcType;
    unsigned short lcid = 0;
    unsigned short qfi = 0;
    //unsigned short _5Qi = 0;
    unsigned short radioBearerId = 0;
    unsigned int cid = 0;
    bool containsSeveralCids = false;
    Direction dir;
};
class QosHandler : public cSimpleModule
{
public:
	QosHandler() {
	}

	virtual ~QosHandler() {
	}

	virtual RanNodeType getNodeType() {
		Enter_Method_Silent("getNodeType");
		return nodeType;
	}


	virtual std::map<MacCid, QosInfo>& getQosInfo() {
		Enter_Method_Silent("getQosInfo");
		return QosInfos;
	}

	virtual void insertQosInfo(MacCid cid, QosInfo info){
		Enter_Method_Silent("insertQosInfo");
		QosInfos[cid] = info;
	}

    virtual unsigned short getLcid(unsigned int nodeId, unsigned short msgCat) {
    	Enter_Method_Silent("getLcid");
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId && var.second.appType == msgCat) {
                return var.second.lcid;
            }
        }
        return 0;
    }

    virtual void deleteNode(unsigned int nodeId) {
    	Enter_Method_Silent("deleteNode");
        std::vector<unsigned int> tmp;
        for (auto const & var : QosInfos) {
            if (var.second.destNodeId == nodeId
                    || var.second.senderNodeId == nodeId) {
                tmp.push_back(var.first);
            }
        }

        for (auto const & var : tmp) {
            QosInfos.erase(var);
        }
    }

    virtual std::vector<QosInfo> getAllQosInfos(unsigned int nodeId) {
		Enter_Method_Silent("getAllQosInfos");
		std::vector<QosInfo> tmp;
		for (auto const &var : QosInfos) {
			if (var.second.destNodeId == nodeId || var.second.senderNodeId == nodeId) {
				if (var.second.lcid != 0)
					tmp.push_back(var.second);
			}
		}
		return tmp;
	}

    virtual int numInitStages() const {
        return 2;
    }


	//sorts the qosInfos by its lcid value (from smallest to highest)
	virtual std::vector<std::pair<MacCid, QosInfo>> getSortedQosInfos(Direction dir) {
		Enter_Method_Silent("getSortedQosInfos");
		std::vector<std::pair<unsigned int, QosInfo>> tmp;
		for (auto var : QosInfos) {
			if (var.second.lcid != 0 && var.second.lcid != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return a.second.lcid > b.second.lcid;
		});
		return tmp;
	}

	//sort by priority from 23.501
	virtual std::vector<std::pair<MacCid, QosInfo>> getPrioritySortedQosInfos(Direction dir) {
		Enter_Method_Silent("getPrioritySortedQosInfos");
		std::vector<std::pair<MacCid, QosInfo>> tmp;
		for (auto &var : QosInfos) {
			if (MacCidToLcid(var.first) != 0 && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return (getPriority(get5Qi(a.second.qfi)) > getPriority(get5Qi(b.second.qfi)));
		});
		return tmp;
	}

	//sort by pdb from 23.501
	virtual std::vector<std::pair<MacCid, QosInfo>> getPdbSortedQosInfos(Direction dir) {
		Enter_Method_Silent("getPdbSortedQosInfos");
		std::vector<std::pair<MacCid, QosInfo>> tmp;
		for (auto &var : QosInfos) {
			if (MacCidToLcid(var.first) != 0 && MacCidToLcid(var.first) != 10 && var.second.dir == dir)
				tmp.push_back(var);
		}
		std::sort(tmp.begin(), tmp.end(), [&](std::pair<unsigned int, QosInfo> &a, std::pair<unsigned int, QosInfo> &b) {
			return (getPdb(get5Qi(a.second.qfi)) > getPdb(get5Qi(b.second.qfi)));
		});
		return tmp;
	}


	//prio --> priority, pdb, weight
	//
	virtual std::map<double, std::vector<QosInfo>> getEqualPriorityMap(Direction dir) {
		Enter_Method_Silent("getEqualPriorityMap");

		double lambdaPriority = getSimulation()->getSystemModule()->par("lambdaPriority").doubleValue();
		double lambdaRemainDelayBudget = getSimulation()->getSystemModule()->par("lambdaRemainDelayBudget").doubleValue();
		double lambdaCqi = getSimulation()->getSystemModule()->par("lambdaCqi").doubleValue();
		double lambdaByteSize = getSimulation()->getSystemModule()->par("lambdaByteSize").doubleValue();
		double lambdaRtx = getSimulation()->getSystemModule()->par("lambdaRtx").doubleValue();

		std::string prio = "default";
		if (getSimulation()->getSystemModule()->hasPar("qosModelPriority")) {
			prio = getSimulation()->getSystemModule()->par("qosModelPriority").stdstringValue();

			if(lambdaPriority == 1 && lambdaByteSize == 0 && lambdaCqi == 0 && lambdaRemainDelayBudget == 0 && lambdaRtx == 0){
				prio = "priority";
			}else if(lambdaPriority == 0 && lambdaByteSize == 0 && lambdaCqi == 0 && lambdaRemainDelayBudget == 1 && lambdaRtx == 0){
				prio = "pdb";
			}else{
				prio = "default";
			}
		}

		//first item is the (calculated prio), second item is the cid
		std::map<double, std::vector<QosInfo>> retValue;

		if (prio == "priority") {
			//use priority from table in 23.501, qos characteristics

			//get the priorityMap
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPrioritySortedQosInfos(dir);

			for (auto &var : tmp) {
				double prio = getPriority(get5Qi(getQfi(var.first)));
				retValue[prio].push_back(var.second);
			}

			return retValue;

		} else if (prio == "pdb") {
			//use packet delay budget from table in 23.501, a shorter pdb gets a higher priority

			//get the priorityMap
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPdbSortedQosInfos(dir);

			//find out the pdb

			for (auto &var : tmp) {
				double prio = getPdb(get5Qi(getQfi(var.first)));
				retValue[prio].push_back(var.second);
			}

			return retValue;

		} else if (prio == "weight") {
			//calculate a weight by priority * pdb * per (values from 23.501)

			//get the priorityMap
			//consider the pdb value as above --> ensure that the most urgent one gets highest prio
			std::vector<std::pair<MacCid, QosInfo>> tmp = getPrioritySortedQosInfos(dir);

			for (auto &var : tmp) {
				double prio = getPriority(get5Qi(getQfi(var.first)));
				double pdb = getPdb(get5Qi(getQfi(var.first)));
				//double per = getPer(getQfi(var.first));
				double weight = prio * pdb;

				retValue[weight].push_back(var.second);
			}

			return retValue;

		} else if (prio == "default") {
			//by lcid
			std::vector<std::pair<MacCid, QosInfo>> tmp = getSortedQosInfos(dir);

			for (auto &var : tmp) {
				double lcid = var.second.lcid;
				retValue[lcid].push_back(var.second);
			}
			return retValue;

		} else {
			throw cRuntimeError("Error - QosHandler::getEqualPriorityMap - unknown priority");
		}

	}

	virtual void clearQosInfos(){
        QosInfos.clear();
    }

    virtual void modifyControlInfo(LteControlInfo * info)
    {

    	Enter_Method_Silent("modifyControlInfo");
        info->setApplication(0);
        MacCid newMacCid = idToMacCid(MacCidToNodeId(info->getCid()),0);
        info->setCid(newMacCid);
        info->setLcid(0);
        info->setQfi(0);
        info->setTraffic(0);
        info->setContainsSeveralCids(true);
        QosInfo tmp((Direction)info->getDirection());
        tmp.appType = (ApplicationType)0;
        tmp.cid = newMacCid;
        tmp.containsSeveralCids = true;
        tmp.destNodeId = info->getDestId();
        tmp.senderNodeId = info->getSourceId();
        tmp.lcid = 0;
        tmp.trafficClass = (LteTrafficClass)0;
        QosInfos[newMacCid] = tmp;

    }

    virtual ApplicationType getApplicationType(MacCid cid) {
    	Enter_Method_Silent("getApplicationType");
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return (ApplicationType) var.second.appType;
        }
        return (ApplicationType)0;
    }


    virtual unsigned short getQfi(MacCid cid) {
    	Enter_Method_Silent("getQfi");
        for (auto & var : QosInfos) {
            if (var.first == cid)
                return var.second.qfi;
        }
        return 0;
    }



    //type is the application type V2X, VOD, VOIP, DATA_FLOW
    //returns the QFI which is pre-configured in QosHandler.ned file
    virtual unsigned short getQfi(ApplicationType type){
    	Enter_Method_Silent("getQfi");
    	auto qfi = 1;
        switch(type){
        case VOD:
            return videoQfi;
        case VOIP:
            return voipQfi;
        case CBR:
            qfi = dataQfi;
            return dataQfi;
        case NETWORK_CONTROL:
            return networkControlQfi;
        default:
            qfi = dataQfi;
        	return dataQfi;
        }
    }
    virtual unsigned short getFlowQfi(std::string pktName){
        Enter_Method_Silent("getQfi value");
        auto qfi = 1;
        if ((pktName.find("VoIP") == 0) || (pktName.find("audio") == 0)){
            return voipQfi;
        }
        else if ((pktName.find("video") == 0) || (pktName.find("VOD") == 0)){
            return videoQfi;
        }
        else if (pktName.find("gaming") == 0){
            return dataQfi;
        }
        else if ((pktName.find("network control") == 0) || (pktName.find("networkControl") == 0)){
            return networkControlQfi;
        }
        else return qfi;

    }

	virtual unsigned short get5Qi(unsigned short qfi) {

		Enter_Method_Silent("get5Qi");
		if (qfi == v2xQfi) {
			return v2x5Qi;
		} else if (qfi == videoQfi) {
			return video5Qi;
		} else if (qfi == voipQfi) {
			return voip5Qi;
		} else if (qfi == dataQfi) {
			return data5Qi;
		}
		else if (qfi == networkControlQfi){
		    return networkControl5Qi;
		}
		else {
			return data5Qi;
		}
	}

    virtual unsigned short getRadioBearerId(ApplicationType type){
    	Enter_Method_Silent("getRadioBearerId");
    	switch(type){
    	        case VOD:
    	            return videoQfiToRadioBearer;
    	        case VOIP:
    	            return voipQfiToRadioBearer;
    	        case CBR:
    	            return dataQfiToRadioBearer;
    	        case NETWORK_CONTROL:
    	            return networkControlQfiToRadioBearer;
    	        case GAMING:
    	            return gamingQfiToRadioBearer;
    	        default:
    	            return dataQfiToRadioBearer;
    	        }

    }

	virtual void initQfiParams() {
		Enter_Method_Silent("initQfiParams");
		this->v2xQfi = par("v2xQfi");
		this->videoQfi = par("videoQfi");
		this->voipQfi = par("voipQfi");
		this->dataQfi = par("dataQfi");
		this->networkControlQfi = par("networkControlQfi");

		this->v2x5Qi = par("v2x5Qi");
		this->video5Qi = par("video5Qi");
		this->voip5Qi = par("voip5Qi");
		this->data5Qi = par("data5Qi");
		this->networkControl5Qi = par("networkControl5Qi");

		this->gamingQfiToRadioBearer = par("gamingQfiToRadioBearer");
		this->videoQfiToRadioBearer = par("videoQfiToRadioBearer");
		this->voipQfiToRadioBearer = par("voipQfiToRadioBearer");
		this->dataQfiToRadioBearer = par("dataQfiToRadioBearer");
		this->networkControlQfiToRadioBearer = par("networkControlQfiToRadioBearer");

	}


    virtual double getCharacteristic(std::string characteristic, unsigned short _5Qi) {
    	Enter_Method_Silent("getCharacteristic");
		QosCharacteristic qosCharacteristics = NRQosCharacteristics::getNRQosCharacteristics()->getQosCharacteristic(_5Qi);

		if (characteristic == "PDB") {
			return qosCharacteristics.getPdb();
		} else if (characteristic == "PER") {
			return qosCharacteristics.getPer();
		} else if (characteristic == "PRIO") {
			return qosCharacteristics.getPriorityLevel();
		} else {
			throw cRuntimeError("Unknown QoS characteristic");
		}
	}


    virtual unsigned short getPriority(unsigned short _5Qi){
    	Enter_Method_Silent("getPriority");
    	return getCharacteristic("PRIO", _5Qi);
    }

    virtual double getPdb(unsigned short _5Qi){
    	Enter_Method_Silent("getPdb");
    	return getCharacteristic("PDB", _5Qi);
    }

    virtual double getPer(unsigned short _5Qi){
    	Enter_Method_Silent("getPer");
    	return getCharacteristic("PER", _5Qi);
    }

    virtual int getQfiFromPcp(std::string pktName){
        auto mappingTable = globalData->getQosMappingTable();
        auto found = -1;
        for (auto& item: mappingTable){
            found = pktName.find(item.first);
            if (found != std::string::npos){
                return std::stoi(item.second.pcp);
              }
        }
        EV<<"Packet not configured!!";
        return -1;
    }

protected:
    RanNodeType nodeType;
    std::map<MacCid, QosInfo> QosInfos;

    unsigned short v2xQfi;
    unsigned short videoQfi;
    unsigned short voipQfi;
    unsigned short dataQfi;
    unsigned short networkControlQfi;

    unsigned short v2x5Qi;
    unsigned short video5Qi;
    unsigned short voip5Qi;
    unsigned short data5Qi;
    unsigned short networkControl5Qi;

    unsigned short gamingQfiToRadioBearer;
    unsigned short videoQfiToRadioBearer;
    unsigned short voipQfiToRadioBearer;
    unsigned short dataQfiToRadioBearer;
    unsigned short networkControlQfiToRadioBearer;

    GlobalData *globalData;
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};
class QosHandlerUE: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

class QosHandlerGNB: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

class QosHandlerUPF: public QosHandler {

protected:
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

#endif
