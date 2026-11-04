## ALCOR-decoder 

Files pretaining to decoding and calibrating ALCOR ASIC data, part of dRICH component of ePIC. Contains:  
-calib.cpp: Program that takes in a binary (.dat) file of ALCOR data and a parameter (.txt) file; outputs a binary file of ALCOR data with calibrated time  
-ccheck.cpp: Checks a calibrated binary file against a raw ALCOR data file  
-ctest.cpp: auxillary test file  
-crcheck.cpp: checks that two root files produced by the decoder.cc and or cdecoder.cc are identical  
-cdecoder.cc: transforms binary data into a root file, while calibrating the time component and checking it agains the raw calibrated data as provided by calib.cpp  
-calib51.cpp: takes in a binary (.dat) file with 32-bit hit words, calibrates, and matches them as leading and trailing edge signals into 51-bit words into a binary file.
-cdecoder51.cc: takes in binary files (.dat) of 51-bit and 32-bit hit words, calibrates and matches 32-bit words to form 51-bit hitwords and ensures that they appear in the 51-bit hit word binary file. Also outputs a ROOT file.
-calib51Q.cpp: takes in a binary (.dat) file with 32-bit hit words, calibrates, and matches them as leading and trailing edge signals into 51-bit words into a binary file. This program uses multiprogramming to read and write at the same time, but since this has proven to be uneccessary, it doesn't work and no further updates will be done to it.
-calibCSV.cpp: takes in a CSV file with 32-bit hit words, ~~calibrates~~ (disabled for testing), and matches them as leading and trailing edge signals into 51-bit words into a CSV file
-CSVcheck.cpp: takes in CSV files with 51-bit and 32-bit hit words and checks that the 32-bit hit words decoded from 51-bit hit words appear in the 32-bit hit word CSV file and in the right order
-data_structs51.h: header file containing all the structs from decoder.cc with some more structs and functions
-parameters.txt: dummy file with numbers that are of the order of actual parameters

Latest 32-bit procedure to check calibrated output
1. With alcdaq.fifo_3.dat and parameters.txt in directory, compile calib.cpp with clang and run  
2. Using ROOT, load and run cdecoder.cc  
3. Once more using ROOT, load and run crcheck.cpp

Note that testing with ROOT files is hardly definitive.

Latest 51-bit procedure to check calibrated output is pending, but uses cdecoder51.cc

Latest CSV procedure to check output (matched only)
1. With IF_64/SR_DC1500kHz_PHY3kHz_shutter7.5ns/output_Col3_notsorted.csv, data_structs51.h, and parameters.txt in the directory, compile and run calibCSV.cpp
2. Using clang, compile and run CSVcheck.cpp. If it shows misses as 0, then the output passes. Else, it will stop as soon as a 32-bit word corresponding to one or two of the words making up the 51-bit word is not found in the original 32-bit CSV file

This is probably not a definitive test, but as close to one as is reasonable. With entries in the hundreds of thousands, testing shows that even erroneous words can be sometimes found much later in the 32-bit CSV file.
