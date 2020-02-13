#pragma once

struct Vector4SoA
{
    __m128 xxxx;
    __m128 yyyy;
    __m128 zzzz;
    __m128 wwww;
};
void Load( Vector4SoA* soa, const vec4_t& a, const vec4_t& b, const vec4_t& c, const vec4_t& d )
{
    /*
    xyzw xyzw xyzw xyzw

    tmp0 = ax|ay|bx|by
    tmp1 = cx|cy|dx|dy
    tmp2 = az|aw|bz|bw
    tmp3 = cz|cw|dz|dw

    ax|bx|cx|dx
    ay|by|cy|dy
    az|bz|cz|dz
    aw|bw|cw|dw

    */

    const __m128& av = (__m128&)a;
    const __m128& bv = (__m128&)b;
    const __m128& cv = (__m128&)c;
    const __m128& dv = (__m128&)d;

    const __m128 tmp0 = _mm_shuffle_ps( av, bv, _MM_SHUFFLE( 1, 0, 1, 0 ) );
    const __m128 tmp1 = _mm_shuffle_ps( cv, dv, _MM_SHUFFLE( 1, 0, 1, 0 ) );
    const __m128 tmp2 = _mm_shuffle_ps( av, bv, _MM_SHUFFLE( 3, 2, 3, 2 ) );
    const __m128 tmp3 = _mm_shuffle_ps( cv, dv, _MM_SHUFFLE( 3, 2, 3, 2 ) );

    soa->xxxx = _mm_shuffle_ps( tmp0, tmp1, _MM_SHUFFLE( 2, 0, 2, 0 ) );
    soa->yyyy = _mm_shuffle_ps( tmp0, tmp1, _MM_SHUFFLE( 3, 1, 3, 1 ) );
    soa->zzzz = _mm_shuffle_ps( tmp2, tmp3, _MM_SHUFFLE( 2, 0, 2, 0 ) );
    soa->wwww = _mm_shuffle_ps( tmp2, tmp3, _MM_SHUFFLE( 3, 1, 3, 1 ) );
}
void Store( vec4_t* a, vec4_t* b, vec4_t* c, vec4_t* d, const Vector4SoA& soa )
{
    /*
    ax|bx|cx|dx
    ay|by|cy|dy
    az|bz|cz|dz
    aw|bw|cw|dw

    tmp0 = ax|bx|ay|by
    tmp1 = cx|dx|cy|dy
    tmp2 = az|bz|aw|bw
    tmp3 = cz|dz|cw|dw

    xyzw xyzw xyzw xyzw
    */

    const __m128 tmp0 = _mm_shuffle_ps( soa.xxxx, soa.yyyy, _MM_SHUFFLE( 1, 0, 1, 0 ) );
    const __m128 tmp1 = _mm_shuffle_ps( soa.xxxx, soa.yyyy, _MM_SHUFFLE( 3, 2, 3, 2 ) );
    const __m128 tmp2 = _mm_shuffle_ps( soa.zzzz, soa.wwww, _MM_SHUFFLE( 1, 0, 1, 0 ) );
    const __m128 tmp3 = _mm_shuffle_ps( soa.zzzz, soa.wwww, _MM_SHUFFLE( 3, 2, 3, 2 ) );

    _mm_store_ps( a->xyzw, _mm_shuffle_ps( tmp0, tmp2, _MM_SHUFFLE( 2, 0, 2, 0 ) ) );
    _mm_store_ps( b->xyzw, _mm_shuffle_ps( tmp0, tmp2, _MM_SHUFFLE( 3, 1, 3, 1 ) ) );
    _mm_store_ps( c->xyzw, _mm_shuffle_ps( tmp1, tmp3, _MM_SHUFFLE( 2, 0, 2, 0 ) ) );
    _mm_store_ps( d->xyzw, _mm_shuffle_ps( tmp1, tmp3, _MM_SHUFFLE( 3, 1, 3, 1 ) ) );
}


struct MatrixSoA
{
    Vector4SoA x;
    Vector4SoA y;
    Vector4SoA z;
    Vector4SoA w;
};
void Load( MatrixSoA* soa, const mat44_t& a, const mat44_t& b, const mat44_t& c, const mat44_t& d )
{
    Load( &soa->x, a.c0, b.c0, c.c0, d.c0 );
    Load( &soa->y, a.c1, b.c1, c.c1, d.c1 );
    Load( &soa->z, a.c2, b.c2, c.c2, d.c2 );
    Load( &soa->w, a.c3, b.c3, c.c3, d.c3 );
}
void Store( mat44_t* a, mat44_t* b, mat44_t* c, mat44_t* d, const MatrixSoA& soa )
{
    Store( &a->c0, &b->c0, &c->c0, &d->c0, soa.x );
    Store( &a->c1, &b->c1, &c->c1, &d->c1, soa.y );
    Store( &a->c2, &b->c2, &c->c2, &d->c2, soa.z );
    Store( &a->c3, &b->c3, &c->c3, &d->c3, soa.w );
}

