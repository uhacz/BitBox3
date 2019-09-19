#include "rtti.h"
#include <foundation/debug.h>
#include <foundation/string_util.h>
#include <foundation/tag.h>
#include <foundation/hash.h>

#include <string.h>
#include <memory>

#define BX_RTTI_TYPE_NAME_HASH_CHECK 1

static constexpr uint32_t MAX_TYPES = 1024 * 16;
static uint64_t __typed_name_hash[MAX_TYPES] = {};

static RTTITypeInfo __types[MAX_TYPES] = {};
static uint32_t __nb_types = 0;

static constexpr uint32_t ATTR_MEMORY_BUDGET = 1024 * 1024;
static uint8_t __attributes_memory[ATTR_MEMORY_BUDGET] = {};
static uint32_t __attributes_offset = 0;

// ---
static uint8_t* AllocateAttributeData( uint32_t size )
{
    SYS_ASSERT( __attributes_offset + size <= ATTR_MEMORY_BUDGET );
    const uint32_t local_offset = __attributes_offset;
    __attributes_offset += size;
    return __attributes_memory + local_offset;
}

static inline bool IsString( const std::type_info& ti )
{
    const std::type_info& cstr_ti = typeid(string_t);
    return cstr_ti == ti;
}

static inline bool IsOffsetValid( uint16_t off )
{
    return off != UINT16_MAX;
}
static inline uint8_t* ValuePtr( const uint16_t& offset )
{
    return TYPE_OFFSET_GET_POINTER( uint8_t, offset );
}

template< typename T >
static inline const std::type_info& TypeInfoHelper( const T& value )
{
    return typeid( std::decay<decltype(value)>::type);
}

static inline uint32_t DataSize( const RTTIAttr& attr, const void* data, const std::type_info& data_type_info )
{
    if( data_type_info == attr._type_info )
        return attr._size;

    if( data_type_info == typeid(string_t) )
    {
        const string_t* str = (const string_t*)data;
        return (str->c_str()) ? string::length( str->c_str() ) + 1 : 0;
    }
    
    const char aaa[16] = "test";

    static const std::type_info& c_str_type_info = TypeInfoHelper( "<empty string>" );
    if( data_type_info == c_str_type_info )
    {
        return (uint32_t)strlen( (const char*)data ) + 1;
    }

    SYS_ASSERT( false ); // we shouldn't be here

    return 0;
}

static void InitializeValue( uint16_t* output_offset, const void* value, uint32_t data_size )
{
    uint8_t* ptr = AllocateAttributeData( data_size );
    memcpy( ptr, value, data_size );
    output_offset[0] = TYPE_POINTER_GET_OFFSET( output_offset, ptr );
}
static void SetOrInitValue( uint16_t* offset_value, const void* data, uint32_t size )
{
    if( IsOffsetValid( *offset_value ) )
    {
        memcpy( (void*)ValuePtr( *offset_value ), data, size );
    }
    else
    {
        InitializeValue( offset_value, data, size );
    }
}
static void InitDefaultsHashCode( RTTIAttr* attr, size_t hash_code )
{
    if( attr->_defaults_storage_hashcode == 0 )
    {
        attr->_defaults_storage_hashcode = hash_code;
    }
    else
    {
        SYS_ASSERT( attr->_defaults_storage_hashcode == hash_code );
    }
}

static uint64_t ComputeTypeNameHash( const char* name )
{
    const uint32_t len = (uint32_t)strlen( name );
    uint32_t lo = murmur3_hash32( name, len, tag32_t( "RTTI" ) );
    uint32_t hi = murmur3_hash32( name, len, lo );

    return uint64_t( hi ) << 32 | uint64_t( lo );
}

RTTITypeInfo::RTTITypeInfo( const std::type_info& ti, const std::type_info& parent_ti, const RTTIAttr* const* attribs, uint32_t nb_attribs )
    : type_info( ti )
    , parent_info( parent_ti )
    , creator( nullptr )
    , attributes( attribs )
    , nb_attributes( nb_attribs )
    , _index( UINT32_MAX )
{}

RTTITypeInfo::RTTITypeInfo()
    : type_info( typeid(void) )
    , parent_info( typeid(void) )
    , attributes(nullptr)
    , nb_attributes(0)
    , creator(nullptr)
    , _index(UINT32_MAX)
{}

void RTTI::RegisterType( const RTTITypeInfo& info )
{
    const uint64_t hash = ComputeTypeNameHash( info.type_name );
#if BX_RTTI_TYPE_NAME_HASH_CHECK == 1
    for( uint32_t i = 0; i < __nb_types; ++i )
        SYS_ASSERT( __typed_name_hash[i] != hash );
#endif

    const uint32_t index = __nb_types++;
    RTTITypeInfo* new_info = __types + index;
    memcpy( new_info, &info, sizeof( RTTITypeInfo ) );
    new_info->_index = index;

    __typed_name_hash[index] = hash;
}

