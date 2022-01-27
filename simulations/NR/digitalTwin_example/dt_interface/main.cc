#include <iostream>
#include "httpResources.h"

int main(int argc, char* argv[])
{
    webserver ws = create_webserver(8080);

    ParametersResource paramRes;
    SimConfigResource simConfigRes;
    SimulationResource simRes;
    simRes.bind(&paramRes, &simConfigRes);

    ws.register_resource("/parameters",&paramRes);
    ws.register_resource("/simconfig",&simConfigRes);
    ws.register_resource("/sim",&simRes);

    ws.start(true);

    return 0;
}
