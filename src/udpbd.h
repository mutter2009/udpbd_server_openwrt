#ifndef UDPBD_H
#define UDPBD_H

#include <stdint.h>
#include <endian.h>

#define UDPBD_PORT            0xBDBD

#define UDPBD_CMD_INFO        0x00
#define UDPBD_CMD_INFO_REPLY  0x01
#define UDPBD_CMD_READ        0x02
#define UDPBD_CMD_READ_RDMA   0x03
#define UDPBD_CMD_WRITE       0x04
#define UDPBD_CMD_WRITE_RDMA  0x05
#define UDPBD_CMD_WRITE_DONE  0x06

#define UDPBD_MAX_SECTOR_READ  512

/*
 * UDPBD v2 Headers (Fixed for Big-Endian Architectures)
 */

struct SUDPBDv2_Header {
    union
    {
        uint16_t cmd16;
        struct
        {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            uint16_t cmdpkt : 8; // 0..255
            uint16_t cmdid  : 3; // 0..8
            uint16_t cmd    : 5; // 0..31
#else
            uint16_t cmd    : 5;
            uint16_t cmdid  : 3;
            uint16_t cmdpkt : 8;
#endif
        };
    };	
} __attribute__((__packed__));

struct SUDPBDv2_InfoRequest {
	struct SUDPBDv2_Header hdr;
} __attribute__((__packed__));

struct SUDPBDv2_InfoReply {
	struct SUDPBDv2_Header hdr;
	uint32_t sector_size;
	uint32_t sector_count;
} __attribute__((__packed__));

struct SUDPBDv2_RWRequest {
	struct SUDPBDv2_Header hdr;
	uint32_t sector_nr;
	uint16_t sector_count;
} __attribute__((__packed__));

struct SUDPBDv2_WriteDone {
	struct SUDPBDv2_Header hdr;
	int32_t result;
} __attribute__((__packed__));

union block_type
{
    uint32_t bt;
    struct
    {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint32_t spare       : 19;
        uint32_t block_count :  9;
        uint32_t block_shift :  4;
#else
        uint32_t block_shift :  4;
        uint32_t block_count :  9;
        uint32_t spare       : 19;
#endif
    };
};	

#define UDP_MAX_PAYLOAD  1472
#define RDMA_MAX_PAYLOAD (UDP_MAX_PAYLOAD - sizeof(struct SUDPBDv2_Header) - sizeof(union block_type))

struct SUDPBDv2_RDMA {
	struct SUDPBDv2_Header hdr;
    union block_type bt;
    uint8_t data[RDMA_MAX_PAYLOAD];
} __attribute__((__packed__));

#endif
