//ctest - program to test root and C++ elements of code in context, a sort of proof-of-concept code
#include <iostream>
#include <fstream>
#include <string>
/*
#include "TTree.h"
#include "TFile.h"
#include "TGraph.h"

*/
struct main_header_t {
  uint32_t caffe;
  uint32_t readout_version;
  uint32_t firmware_release;
  uint32_t run_number;
  uint32_t timestamp;
  uint32_t staging_size;
  uint32_t run_mode;
  uint32_t filter_mode;
  uint32_t device;
  uint32_t reserved1;
  uint32_t reserved2;
  uint32_t reserved3;
  uint32_t reserved4;
  uint32_t reserved5;
  uint32_t reserved6;
  uint32_t reserved7;
};

struct buffer_header_t {
  uint32_t caffe;
  uint32_t id;
  uint32_t counter;
  uint32_t size;
};

struct spill_t {
  uint32_t coarse   : 15;
  uint32_t rollover : 25;
  uint32_t zero     : 8;
  uint32_t counter  : 12;
  uint32_t id       : 4;
};

struct trigger_t {
  uint32_t coarse   : 15;
  uint32_t rollover : 25;
  uint32_t counter  : 16;
  uint32_t type     : 4;
  uint32_t id       : 4;
};

struct alcor_hit_t {
  uint32_t fine   : 9;
  uint32_t coarse : 15;
  uint32_t tdc    : 2;
  uint32_t pixel  : 3;
  uint32_t column : 3;
  void print() {
    printf(" hit: %d %d %d %d %d \n", column, pixel, tdc, coarse, fine);
  }
};

void ctest(){
  //Reading in the parameters. Temp until the file type is finalized
  double par[8]={1,1,1,1,1,1,1,1};

  
  std::ifstream fin,cin;
  
  fin.open("alcdaq.fifo_4.dat", std::ofstream::in | std::ofstream::binary);
  
  // copy this straight to file
  main_header_t main_header;
  fin.read((char*)&main_header, sizeof(main_header_t));

  // create reading buffer

  
  std::ofstream fout("calibtest1.dat", std::ofstream::out | std::ofstream::binary);
  fout.write((char*)(&main_header),sizeof(main_header_t));

  buffer_header_t buffer_header;
  char *buffer = new char[main_header.staging_size];
   while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    
    std::cout<<buffer_header.size<<std::endl;
    if (fin.eof()) break;
        fout.write((char *)(&buffer_header),sizeof(buffer_header_t));
          fin.read(buffer, buffer_header.size);
          std::cout<<buffer[20]<<std::endl;
    //if (fin.eof()) break;
    //std::cout<<buffer_header.size<<" "<<buffer<<std::endl;
    //std::cout<<fin.gcount()<<std::endl;
       
    if (buffer_header.id < 24) {

    int n = buffer_header.size/4;
    auto word = (uint32_t *)buffer;
    std::cout<<sizeof(word)<<std::endl;
    uint32_t pos = 0;
  // loop over buffer data

  while (pos < n) {
      for(int i=0;i<4;i++){
        const std::uint8_t writing=*word&0xFF;
      fout.write(reinterpret_cast<const char *>(&writing),sizeof(writing));
      *word>>=8;
        //std::cout<<writing<<std::endl;

    }
        
    ++word; ++pos;
  }
    
    }

    else if (buffer_header.id == 24) {
      //To the file it goes it seems, maybe make this more efficient by not needing to read in triggers, only id
      fout.write(buffer,buffer_header.size);
    }
    
  

  }

  fin.close();
  fout.close();
  std::cout<<"End"<<std::endl;

}


