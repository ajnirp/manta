#
# 3d pressure solve
# 

import sys
from manta import *
from helperInclude import *

# solver params
res = 64
gs = vec3(res,res,res)
s = Solver(name='main', gridSize = gs, dim=3)
s.timestep = 1.0

# prepare grids
flags    = s.create(FlagGrid)
vel      = s.create(MACGrid)
pressure = s.create(RealGrid)
dummy    = s.create(RealGrid)

flags.initDomain()
flags.fillGrid()

if 0 and (GUI):
	gui = Gui()
	gui.show()
	gui.pause()

velSource = s.create(Box, p0=gs*vec3(0.3,0.4,0.3), p1=gs*vec3(0.7,0.8,0.7) )
    
#main loop
for t in range(1):

	velSource.applyToGrid(grid=vel, value=vec3(1.5, 3, 2.1) )

	setWallBcs(flags=flags, vel=vel) 
	solvePressure(flags=flags, vel=vel, pressure=pressure, openBound='Y')
	setWallBcs(flags=flags, vel=vel)

	s.step()


# check final state
doTestGrid( sys.argv[0], "pressure" , s, pressure , threshold=1e-04, thresholdStrict=1e-10, invertResult=False )

