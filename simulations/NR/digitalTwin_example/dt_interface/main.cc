#include <iostream>
#include "httpResources.h"

int main(int argc, char* argv[])
{
    const int portNumber = 8080;
    webserver ws = create_webserver(portNumber);

    ParametersResource paramRes;
    SimConfigResource simConfigRes;
    SimulationResource simRes;
    simRes.bind(&paramRes, &simConfigRes);

    ws.register_resource("/parameters",&paramRes);
    ws.register_resource("/simconfig",&simConfigRes);
    ws.register_resource("/sim",&simRes);

    cout << "- Listening on port " << portNumber << endl;

    ws.start(true);

    return 0;
}