Vector4SoA Mul( const MatrixSoA& m, const Vector4SoA& v )
{
    Vector4SoA result;
    result.xxxx = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( m.x.xxxx, v.xxxx ), _mm_mul_ps( m.y.xxxx, v.yyyy ) ), _mm_mul_ps( m.z.xxxx, v.zzzz ) ), _mm_mul_ps( m.w.xxxx, v.wwww ) );
    result.yyyy = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( m.x.yyyy, v.xxxx ), _mm_mul_ps( m.y.yyyy, v.yyyy ) ), _mm_mul_ps( m.z.yyyy, v.zzzz ) ), _mm_mul_ps( m.w.yyyy, v.wwww ) );
    result.zzzz = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( m.x.zzzz, v.xxxx ), _mm_mul_ps( m.y.zzzz, v.yyyy ) ), _mm_mul_ps( m.z.zzzz, v.zzzz ) ), _mm_mul_ps( m.w.zzzz, v.wwww ) );
    result.wwww = _mm_add_ps( _mm_add_ps( _mm_add_ps( _mm_mul_ps( m.x.wwww, v.xxxx ), _mm_mul_ps( m.y.wwww, v.yyyy ) ), _mm_mul_ps( m.z.wwww, v.zzzz ) ), _mm_mul_ps( m.w.wwww, v.wwww ) );

    return result;
}

MatrixSoA Mul( const MatrixSoA& a, const MatrixSoA& b )
{
    MatrixSoA result;
    result.x = Mul( a, b.x );
    result.y = Mul( a, b.y );
    result.z = Mul( a, b.z );
    result.w = Mul( a, b.w );

    return result;
}

void SoaTest()
{
    //vec4_t v4_aos[4] = 
    //{
    //    vec4_t( 10, 20, 30, 40 ),
    //    vec4_t( 11, 21, 31, 41 ),
    //    vec4_t( 12, 22, 32, 42 ),
    //    vec4_t( 13, 23, 33, 43 ),
    //};

    //mat44_t mat_aos[4] =
    //{
    //    mat44_t::scale( vec3_t( 1.f ) ),
    //    mat44_t::scale( vec3_t( 2.f ) ),
    //    mat44_t::scale( vec3_t( 3.f ) ),
    //    mat44_t::scale( vec3_t( 4.f ) ),
    //};

    //Vector4SoA v4_soa;
    //Load( &v4_soa, v4_aos[0], v4_aos[1], v4_aos[2], v4_aos[3] );

    //
    //vec4_t v4_aos1[4];
    //Store( &v4_aos1[0], &v4_aos1[1], &v4_aos1[2], &v4_aos1[3], v4_soa );
    //
    //MatrixSoA mat_soa;
    //Load( &mat_soa, mat_aos[0], mat_aos[1], mat_aos[2], mat_aos[3] );
    //
    //mat44_t mat_aos1[4];
    //for( u32 i = 0; i < 4; ++i )
    //{
    //    mat_aos1[i] = mat_aos[i] * mat_aos[i];
    //}
    //
    //MatrixSoA soa1 = Mul( mat_soa, mat_soa );
    //mat44_t mat_aos2[4];
    //Store( &mat_aos2[0], &mat_aos2[1], &mat_aos2[2], &mat_aos2[3], soa1 );

    //int a = 0;

    random_t rnd = RandomInit( 0xBAADC0DEDEADF00D, 1ull );

    constexpr u32 COUNT = 1024 * 1024;
    mat44_t* aos_a = new mat44_t[COUNT];
    mat44_t* aos_b = new mat44_t[COUNT];
    mat44_t* aos_c = new mat44_t[COUNT];


    for( u32 i = 0; i < COUNT; ++i )
    {
        const f32 scale_a = Randomf( &rnd, 0.1f, 2.f );
        const f32 trans_a = Randomf( &rnd, 1.f, 5.f );
        const f32 rotat_a = Randomf( &rnd, -PI, PI );

        const f32 scale_b = Randomf( &rnd, 0.1f, 2.f );
        const f32 trans_b = Randomf( &rnd, 1.f, 5.f );
        const f32 rotat_b = Randomf( &rnd, -PI, PI );

        aos_a[i] = mat44_t::scale( vec3_t( scale_a ) ) * mat44_t::translation( vec3_t( trans_a ) ) * mat44_t::rotation( rotat_a, normalize( vec3_t( rotat_a ) ) );
        aos_b[i] = mat44_t::scale( vec3_t( scale_b ) ) * mat44_t::translation( vec3_t( trans_b ) ) * mat44_t::rotation( rotat_b, normalize( vec3_t( rotat_b ) ) );
    }

    constexpr u32 N_TESTS = 10;
    constexpr u32 LOOPS = 128;


    for( u32 itest = 0; itest < N_TESTS; ++itest )
    {
        BXTimeQuery time_query = BXTimeQuery::Begin();

#if 1
        for( u32 loop = 0; loop < LOOPS; ++loop )
        {
            for( u32 i = 0; i < COUNT; ++i )
            {
                aos_c[i] = aos_b[i] * aos_a[i];
            }
        }
#else
        for( u32 loop = 0; loop < LOOPS; ++loop )
        {
            const u32 n_quads = COUNT / 4;
            for( u32 q = 0; q < n_quads; ++q )
            {
                MatrixSoA soa_a;
                MatrixSoA soa_b;
                Load( &soa_a, aos_a[q + 0], aos_a[q + 1], aos_a[q + 2], aos_a[q + 3] );
                Load( &soa_b, aos_b[q + 0], aos_b[q + 1], aos_b[q + 2], aos_b[q + 3] );

                MatrixSoA soa_c = Mul( soa_a, soa_b );

                Store( &aos_c[q + 0], &aos_c[q + 1], &aos_c[q + 2], &aos_c[q + 3], soa_c );
            }
        }
#endif

        BXTimeQuery::End( &time_query );

        printf( "test%u duration: %.5f [s]\n", itest, time_query.DurationS() );
    }


    delete[] aos_c;
    delete[] aos_b;
    delete[] aos_a;
}
