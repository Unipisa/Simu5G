/*
 * AperiodicSubscriptionTimer.cpp
 *
 *  Created on: Mar 23, 2021
 *      Author: linofex
 */

#include "AperiodicSubscriptionTimer.h"

AperiodicSubscriptionTimer::AperiodicSubscriptionTimer() {
    // TODO Auto-generated constructor stub
}

AperiodicSubscriptionTimer::AperiodicSubscriptionTimer(const char *name, const double& period):AperiodicSubscriptionTimer_m(name)
{
    setPeriod(period);
}

AperiodicSubscriptionTimer::AperiodicSubscriptionTimer(const char *name): AperiodicSubscriptionTimer_m(name){}

AperiodicSubscriptionTimer::~AperiodicSubscriptionTimer() {
    // TODO Auto-generated destructor stub
}

