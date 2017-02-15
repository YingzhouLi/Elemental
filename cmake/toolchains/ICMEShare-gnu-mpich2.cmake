set(CMAKE_C_COMPILER /usr/include/gcc6/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/include/gcc6/bin/g++)
set(CMAKE_Fortran_COMPILER /usr/include/gcc6/bin/gfortran)

# The MPI wrappers for the C and C++ compilers
set(MPI_COMPILER_DIR /usr/lib64/mpich2/bin/)
set(MPI_C_COMPILER       ${MPI_COMPILER_DIR}/mpicc)
set(MPI_CXX_COMPILER     ${MPI_COMPILER_DIR}/mpicxx)
set(MPI_Fortran_COMPILER ${MPI_COMPILER_DIR}/mpif90)

set(GFORTRAN_LIB /usr/include/gcc6/lib64/libgfortran.so)
