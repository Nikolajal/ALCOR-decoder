#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <boost/program_options.hpp>
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TTree.h"
#include "data_structs51.h"

constexpr uint32_t coarse_mask = 0x7FFF;
constexpr uint64_t FIFTY_ONE_BIT_MASK = 0x0007FFFFFFFFFFFF;
constexpr uint32_t THIRTY_TWO_BIT_MASK = 0xFFFFFFFF;


void write_data(TTree *tout, int device, int fifo, int K_code, int type, int FEB_ID, int counter, int column, int pixel, int tdc_lead, int tdc_trail, int rollover, int coarse, int coarse_lead, int coarse_trail, int fine_lead, int fine_trail, data_t& dataPrep)
{
  dataPrep.device = device;
  dataPrep.fifo = fifo;
  dataPrep.K_code = K_code;
  dataPrep.type = type;
  dataPrep.FEB_ID = FEB_ID;
  dataPrep.counter = counter;
  dataPrep.column = column;
  dataPrep.pixel = pixel;
  dataPrep.tdc_lead = tdc_lead;
  dataPrep.tdc_trail = tdc_trail;
  dataPrep.rollover = rollover;
  dataPrep.coarse = coarse;
  dataPrep.coarse_lead = coarse_lead;
  dataPrep.coarse_trail = coarse_trail;
  dataPrep.fine_lead = fine_lead;
  dataPrep.fine_trail = fine_trail;
  tout->Fill();
}

void write_trigger_data(TTree *tout, int device, int fifo, int type, int counter, int rollover, int coarse, data_t& dataPrep)
{
  write_data(tout, device, fifo, -1, type, -1, counter, -1, -1, -1, -1, rollover, coarse, -1, -1, -1, -1, dataPrep);
}
void write_alcor_data(TTree *tout, int device, int fifo, int K_code, int FEB_ID, int tdc_lead, int tdc_trail, int rollover, int coarse_trail, int coarse_lead, int fine_trail, int fine_lead, int column, int pixel, data_t& dataPrep)
{
  write_data(tout, device, fifo, K_code, 1, FEB_ID, -1, column, pixel, tdc_lead, tdc_trail, rollover, -1, coarse_lead, coarse_trail, fine_lead, fine_trail, dataPrep);

}
          
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

void decode_trigger(char *buffer, char *buffer_c, int device, int fifo, int size, TTree *tout, data_t& dataPrep,  std::map<std::string,int> *counters)
{
  //if (verbose) printf(" --- decode_trigger: device-%d fifo-%d, size=%d \n", device, fifo, size); 

  size /= 4;
  auto word = (uint32_t *)buffer;
  auto word_c = (uint32_t *)buffer_c;
  uint32_t pos = 0;

  while (pos < size) {

    /** spill header **/
    if ((*word & 0xf0000000) == 0x70000000) {
  
      uint32_t counter = (*word & 0x0fff0000) >> 16;
      uint64_t trigger_time = 0x0;
      //if (verbose) printf(" 0x%08x -- spill header (counter=%d)\n", *word, counter);
      trigger_time = (uint64_t)(*word & 0xff) << 32;
              if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos;++word_c;
      //if (verbose) printf(" 0x%08x -- spill header continued \n", *word);
      trigger_time |= *word;
      uint32_t coarse = trigger_time & 0x7fff;
      uint32_t rollover = trigger_time >> 15;
      write_trigger_data(tout, device, fifo, 7, counter, rollover, coarse, dataPrep);
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos;++word_c;
    }
    
    /** spill trailer **/
    else if ((*word & 0xf0000000) == 0xf0000000) {
      spill_t *spill = (spill_t *)word;
      uint32_t counter = (*word & 0x0fff0000) >> 16;
      uint64_t trigger_time = 0x0;
     // if (verbose) printf(" 0x%08x -- spill trailer (counter=%d)\n", *word, counter);
      trigger_time = (uint64_t)(*word & 0xff) << 32;
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos;++word_c;
      //if (verbose) printf(" 0x%08x -- spill trailer continued \n", *word);
      trigger_time |= *word;
      uint32_t coarse = trigger_time & 0x7fff;
      uint32_t rollover = trigger_time >> 15;
      write_trigger_data(tout, device, fifo, 15, counter, rollover, coarse, dataPrep);
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos; ++word_c;
    }
    
    /** trigger **/
    else if ((*word & 0xf0000000) == 0x90000000) {
      trigger_t *trigger = (trigger_t *)word;
      uint64_t trigger_time = 0x0;
     // if (verbose) printf(" 0x%08x -- trigger header\n", *word);
      trigger_time = (uint64_t)(*word & 0xff) << 32;
      uint32_t counter = (*word & 0xffff00) >> 16;
        if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos;++word_c;
      //if (verbose) printf(" 0x%08x -- trigger header continued \n", *word);
      trigger_time |= *word;
      uint32_t coarse = trigger_time & 0x7fff;
      uint32_t rollover = trigger_time >> 15;
      write_trigger_data(tout, device, fifo, 9, counter, rollover, coarse, dataPrep);
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos; ++word_c;
    }

    /** else **/
    else {
      printf(" 0x%08x -- unexpected word \n", *word);
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos; ++word_c;
    }
    
  }

}

