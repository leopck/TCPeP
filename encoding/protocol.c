#include "protocol.h"

void printMux(muxstate mux){
    printf("Mux :\n");
    printf("\tsock_fd = %d\n", mux.sock_fd);
    printf("\tsport = %u\n", mux.sport);
    printf("\tdport = %u\n", mux.dport);
    printf("\tremote_ip = %u\n", mux.remote_ip);
    printf("\tremote udp = %u\n", (unsigned int)mux.udpRemote.sin_addr.s_addr);
    printf("\tRandom ID = %u\n", mux.randomId);
    
    switch(mux.state){
        case STATE_INIT:
            printf("\tState init\n");
            break;
        case STATE_CLOSEAWAITING:
            printf("\tState openackawaiting\n");
            break;
        case STATE_OPENED_DUPLEX:
            printf("\tState opened duplex\n");
            break;
        case STATE_OPENED_SIMPLEX:
            printf("\tState opened simplex\n");
            break;
        default:
            printf("\tState unknown\n");
    }
    
    encoderStatePrint(*(mux.encoderState));
    decoderStatePrint(*(mux.decoderState));
}

int assignMux(uint16_t sport, uint16_t dport, uint32_t remote_ip, uint16_t randomId, int sock_fd, muxstate** statesTable, int* tableLength, struct sockaddr_in udpRemoteAddr){
    // If the mux is already known, return its index, otherwise create it
    int i;
    
    for(i=0; i<(*tableLength); i++){
        if(
            ((*statesTable)[i].sport == sport) &&
            ((*statesTable)[i].dport == dport) &&
            ((*statesTable)[i].remote_ip == remote_ip) &&
            ((*statesTable)[i].udpRemote.sin_addr.s_addr == udpRemoteAddr.sin_addr.s_addr) &&
            ((*statesTable)[i].udpRemote.sin_port == udpRemoteAddr.sin_port) &&
            ((*statesTable)[i].randomId == randomId)
        ){
            //printf("Corresponding mux found : %d\n", i);
            //printMux((*statesTable)[i]);
            return i;
        }
    }
    
    printf("No existing mux ; create one\n");
    (*tableLength)++;
    *statesTable = realloc(*statesTable, (*tableLength) * sizeof(muxstate));
    (*statesTable)[(*tableLength) - 1].sport = sport;
    (*statesTable)[(*tableLength) - 1].dport = dport;
    (*statesTable)[(*tableLength) - 1].remote_ip = remote_ip;
    (*statesTable)[(*tableLength) - 1].sock_fd = sock_fd;
    (*statesTable)[(*tableLength) - 1].randomId = randomId;
    (*statesTable)[(*tableLength) - 1].encoderState = encoderStateInit();
    (*statesTable)[(*tableLength) - 1].decoderState = decoderStateInit();
    (*statesTable)[(*tableLength) - 1].state = STATE_INIT;
    
    memset(&((*statesTable)[(*tableLength) - 1].udpRemote), 0, sizeof((*statesTable)[(*tableLength) - 1].udpRemote));
    (*statesTable)[(*tableLength) - 1].udpRemote.sin_family = AF_INET;
    (*statesTable)[(*tableLength) - 1].udpRemote.sin_addr.s_addr = udpRemoteAddr.sin_addr.s_addr;
    (*statesTable)[(*tableLength) - 1].udpRemote.sin_port = udpRemoteAddr.sin_port;
    
    //printMux((*statesTable)[(*tableLength) - 1]);
    
    return (*tableLength) - 1;
}

void removeMux(int index, muxstate** statesTable, int* tableLength){
    if(index >= (*tableLength)){
        my_err("in removeMux : index>= size\n");
        exit(1);
    } else {
        int i;
        
        if((*statesTable)[index].sock_fd != -1){ // Make sure that the file descriptor really points to something
            if(close((*statesTable)[index].sock_fd) != 0){ // Try to close
                perror("In removeMux : error while close()ing");
                exit(1); // DIE !
            }
        } else {
            printf("Removing a Mux whithout opened socket (fd == -1)\n");
        }
        
        encoderStateFree((*statesTable)[index].encoderState);
        decoderStateFree((*statesTable)[index].decoderState);
        for(i = index; i < ((*tableLength) - 1); i++){
            (*statesTable)[i] = (*statesTable)[i + 1];
        }
        (*tableLength)--;
        *statesTable = realloc(*statesTable, (*tableLength) * sizeof(muxstate));
    }
}


void bufferToMuxed(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate mux, uint8_t type){
    uint16_t tmp16;
    uint32_t tmp32;
    uint8_t tmp8;
    
    tmp16 = htons(mux.sport);
    memcpy(dst, &tmp16, 2);
    tmp16 = htons(mux.dport);
    memcpy(dst + 2, &tmp16, 2);
    tmp32 = htonl(mux.remote_ip);
    memcpy(dst + 4, &tmp32, 4);
    tmp8 = type;
    memcpy(dst + 8, &tmp8, 1);
    tmp16 = htons(mux.randomId);
    memcpy(dst + 9, &tmp16, 2);
    
    memcpy(dst + 11, src, srcLen);
    
    (*dstLen) = srcLen + 11;
}

int muxedToBuffer(uint8_t* src, uint8_t* dst, int srcLen, int* dstLen, muxstate* mux, uint8_t* type){
    uint16_t tmp16;
    uint32_t tmp32;
    uint8_t tmp8;
    
    if(srcLen >= 11){
        memcpy(&tmp16, src, 2);
        mux->sport = ntohs(tmp16);
        memcpy(&tmp16, src + 2, 2);
        mux->dport = ntohs(tmp16);
        memcpy(&tmp32, src + 4, 4);
        mux->remote_ip = ntohl(tmp32);
        memcpy(&tmp8, src + 8, 1);
        (*type) = tmp8;
        memcpy(&tmp16, src + 9, 2);
        mux->randomId = ntohs(tmp16);
        if(  (*type == TYPE_ACK) // Test if the buffer indicates a legitimate type
          || (*type == TYPE_CLOSE)
          || (*type == TYPE_CLOSEAWAITING)
          || (*type == TYPE_DATA)
          || (*type == TYPE_EMPTY)
        ){
            memcpy(dst, src + 11, srcLen - 11);
            (*dstLen) = srcLen - 11;
            return true;
        } else {
            printf("In protocol.c/muxedToBuffer : We received an incoherent buffer. DIE.\n");
            exit(1);
        }
    }
    
    return false;
}
