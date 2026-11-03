// calibCSV - code to take in alcor CSV data and to match hits
// This code was sponsored by Marechia - Nu Genea
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <cstdint>
// #include <bit>
// cmath may be needed on certain compilers for std::llround apparently, works on my mac tho
#include <cmath>
// remove data_structs before final delivery: they are only really needed for testing
#include "data_structs51.h"
constexpr uint32_t COARSE_MASK = 0x7FFF;
constexpr uint64_t FIFTY_ONE_BIT_MASK = 0x0007FFFFFFFFFFFF;
constexpr uint32_t THIRTY_TWO_BIT_MASK = 0xFFFFFFFF;

// calibrating the coarse and fine times
// this was written by the Professor
uint32_t corine(uint32_t coarse, double *phase)
{
  if (*phase < 0.)
  {
    if (coarse != 0)
    {
      coarse--;
      *phase += 1.;
    }
    else
    {
      *phase = 0.;
    }
  }
  else if (*phase > 1.)
  {
    if (coarse != 0x7FFF)
    { // this is to avoid a roll-over/move to orbit+1 due to fine calibration
      coarse++;
      *phase -= 1.;
    }
    else
    {
      *phase = 1.;
    }
  }
  *phase = std::llround(*phase * 511.0);
  return (uint64_t)coarse;
}

// forming a 51-bit hit word and writing
void write_hit(std::fstream &fout, uint64_t channel, uint64_t colID, uint64_t leadingFine, uint64_t leadingCoarse, uint64_t trailingFine, uint64_t trailingCoarse, int tdcLead, int tdcTrail, uint32_t *count)
{
  trailingCoarse = trailingCoarse > 0x7F ? 0x7F : trailingCoarse;
  // trailingFine is unchanged by default if there is a significant enough timeout, watch this
  // placeholders
  uint64_t K_CODE = 0;
  uint64_t FEB_id = 0;
  uint64_t *hitWord = new uint64_t((FIFTY_ONE_BIT_MASK & ((K_CODE << 50) | (FEB_id << 48) | ((uint64_t)tdcTrail << 40) | (trailingCoarse << 33) | (trailingFine << 24) |(colID << 45)| (channel << 42) | (tdcLead << 22) | (leadingCoarse << 9) | leadingFine)));
  alcor_hit_51_t *hit1 = (alcor_hit_51_t *)hitWord;
  // std::cout<<"Writing: ";
  hit1->print();
  std::cout << "Writing: " << std::hex << ((*hitWord)) << std::endl;

  uint32_t half1 = *hitWord & THIRTY_TWO_BIT_MASK;
  uint32_t half2 = *hitWord >> 32;
  fout << std::hex << *hitWord;
  (*count)++;
}

// dce-decode, calibrate, encode
// This version relies on the loop in main and uses only the latest element as basis, quite different from the function used on the binary files
void dce(std::fstream &fout, uint32_t (*buffer)[8][3], uint32_t row, double *par, uint32_t *count, int (*evcounts)[8][5])
{
  int tdc = (row >> 24) & 0b11;
  // reminder: each csv file only has 1 column id, differentiate by pixel (channel) id
  uint64_t channel = (row >> 26) & 0x7;
  // consider including below in corine
  //TEMPORARILY removing calibration for testing
  double c_hit = (row) & 0x1FF;//par[tdc + 4] + ((row) & 0x1FF) * par[tdc];
  uint64_t coarse = (row >> 9) & COARSE_MASK;//corine((row >> 9) & COARSE_MASK, &c_hit);
  uint64_t colID = row >> 29;
  // For now, only checking if coarse time is zero for no pending leading; check if this assumption is false
  // Trying to order these cases by likelihood; write a counter per case to test
  //  previous leading edge timing out
  if ((tdc == 0 || tdc == 2) && (*buffer)[channel][0] != 0)
  {
    write_hit(fout, channel, colID, (*buffer)[channel][1], (*buffer)[channel][0], 0xff, 0x7f, (*buffer)[channel][2], (*buffer)[channel][2], count);
    fout << '\n';
    // there has to be a better way for assigning below
    (*buffer)[channel][0] = 0x1FFF & coarse;
    (*buffer)[channel][1] = c_hit;
    (*buffer)[channel][2] = tdc;
    std::cout << "leading edge timeout" << std::endl;
    if(coarse !=(0x1FFF & coarse))(*evcounts)[channel][3]++;
    (*evcounts)[channel][1]++;
    return;
  }
  // trailing edge and match
  else if ((tdc == 1 || tdc == 3) && (*buffer)[channel][0] != 0)
  {
/*     std::cout << std::hex << (*buffer)[channel][0] << std::endl;
    std::cout << std::hex << (*buffer)[channel][1] << std::endl;
    std::cout << std::hex << c_hit << std::endl; */
    //Mask to ensure that the coarse time fits into 7 bits
    uint64_t trailingCoarse = (coarse - (*buffer)[channel][0]);
    if(0x7F < (coarse - (*buffer)[channel][0])){
      trailingCoarse = 127;
      (*evcounts)[channel][4]++;
    }
    write_hit(fout, channel, colID, (*buffer)[channel][1], (*buffer)[channel][0], c_hit, trailingCoarse, (*buffer)[channel][2], tdc, count);
    fout << '\n';
    (*buffer)[channel][0] = 0;
    (*buffer)[channel][1] = 0;
    (*buffer)[channel][2] = 0;
    std::cout << "matched" << std::endl;
    (*evcounts)[channel][0]++;
    return;
  }
  // trailing edge timeout
  else if ((tdc == 1 || tdc == 3) && (*buffer)[channel][0] == 0)
  {
    std::cout<<colID<<std::endl;
    uint64_t trailingCoarse = coarse;
    write_hit(fout, channel, colID, c_hit, trailingCoarse, 0xff, 0x7f, tdc, tdc, count);
    fout << '\n';
    std::cout << "trailing edge timeout" << std::endl;
    if(coarse !=(0x1FFF & coarse))(*evcounts)[channel][4]++;
    (*evcounts)[channel][2]++;
    return;
  }
  // New leading
  else
  {
    (*buffer)[channel][0] = coarse;
    (*buffer)[channel][1] = c_hit;
    (*buffer)[channel][2] = tdc;
    std::cout << "new leading" << std::endl;
    return;
  }
}

