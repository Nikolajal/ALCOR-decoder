#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
//bitset allows outputing of binary sequences, for debugging
//REMEMBER to comment it out
//#include <bitset>
#include "data_structs51.h"
constexpr uint32_t THIRTY_TWO_BIT_MASK = 0xFFFFFFFF;

//to output both 32-bit words at once from the function
struct leadAndTrail{uint32_t lead; uint32_t trail; };

//splitting the 51-bit word to two 32-bit ones; this scheme could be done better probably
leadAndTrail splitting(uint64_t testRead){
    leadAndTrail tempWord;
    uint32_t wordID = (0x7E0000000000 & testRead) >> 42;
    uint32_t trailCoarse = (0x7F & (testRead >> 33));
    tempWord.lead =  (wordID << 26)|(0x3000000 & testRead << 2)|(0x3FFFFF & testRead);
    if(trailCoarse == 127) trailCoarse = 0x7FFF;
    else trailCoarse = (0x1FFF & (testRead >> 9)) + (0x7F & (testRead >> 33));
    tempWord.trail = (wordID << 26)|(0x3000000 & testRead >> 16)|(trailCoarse << 9)|(0xFF & (testRead>>24));
    // alcor_hit_t *hit2= (alcor_hit_t*)&tempWord.trail;
   // std::cout<<"ready"<<std::endl;
   // std::cout<<"tdc "<<std::bitset<32>(0x3000000 & testRead >> 16)<<" "<<std::bitset<32>(0x3 & testRead >> 22)<<std::endl;
    //hit2->print(); 
    return tempWord;
}

//searching for a word in the original 32-bit word file, while keeping track of file pointers so that there is no need to read from the start of the file each time
 bool searching(std::ifstream& fin, uint32_t word, int (*tracking)[8], int channel, int (*ccount)[8][5]){
    uint32_t sampleRead = 0;
    bool found = 0;
    int count = 0;
    bool overrun = 0;
   //aforementioned pointer
    fin.seekg((*tracking)[channel]);
    //This is to further indicate trailing coarse overrun
    if(((0x3 & word>>24) == 1 || (0x3 & word>>24) == 3) && (0x7FFF & word >> 9) == 0x7FFF){
        word = 0xFF0001FF & word;
        overrun = 1;
        (*ccount)[channel][3]++;
    }
       // alcor_hit_t *hit = (alcor_hit_t*)&word;;
    //hit->print(); 
    while(fin >> std::hex >> sampleRead){
       // alcor_hit_t *hit2= (alcor_hit_t*)&sampleRead;
        //hit2->print(); 
            fin.ignore(200, '\n');
            count++;
            if(overrun == true) sampleRead = 0xFF0001FF & sampleRead;
            //std::cout<<(*hit==*hit2)<<"r:"<<std::hex<<sampleRead<<" "<<word<<std::endl;
            if(word == sampleRead){
                //hit2->print(); 
                (*tracking)[channel] = fin.tellg();
                std::cout<<"Found: "<<std::hex<<sampleRead<<" "<<count<<std::endl;
                found = 1;
                return found;
            }
  /*               else if(count == 20){
                std::cout<<"timeout"<<std::endl;
                return found;
            }     */
    }
    std::cout<<std::hex<<word<<" not found"<<std::endl;
    return found;
} 

int main(){
    std::ifstream sampleCSV("IF_64/SR_DC1500kHz_PHY3kHz_shutter7.5ns/output_Col3_notsorted.csv", std::fstream::in);
    std::ifstream testCSV("output_Col3_matched.csv", std::fstream::in);
    leadAndTrail tempWord;
    uint64_t testRead = 0;
    //testCSV.ignore(150, '\n');
    sampleCSV.ignore(150, '\n');
    //This storage of file pointers means that the file does not need to be reread from the start each time
    int tracking[8] = {0};
    //counting entries per channel
    int ccount[8][5] = {0};
    //Skipping past the column headers
    for(int y=0;y<8;y++)tracking[y] = sampleCSV.tellg();
    while (testCSV >> std::hex >> testRead)
    { 
        std::cout<<"Cycle"<<std::endl;
/*         for(int u=0; u<8; u++)
        std::cout<<std::dec<<tracking[u]<<" ";
        std::cout<<'\n'; */
         alcor_hit_51_t *hit1= (alcor_hit_51_t*)&testRead;
       hit1->print(); 
        tempWord = splitting(testRead);
        uint32_t leadTDCId = 0x3 & (tempWord.lead >> 24);
        uint32_t trailTDCId = 0x3 & (tempWord.trail >> 24);
        uint32_t pixelId = 0x7 & (testRead >> 42);
        ccount[pixelId][0]++;
        //Timeout case for both leading and trailing
        if(leadTDCId == trailTDCId){
            std::cout<<"Finding timed out"<<std::endl;
            bool searched = searching(sampleCSV, tempWord.lead, &tracking, pixelId, &ccount);
            //Friendly reminder: trailing edge timeouts are also written to the lead position of the struct, not trailing
            if(searched == 1 && (leadTDCId == 0 ||leadTDCId == 2))ccount[pixelId][1]++;
            else if(searched == 1 && (leadTDCId == 1 ||leadTDCId == 3))ccount[pixelId][2]++;
            //the case for not finding a word
        else{ 
            ccount[pixelId][4]++;
            break;
        }
        }
        //looking for two matched words
        else{
        std::cout<<std::hex<<"Finding leading: "<<tempWord.lead<<std::endl;
        bool searching1 = searching(sampleCSV, tempWord.lead, &tracking, pixelId, &ccount);
        std::cout<<std::hex<<"Finding trail: "<<tempWord.trail<<std::endl;
        bool searching2 =   searching(sampleCSV, tempWord.trail, &tracking, pixelId, &ccount);
        if(searching1 && searching2)ccount[pixelId][0]++;
        else{ 
            ccount[pixelId][4]++;
            break;
        }
        }
    }
    //stats
    int overruns = 0;
    int misses = 0;
     for(int y=0;y<8;y++){
    std::cout<<"Channel "<<std::dec<<y<<": "<<std::endl;
    std::cout<<std::dec<<ccount[y][0]<<" "<<ccount[y][1]<<" "<<ccount[y][2]<<" "<<ccount[y][3]<<std::endl;
    std::cout<<"Channel total: "<<std::dec<<(ccount[y][0]+ccount[y][1]+ccount[y][2]+ccount[y][3])<<std::endl;
    overruns+=ccount[y][3];
    misses+=ccount[y][4];
  } 
  std::cout<<"Misses: "<<std::dec<<misses<<std::endl;    
    testCSV.close();
    sampleCSV.close();
    std::cout << "-- Check complete, so long --" << std::endl;
}