#include <iostream>
#include <type_traits>
#include <cstdint>
struct packet_base {};
#pragma pack(push, 1)
struct PacketHeader { uint32_t a; uint16_t b; };
struct PacketGuildAction : public packet_base { PacketHeader header; uint32_t msg_size; char target[12]; uint32_t amount; char item[20]; };
#pragma pack(pop)
int main() { std::cout << std::is_standard_layout<PacketGuildAction>::value << std::endl; return 0; }
