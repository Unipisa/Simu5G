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

#include <sstream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <map>
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "stack/mac/buffer/LteMacBuffer.h"

using namespace std;
using namespace omnetpp;

LteMaxCiOptMB::LteMaxCiOptMB()
{
    problemFile_ = "./optFile.lp";
    solutionFile_     = "./solution.sol";
}


/*
 * Given N bands, 2^N possible band configuration are obtained, each one describing a
 * possible combination of active bands.
 *
 * example of band configuration for three bands
 *
 *  2   1   0    id
 *  =========
 *  0 | 0 | 1    1
 *  0 | 1 | 0    2
 *  0 | 1 | 1    3
 *  1 | 0 | 0    4
 *  1 | 0 | 1    5
 *  1 | 1 | 0    6
 *  1 | 1 | 1    7
 *
 *  If a user is scheduled for band configuration 3, it will use band 1 and 0 to communicate.
 *
 *  The following function performs the following steps
 *  - reads the per band CQI for each UE and stores them in the "cqiPerBandMatrix" structure ( < ueID , CQIs > )
 *  - for each band configuration, computes the minimum CQI between the bands active within it, and stores
 *    the band id into the "cqiPerConfigMatrix" structure ( < ueID , minBandId >
 *  - generate the optimization problem storing it into the file specified by "problemFile_"
 *
 *  NOTE: bands ID starts from 0, while Band Configuration starts from 1 ( power of two stuffs, easy to handle. You are an adult anyway )
 */
