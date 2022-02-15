#include <vector>
#include "utils/rng.h"

using namespace std;

struct Coord
{
    double x_;
    double y_;
    double z_;

    Coord(double x = 0.0, double y = 0.0, double z = 0.0) 
    { 
        x_ = x; 
        y_ = y; 
        z_ = z; 
    }
};

class Deployment
{
    // this random number generator is used to generate the scenario parameters
    RNG randGen;
    
    struct
    {
        double minX = 750.0;
        double maxX= 1250.0;
        double minY = 750.0;
        double maxY = 1250.0;
    } floorplan;

    int numUEs;
    vector<Coord> positions; 

  public:
    Deployment();

    int getNumUEs() const { return positions.size(); }
    const vector<Coord>& getPositions() const { return positions; }  

    void addUEs(int numNewUEs);
    void removeUEs(int numUEs);
    void updatePositions(double delta);
};
