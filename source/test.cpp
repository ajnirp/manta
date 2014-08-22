/******************************************************************************
 *
 * MantaFlow fluid solver framework
 * Copyright 2011 Tobias Pfaff, Nils Thuerey 
 *
 * This program is free software, distributed under the terms of the
 * GNU General Public License (GPL) 
 * http://www.gnu.org/licenses
 *
 * Use this file to test new functionality
 *
 ******************************************************************************/

#include "levelset.h"
#include "commonkernels.h"
#include <cmath>

using namespace std;

namespace Manta {

//! Kernel: get component (not shifted)
KERNEL(idx) returns(Grid<Real> ret(parent))
Grid<Real> GetComponent2(const Grid<Vec3>& grid, int dim) {
    ret[idx] = grid[idx][dim];
};

PYTHON void testp(Grid<Vec3>& b) {
    Grid<Real> d(parent);
    b(20,20,20) = Vec3(21,22,23); 
    {
        cout <<"middle" << endl;        
        Grid<Real> a = GetComponent2(b,0);
        cout << a(20,20,20) << endl;        
        cout <<"middle" << endl;        
    }
    cout << "end" << endl;errMsg("f");
}


PYTHON void initVelTest(MACGrid& vel, float s, float y) {
	FOR_IJK_BND(vel,1) {
		if(j==int(vel.getSizeY()*y)+0) {
			vel(i,j,k)[1] = s;
		}
		if(j==int(vel.getSizeY()*y)+1) {
			vel(i,j,k)[1] = -s;
		}
	}
}


//! Helper co compute curl of velocities
PYTHON void getCurl(MACGrid& vel, Grid<Real>& vort, int comp) {
    Grid<Vec3> velCenter(parent), curl(parent);
    
    GetCentered(velCenter, vel);
    CurlOp(velCenter, curl);
    GetComponent(curl, vort, comp);
}

    
} //namespace

