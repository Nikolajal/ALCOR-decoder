//calib - code to take in alcor data and output it with the coarse and fine fields replaced by a single calibrated time
//This code was sponsored by Forget Me Nots by Patrice Rushen
#include <iostream>
#include <fstream>
#include <string>
#include "data_structs51.h"

int n_tdc = 4;
uint32_t clear_time = UINT32_MAX-0xFFFFFF;//FFF0000
constexpr uint32_t coarse_mask = 0x7FFF;
constexpr double rollover = 0x5c5c5c5c;
constexpr uint64_t FIFTY_ONE_BIT_MASK = 0x0007FFFFFFFFFFFF;
constexpr uint32_t THIRTY_TWO_BIT_MASK = 0xFFFFFFFF;

//Function for writing a word to file as 4 chars
void dump(std::fstream &fout, const uint32_t* word){
  //Probably change to a nicer loop
  //copying the writing variable just in case
      std::uint32_t word_copy = *word;
     std::cout<<"Writing "<<std::hex<<word_copy<<std::endl;
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
  return (uint64_t)coarse;
}

//forming a 51-bit hit word and sending to write 
void write_hit(std::fstream &fout, uint32_t* word, uint64_t leadingFine, uint64_t leadingCoarse, uint64_t trailingFine, uint64_t trailingCoarse, int tdc, uint32_t *count){
        trailingCoarse = trailingCoarse > 0x7F ? 0x7F : trailingCoarse;
        //placeholders
        uint64_t K_CODE = 1;
        uint64_t FEB_id = 1;
        //Actual data overflows, so I am using hardcoded coarse values for now
        leadingCoarse=2000;
        trailingCoarse=119;
        uint64_t *hitWord = new uint64_t((FIFTY_ONE_BIT_MASK&((K_CODE<<50)|(FEB_id<<48)|((uint64_t)tdc<<46)
                          |(trailingCoarse<<39)|(trailingFine<<30)|((*word>>26)<<24)|((tdc-2)<<22)
                          |(leadingCoarse<<9)|leadingFine)));
        alcor_hit_51_t *hit1 = (alcor_hit_51_t *)hitWord;
      //std::cout<<"Writing: ";
      hit1->print();
      //std::cout<<std::hex<<((*hitWord))<<std::endl;
        uint32_t half1 = *hitWord&THIRTY_TWO_BIT_MASK;
        uint32_t half2 = *hitWord>>32;
        //This means that the first 19 (32) bits of the word come first in the sequence
        dump(fout,&half1);
        dump(fout,&half2);
        *count+=8;
}


