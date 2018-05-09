/*
  Gamma correction with transform that calls a lambda with private
  data.

  This is directly the Intel PSTL's gamma_correction.cpp example.
  Taken from git revision 4424e3dcfedc23e74741f9d215b5d4eadf87cc02.
  The original license is in the included files.

  The adaptation (c) 2017 Pekka Jääskeläinen / Parmance
*/

#include "utils.cpp"
#include "gamma_correction.cpp"

template <typename ExecutionPolicy>
Duration
gamma_correction() {

  const char* input_img_fn =
    "../pstl-benchmark/benchmarks/gamma_correction/input_image.bmp";
  image input_img(800, 800);
  input_img.read(input_img_fn);

  auto start_t = std::chrono::high_resolution_clock::now();
  // apply gamma
  applyGamma<decltype(input_img.rows()),
	     ExecutionPolicy>(input_img.rows(), 1.1);
  auto end_t = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<Duration>(end_t - start_t);

  if constexpr (VERIFY_RESULTS) {

    const char* correct_res_fn =
      "../pstl-benchmark/benchmarks/gamma_correction/image_gamma.bmp";
    image correct_res(800, 800);
    correct_res.read(correct_res_fn);

    ENSURE(correct_res.compare(input_img));
  }
  return duration;
}
