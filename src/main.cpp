#include <iostream>
#include <exception>
#include <stdexcept>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <endian.h>

#include "udpbd.h"

#define BUFLEN  2048

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#define loff_t __int64
#define SENDTO(s, buf, len, flags, addr, addrlen) sendto(s, (const char*)(buf), len, flags, addr, addrlen)
#define SETSOCKOPT(s, lvl, opt, val, vlen) setsockopt(s, lvl, opt, (char*)(val), vlen)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#define SENDTO(s, buf, len, flags, addr, addrlen) sendto(s, buf, len, flags, addr, addrlen)
#define SETSOCKOPT(s, lvl, opt, val, vlen) setsockopt(s, lvl, opt, val, vlen)
#endif

using namespace std;

// 辅助位运算函数：拼装 OPL v2 报头 (cmd:5bit, cmdid:3bit, cmdpkt:8bit)
static inline uint16_t pack_hdr(uint8_t cmd, uint8_t cmdid, uint8_t cmdpkt) {
    uint16_t val = (cmd & 0x1F) | ((cmdid & 0x07) << 5) | ((cmdpkt & 0xFF) << 8);
    return htole16(val);
}

static inline void unpack_hdr(uint16_t raw_cmd16, uint8_t *cmd, uint8_t *cmdid, uint8_t *cmdpkt) {
    uint16_t val = le16toh(raw_cmd16);
    if (cmd)    *cmd    = val & 0x1F;
    if (cmdid)  *cmdid  = (val >> 5) & 0x07;
    if (cmdpkt) *cmdpkt = (val >> 8) & 0xFF;
}

// 辅助位运算函数：拼装 RDMA block_type (shift:4bit, count:9bit, spare:19bit)
static inline uint32_t pack_bt(uint8_t shift, uint16_t count) {
    uint32_t val = (shift & 0x0F) | ((count & 0x01FF) << 4);
    return htole32(val);
}

class CBlockDevice
{
public:
    CBlockDevice(const char *sFileName) : _read_only(false) {
        _fp = open(sFileName, _read_only ? O_RDONLY : O_RDWR);
        if (_fp < 0) {
            _read_only = true;
            _fp = open(sFileName, _read_only ? O_RDONLY : O_RDWR);
            if (_fp < 0)
                throw runtime_error(string("unable to open file ") + sFileName);
        }

        _fsize = lseek(_fp, 0, SEEK_END);
        lseek(_fp, 0, SEEK_SET);

        printf("Opened '%s' as Block Device\n", sFileName);
        printf(" - %s\n", _read_only ? "read-only" : "read/write");
        printf(" - size = %ldMB / %ldMiB\n", (long)(_fsize / (1000*1000)), (long)(_fsize / (1024*1024)));
    }

    ~CBlockDevice() {
        close(_fp);
    }

    void seek(uint32_t sector) {
        loff_t offset = (loff_t)sector * 512;
        lseek(_fp, offset, SEEK_SET);
    }

    void read(void *data, size_t size) {
        ssize_t rv = ::read(_fp, data, size);
        if (rv != (ssize_t)size)
            printf("read error %ld != %ld\n", (long)rv, (long)size);
    }

    void write(const void *data, size_t size) {
        ssize_t rv = ::write(_fp, data, size);
        if (rv != (ssize_t)size)
            printf("write error %ld != %ld\n", (long)rv, (long)size);
    }

    uint32_t get_sector_size()  {return 512;}
    uint32_t get_sector_count() {return _fsize / 512;}

private:
    int _fp;
    bool _read_only;
    loff_t _fsize;
};

class CUDPBDServer
{
public:
    CUDPBDServer(class CBlockDevice &bd) : _bd(bd), _block_shift(0), _total_read(0), _total_write(0) {
        set_block_shift(5);
        struct sockaddr_in si_me;

        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            throw runtime_error("socket");
        }

        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(UDPBD_PORT);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(s, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
            throw runtime_error("bind");
        }

        int broadcastEnable=1;
        SETSOCKOPT(s, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
        
        int rcvbuf = 256 * 1024;
        SETSOCKOPT(s, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));
    }

    ~CUDPBDServer() {
        close(s);
    }

    void run() {
        struct sockaddr_in si_other;
        socklen_t slen = sizeof(si_other);
        int recv_len;
        
        uint8_t buf[BUFLEN] __attribute__((aligned(4)));

        printf("Server running on port %d (0x%x)\n", UDPBD_PORT, UDPBD_PORT);

        while (1) {
            if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
                throw runtime_error("recvfrom");
            }

            struct SUDPBDv2_Header *hdr = (struct SUDPBDv2_Header *)buf;
            uint8_t cmd, cmdid, cmdpkt;
            unpack_hdr(hdr->cmd16, &cmd, &cmdid, &cmdpkt);

            switch (cmd) {
                case UDPBD_CMD_INFO:
                    handle_cmd_info(si_other, (struct SUDPBDv2_InfoRequest *)buf, cmdid);
                    break;
                case UDPBD_CMD_READ:
                    handle_cmd_read(si_other, (struct SUDPBDv2_RWRequest *)buf, cmdid);
                    break;
                case UDPBD_CMD_WRITE:
                    handle_cmd_write(si_other, (struct SUDPBDv2_RWRequest *)buf, cmdid);
                    break;
                case UDPBD_CMD_WRITE_RDMA:
                    handle_cmd_write_rdma(si_other, (struct SUDPBDv2_RDMA *)buf, cmdid);
                    break;
                default:
                    printf("Invalid cmd: 0x%x\n", cmd);
            };
        }
    }

