/*
 * FLaaSUtils.h
 *
 *  Created on: Sep 22, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_
#define APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_

enum FLTrainingMode
{
    SYNCHRONOUS, ASYNCHRONOUS, BOTH
};

struct Endpoint {
    inet::L3Address addr;
    int port;
    std::string str() const { return addr.str() + ":" + std::to_string(port);}
};



#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_ */