// Main loop - reads in the csv file line by line and calls matching (dce function) each time
int main()
{
  // still taking parameters from .txt file for now
  double par[8];
  uint32_t reading = 0;
  std::ifstream pin("parameters.txt", std::ofstream::in);
  uint32_t overPos = 1;
  auto *overNew = new uint32_t(1);
  int i = 0;
  while (i < 8 && pin >> par[i])
    i++;
  pin.close();
  std::ifstream testCSV("IF_64/SR_DC1500kHz_PHY3kHz_shutter7.5ns/output_Col3_notsorted.csv", std::fstream::in);
  std::fstream outCSV("output_Col3_matched.csv", std::fstream::out);
  // buffer for 8 channels with coarse and fine (later calibrated) times and tdc id, respectively
  // check that 32 bit buffer does not interfere with forming the 51 bit word, as in check if there is need for this to be uint64_t
  uint32_t buffer[8][3] = {0};
  //counts for matched, leading and trailing edge timeouts, and leading and trailing edge overflows respecitvely.
  int evcount[8][5] = {0};
  std::string line, cell;
  testCSV.ignore(150, '\n');
  while (testCSV >> std::hex >> reading)
  {
/*     if(reading == 0x6122BA65)break; */
    // Reading in stream as hex directly
    // Apparently there is a need to enter a specific number of characters to skip (which we shouldn't have to care about) as well as the delimiter
    // The only other alternative I can see to this is reading in character by character, which is what may need to happen anyway considering VHDL requirements
    testCSV.ignore(200, '\n');
    std::cout << std::hex << reading << " " << std::dec << overPos << " " << (std::string(80, '/')) << std::endl;
    // below accounts for the headers of the csv file as does the ignore above the while loop, don't need those
    if (overPos == 0)
    {
      // change this so the final csv makes sense with double the columns
      outCSV << "HexWord\n"; //,ColID,PixelID,tdcID,TCoarse,TCoarse_diff,TFine,ToA,FrameNum,ToA_corrected,ToA_corrected_diff,CoarseCounter\n";
      overPos++;
      continue;
    }
    alcor_hit_t *hit1= (alcor_hit_t*)&reading;
    // std::cout<<"Writing: ";
    hit1->print();
    overPos++;
    dce(outCSV, &buffer, reading, par, overNew, &evcount);
/*          if (*overNew > 242)
      break;   */   
      
  } 
  /*
  int tempPos = 0;
for (auto f:buffer) {
  std::cout<<f[11]<<std::endl;

  if (tempPos++ == 10)break;
}
*/
  testCSV.close();
  outCSV.close();
  //stats
  std::cout << "Number of lines in original: " << std::dec << overPos << std::endl;
  std::cout << "Number of lines in new file: " << std::dec << *overNew << std::endl;
  // Delete below at final
  for(int y=0;y<8;y++){
    std::cout<<"Channel "<<y<<": "<<std::endl;
    std::cout<<evcount[y][0]<<" "<<evcount[y][1]<<" "<<evcount[y][2]<<" "<<evcount[y][3]<<" "<<evcount[y][4]<<std::endl;
    std::cout<<"Channel total: "<<(evcount[y][0]+evcount[y][1]+evcount[y][2])<<std::endl;
  }
  int finalCount = 0;
  std::cout<<"Channels with items remaining in buffer at finish: "<<std::endl;
  for(int j=0;j<8;j++)
  if(buffer[j][0] != 0 && buffer[j][1] != 0)
  std::cout<<(j+1)<<std::endl;
  int overruns = 0;
  for(int h=0;h<8;h++)
  overruns+=evcount[h][4];
  std::cout<<"Total trailing edge overruns: "<<overruns<<std::endl;
/*   for (auto l : buffer)
  {
    if (l[0] != 0)
      finalCount++;
  }
  std::cout << "Number of items remaining in the buffer: " << finalCount << std::endl; */
  std::cout << "-- All done, so long --" << std::endl;
}

// each column has 8 channels (pixels)
// Once more: do not worry about matching tdcs
// optimize, see line 94
// disregard order for checking
// lsb of fine is (1/394)/IF 40 ps for IF_64 (Interpolation Factor-lsb is divided by this)
// lsb for coarse is 1/394(MHz) ns