void LteMaxCiOptMB::generateProblem()
{
    int totUes = carrierActiveConnectionSet_->size();
    // skip problem generation if no User is active
    if(totUes==0)
    {
//        cout << NOW << " LteMaxCiOptMB::generateProblem - No Active Connections" << endl;
        return;
    }

    // stores per-band CQI for each UE
    std::map< MacNodeId,std::vector<Cqi> > cqiPerBandMatrix;

    // for each UE, stores the id of the band that contains the minimum CQI for the given band configuration
    // e.g. vector[3] contains the id of the band with the minimum CQI among the bands active in configuration 4 ( not 3! )
    std::map< MacNodeId,std::vector<int> > cqiPerConfigMatrix;

    bool first = true;
    int iUe = 0;
    int iBandConf = 0;
    int iBand = 0;
    int minCqi;
    int bandPattern;

    // amount of available blocks. In this scenario each band has 1 block
    int numBands = eNbScheduler_->readTotalAvailableRbs();
    if(numBands==0)
    {
        EV << NOW <<" LteMaxCiOptMB::generateProblem - No Available RBs" << endl;
        return;
    }
    // number of possible combination of bands
     int totBandConfig = pow(2,numBands)-1;

    double MAX_RATE = 100 * numBands;

    //======= init string and files =======
    stringstream appStream;
    ofstream appFileStream;
    remove(problemFile_.c_str());
    remove(solutionFile_.c_str());
    // open delta file
    appFileStream.clear();
    appFileStream.open(problemFile_.c_str(),(std::ios::app)|(std::ios::out));
    //=====================================

    // config UE ids
    // for each band configuration
    vector<int> cqiPerConfig;
    vector<Cqi> cqiPerBand;
    for ( ActiveSet::iterator it = carrierActiveConnectionSet_->begin ();it != carrierActiveConnectionSet_->end (); ++it )
    {
        cqiPerConfig.clear();
        MacNodeId ueId = MacCidToNodeId(*it);
        ueList_.push_back(ueId);
        cidList_.push_back(*it);
        cqiPerBandMatrix.insert(pair< MacNodeId,std::vector<Cqi> >(ueId,eNbScheduler_->mac_->getAmc()->readMultiBandCqi(ueId,direction_,carrierFrequency_)));
//        sort(cqiMatrix[ueId].begin(),cqiMatrix[ueId].end());

        // ******* DEBUG *******
        appFileStream << ueId<< ") CQI[ " ;
        first = true;
        for( iBand = 0 ; iBand < numBands ; ++ iBand )
        {
            if(first)
                first = false;
            else
                appFileStream << " \t, ";
            unsigned int availableBlocks = eNbScheduler_->readAvailableRbs(ueId,MACRO,iBand);
            unsigned int availableBytes_MB = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs_MB(ueId,iBand, availableBlocks, direction_,carrierFrequency_);

            appFileStream << cqiPerBandMatrix[ueId][iBand] << "/";
            cqiPerBandMatrix[ueId][iBand] = availableBytes_MB;

            appFileStream << cqiPerBandMatrix[ueId][iBand];
        }
        appFileStream << " ]"<< endl ;
        // ***** END debug *****

        /*
         *  TODO this function can be implemente more efficiently by:
         *  - sorting the bands by CQI value
         *  - for each bandConfig, browse the sorted list and select the first CQI whose band belongs to the bandConfig
         */
        // for each band configuration, select the minimum CQI
        for(iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf)
        {
            minCqi = 0;
            for( iBand = 0 ; iBand < numBands ; ++ iBand )
            {
                bandPattern = 1 << iBand;
                if(( iBandConf&bandPattern ) && (cqiPerBandMatrix[ueId][iBand]< cqiPerBandMatrix[ueId][minCqi]) )
                    minCqi = iBand;
            }
            cqiPerConfig.push_back(minCqi);
        }
        cqiPerConfigMatrix.insert(pair< MacNodeId,std::vector<int> >(ueId,cqiPerConfig));
    }
    // ==========================================================================
    // ====================== BUILDING OPTIMIZATION PROBLEM =====================
    // ==========================================================================

    appFileStream << "\\ ================ Objective Function ================"<< NOW << endl;
    appFileStream << "Maximize " << endl;

    first = true;
    appFileStream << "\\ Ue= " << totUes << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        if(first)
            first = false;
        else
            appFileStream << " + ";

        appFileStream << "v" << ueList_[iUe] << " - p" << ueList_[iUe];
    }
    appFileStream<< endl;



    appFileStream << "subject to" << endl;
    appFileStream << "\\ ================ Constraint 1 ================" << endl;
    appFileStream << "\\ Ue= " << totUes << " - totBands= " << totBandConfig << endl;

    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        first = true;
        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
        {
            if(first)
                first = false;
            else
                appFileStream << " + ";
            //               bUe_config
            appFileStream << "b" << ueList_[iUe] << "_"<<iBandConf ;
        }
        appFileStream << " <= 1"<< endl;
    }

    appFileStream << "\\ ================ Constraint 2 ================" << endl;
    for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
    {
        first = true;
        for( iUe = 0 ; iUe < totUes ; ++iUe)
        {
            if(first)
                first = false;
            else
                appFileStream << " + ";
            //               bUe_config
            appFileStream << "b" << ueList_[iUe] << "_"<<iBandConf ;
        }
        appFileStream << " <= 1"<< endl;
    }
    appFileStream << "\\ ================ Constraint 3 ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
        {
            appFileStream << "v" << ueList_[iUe] << "_" << iBandConf << " - "
                          << MAX_RATE << " b" << ueList_[iUe] << "_" << iBandConf  << " <= 0"<< endl;
        }
    }

    appFileStream << "\\ ================ Constraint 4 ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        cqiPerConfig.clear();
        cqiPerConfig = cqiPerConfigMatrix[ueId];

        cqiPerBand.clear();
        cqiPerBand = cqiPerBandMatrix[ueId];

        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
        {
            appFileStream << "v" << ueId<< "_" << iBandConf << " - " ;
            first = true;
            for( iBand = 0 ; iBand < numBands ; ++ iBand )
            {
                bandPattern = 1 << iBand;
                if( iBandConf&bandPattern )
                {
                    if(first)
                        first = false;
                    else
                        appFileStream << " - ";
                    appFileStream << cqiPerBand[cqiPerConfig[iBandConf-1]] << " s" << ueId << "_" << iBand;
                }
            }
            appFileStream << " <= 0 "<< endl;
        }
    }

    appFileStream << "\\ ================ Constraint 5 ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        appFileStream << "v" << ueId ;
        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
        {
            appFileStream << " - v" << ueId << "_" << iBandConf;
        }
        appFileStream << " = 0" << endl;
    }

