#pragma once

#include "containers.h"
#include "array.h"

/// The hash function stores its data in a "list-in-an-array" where
/// indices are used instead of pointers. 
///
/// When items are removed, the array-list is repacked to always keep
/// it tightly ordered.

inline u64 CalcHash( u64 key ) { return key; }

namespace hash
{
#define BX_HASHMAP_TARGS_DECL typename T, typename K
#define BX_HASHMAP_TARGS_INST T, K 
    /// Returns true if the specified key exists in the hash.
    template<BX_HASHMAP_TARGS_DECL> bool has( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key );

    /// Returns the value stored for the specified key, or deffault if the key
    /// does not exist in the hash.
    template<BX_HASHMAP_TARGS_DECL> const T &get( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &deffault );

    /// Sets the value for the key.
    template<BX_HASHMAP_TARGS_DECL> void set( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &value );

    /// Removes the key from the hash if it exists.
    template<BX_HASHMAP_TARGS_DECL> void remove( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key );

    /// Resizes the hash lookup table to the specified size.
    /// (The table will grow automatically when 70 % full.)
    template<BX_HASHMAP_TARGS_DECL> void reserve( hash_t<BX_HASHMAP_TARGS_INST> &h, uint32_t size );

    /// Remove all elements from the hash.
    template<BX_HASHMAP_TARGS_DECL> void clear( hash_t<BX_HASHMAP_TARGS_INST> &h );

    /// Returns a pointer to the first entry in the hash table, can be used to
    /// efficiently iterate over the elements (in random order).
    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *begin( const hash_t<BX_HASHMAP_TARGS_INST> &h );
    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *end( const hash_t<BX_HASHMAP_TARGS_INST> &h );
}

namespace multi_hash
{
    /// Finds the first entry with the specified key.
    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *find_first( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key );

    /// Finds the next entry with the same key as e.
    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *find_next( const hash_t<BX_HASHMAP_TARGS_INST> &h, const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e );

    /// Returns the number of entries with the key.
    template<BX_HASHMAP_TARGS_DECL> uint32_t count( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key );

    /// Returns all the entries with the specified key.
    /// Use a TempAllocator for the array to avoid allocating memory.
    template<BX_HASHMAP_TARGS_DECL> void get( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, array_t<T> &items );

    /// Inserts the value as an additional value for the key.
    template<BX_HASHMAP_TARGS_DECL> void insert( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &value );

    /// Removes the specified entry.
    template<BX_HASHMAP_TARGS_DECL> void remove( hash_t<BX_HASHMAP_TARGS_INST> &h, const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e );

    /// Removes all entries with the specified key.
    template<BX_HASHMAP_TARGS_DECL> void remove_all( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key );
}

namespace hash_internal
{
    const uint32_t END_OF_LIST = 0xffffffffu;

    struct FindResult
    {
        uint32_t hash_i;
        uint32_t data_prev;
        uint32_t data_i;
    };

    template<BX_HASHMAP_TARGS_DECL> uint32_t add_entry( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        typename hash_t<BX_HASHMAP_TARGS_INST>::Entry e;
        e.key = key;
        e.next = END_OF_LIST;
        uint32_t ei = array::size( h._data );
        array::push_back( h._data, e );
        return ei;
    }

    template<BX_HASHMAP_TARGS_DECL> void erase( hash_t<BX_HASHMAP_TARGS_INST> &h, const FindResult &fr )
    {
        if( fr.data_prev == END_OF_LIST )
            h._hash[fr.hash_i] = h._data[fr.data_i].next;
        else
            h._data[fr.data_prev].next = h._data[fr.data_i].next;

        array::pop_back( h._data );

        if( fr.data_i == array::size( h._data ) ) return;

        h._data[fr.data_i] = h._data[array::size( h._data )];

        FindResult last = find( h, h._data[fr.data_i].key );

        if( last.data_prev != END_OF_LIST )
            h._data[last.data_prev].next = fr.data_i;
        else
            h._hash[last.hash_i] = fr.data_i;
    }

