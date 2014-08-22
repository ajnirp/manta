#ifndef TETMESH_HPP
#define TETMESH_HPP

#include "pclass.h"
#include "tetra.hpp"
#include "point.hpp"

namespace Manta {

PYTHON class TetMesh : public PbClass
{
private:
    Point** points;
    Tetra** tetras;
    long num_points;
    long num_tetras;
    Point* pos; // position in world space
public:
    PYTHON TetMesh(FluidSolver* parent, std::string filename, double x=0, double y=0, double z=0);
    ~TetMesh() {
        delete pos;
        for (int i = 0 ; i < num_points ; i++) delete points[i];
        for (int i = 0 ; i < num_tetras ; i++) delete tetras[i];
    };

    // return all its points coordinates in world space
    // and in a double[] array instead of a Point[] array
    // note: this allocates memory on the heap which must be freed
    const double* get_points();
    const int get_num_points();

    // return a pointer to an array of indices in order to draw triangles
    // note: this allocates memory on the heap which must be freed
    const int* get_indices();
    // for each tetrahedron, there are four triangles i.e. 12 indices
    const int get_num_indices();
};

}

#endif