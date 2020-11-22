#ifndef CDataExchange_HPP
#define CDataExchange_HPP

#include <vector>
#include "MPI_Cbindings.hpp"
#include "CTaskDivider.hpp"

using namespace CMPIWrapper;

template<class CCommMethod, typename T>
class CDataExchange{
   private:
   const std::unique_ptr<CCommMethod> CommMethod;     
   const std::unique_ptr<const CTaskDivider>& TaskInfo; // Don't allow this ptr to be changed - and its content.
   std::vector<int> rBufSize, sBufSize;
   const int whoami, npartners;
   const static int tag_alloc = 101010;

   T * __restrict__   ptrSbuf  = nullptr;   //  buffer to be sent to partners.
   T **  __restrict__ ptrRbuf  = nullptr;   //  buffer to be read by partners.

   public:
   
   CDataExchange( ) = delete;

// this constructor for case when "sender" sends same data to all its partners
// TODO: constructor that takes a vector of sBufSize_ for the case when sender has different data for different partner
   CDataExchange( const int sBufSize_ 
                , const int whoami_
                , const int npartners_
                , const std::unique_ptr<const CTaskDivider>& TaskInfo_
                )
   : whoami(whoami_), npartners(npartners_), TaskInfo(TaskInfo_)
   , CommMethod(std::make_unique<CCommMethod>( TaskInfo_ ))
   {

      rBufSize.resize(npartners);
      sBufSize.resize(npartners);

      for( auto& element : sBufSize )
      {
         element = sBufSize_;
      }

      if( npartners == 0 )return;

//      indexSbuf.resize(npartners+1);
//      indexSbuf[0] = 0;
//      std::partial_sum(bufSize.begin(), bufSize.end(), indexSbuf.begin()+1); 


// TODO: create a communicator among whoami and partners and do a mpi_ibcast instead... 
      MPI_Request request_s[npartners];
      MPI_Request request_r[npartners];

      for( int iPartner = 0; iPartner < npartners; iPartner++)
      {
         const int rank_ = TaskInfo->GetPartnerRank( iPartner );
         CMPIWrapper::Isend( &sBufSize[iPartner]
                           , 1
                           , MPI_INT
                           , rank_
                           , tag_alloc
                           , MPI_COMM_WORLD
                           , &request_s[iPartner] ); 
      }

//   For now just sending same data to all my partners. 
      ptrSbuf = CommMethod->AllocSendBuffer( sBufSize[0]);


      for( int iPartner = 0; iPartner < npartners; iPartner++)
      {
         const int rank_ = TaskInfo->GetPartnerRank( iPartner );
         CMPIWrapper::Irecv( &rBufSize[iPartner]
                           , 1
                           , MPI_INT
                           , rank_
                           , tag_alloc
                           , MPI_COMM_WORLD
                           , &request_r[iPartner] ); 
      }

      CMPIWrapper::Waitall(npartners, request_r, MPI_STATUSES_IGNORE);
      CMPIWrapper::Waitall(npartners, request_s, MPI_STATUSES_IGNORE);

      ptrRbuf = (T ** )malloc(sizeof(T*)*npartners);



      for( int iPartner = 0; iPartner < npartners; iPartner++ )
      {
         const int size_=rBufSize[iPartner];
         CommMethod->AllocRecvBuffer( &ptrRbuf[iPartner], size_, iPartner );
      }
      return;
   };
  
   ~CDataExchange()
   {
      int i = 0;
      CommMethod->FreeBuf( ptrSbuf );
      for( int iPartner = 0; iPartner < npartners; iPartner++ )
      {
         CommMethod->FreeBuf(ptrRbuf[iPartner]);
      }
      free(ptrRbuf);
      return;
   }; 


   inline void SetBuffer(const T value, const int ind)
   {
      ptrSbuf[ind] = value;
      return;
   };

   inline T ReadPartnerBuffer( const int ind, const int ipart ) const
   {
      assert(ind < rBufSize[ipart]);
      return ptrRbuf[ipart][ind];
   };
   inline int GetPartnerSize(const int ipart) const 
   {
      assert(ipart < npartners);
      return rBufSize[ipart];
   }

   void ExchangeAllData()
   {
      const int mycol = TaskInfo->GetColor();
      const int whoami = CMPIWrapper::Comm_rank(MPI_COMM_WORLD);
      for( int iTask = 0; iTask < TaskInfo->GetnTasks(); iTask++ )
      {
         if( mycol == iTask )
         {
            for( int iPartner = 0; iPartner < npartners; iPartner++ )
            {
               if(sBufSize[iPartner]==0)continue;
               CommMethod->ISendData(ptrSbuf, iPartner, sBufSize[iPartner]);
            }
         }
         else
         {
            for( int iPartner = 0; iPartner < npartners; iPartner++ )
            {
               if(rBufSize[iPartner]==0)continue;
               CommMethod->IRecvData(ptrRbuf[iPartner], iPartner, rBufSize[iPartner]);
            }
         }
      }
      /* make sure data has been transferred */      
      CommMethod->Synchronize();

   };
};
#endif
