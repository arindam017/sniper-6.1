#ifndef CACHE_SET_PLRU_H
#define CACHE_SET_PLRU_H

#include "cache_set.h"

class CacheSetPLRU : public CacheSet
{
   public:
      CacheSetPLRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetPLRU();

      UInt32 getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag);
void updateLoopBitPolicy(UInt32 index, UInt8 loopbit); //sn

   private:
      UInt8 b[8];
};

#endif /* CACHE_SET_PLRU_H */
