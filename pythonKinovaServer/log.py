from datetime import datetime

filepath = "./logs/"
filename = ""

def setLogFileName(name = None):
    global filename
    if(name is None):
        now = datetime.now()
        name = now.strftime("%d-%m-%Y;;%H.%M.%S") + ".txt"
    filename = filepath + name
    f = open(filename, 'w')
    return


def printLog(*args):
    print(args)
    f = open(filename, 'a')
    for i in args:
        f.write(i)
    f.write("\n")
    f.close
    return