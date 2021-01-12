import sys
import os
import getopt

def main(argv):
    file_name = "webrtc.log"
    out_file_name = "outdata.txt"
    try:
        opts, args = getopt.getopt(argv,"hi:o:",["input=","output="])
    except getopt.GetoptError:
        print ("parse.py -i <inputfile> -o <outputfile>")
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print ("parse.py -i <inputfile> -o <outputfile>")
            sys.exit()
        elif opt in ("-i", "--input"):
            file_name = arg
        elif opt in ("-o", "--output"):
            out_file_name = arg
    substr = "{\"mediaInfo\":"
    f = open(out_file_name,"a")
    for line in open(file_name):
        start = line.find(substr)
        if start != -1:
            data = line[start:]
            f.write(data)
    f.close()
if __name__ == "__main__":
    main(sys.argv[1:])