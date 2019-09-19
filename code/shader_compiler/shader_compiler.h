#pragma once

struct BXIAllocator;

namespace bx{ namespace tool{
    
    int ShaderCompilerCompile( const char* inputFile, const char* outputDir, BXIAllocator* allocator );

}}///