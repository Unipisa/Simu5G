#include <stdlib.h>
#include <unistd.h> // for sleep
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "utils/json.hpp"
#include "deployment.h"


using namespace std;
using namespace nlohmann;

enum HttpMethod { GET, POST };


const unsigned int ueLoad[] = { 20, 30 }; 

string sendHttpMessage(HttpMethod method, string hostname, unsigned int port, const char* resource, const char* qString = NULL, const char* filePath = NULL, const char* data = NULL)
{

    const int MAXLEN = 1024;
    char response[MAXLEN];
    stringstream ss;
    FILE* fp = NULL;

    // === build cURL command === //

    ss << "curl -s -X";
    switch (method) 
    {
    case GET:  ss << "GET "; break;
    case POST: ss << "POST "; break; 
    default: break;
    }

    // add file to be sent, if specified
    if (filePath != NULL)
        ss << "-F \"document=@" << filePath << "\" ";

    if (data != NULL)
        ss << "-d \"" << data << "\" ";

    // specify resource to request
    ss << "\"" << hostname << ":" << port << "/" << resource;
   
    // add query string, if specified
    if (qString != NULL)
        ss << "?" << qString;
    ss << "\"";

    // === send HTTP message === //
    fp = popen(ss.str().c_str(),"r"); 

    // === read response === //
    ss.str("");
    while (fgets(response, MAXLEN, fp) != NULL)
        ss << response;
    fclose(fp);

    return ss.str();
}


json buildSimulationParameters(const Deployment& deployment)
{
    stringstream ss;
    json j;
     
    // add numUEs
    int numUEs = deployment.getNumUEs();
    std::cout << "--> numUEs[" << numUEs << "]" << endl;
    j["numUEs"] = numUEs;
    
    const vector<Coord>& pos = deployment.getPositions(); 
    json jUeArray;
    for (int i = 0; i < numUEs; i++)
    {
        std::cout << "-----> [" << pos[i].x_ << ", " << pos[i].y_ << "]" << endl;
        json jPos;
        jPos["x"] = pos[i].x_; 
        jPos["y"] = pos[i].y_;    
        j["uePos"].push_back(jPos);

        json jTraffic;
        jTraffic["packetSize"] = 2000;  // 160 kbps
        jTraffic["sendPeriod"] = 0.1;  
        j["ueTraffic"].push_back(jTraffic);		
    }

    ofstream outPar("parameters.json");
    outPar << j.dump() << endl;
    outPar.close();

    return j;
} 



int main(int argc, char* argv[])
{
    int numSnapshots;
    string hostname = "http://localhost";
    unsigned int port = 8080; 
    string response;
    json jResponse;
    ofstream csvOutTput, csvOutBlocks;
    double deltaMovement = 10.0;  // CHECK
    
    if (argc != 2)
    {
        cerr << "Usage: ... " << endl;
        exit(1);
    }
    numSnapshots = atoi(argv[1]);

    // create the deployment
    Deployment deployment;
   
    // get list of available metrics
    response = sendHttpMessage(GET, hostname, port, "sim", NULL, NULL);
    jResponse = json::parse(response);
    cout << jResponse.dump(4) << endl;

    // open output files
    csvOutTput.open("stats/macCellThroughputDl.csv");
    csvOutBlocks.open("stats/avgServedBlocksDl.csv");

    // print headers
    csvOutTput << "Time\t"
               << "BestChannel_Mean\tBestChannel_Variance\tBestChannel_Stddev\tBestChannel_ConfidenceInterval\t"
               << "FirstOnly_Mean\tFirstOnly_Variance\tFirstOnly_Stddev\tFirstOnly_ConfidenceInterval\n";
    csvOutBlocks << "Time\t"
               << "BestChannel_Mean\tBestChannel_Variance\tBestChannel_Stddev\tBestChannel_ConfidenceInterval\t"
               << "FirstOnly_Mean\tFirstOnly_Variance\tFirstOnly_Stddev\tFirstOnly_ConfidenceInterval\n";

    for (int snapshot = 0; snapshot < numSnapshots; snapshot++)
    {
        // move existing UEs by a maximum distance of delta
        if (snapshot > 0)
            deployment.updatePositions(deltaMovement);

        // add or remove UEs
        int numUEs = ueLoad[snapshot];
        int currentUes = deployment.getNumUEs(); 
        int diff = numUEs - currentUes;
        if (diff > 0)      
        {
            std::cout << "Adding " << diff << " UEs " << endl;
            deployment.addUEs(diff);
        }
        else if (diff < 0)
        {
            std::cout << "Removing " << diff << " UEs " << endl;
            deployment.removeUEs(diff * -1);
        }

        // build parameters.json
        json jParameters = buildSimulationParameters(deployment);

        // send sim config
        response = sendHttpMessage(POST, hostname, port, "simconfig", NULL, "./simconfig.json");
        cout << response << endl;

        // send parameters
        response = sendHttpMessage(POST, hostname, port, "parameters", NULL, "./parameters.json", NULL);
        cout << response << endl;

        // run simulations and obtain statistics
        response = sendHttpMessage(GET, hostname, port, "sim", "avgServedBlocksDl&macCellThroughputDl", NULL);
        jResponse = json::parse(response);
        cout << jResponse.dump(4) << endl;
        
        // parse statistics
        json j;
        j = jResponse["macCellThroughputDl"]["CA-BestChannel"]["gnb"];
        csvOutTput << snapshot << "\t" << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\t";
        j = jResponse["macCellThroughputDl"]["CA-FirstOnly"]["gnb"];
        csvOutTput << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\n";

        j = jResponse["avgServedBlocksDl"]["CA-BestChannel"]["gnb"];
        csvOutBlocks << snapshot << "\t" << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\t";
        j = jResponse["avgServedBlocksDl"]["CA-FirstOnly"]["gnb"];
        csvOutBlocks << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\n";

    }

    csvOutTput.close();
    csvOutBlocks.close();

    return 0;
}
