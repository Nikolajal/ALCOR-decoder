//calib - code to take in alcor data and output it with the coarse and fine fields replaced by a single calibrated time
//This code was sponsored by Deus Ex Invisible War soundtrack
#include <iostream>
#include <fstream>
#include <string>
#include "data_structs.h"

int n_tdc = 4;
uint32_t clear_time = UINT32_MAX-0xFFFFFF;//FFF0000
constexpr uint32_t coarse_mask = 0x7FFF;
constexpr double rollover = 0x5c5c5c5c;

//Function for writing a word to file as 4 chars
void dump(std::ofstream &fout, const uint32_t* word){
  //Probably change to a nicer loop
  //copying the writing variable just in case
      std::uint32_t word_copy = *word;
      for(int i=0;i<4;i++){
        const std::uint8_t writing = word_copy&0xFF;
      fout.write(reinterpret_cast<const char *>(&writing),sizeof(writing));
      word_copy >>= 8;
    }
}
//calibrating the coarse and fine times
//this was written by the Professor
uint32_t corine(uint32_t coarse, double* phase){
  //std::cout<<*phase<<std::endl;
  if (*phase < 0.){
    if (coarse != 0){
      coarse--;
      *phase += 1.;
    }
    else{
      *phase = 0.;
    }
  } else if (*phase > 1.){
    if (coarse != 0x7FFF){  // this is to avoid a roll-over/move to orbit+1 due to fine calibration
      coarse++;
      *phase -= 1.;
    }
    else{
      *phase = 1.;
    }
  }
  *phase=std::llround(*phase * 511.0);
  return coarse << 9;
}

//dce-decode, calibrate, encode
void dce(std::ofstream &fout, char *buffer, int size, double *par){
  int rollover_counter = 0;//-1; // we start from -1 because the very first word is a rollover
  bool in_spill = false;
  int n = size / 4;
  auto word = (uint32_t *)buffer;
  //Maybe try to replace pos with a condition for NULL
  uint32_t pos = 0;
  // loop over buffer data, make this loop better
  while (pos < n) {
    while (!in_spill && pos < n) {
      if ((*word & 0xf0000000) == 0x70000000){
        dump(fout, word);
        ++word; ++pos;
        dump(fout, word);
        in_spill = true;
        ++word; ++pos;
        break;
      }
      dump(fout, word);
      ++word; ++pos;
      }
    // find spill trailer
    while (pos < n) {
                           
      /** killed fifo **/
      if (*word == 0x666caffe) {
                dump(fout, word);
        ++word; ++pos;
        in_spill = false;
	rollover_counter = 0;
        break;	
      }

      /** spill trailer **/
      if ((*word & 0xf0000000) == 0xf0000000) {
                dump(fout, word);
        ++word; ++pos;
        dump(fout, word);
        ++word; ++pos;
        in_spill = false;
	break;
      }
      /** rollover **/
      if (*word == rollover) {
        //std::cout<<"here"<<std::endl;
                dump(fout, word);
        ++word; ++pos;
        continue;
      }
      /** hit **/
      int tdc = (*word >> 24) & 0b11;
        double c_hit =  par[tdc+4] + ((*word) & 0x1FF) * par[tdc];

            alcor_hit_t *hit1 = (alcor_hit_t *)word;
            std::cout<<"Reading in: ";
      //hit1->print();
      //std::cout<<c_hit<<std::endl;
      //decomposed to 3 lines to avoid possible compiler issues
      uint32_t time_part = corine((*word >> 9)&coarse_mask,&c_hit) | (uint32_t)c_hit;
      *word=(*word & clear_time) | time_part;
      std::cout<<std::hex<<*word<<std::endl;
      //uint32_t corsica=corine((*word>>9)&coarse_mask,&c_hit);
      //auto word1=(*word&clear_time)|corsica;
      //*word=(*word&clear_time);
      //std::cout<<std::hex<<*word<<std::endl;
      //std::cout<<std::hex<<word1<<std::endl;
      //*word=*word|corine((*word>>9)&coarse_mask,&c_hit);
      //std::cout<<std::hex<<*word<<std::endl;
      //*word=*word|(uint32_t)c_hit;
      //break;
      alcor_hit_t *hit2 = (alcor_hit_t *)word;
      //std::cout<<"Calibrated: ";
     // hit2->print();
     // std::cout<<std::hex<<*word<<std::endl;
      //std::cout<<std::string(80,'/')<<std::endl;
            dump(fout, word);
      ++word; ++pos;
    }
     
//break;
  }
}

int main(/*const std::string infilename, const std::string outfilename*/){
  //Below will be changed eventually
const std::string infilename="alcdaq.fifo_3.dat";
const std::string outfilename="calibtest.dat";
  //Reading in the parameters. Temp until the file type is finalized. Maybe make par global
  double par[8];
  std::ifstream fin;
  fin.open(infilename, std::ofstream::in | std::ofstream::binary);
  //Reading in the TDC parameters; temporary in all likelihood
  std::ifstream pin("parameters.txt", std::ofstream::in);
  int i=0;
  while(!pin.eof() || i<8) pin>>par[i++];
  pin.close();
  // copy this straight to file
  main_header_t main_header;
  fin.read((char *)&main_header, sizeof(main_header_t));
  // create reading buffer
  char *buffer = new char[main_header.staging_size];

  /** open output file **/
  std::ofstream fout(outfilename, std::ofstream::out | std::ofstream::binary);
  fout.write(reinterpret_cast<char*>(&main_header), sizeof(main_header_t));
  /** loop over data **/
  buffer_header_t buffer_header;
  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    if (fin.eof()) break;
    fout.write(reinterpret_cast<char*>(&buffer_header), sizeof(buffer_header_t));
    fin.read(buffer, buffer_header.size);
    if (buffer_header.id < 24) {
      dce(fout, buffer, buffer_header.size, par);
      //break;
    }
    else if (buffer_header.id == 24) {
      //To the file it goes it seems, maybe make this more efficient by not needing to read in triggers, only id
      fout.write(buffer,buffer_header.size);
    }
    //
  }
  /** close input file **/
  fin.close();
  fout.close();
 std::cout<<"End"<<std::endl;
}
//There may not be a need in a rollover counter or spill flag

//Each bit is 32768(rollover cycles)/23^2 =0.0035ns
//Decide on how to read in tdc parameters, 64 kbits .dat, do .txt for now

//Start looking at vhdl

//1/320=0.003125ms but
//1/394 is the coarse lsb EIC, make it a const variable
//Fine lsb= coarselsb/0x1FF (0x1FF=512. as const double)

//edit compact.cc to test if what goes in comes out the same

//make a copy of word in dump
//check if coarse in corine needs to be double, could be errors

