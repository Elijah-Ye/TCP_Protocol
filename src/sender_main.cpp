/* 
 * File:   sender_main.c
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
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include "types.h"

#include <queue>
#include <math.h>
#include <iostream>


std::queue <packet_t> packet_buffer_queue; 
std::queue <packet_t> window_buffer_queue;
packet_t current_packet;

FILE *fp;
int sockfd, rv;
int slen;
struct addrinfo hints, si_other;
struct addrinfo *receiverinfo, *p;
state_t state;  // holds current state
float cw;   // current size of congestion window
uint32_t dup_ack;  // number of duplicate acks
float sst_curr; // current SST
int total_packet;
int seq_num;
unsigned long long int bytes_left; // number of bytes

timeval timeout;

void diep(char *s);
void state_transition(bool newACK, bool timeOut);
void duplicateACKHandler();
void timeoutHandler();
void sendPackets();
void populatePacketBuffer(int numPackets);
void timeoutRetransmit();


void diep(char *s) {
    perror(s);
    exit(1);
}

void state_transition(bool newACK, bool timeOut){
    switch(state) {
        //state = state;          //sus, lmao
        //we need to also base this off the current packet that has "come in"/ is going to be processed
        case SLOW_START:
            if(newACK == 1) {
                cw += 1;
                dup_ack = 0;
                state = SLOW_START;
                //send packets based on the window (send everything in your window)
                sendPackets();
            }
            else{
                dup_ack += 1;   //will check for dup_ack == 3 outside
                state = SLOW_START;
            }
            if(cw >= sst_curr) {
                state = CONGESTION_AVOIDENCE;
            }
            if(timeOut) {
                timeoutHandler();
            }

            if(dup_ack == 3) {
                duplicateACKHandler();
                state = FAST_RECOVERY;
            }
            break;
        case CONGESTION_AVOIDENCE:
            if(newACK) {
                cw = cw + 1/(floor(cw));
                dup_ack = 0;
                //transmit based on cw
                sendPackets();
                state = CONGESTION_AVOIDENCE;
            }
            else {
                dup_ack += 1;       //will check for dup_ack == 3 outside
                state = CONGESTION_AVOIDENCE;
            }
            if(dup_ack == 3){
                duplicateACKHandler();
                state = FAST_RECOVERY;
            }
            if(timeOut) {
                timeoutHandler();
            }
            break;
        case FAST_RECOVERY:
            if(timeOut) {
                timeoutHandler();
            }
            if(newACK) {
                dup_ack = 0;
                cw = sst_curr;
                //transmit packets???
                sendPackets();
                state = CONGESTION_AVOIDENCE;
            }
            else {
                cw += 1;
                //transmit packet??
                sendPackets();
                state = FAST_RECOVERY;
            }
            break;
        default:
            break;
    }
}

void duplicateACKHandler() {
    sst_curr = cw/2.0;

    /* Try to prevent sst_curr goes under 1.0 */
    // TODO: 
    if(sst_curr < 97.0){
        sst_curr = 97.0;
    }

    cw = sst_curr + 3;
    if(sendto(sockfd, (void *)&window_buffer_queue.front(), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1 ){
        diep((char *)"sendto in duplicateACKHandler failed. \n");
    }
    sendPackets(); 
}
void timeoutRetransmit(){
    for(int i = 0; i < window_buffer_queue.size(); i++){
        packet_t pkt = window_buffer_queue.front();
        if(sendto(sockfd, (void *)&pkt, sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1 ){
            diep((char *)"sendto in duplicateACKHandler failed. \n");
        }
        window_buffer_queue.pop();
        window_buffer_queue.push(pkt);
    }
}

void timeoutHandler() {
    sst_curr = cw/2.0;

    /* Try to prevent sst_curr goes under 1.0 */
    // TODO:
    if(sst_curr < 300.0){
        sst_curr = 300.0;
    }

    cw = sst_curr/3.0;
    dup_ack = 0;
    // if(sendto(sockfd, (void *)&window_buffer_queue.front(), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1 ){
    //     diep((char *)"sendto in timeoutHandler failed. \n");
    // }
    timeoutRetransmit();
    state = SLOW_START;
}

void sendPackets() {
    // printf("current state: %d\n", state);
    if(cw - window_buffer_queue.size() < 1){
        if(sendto(sockfd, (void*)(&window_buffer_queue.front()), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1) {
            diep((char *)"Error: sending data failed\n");
        }
        return;
    }

    int num_packets_send = 0;
    if(cw - window_buffer_queue.size() <= packet_buffer_queue.size()) {
        // printf("case1: %d\n", cw - window_buffer_queue.size());
        num_packets_send = cw - window_buffer_queue.size();
    }
    else {
        // printf("case2: %d\n", packet_buffer_queue.size());
        num_packets_send = packet_buffer_queue.size();
    }

    
    for(int i = 0; i < num_packets_send; i++) {
        memcpy((void*)(&current_packet), &packet_buffer_queue.front(), sizeof(packet_t));
        if(sendto(sockfd, (void*)(&current_packet), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1) {
            diep((char *)"Error: sending data failed\n");
        }
        // send new packet, add this packet to the waiting for ACK list.
        window_buffer_queue.push(packet_buffer_queue.front());
        packet_buffer_queue.pop();
    }
    populatePacketBuffer(num_packets_send);
}

void populatePacketBuffer(int num_packets){

    for(int i = 0; bytes_left != 0 && i < num_packets; i++) {
        packet_t packet;
        char buf[MAX_DATA_SIZE];
        int numToRead;
        if(bytes_left < MAX_DATA_SIZE) {
            numToRead = bytes_left;
        } else {
            numToRead = MAX_DATA_SIZE;
        }

        int numRead = fread(buf, sizeof(char), numToRead, fp);
        // printf("seq_num: %d, numRead: %d\n", seq_num, numRead);
        if(numRead > 0){
            memset(packet.data, 0, MAX_DATA_SIZE);
            memcpy(packet.data, buf, numRead);
            packet.sequence_no = seq_num;
            packet.ack_no = 0;
            packet.ptype = DATA;
            packet.payload_len = numRead;
            packet_buffer_queue.push(packet);
            seq_num += 1;
        }
        // printf("bytes_left: %d\n", bytes_left);
        bytes_left -= packet.payload_len;
        // printf("bytes_left_post: %d\n", bytes_left);
        if(bytes_left == 0) {
            break;
        }
    }
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    
    //handshake, copy from MP1
    
    char port_no[10];
    sprintf(port_no, "%d", hostUDPport);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
    memset(&receiverinfo, 0, sizeof(receiverinfo));
    if ((rv = getaddrinfo(hostname, port_no, &hints, &receiverinfo)) != 0) {
		diep((char *)"getaddrinfo failed");
	}

    // loop through all the results and bind to the first we can
	for(p = receiverinfo; p != NULL; p = p->ai_next) {
		// server have socket door that welcomes client's contact
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
	            diep((char *)"receiver error: unable to setup");
			    continue;
		}

		break;
	}

    if (p == NULL)  {
		fprintf(stderr, "receiver: failed to bind\n");
	}

    
    //Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

    // check file size
    // fseek(fp, 0, SEEK_END);
    // int filesize = ftell(fp);
    // fseek(fp, 0, SEEK_SET);

    // if (filesize < bytesToTransfer){
    //     bytesToTransfer = filesize;
    // }

    // initialize the globals
    seq_num = 0; 
    state = SLOW_START;
    cw = 100.0;
    dup_ack = 0;
    sst_curr = INIT_SST_THRESHOLD;
    total_packet = 0;
    bytes_left = bytesToTransfer;

    // total_packet = ceil(bytesToTransfer/MAX_DATA_SIZE);

	/* Determine how many bytes to transfer */

    // populate the queue with all the packets
    populatePacketBuffer(MAX_BUFFER_SIZE);
    // for(int i = 0; i < total_packet + 2; i++) {
    //     packet_t packet;
    //     char buf[MAX_DATA_SIZE];
    //     int numToRead;
    //     if(bytes_left < MAX_DATA_SIZE) {
    //         numToRead = bytes_left;
    //     } else {
    //         numToRead = MAX_DATA_SIZE;
    //     }

    //     int numRead = fread(buf, sizeof(char), numToRead, fp);
    //     // printf("seq_num: %d, numRead: %d\n", seq_num, numRead);
    //     if(numRead > 0){
    //         memset(packet.data, 0, MAX_DATA_SIZE);
    //         memcpy(packet.data, buf, numRead);
    //         packet.sequence_no = seq_num;
    //         packet.ack_no = 0;
    //         packet.ptype = DATA;
    //         packet.payload_len = numRead;
    //         packet_buffer_queue.push(packet);
    //         seq_num += 1;
    //     }
    //     // printf("bytes_left: %d\n", bytes_left);
    //     bytes_left -= packet.payload_len;
    //     // printf("bytes_left_post: %d\n", bytes_left);
    //     if(bytes_left <= 0) {
    //         break;
    //     }
    // }
    // printf("done populating queue\n");
    // set timeout using setsockopt
    timeout.tv_sec = 0;    //1 seconds timeout
    timeout.tv_usec = 2*20000;    //20 * 1000 (RTT according to MP2 documentation) * 3 (grace period)
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "setsockopt: %s\n", strerror(errno));
        return;
    }

    //send packets
    sendPackets();

    // trying to finish sending all the packets
    while(!packet_buffer_queue.empty() || !window_buffer_queue.empty()) {
        if(recvfrom(sockfd, (void*)(&current_packet), sizeof(packet_t), 0, (struct sockaddr *)&si_other, (socklen_t*)&slen) == -1) {
        //    printf("receive from receiver failed\n");
           state_transition(0,1);
           if(sendto(sockfd, (void*)(&window_buffer_queue.front()), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen) == -1) {
                diep((char *)"Error: sending data failed\n");
           }
        }
        else {
            if(current_packet.ptype == ACK) {
                //WE GET AN ACK PACKET BUT NEITHER OF THE BELOW CONDITIONS ARE MET!!!!!

                // printf("received ACK from receiver\n");
                // we are receiving duplicate acks
                // if(current_packet.ack_no == window_buffer_queue.front().sequence_no) {
                //     state_transition(1,0);
                // }
                // we are receiving a new ack
                if (current_packet.ack_no >= window_buffer_queue.front().sequence_no){
                    while(!window_buffer_queue.empty() && window_buffer_queue.front().sequence_no <= current_packet.ack_no){
                        window_buffer_queue.pop();
                        state_transition(1, 0);
                    }
                }
                else {
                    state_transition(0, 0);
                }
            }
        } 
    }

    // close file pointer
    fclose(fp);

    // tell the receiver that it is done sending data
    packet_t final_packet;
    final_packet.ptype = FIN;
    final_packet.payload_len = 0;

    // It could be better improved if we understand how to use timeout
    while(1){
        if(sendto(sockfd, (void *)(&final_packet), sizeof(packet_t), 0, p->ai_addr, p->ai_addrlen)== -1){
            diep((char *)"Can't send FIN\n");
        }

        if(recvfrom(sockfd, (void *)(&current_packet), sizeof(packet_t), 0, (struct sockaddr *)&si_other, (socklen_t*)&slen)== -1){
            diep((char *)"Can't receive FINACK from receiver\n");
        }
        else {
            if(current_packet.ptype == FINACK){
                printf("FINACK YAY! \n");
                break;
            }
        }
    }    

    close(sockfd);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);



    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);


    return (EXIT_SUCCESS);
}
