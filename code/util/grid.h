#pragma once

struct grid_t
{
    unsigned width;
    unsigned height;
    unsigned depth;

    //
    grid_t()
        : width(0), height(0), depth(0)
    {}

    grid_t( unsigned w, unsigned h, unsigned d )
        : width(w), height(h), depth(d)
    {}

    //
    int NumCells() const
    { 
        return width * height * depth; 
    }
    int Index( int x, int y, int z ) const
    {
        return x + y * width + z*width*height;    
    }
    void Coords( unsigned xyz[3], unsigned idx ) const
    {
        const int wh = width*height;
        const int index_mod_wh = idx % wh;
        
        xyz[0] = ( index_mod_wh % width );
        xyz[1] = ( index_mod_wh / width );
        xyz[2] = ( idx / wh );
    }
};