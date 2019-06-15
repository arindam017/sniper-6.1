#ifndef CACHE_SET_RANDOM_H
#define CACHE_SET_RANDOM_H

#include "cache_set.h"

class CacheSetRandom : public CacheSet
{
   public:
      CacheSetRandom(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetRandom();

      UInt32 getReplacementIndex(CacheCntlr *cntlr, UInt8 l3_hit_flag, IntPtr eip);
      void updateReplacementIndex(UInt32 accessed_index, UInt8 write_flag);
void updateLoopBitPolicy(UInt32 index, UInt8 loopbit); //sn

   private:
      Random m_rand;
};

#endif /* CACHE_SET_RANDOM_H */
