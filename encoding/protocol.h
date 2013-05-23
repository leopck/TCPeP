#ifndef _PROTOCOL_
#define _PROTOCOL_
#include "utils.h"
#include "packet.h"
#include "decoding.h"
#include "encoding.h"

#define TYPE_DATA 0x00
#define TYPE_ACK 0x01
#define TYPE_CLOSE 0x02
#define TYPE_CLOSEAWAITING 0x03
#define TYPE_EMPTY 0x05


#define STATE_CLOSEAWAITING 0x01
#define STATE_OPENED_SIMPLEX 0x02
#define STATE_OPENED_DUPLEX 0x03
#define STATE_INIT 0x04


typedef struct muxstate_t {
    int sock_fd;    // local TCP socket
    
    // From the perspective of the client :
    uint16_t sport; // Source port (client application TCP socket)
    uint16_t dport; // Destination port (Server application)
    uint32_t remote_ip; // Remote IP Address (Server Address)
    struct sockaddr_in udpRemote; // Remote UDP endpoint : either client system or proxy system
    
    uint16_t randomId; // Random connection identifier
    
    // Encoder and decoder structures
    encoderstate* encoderState;
    decoderstate* decoderState;

    // State of the connection
    int state;
} muxstate;

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, uint16_t randomId, int sock_fd, muxstate** statesTable, int* tableLength, struct sockaddr_in udpRemoteAddr);
void removeMux(int i, muxstate** statesTable, int* tableLength);

void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type);

int muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type);

void printMux(muxstate mux);
#endif



