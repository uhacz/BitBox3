#pragma once

#include "type.h"
#include "debug.h"
#include <memory/memory.h>

//
// dynamic array
//
template< typename T >
struct array_t
{
    using type_t = T;

    uint32_t size;
    uint32_t capacity;
    T* data;
    BXIAllocator* allocator;

    explicit array_t( BXIAllocator* alloc = BXDefaultAllocator() )
        : size( 0 ), capacity( 0 ), allocator( alloc ), data( 0 ) 
    {
        SYS_ASSERT( alloc != nullptr );
    }

    ~array_t()
    {
        BX_FREE0( allocator, data );
    }

    array_t( array_t<T>&& other )
    {
        size = other.size;
        capacity = other.capacity;
        data = other.data;
        allocator = other.allocator;

        other.size = 0;
        other.capacity = 0;
        other.data = nullptr;
    }

    array_t<T>& operator = ( array_t<T>&& other )
    {
        size = other.size;
        capacity = other.capacity;
        data = other.data;
        allocator = other.allocator;

        other.size = 0;
        other.capacity = 0;
        other.data = nullptr;
        return *this;
    }

          T &operator[]( int i) { return data[i]; }
    const T &operator[]( int i) const { return data[i]; }

    T* begin() { return data; }
    T* end  () { return data + size; }

    const T* begin() const { return data; }
    const T* end  () const { return data + size; }
};

template< typename T >
struct c_array_t
{
    u32 size = 0;
    u32 capacity = 0;
    BXIAllocator* allocator = nullptr;
    
    static constexpr u32 alignment = ALIGNOF( T );
    BIT_ALIGNMENT( alignment ) T data[1];

    c_array_t() {}
    ~c_array_t() {}

          T &operator[]( int i ) { return data[i]; }
    const T &operator[]( int i ) const { return data[i]; }

    T* begin() { return &data[0]; }
    T* end() { return &data[0] + size; }

    const T* begin() const { return &data[0]; }
    const T* end() const { return &data[0] + size; }
};

//
// static array
//
template< typename T, uint32_t MAX >
struct static_array_t
{
    using type_t = T;

    static_array_t() {}
    static_array_t( uint32_t initial_size )
        : size( initial_size ) {}
    
    T data[MAX];
    uint32_t size = 0;

    T &operator[]( int i ) { return data[i]; }
    const T &operator[]( int i ) const { return data[i]; }

    T* begin() { return data; }
    T* end() { return data + size; }

    const T* begin() const { return data; }
    const T* end() const { return data + size; }
};
namespace array
{
    template< typename Tarray >
    inline void zero( Tarray& arr )
    {
        memset( arr.begin(), 0x00, arr.size * sizeof( Tarray::type_t ) );
    }
}


//
// array span
//
template< class T >
struct array_span_t
{
    using type_t = T;

    array_span_t()
        : _begin( nullptr ), _size( 0 )
    {}

    explicit array_span_t( T* b, uint32_t size )
        : _begin( b ), _size( size )
    {}

    explicit array_span_t( T* b, T* e )
        : _begin( b ), _size( (uint32_t)(ptrdiff_t)(e - b) )

    {}
    explicit array_span_t( const array_t<T>& arr )
        : _begin( arr.begin() ), _size( arr.size )
    {}

    template< uint32_t MAX>
    explicit array_span_t( const static_array_t<T, MAX>& arr )
        : _begin( arr.begin() ), _size( arr.size )
    {}

    T* begin() { return _begin; }
    T* end() { return _begin + _size; }

    const T* begin() const { return _begin; }
    const T* end()   const { return _begin + _size; }

    uint32_t size() const { return _size; }

    T& operator[] ( uint32_t i ) 
    { 
        SYS_ASSERT( i < _size );
        return _begin[i]; 
    }
    const T& operator[] ( uint32_t i ) const 
    { 
        SYS_ASSERT( i < _size );
        return _begin[i]; 
    }

private:
    T* _begin = nullptr;
    uint32_t _size = 0;
};

template< typename T >
inline array_span_t<T> to_array_span( const Blob& blob )
{
    SYS_ASSERT( (blob.size % sizeof( T )) == 0 );
    return array_span_t<T>( (T*)blob.data, (T*)( blob.data + blob.size ) );
}

template< typename T, typename U >
inline array_span_t<T> to_array_span( array_t<U>& arr ) 
{ 
    SYS_STATIC_ASSERT( sizeof( T ) == sizeof( U ) );
    return array_span_t<T>( (T*)arr.begin(), (T*)arr.end() ); 
}

