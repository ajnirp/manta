#include "tetra.hpp"

namespace Manta {

Tetra::Tetra(FluidSolver* parent) : PbClass(parent) {}

void Tetra::set(int pt0, int pt1, int pt2, int pt3) {
    points[0] = pt0; points[1] = pt1; points[2] = pt2; points[3] = pt3;
}

}