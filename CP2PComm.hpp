#ifndef CP2PComm_HPP
#define CP2PComm_HPP

#include <mpi.h>
#include "MPI_Cbindings.hpp"


template<typename T>
class CP2PComm{

   private:
   const static int tag_data  = 202020;

   MPI_Datatype MPIDataType;
   MPI_Request* request;
   
   const std::unique_ptr<const CTaskDivider>& TaskInfo; // Don't allow this ptr to be changed - and its content.

   const std::vector<int>& partners; //
   const int npartners;

   public: 

   CP2PComm( const std::unique_ptr<const CTaskDivider>& TaskInfo_ ) 
   : TaskInfo(TaskInfo_), partners(TaskInfo_->GetAllPartnerRanks())
   , npartners(TaskInfo->GetNPartners()) 
   {
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
 // static_assert does not work with constexpr - it will be invoked regardsless of above!
         assert(1==1 && "The datatype used undefined"); // runtime checking
      }

      if( (request = (MPI_Request*) malloc(sizeof(MPI_Request)*npartners))
      == NULL )CMPIWrapper::Abort(MPI_COMM_WORLD, "Failed to allocate request");

      return;
   };


   T* AllocSendBuffer( const int bufSize )
   {  
      T* bufPtr;

      if( (bufPtr = (T*) malloc(bufSize*sizeof(T)))==NULL )
         CMPIWrapper::Abort(MPI_COMM_WORLD, "Failed to allocate buffer");

      return bufPtr;
   };
    

   void AllocRecvBuffer( T **bufPtr, const int bufSize, const int iPartner )
   {

      const int rank_ = partners[iPartner];


      if( (*bufPtr = (T*) malloc((bufSize)*sizeof(T)))==NULL )
         CMPIWrapper::Abort(MPI_COMM_WORLD, "Failed to allocate buffer");

      return;
   };

   void ISendData( T* buf, const int iPartner, const int bufSize )
   {
      const int whoami = CMPIWrapper::Comm_rank(MPI_COMM_WORLD);
      const int rank_= partners[iPartner];

      CMPIWrapper::Isend( buf
                        , bufSize
                        , MPIDataType
                        , rank_
                        , tag_data
                        , MPI_COMM_WORLD
                        , &request[iPartner]); 
      return;
   };

   void IRecvData(  T* buf, const int iPartner, const int bufSize  )
   {
      const int whoami = CMPIWrapper::Comm_rank(MPI_COMM_WORLD);
      const int rank_= partners[iPartner];

      CMPIWrapper::Irecv( buf
                        , bufSize
                        , MPIDataType
                        , rank_
                        , tag_data
                        , MPI_COMM_WORLD
                        , &request[iPartner] ); 
      return;
   };

   void Synchronize( )
   {
      CMPIWrapper::Waitall(npartners, request, MPI_STATUSES_IGNORE);
      return;
   };


   void FreeBuf( T* buf ) 
   {
      free(buf);
      return;
   };

   ~CP2PComm( )
   {
      for( int iPartner = 0; iPartner < npartners; iPartner++)
      {
         if(request[iPartner] != MPI_REQUEST_NULL)
         {
            MPI_Request_free(&request[iPartner]);
         }
      }
      free(request);
      return;
   };

};

#endif