//dce-decode, calibrate, encode
void dce(std::fstream &fout, char *buffer, int size, double *par, uint32_t* count){
  uint32_t posi[2]={0,0};
  uint32_t posf[2]={0,0};
  int rollover_counter = 0;//-1; // we start from -1 because the very first word is a rollover
  bool in_spill = false;
  int n = size / 4;
  auto word = (uint32_t *)buffer;
  //flag to signify that a leading edge has been detected
  bool leadingFlag[2]={0,0};
  uint64_t leadingCoarse[2]={0,0};
  double leadingFine[2]={0,0};
  uint64_t *hitWord;
  //Maybe try to replace pos with a condition for NULL
  uint32_t pos = 0;
  // loop over buffer data, make this loop better
      //std::cout<<pos<<std::endl;
  while (pos < n) {
    while (!in_spill && pos < n) {
      //std::cout<<"Read "<<std::hex<<*word<<std::endl;
      if ((*word & 0xf0000000) == 0x70000000){     
        dump(fout, word);
        ++word; ++pos; *count+=4;
        dump(fout, word);
        in_spill = true;
        ++word; ++pos; *count+=4;
        break;
      }
      dump(fout, word);
      ++word; ++pos; *count+=4;
      }
    // find spill trailer
          std::cout<<"Starting: "<<std::hex<<pos<<std::endl;
    while (pos < n) { 
      std::cout<<std::hex<<pos<<" "<<*word<<std::endl;
     // std::cout<<"Read "<<std::hex<<*word<<std::endl;
      /** killed fifo **/
      if (*word == 0x666caffe) {
                dump(fout, word);
        ++word; ++pos; *count+=4;
        in_spill = false;
	rollover_counter = 0;
        break;	
      }

      /** spill trailer **/
      if ((*word & 0xf0000000) == 0xf0000000) {
                dump(fout, word);
        ++word; ++pos; *count+=4;
        dump(fout, word);
        ++word; ++pos; *count+=4;
        in_spill = false;
	break;
      }
      /** rollover **/
      if (*word == rollover) {
        //std::cout<<"rolling"<<std::endl;
                dump(fout, word);
        ++word; ++pos; *count+=4;
        continue;
      }
      /** hit **/
      int tdc = (*word >> 24) & 0b11;
        double c_hit =  par[tdc+4] + ((*word) & 0x1FF) * par[tdc];
        //dealing with the leading edge
        /*
        if(pos==25){
                    alcor_hit_t *hit;
              hit = (alcor_hit_t *)word;
                      hit->print();
          std::cout<<"fine time "<<((*word) & 0x1ff)<<std::endl;
          std::cout<<"fine time "<<((*word>>9) & coarse_mask)<<std::endl;
                  std::cout<<std::hex<<*word<<std::endl;
        }
                  */
          //std::cout<<"Position: "<<pos<<std::endl;
      if(tdc<2){
                            std::cout<<"TDC number"<<tdc<<std::endl;
        if(leadingFlag[tdc] == true){
          std::cout<<"Leading edge timed out"<<std::endl;
          //std::cout<<tdc<<" "<<std::endl;
          write_hit(fout, word, leadingFine[tdc], leadingCoarse[tdc], 0xff, 0x7f, tdc+2, count);
          //std::cout<<"Leading position for last write: "<<posi[tdc]<<std::endl;
                std::cout<<(std::string(80,'/'))<<std::endl;
        }
        posi[tdc]=pos;
        leadingFine[tdc] = c_hit;
        leadingCoarse[tdc] = corine((*word>>9)&coarse_mask,&leadingFine[tdc]);
        leadingFlag[tdc] = true;

      }
      //dealing with trailing edge; only if there was a previous leading edge
      else if(tdc > 1 && leadingFlag[tdc-2] == true){ 
        std::cout<<"Trailing edge found"<<std::endl;
        //std::cout<<pos<<std::endl;
        posf[tdc-2]=pos;
        //removed subtracting leading coarse time for now
        uint64_t trailingCoarse = (corine((*word>>9)&coarse_mask,&c_hit));//-leadingCoarse[tdc-2];
        write_hit(fout, word, leadingFine[tdc-2], leadingCoarse[tdc-2], c_hit, trailingCoarse, tdc, count);
        leadingFlag[tdc-2]=false;
        //std::cout<<"Leading/trailing positions for last write: "<<posi[tdc-2]<<" "<<pos<<std::endl;
              //std::cout<<(std::string(80,'/'))<<std::endl;
      }
      ++word; ++pos;
    }
  }
}

int main(/*const std::string infilename, const std::string outfilename*/){
  //Below will be changed eventually
const std::string infilename="alcdaq.fifo_3.dat";
const std::string outfilename="calibtest51.dat";
  //Reading in the parameters. Temp until the file type is finalized. Maybe make par global
  double par[8];
  uint32_t* count = new uint32_t(0);
  std::fstream fin;
  fin.open(infilename, std::fstream::in | std::fstream::out | std::ofstream::binary);
  //Reading in the TDC parameters; temporary in all likelihood
  std::ifstream pin("parameters.txt", std::ofstream::in);
  int i=0;
  while (i < 8 && pin >> par[i]) i++;
  pin.close();
  // copy this straight to file
  main_header_t main_header;
  fin.read((char *)&main_header, sizeof(main_header_t));
  // create reading buffer
  char *buffer = new char[main_header.staging_size];

  /** open output file **/
  std::fstream fout(outfilename, std::fstream::out | std::fstream::out | std::ofstream::binary);
  fout.write(reinterpret_cast<char*>(&main_header), sizeof(main_header_t));
  /** loop over data **/
  buffer_header_t buffer_header;
  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    if (fin.eof()) break;
    int buffer_start = fout.tellp();
    fout.write(reinterpret_cast<char*>(&buffer_header), sizeof(buffer_header_t));
    //std::cout<<(std::string(80,'/'))<<std::endl;
    //std::cout<<std::hex<<buffer_header.size<<std::endl;
    //std::cout<<sizeof(buffer_header)<<std::endl;
    fin.read(buffer, buffer_header.size);
    if (buffer_header.id < 24) {
      dce(fout, buffer, buffer_header.size, par, count);
      std::cout<<"cycle"<<std::endl;
      //This is to rewrite the buffer header size to fit the new one should the sizes change
      int buffer_end = fout.tellp();
      fout.seekp(buffer_start+12);//+12
      std::cout<<std::dec<<*count<<" "<<buffer_header.size<<std::endl;
      dump(fout, count);
        //std::cout<<(uint64_t*)buffer_temp<<std::endl;
      fout.seekp(buffer_end);
     *count=0;
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
//read/write pointers are a bit clumsy
//do i match trail/edge over rollovers 
//do i necessarily write the new word at the trailing edge word position?
//check that the ordering of the 2 halves of the hit word are correct
//maybe investigate why pos, posf, and posi are being treated as hex. This is not (yet) the case in cdecoder51
//check if only warp is skipping cout streams or if terminal does it too