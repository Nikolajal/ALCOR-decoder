//calib - code to take in alcor data and output it with the coarse and fine fields replaced by a single calibrated time
//This code was sponsored by Deus Ex Invisible War soundtrack
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

int rollover_counter = 0;//-1; // we start from -1 because the very first word is a rollover
int n_tdc =4;
#define UINT32_MAX  (0xffffffff)
//mask names a misnomer; they are masks for fine/coarse and everything upwards of 24th bit
uint32_t coarse_mask=UINT32_MAX-511;
uint32_t fine_mask=UINT32_MAX-pow(2.0,24)+510;

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

//dce-decode, calibrate, encode
void dce(std::ofstream &fout, char *buffer, int size, double *par, int tdc)
{
  int n = size/4;
  auto word = (uint32_t *)buffer;
  //Maybe try to replace pos with a condition for NULL
  uint32_t pos = 0;
  //integer value for correction of coarse
  double ccorrection=0;
  // loop over buffer data, make this loop better
  while (pos < n) {
    // find spill header if not in spill already
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
      if (*word == 0x5c5c5c5c) {
        //std::cout<<"here"<<std::endl;
                dump(fout,word);
        ++word; ++pos;
        continue;
      }
      /** hit **/
      //alcor_hit_t *hit = (alcor_hit_t *)word;
      //figure out if its &hit or just hit below
      //fout.write(reinterpret_cast<char*>(&hit),sizeof(alcor_hit_t));
      std::cout<<*word<<std::endl;
      //equation from decoder
      double c_hit = ((*word>>9)&0b111111111111111) - par[tdc] + ((*word)&0b111111111) * par[tdc+4];
      if(c_hit>max_calib)max_calib=c_hit;
      if(c_hit<min_calib)min_calib=c_hit;
      //Logic for the assignments below; convert TDC fine time to ns; get coarse and fine corrections as modulo and int;
      //get calibrated TDC fine time back without the integer correction and overwrite fine time; apply coarse correction with calibrated time whole numbers
      //in clock cycle time. The 9 bits of the fine time are intended to be in two's complement
      //Assuming 24-bits (9 for signed calibrated time, 15 for coarse) for the entire calibrated time, comment this out to simply copy files
      *word=((*word&coarse_mask)|fine_mask&(uint32_t)(round(modf(c_hit*0.0035,&ccorrection)/0.0035)));
      *word=((*word&fine_mask)|((*word&coarse_mask>>9)+(int)(ccorrection/3.125))<<9);
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
  std::cout<<"Here"<<std::endl;
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
//Get possible ranges of TDC parameters

//New tasks:
//Add sign bit or signed binary
//modify decoder to accept calibrated time
//From now on, fine will only take up 9 signed bits: if overrunning (i.e. 1.2), increment or decrement coarse counter,
// coarse time cannot go to zero!

//warning: signed int will not auto cast to char correctly, do not use
//check for the ranges of the calibrated times, has to be 127 to -255

//bins for the calibrated time in clock cycles are 0.002 wide, so have to remove msb from calibrated time if decimal, increment coarse and round up or down the lsb to 0.002

//Each bit is 32768(rollover cycles)/23^2 =0.0035ns
//decide whether to write calibrated in clock cycles (as is now) or ns
//Decide on how to read in tdc parameters, 64 kbits .dat, do .txt for now
//Check maximum for calibrated time in practice

//Start looking at vhdl

//If data goes below the first rollover with coarse=0, consider it to be coarse 0 and calibrated=0