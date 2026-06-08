//calib - code to take in alcor data and output it with the coarse and fine fields replaced by a single calibrated time
//This code was sponsored by Starship Troopers soundtrack
#include <iostream>
#include <fstream>
#include <string>
//#include <pthread.h>
#include <thread>
#include "data_structs51.h"

int rollover_counter = 0;//-1; // we start from -1 because the very first word is a rollover
int n_tdc =4;
//#define UINT32_MAX  (0xffffffff)
uint32_t clear_time=UINT32_MAX-0xFFFFFF;//FFF0000
uint32_t fine_mask=0x1FF;//FFF0000
constexpr uint32_t coarse_mask=0x7FFF;
constexpr double EICCLK = 394.0;  // MHz
constexpr double coarse_clock=3.125;
constexpr double fine_binning=coarse_clock/511.;
constexpr double FWF = static_cast<double>(0x1FF);
constexpr double COARSE_LSB_PS = 1.E+6/EICCLK;
constexpr double FINE_LSB_PS = COARSE_LSB_PS/static_cast<double>(0x1FF);
constexpr uint64_t FIFTY_ONE_BIT_MASK = 0x7FFFFFFFFFFFF;
constexpr uint8_t WRITING_BUFFER_SIZE = 64;
constexpr double rollover = 0x5c5c5c5c;


//To synchronize operations on the writing buffer
std::mutex buffering;
std::mutex checking_eob;
bool in_spill = false;

//function to write whatever is in the writing_buffer to file
void dump(std::ofstream &fout, uint64_t* writing_buffer, uint32_t* count, bool* eob){
  checking_eob.lock();
  while(!*eob) {
    checking_eob.unlock();

    //this has to be in a separate loop as behaviour unclear if it would keep writing if it was only in 1 loop
    buffering.lock();

    while(*count>=8) {
      buffering.unlock();
      const std::uint8_t writing=*writing_buffer&0xFF;
      fout.write(reinterpret_cast<const char *>(&writing),sizeof(writing));
      buffering.lock();
      *writing_buffer >>= 8;
      *count -= 8;
      buffering.unlock();
    }
    buffering.unlock();
    checking_eob.lock();
  }
  checking_eob.unlock();
  if(*count!=0) fout.write(reinterpret_cast<const char *>(*writing_buffer&0xFF),sizeof(*writing_buffer&0xFF));
 
}


//function to add word/hit to the writing buffer
template <typename T> void slate(T* word, uint64_t* writing_buffer, uint32_t* count, const int bits){
  //std::cout<<"ending"<<std::endl;
  int ibit = 0;
  //std::cout<<std::hex<<*word<<std::endl;
  auto word_copy = *word;
             // std::cout<<std::hex<<word_copy<<std::endl;
  //this assumes that if unable to write cause buffer is full, then will skip and unlock
  while(ibit < bits) {
    buffering.lock();
    //there should not be any random stuff from bit-shifting word I think, should check
    *writing_buffer = *writing_buffer | (word_copy<<*count);
    //below will count number of bits written in the event that the number of bits just written is less than available buffer space
            //  std::cout<<"here"<<std::endl;
    int bits_slated = (WRITING_BUFFER_SIZE - *count > bits - ibit) ? bits - ibit : WRITING_BUFFER_SIZE - *count;
    *count += bits_slated;
    buffering.unlock();
    ibit += bits_slated;
    word_copy >>= bits_slated;
  }
}


//calibrating the coarse and fine times
//this was written by the Professor
uint32_t corine(uint32_t coarse, double* phase){
  //std::cout<<*phase<<std::endl;
  if (*phase < 0.) {
    if (coarse!=0) {
      coarse--;
      *phase += 1.;
    } else {
      *phase = 0.;
    }
  } else if (*phase > 1.) {
    if (coarse != 0x7FFF) {  // this is to avoid a roll-over/move to orbit+1 due to fine calibration
      coarse++;
      *phase -= 1.;
    } else {
      *phase = 1.;
    }
  }
  *phase=std::llround(*phase * 511.0);
  return (uint64_t)coarse;
}

