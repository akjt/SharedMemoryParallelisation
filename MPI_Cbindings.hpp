#ifndef MPI_CBindings_HPP
#define MPI_CBindings_HPP 

#include <mpi.h>
#include <iostream>

#ifdef __cplusplus
namespace CMPIWrapper{
#endif

// TODO: Put assertion where relevant and check for error returns from mpi APIs. 
inline int Init( int argc, char* argv [] )
{
   return MPI_Init(&argc, &argv) ;
}

inline void Barrier(MPI_Comm comm)
{
   MPI_Barrier(comm);
   return;
} 
inline void Comm_size( MPI_Comm comm, int* size_ )
{
   MPI_Comm_size( comm, size_ ); 
}

inline void Comm_rank( MPI_Comm comm, int* rank_ )
{
   MPI_Comm_rank( comm, rank_ );
}

inline int Comm_rank( MPI_Comm comm )
{
   int rank_;
   MPI_Comm_rank( comm, &rank_ );
   return rank_;
}

inline void Comm_group( MPI_Comm comm, MPI_Group* group )
{
   MPI_Comm_group(comm, group);
   return;
}

inline void Group_translate_ranks( MPI_Group group1, const int nranks
                                 , const int* rank1, MPI_Group group2
                                 , int* rank2 )
{
   MPI_Group_translate_ranks(group1, nranks, rank1, group2, rank2 );
   return;
}

inline void Comm_split( MPI_Comm comm, const int color
                      , const int key, MPI_Comm* newcom)
{
   MPI_Comm_split(comm, color, key, newcom);
}

inline void Comm_split_type(MPI_Comm comm, int split_type, const int key, MPI_Info info, MPI_Comm* newcomm)
{
   MPI_Comm_split_type(comm, split_type, key, info, newcomm);
   return;
}

template<typename T>
inline void Win_shared_query( MPI_Win win, int rank, MPI_Aint* size
                            , int* disp_unit, T** bufptr ) 
{
   MPI_Win_shared_query( win, rank, size, disp_unit, bufptr );
   return;
}

template<typename T>
inline void Win_allocate_shared( MPI_Aint size, int disp, MPI_Info info
                               , MPI_Comm comm, T** bufptr, MPI_Win* win ) 
{
   int ierr = MPI_Win_allocate_shared( size, disp, info, comm, bufptr, win );
   return;
}

inline void Info_create(MPI_Info* info)
{
   MPI_Info_create(info);
   return;
}

inline void Info_set(MPI_Info info, const char* key, const char* value)
{
   MPI_Info_set(info, key, value);
   return;
}

inline void Type_get_extent(MPI_Datatype dtype, MPI_Aint* lb, MPI_Aint* size)
{
   MPI_Type_get_extent(dtype, lb, size);
   return;
}

template<typename T>
inline void Send( T* data, int size, MPI_Datatype type, const int rank
                , const int tag, MPI_Comm comm )
{
   MPI_Send( data, size, type, rank, tag, comm);
}

template<typename T>
inline void Recv( T* data, int size, MPI_Datatype type, const int rank
                , const int tag, MPI_Comm comm, MPI_Status* status )
{
   MPI_Recv( data, size, type, rank, tag, comm, status);
}

template<typename T>
inline void Reduce( const T* data_in, T* data_out, int size, MPI_Datatype type
                  , MPI_Op operation, const int root, MPI_Comm comm )
{
   MPI_Reduce( data_in, data_out, size, type, operation, root, comm);
}


template<typename T>
inline void Isend( T* data, int size, MPI_Datatype type, const int rank
                , const int tag, const MPI_Comm comm, MPI_Request* req )
{
   MPI_Isend( data, size, type, rank, tag, comm, req );
}

template<typename T>
inline void Irecv( T* data, int size, MPI_Datatype type, const int rank
                , const int tag, const MPI_Comm comm, MPI_Request* req )
{
   MPI_Irecv( data, size, type, rank, tag, comm, req );
}
inline void Wait( MPI_Request* req, MPI_Status* status )
{
   MPI_Wait( req, status );
   return;
}

inline void Waitall(const int nReq, MPI_Request* req, MPI_Status* status )
{
   MPI_Waitall( nReq, req, status );
   return;
}

inline void Abort(MPI_Comm comm, const char * ErrorMsg )
{
   std::cout << " ERROR MESSAGE: " << ErrorMsg << ". Aborting now...";
   MPI_Abort(comm, 1);
   return;
}


} // namespace CMPIWrapper

#endif
