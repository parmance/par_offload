/*
  Dot product with transform_reduce.

  Adapted from the Intel PSTL dot_product example. The original
  license and copyright below.

  The adaptation (c) 2017 Pekka Jääskeläinen / Parmance
*/

/*
    Copyright (c) 2017 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <vector>
#include <random>
#include <iostream>
#include <cmath>

template <typename ExecPolicy, typename DataType=double>
Duration
dot_product() {

  constexpr size_t size = 100000000;

#ifndef HSA_BASE_PROFILE
  DataType *v1 = new DataType[size];
  DataType *v2 = new DataType[size];
#else
  DataType *v1 = (DataType*)hsa_alloc_shared(size*sizeof(DataType));
  DataType *v2 = (DataType*)hsa_alloc_shared(size*sizeof(DataType));
#endif
  assert(v1 != nullptr);
  assert(v2 != nullptr);

  for (size_t i = 0; i < size; ++i) {
    v1[i] = (DataType)i;
    v2[i] = (DataType)i;
  }

  auto start_t = std::chrono::high_resolution_clock::now();
  ExecPolicy exec;
  DataType res =
    std::transform_reduce(exec, v1, v1 + size, v2, (DataType)0,
			  std::plus<DataType>(), std::multiplies<DataType>());
  auto end_t = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<Duration>(end_t - start_t);

  if constexpr (VERIFY_RESULTS) {
      if constexpr (std::is_same<DataType, double>::value) {
	DataType delta = fabsf(res - 3.40696e+23);
	if (delta > 1.0e+24) {
	  ABORT_WITH_ERROR("Result wrong, got: " << res << " delta == "
			   << delta << std::endl);
	}
      } else if constexpr (std::is_same<DataType, float>::value) {
	DataType delta = fabsf(res - 3.40696e+23);
	if (delta > 1.0e+23)
	  ABORT_WITH_ERROR("Result wrong, got: " << res << " delta == "
			   << delta << std::endl);
      } else {
	if (res != -1452071552)
	  ABORT_WITH_ERROR("Result wrong, got " << res);
      }
  }
#ifndef HSA_BASE_PROFILE
  delete[] v1;
  delete[] v2;
#else
  hsa_memory_free(v1);
  hsa_memory_free(v2);
#endif

  return duration;
}
