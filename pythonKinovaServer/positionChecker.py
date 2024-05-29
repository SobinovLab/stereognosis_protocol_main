from link6Arm import KinovaArm
import itertools
from log import setLogFileName

f1 = None
f2 = None

def checkPosition(arm, x, y, z, tilt, pitch, yaw):
	ret = arm.inverseKinematics(x, y, z, tilt, pitch, yaw)
	if ret is None:
		return (False, None)
	else:
		return (True, ret)


def outerProductCheck(arm, lateral, depth, height, tilt, pitch, yaw):
	lateral = [j / 1000 for j in lateral]
	depth = [j / 1000 for j in depth]
	height = [j / 1000 for j in height]
	oProd = itertools.product(lateral, depth, height, tilt, pitch, yaw)
	checks = []
	positions = []
	counter = 0
	for i in oProd:
		positions.append(i)
		ret = checkPosition(arm, *i)
		checks.append(ret)
		val = 1 if ret[0] == False else 0
		f1.write("{}, {}, {}, {}, {}, {}, {}\n".format(*i, val))
		counter += 1
		if(counter % 2000 == 0):
			print(counter // 2000, "%")
	return positions, checks

def analyze(arm, pos, check, move=False):
	totFails = 0
	totSuccess = 0
	for i, val in enumerate(check):
		c, y = val
		if c == False:
			print("FAILED TO FIND POS FOR: X:{} Y:{} Z:{} T:{}".format(pos[i][0],
			pos[i][1], pos[i][2], pos[i][3]))
			totFails += 1
			f2.write("{}, {}, {}, {}, {}, {}, {}\n".format(*pos[i], 1))
		else:
			print("SUCCESS FOR: X:{} Y:{} Z:{} T:{}".format(pos[i][0],
			pos[i][1], pos[i][2], pos[i][3]))
			if move:
				ret = arm.referencedMoveArmCartesian(*pos[i])
				print(ret)

				suc = waitForArmStop()
				time.sleep(.5)
				if suc:
					f2.write("{}, {}, {}, {}, {}, {}, {}\n".format(*pos[i], 0))
				else:
					f2.write("{}, {}, {}, {}, {}, {}, {}\n".format(*pos[i], -1))
				ret = arm.moveHome()
				print(ret)
				waitForArmStop()
				time.sleep(.5)

			totSuccess += 1
			#print(y)
	print("TotalFails: ", totFails)
	print("TotalSuccess: ", totSuccess)
	return


lateral = range(245, 381, 5)
depth = range(0, 401, 5)
height = range(-100, 101, 5)
tilt = [0]
pitch = [0]
yaw = [0]
#lateral = [-125]
#depth = [345]
#height = [-45]
#tilt = [-20, 20]
#pitch = [0]
#yaw = [0]

f1 = open("simpleCheck2.csv", "w")
f2 = open("completeCheck.csv", "w")
f1.write("X, Y, Z, Tilt, Pitch, Yaw, Success\n")
f2.write("X, Y, Z, Tilt, Pitch, Yaw, Success\n")

setLogFileName(False)
x = KinovaArm()
x.connectToBase()
x.moveHome()
pos, check = outerProductCheck(x, lateral, depth, height, tilt, pitch, yaw)
f1.close()
print("\n\n ##############################\n Finished With OuterproductCheck \n##############################\n\n")
#analyze(x, pos, check, move=True)
f2.close()
x.disconnectBase()