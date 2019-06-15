#include "cache_set_lru_l2.h"
#include "log.h"
#include "stats.h"

UInt8 global_loop_bit_writeback; //nss; This is the loop bit to be written back from L2 to L3

CacheSetLRUL2::CacheSetLRUL2(
	CacheBase::cache_t cache_type,
	UInt32 associativity, UInt32 blocksize, CacheSetInfoLRU* set_info, UInt8 num_attempts)
	: CacheSet(cache_type, associativity, blocksize)
    , m_num_attempts(num_attempts)
    , m_set_info(set_info)
{
	m_lru_bits = new UInt8[m_associativity];
    for (UInt32 i = 0; i < m_associativity; i++)
      m_lru_bits[i] = i;

  	loop_bit_l2 = new UInt8[m_associativity];
  	for (UInt32 i = 0; i < m_associativity; i++)
      loop_bit_l2[i] = 0;

/*   dirty_bit_l2 = new UInt8[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
      dirty_bit_l2[i] = 0; */
}

CacheSetLRUL2::~CacheSetLRUL2()
{
   delete [] m_lru_bits;
   delete [] loop_bit_l2;
   //delete [] dirty_bit_l2;
}


UInt32
CacheSetLRUL2::getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag, IntPtr eip)	//sn l3_hit_flag added
{
  // For victim selection in case of L2 miss, the priority is as follows [ARINDAM]
  // invalid blocks
  // non loop blocks
  // loop blocks
  
  //printf("getReplacementIndex for L2 called \n"); //nss
  // invalid blocks selection

  //printf("this is lapl2 and flag is %d \n", l3_hit_flag); //sn

  //printf("l3_hit_flag is %d \n", l3_hit_flag);


	for (UInt32 i = 0; i < m_associativity; i++)
    {
       if (!m_cache_block_info_array[i]->isValid())
       {
          // Mark our newly-inserted line as most-recently used
          moveToMRU(i);

          global_loop_bit_writeback=loop_bit_l2[i];  //nss
          //printf("global_loop_bit_writeback is %d \n", global_loop_bit_writeback);  //sn

          if(l3_hit_flag==0)
            loop_bit_l2[i]=0;
          else if (l3_hit_flag==1)
            loop_bit_l2[i]=1;
          else
          {
            //printf("error: l3_hit_flag is %d, printed in lapl2\n", l3_hit_flag);
          }
          //l3_hit_flag=0;


        ///////////////////////////////////////////////////////////////////////////
        /*
        for(UInt8 j=0;j<m_associativity;j++)
        {
          printf("%d ",loop_bit_l2[j]);
        }
        printf("\n");
        printf("%d th index is returned from getReplacementIndexL2 (invalid) \n",i);
        printf("************************************ \n");
        */
        ////////////////////////////////////////////////////////////////////////////

          return i;
       }
    }

   
   for(UInt8 attempt = 0; attempt < m_num_attempts; ++attempt)
   {
      UInt32 index = 0;
      UInt8 max_bits = 0;
      for (UInt32 i = 0; i < m_associativity; i++)
      {
         if (m_lru_bits[i] > max_bits && isValidReplacement(i))
         {
            index = i;
            max_bits = m_lru_bits[i];
         }
      }
      LOG_ASSERT_ERROR(index < m_associativity, "Error Finding LRU bits");

      bool qbs_reject = false;
      if (attempt < m_num_attempts - 1)
      {
         LOG_ASSERT_ERROR(cntlr != NULL, "CacheCntlr == NULL, QBS can only be used when cntlr is passed in");
         qbs_reject = cntlr->isInLowerLevelCache(m_cache_block_info_array[index]);
      }

      if (qbs_reject)
      {
         // Block is contained in lower-level cache, and we have more tries remaining.
         // Move this block to MRU and try again
         moveToMRU(index);
         cntlr->incrementQBSLookupCost();
         continue;
      }
      else
      {
         // Mark our newly-inserted line as most-recently used
        moveToMRU(index);
        m_set_info->incrementAttempt(attempt);

        global_loop_bit_writeback=loop_bit_l2[index]; //nss
        //printf("global_loop_bit_writeback is %d \n", global_loop_bit_writeback);  //sn

        if(l3_hit_flag==0)
          loop_bit_l2[index]=0;
        else if (l3_hit_flag==1)
          loop_bit_l2[index]=1;
        else
        {
          //printf("error: l3_hit_flag is %d \n", l3_hit_flag);
        }
        //l3_hit_flag=0;

        ///////////////////////////////////////////////////////////////////////////
        /*
        for(UInt8 j=0;j<m_associativity;j++)
        {
          printf("%d ",loop_bit_l2[j]);
        }
        printf("\n");
        printf("%d th index is returned from getReplacementIndexL2 (valid) \n",index);
        printf("************************************ \n");
        */
        ////////////////////////////////////////////////////////////////////////////

        return index;
      }
   }

   // comes here if no loop block is found
  

   LOG_PRINT_ERROR("Should not reach here");   
}

void
CacheSetLRUL2::updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag)
{
   //printf("updateReplacementIndex for L2 called \n"); //nss
   if(write_flag==1)
   {
    printf("write_flag in l2 is 1 \n");
    loop_bit_l2[accessed_index]=0;
   }
   m_set_info->increment(m_lru_bits[accessed_index]);
   moveToMRU(accessed_index);
}

void
CacheSetLRUL2::moveToMRU(UInt32 accessed_index)
{
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_lru_bits[i] < m_lru_bits[accessed_index])
         m_lru_bits[i] ++;
   }
   m_lru_bits[accessed_index] = 0;
}

////////////created by Arindam//////////////////sn
void
CacheSetLRUL2::updateLoopBitPolicy(UInt32 index, UInt8 loopbit)
{
  
}
//////////////////////////////////////////////////
/*
CacheSetInfoLRU::CacheSetInfoLRU(String name, String cfgname, core_id_t core_id, UInt32 associativity, UInt8 num_attempts)
   : m_associativity(associativity)
   , m_attempts(NULL)
{
   m_access = new UInt64[m_associativity];
   for(UInt32 i = 0; i < m_associativity; ++i)
   {
      m_access[i] = 0;
      registerStatsMetric(name, core_id, String("access-mru-")+itostr(i), &m_access[i]);
   }

   if (num_attempts > 1)
   {
      m_attempts = new UInt64[num_attempts];
      for(UInt32 i = 0; i < num_attempts; ++i)
      {
         m_attempts[i] = 0;
         registerStatsMetric(name, core_id, String("qbs-attempt-")+itostr(i), &m_attempts[i]);
      }
   }
};

CacheSetInfoLRU::~CacheSetInfoLRU()
{
   delete [] m_access;
   if (m_attempts)
      delete [] m_attempts;
}*/
