from link6Arm import KinovaArm
import itertools


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
	for i in oProd:
		positions.append(i)
		checks.append(checkPosition(arm, *i))
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
		else:
			print("SUCCESS FOR: X:{} Y:{} Z:{} T:{}".format(pos[i][0],
			pos[i][1], pos[i][2], pos[i][3]))
			if move:
				print("Moving there now, Waiting for input to do go again")
				ret = arm.referencedMoveArmCartesian(*pos[i])
				print(ret)
				input()
				print("Finished move, going home now")
				ret = arm.moveHome()
				print(ret)
				input()
				print("Home Achieved, moving on")
			totSuccess += 1
			#print(y)
	print("TotalFails: ", totFails)
	print("TotalSuccess: ", totSuccess)
	return



lateral = [10, 60, 110]
depth = [200]
height = [-50, 0]
tilt = [0]
pitch = [0]
yaw = [0]

x = KinovaArm()
x.connectToBase()
x.moveHome()
pos, check = outerProductCheck(x, lateral, depth, height, tilt, pitch, yaw)
#analyze(x, pos, check)
analyze(x, pos, check, move=True)
x.disconnectBase()