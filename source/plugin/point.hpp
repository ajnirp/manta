#ifndef POINT_HPP
#define POINT_HPP

#include "pclass.h"
#include <bits/stdc++.h>

namespace Manta {

PYTHON class Point : public PbClass {
public:
    double x, y, z;
    PYTHON Point(FluidSolver* parent, double xx=0, double yy=0, double zz=0);
    PYTHON void set(double xx, double yy, double zz);
    PYTHON void print();
};

}

#endif