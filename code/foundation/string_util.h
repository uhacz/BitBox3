#pragma once

struct BXIAllocator;
namespace string
{
    char*    token    ( char* str, char* tok, size_t toklen, char* delim );
    char*    duplicate( char* old_string, const char* new_string, BXIAllocator* allocator );
    char*    create   ( const char* str, size_t len, BXIAllocator* allocator );
    int      append   ( char* buffer, int size_buffer, const char* format, ... );
    unsigned count    ( const char* str, const char c );
    unsigned length   ( const char* str );
    void     free     ( char* str, BXIAllocator* allocator );
    bool     equal    ( const char* str0, const char* str1 );
    char*    find     ( const char* str, const char* to_find, char** next = 0 );

    inline void free_and_null( char** str, BXIAllocator* allocator ) { string::free( str[0], allocator ); str[0] = 0; }
}

struct string_t
{
    static constexpr unsigned MAX_STATIC_LENGTH = 15;
    static constexpr unsigned MAX_STATIC_SIZE = MAX_STATIC_LENGTH + 1;
    BXIAllocator* _allocator = nullptr;
    union
    {
        char _static[MAX_STATIC_SIZE] = {};
        char* _dynamic;

    };

    string_t();
    string_t( const char* str );
    string_t( const string_t& other );
    string_t( string_t&& other );

    ~string_t();

    string_t& operator = ( const string_t& other );
    string_t& operator = ( string_t&& other );

          char* c_str()       { return (_allocator) ? _dynamic : _static; }
    const char* c_str() const { return (_allocator) ? _dynamic : _static; }
    unsigned length() const;
};

struct string_buffer_t
{
    char* _base = nullptr;
    unsigned _offset = 0;
    unsigned _capacity = 0;
    BXIAllocator* _allocator = nullptr;


    ~string_buffer_t();
    bool null() const { return _base == nullptr; }
};
struct string_buffer_it
{
    char* pointer = nullptr;
    unsigned offset = 0;

    bool null() const { return pointer == nullptr; }
};

namespace string
{
    void create( string_t* s, const char* data, BXIAllocator* allocator );
    void reserve( string_t* s, unsigned length, BXIAllocator* allocator );
    void copy( string_t* dst, const string_t& src );
    void move( string_t* dst, string_t&& src );
    void free( string_t* s );

    void create( string_buffer_t* s, unsigned capacity, BXIAllocator* allocator );
    void free( string_buffer_t* s );

    const char* append( string_buffer_t* s, const char* str, char terminator = '\0' );
    const char* appendn( string_buffer_t* s, const char* str, unsigned len, char terminator = '\0' );

    string_buffer_it iterate( const string_buffer_t& s, const string_buffer_it current = string_buffer_it() );
}