    template<BX_HASHMAP_TARGS_DECL> FindResult find( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        FindResult fr;
        fr.hash_i = END_OF_LIST;
        fr.data_prev = END_OF_LIST;
        fr.data_i = END_OF_LIST;

        if( array::size( h._hash ) == 0 )
            return fr;

        const u64 key_hash = CalcHash( key );
        fr.hash_i = key_hash % array::size( h._hash );
        fr.data_i = h._hash[fr.hash_i];
        while( fr.data_i != END_OF_LIST ) {
            if( h._data[fr.data_i].key == key )
                return fr;
            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<BX_HASHMAP_TARGS_DECL> FindResult find( const hash_t<BX_HASHMAP_TARGS_INST> &h, const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e )
    {
        FindResult fr;
        fr.hash_i = END_OF_LIST;
        fr.data_prev = END_OF_LIST;
        fr.data_i = END_OF_LIST;

        if( array::size( h._hash ) == 0 )
            return fr;

        fr.hash_i = e->key % array::size( h._hash );
        fr.data_i = h._hash[fr.hash_i];
        while( fr.data_i != END_OF_LIST ) {
            if( &h._data[fr.data_i] == e )
                return fr;
            fr.data_prev = fr.data_i;
            fr.data_i = h._data[fr.data_i].next;
        }
        return fr;
    }

    template<BX_HASHMAP_TARGS_DECL> uint32_t find_or_fail( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        return find( h, key ).data_i;
    }

    template<BX_HASHMAP_TARGS_DECL> uint32_t find_or_make( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        const FindResult fr = find( h, key );
        if( fr.data_i != END_OF_LIST )
            return fr.data_i;

        uint32_t i = add_entry( h, key );
        if( fr.data_prev == END_OF_LIST )
            h._hash[fr.hash_i] = i;
        else
            h._data[fr.data_prev].next = i;
        return i;
    }

    template<BX_HASHMAP_TARGS_DECL> uint32_t make( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        const FindResult fr = find( h, key );
        const uint32_t i = add_entry( h, key );

        if( fr.data_prev == END_OF_LIST )
            h._hash[fr.hash_i] = i;
        else
            h._data[fr.data_prev].next = i;

        h._data[i].next = fr.data_i;
        return i;
    }

    template<BX_HASHMAP_TARGS_DECL> void find_and_erase( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        const FindResult fr = find( h, key );
        if( fr.data_i != END_OF_LIST )
            erase( h, fr );
    }

    template<BX_HASHMAP_TARGS_DECL> void rehash( hash_t<BX_HASHMAP_TARGS_INST> &h, uint32_t new_size )
    {
        hash_t<BX_HASHMAP_TARGS_INST> nh( h._hash.allocator );
        array::resize( nh._hash, new_size );
        array::reserve( nh._data, array::size( h._data ) );
        for( uint32_t i = 0; i < new_size; ++i )
            nh._hash[i] = END_OF_LIST;
        for( uint32_t i = 0; i < array::size( h._data ); ++i ) {
            const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry &e = h._data[i];
            multi_hash::insert( nh, e.key, e.value );
        }

        hash_t<BX_HASHMAP_TARGS_INST> empty( h._hash.allocator );
        h.~hash_t<BX_HASHMAP_TARGS_INST>();
        memcpy( &h, &nh, sizeof( hash_t<BX_HASHMAP_TARGS_INST> ) );
        memcpy( &nh, &empty, sizeof( hash_t<BX_HASHMAP_TARGS_INST> ) );
    }

    template<BX_HASHMAP_TARGS_DECL> bool full( const hash_t<BX_HASHMAP_TARGS_INST> &h )
    {
        const float max_load_factor = 0.7f;
        return array::size( h._data ) >= array::size( h._hash ) * max_load_factor;
    }

    template<BX_HASHMAP_TARGS_DECL> void grow( hash_t<BX_HASHMAP_TARGS_INST> &h )
    {
        const uint32_t new_size = array::size( h._data ) * 2 + 10;
        rehash( h, new_size );
    }
}

namespace hash
{
    template<BX_HASHMAP_TARGS_DECL> bool has( const hash_t<T,K> &h, const K& key )
    {
        return hash_internal::find_or_fail( h, key ) != hash_internal::END_OF_LIST;
    }

    template<BX_HASHMAP_TARGS_DECL> const T &get( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &deffault )
    {
        const uint32_t i = hash_internal::find_or_fail( h, key );
        return i == hash_internal::END_OF_LIST ? deffault : h._data[i].value;
    }

    template<BX_HASHMAP_TARGS_DECL> void set( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &value )
    {
        if( array::size( h._hash ) == 0 )
            hash_internal::grow( h );

        const uint32_t i = hash_internal::find_or_make( h, key );
        h._data[i].value = value;
        if( hash_internal::full( h ) )
            hash_internal::grow( h );
    }

    template<BX_HASHMAP_TARGS_DECL> void remove( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        hash_internal::find_and_erase( h, key );
    }

    template<BX_HASHMAP_TARGS_DECL> void reserve( hash_t<BX_HASHMAP_TARGS_INST> &h, uint32_t size )
    {
        hash_internal::rehash( h, size );
    }

    template<BX_HASHMAP_TARGS_DECL> void clear( hash_t<BX_HASHMAP_TARGS_INST> &h )
    {
        array::clear( h._data );
        array::clear( h._hash );
    }

    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *begin( const hash_t<BX_HASHMAP_TARGS_INST> &h )
    {
        return array::begin( h._data );
    }

    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *end( const hash_t<BX_HASHMAP_TARGS_INST> &h )
    {
        return array::end( h._data );
    }
}

namespace multi_hash
{
    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *find_first( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        const uint32_t i = hash_internal::find_or_fail( h, key );
        return i == hash_internal::END_OF_LIST ? 0 : &h._data[i];
    }

    template<BX_HASHMAP_TARGS_DECL> const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *find_next( const hash_t<BX_HASHMAP_TARGS_INST> &h, const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e )
    {
        uint32_t i = e->next;
        while( i != hash_internal::END_OF_LIST ) {
            if( h._data[i].key == e->key )
                return &h._data[i];
            i = h._data[i].next;
        }
        return 0;
    }

    template<BX_HASHMAP_TARGS_DECL> uint32_t count( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        uint32_t i = 0;
        const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e = find_first( h, key );
        while( e ) {
            ++i;
            e = find_next( h, e );
        }
        return i;
    }

    template<BX_HASHMAP_TARGS_DECL> void get( const hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, array_t<T> &items )
    {
        const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e = find_first( h, key );
        while( e ) {
            array::push_back( items, e->value );
            e = find_next( h, e );
        }
    }

    template<BX_HASHMAP_TARGS_DECL> void insert( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key, const T &value )
    {
        if( array::size( h._hash ) == 0 )
            hash_internal::grow( h );

        const uint32_t i = hash_internal::make( h, key );
        h._data[i].value = value;
        if( hash_internal::full( h ) )
            hash_internal::grow( h );
    }

    template<BX_HASHMAP_TARGS_DECL> void remove( hash_t<BX_HASHMAP_TARGS_INST> &h, const typename hash_t<BX_HASHMAP_TARGS_INST>::Entry *e )
    {
        const hash_internal::FindResult fr = hash_internal::find( h, e );
        if( fr.data_i != hash_internal::END_OF_LIST )
            hash_internal::erase( h, fr );
    }

    template<BX_HASHMAP_TARGS_DECL> void remove_all( hash_t<BX_HASHMAP_TARGS_INST> &h, const K& key )
    {
        while( hash::has( h, key ) )
            hash::remove( h, key );
    }
}
