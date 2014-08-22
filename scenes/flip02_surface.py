#
# Simple flip with level set and basic resampling
# 
from manta import *

# solver params
dim = 2
res = 64
gs = vec3(res,res,res)
if (dim==2):
	gs.z=1
s = Solver(name='main', gridSize = gs, dim=dim)
s.timestep = 0.5
minParticles = pow(2,dim)

# prepare grids and particles
flags    = s.create(FlagGrid)
phi      = s.create(LevelsetGrid)

vel      = s.create(MACGrid)
velOld   = s.create(MACGrid)
pressure = s.create(RealGrid)
tmpVec3  = s.create(VecGrid)
tstGrid  = s.create(RealGrid)

pp       = s.create(BasicParticleSystem) 
pVel     = pp.create(PdataVec3) 
# test real value, not necessary for simulation
pTest    = pp.create(PdataReal) 
mesh     = s.create(Mesh)

# scene setup
flags.initDomain(boundaryWidth=0)
# enable one of the following
fluidbox = s.create(Box, p0=gs*vec3(0,0,0), p1=gs*vec3(0.4,0.6,1)) # breaking dam
#fluidbox = s.create(Box, p0=gs*vec3(0.4,0.72,0.4), p1=gs*vec3(0.6,0.92,0.6)) # centered falling block
phi = fluidbox.computeLevelset()
flags.updateFromLevelset(phi)

sampleLevelsetWithParticles( phi=phi, flags=flags, parts=pp, discretization=2, randomness=0.2 )

# testing the real channel while resampling - original particles
# will have a value of 0.1, new particle will get a value from the tstGrid
testInitGridWithPos(tstGrid)
setConstPdata( pTest , 0.1 )
    
if (GUI):
    gui = Gui()
    gui.show()
    #gui.pause()
    
#main loop
for t in range(2500):
    
    # FLIP 
    pp.advectInGrid(flags=flags, vel=vel, integrationMode=IntRK4, deleteInObstacle=False )

    # make sure we have velocities throught liquid region
    mapPartsToMAC(vel=vel, flags=flags, velOld=velOld, parts=pp, partVel=pVel, weight=tmpVec3 ) 
    extrapolateMACFromWeight( vel=vel , distance=2, weight=tmpVec3 ) 
    markFluidCells( parts=pp, flags=flags )

	# create approximate surface level set, resample particles
    # note - this is slow right now, and could be optimized by only computing a narrow band
    unionParticleLevelset( pp, phi )
    phi.reinitMarching(flags=flags, maxTime=2 )
    pVel.setSource( vel, isMAC=True )
    pTest.setSource( tstGrid );
    adjustNumber( parts=pp, vel=vel, flags=flags, minParticles=1*minParticles, maxParticles=2*minParticles, phi=phi ) 

	# forces & pressure solve
    addGravity(flags=flags, vel=vel, gravity=(0,-0.001,0))
    setWallBcs(flags=flags, vel=vel)    
    solvePressure(flags=flags, vel=vel, pressure=pressure)
    setWallBcs(flags=flags, vel=vel)

    # make sure we have proper velocities
    extrapolateMACSimple( flags=flags, vel=vel )
    
    flipVelocityUpdate(vel=vel, velOld=velOld, flags=flags, parts=pp, partVel=pVel, flipRatio=0.97 )

    if (dim==3):
        phi.createMesh(mesh)
    
    #gui.screenshot( 'flipt_%04d.png' % t );
    #s.printMemInfo()
    #pp.save( 'parts_%04d.txt' % t );
    s.step()

