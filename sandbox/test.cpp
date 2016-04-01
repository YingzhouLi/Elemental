#include "El.hpp"
using namespace El;

int main( int argc, char* argv[] )
{
    Environment env( argc, argv );

    try
    {
        const Int m = Input("--m","matrix height",100);
        const Int n = Input("--n","matrix width",100);
        ProcessInput();

        DistMatrix<double> A;
        Zeros( A, m, n );
        for( int i=0; i < n; ++i )
            A.Set(i,i,double(i));
        DistMatrix<double> Ainv;
        Zeros( Ainv, m, n );
        for( int i=0; i < n; ++i )
            Ainv.Set(i,i,1.0/double(i));
        auto applyA = 
            [&]( double alpha, const DistMultiVec<double>& X,
                 double beta, DistMultiVec<double>& Y )
            {
                DistMatrix<double> XMat;
                Copy( X, XMat );
                DistMatrix<double> YMat;
                Copy( Y, YMat );
                Gemm( NORMAL, NORMAL, alpha, A, XMat, beta, YMat );    
                Copy( YMat, Y );
            };
        auto precond = 
            [&]( DistMultiVec<double>& W )
            {
                /*
                DistMatrix<double> WCopy;
                Copy( W, WCopy );
                DistMatrix<double> WMat;
                Copy( W, WMat );
                Gemm( NORMAL, NORMAL, double(1), Ainv, WCopy, double(0), WMat );
                Copy( WMat, W );
                */
            };

        DistMatrix<double> bb;
        Uniform( bb, n, 1 );
        DistMultiVec<double> b;
        Copy( bb, b );
        double relTol = 1e-12;
        int restart = 30;
        int maxIts = floor(m/restart);
        bool progress = true;

        int iter = LGMRES( applyA, precond, b, relTol,
                           restart, maxIts, progress );
    }
    catch( std::exception& e ) { ReportException(e); }

    return 0;
}