//dce-decode, calibrate, encode
void dce(char *buffer, int size, double *par, uint64_t* writing_buffer, uint32_t* count, int* buffer_bits)
{
  bool leadingFlag[2]={0,0};
  uint64_t leadingCoarse[2]={0,0};
  double leadingFine[2]={0,0};
  int n = size/4;
  uint64_t *hitWord;
  auto word = (uint32_t *)buffer;
  //Maybe try to replace pos with a condition for NULL
  uint32_t pos = 0;
  // loop over buffer data, make this loop better 
  while (pos < n) {
    //std::cout<<*word<<std::endl;
    while (!in_spill && pos < n) {
      if ((*word & 0xf0000000) == 0x70000000){
        slate(word, writing_buffer, count, 32);
        ++word; ++pos;
        slate(word, writing_buffer, count, 32);
        in_spill = true;
        ++word; ++pos;
        buffer_bits+=64;
        break;
      }
      slate(word, writing_buffer, count, 32);
      buffer_bits+=32;
      ++word; ++pos;
      }
    // find spill trailer
    while (pos < n) {
      //std::cout<<std::hex<<*word<<std::endl;
      /** killed fifo **/
      if (*word == 0x666caffe) {
        slate(word, writing_buffer, count, 32);
        buffer_bits+=32;
        ++word; ++pos;
        in_spill = false;
	rollover_counter = 0;
        break;	
      }
      /** spill trailer **/
      if ((*word & 0xf0000000) == 0xf0000000) {
        //std::cout<<"trailer"<<std::endl;
        slate(word, writing_buffer, count, 32);
        ++word; ++pos;
        slate(word, writing_buffer, count, 32);
        ++word; ++pos;
        buffer_bits+=64;
        in_spill = false;
	break;
      }
      /** rollover **/
      if (*word == rollover) {
        //std::cout<<"roll"<<std::endl;
        slate(word, writing_buffer, count, 32);
        buffer_bits+=32;
        ++word; ++pos;
        continue;
      }
      /** hit **/
      int tdc=(*word>>24)&0b11;
      double c_hit =  par[tdc+4] + ((*word) & 0x1FF) * par[tdc];
      //not checking for flag, assuming previous leading edge signal timed out
      //std::cout<<std::hex<<*word<<std::endl;
      if(tdc<2){
        leadingFine[tdc] = c_hit;
        leadingCoarse[tdc] = corine((*word>>9)&coarse_mask,&leadingFine[tdc]);
        leadingFlag[tdc] = true;
        ++word; ++pos;
        continue;
      }
      uint64_t trailingCoarse = (corine((*word>>9)&coarse_mask,&c_hit)) - leadingCoarse[tdc-2];
      //checking if trailing edge is not too late
                    //std::cout<<"here"<<std::endl;
      if(trailingCoarse>0x7F){
        leadingFlag[tdc-2] = false;
      }
      //trailing edge, only reading in if there is a previous leading edge
      else if(tdc > 1 && leadingFlag[tdc-2] == 1){
        uint64_t K_CODE=1;
        uint64_t FEB_id=2;
        *hitWord = (FIFTY_ONE_BIT_MASK&((K_CODE<<51)|(FEB_id<<48)|((uint64_t)tdc<<46)
                          |(trailingCoarse<<39)|((uint64_t)c_hit<<30)|((uint64_t)(*word>>26)<<24)|((uint64_t)(tdc-2)<<22)
                          |(leadingCoarse[tdc-2]<<9)|(uint64_t)leadingFine[tdc-2]));
        leadingFlag[tdc-2] = false;
              //std::cout<<std::hex<<*hitWord<<std::endl;
              //std::cout<<"64bit"<<std::endl;
        slate(hitWord, writing_buffer, count, 51);
      alcor_hit_51_t *hit1 = (alcor_hit_51_t *)hitWord;
     // std::cout<<"Reading in: ";
     std::cout<<(tdc-2)<<std::endl;
      hit1->print();
            //break;
     // std::cout<<c_hit<<std::endl;
     // std::cout<<"Calibrated: ";
      //std::cout<<std::hex<<*hitWord<<std::endl;
      //std::cout<<(std::string(80,'/'))<<std::endl;
        buffer_bits+=51;
      }
      ++word; ++pos;
    }
//break;
  }
}

