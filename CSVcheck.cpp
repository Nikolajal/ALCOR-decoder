#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include "data_structs51.h"
constexpr uint32_t THIRTY_TWO_BIT_MASK = 0xFFFFFFFF;

struct leadAndTrail{uint32_t lead; uint32_t trail; };

leadAndTrail splitting(uint64_t testRead){
    leadAndTrail tempWord;
    uint32_t wordID = (0x7E0000000000 & testRead) >> 42;
    tempWord.lead =  (wordID << 26)|(0x1800000 & testRead << 2)|(0x1FFFFF & testRead);
    uint32_t trailCoarse = (0x1FFF & (testRead >> 9)) + (0x3F & (testRead >> 32));
    tempWord.trail = (wordID << 26)|(0x1800000 & testRead >> 18)|(trailCoarse << 9)|(0xFF & (testRead>>24));
     uint32_t tester = (0x1FFFFF & testRead);
       alcor_hit_t *hit1= (alcor_hit_t*)&tempWord.trail;
        // std::cout<<"Writing: ";
        hit1->print();  
    return tempWord;
}

 void searching(std::ifstream& fin, uint32_t word, int (*tracking)[8], int channel){
    uint32_t sampleRead = 0;
    std::string testing;
    fin.seekg((*tracking)[channel]);
    while(fin >> std::hex >> sampleRead){
            fin.ignore(200, '\n');
            //check channel 
            if(sampleRead == word){
                (*tracking)[channel] = fin.tellg();
                return;
            }
    }
    std::cout<<std::hex<<word<<" not found"<<std::endl;
} 

int main(){
    std::ifstream sampleCSV("IF_64/SR_DC1500kHz_PHY3kHz_shutter7.5ns/output_Col3_notsorted.csv", std::fstream::in);
    std::ifstream testCSV("output_Col3_matched.csv", std::fstream::in);
    leadAndTrail tempWord;
    uint64_t testRead = 0;
    uint32_t leadTest, trailTest, sampleRead = 0;
    //testCSV.ignore(150, '\n');
    sampleCSV.ignore(150, '\n');
    //This storage of file pointers means that the file does not need to be reread from the start each time
    int tracking[8] = {0};
    //counting entries per channel
    int ccount[8] = {0};
    //Skipping past the column headers
    for(int y=0;y<8;y++)tracking[y] = sampleCSV.tellg();
    while (testCSV >> std::hex >> testRead)
    { 
/*         alcor_hit_51_t *hit1= (alcor_hit_51_t*)&testRead;
        // std::cout<<"Writing: ";
        hit1->print(); */
        tempWord = splitting(testRead);
        uint32_t leadTDCId = 0x3 & (tempWord.lead >> 24);
        uint32_t trailTDCId = 0x3 & (tempWord.trail >> 24);
        uint32_t pixelId = 0x7 & (testRead >> 41);
        //std::cout<<"PIxel"<<pixelId<<std::endl;
        ccount[pixelId]++;
        if(leadTDCId == trailTDCId && (leadTDCId == 0 || leadTDCId == 2)){
            searching(sampleCSV, tempWord.lead, &tracking, pixelId);
        }
        else if(leadTDCId == trailTDCId && (leadTDCId == 1 || leadTDCId == 3)){
            searching(sampleCSV, tempWord.trail, &tracking, pixelId);
        }
        else{
            searching(sampleCSV, tempWord.lead, &tracking, pixelId);
            searching(sampleCSV, tempWord.trail, &tracking, pixelId);
        }
        if(testRead == 0x65ba4a8dc855)break;
    }
    for(int y=0;y<8;y++){
    std::cout<<"Channel "<<std::dec<<y<<": "<<std::endl;
    std::cout<<std::dec<<ccount[y]<<" "<<ccount[y]<<" "<<ccount[y]<<std::endl;
    std::cout<<"Channel total: "<<std::dec<<(ccount[y]+ccount[y]+ccount[y])<<std::endl;
  }
    testCSV.close();
    sampleCSV.close();
}