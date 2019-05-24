#include "cache_set_lru_l3.h"
#include "log.h"
#include "stats.h"


CacheSetLRUL3::CacheSetLRUL3(
	CacheBase::cache_t cache_type,
	UInt32 associativity, UInt32 blocksize, CacheSetInfoLRU* set_info, UInt8 num_attempts)
	: CacheSet(cache_type, associativity, blocksize)
    , m_num_attempts(num_attempts)
    , m_set_info(set_info)
{
	m_lru_bits = new UInt8[m_associativity];
    for (UInt32 i = 0; i < m_associativity; i++)
      m_lru_bits[i] = i;

  	loop_bit_l3 = new UInt8[m_associativity];
  	for (UInt32 i = 0; i < m_associativity; i++)
      loop_bit_l3[i] = 0;

/*   dirty_bit_l3 = new UInt8[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
      dirty_bit_l3[i] = 0; */

}

CacheSetLRUL3::~CacheSetLRUL3()
{
   delete [] m_lru_bits;
   delete [] loop_bit_l3;
   //delete [] dirty_bit_l3;
}


UInt32
CacheSetLRUL3::getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag)
{
  // For victim selection in case of L3 miss, the priority is as follows [ARINDAM]
  // invalid blocks
  // non loop blocks
  // loop blocks

  //printf("getReplacementIndex for L3 called \n"); //nss
  // invalid blocks selection
	for (UInt32 i = 0; i < m_associativity; i++)
    {
       if (!m_cache_block_info_array[i]->isValid())
       {
          // Mark our newly-inserted line as most-recently used
          moveToMRU(i);
          loop_bit_l3[i]=0;

        /////////////////////////////////////////////////////////////////////////////
        /*
        for(UInt8 j=0;j<m_associativity;j++)
        {
          printf("%d ",loop_bit_l3[j]);
        }
        printf("\n");
        printf("%d th index is returned from getReplacementIndexL3 (invalid) \n",i);
        */
        ////////////////////////////////////////////////////////////////////////////

          return i;
       }
    }

   // non loop blocks selection as eviction candidate [ARINDAM]
   for(UInt8 attempt = 0; attempt < m_num_attempts; ++attempt)
   {
      UInt32 index = 0;
      UInt8 max_bits = 0;
      for (UInt32 i = 0; i < m_associativity; i++)
      {
         if (m_lru_bits[i] > max_bits && isValidReplacement(i) && (loop_bit_l3[i]==0))
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

         ////////////////////////////////////////////////////////////////////////
         /*
        for(UInt8 j=0;j<m_associativity;j++)
        {
          printf("%d ",loop_bit_l3[j]);
        }
        printf("\n");
        printf("%d th index is returned from getReplacementIndexL3 (non-loop) \n",index);
        */
        ///////////////////////////////////////////////////////////////////////

         return index;
      }
   }

   // comes here if no loop block is found
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
         loop_bit_l3[index]=0;

        /////////////////////////////////////////////////////////////////////////////////// 
        /*
        for(UInt8 j=0;j<m_associativity;j++)
        {
          printf("%d ",loop_bit_l3[j]);
        }
        printf("\n");
        printf("%d th index is returned from getReplacementIndexL3 (loop) \n",index);
        */
        ///////////////////////////////////////////////////////////////////////////////////

         return index;
      }
    }

   LOG_PRINT_ERROR("Should not reach here");   
}

void
CacheSetLRUL3::updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag)
{
   //printf("updateReplacementIndex for L3 called \n"); //nss
   //printf("loop_bit_write_back_flag is %d \n", loop_bit_write_back_flag); //nss
   m_set_info->increment(m_lru_bits[accessed_index]);
   moveToMRU(accessed_index);
}

void
CacheSetLRUL3::moveToMRU(UInt32 accessed_index)
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
CacheSetLRUL3::updateLoopBitPolicy(UInt32 index, UInt8 loopbit)
{
  loop_bit_l3[index]=loopbit;

  /////////////////////////////////////////////////////////////////////////////////////////
  /*
  printf("\n loopbit writeback in l3 is %d and index for writeback is %d \n", loopbit, index);
  printf("\n lru bits of l3\n");
  for(UInt8 j=0;j<m_associativity;j++)
  {
    printf("%d ", m_lru_bits[j]);
  }

  printf("\n");

  printf("\n loopbit bits of l3 before writeback\n");
  for(UInt8 j=0;j<m_associativity;j++)
  {
    printf("%d ", loop_bit_l3[j]);
  }

  loop_bit_l3[index]=loopbit;

  printf("\n loopbit bits of l3 after writeback\n");
  for(UInt8 j=0;j<m_associativity;j++)
  {
    printf("%d ", loop_bit_l3[j]);
  }

   
    printf("\n");
    printf("************end****************** \n");
  */
  //////////////////////////////////////////////////////////////////////////////////////////

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
