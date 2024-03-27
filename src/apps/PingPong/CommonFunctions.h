// CommonFunctions.h
#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H





#include "common/binder/Binder.h"


inet::Coord getCoordinates(const MacNodeId id);
inet::Coord getCoordinates_(cModule* mod);
cModule* getModule_(const MacNodeId id);
std::list<MacNodeId>  getAllCars();
inet::Coord getMyCurrentPosition_(cModule* module);
void showCircle(double radius, inet::Coord centerPosition,cModule* senderCarModule, cOvalFigure * circle );
std::list<MacNodeId>  getNeighbors(std::list<MacNodeId> all_cars,double Radius_,cModule*  mymodule);
float get_next_time_packet(int lambda=1) ;
double uniformGenerator(int min,int max);
int poissonGenerator(double lambda) ;
double exponentialGenerator(double lambda) ;




#endif // COMMONFUNCTIONS_H