private:
    void print_stats() {
        printf("Total read: %ld KiB, total write: %ld KiB\r", (long)(_total_read/1024), (long)(_total_write/1024));
        fflush(stdout);
    }

    void set_block_shift(uint32_t shift) {
        if (shift != _block_shift) {
            _block_shift       = shift;
            _block_size        = 1 << (_block_shift + 2);
            _blocks_per_packet = RDMA_MAX_PAYLOAD / _block_size;
            _blocks_per_sector = _bd.get_sector_size() / _block_size;
        }
    }

    void set_block_shift_sectors(uint32_t sectors) {
        uint32_t shift;
        uint32_t size = sectors * 512;
        uint32_t packetsMIN  = (size + 1440 - 1) / 1440;
        uint32_t packets128 = (size + 1408 - 1) / 1408;
        uint32_t packets256 = (size + 1280 - 1) / 1280;
        uint32_t packets512 = (size + 1024 - 1) / 1024;

        if (packets512 == packetsMIN) shift = 7;
        else if (packets256 == packetsMIN) shift = 6;
        else if (packets128 == packetsMIN) shift = 5;
        else shift = 3;

        set_block_shift(shift);
    }

    void handle_cmd_info(struct sockaddr_in &si_other, struct SUDPBDv2_InfoRequest *request, uint8_t cmdid) {
        struct SUDPBDv2_InfoReply reply;
        memset(&reply, 0, sizeof(reply));

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &si_other.sin_addr, str, INET_ADDRSTRLEN);
        printf("\nUDPBD_CMD_INFO from %s\n", str);

        // 拼接打包 Header，确保字节序在 MIPS 下完全正确
        reply.hdr.cmd16    = pack_hdr(UDPBD_CMD_INFO_REPLY, cmdid, 1);
        reply.sector_size  = htole32(_bd.get_sector_size());
        reply.sector_count = htole32(_bd.get_sector_count());

        if (SENDTO(s, &reply, sizeof(reply), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
            throw runtime_error("sendto");
        }
    }

    void handle_cmd_read(struct sockaddr_in &si_other, struct SUDPBDv2_RWRequest *request, uint8_t cmdid) {
        struct SUDPBDv2_RDMA reply;

        uint32_t sector_nr = le32toh(request->sector_nr);
        uint16_t sector_count = le16toh(request->sector_count);

        set_block_shift_sectors(sector_count);

        uint32_t blocks_left = sector_count * _blocks_per_sector;
        _total_read += blocks_left * _block_size;
        print_stats();

        _bd.seek(sector_nr);

        uint16_t pkt_count = 1;
        while (blocks_left > 0) {
            uint32_t cur_blocks = (blocks_left > _blocks_per_packet) ? _blocks_per_packet : blocks_left;
            
            reply.hdr.cmd16 = pack_hdr(UDPBD_CMD_READ_RDMA, cmdid, pkt_count);
            reply.bt.bt     = pack_bt(_block_shift, cur_blocks);

            blocks_left -= cur_blocks;

            _bd.read(reply.data, cur_blocks * _block_size);

            if (SENDTO(s, &reply, sizeof(struct SUDPBDv2_Header) + sizeof(struct SUDPBDv2_BlockType) + (cur_blocks * _block_size), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
                throw runtime_error("sendto");
            }
            pkt_count++;
        }
    }

    void handle_cmd_write(struct sockaddr_in &si_other, struct SUDPBDv2_RWRequest *request, uint8_t cmdid) {
        uint32_t sector_nr = le32toh(request->sector_nr);
        uint16_t sector_count = le16toh(request->sector_count);

        _bd.seek(sector_nr);
        _write_size_left = sector_count * 512;
        _total_write += _write_size_left;
        print_stats();
    }

    void handle_cmd_write_rdma(struct sockaddr_in &si_other, struct SUDPBDv2_RDMA *request, uint8_t cmdid) {
        uint32_t raw_bt = le32toh(request->bt.bt);
        uint8_t shift = raw_bt & 0x0F;
        uint16_t block_count = (raw_bt >> 4) & 0x01FF;

        size_t size = block_count * (1 << (shift + 2));
        _bd.write(request->data, size);
        _write_size_left -= size;

        if(_write_size_left == 0) {
            struct SUDPBDv2_WriteDone reply;
            memset(&reply, 0, sizeof(reply));
            reply.hdr.cmd16 = pack_hdr(UDPBD_CMD_WRITE_DONE, cmdid, cmdid + 1);
            reply.result    = 0;

            if (SENDTO(s, &reply, sizeof(reply), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
                throw runtime_error("sendto");
            }
        }
    }

    class CBlockDevice &_bd;
    uint32_t _block_shift;
    uint32_t _block_size;
    uint32_t _blocks_per_packet;
    uint32_t _blocks_per_sector;
    int s;

    uint64_t _total_read;
    uint64_t _total_write;
    uint32_t _write_size_left;
};

int main(int argc, char * argv[])
{
    if (argc < 2) {
        printf("Usage:\n  %s <file|/dev/sdX>\n", argv[0]);
        return -1;
    }

    try {
        class CBlockDevice bd(argv[1]);
        class CUDPBDServer srv(bd);
        srv.run();
    } catch (exception& e) {
        cout << e.what() << '\n';
        return -2;
    }

    return 0;
}
