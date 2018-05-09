/*
  Multiplies a vector of characters with a scalar constant.

  This an extremely easily SIMDified case of DLP.

  (c) 2017-2018 Pekka Jääskeläinen / Parmance
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

template <typename ExecPolicy>
Duration
pixel_const_mul() {

  constexpr int DATA_SIZE = 100*1024*1024;
#ifndef HSA_BASE_PROFILE
  char *pixel_data = new char[DATA_SIZE];
#else
  char *pixel_data = (char*)hsa_alloc_shared(DATA_SIZE);
#endif
  for (size_t i = 0; i < DATA_SIZE; ++i) {
    pixel_data[i] = (char)i;
  }

  auto start_t = std::chrono::high_resolution_clock::now();

  ExecPolicy exec;
  std::for_each(exec,
		pixel_data, pixel_data + DATA_SIZE,
		[](char& c) -> void {
		  c *= 16;
		});

  auto end_t = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<Duration>(end_t - start_t);

  if constexpr (VERIFY_RESULTS) {
    for (size_t i = 0; i < DATA_SIZE; ++i) {
      if (pixel_data[i] != (char)(i * 16)) {
	std::cerr << "Fail at " << i << " should be " << (char)(i * 16) << " was "
		  << (int)pixel_data[i] << std::endl;
	abort();
      }
    }
  }
#ifndef HSA_BASE_PROFILE
  delete[] pixel_data;
#else
  hsa_memory_free(pixel_data);
#endif
  return duration;
}
