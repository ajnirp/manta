/******************************************************************************
 *
 * MantaFlow fluid solver framework
 * Copyright 2011 Tobias Pfaff, Nils Thuerey 
 *
 * This program is free software, distributed under the terms of the
 * GNU General Public License (GPL) 
 * http://www.gnu.org/licenses
 *
 * Set boundary conditions, gravity
 *
 ******************************************************************************/

#include "vectorbase.h"
#include "grid.h"
#include "commonkernels.h"

using namespace std;

namespace Manta { 

//! add Forces between fl/fl and fl/em cells
KERNEL(bnd=1) void KnAddForceField(FlagGrid& flags, MACGrid& vel, Grid<Vec3>& force) {
    bool curFluid = flags.isFluid(i,j,k);
    bool curEmpty = flags.isEmpty(i,j,k);
    if (!curFluid && !curEmpty) return;
    
    if (flags.isFluid(i-1,j,k) || (curFluid && flags.isEmpty(i-1,j,k))) 
        vel(i,j,k).x += 0.5*(force(i-1,j,k).x + force(i,j,k).x);
    if (flags.isFluid(i,j-1,k) || (curFluid && flags.isEmpty(i,j-1,k))) 
        vel(i,j,k).y += 0.5*(force(i,j-1,k).y + force(i,j,k).y);
    if (vel.is3D() && (flags.isFluid(i,j,k-1) || (curFluid && flags.isEmpty(i,j,k-1))))
        vel(i,j,k).z += 0.5*(force(i,j,k-1).z + force(i,j,k).z);
}

//! add Forces between fl/fl and fl/em cells
KERNEL(bnd=1) void KnAddForce(FlagGrid& flags, MACGrid& vel, Vec3 force) {
    bool curFluid = flags.isFluid(i,j,k);
    bool curEmpty = flags.isEmpty(i,j,k);
    if (!curFluid && !curEmpty) return;
    
    if (flags.isFluid(i-1,j,k) || (curFluid && flags.isEmpty(i-1,j,k))) 
        vel(i,j,k).x += force.x;
    if (flags.isFluid(i,j-1,k) || (curFluid && flags.isEmpty(i,j-1,k))) 
        vel(i,j,k).y += force.y;
    if (vel.is3D() && (flags.isFluid(i,j,k-1) || (curFluid && flags.isEmpty(i,j,k-1))))
        vel(i,j,k).z += force.z;
}

//! add gravity forces to all fluid cells
PYTHON void addGravity(FlagGrid& flags, MACGrid& vel, Vec3 gravity) {    
    Vec3 f = gravity * parent->getDt() / flags.getDx();
    KnAddForce(flags, vel, f);
}

//! add Buoyancy force based on smoke density
KERNEL(bnd=1) void KnAddBuoyancy(FlagGrid& flags, Grid<Real>& density, MACGrid& vel, Vec3 strength) {    
    if (!flags.isFluid(i,j,k)) return;
    if (flags.isFluid(i-1,j,k))
        vel(i,j,k).x += (0.5 * strength.x) * (density(i,j,k)+density(i-1,j,k));
    if (flags.isFluid(i,j-1,k))
        vel(i,j,k).y += (0.5 * strength.y) * (density(i,j,k)+density(i,j-1,k));
    if (vel.is3D() && flags.isFluid(i,j,k-1))
        vel(i,j,k).z += (0.5 * strength.z) * (density(i,j,k)+density(i,j,k-1));    
}

//! add Buoyancy force based on smoke density
PYTHON void addBuoyancy(FlagGrid& flags, Grid<Real>& density, MACGrid& vel, Vec3 gravity) {
    Vec3 f = - gravity * parent->getDt() / parent->getDx();
    KnAddBuoyancy(flags,density, vel, f);
}

        
//! set no-stick wall boundary condition between ob/fl and ob/ob cells
KERNEL void KnSetWallBcs(FlagGrid& flags, MACGrid& vel) {
    bool curFluid = flags.isFluid(i,j,k);
    bool curObstacle = flags.isObstacle(i,j,k);
    if (!curFluid && !curObstacle) return;
    
    // we use i>0 instead of bnd=1 to check outer wall
    if (i>0 && (flags.isObstacle(i-1,j,k) || (curObstacle && flags.isFluid(i-1,j,k))))
        vel(i,j,k).x = 0;
    if (j>0 && (flags.isObstacle(i,j-1,k) || (curObstacle && flags.isFluid(i,j-1,k))))
        vel(i,j,k).y = 0;
    if (vel.is2D() || (k>0 && (flags.isObstacle(i,j,k-1) || (curObstacle && flags.isFluid(i,j,k-1)))))
        vel(i,j,k).z = 0;
		
	if (curFluid) {
		if ((i>0 && flags.isStick(i-1,j,k)) || (i<flags.getSizeX()-1 && flags.isStick(i+1,j,k)))
			vel(i,j,k).y = vel(i,j,k).z = 0;
		if ((j>0 && flags.isStick(i,j-1,k)) || (j<flags.getSizeY()-1 && flags.isStick(i,j+1,k)))
			vel(i,j,k).x = vel(i,j,k).z = 0;
		if (vel.is3D() && ((k>0 && flags.isStick(i,j,k-1)) || (k<flags.getSizeZ()-1 && flags.isStick(i,j,k+1))))
			vel(i,j,k).x = vel(i,j,k).y = 0;
	}
}

//! set no-stick boundary condition on walls
PYTHON void setWallBcs(FlagGrid& flags, MACGrid& vel) {
    KnSetWallBcs(flags, vel);
} 

//! Kernel: gradient norm operator
KERNEL(bnd=1) void KnConfForce(Grid<Vec3>& force, const Grid<Real>& grid, const Grid<Vec3>& curl, Real str) {
    Vec3 grad = 0.5 * Vec3(        grid(i+1,j,k)-grid(i-1,j,k), 
                                   grid(i,j+1,k)-grid(i,j-1,k), 0.);
    if(grid.is3D()) grad[2]= 0.5*( grid(i,j,k+1)-grid(i,j,k-1) );
    normalize(grad);
    force(i,j,k) = str * cross(grad, curl(i,j,k));
}

PYTHON void vorticityConfinement(MACGrid& vel, FlagGrid& flags, Real strength) {
    Grid<Vec3> velCenter(parent), curl(parent), force(parent);
    Grid<Real> norm(parent);
    
    GetCentered(velCenter, vel);
    CurlOp(velCenter, curl);
    GridNorm(norm, curl);
    KnConfForce(force, norm, curl, strength);
    KnAddForceField(flags, vel, force);
}

//! enforce a constant inflow/outflow at the grid boundaries
KERNEL void KnSetInflow(MACGrid& vel, int dim, int p0, const Vec3& val) {
    Vec3i p(i,j,k);
    if (p[dim] == p0 || p[dim] == p0+1)
        vel(i,j,k) = val;
}

//! enforce a constant inflow/outflow at the grid boundaries
PYTHON void setInflowBcs(MACGrid& vel, string dir, Vec3 value) {
    for(size_t i=0; i<dir.size(); i++) {
        if (dir[i] >= 'x' && dir[i] <= 'z') { 
            int dim = dir[i]-'x';
            KnSetInflow(vel,dim,0,value);
        } else if (dir[i] >= 'X' && dir[i] <= 'Z') {
            int dim = dir[i]-'X';
            KnSetInflow(vel,dim,vel.getSize()[dim]-1,value);
        } else 
            errMsg("invalid character in direction string. Only [xyzXYZ] allowed.");
    }
}

} // namespace
