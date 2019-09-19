#pragma once

#include "containers.h"
#include <intrin.h>

namespace bitset
{
#define BITSET_TEMPLATE_ARGS uint32_t SIZE
#define BITSET_T bitset_t<SIZE>

    namespace _internal
    {
        struct bit_address_t
        {
            uint32_t element;
            uint32_t bit;

            template< typename Tresult >
            Tresult mask() const { return (Tresult)1 << bit; }
        };

        template< typename T >
        inline bit_address_t compute_bit_address( const T& bs, uint32_t index )
        {
            bit_address_t addr;
            addr.element = index >> T::DIV_SHIFT;
            addr.bit = index & T::MOD_MASK;
            SYS_ASSERT( addr.element < T::NUM_ELEMENTS );
            return addr;
        }

        template< typename T >
        FORCE_INLINE uint8_t bitscan_forward( uint32_t* index, T mask ) { return 0; }

        template<>
        FORCE_INLINE uint8_t bitscan_forward<uint32_t>( uint32_t* index, uint32_t mask )
        {
            return _BitScanForward( (unsigned long*)index, mask );
        }

        template<>
        FORCE_INLINE uint8_t bitscan_forward<uint64_t>( uint32_t* index, uint64_t mask )
        {
            return _BitScanForward64( (unsigned long*)index, mask );
        }

        template< typename T >
        FORCE_INLINE uint32_t population( T mask ) { return 0; }

        template<>
        FORCE_INLINE uint32_t population( uint32_t mask ) { return __popcnt( mask ); }
        template<>
        FORCE_INLINE uint32_t population( uint64_t mask ) { return (uint32_t)__popcnt64( mask ); }

    }

    template< BITSET_TEMPLATE_ARGS > void set_all( BITSET_T& bs )
    {
        for( uint32_t i = 0; i < bs.NUM_ELEMENTS; ++i )
            bs.bits[i] = ~BITSET_T::type_t(0);
    }

    template< BITSET_TEMPLATE_ARGS > void clear_all( BITSET_T& bs )
    {
        for( uint32_t i = 0; i < bs.NUM_ELEMENTS; ++i )
            bs.bits[i] = 0;
    }
    template< BITSET_TEMPLATE_ARGS > void clear( BITSET_T& bs, uint32_t index )
    {
        const _internal::bit_address_t bit_addr = _internal::compute_bit_address( bs, index );
        bs.bits[bit_addr.element] &= ~(bit_addr.mask<BITSET_T::type_t>());
    }
    template< BITSET_TEMPLATE_ARGS > void set( BITSET_T& bs, uint32_t index )
    {
        const _internal::bit_address_t bit_addr = _internal::compute_bit_address( bs, index );
        bs.bits[bit_addr.element] |= (bit_addr.mask<BITSET_T::type_t>());
    }
    template< BITSET_TEMPLATE_ARGS > bool get( const BITSET_T& bs, uint32_t index )
    {
        const _internal::bit_address_t bit_addr = _internal::compute_bit_address( bs, index );
        return ( bs.bits[bit_addr.element] & bit_addr.mask<BITSET_T::type_t>() ) != 0;
    }

    template< BITSET_TEMPLATE_ARGS > bool is_any_set( const BITSET_T& bs )
    {
        for( uint32_t i = 0; i < BITSET_T::NUM_ELEMENTS; ++i )
        {
            if( bs.bits[i] )
                return true;
        }
        return false;
    }

    template< BITSET_TEMPLATE_ARGS > bool empty( const BITSET_T& bs )
    {
        for( uint32_t i = 0; i < BITSET_T::NUM_ELEMENTS; ++i )
        {
            if( bs.bits[i] )
                return false;
        }
        return true;
    }

    template< BITSET_TEMPLATE_ARGS > uint32_t population( const BITSET_T& bs )
    {
        uint32_t result = 0;
        for( uint32_t i = 0; i < BITSET_T::NUM_ELEMENTS; ++i )
            result += _internal::population( bs.bits[i] );

        return result;
    }


    template< BITSET_TEMPLATE_ARGS > uint32_t find_next_set( const BITSET_T& bs, uint32_t begin )
    {
        if( begin >= SIZE )
            return SIZE;

        const _internal::bit_address_t bit_addr = _internal::compute_bit_address( bs, begin );
        
        using value_type_t = BITSET_T::type_t;

        value_type_t element_index = bit_addr.element;
        value_type_t shift = bit_addr.bit;
        value_type_t value = bs.bits[element_index] >> shift;
        while( !value && ++element_index < BITSET_T::NUM_ELEMENTS )
        {
            value = bs.bits[element_index];
            shift = 0;
        }
    
        uint32_t result = SIZE;
        uint32_t found = _internal::bitscan_forward( &result, value );
        return (found) ? (uint32_t)( element_index * BITSET_T::ELEMENT_BITS + (result + shift) ) : SIZE;
    }

    template< typename Tbitset >
    struct const_iterator
    {
        explicit const_iterator( Tbitset& bs, uint32_t begin = 0 )
            : _bs( bs )
        {
            _index = find_next_set( _bs, begin );
        }

        void next() { _index = find_next_set( _bs, _index + 1 ); }
        bool ok () const { return _index != _bs.NUM_BITS; }
        bool get() const { return bitset::get( _bs, _index ); }
        uint32_t index() const { return _index; }

    protected:
        Tbitset& _bs;
        uint32_t _index;
    };

    template< typename Tbitset >
    struct iterator : public const_iterator< Tbitset >
    {
        explicit iterator( Tbitset& bs, uint32_t begin = 0 )
            : const_iterator( bs, begin )
        {}

        void set() { bitset::set( _bs, _index ); }
        void clear() { bitset::clear( _bs, _index ); }
    };

}