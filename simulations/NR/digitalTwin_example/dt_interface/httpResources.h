#include <httpserver.hpp>   // library for implementing an HTTP server
#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace httpserver;
using namespace std;

struct Position {
    double x;
    double y;
    Position(double a = 0.0, double b = 0.0) { x = a; y = b; };
};

// we assume a CBR traffic
struct TrafficProfile {
    unsigned int packetSize;
    double sendPeriod;
    TrafficProfile(unsigned int a = 40, double b = 0.2) { packetSize = a; sendPeriod = b; };
};


// --- HTTP RESOURCES --- //
class ParametersResource : public http_resource
{
     friend class SimulationResource;

     unsigned int numUEs_;
     std::vector<Position> uePos_;
     std::vector<TrafficProfile> ueTraffic_;
   
     void parseParametersFromJson(string content);

     // --- getters --- //
     unsigned int getNumUEs() { return numUEs_; }
     std::vector<Position>& getUePos() { return uePos_; }
     std::vector<TrafficProfile>& getUeTraffic() { return ueTraffic_; }     

  public:

     const std::shared_ptr<http_response> render_POST(const http_request& req);
};

class SimConfigResource : public http_resource
{
    friend class SimulationResource;

    std::vector<string> config_;   // stores the configurations that will be run
    unsigned int maxProc_;         // maximum number of parallel processes
    std::string runs_;             // runs to be launched
   
    void parseSimConfigFromJson(string content);

     // --- getters --- //
     std::vector<string>& getConfigs() { return config_; }
     unsigned int& getMaxProc() { return maxProc_; }
     std::string& getRuns() { return runs_; }

  public:

     const std::shared_ptr<http_response> render_POST(const http_request& req);
};

class SimulationResource : public http_resource
{
    ParametersResource* paramRes_;
    SimConfigResource* simConfigRes_;
    
    void prepareSimulation();
    void runSimulation();

  public:

    void bind(ParametersResource* paramRes, SimConfigResource* simConfigRes);

    const std::shared_ptr<http_response> render_GET(const http_request& req);
};