const RTTITypeInfo* RTTI::FindType( const char* name )
{
    const uint64_t hash = ComputeTypeNameHash( name );
    for( uint32_t i = 0; i < __nb_types; ++i )
    {
        if( __typed_name_hash[i] == hash )
        {
            return &__types[i];
        }
    }

    return nullptr;
}

const RTTITypeInfo* RTTI::FindChildType( const std::type_info& parent_ti, const RTTITypeInfo* current )
{
    uint32_t current_index = (current) ? current->_index + 1 : 0;
    if( current_index >= __nb_types )
        return nullptr;
    
    while( current_index < __nb_types )
    {
        const RTTITypeInfo& type = __types[current_index];
        if( type.parent_info == parent_ti )
            return &type;

        ++current_index;
    }

    return nullptr;
}

// ---
RTTIAttr* RTTI::AllocateAttribute( uint32_t size )
{
    return (RTTIAttr*)AllocateAttributeData( size );
}



RTTIAttr* RTTIAttr::SetDefaultData( const void* value, uint32_t data_size, const std::type_info& data_type_info )
{
#if ASSERTION_ENABLED
    if( !IsString( _type_info ) )
        SYS_ASSERT( data_size == _size );
#endif
    const uint32_t size = DataSize( *this, value, data_type_info );
    SetOrInitValue( &_offset_default_value, value, size );
    InitDefaultsHashCode( this, data_type_info.hash_code() );
    return this;
}

RTTIAttr* RTTIAttr::SetMinData( const void* value, uint32_t data_size, const std::type_info& data_type_info )
{
#if ASSERTION_ENABLED
    if( !IsString( _type_info ) )
        SYS_ASSERT( data_size == _size );
#endif
    const uint32_t size = DataSize( *this, value, data_type_info );
    SetOrInitValue( &_offset_min_value, value, size );
    InitDefaultsHashCode( this, data_type_info.hash_code() );
    return this;
}

RTTIAttr* RTTIAttr::SetMaxData( const void* value, uint32_t data_size, const std::type_info& data_type_info )
{
#if ASSERTION_ENABLED
    if( !IsString( _type_info ) )
        SYS_ASSERT( data_size == _size );
#endif
    const uint32_t size = DataSize( *this, value, data_type_info );
    SetOrInitValue( &_offset_max_value, value, size );
    InitDefaultsHashCode( this, data_type_info.hash_code() );
    return this;
}

const char* RTTIAttr::TypeName() const
{
    return _type_info.name();
}
const char* RTTIAttr::Name() const
{
    return TYPE_OFFSET_GET_POINTER( const char, _offset_attr_name );
}
const uint8_t* RTTIAttr::DefaultPtr() const
{
    return IsOffsetValid(_offset_default_value) ? TYPE_OFFSET_GET_POINTER( uint8_t, _offset_default_value ) : nullptr;
}
const uint8_t* RTTIAttr::MinPtr() const
{
    return IsOffsetValid( _offset_min_value ) ? TYPE_OFFSET_GET_POINTER( uint8_t, _offset_min_value ) : nullptr;
}
const uint8_t* RTTIAttr::MaxPtr() const
{
    return IsOffsetValid( _offset_max_value ) ? TYPE_OFFSET_GET_POINTER( uint8_t, _offset_max_value ) : nullptr;
}

const uint8_t* RTTIAttr::ValuePtr( const void* obj ) const
{
    return (uint8_t*)obj + _offset;
}

#include <3rd_party/pugixml/pugixml.hpp>
uint32_t RTTI::_SerializeTxt( uint8_t* buffer, uint32_t buffer_capacity, const RTTITypeInfo& tinfo, const void* obj )
{
    pugi::xml_document doc;
    pugi::xml_node root = doc.root();
    root.set_name( tinfo.type_name );

    for( uint32_t i = 0; i < tinfo.nb_attributes; ++i )
    {
        
    }

    return 0;
}

uint32_t RTTI::_UnserializeTxt( void* obj, const RTTITypeInfo& tinfo, const uint8_t* buffer, uint32_t buffer_size, BXIAllocator* allocator )
{
    return 0;
}

struct RTTIBlobHeader
{
    uint16_t nb_attributes;
    uint16_t offset_names;
    uint32_t offset_data;
};

