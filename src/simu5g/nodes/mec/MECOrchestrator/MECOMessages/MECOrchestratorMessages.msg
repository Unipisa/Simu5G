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

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

message MECOrchestratorMessage {
    int contextId; // identifier of the ueMecApp 
    int requestId; // used to inform the UALCMP to which request the response refers
    string type; // trigger of mecApp setup or termination 	 
    bool success;   
}

class CreateAppMessage extends inet::FieldsChunk {
   
    unsigned int sno;
    simtime_t timestamp;
    
    //type of message
    string type;
    
    //communication informations
    string sourceAddress;
    int	   sourcePort;   // added by Alessandro Noferi
    string destinationAddress;
    int	   destinationPort;   // added by Alessandro Noferi
    string destinationMecAppAddress;
    int	   destinationMecAppPort;   // added by Alessandro Noferi
    
    
    //instantiation informations
    string MEModuleType;			//path where to find the cModule of the MEApp to instantiate
    string MEModuleName;			//module class name of the MEApp to instantiate
    int contextId;
    
    //identification information
    int ueAppID;
    
    //resources required
    double requiredDisk;
    double requiredRam;
    double requiredCpu;
    
    //service required
    string requiredService;
    //service provded
    string providedService;
    
}

class DeleteAppMessage extends inet::FieldsChunk {
   
    unsigned int sno;
    simtime_t timestamp;
    
    //type of message
    string type;
    
    //communication informations
    string sourceAddress;
    int	   sourcePort;   // added by Alessandro Noferi
    string destinationAddress;
    int	   destinationPort;   // added by Alessandro Noferi
    string destinationMecAppAddress;
    int	   destinationMecAppPort;   // added by Alessandro Noferi
    
    
    //instantiation informations
    string MEModuleType;			//path where to find the cModule of the MEApp to instantiate
    string MEModuleName;			//module class name of the MEApp to instantiate
    
    //identification information
    int ueAppID;
    
    //resources required
    unsigned int requiredDisk;
    unsigned int requiredRam;
    double requiredCpu;
    
    //service required
    string requiredService;
}

