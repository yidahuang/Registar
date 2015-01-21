#!/usr/bin/python

import glob;
import os;

command = "/home/yiztang/Downloads/software/MeshLab/MeshLabSrc_AllInc_v133/meshlab/src/distrib/meshlabserver"; 
output_directory = "ply";

filenamelist = glob.glob("*.obj");
filenamelist.sort();

for filename in filenamelist:
	newfilename = filename.replace(".obj",".ply");
	os.system( command + " -i " + filename + " -o " + output_directory + "/" + newfilename );
#	print newfilename;
