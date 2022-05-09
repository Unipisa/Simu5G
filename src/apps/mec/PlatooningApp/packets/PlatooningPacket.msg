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


import nodes.mec.MECPlatform.MECPackets;
import inet.networklayer.common.L3Address;
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;


cplusplus {{
    #include "inet/common/geometry/common/Coord.h"
    using namespace inet;
    #include "nodes/mec/utils/httpUtils/json.hpp"
	using namespace nlohmann;
	#include "apps/mec/PlatooningApp/platoonController/PlatoonVehicleInfo.h"
	#include "apps/mec/PlatooningApp/PlatooningUtils.h"
	
	#include <vector>
	#include <map>
	
	typedef std::map<int, PlatoonControllerStatus *> Platoons;
	typedef std::vector<PlatoonVehicleInfo> Vehicles;
}};

class Platoons {
    @existingClass;
};

class Vehicles {
    @existingClass;
};


class PlatoonVehicleInfo {
    @existingClass;
}

class Coord {
    @existingClass;
}

class json {
    @existingClass;
}

class PlatooningAppPacket extends inet::FieldsChunk 
{
    int type;
    int consumerAppId;
    int producerAppId;
    
}


// === Packets between UEPlatooningApp and MECPlatooningApp/MECPlatooningProducerApp === //

class PlatooningJoinPacket extends PlatooningAppPacket
{       
    inet::L3Address ueAddress; // address of the UE
    
    int controllerId;  // index of the controller instance
                          // in request messages, it is the platoon the UE would like to join
                          // in response messages, it is the platoon the UE actually joined
    
        
    
    // --- request --- //
    Coord direction;      // cartesian coordinates to specify the direction of the UE
    Coord lastPosition;   // last coordinates of the UE
    
    // --- response --- //
    bool response;        // flag set by the MEC app with the ACK/NACK to join a platoon
    string color;         // color assigned to the members of the platoon
}



class PlatooningNotificationPacket extends PlatooningAppPacket
{
	int controllerId;  // index of the controller instance
    // --- response --- //
    bool response;        // flag set by the MEC app with the ACK/NACK to join a platoon
    string color;         // color assigned to the members of the platoon
}

class PlatooningLeavePacket extends PlatooningAppPacket
{       
    // --- request --- //
    int controllerId;  // index of the controller instance
	    // id of the producer app (in case of federation)
	
    // --- response --- //
    bool response;        // flag set by the MEC app with the ACK/NACK to leave a platoon

}

class PlatooningInfoPacket extends PlatooningAppPacket
{       
    Coord newAcceleration;   // new acceleration value to assign to the UE 
}


// === Packets between MECPlatooningApp and MECPlatooningProducerApp === //

class PlatooningRegistrationPacket extends PlatooningAppPacket
{
    //string mecAppAddress;
    //int mecAppPort;
}


// === Packets between MECProducerApps === //

class PlatooningAvailablePlatoonsPacket extends PlatooningAppPacket
{
    int requestId;
    
	Platoons platoons;
}


class PlatooningAddMemberPacket extends PlatooningAppPacket
{
    inet::L3Address ueAddress;
    
    inet::L3Address consumerAddress;
    int consumerPort;
     
    Coord position; 
    
    //int mecAppId; already in the base packet
    int controllerId; 
    bool response;
}



class PlatooningConfigureControllerPacket extends PlatooningAppPacket
{
    Coord direction;
    int controllerId;
    
    string lateralController;
    string longitudinalController;
    double lateralControlPeriod; 
	double lateralUpdatePositionPeriod; 
	double longitudinalControlPeriod; 
	double longitudinalUpdatePositionPeriod; 
	double heartbeatPeriod;
	bool adjustPosition = false;
	bool sendBulk = true;
   				
    bool response;
}


class PlatooningDiscoverPlatoonPacket extends PlatooningAppPacket
{
    bool onlyLocalPlatoons;
}

class PlatooningDiscoverAndAssociatePlatoonPacket extends PlatooningDiscoverPlatoonPacket
{
               
        // --- request --- //
        Coord direction;      // cartesian coordinates to specify the direction of the UE
        Coord lastPosition;   // last coordinates of the UE
        inet::L3Address ueAddress; // address of the UE
        
        // --- response --- //
        bool result;
        int controllerId;
        inet::L3Address controllerAddress;
		int controllerPort;
		
		string color;         // color assigned to the members of the platoon
}

class PlatooningAssociatePlatoonPacket extends PlatooningDiscoverAndAssociatePlatoonPacket
{
}


class PlatooningControllerNotificationPacket extends PlatooningAppPacket
{
    int notificationType;
    
    int controllerId; 
   
    Vehicles vehiclesOrder;
 	simtime_t heartbeatTimeStamp;
}




