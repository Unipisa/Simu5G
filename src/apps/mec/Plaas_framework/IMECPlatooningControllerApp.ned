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

package simu5g.apps.mec.Plaas_framework;


import inet.applications.contract.IApp;
import simu5g.nodes.mec.MECPlatform.IMECApp;

//
//  This is a MEC app interface that ... TBC
//
moduleinterface IMECPlatooningControllerApp extends IMECApp
{
    parameters:
        @display("i=block/app;is=s");
        @class("MECPlatooningControllerApp");
		string transitionalController;
		string longitudinalController;
		double controlPeriod @unit(s);         
		double updatePositionPeriod @unit(s);
		bool adjustPosition;       
		bool sendBulk;
}


