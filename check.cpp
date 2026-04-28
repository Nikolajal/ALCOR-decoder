//Porgram to check if two ALCOR .dat files are the same
  #include <iostream>
#include <fstream>
#include <string>

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
  
void ccheck(){
  std::ifstream fin;
  
  fin.open("alcdaq.fifo_3.dat", std::ofstream::in | std::ofstream::binary);
  main_header_t main_header1;
  main_header_t main_header2;
  buffer_header_t buffer_header1;
  buffer_header_t buffer_header2;
  std::ifstream rin("calibtest.dat", std::ofstream::in | std::ofstream::binary);
  fin.read((char *)&main_header1, sizeof(main_header_t));
  rin.read((char *)&main_header2, sizeof(main_header_t));
  if(main_header1.caffe!=main_header2.caffe)std::cout<<"head"<<std::endl;
  char *buffer1 = new char[main_header1.staging_size];
  char *buffer2 = new char[main_header2.staging_size];
  //std::cout<<main_header1.caffe<<std::endl;
  //std::cout<<main_header2.caffe<<std::endl;
  int miss=0;
  while (true) {
    fin.read((char *)(&buffer_header1), sizeof(buffer_header_t));
    rin.read((char *)(&buffer_header2), sizeof(buffer_header_t));
    std::cout<<"BH1: "<<buffer_header1.size<<std::endl;
    std::cout<<"BH2: "<<buffer_header2.size<<std::endl;
    //std::cout<<"here"<<std::endl;
    if (fin.eof()||rin.eof()) break;
    fin.read(buffer1, buffer_header1.size);
    rin.read(buffer2, buffer_header2.size);
    std::cout<<"Bg1: "<<fin.gcount()<<std::endl;
    std::cout<<"Bg2: "<<rin.gcount()<<std::endl;
    for(int i=0;i<buffer_header2.size;i++)
    if(buffer1[i]!=buffer2[i])
    miss++;

  }
std::cout<<"Misses: "<<miss<<std::endl;
std::cout<<buffer1[buffer_header2.size]<<std::endl;
std::cout<<buffer2[buffer_header2.size]<<std::endl;
  fin.close();
  rin.close();
  std::cout<<"End"<<std::endl;

}

