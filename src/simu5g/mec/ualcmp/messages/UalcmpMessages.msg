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

namespace simu5g;

message UalcmpMessage {
    string type; // UalcmpMessage messageType (appListReq, appListRes, create, delete)
    int connectionId; // Placeholder to retrieve the correct devApp socket
    int requestId;
}


message CreateContextApp extends UalcmpMessage
{
	bool onboarded;
	string appPackagePath; // if onboarded == false
	string appDId;		   // if onboarded == true

	string ueIpAddress;   // this is used by the simulator to check if the ue has left the netowrk!

	string devAppId;      //device app identifier, used as identifier for the mecapp (i.e. ueAppId)
}

message CreateContextAppAck extends UalcmpMessage
{
	bool success;
	string appInstanceUri;

	string appInstanceId;
	int contextId;
}

message DeleteContextAppMessage extends UalcmpMessage
{
	int contextId;
}

message DeleteContextAppAckMessage extends UalcmpMessage
{
    bool success;
	int contextId;
}