template< typename T >
inline array_span_t<const T> to_array_span( const array_t<T>& arr ) { return array_span_t<const T>( arr.begin(), arr.end() ); }

template< typename T >
inline array_span_t<const T> to_array_span( const T* begin, uint32_t size ) { return array_span_t<const T>( begin, size ); }

template< typename T >
inline array_span_t<T> to_array_span( T* begin, uint32_t size ) { return array_span_t<T>( begin, size ); }

template< typename T, typename TBlob >
inline array_span_t<T> to_array_span( const TBlob& blob, uint32_t num_elements )
{
    SYS_ASSERT( blob.size >= num_elements * sizeof( T ) );
    return array_span_t<T>( (T*)blob.raw, num_elements );
}


template< typename T >
struct queue_t
{
    using type_t = T;

    array_t<T> data;
    uint32_t size;
    uint32_t offset;

    explicit queue_t( BXIAllocator* alloc = BXDefaultAllocator() )
        : data( alloc ) , size( 0 ) , offset( 0 )
    {}
    ~queue_t()
    {}

          T& operator[]( int i )        { return data[(i + offset) % data.size]; }
    const T& operator[]( int i ) const  { return data[(i + offset) % data.size]; }
};

template<typename T, typename K = u64>
struct hash_t
{
    using type_t = T;
    using key_t = K;

    hash_t( BXIAllocator* a = BXDefaultAllocator() )
        : _hash( a )
        , _data( a )
    {}

    struct Entry {
        key_t key;
        uint32_t next;
        type_t value;
    };

    array_t<uint32_t> _hash;
    array_t<Entry> _data;
};

struct data_buffer_t
{
    uint8_t* data = nullptr;
    uint32_t write_offset = 0;
    uint32_t read_offset = 0;
    uint32_t capacity = 0;

    uint32_t alignment = 0;
    BXIAllocator* allocator = nullptr;

    ~data_buffer_t();

    const uint8_t* begin() const { return data; }
    const uint8_t* end()   const { return data + write_offset; }

    uint8_t* begin() { return data; }
    uint8_t* end()   { return data + write_offset; }
};

union id_t
{
    uint32_t hash;
    struct{
        uint16_t id;
        uint16_t index;
    };
};
inline bool operator == ( id_t a, id_t b ) { return a.hash == b.hash; }

inline id_t make_id( uint32_t hash ){
    id_t id = { hash };
    return id;
}

struct id_array_destroy_info_t
{
    uint16_t copy_data_from_index;
    uint16_t copy_data_to_index;
};

template <uint32_t MAX, typename Tid = id_t >
struct id_array_t
{
    id_array_t() : _freelist( MAX ) , _next_id( 0 ) , _size( 0 )
    {
        for( uint32_t i = 0; i < MAX; i++ )
        {
            _sparse[i].id = -1;
        }
    }
    
    uint32_t capacity() const { return MAX; }

    uint16_t _freelist;
    uint16_t _next_id;
    uint16_t _size;

    Tid _sparse[MAX];
    uint16_t _sparse_to_dense[MAX];
    uint16_t _dense_to_sparse[MAX];
};

template <uint32_t MAX, typename Tid = id_t>
struct id_table_t
{
    id_table_t() : _freelist( (u16)MAX ) , _next_id( 0 ) , _size( 0 )
    {
        for( uint32_t i = 0; i < MAX; i++ )
        {
            _ids[i].id = (decltype( _ids[i].id ) )(-1);
        }
    }

    uint16_t _freelist;
    uint16_t _next_id;
    uint16_t _size;

    Tid _ids[MAX];
};

template< typename T > T makeInvalidHandle()
{
    T h = { 0 };
    return h;
}

template< uint32_t SIZE >
struct bitset_t
{
    using type_t = uint64_t;

    static constexpr uint32_t ELEMENT_SIZE = sizeof( type_t );
    static constexpr uint32_t ELEMENT_BITS = ELEMENT_SIZE * 8;
    static constexpr uint32_t NUM_BITS = SIZE;
    static constexpr uint32_t NUM_ELEMENTS = 1 + ( ( SIZE - 1 ) / ELEMENT_BITS );
    static constexpr uint32_t DIV_SHIFT = 6;
    static constexpr uint32_t MOD_MASK = ELEMENT_BITS - 1;
    
    type_t bits[NUM_ELEMENTS] = {};
};


// TODO
struct ring_t
{
    struct range_t
    {
        uint64_t begin;
        uint64_t end;
    };

    ring_t( uint64_t size )
        : end( size )
    {}

    uint64_t head = 0;
    uint64_t tail = 0;
    uint64_t end = 0;
};