#ifndef DATAGRAM_HPP
#define DATAGRAM_HPP

#include <cstdint>
#include <vector>

class File;

struct Packet
{
private:
    std::uint8_t type;
    std::uint16_t size;
    std::vector<std::uint8_t> payload;

public:
    Packet(uint8_t type, uint16_t size);

    std::uint8_t get_type();
    std::uint16_t get_size();
    std::vector<std::uint8_t> &get_payload();

    void set_payload(const std::uint8_t *bytes, size_t length);

    static Packet create_res_packet();
    static Packet create_file_ack_packet();
    static Packet create_connect_packet();
    static Packet create_disconnect_packet();
    static Packet create_starting_file_packet(std::uint8_t color_code);
    static Packet create_ending_file_packet();
    static std::vector<Packet> create_file_packets(File &file);
    static std::vector<uint8_t> serialize_packet(Packet packet);
};

int deserialize_type(const std::vector<uint8_t> &data);
int deserialize_size(const std::vector<uint8_t> &data);
std::vector<uint8_t> deserialize_payload(const std::vector<uint8_t> &data, int payload_size);

#endif