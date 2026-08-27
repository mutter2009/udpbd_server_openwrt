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
        printf(" - size = %ldMB / %ldMiB\n", _fsize / (1000*1000), _fsize / (1024*1024));
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
    uint32_t get_sector_count() {return _fsize/512;}

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
        
        // Optimize socket buffer for router performance
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
        
        // Alignment buffer for MIPS CPU
        uint8_t buf[BUFLEN] __attribute__((aligned(4)));

        printf("Server running on port %d (0x%x)\n", UDPBD_PORT, UDPBD_PORT);

        while (1) {
            if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
                throw runtime_error("recvfrom");
            }

            struct SUDPBDv2_Header *hdr = (struct SUDPBDv2_Header *)buf;

            switch (hdr->cmd) {
                case UDPBD_CMD_INFO:
                    handle_cmd_info(si_other, (struct SUDPBDv2_InfoRequest *)buf);
                    break;
                case UDPBD_CMD_READ:
                    handle_cmd_read(si_other, (struct SUDPBDv2_RWRequest *)buf);
                    break;
                case UDPBD_CMD_WRITE:
                    handle_cmd_write(si_other, (struct SUDPBDv2_RWRequest *)buf);
                    break;
                case UDPBD_CMD_WRITE_RDMA:
                    handle_cmd_write_rdma(si_other, (struct SUDPBDv2_RDMA *)buf);
                    break;
                default:
                    printf("Invalid cmd: 0x%x\n", hdr->cmd);
            };
        }
    }

private:
    void print_stats() {
        printf("Total read: %ld KiB, total write: %ld KiB\r", (long)_total_read/1024, (long)_total_write/1024);
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

    void handle_cmd_info(struct sockaddr_in &si_other, struct SUDPBDv2_InfoRequest *request) {
        struct SUDPBDv2_InfoReply reply;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &si_other.sin_addr, str, INET_ADDRSTRLEN);

        printf("\nUDPBD_CMD_INFO from %s\n", str);

        reply.hdr.cmd      = UDPBD_CMD_INFO_REPLY;
        reply.hdr.cmdid    = request->hdr.cmdid;
        reply.hdr.cmdpkt   = 1;
        
        // Convert integers to Little-Endian for PS2
        reply.sector_size  = htole32(_bd.get_sector_size());
        reply.sector_count = htole32(_bd.get_sector_count());

        if (SENDTO(s, &reply, sizeof(reply), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
            throw runtime_error("sendto");
        }
    }

    void handle_cmd_read(struct sockaddr_in &si_other, struct SUDPBDv2_RWRequest *request) {
        struct SUDPBDv2_RDMA reply;

        uint32_t sector_nr = le32toh(request->sector_nr);
        uint16_t sector_count = le16toh(request->sector_count);

        set_block_shift_sectors(sector_count);

        reply.hdr.cmd        = UDPBD_CMD_READ_RDMA;
        reply.hdr.cmdid      = request->hdr.cmdid;
        reply.hdr.cmdpkt     = 1;
        reply.bt.block_shift = _block_shift;

        uint32_t blocks_left = sector_count * _blocks_per_sector;
        _total_read += blocks_left * _block_size;
        print_stats();

        _bd.seek(sector_nr);

        while (blocks_left > 0) {
            reply.bt.block_count = (blocks_left > _blocks_per_packet) ? _blocks_per_packet : blocks_left;
            blocks_left -= reply.bt.block_count;

            _bd.read(reply.data, reply.bt.block_count * _block_size);

            if (SENDTO(s, &reply, sizeof(struct SUDPBDv2_Header) + 4 + (reply.bt.block_count * _block_size), 0, (struct sockaddr*) &si_other, sizeof(si_other)) == -1) {
                throw runtime_error("sendto");
            }
            reply.hdr.cmdpkt++;
        }
    }

    void handle_cmd_write(struct sockaddr_in &si_other, struct SUDPBDv2_RWRequest *request) {
        uint32_t sector_nr = le32toh(request->sector_nr);
        uint16_t sector_count = le16toh(request->sector_count);

        _bd.seek(sector_nr);
        _write_size_left = sector_count * 512;
        _total_write += _write_size_left;
        print_stats();
    }

    void handle_cmd_write_rdma(struct sockaddr_in &si_other, struct SUDPBDv2_RDMA *request) {
        size_t size = request->bt.block_count * (1 << (request->bt.block_shift + 2));
        _bd.write(request->data, size);
        _write_size_left -= size;

        if(_write_size_left == 0) {
            struct SUDPBDv2_WriteDone reply;
            reply.hdr.cmd      = UDPBD_CMD_WRITE_DONE;
            reply.hdr.cmdid    = request->hdr.cmdid;
            reply.hdr.cmdpkt   = request->hdr.cmdid + 1;
            reply.result       = 0;

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
