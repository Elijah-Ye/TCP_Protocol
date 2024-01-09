#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

#define MAX_DATA_SIZE 3000              // MTU on test network = 1472 + 20 + 8
#define INIT_SST_THRESHOLD 750         // SET the initial SST as high as possible
#define TIMES              3         // 10% drop rate
#define MAX_BUFFER_SIZE 750

typedef enum state{
    SLOW_START,
    CONGESTION_AVOIDENCE,
    FAST_RECOVERY
}state_t;

typedef enum packet_type {
    DATA,
    ACK,
    FIN,
    FINACK // connection teardown
} packet_type_t;

typedef struct packet{
    char data[MAX_DATA_SIZE];
    int sequence_no; 
    int ack_no;
    int payload_len;  
    packet_type_t ptype;
} packet_t;

struct compare {
    bool operator()(packet_t a, packet_t b) {
        return  a.sequence_no >= b.sequence_no;
    }
};

#endif