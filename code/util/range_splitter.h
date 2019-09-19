#pragma once

#include <foundation/type.h>
#include <foundation/debug.h>


struct RangeSplitter
{
private:
    RangeSplitter( uint32_t ne, uint32_t gs ): nElements( ne ), grabSize(gs), grabbedElements(0) {}
public:

    static inline RangeSplitter SplitByTask( uint32_t nElements, uint32_t nTasks )   { return RangeSplitter( nElements, 1 + ( (nElements-1) / nTasks ) ); }
    static inline RangeSplitter SplitByGrab( uint32_t nElements, uint32_t grabSize ) { return RangeSplitter( nElements, grabSize ); }

    inline uint32_t ElementsLeft() const { return nElements - grabbedElements; }

    struct Grab
    {
        uint32_t begin;
        uint32_t count;

        uint32_t end() const { return begin + count; }
    };

    inline Grab NextGrab()
    {
        Grab result = { 0, 0 };
        result.begin = grabbedElements;
        const uint32_t eleft = ElementsLeft();
        result.count = ( eleft < grabSize ) ? eleft : grabSize;
        grabbedElements += result.count;
        SYS_ASSERT( grabbedElements <= nElements );
        return result;
    }

    const uint32_t nElements;
    const uint32_t grabSize;
    uint32_t grabbedElements;
};
