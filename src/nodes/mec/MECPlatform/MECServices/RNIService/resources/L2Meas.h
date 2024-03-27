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

#ifndef _L2MEAS_H_
#define _L2MEAS_H_

#include "common/LteCommon.h"
#include <vector>
#include <map>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

class BaseStationStatsCollector;
class Binder;

/*
* This class is responsable to retrieve the L2 measures and create the JSON body
* response. It maintains the list of all the eNB/gNBs associated with the MEC host
* where the RNIS is running and keeps the pointers to the s.*
*/


class L2Meas : public AttributeBase
{
	public:
        L2Meas();
		L2Meas(std::set<omnetpp::cModule*>& eNodeBs);
		virtual ~L2Meas();

		nlohmann::ordered_json toJson() const override;

		void addEnodeB(std::set<omnetpp::cModule*>& eNodeBs);
		void addEnodeB(omnetpp::cModule* eNodeB);

		nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
		nlohmann::ordered_json toJsonUe(std::vector<inet::Ipv4Address>& uesID) const;
		nlohmann::ordered_json toJson(std::vector<MacNodeId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const;
		

	protected:

		TimeStamp timestamp_;
		std::map<MacCellId, BaseStationStatsCollector*> eNodeBs_;
		Binder *binder_;
};


#endif // _L2MEAS_H_
