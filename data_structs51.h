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
    uint32_t fine_lead : 9;
    uint32_t coarse_lead : 13;
    uint32_t tdc_lead : 2;
    uint32_t pixel  : 3;
    uint32_t column : 3;
    uint32_t fine_trail : 9;
    uint32_t coarse_trail : 7;
    uint32_t tdc_trail : 2;
    uint32_t FEB_ID : 2;
    uint32_t K_code : 1;
    void print() {
        printf(" hit: %d %d %d %d %d %d %d %d\n", column, pixel, tdc_lead , tdc_trail , coarse_lead, coarse_trail, fine_lead, fine_trail);
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
    int type;
    int counter;
    int column;
    int pixel;
    int tdc;
    int rollover;
    int coarse;
    int fine;
};

#endif
