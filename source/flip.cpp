/******************************************************************************
 *
 * MantaFlow fluid solver framework
 * Copyright 2011 Tobias Pfaff, Nils Thuerey 
 *
 * This program is free software, distributed under the terms of the
 * GNU General Public License (GPL) 
 * http://www.gnu.org/licenses
 *
 * FLIP (fluid implicit particles)
 * for use with particle data fields
 *
 ******************************************************************************/

#include "particle.h"
#include "grid.h"
#include "randomstream.h"
#include "levelset.h"

using namespace std;
namespace Manta {




// init

PYTHON void sampleFlagsWithParticles( FlagGrid& flags, BasicParticleSystem& parts, 
		int discretization, Real randomness ) 
{
	bool is3D = flags.is3D();
    Real jlen = randomness / discretization;
    Vec3 disp (1.0 / discretization, 1.0 / discretization, 1.0/discretization);
    RandomStream mRand(9832);
 
    //clear(); 

    FOR_IJK_BND(flags, 0) {
        if ( flags.isObstacle(i,j,k) ) continue;
        if ( flags.isFluid(i,j,k) ) {
            Vec3 pos (i,j,k);
            for (int dk=0; dk<(is3D ? discretization : 1); dk++)
            for (int dj=0; dj<discretization; dj++)
            for (int di=0; di<discretization; di++) {
                Vec3 subpos = pos + disp * Vec3(0.5+di, 0.5+dj, 0.5+dk);
                subpos += jlen * (Vec3(1,1,1) - 2.0 * mRand.getVec3());
				if(!is3D) subpos[2] = 0.5; 
                parts.add( BasicParticleData(subpos) );
            }
        }
    }
}

PYTHON void sampleLevelsetWithParticles( LevelsetGrid& phi, FlagGrid& flags, BasicParticleSystem& parts, 
		int discretization, Real randomness ) 
{
	bool is3D = phi.is3D();
    Real jlen = randomness / discretization;
    Vec3 disp (1.0 / discretization, 1.0 / discretization, 1.0/discretization);
    RandomStream mRand(9832);
 
    //clear(); 

    FOR_IJK_BND(phi, 0) {
        if ( flags.isObstacle(i,j,k) ) continue;
        if ( phi(i,j,k) < 1.733 ) {
            Vec3 pos (i,j,k);
            for (int dk=0; dk<(is3D ? discretization : 1); dk++)
            for (int dj=0; dj<discretization; dj++)
            for (int di=0; di<discretization; di++) {
                Vec3 subpos = pos + disp * Vec3(0.5+di, 0.5+dj, 0.5+dk);
                subpos += jlen * (Vec3(1,1,1) - 2.0 * mRand.getVec3());
				if(!is3D) subpos[2] = 0.5; 
				if( phi.getInterpolated(subpos) > 0. ) continue; 
                parts.add( BasicParticleData(subpos) );
            }
        }
    }
}

PYTHON void markFluidCells(BasicParticleSystem& parts, FlagGrid& flags) {
    // remove all fluid cells
    FOR_IJK(flags) {
        if (flags.isFluid(i,j,k)) {
            flags(i,j,k) = (flags(i,j,k) | FlagGrid::TypeEmpty) & ~FlagGrid::TypeFluid;
        }
    }
    
    // mark all particles in flaggrid as fluid
    for(int i=0;i<parts.size();i++) {
    	if (!parts.isActive(i)) continue;
        Vec3i p = toVec3i( parts.getPos(i) );
        if (flags.isInBounds(p) && flags.isEmpty(p))
            flags(p) = (flags(p) | FlagGrid::TypeFluid) & ~FlagGrid::TypeEmpty;
    }
}

// for testing purposes only...
PYTHON void testInitGridWithPos(RealGrid &grid) {
    FOR_IJK(grid) { grid(i,j,k) = norm( Vec3(i,j,k) ); }
}

// compute simple levelset without interpolation (fast, low quality), to be used during simulation
KERNEL(pts, single) 
void ComputeUnionLevelset(BasicParticleSystem& p, LevelsetGrid& phi, Real radius=1.) {
    if (!p.isActive(i)) return;

	const Vec3 pos = p[i].pos - Vec3(0.5); // offset for centered LS value
	//const Vec3i size = phi.getSize();
    const int xi = (int)pos.x, yi = (int)pos.y, zi = (int)pos.z; 

	int r  = int(2. * radius) + 1;
	int rZ = phi.is3D() ? r : 0;
	for(int zj=zi-rZ; zj<=zi+rZ; zj++) 
	for(int yj=yi-r ; yj<=yi+r ; yj++) 
	for(int xj=xi-r ; xj<=xi+r ; xj++) 
	{
		if (! phi.isInBounds(Vec3i(xj,yj,zj)) ) continue;
		phi(xj,yj,zj) = std::min( phi(xj,yj,zj) , fabs( norm(Vec3(xj,yj,zj)-pos) )-radius );
	}
}
PYTHON void unionParticleLevelset (BasicParticleSystem& p, LevelsetGrid& phi, Real radiusFactor=1.) {
	// make sure we cover at least 1 cell by default (1% safety margin)
	Real radius = 0.5 * (phi.is3D() ? sqrt(3.) : sqrt(2.) ) * (radiusFactor+.01);

	// reset
	FOR_IJK(phi) {
		//phi(i,j,k) = radius + VECTOR_EPSILON;
		phi(i,j,k) = 1e10;
	} 
	ComputeUnionLevelset(p, phi, radius);
}

PYTHON void adjustNumber( BasicParticleSystem& parts, MACGrid& vel, FlagGrid& flags, 
		int minParticles, int maxParticles, LevelsetGrid& phi ) 
{
	// which levelset to use as threshold, todo - make depend on particle radius
	const Real SURFACE_LS = -1.5; 
    Grid<int> tmp( vel.getParent() );
	std::ostringstream out;

    // count particles in cells, and delete excess particles
    for (int i=0; i<(int)parts.size(); i++) {
        if (parts.isActive(i)) {
            Vec3i p = toVec3i( parts.getPos(i) );
			if (!tmp.isInBounds(p) ) {
                parts.kill(i); // out of domain, remove
				continue;
			}
            int num = tmp(p);

			bool atSurface = false;
			Real phiv = phi.getInterpolated( parts.getPos(i) );
			if (phiv > SURFACE_LS) atSurface = true;
            
            // dont delete particles in non fluid cells here, the particles are "always right"
            if ( num > maxParticles && (!atSurface) ) {
                parts.kill(i);
			} else {
                tmp(p) = num+1;
			}
        }
    }

    // seed new particles
    RandomStream mRand(9832);
	FOR_IJK(tmp) {
        int cnt = tmp(i,j,k);
		
		// skip cells near surface
		if (phi(i,j,k) > SURFACE_LS) continue;

        if (flags.isFluid(i,j,k) && cnt < minParticles) {
            for (int m=cnt; m < minParticles; m++) { 
                Vec3 pos = Vec3(i,j,k) + mRand.getVec3();
                //Vec3 pos (i + 0.5, j + 0.5, k + 0.5); // cell center
                parts.addBuffered( pos ); 
            }
        }
    }

    parts.doCompress();
	parts.insertBufferedParticles();
}

// simple and slow helper conversion to show contents of int grids like a real grid in the ui
PYTHON void debugIntToReal( Grid<int>& source, Grid<Real>& dest, Real factor=1. )
{
	FOR_IJK( source ) { dest(i,j,k) = (Real)source(i,j,k) * factor; }
}

PYTHON void gridParticleIndex( BasicParticleSystem& parts, ParticleIndexSystem& indexSys, 
		FlagGrid& flags, Grid<int>& index, Grid<int>* counter=NULL) 
{
	bool delCounter = false;
    if(!counter) { counter = new Grid<int>(  flags.getParent() ); delCounter=true; }
	else         { counter->clear(); }
    
    // count particles in cells, and delete excess particles
	index.clear();
	int inactive = 0;
    for (int i=0; i<(int)parts.size(); i++) {
        if (parts.isActive(i)) {
			// check index for validity...
			Vec3i p = toVec3i( parts.getPos(i) );
			if (! index.isInBounds(p)) { inactive++; continue; }

            index(p)++;
        } else {
			inactive++;
		}
    }

	//ParticleIndexSystem* indexSys = new ParticleIndexSystem( parts.getParent() );
	// note - this one might be smaller...
	indexSys.resize( parts.size()-inactive );

	// convert per cell number to continuous index
	int idx=0;
	FOR_IJK( index ) {
		//if( index(i,j,k) ) debMsg("IDDX "<<Vec3i(i,j,k)<<" "<<idx<<"  "<< index(i,j,k) ,1); // debug entries
		int num = index(i,j,k);
		index(i,j,k) = idx;
		idx += num;
	}

	// add particles to indexed array, we still need a per cell particle counter
    for (int i=0; i<(int)parts.size(); i++) {
        if (!parts.isActive(i)) continue;
        Vec3i p = toVec3i( parts.getPos(i) );
		if (! index.isInBounds(p)) { continue; }

		// initialize position and index into original array
		indexSys[ index(p)+(*counter)(p) ].pos        = parts[i].pos;
		indexSys[ index(p)+(*counter)(p) ].otherIndex = i;
		(*counter)(p)++;
    }

	if(delCounter) delete counter;
}

KERNEL
void ComputeUnionLevelset2(BasicParticleSystem& parts, ParticleIndexSystem& indexSys, 
		Grid<int>& index, LevelsetGrid& phi, Real radius=1.) 
{
	const Vec3 gridPos = Vec3(i,j,k) + Vec3(0.5);
	Real phiv = 1e10; 

	int r  = int(radius) + 1;
	int rZ = phi.is3D() ? r : 0;
	for(int zj=k-rZ; zj<=k+rZ; zj++) 
	for(int yj=j-r ; yj<=j+r ; yj++) 
	for(int xj=i-r ; xj<=i+r ; xj++) {
		if ( (!phi.isInBounds(Vec3i(xj-1,yj,zj))) || (!phi.isInBounds(Vec3i(xj,yj,zj))) ) continue;

		for(int p=index(xj-1,yj,zj); p<index(xj,yj,zj); ++p) {
			const Vec3 pos = indexSys[p].pos; // - Vec3(0.5); // offset for centered LS value

			phiv = std::min( phiv , fabs( norm(gridPos-pos) )-radius );
			//debMsg("at "<<Vec3i(xj,yj,zj)<<" "<<p<<" "<<gridPos<<" "<< pos   <<"  "<<phiv ,1);
		}
	}
	phi(i,j,k) = phiv;
}

PYTHON void unionParticleLevelset2( BasicParticleSystem& parts, ParticleIndexSystem& indexSys, 
		FlagGrid& flags, Grid<int>& index, LevelsetGrid& phi, Real radiusFactor=1. ) 
{
	Real radius = 0.5 * (phi.is3D() ? sqrt(3.) : sqrt(2.) ) * (radiusFactor+.01);
	ComputeUnionLevelset2(parts, indexSys, index, phi, radius);
}

// grid interpolation functions

KERNEL(idx) template<class T> 
void knSafeDivReal(Grid<T>& me, const Grid<Real>& other, Real cutoff=VECTOR_EPSILON) { 
	if(other[idx]<cutoff) {
		me[idx] = 0.;
	} else {
		T div( other[idx] );
		me[idx] = safeDivide(me[idx], div ); 
	}
}

// Set velocities on the grid from the particle system

KERNEL(idx)
void knStompVec3PerComponent(Grid<Vec3>& grid, Real threshold) {
	if(grid[idx][0] < threshold) grid[idx][0] = 0.;
	if(grid[idx][1] < threshold) grid[idx][1] = 0.;
	if(grid[idx][2] < threshold) grid[idx][2] = 0.;
}

KERNEL(pts, single) 
void knMapLinearVec3ToMACGrid( BasicParticleSystem& p, FlagGrid& flags, MACGrid& vel, Grid<Vec3>& tmp, 
	ParticleDataImpl<Vec3>& pvel ) 
{
    unusedParameter(flags);
    if (!p.isActive(i)) return;
    vel.setInterpolated( p[i].pos, pvel[i], &tmp[0] );
}

// optionally , this function can use an existing vec3 grid to store the weights
// this is useful in combination with the simple extrapolation function
PYTHON void mapPartsToMAC( FlagGrid& flags, MACGrid& vel , MACGrid& velOld , 
		BasicParticleSystem& parts , ParticleDataImpl<Vec3>& partVel , Grid<Vec3>* weight=NULL ) 
{
    // interpol -> grid. tmpgrid for particle contribution weights
	bool freeTmp = false;
	if(!weight) {
    	weight = new Grid<Vec3>(flags.getParent());
		freeTmp = true;
	} else {
		weight->clear(); // make sure we start with a zero grid!
	}
    vel.clear();
    knMapLinearVec3ToMACGrid( parts, flags, vel, *weight, partVel );

	// stomp small values in weight to zero to prevent roundoff errors
	knStompVec3PerComponent( *weight, VECTOR_EPSILON );
    vel.safeDivide(*weight);
    
    // store original state
    velOld = vel;
	if(freeTmp) delete weight;
}


KERNEL(pts, single) template<class T>
void knMapLinear( BasicParticleSystem& p, FlagGrid& flags, Grid<T>& target, Grid<Real>& gtmp, 
	ParticleDataImpl<T>& psource ) 
{
    unusedParameter(flags);
    if (!p.isActive(i)) return;
    target.setInterpolated( p[i].pos, psource[i], gtmp );
} 
template<class T>
void mapLinearRealHelper( FlagGrid& flags, Grid<T>& target , 
		BasicParticleSystem& parts , ParticleDataImpl<T>& source ) 
{
    Grid<Real> tmp(flags.getParent());
    target.clear();
    knMapLinear<T>( parts, flags, target, tmp, source ); 
	knSafeDivReal<T>( target, tmp );
}

PYTHON void mapPartsToGrid    ( FlagGrid& flags, Grid<Real>& target , BasicParticleSystem& parts , ParticleDataImpl<Real>& source ) {
	mapLinearRealHelper<Real>(flags,target,parts,source);
}
PYTHON void mapPartsToGridVec3( FlagGrid& flags, Grid<Vec3>& target , BasicParticleSystem& parts , ParticleDataImpl<Vec3>& source ) {
	mapLinearRealHelper<Vec3>(flags,target,parts,source);
}
// integers need "max" mode, not yet implemented
//PYTHON void mapPartsToGridInt ( FlagGrid& flags, Grid<int >& target , BasicParticleSystem& parts , ParticleDataImpl<int >& source ) {
//	mapLinearRealHelper<int >(flags,target,parts,source);
//}

KERNEL(pts) template<class T>
void knMapFromGrid( BasicParticleSystem& p, Grid<T>& gsrc, ParticleDataImpl<T>& target ) 
{
    if (!p.isActive(i)) return;
    target[i] = gsrc.getInterpolated( p[i].pos );
} 
PYTHON void mapGridToParts    ( Grid<Real>& source , BasicParticleSystem& parts , ParticleDataImpl<Real>& target ) {
	knMapFromGrid<Real>(parts, source, target);
}
PYTHON void mapGridToPartsVec3( Grid<Vec3>& source , BasicParticleSystem& parts , ParticleDataImpl<Vec3>& target ) {
	knMapFromGrid<Vec3>(parts, source, target);
}


// Get velocities from grid

KERNEL(pts) 
void knMapLinearMACGridToVec3_PIC( BasicParticleSystem& p, FlagGrid& flags, MACGrid& vel, ParticleDataImpl<Vec3>& pvel ) 
{
    if (!p.isActive(i)) return;
	// pure PIC
    pvel[i] = vel.getInterpolated( p[i].pos );
}
PYTHON void mapMACToParts(FlagGrid& flags, MACGrid& vel , 
		BasicParticleSystem& parts , ParticleDataImpl<Vec3>& partVel ) {
    knMapLinearMACGridToVec3_PIC( parts, flags, vel, partVel );
}

// with flip delta interpolation 
KERNEL(pts) 
void knMapLinearMACGridToVec3_FLIP( BasicParticleSystem& p, FlagGrid& flags, MACGrid& vel, MACGrid& oldVel, ParticleDataImpl<Vec3>& pvel , Real flipRatio) 
{
    if (!p.isActive(i)) return; 
    Vec3 v     =        vel.getInterpolated(p[i].pos);
    Vec3 delta = v - oldVel.getInterpolated(p[i].pos); 
    pvel[i] = flipRatio * (pvel[i] + delta) + (1.0 - flipRatio) * v;    
}

PYTHON void flipVelocityUpdate(FlagGrid& flags, MACGrid& vel , MACGrid& velOld , 
		BasicParticleSystem& parts , ParticleDataImpl<Vec3>& partVel , Real flipRatio ) {
    knMapLinearMACGridToVec3_FLIP( parts, flags, vel, velOld, partVel, flipRatio );
}


} // namespace

