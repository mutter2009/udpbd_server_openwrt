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
 * UDPBD v2 Headers
 * 废弃位域，使用标准 16 位和 32 位整型，防止 MIPS 编译器位域翻转错位
 */

struct SUDPBDv2_Header {
    uint16_t cmd16;
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

struct SUDPBDv2_BlockType {
    uint32_t bt;
} __attribute__((__packed__));

#define UDP_MAX_PAYLOAD  1472
#define RDMA_MAX_PAYLOAD (UDP_MAX_PAYLOAD - sizeof(struct SUDPBDv2_Header) - sizeof(struct SUDPBDv2_BlockType))

struct SUDPBDv2_RDMA {
    struct SUDPBDv2_Header hdr;
    struct SUDPBDv2_BlockType bt;
    uint8_t data[RDMA_MAX_PAYLOAD];
} __attribute__((__packed__));

#endif
