/*
   Copyright (c) 1992-2008 The University of Tennessee. 
   All rights reserved.

   Copyright (c) 2009-2012, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is partially based upon the LAPACK 
   routines dlarfg.f and zlarfg.f.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

namespace elem {
namespace internal {

//
// Follows the LAPACK convention of defining tau such that
//
//   H = I - tau [1; w] [1, w'],
//
// but adjoint(H) [chi; y] = [beta; 0]. 
//
// In our case, y=x^T, and w=v^T, so that 
//
//   (I - conj(tau) [1; v^T] [1, conj(v)]) [chi; x^T] = [beta; 0],
//
// and thus
//
//   [chi, x] (I - conj(tau) [1; v^H] [1, v]) = [beta, 0].
//
// In the case of real data, everything simplifies to
//
//   [chi, x] (I - tau [1; v^T] [1, v]) = [beta, 0].
//
// On exit, chi is overwritten with beta, and x is overwritten with v.
//
// The major difference from LAPACK is in the treatment of the special case 
// of x=0, where LAPACK would put H := I, which is not a valid Householder 
// reflector. We instead follow the FLAME convention of defining H such that 
//    adjoint(H) [chi; 0] = [-chi; 0],
// which is accomplished by setting tau=2, and v^T=0.
//

template<typename R> 
inline R
RowReflector( DistMatrix<R>& chi, DistMatrix<R>& x )
{
#ifndef RELEASE
    PushCallStack("internal::RowReflector");
    if( chi.Grid() != x.Grid() )
        throw std::logic_error
        ("chi and x must be distributed over the same grid");
    if( chi.Height() != 1 || chi.Width() != 1 )
        throw std::logic_error("chi must be a scalar");
    if( x.Height() != 1 )
        throw std::logic_error("x must be a row vector");
    if( chi.Grid().Row() != chi.ColAlignment() )
        throw std::logic_error("Reflecting with incorrect row of processes");
    if( x.Grid().Row() != x.ColAlignment() )
        throw std::logic_error("Reflecting with incorrect row of processes");
#endif
    const Grid& grid = x.Grid();
    mpi::Comm rowComm = grid.RowComm();
    const int gridCol = grid.Col();
    const int gridWidth = grid.Width();
    const int rowAlignment = chi.RowAlignment();

    std::vector<R> localNorms(gridWidth);
    R localNorm = Nrm2( x.LockedLocalMatrix() ); 
    mpi::AllGather( &localNorm, 1, &localNorms[0], 1, rowComm );
    R norm = blas::Nrm2( gridWidth, &localNorms[0], 1 );

    if( norm == 0 )
    {
        if( gridCol == rowAlignment )
            chi.SetLocalEntry(0,0,-chi.GetLocalEntry(0,0));
#ifndef RELEASE
        PopCallStack();
#endif
        return (R)2;
    }

    R alpha;
    if( gridCol == rowAlignment )
        alpha = chi.GetLocalEntry(0,0);
    mpi::Broadcast( &alpha, 1, rowAlignment, rowComm );

    R beta;
    if( alpha <= 0 )
        beta = lapack::SafeNorm( alpha, norm );
    else
        beta = -lapack::SafeNorm( alpha, norm );

    const R one = 1;
    const R safeMin = lapack::MachineSafeMin<R>();
    const R epsilon = lapack::MachineEpsilon<R>();
    const R safeInv = safeMin/epsilon;
    int count = 0;
    if( Abs(beta) < safeInv )
    {
        R invOfSafeInv = one/safeInv;
        do
        {
            ++count;
            Scale( invOfSafeInv, x );
            alpha *= invOfSafeInv;
            beta *= invOfSafeInv;
        } while( Abs(beta) < safeInv );

        localNorm = Nrm2( x.LockedLocalMatrix() );
        mpi::AllGather( &localNorm, 1, &localNorms[0], 1, rowComm );
        norm = blas::Nrm2( gridWidth, &localNorms[0], 1 );
        if( alpha <= 0 )
            beta = lapack::SafeNorm( alpha, norm );
        else
            beta = -lapack::SafeNorm( alpha, norm );
    }

    R tau = (beta-alpha)/beta;
    Scale( one/(alpha-beta), x );

    for( int j=0; j<count; ++j )
        beta *= safeInv;
    if( gridCol == rowAlignment )
        chi.SetLocalEntry(0,0,beta);
        
#ifndef RELEASE
    PopCallStack();
#endif
    return tau;
}

template<typename R>
inline Complex<R>
RowReflector
( DistMatrix<Complex<R> >& chi, DistMatrix<Complex<R> >& x )
{
#ifndef RELEASE
    PushCallStack("internal::RowReflector");    
    if( chi.Grid() != x.Grid() )
        throw std::logic_error
        ("chi and x must be distributed over the same grid");
    if( chi.Height() != 1 || chi.Width() != 1 )
        throw std::logic_error("chi must be a scalar");
    if( x.Height() != 1 )
        throw std::logic_error("x must be a row vector");
    if( chi.Grid().Row() != chi.ColAlignment() )
        throw std::logic_error("Reflecting with incorrect row of processes");
    if( x.Grid().Row() != x.ColAlignment() )
        throw std::logic_error("Reflecting with incorrect row of processes");
#endif
    typedef Complex<R> C;
    const Grid& grid = x.Grid();
    mpi::Comm rowComm = grid.RowComm();
    const int gridCol = grid.Col();
    const int gridWidth = grid.Width();
    const int rowAlignment = chi.RowAlignment();

    std::vector<R> localNorms(gridWidth);
    R localNorm = Nrm2( x.LockedLocalMatrix() ); 
    mpi::AllGather( &localNorm, 1, &localNorms[0], 1, rowComm );
    R norm = blas::Nrm2( gridWidth, &localNorms[0], 1 );

    C alpha;
    if( gridCol == rowAlignment )
        alpha = chi.GetLocalEntry(0,0);
    mpi::Broadcast( &alpha, 1, rowAlignment, rowComm );

    if( norm == (R)0 && alpha.imag == (R)0 )
    {
        if( gridCol == rowAlignment )
            chi.SetLocalEntry(0,0,-chi.GetLocalEntry(0,0));
#ifndef RELEASE
        PopCallStack();
#endif
        return (C)2;
    }

    R beta;
    if( alpha.real <= 0 )
        beta = lapack::SafeNorm( alpha.real, alpha.imag, norm );
    else
        beta = -lapack::SafeNorm( alpha.real, alpha.imag, norm );

    const R one = 1;
    const R safeMin = lapack::MachineSafeMin<R>();
    const R epsilon = lapack::MachineEpsilon<R>();
    const R safeInv = safeMin/epsilon;
    int count = 0;
    if( Abs(beta) < safeInv )
    {
        R invOfSafeInv = one/safeInv;
        do
        {
            ++count;
            Scale( invOfSafeInv, x );
            alpha *= invOfSafeInv;
            beta *= invOfSafeInv;
        } while( Abs(beta) < safeInv );

        localNorm = Nrm2( x.LockedLocalMatrix() );
        mpi::AllGather( &localNorm, 1, &localNorms[0], 1, rowComm );
        norm = blas::Nrm2( gridWidth, &localNorms[0], 1 );
        if( alpha.real <= 0 )
            beta = lapack::SafeNorm( alpha.real, alpha.imag, norm );
        else
            beta = -lapack::SafeNorm( alpha.real, alpha.imag, norm );
    }

    C tau = C( (beta-alpha.real)/beta, -alpha.imag/beta );
    Scale( one/(alpha-beta), x );

    for( int j=0; j<count; ++j )
        beta *= safeInv;
    if( gridCol == rowAlignment )
        chi.SetLocalEntry(0,0,beta);
        
#ifndef RELEASE
    PopCallStack();
#endif
    return tau;
}

} // namespace internal
} // namespace elem