uint32_t RTTI::_Serialize( uint8_t* buffer, uint32_t buffer_capacity, const RTTIAttr** attributes, uint32_t nb_attributes, const void* obj )
{
    uint32_t names_size = 0;
    uint32_t data_size = 0;

    for( uint32_t i = 0; i < nb_attributes; ++i )
    {
        const char* attr_name = attributes[i]->Name();
        const uint8_t* attr_data = attributes[i]->ValuePtr( obj );
        const uint32_t attr_name_size = (uint32_t)strlen( attr_name ) + 1;
        const uint32_t attr_data_size = DataSize( *attributes[i], attr_data, attributes[i]->_type_info );
        names_size += attr_name_size;
        data_size += attr_data_size;
    }

    const uint32_t header_bytes = sizeof( RTTIBlobHeader ) + nb_attributes * (sizeof( uint32_t ) + sizeof( uint32_t ));
    const uint32_t total_bytes = names_size + data_size + header_bytes;
    if( total_bytes > buffer_capacity )
        return UINT32_MAX;

    RTTIBlobHeader* header = (RTTIBlobHeader*)buffer;
    uint8_t* ptr = (uint8_t*)(header + 1);

    header->nb_attributes = nb_attributes;
    uint32_t* names_offsets_ptr = (uint32_t*)ptr;
    uint32_t* values_offsets_ptr = names_offsets_ptr + nb_attributes;
    header->offset_names = TYPE_POINTER_GET_OFFSET( &header->offset_names, names_offsets_ptr );
    header->offset_data = TYPE_POINTER_GET_OFFSET( &header->offset_data, values_offsets_ptr );

    ptr = (uint8_t*)(values_offsets_ptr + nb_attributes);

    for( uint32_t i = 0; i < nb_attributes; ++i )
    {
        const char* attr_name = attributes[i]->Name();
        const uint32_t attr_name_size = (uint32_t)strlen( attr_name ) + 1;

        names_offsets_ptr[i] = TYPE_POINTER_GET_OFFSET( &names_offsets_ptr[i], ptr );
        memcpy( ptr, attr_name, attr_name_size );
        ptr += attr_name_size;    
    }

    for( uint32_t i = 0; i < nb_attributes; ++i )
    {
        const uint8_t* attr_value_ptr = attributes[i]->ValuePtr( obj );
        const uint32_t attr_value_size = DataSize( *attributes[i], attr_value_ptr, attributes[i]->_type_info );

        if( IsString( attributes[i]->_type_info ) )
        {
            const string_t* str = (const string_t*)attr_value_ptr;
            attr_value_ptr = (const uint8_t*)str->c_str();
        }
        
        values_offsets_ptr[i] = TYPE_POINTER_GET_OFFSET( &values_offsets_ptr[i], ptr );
        memcpy( ptr, attr_value_ptr, attr_value_size );

        ptr += attr_value_size;
    }

    SYS_ASSERT( (buffer + total_bytes) == ptr );

    return total_bytes;
}

uint32_t RTTI::_Unserialize( void* obj, const RTTIAttr** attributes, uint32_t nb_attributes, const uint8_t* buffer, uint32_t buffer_size, BXIAllocator* allocator )
{
    const RTTIBlobHeader* header = (RTTIBlobHeader*)buffer;
    const uint32_t* names_offsets_ptr = TYPE_OFFSET_GET_POINTER( uint32_t, header->offset_names );
    const uint32_t* values_offsets_ptr = TYPE_OFFSET_GET_POINTER( uint32_t, header->offset_data );

    uint32_t nb_unserialized_attributes = 0;

    for( uint32_t i = 0; i < nb_attributes; ++i )
    {
        const char* attr_name = TYPE_OFFSET_GET_POINTER( const char, names_offsets_ptr[i] );
        const RTTIAttr* attr = RTTI::_FindAttr( attributes, nb_attributes, attr_name );
        if( attr )
        {
            const uint8_t* current_value_ptr = attr->ValuePtr( obj );
            const uint8_t* new_value_ptr = TYPE_OFFSET_GET_POINTER( uint8_t, values_offsets_ptr[i] );
            
            const uint32_t current_data_size = DataSize( *attr, current_value_ptr, attr->_type_info );
            const std::type_info& new_data_type_info = IsString( attr->_type_info ) ? typeid(const char*) : attr->_type_info;
            
            const uint32_t new_data_size = DataSize( *attr, new_value_ptr, new_data_type_info );

            if( IsString( attributes[i]->_type_info ) )
            {
                string_t* str = (string_t*)current_value_ptr;
                string::free( str );
                string::create( str, (const char*)new_value_ptr, allocator );
            }
            else
            {
                SYS_ASSERT( current_data_size == new_data_size );

                uint8_t* dst_ptr = (uint8_t*)current_value_ptr;
                memcpy( dst_ptr, new_value_ptr, new_data_size );
            }
            
            
            ++nb_unserialized_attributes;
        }
    }

    return nb_unserialized_attributes;
}

const RTTIAttr* RTTI::_FindAttr( const RTTIAttr** attributes, uint32_t nb_attributes, const char* name )
{
    for( uint32_t i = 0; i < nb_attributes; ++i )
        if( !strcmp( name, attributes[i]->Name() ) )
            return attributes[i];

    return nullptr;
}

RTTI_EXPORT void* BXLoad_rtti( BXIAllocator* allocator )
{
    memset( __attributes_memory, 0x00, ATTR_MEMORY_BUDGET );
    return __attributes_memory;
}

RTTI_EXPORT void BXUnload_rtti( void* plugin, BXIAllocator* allocator )
{
    SYS_ASSERT( plugin == __attributes_memory );
}

