#include "deployment.h"

Deployment::Deployment() : randGen(0)
{
}

void Deployment::addUEs(int numNewUEs)
{
    for (int i = 0; i < numNewUEs; i++)
    {
        Coord newPos;
        newPos.x_ = randGen.uniform(floorplan.minX, floorplan.maxX);
        newPos.y_ = randGen.uniform(floorplan.minY, floorplan.maxY); 

        positions.push_back(newPos);
    }
}

void Deployment::removeUEs(int numUEs)
{
    for (int i = 0; i < numUEs; i++)
        positions.pop_back();
}

void Deployment::updatePositions(double delta)
{
    for (auto it = positions.begin(); it != positions.end(); ++it)
    {
        // compute a variation in the range [-delta,delta] for each axes 
        double deltaX = randGen.uniform(delta*2) - delta;
        double deltaY = randGen.uniform(delta*2) - delta;
            
        double x = it->x_ + deltaX;
        double y = it->y_ + deltaY;

        // check if the new position is still inside the floorplan
        it->x_ = x;
        it->y_ = y;
    }
}
