## ALCOR-decoder 

Files pretaining to decoding and calibrating ALCOR ASIC data, part of dRICH component of ePIC. Contains:  
-calib.cpp: Program that takes in a binary (.dat) file of ALCOR data and a parameter (.txt) file; outputs a binary file of ALCOR data with calibrated time  
-ccheck.cpp: Checks a calibrated binary file against a raw ALCOR data file  
-ctest.cpp: auxillary test file  
-crcheck.cpp: checks that two root files produced by the decoder.cc and or cdecoder.cc are identical  
-cdecoder.cc: transforms binary data into a root file, while calibrating the time component and checking it agains the raw calibrated data as provided by calib.cpp  
-data_structs51.h: header file containing all the structs from decoder.cc with some more structs and functions
-parameters.txt: dummy file with numbers that are of the order of actual parameters

Latest 32-bit procedure to check calibrated output
1. With alcdaq.fifo_3.dat and parameters.txt in directory, compile calib.cpp with clang and run  
2. Using ROOT, load and run cdecoder.cc  
3. Once more using ROOT, load and run crcheck.cpp