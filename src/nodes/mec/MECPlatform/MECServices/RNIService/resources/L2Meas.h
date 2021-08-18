//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _L2MEAS_H_
#define _L2MEAS_H_

#include "common/LteCommon.h"
#include <vector>
#include <map>
#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/CellInfo.h"
#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"

class EnodeBStatsCollector;
class LteBinder;

/*
* This class is responsable to retrieve the L2 measures and create the JSON body
* response. It maintains the list of all the eNB/gNBs associated with the MEC host
* where the RNIS is running and keeps the pointers to the EnodeBStatsCollector.*
*/


class L2Meas : public AttributeBase
{
	public:
        L2Meas();
		L2Meas(std::set<cModule*>& eNodeBs);
		virtual ~L2Meas();

		nlohmann::ordered_json toJson() const override;

		void addEnodeB(std::set<cModule*>& eNodeBs);
		void addEnodeB(cModule* eNodeB);

		nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
		nlohmann::ordered_json toJsonUe(std::vector<inet::Ipv4Address>& uesID) const;
		nlohmann::ordered_json toJson(std::vector<MacNodeId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const;
		

	protected:

		TimeStamp timestamp_;
		std::map<MacCellId, EnodeBStatsCollector*> eNodeBs_;
		LteBinder *binder_;

		

};


#endif // _L2MEAS_H_
