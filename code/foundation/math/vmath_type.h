#pragma once

#ifdef _MSC_VER
#define VEC_ALIGNMENT( alignment )	__declspec(align(alignment))	
#define VEC_FORCE_INLINE __forceinline
#else
#define VEC_ALIGNMENT( alignment ) __attribute__ ((aligned(alignment)))
#define VEC_FORCE_INLINE VEC_FORCE_INLINE __attribute__( (always_inline) )
#endif

#ifdef _MSC_VER
#pragma warning( disable: 4201 )
#endif

struct vec2_t
{
	union
	{
		float xy[2];
        struct{ float x, y; };
	};

	         VEC_FORCE_INLINE vec2_t(                    ) {}
    explicit VEC_FORCE_INLINE vec2_t( float s            ) : x( s ), y( s ) {}
	         VEC_FORCE_INLINE vec2_t( float x_, float y_ ) : x( x_ ), y( y_ ) {}
	         VEC_FORCE_INLINE vec2_t( const vec2_t& v    ) : x( v.x ), y( v.y ) {}

    VEC_FORCE_INLINE vec2_t operator-()                  const { return vec2_t( -x, -y ); }
    VEC_FORCE_INLINE vec2_t operator+( const vec2_t& v ) const { return vec2_t( x + v.x, y + v.y ); }
    VEC_FORCE_INLINE vec2_t operator-( const vec2_t& v ) const { return vec2_t( x - v.x, y - v.y ); }
    VEC_FORCE_INLINE vec2_t operator*( float f )         const { return vec2_t( x * f, y * f ); }
    VEC_FORCE_INLINE vec2_t operator/( float f )         const { f = 1.0f / f; return vec2_t( x * f, y * f ); }

    VEC_FORCE_INLINE vec2_t& operator+=( const vec2_t& v ) { x += v.x; y += v.y; return *this; }
    VEC_FORCE_INLINE vec2_t& operator-=( const vec2_t& v ) { x -= v.x; y -= v.y; return *this; }
    VEC_FORCE_INLINE vec2_t& operator*=( float f ) { x *= f; y *= f; return *this; }
    VEC_FORCE_INLINE vec2_t& operator/=( float f ) { f = 1.0f / f; x *= f; y *= f; return *this; }

};


struct vec3_t
{
    union
    {
        float xyz[3];
        struct{ float x, y, z; };
    };

             VEC_FORCE_INLINE vec3_t() {}
    explicit constexpr vec3_t( float s                      ) : x( s ), y( s ), z( s ) {}
             constexpr vec3_t( float x_, float y_, float z_ ) : x( x_ ), y( y_ ), z( z_ ) {}
             constexpr vec3_t( const vec2_t& v, float z_    ) : x( v.x ), y( v.y ), z( z_ ) {}
             constexpr vec3_t( const vec3_t& v              ) : x( v.x ), y( v.y ), z( v.z ) {}

    VEC_FORCE_INLINE float& operator [] ( unsigned i ) { return xyz[i]; }
    VEC_FORCE_INLINE const float& operator [] ( unsigned i ) const { return xyz[i]; }

    VEC_FORCE_INLINE vec3_t  operator- ()                  const { return vec3_t( -x, -y, -z ); }
    VEC_FORCE_INLINE vec3_t  operator+ ( const vec3_t& v ) const { return vec3_t( x + v.x, y + v.y, z + v.z ); }
    VEC_FORCE_INLINE vec3_t  operator- ( const vec3_t& v ) const { return vec3_t( x - v.x, y - v.y, z - v.z ); }
    VEC_FORCE_INLINE vec3_t  operator* ( float f )         const { return vec3_t( x * f, y * f, z * f ); }
    VEC_FORCE_INLINE vec3_t  operator/ ( float f )         const { f = 1.0f / f; return vec3_t( x * f, y * f, z * f ); }

