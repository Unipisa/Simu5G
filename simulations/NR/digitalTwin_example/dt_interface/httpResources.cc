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
         cout << "--- Received new simulation parameters from " << req.get_requestor() << endl;
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
         cout << "--- Received new simulation configurations from " << req.get_requestor() << endl;
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
    cout << "--- Received request to start a new simulation campaign from " << req.get_requestor() << endl;

    // get requested metrics
    cout << "Requested metrics:" << endl;
    std::vector<string> metrics;
    const std::map<string, string, http::arg_comparator> args = req.get_args();
    for (auto it = args.begin(); it != args.end(); ++it)
    {   
        cout << "\t" << it->first << endl;
        metrics.push_back(it->first);  // TODO check if the metric is listed among the available ones (report some error or null value in the response)
    }

    // prepare and run simulation campaign
    prepareSimulation();
    runSimulation();

    // extract metrics and build the JSON-formatted response 
    string_response* response = parseResults(metrics);

    cout << "--- Response sent to " << req.get_requestor() << endl;
    cout << "-----------------------------------------------------" << endl << endl;
    return std::shared_ptr<http_response>(response);
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
    outRunScript.open("run_dt.sh");  // TODO maybe you need to make sure that execution permission is assigned

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
                     << "-r " << runs << " ";
        if (i == 0)
            outRunScript << "> sim_log" << endl;
        else
            outRunScript << ">> sim_log" << endl;
    }

    outRunScript.close();
}

void SimulationResource::runSimulation()
{
    cout << "--- Now running simulation campaign... ";
    cout.flush();

    // run simulations
    FILE* fp = popen("./run_dt.sh","r");
    fclose(fp);

    cout << "DONE ---" << endl;
}

string_response* SimulationResource::parseResults(const std::vector<string>& metrics)
{
    nlohmann::json jResp;

    cout << "--- Now parsing simulation results... ";
    cout.flush();

    string stat[4] = {"mean", "variance", "stddev", "confidenceinterval"};
    stringstream ss;
    char result[64];
    std::vector<string>& config = simConfigRes_->getConfigs();
    for (unsigned int i = 0; i < config.size(); i++)
    {
        ss.str("");
        ss << "./scripts/extractStats.sh " << config.at(i);
        for (unsigned int j = 0; j < metrics.size(); j++)
        {
            ss << " " << metrics[j];

            // run script for parsing the given metric
            FILE* fp = popen(ss.str().c_str(),"r");
   
            // read from the pipe and build JSON response
            nlohmann::json obj_primary;
            for (unsigned int i = 0; i < 4; i++)
            {
                fgets(result,64,fp);
                result[strlen(result)-1] = '\0';
                obj_primary[stat[i]] = result;
            }
            jResp[metrics[j].c_str()][config[i].c_str()]["primaryCell"] = obj_primary;

            nlohmann::json obj_secondary;
            for (unsigned int i = 0; i < 4; i++)
            {
                fgets(result,64,fp);
                result[strlen(result)-1] = '\0';
                obj_secondary[stat[i]] = result;
            }
            jResp[metrics[j].c_str()][config[i].c_str()]["secondaryCell"] = obj_secondary;

            fclose(fp);
        }
    }

    cout << "DONE ---" << endl << endl;

    string_response* response = new string_response(jResp.dump(4).c_str());
    return response;
}
