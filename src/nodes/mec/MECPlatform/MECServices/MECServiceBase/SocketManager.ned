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

package simu5g.nodes.mec.MECPlatform.MECServices.MECServiceBase;

//
// This simple module is responsible for managing the state of TCP connections for MEC services.
// It is instantiated by MecServiceBase to handle each new connection, utilizing the
// inet::TcpSocket::CallbackInterface. It processes incoming HTTP packets and manages
// requests and responses within the system.
//
simple SocketManager
{
}
