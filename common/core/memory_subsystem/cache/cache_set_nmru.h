#ifndef CACHE_SET_NMRU_H
#define CACHE_SET_NMRU_H

#include "cache_set.h"

class CacheSetNMRU : public CacheSet
{
   public:
      CacheSetNMRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetNMRU();

      UInt32 getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag);
void updateLoopBitPolicy(UInt32 index, UInt8 loopbit); //sn

   private:
      UInt8* m_lru_bits;
      UInt8  m_replacement_pointer;
};

#endif /* CACHE_SET_NMRU_H */
