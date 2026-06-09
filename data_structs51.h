//data_structs.h - stores all structs from original decoder.cc code
#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H
#include <iostream>
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
        void print() {
        printf(" hit: %d %d %d %d \n", caffe, id, counter, size);
    }
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
    uint32_t fine   : 9;
    uint32_t coarse : 15;
    uint32_t tdc    : 2;
    uint32_t pixel  : 3;
    uint32_t column : 3;
    void print() {
        printf(" hit: %d %d %d %d %d \n", column, pixel, tdc, coarse, fine);
    }
    bool operator==(alcor_hit_t t) {
        return fine == t.fine && coarse == t.coarse && tdc == t.tdc && pixel == t.pixel && column == t.column;
    }
};

struct alcor_hit_51_t {
    uint64_t fine_lead : 9;
    uint64_t coarse_lead : 13;
    uint64_t tdc_lead : 2;
    uint64_t pixel  : 3;
    uint64_t column : 3;
    uint64_t fine_trail : 9;
    uint64_t coarse_trail : 7;
    uint64_t tdc_trail : 2;
    uint64_t FEB_ID : 2;
    uint64_t K_code : 2;
    void print() {
        printf(" hit: %d %d %d %d %d %d %d %d %d %d \n", K_code, FEB_ID, tdc_trail, coarse_trail, fine_trail, column, pixel, tdc_lead ,  coarse_lead,  fine_lead);
    }
    bool operator==(alcor_hit_51_t t) {
        return fine_lead == t.fine_lead && fine_trail == t.fine_trail && coarse_lead == t.coarse_lead
        && coarse_trail == t.coarse_trail && tdc_lead == t.tdc_lead && tdc_trail == t.tdc_trail
        && pixel == t.pixel && column == t.column && FEB_ID == t.FEB_ID && K_code == t.K_code;
    }
};

struct data_t {
    int device;
    int fifo;
    int K_code;
    int type;
    int FEB_ID;
    int counter;
    int column;
    int pixel;
    int tdc_lead;
    int tdc_trail;
    int rollover;
    int coarse;
    int coarse_lead;
    int coarse_trail;
    int fine_lead;
    int fine_trail;
};

#endif
