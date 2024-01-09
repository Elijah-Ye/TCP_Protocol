/* 
 * File:   receiver_main.cpp
 * Author: Elijah & Ananya 
 *
 * Created on Oct 8, 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "types.h"
#include <queue>
#include <iostream>
#include <vector>
#include <netdb.h>


std::priority_queue<packet_t, std::vector<packet_t>, compare> receiverQueue;

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *msg) {
    perror(msg);
    exit(1);
}

/* Helper function to send ACK */
void send_ACK(int ack_num, packet_type_t type){
    packet_t ack;
    ack.ack_no = ack_num;
    ack.ptype = type;
    if(sendto(s, &ack, sizeof(packet_t), 0, (struct sockaddr*) &si_other, (socklen_t)sizeof(si_other)) == -1){
        diep((char *)"Failed to send ACK\n");
    }
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep((char *)"socket");


    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep((char *)"bind failed\n");


	/* Now receive data and send acknowledgements */    
    FILE* fp;  
    if((fp = fopen(destinationFile, "wb"))== NULL){
        diep((char *)"Cannot open the destination file\n");
    } 

    int curAck = 0;

    while(1){
        packet_t recPKT;
        if(recvfrom(s, &recPKT, sizeof(recPKT),0, (struct sockaddr*) &si_other, (socklen_t*)&slen) == -1){
            diep((char *)"recvfrom sender error");
        }
        if(recPKT.ptype == FIN){
            for(int i = 0; i < TIMES; i++){
                send_ACK(curAck, FINACK);
            }
            break;
        }
        else if (recPKT.ptype == DATA){
            // printf("received data: %d\n", recPKT.sequence_no);
            // printf("curAck: %d\n", curAck);
            if(recPKT.sequence_no < curAck){
                send_ACK(recPKT.sequence_no, ACK);
                // do nothing, since we have already receive this packet
            }
            // out of order packet, we need to put into our buffer
            else if (recPKT.sequence_no > curAck){
                // TODO: changed here
                int tempAck;

                if(curAck > 0){
                    tempAck = curAck - 1;
                } else {
                    tempAck = 0;
                }
                send_ACK(tempAck, ACK);
                receiverQueue.push(recPKT);
            }
            // it is in order, we need to pop all the packets from buff as long as they are in order
            else {
                // printf("data: %s\n", recPKT.data);
                fwrite(recPKT.data, sizeof(char), recPKT.payload_len, fp);
                send_ACK(curAck, ACK);
                curAck += 1;

                while(!receiverQueue.empty() && receiverQueue.top().sequence_no == curAck){
                    fwrite(receiverQueue.top().data, sizeof(char), receiverQueue.top().payload_len, fp);
                    send_ACK(curAck, ACK);
                    curAck += 1;
                    receiverQueue.pop();
                }
            }
            
        }
    }

    fclose(fp);
    close(s);
	printf("%s received.", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}