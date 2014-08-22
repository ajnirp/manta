from manta import *

dim = 3
particleNumber = 2
res = 64
gs = vec3(res,res,res)
if (dim==2):
	gs.z=1
	particleNumber = 3 # use more particles in 2d
s = Solver(name='main', gridSize = gs, dim=dim)
s.timestep = 0.5

p = s.create(Tetra)
t = s.create(TetMesh, "resources/vtk/cube.vtk")