#include <stdlib.h>
#include <unistd.h> // for sleep
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "utils/json.hpp"
#include "utils/rng.h"
#include "deployment.h"


using namespace std;
using namespace nlohmann;

enum HttpMethod { GET, POST };

const int numSnapshots = 30;  // can be read from command line
unsigned int ueLoad[numSnapshots]; 

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
        jTraffic["packetSize"] = 2000;  // 320 kbps
        jTraffic["sendPeriod"] = 0.05;  
        j["ueTraffic"].push_back(jTraffic);		
    }

    ofstream outPar("parameters.json");
    outPar << j.dump() << endl;
    outPar.close();

    return j;
} 



int main(int argc, char* argv[])
{
    string hostname = "http://localhost";
    unsigned int port = 8080; 
    string response;
    json jResponse;
    ofstream csvOutTput, csvOutBlocks, csvOutPower;
    double deltaMovement = 50.0;  // CHECK
    
    RNG randGen(1);  // rng for traffic profile

    // configure traffic profile

    for (unsigned int i = 0; i < 4; i++)
        ueLoad[i] = randGen.uniform(10,15);
    for (unsigned int i = 4; i < 8; i++)
        ueLoad[i] = randGen.uniform(13,18);
    for (unsigned int i = 8; i < 12; i++)
        ueLoad[i] = randGen.uniform(15,22);
    for (unsigned int i = 12; i < 15; i++)
        ueLoad[i] = randGen.uniform(18,23);
    for (unsigned int i = 15; i < 18; i++)
        ueLoad[i] = randGen.uniform(20,28);
    for (unsigned int i = 18; i < 23; i++)
        ueLoad[i] = randGen.uniform(25,35);
    for (unsigned int i = 23; i < 27; i++)
        ueLoad[i] = randGen.uniform(20,27);
    for (unsigned int i = 27; i < 30; i++)
        ueLoad[i] = randGen.uniform(12,20);


    // create the deployment
    Deployment deployment;
   
    // get list of available metrics
    response = sendHttpMessage(GET, hostname, port, "sim", NULL, NULL);
    jResponse = json::parse(response);
    cout << jResponse.dump(4) << endl;

    // open output files
    csvOutTput.open("stats/macCellThroughputDl.csv");
    csvOutBlocks.open("stats/avgServedBlocksDl.csv");
    csvOutPower.open("stats/avgPowerConsumption.csv");

    // print headers
    csvOutTput << "Time\t"
               << "BestChannel_Mean\tBestChannel_Variance\tBestChannel_Stddev\tBestChannel_ConfidenceInterval\t"
               << "FirstOnly_Mean\tFirstOnly_Variance\tFirstOnly_Stddev\tFirstOnly_ConfidenceInterval\n";
    csvOutBlocks << "Time\t"
               << "BestChannel_Mean\tBestChannel_Variance\tBestChannel_Stddev\tBestChannel_ConfidenceInterval\t"
               << "FirstOnly_Mean\tFirstOnly_Variance\tFirstOnly_Stddev\tFirstOnly_ConfidenceInterval\n";
    csvOutPower << "Time\t"
               << "BestChannel_Mean\tBestChannel_Variance\tBestChannel_Stddev\tBestChannel_ConfidenceInterval\t"
               << "FirstOnly_Mean\tFirstOnly_Variance\tFirstOnly_Stddev\tFirstOnly_ConfidenceInterval\n";

    csvOutTput.close();
    csvOutBlocks.close();
    csvOutPower.close();

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
        response = sendHttpMessage(GET, hostname, port, "sim", "avgCellPowerConsumption&avgServedBlocksDl&macCellThroughputDl", NULL);
        jResponse = json::parse(response);
        cout << jResponse.dump(4) << endl;
        
        csvOutTput.open("stats/macCellThroughputDl.csv", std::ios::app);
        csvOutBlocks.open("stats/avgServedBlocksDl.csv", std::ios::app);
        csvOutPower.open("stats/avgPowerConsumption.csv", std::ios::app);

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

        j = jResponse["avgCellPowerConsumption"]["CA-BestChannel"]["gnb"];
        csvOutPower << snapshot << "\t" << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\t";
        j = jResponse["avgCellPowerConsumption"]["CA-FirstOnly"]["gnb"];
        csvOutPower << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\n";

        csvOutTput.close();
        csvOutBlocks.close();
        csvOutPower.close();
    }

    csvOutTput.close();
    csvOutBlocks.close();
    csvOutPower.close();

    return 0;
}