//decode(buffer, main_header.device, buffer_header.id, buffer_header.size, tout, is_filtered);
void decode(char *buffer, char *buffer_c, int device, int fifo, int size, int size_c, TTree *tout, bool is_filtered, data_t &dataPrep, TGraph* gRollover, std::map<std::string,int> *counters, double *par)
{
  size /= 4;
  //assuming that cannot be in spill going out of the buffer
  bool in_spill = false;
  auto word = (uint32_t *)buffer;
  auto word_c = (uint32_t *)buffer_c;
  bool leadingFlag[2]={0,0};
  uint64_t leadingCoarse[2]={0,0};
  double leadingFine[2]={0,0};
  //queue to read in anything but hits from the 51 bit file. This relies on the 51 bit file being no longer than the 32 bit file
  std::queue<uint32_t> word_check;
  uint64_t *hitWord;
  alcor_hit_51_t *hit;
  alcor_hit_51_t *hit_c;
  uint32_t pos = 0;

  // loop over buffer data
  while (pos < size) {
    // find spill header if not in spill already, assuming files are identical here
    while (!in_spill && pos < size) {
      
      /** spill header **/
      if ((*word & 0xf0000000) == 0x70000000) {
        if(*word_c!=*word)counters->at("misses")++;
        uint32_t counter = (*word & 0x0fff0000) >> 16;
        uint64_t trigger_time = 0x0;
       // if (verbose) printf(" 0x%08x -- spill header (counter=%d)\n", *word, counter);
        trigger_time = (uint64_t)(*word & 0xff) << 32;
        ++word; ++pos;++word_c;
        //if (verbose) printf(" 0x%08x -- spill header continued \n", *word);
        trigger_time |= *word;
        uint32_t coarse = trigger_time & 0x7fff;
        uint32_t rollover = trigger_time >> 15;
        write_trigger_data(tout, device, fifo, 7, counter, rollover, coarse, dataPrep);
        if(*word_c!=*word)counters->at("misses")++;
        ++word; ++pos;++word_c;
        in_spill = true;

	break;
      }

      /** something else **/
      //if (verbose) {
	//	if (!in_spill)
	//	  printf(" 0x%08x -- filler (pos=%d)\n", *word, pos % 16);
	//	else 
	 // printf(" 0x%08x -- \n", *word);
     // }
      if(*word_c!=*word)counters->at("misses")++;
      ++word; ++pos;++word_c;
    }
    
    // find spill trailer
    while (pos < size) {
      while(*word_c == 0x666caffe || (*word_c & 0xf0000000) == 0xf0000000 || (*word_c == 0x5c5c5c5c)){
        word_check.push(*word_c);
        ++word_c;
      }
      /** killed fifo **/
      if (*word == 0x666caffe) {
        //if (verbose) printf(" 0x%08x -- killed fifo \n", *word);
        write_trigger_data(tout, device, fifo, 15, -1, -1, -1, dataPrep);
        if( word_check.front()==*word){
          word_check.pop();
        }
        else
        counters->at("misses")++;
        ++word; ++pos; 
        in_spill = false;
	counters->at("rollover_counter") = 0;
        break;	
      }
      
      /** spill trailer **/
      if ((*word & 0xf0000000) == 0xf0000000) {
        spill_t *spill = (spill_t *)word;
        uint32_t counter = (*word & 0x0fff0000) >> 16;
        uint64_t trigger_time = 0x0;
        //if (verbose) printf(" 0x%08x -- spill trailer (counter=%d)\n", *word, counter);
        trigger_time = (uint64_t)(*word & 0xff) << 32;
        if(*word_c!=*word)counters->at("misses")++;
        ++word; ++pos; ++word_c;
        //if (verbose) printf(" 0x%08x -- spill trailer continued \n", *word);
        trigger_time |= *word;
        uint32_t coarse = trigger_time & 0x7fff;
        uint32_t rollover = trigger_time >> 15;
        write_trigger_data(tout, device, fifo, 15, counter, rollover, coarse, dataPrep);
        if( word_check.front()==*word){
          word_check.pop();
        }
        else
        counters->at("misses")++;
        ++word; ++pos;
        in_spill = false;
	gRollover->AddPoint(counters->at("integrated_spill"), counters->at("rollover_counter"));
	counters->at("integrated_spill")++;
	counters->at("rollover_counter") = 0;
	break;
      }

      /** rollover **/
      if (*word == 0x5c5c5c5c) {
        //if (verbose) printf(" 0x%08x -- rollover (counter=%d) \n", *word, rollover_counter);
        ++counters->at("rollover_counter");
	++counters->at("integrated_rollover");
        if( word_check.front()==*word){
          word_check.pop();
        }
        else
        counters->at("misses")++;
        ++word; ++pos; 
        continue;
      }
//std::cout<<"here"<<std::endl;
      /** hit **/
      int tdc = (*word >> 24) & 0b11;
      double c_hit =  par[tdc+4] + ((*word) & 0x1FF) * par[tdc];
      if(tdc<2){
        leadingFine[tdc] = c_hit;
        leadingCoarse[tdc] = corine((*word>>9)&coarse_mask,&leadingFine[tdc]);
        leadingFlag[tdc] = true;
      }
            //dealing with trailing edge; only if there was a previous leading edge
      else if(tdc > 1 && leadingFlag[tdc-2] == 1){ 
        uint64_t trailingCoarse = (corine((*word>>9)&coarse_mask,&c_hit)) - leadingCoarse[tdc-2];
        trailingCoarse = trailingCoarse > 0x7F ? 0x7F : trailingCoarse;
        uint64_t K_CODE = 0;
        uint64_t FEB_id = 0;
        std::cout<<"here"<<std::endl;
        *hitWord = (FIFTY_ONE_BIT_MASK&((K_CODE<<50)|(FEB_id<<48)|((uint64_t)tdc<<46)
                          |(trailingCoarse<<39)|((uint64_t)c_hit<<30)|((uint64_t)(*word>>26)<<24)|((uint64_t)(tdc-2)<<22)
                          |(leadingCoarse[tdc-2]<<9)|(uint64_t)leadingFine[tdc-2]));
                          //std::cout<<std::hex<<((uint32_t)(*hitWord&THIRTY_TWO_BIT_MASK))<<std::endl;
                          std::cout<<"here1"<<std::endl;
      hit = (alcor_hit_51_t *)hitWord;
      uint64_t half1 = (uint64_t)*word_c<<32;
      hit_c = (alcor_hit_51_t *)(half1|*(++word_c));
      std::cout<<hit->column<<std::endl;
      write_alcor_data(tout, device, fifo, hit->K_code, hit->FEB_ID, hit->tdc_lead, hit->tdc_trail, counters->at("rollover_counter"), hit->coarse_trail, hit->coarse_lead, hit->fine_trail, hit->fine_lead, hit->column, hit->pixel, dataPrep);
      counters->at("integrated_hits")++;
      hit_c->print();
      if(!(*hit_c==*hit)){
        //hit->print();
        //hit_c->print();
       counters->at("misses_c")++;
      }
      ++word_c;
      }

            //if(hit->coarse!=0|hit->calib!=0|hit->tdc!=0|hit->pixel!=0|hit->column!=0)
     // hit->print();
      //if (verbose) printf(" 0x%08x -- hit (coarse=%d, fine=%d, column=%d, pixel=%d --> channel=%d)\n", *word, hit->coarse, hit->fine, hit->column, hit->pixel, hit->column * 4 + hit->pixel);
      ++word; ++pos;
      
    }
  }
counters->at("total_bytes") += size;
}

