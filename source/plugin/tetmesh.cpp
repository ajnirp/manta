#include "tetmesh.hpp"

namespace Manta {

Point::Point(FluidSolver* parent, double xx, double yy, double zz) : PbClass(parent), x(xx), y(yy), z(zz) {}

void Point::set(double xx, double yy, double zz) {
    x = xx;  y = yy; z = zz;
}
void Point::print() {
    std::cout << x << " " << y << " " << z << std::endl;
}

Tetra::Tetra(FluidSolver* parent) : PbClass(parent) {}

void Tetra::set(int pt0, int pt1, int pt2, int pt3) {
    points[0] = pt0; points[1] = pt1; points[2] = pt2; points[3] = pt3;
}

TetMesh::TetMesh(FluidSolver* parent, std::string filename, double x, double y, double z) : PbClass(parent) {
    pos = new Point(parent);
    pos->set(x,y,z);
    try {
        std::ifstream is;
        is.open(filename.c_str());
        if (is.is_open()) {
            std::string line;
            int line_num = 0;
            while (is.good()) {
                line_num++;
                getline(is, line);
                if (line[0] == '#') {
                    // std::cout << line_num << ": ignoring comment " << std::endl;
                    continue;
                }
                if (line == "") {
                    // std::cout << line_num << ": empty line " << std::endl;
                    continue;
                }
                if (line == "ASCII") {
                    // std::cout << line_num << ": saw ASCII line " << std::endl;
                    continue;
                }
                if (line.length() >= 6 and line.substr(0, 6) == "POINTS") {
                    // std::cout << line_num << ": saw POINTS line " << std::endl;
                    std::stringstream ss(line);
                    { std::string _; ss >> _; } // we don't care about this sring
                    ss >> this->num_points; this->points = new Point*[this->num_points];
                    for (int i = 0 ; i < num_points ; i++) {
                        line_num++;
                        points[i] = new Point(parent);
                        is >> points[i]->x >> points[i]->y >> points[i]->z;
                        // std::cout << "\t" << line_num << ": read point " << i << ": "; points[i].print();
                    }
                }
                if (line.length() >= 5 and line.substr(0, 5) == "CELLS") {
                    // std::cout << line_num << ": saw CELLS line " << std::endl;
                    std::stringstream ss(line);
                    { std::string _; ss >> _; } // we don't care about this sring
                    ss >> this->num_tetras;
                    this->tetras = new Tetra*[this->num_tetras];
                    for (int i = 0 ; i < num_tetras ; i++) {
                        line_num++;
                        { int _; is >> _; } // num points is always 4 for a tet mesh
                        tetras[i] = new Tetra(parent);
                        int i0,i1,i2,i3; is >> i0 >> i1 >> i2 >> i3;
                        tetras[i]->set(i0,i1,i2,i3);
                        // std::cout << "\t" << line_num << ": read tetra " << i << ": " << indices[0] << " " << indices[1] << " " << indices[2] << " " << indices[3] << std::endl;
                    }
                }
            }
        }
    }
    catch (const char* err) {
        std::cerr << err << std::endl;
    }
}

// return all its points coordinates in world space
// and in a double[] array instead of a Point[] array
// note: this allocates memory on the heap which must be freed
const double* TetMesh::get_points() {
    double* result = new double[3*this->num_points];        
    for (int i = 0 ; i < this->num_points ; i++) {
        result[3*i] = this->pos->x + this->points[i]->x;
        result[3*i+1] = this->pos->y + this->points[i]->y;
        result[3*i+2] = this->pos->z + this->points[i]->z;
    }
    return result;
}
const int TetMesh::get_num_points() { return this->num_points; };

// return a pointer to an array of indices in order to draw triangles
// note: this allocates memory on the heap which must be freed
const int* TetMesh::get_indices() {
    int* result = new int[12*num_tetras];
    int i = 0;
    for (int j = 0 ; j < num_tetras ; j++) {
        Tetra* t = this->tetras[j];
        result[i]   = t->points[0]; result[i+1]  = t->points[1]; result[i+2]  = t->points[2];
        result[i+3] = t->points[0]; result[i+4]  = t->points[1]; result[i+5]  = t->points[3];
        result[i+6] = t->points[1]; result[i+7]  = t->points[2]; result[i+8]  = t->points[3];
        result[i+9] = t->points[0]; result[i+10] = t->points[2]; result[i+11] = t->points[3];
        i += 12;
    }
    return result;
}
// for each tetrahedron, there are four triangles i.e. 12 indices
const int TetMesh::get_num_indices() { return 12 * this->num_tetras; }

}