#pragma once

union guid_t
{
    unsigned char data[16];
    unsigned int data4[4];
    unsigned long long data8[2];
};

guid_t GenerateGUID();

inline bool operator == ( const guid_t& a, const guid_t& b )
{
    return (a.data8[0] == b.data8[0]) && (a.data8[1] == b.data8[1]);
}

struct guid_string_t
{
    static constexpr unsigned SIZE = 64;
    char data[SIZE];
};
void ToString( guid_string_t* str, const guid_t& g );
void FromString( guid_t* g, const guid_string_t& str );
void FromString( guid_t* g, const char* str );