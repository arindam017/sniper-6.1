#include "cache_perf_model.h"
#include "cache_perf_model_parallel.h"
#include "cache_perf_model_sequential.h"
#include "log.h"

/* In my understanding, there are two cache performance models namely
 * sequential and parallel. 
 *
 * In sequential, the tags and the data is accessed  sequentially. 
 * Hence, the access time will be sum of tag_access_time + data_access_time.
 *
 * In parallel model the tag and the data is accessed in parallel. The latency
 * of access in that case should be maximum(tag_access_time, data_access_time)
 * Generally, data_access_time is larger than tag_access_time.
 */
CachePerfModel::CachePerfModel(const ComponentLatency& cache_data_access_time, const ComponentLatency& cache_tags_access_time, const ComponentLatency& cache_data_write_time):
   m_cache_data_access_time(cache_data_access_time),
   m_cache_tags_access_time(cache_tags_access_time),
   m_cache_data_write_time(cache_data_write_time)
{}

CachePerfModel::~CachePerfModel()
{}

CachePerfModel*
CachePerfModel::create(String cache_perf_model_type,
      const ComponentLatency& cache_data_access_time, const ComponentLatency& cache_tags_access_time, const ComponentLatency& cache_data_write_time)
{
   PerfModel_t perf_model = parseModelType(cache_perf_model_type);

   switch(perf_model)
   {
      case(CACHE_PERF_MODEL_PARALLEL):
         return new CachePerfModelParallel(cache_data_access_time, cache_tags_access_time,cache_data_write_time);

      case(CACHE_PERF_MODEL_SEQUENTIAL):
         return new CachePerfModelSequential(cache_data_access_time, cache_tags_access_time,cache_data_write_time);

      default:
         LOG_ASSERT_ERROR(false, "Unsupported CachePerfModel type: %s", cache_perf_model_type.c_str());
         return NULL;
   }
}

CachePerfModel::PerfModel_t
CachePerfModel::parseModelType(String model_type)
{
   if (model_type == "parallel")
   {
      return CACHE_PERF_MODEL_PARALLEL;
   }
   else if (model_type == "sequential")
   {
      return CACHE_PERF_MODEL_SEQUENTIAL;
   }
   else
   {
      LOG_PRINT_ERROR("Unsupported CacheModel type: %s", model_type.c_str());
      return NUM_CACHE_PERF_MODELS;
   }
}

