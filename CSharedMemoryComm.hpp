#ifndef CSharedMemoryComm_HPP
#define CSharedMemoryComm_HPP

#include <mpi.h>
#include <assert.h>
#include "MPI_Cbindings.hpp"


template<typename T>
class CSharedMemoryComm{

   private:
   MPI_Win win = MPI_WIN_NULL;
   MPI_Info infoAlloc; 
   MPI_Datatype MPIDataType;
   MPI_Comm MPI_COMM_SHM = MPI_COMM_NULL;
   const std::vector<int>& partners; 

   const std::unique_ptr<const CTaskDivider>& TaskInfo; // Don't allow this ptr to be changed - and its content.

   public: 

   CSharedMemoryComm( const std::unique_ptr<const CTaskDivider>& TaskInfo_ ) 
   : TaskInfo(TaskInfo_), partners(TaskInfo_->GetAllPartnerRanks())
   {
      int contig_alloc = 0;
      CMPIWrapper::Info_create( &infoAlloc );

      if(contig_alloc == 0)
      {
         CMPIWrapper::Info_set( infoAlloc, "alloc_shared_noncontig", "true" );
      }else
      {
         infoAlloc = MPI_INFO_NULL;
      }

      if constexpr ( std::is_same_v<T, float> )
      {
         MPIDataType = MPI_REAL;
      }
      else if constexpr ( std::is_same_v<T, double> )
      {
         MPIDataType = MPI_DOUBLE_PRECISION; 
      }
      else if constexpr ( std::is_same_v<T, int> )
      {
         MPIDataType = MPI_INTEGER;
      }
      else
      {
         assert(1==1 && "The datatype used undefined"); // checking
      }

      int key = 0; // Don't care how ranks will be assigned to MPI tasks in the new communicator.
      CMPIWrapper::Comm_split_type( MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, key, MPI_INFO_NULL, &MPI_COMM_SHM);

   };


   T* AllocSendBuffer( const int bufSize )
   {  
      MPI_Aint size_of_T, lb, size_bytes;
      T* shmSbufPtr;

      CMPIWrapper::Type_get_extent(MPIDataType, &lb, &size_of_T);

      size_bytes    = bufSize*size_of_T;
      int disp_unit = size_of_T;

      CMPIWrapper::Win_allocate_shared( size_bytes, disp_unit, infoAlloc
                                      , MPI_COMM_SHM, &shmSbufPtr, &win);
      return shmSbufPtr;
   };
    

   void AllocRecvBuffer( T **shmRbufPtr, const int bufsize_, const int iPartner)
   {
      int disp_unit, size_1; 
      const int rank_ = partners[iPartner];
      MPI_Aint size_;
      CMPIWrapper::Win_shared_query( win, rank_, &size_
                                    , &disp_unit, shmRbufPtr  );
                                    
      size_1 = static_cast<int>(size_)/disp_unit;
      if( bufsize_ > size_1 )CMPIWrapper::Abort(MPI_COMM_WORLD 
         , " allocated shared buffer seems smaller than expected! ");

      return;
   };

   void ISendData( T* buf, const int iPartner, const int bufsize )const
   {
      return;
   };

   void IRecvData( T* buf, const int iPartner, const int bufsize )const
   {
      return;
   };

   void Synchronize( )const
   {
      CMPIWrapper::Barrier(MPI_COMM_SHM);
      return;
   };

   void FreeBuf( T* buf ) 
   {
      if(win != MPI_WIN_NULL)MPI_Win_free( &win );
      buf = nullptr;
      return;
   }

   ~CSharedMemoryComm( )
   {
      if(win != MPI_WIN_NULL)MPI_Win_free( &win );
      if(MPI_COMM_SHM != MPI_COMM_NULL) MPI_Comm_free( &MPI_COMM_SHM );
      return;
   }

   
};

#endif