    VEC_FORCE_INLINE vec3_t& operator+=( const vec3_t& v ) { x += v.x; y += v.y; z += v.z; return *this; }
    VEC_FORCE_INLINE vec3_t& operator-=( const vec3_t& v ) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    VEC_FORCE_INLINE vec3_t& operator*=( float f ) { x *= f; y *= f; z *= f; return *this; }
    VEC_FORCE_INLINE vec3_t& operator/=( float f ) { f = 1.0f / f; x *= f; y *= f; z *= f; return *this; }

    static vec3_t ax() { return vec3_t( 1.f, 0.f, 0.f ); }
    static vec3_t ay() { return vec3_t( 0.f, 1.f, 0.f ); }
    static vec3_t az() { return vec3_t( 0.f, 0.f, 1.f ); }
};


struct VEC_ALIGNMENT(16) vec4_t
{
	union
	{
        float xyzw[4];
        struct{ float x, y, z, w; };
    };

	         VEC_FORCE_INLINE vec4_t(                                        ) {}
	explicit constexpr vec4_t( float s                                ) : x( s ), y( s ), z( s ), w( s ) {}
	         constexpr vec4_t( float x_, float y_, float z_, float w_ ) : x( x_ ), y( y_ ), z( z_ ), w( w_ ) {}
	         constexpr vec4_t( const vec3_t& v, float w_              ) : x( v.x ), y( v.y ), z( v.z ), w( w_ ) {}
             constexpr vec4_t( const vec4_t& v                        ) : x( v.x ), y( v.y ), z( v.z ), w( v.w ) {}

    VEC_FORCE_INLINE vec4_t operator-()                  const { return vec4_t( -x, -y, -z, -w ); }
    VEC_FORCE_INLINE vec4_t operator+( const vec4_t& v ) const { return vec4_t( x + v.x, y + v.y, z + v.z, w + v.w ); }
    VEC_FORCE_INLINE vec4_t operator-( const vec4_t& v ) const { return vec4_t( x - v.x, y - v.y, z - v.z, w - v.w ); }
    VEC_FORCE_INLINE vec4_t operator*( float f )         const { return vec4_t( x * f, y * f, z * f, w * f ); }
    VEC_FORCE_INLINE vec4_t operator/( float f )         const { f = 1.0f / f; return vec4_t( x * f, y * f, z * f, w * f ); }

    VEC_FORCE_INLINE vec4_t& operator+=( const vec4_t& v ) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    VEC_FORCE_INLINE vec4_t& operator-=( const vec4_t& v ) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    VEC_FORCE_INLINE vec4_t& operator*=( float f ) { x *= f; y *= f; z *= f; w *= f; return *this; }
    VEC_FORCE_INLINE vec4_t& operator/=( float f ) { f = 1.0f / f; x *= f; y *= f; z *= f; w *= f; return *this; }

    static vec4_t ax() { return vec4_t( 1.f, 0.f, 0.f, 0.f ); }
    static vec4_t ay() { return vec4_t( 0.f, 1.f, 0.f, 0.f ); }
    static vec4_t az() { return vec4_t( 0.f, 0.f, 1.f, 0.f ); }
    static vec4_t aw() { return vec4_t( 0.f, 0.f, 0.f, 1.f ); }

    vec3_t xyz() const { return vec3_t( x, y, z ); }
    void   set_xyz( const vec3_t& v ) { x = v.x; y = v.y; z = v.z; }
};

struct mat33_t;
struct VEC_ALIGNMENT(16) quat_t
{
	union
	{
		float xyzw[4];
        struct{ float x, y, z, w; };
	};

	VEC_FORCE_INLINE   quat_t(                                        ) {}
	explicit constexpr quat_t( float s                                ) : x( s ), y( s ), z( s ), w( s ) {}
	         constexpr quat_t( float x_, float y_, float z_, float w_ ) : x( x_ ), y( y_ ), z( z_ ), w( w_ ) {}
	         constexpr quat_t( const vec3_t& v, float w_              ) : x( v.x ), y( v.y ), z( v.z ), w( w_ ) {}
	explicit constexpr quat_t( const vec4_t& v                        ) : x( v.x ), y( v.y ), z( v.z ), w( v.w ) {}
	         constexpr quat_t( const quat_t& q                        ) : x( q.x ), y( q.y ), z( q.z ), w( q.w ) {}
	explicit VEC_FORCE_INLINE quat_t( const mat33_t& m                       );

