/*

  A benchmark suite for PSTL offloading that compares against
  Intel's CPU-only PSTL.

  Compile and run in a gcc build dir with a gcc par_offload build
  you want to benchmark:

  ../pstl-benchmark/build && ../pstl-benchmark/run

*/

// Copyright (C) 2017-2018 Free Software Foundation, Inc.
// Contributed by Pekka Jaaskelainen <pekka.jaaskelainen@parmance.com>
// for General Processor Tech.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.


#include <cctype>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <ctime>

const std::string COMP_DESC = "Intel PSTL";

#include <intel-pstl/algorithm>
#include <intel-pstl/execution>
#include <intel-pstl/numeric>

using pstl::execution::sequenced_policy;
using pstl::execution::parallel_unsequenced_policy;


#define _TESTING_PAR_OFFLOAD

#define ABORT_WITH_ERROR(__X__)					\
  do { std::cerr << __X__ << std::endl; abort(); } while (0)

//#define HSA_BASE_PROFILE

#ifdef HSA_BASE_PROFILE

#include <hsa.h>

/* From HSA specs.  */
/* http://www.hsafoundation.com/html/HSA_Library.htm#Runtime/Topics/02_Core/example_application_processes_allocation_service_requests_from_kernel_agent.htm?Highlight=hsa_agent_iterate_regions */

hsa_status_t get_first_agent(hsa_agent_t agent, void* data) {
  hsa_agent_t* ret = (hsa_agent_t*)data;
  *ret = agent;
  return HSA_STATUS_INFO_BREAK;
}

hsa_status_t get_fine_grained_region(hsa_region_t region, void* data) {

  hsa_region_segment_t segment;
  hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment);
  if (segment != HSA_REGION_SEGMENT_GLOBAL) {
    return HSA_STATUS_SUCCESS;
  }

  hsa_region_global_flag_t flags;
  hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);

  if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED) {
    hsa_region_t* ret = (hsa_region_t*) data;
    *ret = region;
    return HSA_STATUS_INFO_BREAK;
  }
  return HSA_STATUS_SUCCESS;
}

/* Allocate a chunk of data from a global fine grained shared region
   of the first agent found. */
void* hsa_alloc_shared(size_t size) {

  hsa_agent_t agent;
  if (hsa_iterate_agents(get_first_agent, &agent) != HSA_STATUS_INFO_BREAK) {
    ABORT_WITH_ERROR("Could not find an Agent to allocate shared data with.");
  }

  hsa_region_t region;
  hsa_agent_iterate_regions(agent, get_fine_grained_region, &region);

  void *chunk = NULL;
  hsa_memory_allocate(region, size, &chunk);
  return chunk;
}
#endif

#include <experimental/par_offload>

using std::experimental::execution::par_offload;
using std::experimental::execution::parallel_offload_policy;

typedef std::chrono::microseconds Duration;

constexpr bool RUN_HSA_ONLY = false;

constexpr bool VERIFY_RESULTS = true;

#define ENSURE(__PRED)							\
  do {									\
    if (!(__PRED)) ABORT_WITH_ERROR("Check failed: !("#__PRED ")");	\
  } while (0)

#define SCREAM(__X__)						\
  do { std::cerr << __X__ << std::endl; } while (0)


#include "benchmarks/str-to-upper.h"
#include "benchmarks/pixel-const-mul.h"
#include "benchmarks/dot-product.h"
#include "benchmarks/gamma_correction/gamma-correction.h"
#include "benchmarks/convex_hull/convex-hull.h"

typedef Duration (*BenchmarkFunction)();

Duration best_run(BenchmarkFunction f, int runs=1) {
  Duration best = Duration::max();
  for (int i = 0; i < runs; ++i) {
    Duration last = f();
    if (last < best)
      best = last;
  }
  return best;
}

void run_benchmark(std::string desc,
		   BenchmarkFunction BMSeq,
		   BenchmarkFunction BMParUnseq,
		   BenchmarkFunction BMParOffload) {

  constexpr int TITLE_COL_W = 24;
  constexpr int RUNTIME_COL_W = 8;
  constexpr int VS_SEQ_COL_W = 8;
  constexpr int VS_UNSEQ_COL_W = 8;

  using std::setw;

  std::cout << "## " << desc << std::endl << std::endl;

  Duration::rep seq_d, par_unseq_d;

  bool &offloaded = std::experimental::execution::__pstl_was_offloaded;
  if constexpr (!RUN_HSA_ONLY) {
    offloaded = false;
    std::cout << setw(TITLE_COL_W) << COMP_DESC + " seq: " << std::flush;
    Duration seq_duration = best_run(BMSeq);
    seq_d = seq_duration.count();

    std::cout << setw(RUNTIME_COL_W) << seq_d << std::endl;
    if (offloaded) abort();

    std::cout << setw(TITLE_COL_W) << COMP_DESC + " par_unseq: " << std::flush;
    Duration par_unseq_duration = best_run(BMParUnseq);
    par_unseq_d = par_unseq_duration.count();
    std::cout << setw(RUNTIME_COL_W) << par_unseq_d
	      << setw(VS_SEQ_COL_W) << std::setprecision(2)
	      << std::setiosflags(std::ios::fixed)
	      << (float)seq_d / par_unseq_d << "x" << std::endl;
    ENSURE(!offloaded);
  }

  std::cout << setw(TITLE_COL_W) << "HSA par_offload: " << std::flush;
  Duration par_offload_duration = best_run(BMParOffload);
  auto par_offload_d = par_offload_duration.count();
  std::cout << setw(RUNTIME_COL_W) << par_offload_d
	    << setw(VS_SEQ_COL_W) << std::setprecision(2);
  if constexpr (!RUN_HSA_ONLY) {
    std::cout << (float)seq_d / par_offload_d << "x"
	      << setw(VS_UNSEQ_COL_W)
	      << (float)par_unseq_d / par_offload_d << "x";
  }
  std::cout << std::endl;
  ENSURE(offloaded);
  std::cout << std::endl;
}

#define RUN(__DESC__, __BM__) \
  run_benchmark(__DESC__, __BM__<sequenced_policy>, \
		__BM__<parallel_unsequenced_policy>, \
		__BM__<parallel_offload_policy>)

#define RUN_DT(__DESC__, __BM__, __DT__)			\
  run_benchmark(__DESC__, __BM__<sequenced_policy, __DT__>,	\
		__BM__<parallel_unsequenced_policy, __DT__>,	\
		__BM__<parallel_offload_policy, __DT__>)

int main() {
#ifdef HSA_BASE_PROFILE
  hsa_init();
#endif
  // for_each() benchmarks
  RUN("for_each: Convert a 100MB std::string to upper case",
      huge_string_to_upper);
  RUN("for_each: Multiply 100MB of char data in a vector with a constant",
      pixel_const_mul);

  // transform() benchmarks
  RUN("transform: Gamma correct a 800x600 image",
      gamma_correction);

  // transform_reduce() benchmarks
  RUN_DT("transform_reduce: Dot product (100M floats)",
	 dot_product, float);
  RUN_DT("transform_reduce: Dot product (100M doubles)",
	 dot_product, double);
  RUN_DT("transform_reduce: Dot product (100M ints)",
	 dot_product, int);
  // Benchmarks using multiple PSTL algorithms
  RUN("copy_if, max_element, minmax_element: Convex hull",
      convex_hull);

  return 0;
}