int main(/*const std::string infilename, const std::string outfilename*/){
  //Below will be changed eventually
const std::string infilename="alcdaq.fifo_3.dat";
const std::string outfilename="calibtest51Q.dat";
  //Reading in the parameters. Temp until the file type is finalized. Maybe make par global
  double par[8];
  std::ifstream fin;
  fin.open(infilename, std::ofstream::in | std::ofstream::binary);
  std::ofstream fout(outfilename, std::ofstream::out | std::ofstream::binary);
  //Reading in the TDC parameters; temporary in all likelihood
  std::ifstream pin("parameters.txt", std::ofstream::in);
  int i=0;
  while(!pin.eof()||i<8) pin>>par[i++];
  pin.close();

  // copy this straight to file
  main_header_t main_header;
  fin.read((char *)&main_header, sizeof(main_header_t));

  // create reading buffer
  char *buffer = new char[main_header.staging_size];
  uint64_t* writing_buffer = new uint64_t(0);
  uint32_t* count = new uint32_t(0);
  bool* eob =new bool();
  int *buffer_bits = 0;
  //thread has some syntax considerations that are very not obvious
  std::thread t1(&dump, std::ref(fout), writing_buffer, count, eob);
  /** open output file **/
  fout.write(reinterpret_cast<char*>(&main_header),sizeof(main_header_t));
  /** loop over data **/
  buffer_header_t buffer_header;
  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    if (fin.eof()) break;
    int buffer_start = fout.tellp();
    fin.read(buffer, buffer_header.size);
    fout.write(reinterpret_cast<char*>(&buffer_header),sizeof(buffer_header_t));
    if (buffer_header.id < 24) {
            std::cout<<"here"<<std::endl;
      dce(buffer, buffer_header.size, par, writing_buffer, count, buffer_bits);
      
      int buffer_end = fout.tellp();
      fout.seekp(buffer_start+12);
      slate(count, writing_buffer, count, 32);
      //may need a mutex here to not move pointer before writing
      fout.seekp(buffer_end);
      *count = 0;
      
    }
    else if (buffer_header.id == 24) {
      //To the file it goes it seems, maybe make this more efficient by not needing to read in triggers, only id
      fout.write(buffer,buffer_header.size);
    }
    //
  }
          std::cout<<"out"<<std::endl;
  checking_eob.lock();
  *eob=true;
  checking_eob.unlock();
  t1.join();
  /** close input file **/
  fin.close();
  fout.close();
  delete[] buffer;
  delete writing_buffer;
 std::cout<<"End"<<std::endl;
}
//There may not be a need in a rollover counter or spill flag

//New tasks:
//modify decoder to accept calibrated time

//Start looking at vhdl

//1/320=0.003125ms but
//1/394 is the coarse lsb EIC, make it a const variable
//Fine lsb= coarselsb/0x1FF (0x1FF=512. as const double)

//adapt to 51 bit word
//pairs are (leading-trailing) TDC id 0-2, 1-3

//figure out what to do about buffer size (can ignore)
//and staging size (ignore)
//clarify, that only hit words are 51-bit (confirmed)

//make a header file for all structs
//make rollover and inspill local
//remove idea and other clion/vsc files from github
//unify formatiting (indents, spaces etc.)
//buffer has to be at least for 2 hit words (make it 32 bytes)
//consider using deque for buffer
//use the fact that there is nothing between spill header and spill footer.
//use multiprogramming for write

//Consider performance:
//Big or small buffer size--> possibly more function calls
//Maybe also send headers to be written by the threaded function
//Maybe even have dce be run in a thread
//Make sure as little tramp data as possible exists
//Remember to deal with buffer header.size
//could paralellize reading different buffer header blocks
//I abandoned using threads as opposed to POSIX threads, but maybe they are better?
//Is there any need to not open the writing stream from within the thread

//If trailing edge times out or comes late, write maximum time in trailing time