    VEC_FORCE_INLINE vec4_t to_vec4() const { return vec4_t( x, y, z, w ); }
    VEC_FORCE_INLINE vec3_t xyz() const { return vec3_t( x, y, z ); }

	static VEC_FORCE_INLINE quat_t identity() { return quat_t( 0.f, 0.f, 0.f, 1.f ); }
	static VEC_FORCE_INLINE quat_t rotationx( float radians );
	static VEC_FORCE_INLINE quat_t rotationy( float radians );
	static VEC_FORCE_INLINE quat_t rotationz( float radians );
	static VEC_FORCE_INLINE quat_t rotation( float radians, const vec3_t& axis );

    VEC_FORCE_INLINE quat_t operator*( const quat_t& q ) const
    {
        return quat_t( 
            w * q.x + q.w * x + y * q.z - q.y * z, 
            w * q.y + q.w * y + z * q.x - q.z * x,
            w * q.z + q.w * z + x * q.y - q.x * y,
            w * q.w - q.x * x - y * q.y - q.z * z );
    }
    VEC_FORCE_INLINE quat_t operator-() const { return quat_t( -x, -y, -z, -w ); }
    VEC_FORCE_INLINE quat_t operator+( const quat_t& q ) const { return quat_t( x + q.x, y + q.y, z + q.z, w + q.w ); }
    VEC_FORCE_INLINE quat_t operator-( const quat_t& q ) const { return quat_t( x - q.x, y - q.y, z - q.z, w - q.w ); }
    VEC_FORCE_INLINE quat_t operator*( float r )         const { return quat_t( x * r, y * r, z * r, w * r ); }

    VEC_FORCE_INLINE quat_t& operator*=( const quat_t& q )
    {
        const float tx = w * q.x + q.w * x + y * q.z - q.y * z;
        const float ty = w * q.y + q.w * y + z * q.x - q.z * x;
        const float tz = w * q.z + q.w * z + x * q.y - q.x * y;

        w = w * q.w - q.x * x - y * q.y - q.z * z;
        x = tx;
        y = ty;
        z = tz;

        return *this;
    }

    VEC_FORCE_INLINE quat_t& operator+=( const quat_t& q ) { x += q.x; y += q.y; z += q.z; w += q.w; return *this; }
    VEC_FORCE_INLINE quat_t& operator-=( const quat_t& q ) { x -= q.x; y -= q.y; z -= q.z; w -= q.w; return *this; }
    VEC_FORCE_INLINE quat_t& operator*=( const float s ) { x *= s; y *= s; z *= s; w *= s; return *this; }
};


struct mat44_t;
struct VEC_ALIGNMENT(16) xform_t
{
	quat_t rot;
	vec3_t pos;

    VEC_FORCE_INLINE xform_t() {}
    constexpr xform_t( const quat_t& q, const vec3_t& p ) : rot( q ), pos( p ) {}
    
    explicit VEC_FORCE_INLINE xform_t( const quat_t& q  ) : rot( q ), pos( 0.f ) {}
	explicit VEC_FORCE_INLINE xform_t( const vec3_t& p  ) : rot( quat_t::identity() ), pos( p ) {}
    explicit VEC_FORCE_INLINE xform_t( const mat33_t& m ) : rot( m ), pos( 0.f ) {}
    explicit VEC_FORCE_INLINE xform_t( const mat44_t& m );

    static xform_t identity() { return xform_t( quat_t::identity(), vec3_t( 0.f ) ); }

    VEC_FORCE_INLINE xform_t  operator *  ( const xform_t& x ) const;
    VEC_FORCE_INLINE xform_t& operator *= ( const xform_t& x )       { *this = *this * x; return *this; }

};

struct mat33_t
{
	vec3_t c0;
	vec3_t c1;
	vec3_t c2;

