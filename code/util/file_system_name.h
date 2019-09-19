#pragma once

struct FSName
{
    enum
    {
        MAX_LENGTH = 255,
        MAX_SIZE = MAX_LENGTH + 1,
    };

    char _data[MAX_SIZE] = {};

    unsigned _total_length = 0;
    unsigned _relative_path_offset = 0;
    unsigned _relative_path_length = 0;

    void Clear();
    bool Empty() const { return _total_length == 0; }

    bool Append( const char* str );
    bool AppendRelativePath( const char* str );

    const char* RelativePath() const { return _data + _relative_path_offset; }
    unsigned	RelativePathLength() const { return _relative_path_length; }
    const char* AbsolutePath() const { return _data; }
};
