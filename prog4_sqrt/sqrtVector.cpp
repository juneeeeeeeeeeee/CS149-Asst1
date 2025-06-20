#include <stdio.h>
#include <stdlib.h>
#include <immintrin.h>


void sqrtVector(int N,
                float initialGuess,
                float values[],
                float output[])
{
    __m256 kThreshold_v = _mm256_set1_ps(0.00001f);
    __m256 three_v = _mm256_set1_ps(3.0f);
    __m256 one_v = _mm256_set1_ps(1.0f);
    __m256 half_v = _mm256_set1_ps(0.5f);
    for (int i=0; i<N; i+=8) {
        __m256 x_v = _mm256_load_ps(values+i);
        __m256 guess_v = _mm256_set1_ps(initialGuess);

        __m256 term1_v = _mm256_mul_ps(guess_v, guess_v);
        term1_v = _mm256_mul_ps(term1_v, x_v);
        __m256 error_v = _mm256_sub_ps(term1_v, one_v);

        // abs of floating point is not implemented idk why
        const __m256 sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
        error_v = _mm256_and_ps(error_v, sign_mask);

        // mask
        __m256 mask = _mm256_cmp_ps(error_v, kThreshold_v, _CMP_GT_OQ);
        while(_mm256_movemask_ps(mask))
        {
            // AVX2 does not have floating point mask operation. PoroSad
            __m256 term2_v = _mm256_mul_ps(three_v, guess_v);
            __m256 term3_1_v = _mm256_mul_ps(x_v, guess_v);
            __m256 term3_2_v = _mm256_mul_ps(guess_v, guess_v);
            __m256 term3_v = _mm256_mul_ps(term3_1_v, term3_2_v);
            guess_v = _mm256_sub_ps(term2_v, term3_v);
            guess_v = _mm256_mul_ps(guess_v, half_v);

            term1_v = _mm256_mul_ps(guess_v, guess_v);
            term1_v = _mm256_mul_ps(term1_v, x_v);
            error_v = _mm256_sub_ps(term1_v, one_v);
            error_v = _mm256_and_ps(error_v, sign_mask);
            mask = _mm256_cmp_ps(error_v, kThreshold_v, _CMP_GT_OQ);
        }

        __m256 final_output_v = _mm256_mul_ps(x_v, guess_v);
        _mm256_store_ps(output + i, final_output_v);
    }
}