void cdecoder51(/*int argc, char *argv[]*/)
{
  bool verbose = false;
  TGraph *gRollover = nullptr;
  std::map<std::string,int> *counters=new std::map<std::string,int>{{"integrated_rollover",0},{"integrated_spill",0},
  {"integrated_hits",0},{"rollover_counter",0},{"frame",0},{"misses",0},{"misses_c",0},{"total_bytes",0}};

  //std::cout << " --- ALCOR decoder ---" << std::endl;

  std::string input_filename, output_filename;
  
  /** process arguments **/
/*
  namespace po = boost::program_options;
  po::options_description desc("Options");
  try {
    desc.add_options()
      ("help"    , "Print help messages")
      ("input"   , po::value<std::string>(&input_filename)->required(), "Input data file")
      ("output"  , po::value<std::string>(&output_filename)->required(), "Output data file")
      ("verbose" , po::bool_switch(&verbose)->default_value(false), "Verbose mode flag")
      ;
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return; 
    }
  }
  catch(std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    std::cout << desc << std::endl;
    return;
  }

  /** open input file **/
  //std::cout << " --- opening input file: " << input_filename << std::endl;
  double par[8];
  std::ifstream pin("parameters.txt", std::ofstream::in);
  int i=0;
  while(!pin.eof() || i<8) pin>>par[i++];
  pin.close();

  std::ifstream fin;
  std::ifstream rin;
  fin.open("alcdaq.fifo_3.dat", std::ofstream::in | std::ofstream::binary);
  rin.open("calibtest51.dat", std::ofstream::in | std::ofstream::binary);
  output_filename="rawcalib.root";
  /** read main header **/ 
  main_header_t main_header;
  fin.read((char *)&main_header, sizeof(main_header_t));
  if (main_header.caffe != 0x000caffe) {
    printf(" --- [ERROR] caffe header mismatch in main header: 0x%08x \n", main_header.caffe);
    //return 1;
  }
  /*
  if (verbose) {
    printf(" --- [main header] caffe header detected: 0x%08x \n", main_header.caffe);
    printf(" --- [main header] readout version: 0x%08x \n", main_header.readout_version);
    printf(" --- [main header] firmware release: 0x%08x \n", main_header.firmware_release);
    printf(" --- [main header] run number: %d \n", main_header.run_number);
    printf(" --- [main header] timestamp: %d \n", main_header.timestamp);
    printf(" --- [main header] staging buffer size: %d \n", main_header.staging_size);
    printf(" --- [main header] run mode: 0x%1x \n", main_header.run_mode);
    printf(" --- [main header] filter mode: 0x%1x \n", main_header.filter_mode);
    printf(" --- [main header] device: %d \n", main_header.device);
    printf(" --- [main header] timestamp: %d \n", main_header.timestamp);
  }
*/
  // check that we know how to decode it
  bool is_filtered;
  if (main_header.filter_mode == 0x0)
    is_filtered = false;
  else if (main_header.filter_mode == 0xf)
    is_filtered = true;
  else {
    printf(" --- [ERROR] filter mode not supported: 0x%01x \n", main_header.filter_mode);
    //return 1;
  }
  main_header_t main_header_c;
  rin.read((char *)&main_header_c, sizeof(main_header_t));
  // create reading buffer
  auto staging_size = main_header.staging_size;
  if(main_header_c.staging_size != staging_size)std::cout<<"Warning: main header staging sizes do not match!"<<std::endl;
  char *buffer = new char[staging_size];
  char *buffer_c = new char[main_header_c.staging_size];
  /** open output file **/
  //std::cout << " --- opening output file: " << output_filename << std::endl;
  auto fout = TFile::Open(output_filename.c_str(), "RECREATE");
  auto tout = new TTree("alcor", "ALCOR");
  data_t dataPrep;
  tout->Branch("device", &dataPrep.device, "device/I");
  tout->Branch("fifo", &dataPrep.fifo, "fifo/I");
  tout->Branch("K_code", &dataPrep.fifo, "K_code/I");
  tout->Branch("type", &dataPrep.type, "type/I");
  tout->Branch("FEB_ID", &dataPrep.type, "FEB_ID/I");
  tout->Branch("counter", &dataPrep.counter, "counter/I");
  tout->Branch("column", &dataPrep.column, "column/I");
  tout->Branch("pixel", &dataPrep.pixel, "pixel/I");
  tout->Branch("tdc_lead", &dataPrep.tdc_lead, "tdc_lead/I");
  tout->Branch("tdc_trail", &dataPrep.tdc_trail, "tdc_trail/I");
  tout->Branch("rollover", &dataPrep.rollover, "rollover/I");
  tout->Branch("coarse", &dataPrep.coarse, "coarse/I");
  tout->Branch("coarse_lead", &dataPrep.coarse_lead, "coarse_lead/I");
  tout->Branch("coarse_trail", &dataPrep.coarse_trail, "coarse_trail/I");
  tout->Branch("fine_lead", &dataPrep.fine_lead, "fine_lead/I");
  tout->Branch("fine_trail", &dataPrep.fine_trail, "fine_trail/I");

  /** output histograms **/
  auto hCounters = new TH1F("hCounters", "", 3, 0, 3);
  gRollover = new TGraph;
  
  /** loop over data **/
  buffer_header_t buffer_header;
  buffer_header_t buffer_header_c;

  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    rin.read((char *)(&buffer_header_c), sizeof(buffer_header_t));

std::cout<<buffer_header_c.size<<" "<<buffer_header.size<<std::endl;
    if (fin.eof()) break;
    if (buffer_header.caffe != 0x123caffe) {
      printf(" --- [ERROR] caffe header mismatch in buffer header: %08x \n", buffer_header.caffe);
      break;
    }
    fin.read(buffer, buffer_header.size);
    rin.read(buffer_c, buffer_header_c.size);
   // std::cout<<buffer_header.size<<std::endl;
    if (buffer_header.id < 24) {
      decode(buffer, buffer_c, main_header.device, buffer_header.id, buffer_header.size, buffer_header_c.size, tout, is_filtered, dataPrep, gRollover, counters, par);
    }
    else if (buffer_header.id == 24) {
      decode_trigger(buffer, buffer_c, main_header.device, buffer_header.id, buffer_header.size, tout, dataPrep, counters);
    }
  }
  
  double integrated = (double)counters->at("integrated_rollover") * 0.0001024;
  //std::cout << " --- integrated seconds: " << integrated << std::endl;

  /** write tree and close output */
  tout->Write();
  //std::cout << " --- integrated spill: " << integrated_spill << std::endl;
  hCounters->SetBinContent(1, counters->at("integrated_spill"));
  hCounters->SetBinContent(2, counters->at("integrated_rollover"));
  hCounters->SetBinContent(3, counters->at("integrated_hits"));
  hCounters->Write();
  gRollover->Write("gRollover");
  fout->Close();
  
  /** close input file **/
  fin.close();
  rin.close();
  std::cout<<"Hit misses: "<<counters->at("misses_c")<<std::endl;
  std::cout<<"Other word misses: "<<counters->at("misses")<<std::endl;
  std::cout<<"Total non-header words: "<<counters->at("total_bytes")<<std::endl;
  std::cout << " --- all done, so long ---" << std::endl;

  //return 0;
}


//have a more elegant way of handling if the two files are of different lengths for checking
//make misses local and specific to each function