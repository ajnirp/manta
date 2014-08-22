from manta import *

dim = 3
particleNumber = 2
res = 64
gs = vec3(res,res,res)
if (dim==2):
	gs.z=1
	particleNumber = 3 # use more particles in 2d
s = Solver(name='main', gridSize = gs, dim=dim)
s.timestep = 1.0

flags = s.create(FlagGrid)
vel = s.create(MACGrid)
density = s.create(RealGrid)
pressure = s.create(RealGrid)

flags.initDomain(boundaryWidth=0)