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


json buildSimulationParameters(int bgApp1, int bgApp2)
{
    json j;
    j["numUEs"] = 1;
    j["bgApp1"] = bgApp1;
    j["bgApp2"] = bgApp2;

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
    ofstream csvOutRtt;
    
    RNG randGen(1);  // rng for traffic profile

    // get list of available metrics
    response = sendHttpMessage(GET, hostname, port, "sim", NULL, NULL);
    jResponse = json::parse(response);
    cout << jResponse.dump(4) << endl;

    // open output files
    csvOutRtt.open("stats/avgTaskOffloadingRtt.csv");


    // print headers
    csvOutRtt << "Time\t"
               << "MecHost1_Mean\tMecHost1_Variance\tMecHost1_Stddev\tMecHost1_ConfidenceInterval\t"
               << "MecHost2_Mean\tMecHost2_Variance\tMecHost2_Stddev\tMecHost2_ConfidenceInterval\n";
    csvOutRtt.close();

    int bgApp1 = 0;
    int bgApp2 = 0;
    for (int snapshot = 0; snapshot < numSnapshots; snapshot++)
    {
        // build parameters.json
        json jParameters = buildSimulationParameters(bgApp1, bgApp2);

        // send sim config
        response = sendHttpMessage(POST, hostname, port, "simconfig", NULL, "./simconfig.json");
        cout << response << endl;

        // send parameters
        response = sendHttpMessage(POST, hostname, port, "parameters", NULL, "./parameters.json", NULL);
        cout << response << endl;

        // run simulations and obtain statistics
        response = sendHttpMessage(GET, hostname, port, "sim", "taskOffloadingRtt", NULL);
        jResponse = json::parse(response);
        cout << jResponse.dump(4) << endl;
        
        csvOutRtt.open("stats/avgTaskOffloadingRtt.csv", std::ios::app);

        // parse statistics
        json j;
        j = jResponse["taskOffloadingRtt"]["MecHost1"]["ue[0]"];
        csvOutRtt << snapshot << "\t" << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\t";

        double rtt1 = j["mean"];

        j = jResponse["taskOffloadingRtt"]["MecHost2"]["ue[0]"];
        csvOutRtt << j["mean"] << "\t" << j["variance"] << "\t" << j["stddev"] << "\t" << j["confidenceinterval"] << "\n";

        double rtt2 = j["mean"];

        csvOutRtt.close();

        if (rtt1 < rtt2)
            bgApp1++;
        else
            bgApp2++;

    }

    csvOutRtt.close();

    return 0;
}