//    LteMacBufferMap * buf = mac_->getMacBuffers();
//    LteMacBufferMap::iterator it = buf->begin() , et=buf->end();
//    for( ; it!=et ; ++it )
//        cout << "\t CID["<< it->first << "] - ";
//    cout << endl;

    appFileStream << "\\ ================ Constraint 6 ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        LteMacBufferMap * buf = mac_->getMacBuffers();
        LteMacBufferMap::iterator it = buf->find(cidList_[iUe]);
        int queue = 0;
        if(it == mac_->getMacBuffers()->end())
        {
            cRuntimeError("LteMaxCiOptMB::generateProblem Cannot find CID[%u]. Aborting... ",cidList_[iUe]);
        }
        queue = it->second->getQueueOccupancy();
//        appFileStream << iUe<< ")UE=" << ueList[iUe] << " - cid=" << cidList[iUe] << endl;

        MacNodeId ueId = ueList_[iUe];
        appFileStream << "v" << ueId << " - p" << ueId << " <= " << queue << endl;
    }

    appFileStream << "\\ ================ Constraint 7 ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        cqiPerConfig.clear();
        cqiPerConfig = cqiPerConfigMatrix[ueId];

        cqiPerBand.clear();
        cqiPerBand = cqiPerBandMatrix[ueId];

        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
        {
//            appFileStream << "\\ BandConf[" << iBandConf<< "] - minBand[" << cqiPerConfig[iBandConf-1]
//                          << "] - minCqi[" << cqiPerBand[cqiPerConfig[iBandConf-1]] << "]" << endl;
            appFileStream << "p" << ueId << " + " << MAX_RATE << " b" << ueId << "_" << iBandConf
                          << " <= " << (cqiPerBand[cqiPerConfig[iBandConf-1]] + MAX_RATE - 1) << endl;
        }
    }

    appFileStream << "\\ ================ Constraint 8a ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        for( iBand = 0 ; iBand < numBands ; ++ iBand )
        {
            bandPattern = 1 << iBand;

            first = true;
            for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
            {
                if( iBandConf&bandPattern )
                {
                    if(first)
                         first = false;
                     else
                         appFileStream << " + ";

                    appFileStream << " b"<< ueId << "_" << iBandConf;
                }
            }
            appFileStream << " - s" << ueId << "_" << iBand << " <= 0"<< endl;
        }
    }

    appFileStream << "\\ ================ Constraint 8b ================" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        for( iBand = 0 ; iBand < numBands ; ++ iBand )
        {
            appFileStream << "s" << ueId << "_" << iBand << " - ";
            bandPattern = 1 << iBand;

            first = true;
            for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
            {
                if( iBandConf&bandPattern )
                {
                    if(first)
                         first = false;
                     else
                         appFileStream << " - ";

                    appFileStream << " b"<< ueId << "_" << iBandConf;
                }
            }
            appFileStream << " <= 0"<< endl;
        }
    }
    appFileStream << "\\ ================ Constraint 9 ================" << endl;

    for( iBand = 0 ; iBand < numBands ; ++ iBand )
    {
        first = true;
        for( iUe = 0 ; iUe < totUes ; ++iUe)
        {
            MacNodeId ueId = ueList_[iUe];
            if(first)
                 first = false;
             else
                 appFileStream << " + ";
            appFileStream << " s" << ueId << "_" << iBand;
        }
        appFileStream << "<= 1" << endl;
    }


    appFileStream << "\\================= variables definition =================" << endl;
    appFileStream << "\\================= Integer variables =====================" << endl;
    appFileStream << "GENERAL" << endl;

    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];

        for( iBand = 0 ; iBand < numBands ; ++ iBand )
            appFileStream << "s" << ueId << "_" << iBand << endl;

        appFileStream << "v" << ueId << endl;
        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
            appFileStream << "v" << ueId << "_" << iBandConf << endl;

        appFileStream << "p" << ueId << endl;
    }


    appFileStream << "\\================= binary variables =====================" << endl;
    appFileStream << "Binary" << endl;
    for( iUe = 0 ; iUe < totUes ; ++iUe)
    {
        MacNodeId ueId = ueList_[iUe];
        for( iBandConf = 1 ; iBandConf <= totBandConfig ; ++iBandConf )
            appFileStream << "b" << ueId << "_" << iBandConf << endl;
    }
    // ==========================================================================
    // ==========================================================================
    // ==========================================================================


    appFileStream << "end" << endl;
    appFileStream.close();
}


