#include "httpResources.h"
#include "json.hpp"         // library for handling JSON files
#include <sstream>
#include <fstream>

#define WARN_MSG "# this file is generated automatically - DO NOT EDIT"

const std::shared_ptr<http_response> ParametersResource::render_POST(const http_request& req)
{
    std::map<std::string, std::string, http::header_comparator> hdr = req.get_headers();

     if (req.get_arg("document") == "")
     {
         // we received url-encoded data
         std::cout << " NO FILE " << std::endl;
     }  
     else
     {
         // we received a file
         parseParametersFromJson(req.get_arg("document"));
     }
     return std::shared_ptr<http_response>(new string_response("Parameters are set!\n"));
}

void ParametersResource::parseParametersFromJson(string content)
{
    // parse JSON
    nlohmann::json jsonBody;
    try
    {
        jsonBody = nlohmann::json::parse(content);   // get the JSON structure
    }
    catch(nlohmann::detail::parse_error e)
    {
        std::cout  <<  e.what() << std::endl;
        // file is not correctly formatted in JSON
        exit(1);
    }

    if(jsonBody.contains("numUEs"))
    {
        numUEs_ = jsonBody["numUEs"];
    }
    else
    {
        std::cerr << "parameter 'numUEs' not found in JSON file" << endl;
        exit(1);
    }

    if(jsonBody.contains("uePos"))
    {
        nlohmann::json json_uePos = jsonBody["uePos"];
        for ( nlohmann::json::iterator jt = json_uePos.begin(); jt != json_uePos.end(); ++jt)
        {
            nlohmann::json element = *jt;
            double x = element["x"];
            double y = element["y"];
            Position posElem(x,y);
            uePos_.push_back(posElem);
        }
    }
    else
    {
        std::cerr << "parameter 'uePos' not found in JSON file" << endl;
        exit(1);
    }

    if(jsonBody.contains("ueTraffic"))
    {
        nlohmann::json json_ueTraffic = jsonBody["ueTraffic"];
        for ( nlohmann::json::iterator jt = json_ueTraffic.begin(); jt != json_ueTraffic.end(); ++jt)
        {
            nlohmann::json element = *jt;
            unsigned int packetSize = element["packetSize"];
            double sendPeriod = element["sendPeriod"];
            TrafficProfile trafficElem(packetSize,sendPeriod);
            ueTraffic_.push_back(trafficElem);
        }
    }
    else
    {
        std::cerr << "parameter 'ueTraffic' not found in JSON file" << endl;
        exit(1);
    }
}


const std::shared_ptr<http_response> SimConfigResource::render_POST(const http_request& req)
{
     if (req.get_arg("document") == "")
     {
         // we received url-encoded data
         std::cout << " NO FILE " << std::endl;
     }  
     else
     {
         // we received a file
         parseSimConfigFromJson(req.get_arg("document"));
     }
     return std::shared_ptr<http_response>(new string_response("Sim configs are set!\n"));
}


void SimConfigResource::parseSimConfigFromJson(string content)
{
    // parse JSON
    nlohmann::json jsonBody;
    try
    {
        jsonBody = nlohmann::json::parse(content);   // get the JSON structure
    }
    catch(nlohmann::detail::parse_error e)
    {
        std::cout  <<  e.what() << std::endl;
        // file is not correctly formatted in JSON
        exit(1);
    }

    if(jsonBody.contains("maxProc"))
    {
        maxProc_ = jsonBody["maxProc"];
    }
    else
    {
        std::cerr << "parameter 'maxProc' not found in JSON file" << endl;
        exit(1);
    }

    if(jsonBody.contains("config"))
    {
        nlohmann::json json_config = jsonBody["config"];
        for ( nlohmann::json::iterator jt = json_config.begin(); jt != json_config.end(); ++jt)
        {
            string element = *jt;
            config_.push_back(element);
        }
    }
    else
    {
        std::cerr << "parameter 'config' not found in JSON file" << endl;
        exit(1);
    }

    if(jsonBody.contains("repetitions"))
    {
        int numRepetitions = jsonBody["repetitions"];
        std::stringstream ss;
        for(int i=0; i<numRepetitions; i++)
        {
            ss << i;
            if (i < numRepetitions-1)
                ss << ",";
        }
        runs_ = ss.str();
    }
    else
    {
        std::cerr << "parameter 'maxProc' not found in JSON file" << endl;
        exit(1);
    }
}

void SimulationResource::bind(ParametersResource* paramRes, SimConfigResource* simConfigRes)
{
    paramRes_ = paramRes;
    simConfigRes_ = simConfigRes;
}

const std::shared_ptr<http_response> SimulationResource::render_GET(const http_request& req)
{
     string responseString;

     if (req.get_arg("start") != "")
     {
         prepareSimulation();
         responseString = "done\n";         
     }
     else
         responseString = "start key not found\n";

     return std::shared_ptr<http_response>(new string_response(responseString.c_str()));
}

void SimulationResource::prepareSimulation()
{
    ofstream outScenarioConf;
    ofstream outRunScript;

    // obtain parameters
    unsigned int numUEs = paramRes_->getNumUEs();
    std::vector<Position>& uePos = paramRes_->getUePos();
    std::vector<TrafficProfile>& ueTraffic = paramRes_->getUeTraffic();

    // obtain simulation configurations
    std::vector<string>& config = simConfigRes_->getConfigs();
    unsigned int maxProc = simConfigRes_->getMaxProc();
    std::string& runs = simConfigRes_->getRuns();
    

    // write scenario configuration
    outScenarioConf.open("input_data.ini");

    outScenarioConf << WARN_MSG << endl << endl;
    outScenarioConf << "# UEs configuration" << endl;
    outScenarioConf << "*.numUe = ${numUEs=" << numUEs << "}" << endl;
    for (unsigned int i = 0; i < numUEs; i++)
    {
        // set position
        outScenarioConf << "*.ue[" << i << "].mobility.initialX = " << uePos.at(i).x << "m" << endl;
        outScenarioConf << "*.ue[" << i << "].mobility.initialX = " << uePos.at(i).y << "m" << endl;

        // set traffic
        outScenarioConf << "*.server.app[" << i << "].PacketSize = " << ueTraffic.at(i).packetSize << endl;
        outScenarioConf << "*.server.app[" << i << "].sampling_time = " << ueTraffic.at(i).sendPeriod << "s" << endl;

        outScenarioConf << endl;
    }
    outScenarioConf.close();


    // write run script
    outRunScript.open("run_dt.sh");

    outRunScript << "#!/bin/bash" << endl << endl;
    outRunScript << WARN_MSG << endl << endl;

    outRunScript << "cd .." << endl;

    outRunScript << "SIMU5G_SRC=$SIMU5G_ROOT/src" << endl;
    outRunScript << "INET_SRC=`(cd $SIMU5G_ROOT/../inet4.3/src ; pwd)`" << endl;;
    outRunScript << "COMMAND_LINE_OPTIONS=\"$SIMU5G_ROOT/simulations:$SIMU5G_ROOT/emulation:$SIMU5G_SRC:$INET_SRC\"" << endl << endl;

    // for each config
    for (unsigned int i = 0; i < config.size(); i++)
    {
        outRunScript << "opp_runall -j" << maxProc << " "
                     << " opp_run -l $SIMU5G_SRC/simu5g "
                     << "-n $COMMAND_LINE_OPTIONS "
                     << "-c " << config.at(i) << " "
                     << "-r " << runs << endl;
    }
    outRunScript.close();


}
