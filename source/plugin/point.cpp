#include "point.hpp"

namespace Manta {

Point::Point(FluidSolver* parent, double xx, double yy, double zz) : PbClass(parent), x(xx), y(yy), z(zz) {}

void Point::set(double xx, double yy, double zz) {
    x = xx;  y = yy; z = zz;
}
void Point::print() {
    std::cout << x << " " << y << " " << z << std::endl;
}

}