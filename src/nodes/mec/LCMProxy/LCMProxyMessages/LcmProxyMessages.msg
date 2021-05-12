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

message LcmProxyMessage {
    string type; // LcmProxy messageType (appListReq, appListRes, create, delete)
    int connectionId; // Placeholder to retrieve the correct devApp socket
    int requestId;
}


message CreateContextApp extends LcmProxyMessage
{
	bool onboarded;
	string appPackagePath; // if onboarded == false
	string appDId;		   // if onboarded == true
	
	string ueIpAddress;   // this is used by the simulator to check if the ue has left the netowrk!
	
	string devAppId;      //device app identifier, used as identifier for the mecapp (i.e. ueAppId)
}

message CreateContextAppAck extends LcmProxyMessage
{
	bool success;
	string appInstanceUri;

	string appInstanceId;
	int contextId;	
}

message DeleteContextAppMessage extends LcmProxyMessage
{
	int contextId;
}

message DeleteContextAppAckMessage extends LcmProxyMessage
{
    bool success;
	int contextId;
}
