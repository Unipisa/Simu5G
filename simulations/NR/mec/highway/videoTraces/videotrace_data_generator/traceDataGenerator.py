

import sys, getopt
import numpy as np
from scipy.stats import chi


# This script creates video traces data files to be used within the OMNeT++ platform by 
# the RTP implementation.
# The tipycal structure of the file is:
#   - extension: .mpg.gdf
#   - first line: fps    [frames/second] Frame Rate
#   - second line: delay [seconds]   Initial Delay
#   - then: frame_size		[bits]		frame_type
#
#   e.g.
#        30        	[frames/second]	Frame Rate
#        0.08      	[seconds]	Initial Delay
#        46736		[bits]		I-Frame
#        59948		[bits]		B-Frame
#        60012		[bits]		B-Frame
#        59948		[bits]		P-Frame
#
# input arguments:
#   + -f fps
#   + -s frame_size (if it is fixed)
#   + -d initial_delay (in seconds)
#   + -t video duration (in seconds)

outputFileName = ""
frameSize = -1
fps = -1
delay = 0
duration = 0

def readInputArguments(argv):
    global outputFileName, frameSize, fps, delay, duration
    try:
        opts, args = getopt.getopt(argv,"hf:s:d:o:t:",["fps=","fSize=", "delay=", "ofile=", "duration="])
    except getopt.GetoptError:
        print('traceDataGenerator.py -f <fps> -t <duration> -s <frameSize> -d <delay> -o <outputfile>')
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print('traceDataGenerator.py -f <fps> -t <duration> -s <frameSize> -d <delay> -o <outputfile>')
            sys.exit()
        elif opt in ("-f", "--fps"):
            fps = int(arg)
        elif opt in ("-s", "--fSize"):
            frameSize = int(arg)
        elif opt in ("-d", "--delay"):
            delay = float(arg)
        elif opt in ("-o", "--ofile"):
            outputFileName = arg+".mpg.fdg"
        elif opt in ("-t", "--duration"):
            duration = int(arg)
        
    if(fps == -1):
        print("Number of fps argument is mandatory")
        sys.exit()
    elif(outputFileName == ""):
        print("output file name argument is mandatory")
        sys.exit()
    elif(outputFileName == ""):
        print("trace duraton argument is mandatory")
        sys.exit()

    

def generateTraceData():
    f = open(outputFileName, "w")
    # write first line (fps)
    f.write(str(fps)+"\t[frames/second]\tFrame Rate\n")
    # write second line
    f.write(str(delay)+"\t[seconds]\tInitial Delay\n")
    # start writing frames
    frameNumber = duration * fps

    size = 0
    # TODO for now frame types are not used for our experiments
    # it is assumed that:
    # in accoradnce with h264 standard ()
    #   - an I frame is placed every 15-20 frames at the begin of a GOP
    #   - frame dimension I > P > B 

    lIframe = 15
    hIframe = 20
    frameTypes = ["I-Frame", "B-Frame", "P-Frame"]
    # frameTypeProbs = [0.2, 0.5, 0.3]
    ### np.random.choice(frameTypes, 1, frameTypeProbs)[0]
    frameTypeProbs = [0.3, 0.5, 0.2]
    lastIFrame=0
    Icount = 0

    meanI = 90220
    meanP = 66316
    meanB = 21489

    for i in range(frameNumber):
        if(i == 0 or Icount == (i - lastIFrame)):
            frameType = frameTypes[0]
            lastIFrame = i
            Icount = np.random.randint(lIframe, hIframe)
        elif(Icount - 1 == (i - lastIFrame)):
            frameType = frameTypes[2]
        else:
            frameType = np.random.choice(frameTypes[1:], 1, frameTypeProbs[1:])[0]
        if(frameSize != -1):
            size = frameSize
        else:
            if(frameType == "I-Frame"):
                size = np.random.randint(1824,297848)
                # extract the dimension from a random dristibution
            elif(frameType == "P-Frame"):
                size = np.random.randint(296,280744)
            elif(frameType == "B-Frame"):
                size = np.random.randint(288,169392)
        f.write(str(size)+"\t[bits]\t"+frameType+"\n")
    
    f.close


if __name__ == "__main__":
   readInputArguments(sys.argv[1:])
   generateTraceData()








