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

#ifndef _LOCATION_H_
#define _LOCATION_H_

#include "common/LteCommon.h"
#include <set>
#include <map>

#include "nodes/mec/MECPlatform/MECServices/Resources/TimeStamp.h"
#include "common/binder/Binder.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "nodes/mec/MECPlatform/MECServices/LocationService/resources/UserInfo.h"
//#include "inet/common/geometry/common/Coord.h"

using namespace omnetpp;

class CellInfo;

class LocationResource : public AttributeBase
{
	public:
		/**
		 * the constructor takes a vector of te eNodeBs connceted to the MeHost
		 * and creates a CellInfo object
		*/
        LocationResource();
		LocationResource(std::string& baseUri, std::set<cModule*>& eNodeBs,  Binder* binder);
		virtual ~LocationResource();

		nlohmann::ordered_json toJson() const override;

		void addEnodeB(std::set<cModule*>& eNodeBs);
		void addEnodeB(cModule* eNodeB);
		void addBinder(Binder* binder);
		void setBaseUri(const std::string& baseUri);
		nlohmann::ordered_json toJsonCell(std::vector<MacCellId>& cellsID) const;
		nlohmann::ordered_json toJsonUe(std::vector<inet::Ipv4Address>& uesID) const;
		nlohmann::ordered_json toJson(std::vector<MacNodeId>& cellsID, std::vector<inet::Ipv4Address>& uesID) const;
		
	protected:
		//better mappa <cellID, Cellinfo>
		Binder *binder_;
		TimeStamp timestamp_;
		std::map<MacCellId, CellInfo*> eNodeBs_;
		std::string baseUri_;

		UserInfo getUserInfoByNodeId(MacNodeId nodeId, MacCellId cellId) const;
		User getUserByNodeId(MacNodeId nodeId, MacCellId cellId) const;

		nlohmann::ordered_json getUserListPerCell(std::map<MacCellId, CellInfo *>::const_iterator it) const;


};


#endif // _LOCATION_H_
