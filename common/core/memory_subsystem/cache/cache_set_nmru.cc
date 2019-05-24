#include "cache_set_nmru.h"
#include "log.h"

// NMRU: Not Most Recently Used

CacheSetNMRU::CacheSetNMRU(
      CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize) :
   CacheSet(cache_type, associativity, blocksize)
{
   m_lru_bits = new UInt8[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
      m_lru_bits[i] = i;

   m_replacement_pointer = 0;
}

CacheSetNMRU::~CacheSetNMRU()
{
   delete [] m_lru_bits;
}

UInt32
CacheSetNMRU::getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag)
{
   // Invalidations may mess up the LRU bits

   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (!m_cache_block_info_array[i]->isValid())
      {
         updateReplacementIndex(i,0);
         return i;
      }
   }

   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_lru_bits[m_replacement_pointer] != 0 && isValidReplacement(m_replacement_pointer))
      {
         // We choose the first line that is not MRU as the victim (note that we start searching from the replacement pointer position)
         UInt8 index = m_replacement_pointer;
         m_replacement_pointer = (m_replacement_pointer + 1) % m_associativity;
         updateReplacementIndex(index,0);
         return index;
      }

      m_replacement_pointer = (m_replacement_pointer + 1) % m_associativity;
   }

   LOG_PRINT_ERROR("Error Finding LRU bits");
}

void
CacheSetNMRU::updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag)
{
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_lru_bits[i] < m_lru_bits[accessed_index])
         m_lru_bits[i]++;
   }
   m_lru_bits[accessed_index] = 0;
}

////////////created by Arindam//////////////////sn
void
CacheSetNMRU::updateLoopBitPolicy(UInt32 index, UInt8 loopbit)
{
  
}
//////////////////////////////////////////////////