void LteMaxCiOptMB::prepareSchedule()
{
    EV << "LteMaxCiOptMB::prepareSchedule - TEST" << endl;

    // clean all the structures
    activeConnectionTempSet_ = *activeConnectionSet_;
    cidList_.clear();
    ueList_.clear();
    schedulingDecision_.clear();
    usableBands_.clear();

    // generate the problem
    generateProblem();

    // skip the scheduling operation if no connections are active
    if(cidList_.size() == 0)
        EV << NOW << " LteMaxCiOptMB::prepareSchedule  no active connections" << endl;
    else
    {
        EV << NOW << " LteMaxCiOptMB::prepareSchedule - Launching problem..." << endl;
        launchProblem();
        EV << NOW << " LteMaxCiOptMB::prepareSchedule - Problem Solved" << endl;
        readSolution();
    }
    applyScheduling();
}

// TODO use the XML built in functions
void LteMaxCiOptMB::readSolution()
{
//    mac_->par("optSolution").setXMLValue("solution.sol");
//    cXMLElement* root = mac_->par("optSolution").xmlValue();
//    cXMLElement* elem;
//    cXMLElementList list;
//    cXMLElementList::const_iterator it;
//    string nameString;
//    string ue , band;
//    std::size_t position;
//    BandLimit bandLimit;
//
//    if (root == NULL) {
//        cRuntimeError("FATAL! LteMaxCiOptMB::readSolution - No solution file found. Aborting");
//    }
//    list = root->getChildren();
//    for (it = list.begin(); it != list.end(); ++it)
//    {
//        elem = *it;
//        const char * nodeName = elem->getTagName();
//
////        cout << NOW << " LteMaxCiOptMB::readSolution - TagName[" << nodeName << "]" << endl;
//
//        if(strcasecmp(nodeName, "variables") == 0)
//        break;
//    }
//    if(it == list.end())
//        cRuntimeError("FATAL! LteMaxCiOptMB::readSolution - Cannot find TAG Variables. Aborting");
//
//    list = elem->getChildren();
//    for (it = list.begin(); it != list.end(); ++it)
//    {
//
//        elem = *it;
//        const char * name = elem->getAttribute("name");
//        nameString.assign(name);
//        position = nameString.find('s');
//        if(position!=string::npos)
//        {
//            ue = nameString.substr(position+1,4);
//            band = nameString.substr(position+6);
//            const char * value = elem->getAttribute("value");
////            cout  << NOW << " LteMaxCiOptMB::readSolution - name[" << name << "/"<< nameString<<"] - value[" << value << "]" << endl;
//            cout  << NOW << " LteMaxCiOptMB::readSolution - Ue[" << ue<<"] - band[" << band<<"] - value[" << value << "]" << endl;
//
//            bandLimit.limit_.push_back((*(value)=='1')? -1 : -2);
//            bandLimit.band_  = atoi(band.c_str());
//            schedulingDecision_[atoi(ue.c_str())].push_back(bandLimit);
//        }
//    }

    std::ifstream file;
    std::string line;
    size_t pos;

    string nameString;
    string ue , band , value;
    BandLimit bandLimit;

    // open the solution file
    file.clear();
    file.open(solutionFile_.c_str());

    UsableBands usableBands;



    // read from file
    while(1)
    {
        if((file.rdstate() & std::istream::eofbit) != 0)
            break;
        getline(file,line);

        // find the next line containing one x variable
        pos = line.find("me=\"s");
        if(pos == string::npos)
            continue;

        // read UE id and Band ID
        ue = line.substr(pos+5,4);
        band = line.substr(pos+10,1);

        // read the value
        pos = line.find("value=");
        value = line.substr(pos+7,1);

        int limit;

        // fill the bandLimit and usableBand structures
//        cout  << NOW << " LteMaxCiOptMB::readSolution - Ue[" << ue<<"] - band[" << band<<"] - value[" << value << "]" << endl;
        limit = (value.find('1')!=string::npos)? -1 : -2;
        bandLimit.limit_.push_back(limit);
        bandLimit.band_  = atoi(band.c_str());
        schedulingDecision_[atoi(ue.c_str())].push_back(bandLimit);

        if(limit==-1)
        {
            usableBands_[atoi(ue.c_str())].push_back(bandLimit.band_);
            EV << " LteMaxCiOptMB::readSolution - Adding usable band[" << bandLimit.band_ << "] for UE[" <<  atoi(ue.c_str()) << "]" << endl;
        }

    }

    // int totUes = cidList_.size();
    int ueId;

    UsableBandList::iterator itUsable = usableBands_.begin(),
                             etUsable = usableBands_.end();
    for( ; itUsable!=etUsable ; ++itUsable )
    {
        ueId = itUsable->first;
        eNbScheduler_->mac_->getAmc()->setPilotUsableBands(ueId,itUsable->second);
    }
}


