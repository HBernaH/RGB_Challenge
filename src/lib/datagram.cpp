#include <cmath>
#include <fstream>
#include <iostream>
#include <cstring>
#include "datagram.hpp"
#include "file_tools.hpp"

using namespace std;

// Packet class methods
Packet::Packet(uint8_t t, uint16_t s)
{
    type = t;
    size = s;
}

uint8_t Packet::get_type()
{
    return type;
}

uint16_t Packet::get_size()
{
    return size;
}

vector<uint8_t> &Packet::get_payload()
{
    return payload;
}

void Packet::set_payload(const uint8_t *bytes, size_t length)
{
    payload.assign(bytes, bytes + length);
}

Packet Packet::create_connect_packet()
{
    Packet packet = Packet(0, 0);
    return packet;
}

Packet Packet::create_disconnect_packet()
{
    Packet packet = Packet(1, 0);
    return packet;
}

Packet Packet::create_res_packet()
{
    Packet packet = Packet(2, 8);
    return packet;
}

Packet Packet::create_file_ack_packet()
{
    Packet packet = Packet(3, 0);
    return packet;
}

Packet Packet::create_starting_file_packet(uint8_t color_code)
{
    Packet packet = Packet(5, 1);

    std::vector<uint8_t> data = {color_code};

    packet.get_payload() = data;

    return packet;
}

Packet Packet::create_ending_file_packet()
{
    Packet packet = Packet(6, 0);
    return packet;
}

vector<Packet> Packet::create_file_packets(File &file)
{

    int file_size = file.get_size();
    int num_packets = (file_size + 509 - 1) / 509;

    vector<Packet> packets;

    ifstream file_stream(file.get_fullpath(), std::ios::binary);
    if (!file_stream.is_open())
    {
        throw runtime_error("Failed to open" + file.get_filename() + ".\n");
    }

    uint8_t buffer[509];
    for (int packets_made = 0; packets_made < num_packets - 1; packets_made++)
    {
        Packet packet(4, 509);

        file_stream.read(reinterpret_cast<char *>(buffer), 509);
        packet.set_payload(buffer, 509);
        packets.push_back(packet);
    }

    int bytes_remaining = file_size - (num_packets - 1) * 509;
    if (bytes_remaining > 0)
    {

        Packet last_packet(4, bytes_remaining);
        file_stream.read(reinterpret_cast<char *>(buffer), bytes_remaining);
        last_packet.set_payload(buffer, bytes_remaining);
        packets.push_back(last_packet);
    }
    file_stream.close();
    return packets;
}

vector<uint8_t> Packet::serialize_packet(Packet packet)
{
    vector<uint8_t> buffer;

    ssize_t size = packet.get_size();

    // Type(1byte) + Size(2bytes) + Payload(N bytes);
    // Type
    buffer.push_back(packet.get_type());

    // Size
    buffer.push_back(static_cast<uint8_t>(packet.get_size() & 0xFF));        // Keep lower bits (first byte)
    buffer.push_back(static_cast<uint8_t>((packet.get_size() >> 8) & 0xFF)); // Shift right (second byte)

    if (size > 0)
        // Payload
        buffer.insert(buffer.end(), packet.get_payload().begin(), packet.get_payload().end());

    return buffer;
}

int deserialize_type(const vector<uint8_t> &data)
{
    return static_cast<int>(data[0]);
}
// other functions
int deserialize_size(const vector<uint8_t> &data)
{
    if (data.size() < 3)
        throw runtime_error("Invalid packet size!");

    uint16_t size = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[2]) << 8);

    return static_cast<int>(size);
}

vector<uint8_t> deserialize_payload(const vector<uint8_t> &data, int payload_size)
{
    std::vector<uint8_t> payload;
    if (data.size() < 3 + payload_size)
    {
        throw std::runtime_error("Payload truncated!");
    }

    // Copy the range of bytes: [index 3, index 3 + payload_size)
    auto start_it = data.begin() + 3;
    auto end_it = start_it + payload_size;

    return vector<uint8_t>(start_it, end_it);
}
