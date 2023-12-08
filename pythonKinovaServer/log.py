from datetime import datetime
from pathlib import Path

filepath = ""
filename = ""

def setLogFileName(name = None):
    global filename
    global filepath
    if name == False:
        filename = None
        return
    filepath = str(Path(__file__).parent)
    if(name is None):
        now = datetime.now()
        name = now.strftime("%d-%m-%Y;;%H_%M_%S") + ".txt"
    filename = filepath + "/logs/" + name
    f = open(filename, 'w+')
    return


def printLog(*args):

    print(args)
    if filename is None:
        return
    f = open(filename, 'a')
    for i in args:
        f.write(str(i))
    f.write("\n")
    f.close
    return