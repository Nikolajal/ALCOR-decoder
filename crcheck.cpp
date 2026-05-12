#include <iostream>
#include <fstream>
#include <string>
//#include <boost/program_options.hpp>
#include "TFile.h"
#include "TTree.h"

struct hit_t {
  int fifo, type, counter, column, pixel, tdc, rollover, coarse, fine;
  double time;
};

void crcheck(const std::string infilename1, const std::string infilename2)
{  auto in1=TFile::Open(infilename1.c_str());
  auto in2=TFile::Open(infilename2.c_str());
  TTree *fin = (TTree*)in1->Get("alcor");
  TTree *rin = (TTree*)in2->Get("alcor");

  auto nev = fin->GetEntries();
  hit_t hit1;
  fin->SetBranchAddress("fifo", &hit1.fifo);
  fin->SetBranchAddress("type", &hit1.type);
  fin->SetBranchAddress("counter", &hit1.counter);
  fin->SetBranchAddress("column", &hit1.column);
  fin->SetBranchAddress("pixel", &hit1.pixel);
  fin->SetBranchAddress("tdc", &hit1.tdc);
  fin->SetBranchAddress("rollover", &hit1.rollover);
  fin->SetBranchAddress("coarse", &hit1.coarse);
  fin->SetBranchAddress("fine", &hit1.fine);

  hit_t hit2;
  rin->SetBranchAddress("fifo", &hit2.fifo);
  rin->SetBranchAddress("type", &hit2.type);
  rin->SetBranchAddress("counter", &hit2.counter);
  rin->SetBranchAddress("column", &hit2.column);
  rin->SetBranchAddress("pixel", &hit2.pixel);
  rin->SetBranchAddress("tdc", &hit2.tdc);
  rin->SetBranchAddress("rollover", &hit2.rollover);
  rin->SetBranchAddress("coarse", &hit2.coarse);
  rin->SetBranchAddress("fine", &hit2.fine);
      for (int iev = 0; iev < nev; ++iev) {
        std::cout<<iev<<std::endl;
        fin->GetEntry(iev);
        rin->GetEntry(iev);
        if(hit1.fifo!=hit2.fifo|hit1.type!=hit2.type|hit1.counter!=hit2.counter|hit1.column!=hit2.column|hit1.pixel!=hit2.pixel|hit1.tdc!=hit2.tdc|
        hit1.rollover!=hit2.rollover|hit1.coarse!=hit2.coarse|hit1.fine!=hit2.fine)
        std::cout<<"hit"<<std::endl;
      }
  in1->Close();
  in2->Close();

}