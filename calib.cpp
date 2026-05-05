//calib - code to take in alcor data and output it with the coarse and fine fields replaced by a single calibrated time
//This code was sponsored by Deus Ex Invisible War soundtrack
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

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

constexpr double rollover = 0x5c5c5c5c;

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
  //depends on tdc, ~every 25 or 40ps
  uint32_t fine   : 9;
  //clock cycle, 3.125ns
  uint32_t coarse : 15;
  uint32_t tdc    : 2;
  uint32_t pixel  : 3;
  uint32_t column : 3;
  void print() {
    printf(" hit: %d %d %d %d %d \n", column, pixel, tdc, coarse, fine);
  }
};

bool in_spill = false;
uint32_t max_calib=0;
uint32_t min_calib=0;
//Function for writing a word to file as 4 chars
//Presumably requires this split to accomodate char casting
void dump(std::ofstream &fout, uint32_t* word){
  //Probably change to a nicer loop
  //Add a separate variable for word in the write function to simplify code in dce
      for(int i=0;i<4;i++){
        const std::uint8_t writing=*word&0xFF;
      fout.write(reinterpret_cast<const char *>(&writing),sizeof(writing));
      *word>>=8;
    }
}
//calibrating the coarse and fine times
//this was written by the Professor
uint32_t corine(double coarse, double* phase){
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
  return (uint32_t)coarse;
}

//dce-decode, calibrate, encode
void dce(std::ofstream &fout, char *buffer, int size, double *par, int tdc)
{
  double a = 1./static_cast<double>(0x1FF);   /// with a = 1/FINEWIDTH and b = 0.5 we don't need calibration....
  double b = 0.5;
  int n = size/4;
  auto word = (uint32_t *)buffer;
  //Maybe try to replace pos with a condition for NULL
  uint32_t pos = 0;
  // loop over buffer data, make this loop better
  while (pos < n) {
    while (!in_spill && pos < n) {
      if ((*word & 0xf0000000) == 0x70000000){
        dump(fout,word);
        ++word; ++pos;
        dump(fout,word);
        in_spill = true;
        ++word; ++pos;
        break;
      }
      dump(fout,word);
      ++word; ++pos;
      }
    // find spill trailer
    while (pos < n) {
                           
      /** killed fifo **/
      if (*word == 0x666caffe) {
                dump(fout,word);
        ++word; ++pos;
        in_spill = false;
	rollover_counter = 0;
        break;	
      }

      /** spill trailer **/
      if ((*word & 0xf0000000) == 0xf0000000) {
                dump(fout,word);
        ++word; ++pos;
        dump(fout,word);
        ++word; ++pos;
        in_spill = false;
	break;
      }
      /** rollover **/
      if (*word == rollover) {
        //std::cout<<"here"<<std::endl;
                dump(fout,word);
        ++word; ++pos;
        continue;
      }
      /** hit **/
      //equation from compact.cc
      double c_hit =  b + ((*word)&0x1FF) * a - 0.5;
      *word=(*word&clear_time)|corine(*word&coarse_mask,&c_hit)|(uint32_t)c_hit;
            dump(fout,word);
      ++word; ++pos;
    }
     

  }
}

int main(/*const std::string infilename, const std::string outfilename*/){
  //Below will be changed eventually
const std::string infilename="alcdaq.fifo_3.dat";
const std::string outfilename="calibtest.dat";
  //Reading in the parameters. Temp until the file type is finalized. Maybe make par global
  double par[8];
  //below temporary, not yet sure how to pass tdc number or if I will even need it
  int tdc=infilename[12];
  std::ifstream fin;
  fin.open(infilename, std::ofstream::in | std::ofstream::binary);
  //Reading in the TDC parameters; temporary in all likelihood
  std::ifstream pin("parameters.txt", std::ofstream::in);
  int i=0;
  while(!pin.eof()) pin>>par[i++];
  pin.close();

  // copy this straight to file
  main_header_t main_header;
  fin.read((char *)&main_header, sizeof(main_header_t));
  // create reading buffer
  char *buffer = new char[main_header.staging_size];

  /** open output file **/
  std::ofstream fout(outfilename, std::ofstream::out | std::ofstream::binary);
  fout.write(reinterpret_cast<char*>(&main_header),sizeof(main_header_t));
  /** loop over data **/
  buffer_header_t buffer_header;
  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    if (fin.eof()) break;
    fout.write(reinterpret_cast<char*>(&buffer_header),sizeof(buffer_header_t));
    fin.read(buffer, buffer_header.size);
    if (buffer_header.id < 24) {
      dce(fout, buffer, buffer_header.size, par, tdc);
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
std::cout<<max_calib<<std::endl;
  std::cout<<min_calib<<std::endl;
 std::cout<<"End"<<std::endl;
}
//There may not be a need in a rollover counter or spill flag

//New tasks:
//modify decoder to accept calibrated time

//warning: signed int will not auto cast to char correctly, do not use
//check for the ranges of the calibrated times, has to be 127 to -255

//Each bit is 32768(rollover cycles)/23^2 =0.0035ns
//decide whether to write calibrated in clock cycles (as is now) or ns
//Decide on how to read in tdc parameters, 64 kbits .dat, do .txt for now

//Start looking at vhdl

//1/320=0.003125ms but
//1/394 is the coarse lsb EIC, make it a const variable
//Fine lsb= coarselsb/0x1FF (0x1FF=512. as const double)