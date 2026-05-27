#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <boost/program_options.hpp>
#include "TFile.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TTree.h"
#include "data_structs.h"







void write_data(TTree *tout, int device, int fifo, int type, int counter, int column, int pixel, int tdc, int rollover, int coarse, int fine, data_t& dataPrep)
{
  dataPrep.device = device;
  dataPrep.fifo = fifo;
  dataPrep.type = type;
  dataPrep.counter = counter;
  dataPrep.column = column;
  dataPrep.pixel = pixel;
  dataPrep.tdc = tdc;
  dataPrep.rollover = rollover;
  dataPrep.coarse = coarse;
  dataPrep.fine = fine;
  tout->Fill();
}

void write_trigger_data(TTree *tout, int device, int fifo, int type, int counter, int rollover, int coarse, data_t& dataPrep)
{
  write_data(tout, device, fifo, type, counter, -1, -1, -1, rollover, coarse, -1, dataPrep);
}

void write_alcor_data(TTree *tout, int device, int fifo, int column, int pixel, int tdc, int rollover, int coarse, int fine, data_t& dataPrep)
{
  write_data(tout, device, fifo, 1, -1, column, pixel, tdc, rollover, coarse, fine, dataPrep);
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
void decode(char *buffer, char *buffer_c, int device, int fifo, int size, TTree *tout, bool is_filtered, data_t &dataPrep, TGraph* gRollover, std::map<std::string,int> *counters)
{
  size /= 4;
  //assuming that cannot be in spill going out of the buffer
  bool in_spill = false;
  auto word = (uint32_t *)buffer;
  auto word_c = (uint32_t *)buffer_c;
  alcor_hit_t *hit;
  alcor_hit_t *hit_c;
  uint32_t pos = 0;

  // loop over buffer data
  while (pos < size) {

    // find spill header if not in spill already
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

      /** killed fifo **/
      if (*word == 0x666caffe) {
        //if (verbose) printf(" 0x%08x -- killed fifo \n", *word);
        write_trigger_data(tout, device, fifo, 15, -1, -1, -1, dataPrep);
        if(*word_c!=*word)counters->at("misses")++;
        ++word; ++pos; ++word_c;
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
        if(*word_c!=*word)counters->at("misses")++;
        ++word; ++pos;++word_c;
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
        if(*word_c!=*word)counters->at("misses")++;
        ++word; ++pos; ++word_c;
        continue;
      }
//std::cout<<"here"<<std::endl;
      /** hit **/
      hit = (alcor_hit_t *)word;
      hit_c = (alcor_hit_t *)word_c;
      double b=-0.5;
      double a=0.015;
      double c_hit =  b + (hit->fine) * a;
       if (c_hit < 0.) {
    if (hit->coarse!=0) {
      hit->coarse--;
      c_hit += 1.;
    } else {
      c_hit = 0.;
    }
  } else if (c_hit > 1.) {
    if (hit->coarse != 0x7FFF) {  // this is to avoid a roll-over/move to orbit+1 due to fine calibration
      hit->coarse++;
      c_hit -= 1.;
    } else {
      c_hit = 1.;
    }
  }
  c_hit=std::llround(c_hit * 511.0);
hit->fine=c_hit;
            //if(hit->coarse!=0|hit->calib!=0|hit->tdc!=0|hit->pixel!=0|hit->column!=0)
     // hit->print();
      //if (verbose) printf(" 0x%08x -- hit (coarse=%d, fine=%d, column=%d, pixel=%d --> channel=%d)\n", *word, hit->coarse, hit->fine, hit->column, hit->pixel, hit->column * 4 + hit->pixel);
      write_alcor_data(tout, device, fifo, hit->column, hit->pixel, hit->tdc, counters->at("rollover_counter"), hit->coarse, hit->fine, dataPrep);
      counters->at("integrated_hits")++;

      if(!(*hit_c==*hit)){
        hit->print();
        hit_c->print();
       counters->at("misses_c")++;
      }
      ++word; ++pos; ++word_c;
      
    }
  }
counters->at("total_bytes") += size;
}

void cdecoder(/*int argc, char *argv[]*/)
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
  std::ifstream fin;
  std::ifstream rin;
  fin.open("alcdaq.fifo_3.dat", std::ofstream::in | std::ofstream::binary);
  rin.open("calibtest.dat", std::ofstream::in | std::ofstream::binary);
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
  tout->Branch("type", &dataPrep.type, "type/I");
  tout->Branch("counter", &dataPrep.counter, "counter/I");
  tout->Branch("column", &dataPrep.column, "column/I");
  tout->Branch("pixel", &dataPrep.pixel, "pixel/I");
  tout->Branch("tdc", &dataPrep.tdc, "tdc/I");
  tout->Branch("rollover", &dataPrep.rollover, "rollover/I");
  tout->Branch("coarse", &dataPrep.coarse, "coarse/I");
  tout->Branch("fine", &dataPrep.fine, "fine/I");

  /** output histograms **/
  auto hCounters = new TH1F("hCounters", "", 3, 0, 3);
  gRollover = new TGraph;
  
  /** loop over data **/
  buffer_header_t buffer_header;
  buffer_header_t buffer_header_c;

  while (true) {
    fin.read((char *)(&buffer_header), sizeof(buffer_header_t));
    rin.read((char *)(&buffer_header_c), sizeof(buffer_header_t));
      if(buffer_header_c.size!= buffer_header.size)std::cout<<"Warning: Buffer sizes do not match: "<<buffer_header_c.size
  <<" "<<buffer_header.size<<std::endl;

std::cout<<buffer_header_c.size<<" "<<buffer_header.size<<std::endl;
    if (fin.eof()) break;
    if (buffer_header.caffe != 0x123caffe) {
      printf(" --- [ERROR] caffe header mismatch in buffer header: %08x \n", buffer_header.caffe);
      break;
    }
    fin.read(buffer, buffer_header.size);
    if (rin.eof()){
      std::cout<<"Calibrated raw data prematurely reached end of file. Stopping"<<std::endl;
      break;
    }
    rin.read(buffer_c, buffer_header_c.size);
   // std::cout<<buffer_header.size<<std::endl;
    if (buffer_header.id < 24) {
      decode(buffer, buffer_c, main_header.device, buffer_header.id, buffer_header.size, tout, is_filtered, dataPrep, gRollover, counters);
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