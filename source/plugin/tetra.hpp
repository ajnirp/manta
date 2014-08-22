#ifndef TETRA_HPP
#define TETRA_HPP

#include "pclass.h"
#include <bits/stdc++.h>
#include <GL/glew.h>

namespace Manta {

PYTHON class Tetra : public PbClass {
public:
    int points[4]; // points referred to by their tetmesh indices
    PYTHON Tetra(FluidSolver* parent);
    PYTHON void set(int pt0, int pt1, int pt2, int pt3);
};

}

#endif