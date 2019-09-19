#pragma once

#include "containers.h"

#define BX_ID_TABLE_T_DEF uint32_t MAX, typename Tid
#define BX_ID_TABLE_T_ARG MAX, Tid

namespace id_table
{
    template <BX_ID_TABLE_T_DEF>
    inline Tid create( id_table_t<BX_ID_TABLE_T_ARG>& a )
    {
        SYS_ASSERT( a._size < MAX );
        // Obtain a new id
        Tid id;
        id.id = ++a._next_id;

        // Recycle slot if there are any
        if( a._freelist < MAX )
        {
            id.index = a._freelist;
            a._freelist = a._ids[a._freelist].index;
        }
        else
        {
            id.index = a._size;
        }

        a._ids[id.index] = id;
        a._size++;

        return id;
    }

    template <BX_ID_TABLE_T_DEF>
    inline Tid invalidate( id_table_t<BX_ID_TABLE_T_ARG>& a, Tid id )
    {
        SYS_ASSERT_TXT( has( a, id ), "IdTable does not have ID: %d,%d", id.id, id.index );

        a._ids[id.index].id += 1;
        return a._ids[id.index];
    }

    template <BX_ID_TABLE_T_DEF>
    inline void destroy( id_table_t<BX_ID_TABLE_T_ARG>& a, Tid id )
    {
        SYS_ASSERT_TXT( has( a, id ), "IdTable does not have ID: %d,%d", id.id, id.index );

        a._ids[id.index].id = -1;
        a._ids[id.index].index = a._freelist;
        a._freelist = id.index;
        a._size--;
    }

    template <BX_ID_TABLE_T_DEF>
    inline bool has( const id_table_t<BX_ID_TABLE_T_ARG>& a, Tid id )
    {
        return id.index < MAX && a._ids[id.index].id == id.id;
    }

    template <BX_ID_TABLE_T_DEF>
    inline Tid id( const id_table_t<BX_ID_TABLE_T_ARG>& a, uint32_t index )
    {
        SYS_ASSERT( index < MAX );
        SYS_ASSERT( a._ids[index].index == index );
        return a._ids[index];
    }

    template <BX_ID_TABLE_T_DEF>
    inline uint16_t size( const id_table_t<BX_ID_TABLE_T_ARG>& a )
    {
        return a._size;
    }

    template <BX_ID_TABLE_T_DEF>
    inline const Tid* begin( const id_table_t<BX_ID_TABLE_T_ARG>& a )
    {
        return a._ids;
    }

    template <BX_ID_TABLE_T_DEF>
    inline const Tid* end( const id_table_t<BX_ID_TABLE_T_ARG>& a )
    {
        return a._ids + MAX;
    }
} // namespace id_table