	VEC_FORCE_INLINE mat33_t() {}
   constexpr mat33_t( float s ) : c0( s, 0.f, 0.f ), c1( 0.f, s, 0.f ), c2( 0.f, 0.f, s ) {}
   constexpr mat33_t( const mat33_t& m ) : c0( m.c0 ), c1( m.c1 ), c2( m.c2 ) {}
   constexpr mat33_t( const vec3_t& v0, const vec3_t& v1, const vec3_t& v2 ) : c0( v0 ), c1( v1 ), c2( v2 ) {}

    explicit VEC_FORCE_INLINE mat33_t( const quat_t& q );
    explicit VEC_FORCE_INLINE mat33_t( const vec3_t& v ) : c0( v ), c1( v ), c2( v ) {}

    static mat33_t identity() { return mat33_t( vec3_t::ax(), vec3_t::ay(), vec3_t::az() ); }

    VEC_FORCE_INLINE const mat33_t operator-() const { return mat33_t( -c0, -c1, -c2 ); }
    VEC_FORCE_INLINE const mat33_t operator+( const mat33_t& other ) const { return mat33_t( c0 + other.c0, c1 + other.c1, c2 + other.c2 ); }
    VEC_FORCE_INLINE const mat33_t operator-( const mat33_t& other ) const { return mat33_t( c0 - other.c0, c1 - other.c1, c2 - other.c2 ); }
    VEC_FORCE_INLINE const mat33_t operator*( float scalar ) const { return mat33_t( c0 * scalar, c1 * scalar, c2 * scalar ); }
    VEC_FORCE_INLINE const vec3_t  operator*( const vec3_t& vec ) const {return c0 * vec.x + c1 * vec.y + c2 * vec.z; }
    VEC_FORCE_INLINE const mat33_t operator*( const mat33_t& other ) const { return mat33_t( (*this) * other.c0, (*this) * other.c1, (*this) * other.c2 ); }

    VEC_FORCE_INLINE mat33_t& operator+=( const mat33_t& other ) { c0 += other.c0; c1 += other.c1; c2 += other.c2; return *this; }
    VEC_FORCE_INLINE mat33_t& operator-=( const mat33_t& other ) { c0 -= other.c0; c1 -= other.c1; c2 -= other.c2; return *this; }
    VEC_FORCE_INLINE mat33_t& operator*=( float scalar ) { c0 *= scalar; c1 *= scalar; c2 *= scalar; return *this; }
    VEC_FORCE_INLINE mat33_t& operator*=( const mat33_t& other ) { *this = *this * other; return *this; }

};


struct VEC_ALIGNMENT(16) mat44_t
{
	vec4_t c0;
	vec4_t c1;
	vec4_t c2;
	vec4_t c3;

	VEC_FORCE_INLINE mat44_t(){}
	constexpr mat44_t( float s ) : c0( s ), c1( s ), c2( s ), c3(s) {}
    constexpr mat44_t( const mat44_t& m ) : c0( m.c0 ), c1( m.c1 ), c2( m.c2 ), c3( m.c3 ) {}
    constexpr mat44_t( const vec4_t& v0, const vec4_t& v1, const vec4_t& v2, const vec4_t& v3 ) : c0( v0 ), c1( v1 ), c2( v2 ), c3( v3 ) {}
    constexpr mat44_t( const mat33_t& rot, const vec3_t& pos ) : c0( vec4_t( rot.c0, 0.f ) ), c1( vec4_t( rot.c1, 0.f ) ), c2( vec4_t( rot.c2, 0.f ) ), c3( pos, 1.f ) {}
    constexpr mat44_t( const mat33_t& rot, const vec4_t& pos ) : c0( vec4_t( rot.c0, 0.f ) ), c1( vec4_t( rot.c1, 0.f ) ), c2( vec4_t( rot.c2, 0.f ) ), c3( pos ) {}
    VEC_FORCE_INLINE mat44_t( const quat_t& rot, const vec3_t& pos );
    VEC_FORCE_INLINE mat44_t( const quat_t& rot, const vec4_t& pos );

