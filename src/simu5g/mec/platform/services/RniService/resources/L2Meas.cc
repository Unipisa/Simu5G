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

#include "simu5g/mec/platform/services/RniService/resources/L2Meas.h"
#include "simu5g/corenetwork/statsCollector/UeStatsCollector.h"
#include "simu5g/corenetwork/statsCollector/BaseStationStatsCollector.h"
#include "simu5g/mec/platform/services/RniService/resources/RniCellInfo.h"
#include "simu5g/common/binder/Binder.h"
#include "CellUeInfo.h"

namespace simu5g {

using namespace omnetpp;

L2Meas::L2Meas() : binder_(nullptr) {
}

L2Meas::L2Meas(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs) {
    for (auto *module : eNodeBs) {
        BaseStationStatsCollector *collector = check_and_cast<BaseStationStatsCollector *>(module->getSubmodule("collector"));
        eNodeBs_.insert({collector->getCellId(), collector});
    }
}

void L2Meas::addEnodeB(std::set<cModule *, simu5g::utils::cModule_LessId>& eNodeBs) {
    for (cModule* module : eNodeBs) {
        BaseStationStatsCollector *collector = check_and_cast<BaseStationStatsCollector *>(module->getSubmodule("collector"));
        eNodeBs_.insert({collector->getCellId(), collector});
    }
}

void L2Meas::addEnodeB(cModule *eNodeB) {
    BaseStationStatsCollector *collector = check_and_cast<BaseStationStatsCollector *>(eNodeB->getSubmodule("collector"));
    eNodeBs_.insert({collector->getCellId(), collector});
}


nlohmann::ordered_json L2Meas::toJson() const {
    nlohmann::ordered_json val;
    nlohmann::ordered_json l2Meas;
    nlohmann::ordered_json cellArray;
    nlohmann::ordered_json ueArray;

    if (timestamp_.isValid()) {
        val["timestamp"] = timestamp_.toJson();
    }

    for (const auto &[macCellId, baseStationStatsCollector] : eNodeBs_) {
        UeStatsCollectorMap *ueMap = baseStationStatsCollector->getCollectorMap();
        for (const auto &[ueId, ueStatsCollector] : *ueMap) {
            CellUeInfo cellUeInfo = CellUeInfo(ueStatsCollector, baseStationStatsCollector->getEcgi());
            ueArray.push_back(cellUeInfo.toJson());
        }
        RniCellInfo cellInfo = RniCellInfo(baseStationStatsCollector);
        cellArray.push_back(cellInfo.toJson());
    }

    if (cellArray.size() > 1) {
        val["cellInfo"] = cellArray;
    }
    else if (cellArray.size() == 1) {
        val["cellInfo"] = cellArray[0];
    }

    if (ueArray.size() > 1) {
        val["cellUEInfo"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["cellUEInfo"] = ueArray[0];
    }

    return val;
}

//
nlohmann::ordered_json L2Meas::toJsonUe(std::vector<inet::Ipv4Address>& uesID) const {
    nlohmann::ordered_json val;
    nlohmann::ordered_json l2Meas;
    nlohmann::ordered_json ueArray;

    if (timestamp_.isValid()) {
        val["timestamp"] = timestamp_.toJson();
    }

    std::map<MacCellId, BaseStationStatsCollector *>::const_iterator eit;
    bool found = false;
    ASSERT(binder_ != nullptr);
    for (auto ipAddress: uesID) {

        /*
         * An UE can be connected to both eNB and gNB (at the same time)
         * I decided to report both structures.
         *
         *
         */
        MacNodeId lteNodeId = binder_->getMacNodeId(ipAddress);
        MacNodeId nrNodeId = binder_->getNrMacNodeId(ipAddress);
        std::vector<MacNodeId> nodeIds;

        // TODO REORGANIZE CODE!!

        if (lteNodeId == NODEID_NONE && nrNodeId == NODEID_NONE) {
            std::string notFound = "Address: " + ipAddress.str() + " Not found.";
            ueArray.push_back(notFound);
            break;
        }
        else if (lteNodeId != NODEID_NONE && lteNodeId == nrNodeId) { //only nr
            found = false;
            for (const auto& [cellId, baseStation]: eNodeBs_) {
                if (baseStation->getCellNodeType() == ENODEB) // enodeb does not have nrNodeId
                    continue;
                if (baseStation->hasUeCollector(nrNodeId)) {
                    UeStatsCollector *ueColl = baseStation->getUeCollector(nrNodeId);
                    CellUeInfo cellUeInfo = CellUeInfo(ueColl, baseStation->getEcgi());
                    ueArray.push_back(cellUeInfo.toJson());
                    found = true;
                    break; // next ue id
                }
            }
            if (!found) {
                // ETSI sandbox does not return anything in case the ip is not valid
                std::string notFound = "Address: " + ipAddress.str() + " Not found.";
                ueArray.push_back(notFound);
            }
        }
        else if (lteNodeId != NODEID_NONE && nrNodeId == NODEID_NONE) { //only lte
            found = false;
            for (const auto& [cellId, baseStation]: eNodeBs_) {
                if (baseStation->getCellNodeType() == GNODEB) // gnodeb does not have lteNodeId
                    continue;
                if (baseStation->hasUeCollector(lteNodeId)) {
                    UeStatsCollector *ueColl = baseStation->getUeCollector(lteNodeId);
                    CellUeInfo cellUeInfo = CellUeInfo(ueColl, baseStation->getEcgi());
                    ueArray.push_back(cellUeInfo.toJson());
                    found = true;
                    break; // next ue id
                }
            }
            if (!found) {
                std::string notFound = "Address: " + ipAddress.str() + " Not found.";
                ueArray.push_back(notFound);
            }
        }
        else if (lteNodeId != nrNodeId && nrNodeId != NODEID_NONE && lteNodeId != NODEID_NONE) { // both lte and nr
            found = false;
            for (const auto& [cellId, baseStation]: eNodeBs_) {
                if (baseStation->hasUeCollector(lteNodeId)) {
                    UeStatsCollector *ueColl = baseStation->getUeCollector(lteNodeId);
                    CellUeInfo cellUeInfo = CellUeInfo(ueColl, baseStation->getEcgi());
                    ueArray.push_back(cellUeInfo.toJson());
                    found = true;
                    break; // next ue id
                }
            }

            found = false;
            for (const auto& [cellId, baseStation]: eNodeBs_) {
                if (baseStation->hasUeCollector(nrNodeId)) {
                    UeStatsCollector *ueColl = baseStation->getUeCollector(nrNodeId);
                    CellUeInfo cellUeInfo = CellUeInfo(ueColl, baseStation->getEcgi());
                    ueArray.push_back(cellUeInfo.toJson());
                    found = true;
                    break; // next ue id
                }
            }
            if (!found) {
                std::string notFound = "Address: " + ipAddress.str() + " Not found.";
                ueArray.push_back(notFound);
            }
        }
    }

    if (ueArray.size() > 1) {
        val["cellUEInfo"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["cellUEInfo"] = ueArray[0];
    }

    return val;
}

//
nlohmann::ordered_json L2Meas::toJsonCell(std::vector<MacCellId>& cellsID) const
{
    nlohmann::ordered_json val;
    nlohmann::ordered_json l2Meas;
    nlohmann::ordered_json cellArray;
    nlohmann::ordered_json ueArray;

    if (timestamp_.isValid()) {
        val["timestamp"] = timestamp_.toJson();
    }

    for (const auto& cid : cellsID) {
        auto it = eNodeBs_.find(cid);
        if (it != eNodeBs_.end()) {
            RniCellInfo cellInfo = RniCellInfo(it->second);
            cellArray.push_back(cellInfo.toJson());

            UeStatsCollectorMap *ueMap = it->second->getCollectorMap();
            for (const auto& [ueId, ueCollector] : *ueMap) {
                CellUeInfo cellUeInfo = CellUeInfo(ueCollector, it->second->getEcgi());
                ueArray.push_back(cellUeInfo.toJson());
            }
        }
    }

    if (cellArray.size() > 1) {
        val["cellInfo"] = cellArray;
    }
    else if (cellArray.size() == 1) {
        val["cellInfo"] = cellArray[0];
    }

    if (ueArray.size() > 1) {
        val["cellUEInfo"] = ueArray;
    }
    else if (ueArray.size() == 1) {
        val["cellUEInfo"] = ueArray[0];
    }
    return val;
}

////
nlohmann::ordered_json L2Meas::toJson(std::vector<MacCellId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const
{
    nlohmann::ordered_json val;
    nlohmann::ordered_json l2Meas;
    val["cellInfo"] = toJsonCell(cellsID)["L2Meas"]["cellInfo"];
    val["cellUEInfo"] = toJsonUe(uesID)["L2Meas"]["cellUEInfo"];
    l2Meas["L2Meas"] = val;
    return l2Meas;
}

} //namespace