void LteMaxCiOptMB::launchProblem()
{
    std::stringstream cmd;
    FILE* fp = popen("cplex -c","w");
    if (fp != NULL)
    {
        cmd << "\"set logfile *\" \"read " << problemFile_.c_str() << " lp\" \"optimize\" ";
        cmd << "\"write " << solutionFile_.c_str() << "\" \"y\" ";
        cmd << " > /dev/null" << endl;
        fprintf(fp, "%s", cmd.str().c_str());

        fclose(fp);
    }
}

void LteMaxCiOptMB::applyScheduling()
{
//    cout << NOW << " "<< ueList_.size() << "/" << cidList_.size() << "/" << schedulingDecision_.size() << endl;

    if(cidList_.size() !=schedulingDecision_.size())
    {
        cRuntimeError("LteMaxCiOptMB::applyScheduling - number of CIDs and schedulingDecision size doesn't match. Aborting...");
    }

    int totUes = cidList_.size();
    int iUe;
    MacNodeId ueId;
    MacCid ueCid;

    for( iUe = 0 ; iUe<totUes ; ++iUe )
    {
        ueCid = cidList_[iUe];
        ueId = ueList_[iUe];

        // Grant data to that connection.
        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted = requestGrant (ueCid, 4294967295U, terminate, active, eligible,&schedulingDecision_[ueId]);

        EV << NOW << "LteMaxCiMultiband::schedule granted " << granted << " bytes to UE" << ueId << endl;

        // Exit immediately if the terminate flag is set.
        if ( terminate ) break;

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if ( ! active || ! eligible )
        {
            EV << NOW << "LteMaxCiMultiband::schedule  UE " << ueId << " was found ineligible" << endl;
        }

        // Set the connection as inactive if indicated by the grant ().
        if ( ! active )
        {
            EV << NOW << "LteMaxCiMultiband::schedule scheduling UE " << ueId << " set to inactive " << endl;
            carrierActiveConnectionSet_->erase(ueCid);
            activeConnectionTempSet_.erase (ueCid);
        }
    }
}

void LteMaxCiOptMB::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}