    explicit VEC_FORCE_INLINE mat44_t( const vec4_t& v ) : c0( v ), c1( v ), c2( v ), c3( v ) {}
	explicit VEC_FORCE_INLINE mat44_t( const quat_t& q );
    explicit VEC_FORCE_INLINE mat44_t( const xform_t& xf );
    explicit VEC_FORCE_INLINE mat44_t( const mat33_t& m ) : c0( vec4_t(m.c0, 0.f) ), c1( vec4_t( m.c1, 0.f ) ), c2( vec4_t( m.c2, 0.f ) ), c3( 0.f, 0.f, 0.f, 1.f ) {}

    static VEC_FORCE_INLINE mat44_t identity() { return mat44_t( vec4_t::ax(), vec4_t::ay(), vec4_t::az(), vec4_t::aw() ); }
    static VEC_FORCE_INLINE mat44_t translation( const vec3_t& v );
    static VEC_FORCE_INLINE mat44_t rotationx( float rad );
    static VEC_FORCE_INLINE mat44_t rotationy( float rad );
    static VEC_FORCE_INLINE mat44_t rotationz( float rad );
    static VEC_FORCE_INLINE mat44_t rotationzyx( const vec3_t& radxyz );
    static VEC_FORCE_INLINE mat44_t rotation( float rad, const vec3_t& axis );
    static VEC_FORCE_INLINE mat44_t scale( const vec3_t& v );

    mat33_t upper3x3   () const { return mat33_t( c0.xyz(), c1.xyz(), c2.xyz() ); }
    vec3_t  translation() const { return c3.xyz(); }

    void set_translation( const vec3_t& v ) { c3 = vec4_t( v, 1.0f ); }
    void set_rotation( const mat33_t& v ) 
    {
        c0 = vec4_t( v.c0, 0.f );
        c1 = vec4_t( v.c1, 0.f );
        c2 = vec4_t( v.c2, 0.f );
    }
    

    //! Unary minus
    VEC_FORCE_INLINE const mat44_t operator-() const
    {
        return mat44_t( -c0, -c1, -c2, -c3 );
    }

    //! Add
    VEC_FORCE_INLINE const mat44_t operator+( const mat44_t& other ) const
    {
        return mat44_t( c0 + other.c0, c1 + other.c1, c2 + other.c2, c3 + other.c3 );
    }

    //! Subtract
    VEC_FORCE_INLINE const mat44_t operator-( const mat44_t& other ) const
    {
        return mat44_t( c0 - other.c0, c1 - other.c1, c2 - other.c2, c3 - other.c3 );
    }

    //! Scalar multiplication
    VEC_FORCE_INLINE const mat44_t operator*( float scalar ) const
    {
        return mat44_t( c0 * scalar, c1 * scalar, c2 * scalar, c3 * scalar );
    }
    VEC_FORCE_INLINE const vec4_t operator*( const vec4_t& other ) const
    {
        return c0 * other.x + c1 * other.y + c2 * other.z + c3 * other.w;
    }
    
    //! Matrix multiplication
    VEC_FORCE_INLINE const mat44_t operator*( const mat44_t& other ) const
    {
        return mat44_t( 
            *this * other.c0, 
            *this * other.c1, 
            *this * other.c2,
            *this * other.c3 );
    }

    VEC_FORCE_INLINE mat44_t& operator+=( const mat44_t& other )
    {
        c0 += other.c0;
        c1 += other.c1;
        c2 += other.c2;
        c3 += other.c3;
        return *this;
    }

    //! Equals-sub
    VEC_FORCE_INLINE mat44_t& operator-=( const mat44_t& other )
    {
        c0 -= other.c0;
        c1 -= other.c1;
        c2 -= other.c2;
        c3 -= other.c3;
        return *this;
    }

    VEC_FORCE_INLINE mat44_t& operator*=( float scalar )
    {
        c0 *= scalar;
        c1 *= scalar;
        c2 *= scalar;
        c3 *= scalar;
        return *this;
    }

    VEC_FORCE_INLINE mat44_t& operator*=( const mat44_t& other )
    {
        *this = *this * other;
        return *this;
    }

        
};

