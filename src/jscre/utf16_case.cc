/* vim: set ft=cpp fenc=utf-8 sw=2 ts=2 et: */
/*
 * Copyright (c) 2013 Chongyu Zhu <lembacon@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "utf16_case.h"

#if defined(__x86_64__) && defined(__SSE2__)
#include <emmintrin.h>
#endif /* defined(__x86_64__) && defined(__SSE2__) */

namespace jscre {
namespace utf16 {

namespace {

__attribute__((always_inline))
uint16_t *to_lower_slow(uint16_t *text, size_t length)
{
  size_t i;
  for (i = 0; i < length; ++i) {
    if (__builtin_expect(text[i] <= 'Z' && text[i] >= 'A', false)) {
      text[i] += 'a' - 'A';
    }
  }
  return text;
}

__attribute__((always_inline))
uint16_t *to_upper_slow(uint16_t *text, size_t length)
{
  size_t i;
  for (i = 0; i < length; ++i) {
    if (__builtin_expect(text[i] <= 'z' && text[i] >= 'a', false)) {
      text[i] += 'A' - 'a';
    }
  }
  return text;
}

#if defined(__x86_64__) && defined(__SSE2__)
constexpr size_t alignment = sizeof(__m128i);
constexpr size_t alignment_mask = alignment - 1;
constexpr size_t alignment_length = alignment / sizeof(uint16_t);
#endif /* defined(__x86_64__) && defined(__SSE2__) */

} // end namespace

uint16_t *to_lower(uint16_t *text, size_t length)
{
  if (__builtin_expect(text == nullptr || length == 0, false)) {
    return nullptr;
  }
#if defined(__x86_64__) && defined(__SSE2__)
  else if (__builtin_expect(length < alignment_length, false)) {
    return to_lower_slow(text, length);
  }

  uint16_t *original = text;

  bool aligned = true;
  uintptr_t prealign_offset = reinterpret_cast<uintptr_t>(text) & alignment_mask;
  if (__builtin_expect(!!prealign_offset, false)) {
    size_t prealign_length = alignment - prealign_offset;
    if (__builtin_expect(!!(prealign_length & 1), false)) {
      aligned = false;
      prealign_length += 1;
    }

    prealign_length >>= 1;
    to_lower_slow(text, prealign_length);
    text += prealign_length;
    length -= prealign_length;
  }

  const __m128i AAAA = _mm_set1_epi16('A' - 1);
  const __m128i ZZZZ = _mm_set1_epi16('Z' + 1);
  const __m128i diff = _mm_set1_epi16('a' - 'A');

  if (__builtin_expect(aligned, true)) {
    while (__builtin_expect(length >= alignment_length, true)) {
      _mm_prefetch(text + alignment_length, _MM_HINT_NTA);

      const __m128i value = _mm_load_si128(reinterpret_cast<const __m128i *>(text));
      const __m128i mask = _mm_and_si128(_mm_cmpgt_epi16(value, AAAA),
                                         _mm_cmpgt_epi16(ZZZZ, value));

      const __m128i result = _mm_or_si128(_mm_andnot_si128(mask, value),
                                          _mm_and_si128(_mm_add_epi16(value, diff),
                                                        mask));
      _mm_stream_si128(reinterpret_cast<__m128i *>(text), result);

      text += alignment_length;
      length -= alignment_length;
    }

    _mm_sfence();
  }
  else {
    while (__builtin_expect(length >= alignment_length, true)) {
      _mm_prefetch(text + alignment_length, _MM_HINT_NTA);

      const __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i *>(text));
      const __m128i mask = _mm_and_si128(_mm_cmpgt_epi16(value, AAAA),
                                         _mm_cmpgt_epi16(ZZZZ, value));

      const __m128i result = _mm_or_si128(_mm_andnot_si128(mask, value),
                                          _mm_and_si128(_mm_add_epi16(value, diff),
                                                        mask));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(text), result);

      text += alignment_length;
      length -= alignment_length;
    }
  }

  if (__builtin_expect(length > 0, true)) {
    to_lower_slow(text, length);
  }

  return original;
#else /* defined(__x86_64__) && defined(__SSE2__) */
  return to_lower_slow(text, length);
#endif /* defined(__x86_64__) && defined(__SSE2__) */
}

uint16_t *to_upper(uint16_t *text, size_t length)
{
  if (__builtin_expect(text == nullptr || length == 0, false)) {
    return nullptr;
  }
#if defined(__x86_64__) && defined(__SSE2__)
  else if (__builtin_expect(length < alignment_length, false)) {
    return to_upper_slow(text, length);
  }

  uint16_t *original = text;

  bool aligned = true;
  uintptr_t prealign_offset = reinterpret_cast<uintptr_t>(text) & alignment_mask;
  if (__builtin_expect(!!prealign_offset, false)) {
    size_t prealign_length = alignment - prealign_offset;
    if (__builtin_expect(!!(prealign_length & 1), false)) {
      aligned = false;
      prealign_length += 1;
    }

    prealign_length >>= 1;
    to_upper_slow(text, prealign_length);
    text += prealign_length;
    length -= prealign_length;
  }

  const __m128i aaaa = _mm_set1_epi16('a' - 1);
  const __m128i zzzz = _mm_set1_epi16('z' + 1);
  const __m128i diff = _mm_set1_epi16('A' - 'a');

  if (__builtin_expect(aligned, true)) {
    while (__builtin_expect(length >= alignment_length, true)) {
      _mm_prefetch(text + alignment_length, _MM_HINT_NTA);

      const __m128i value = _mm_load_si128(reinterpret_cast<const __m128i *>(text));
      const __m128i mask = _mm_and_si128(_mm_cmpgt_epi16(value, aaaa),
                                         _mm_cmpgt_epi16(zzzz, value));

      const __m128i result = _mm_or_si128(_mm_andnot_si128(mask, value),
                                          _mm_and_si128(_mm_add_epi16(value, diff),
                                                        mask));
      _mm_stream_si128(reinterpret_cast<__m128i *>(text), result);

      text += alignment_length;
      length -= alignment_length;
    }

    _mm_sfence();
  }
  else {
    while (__builtin_expect(length >= alignment_length, true)) {
      _mm_prefetch(text + alignment_length, _MM_HINT_NTA);

      const __m128i value = _mm_loadu_si128(reinterpret_cast<const __m128i *>(text));
      const __m128i mask = _mm_and_si128(_mm_cmpgt_epi16(value, aaaa),
                                         _mm_cmpgt_epi16(zzzz, value));

      const __m128i result = _mm_or_si128(_mm_andnot_si128(mask, value),
                                          _mm_and_si128(_mm_add_epi16(value, diff),
                                                        mask));
      _mm_storeu_si128(reinterpret_cast<__m128i *>(text), result);

      text += alignment_length;
      length -= alignment_length;
    }
  }

  if (__builtin_expect(length > 0, true)) {
    to_upper_slow(text, length);
  }

  return original;
#else /* defined(__x86_64__) && defined(__SSE2__) */
  return to_upper_slow(text, length);
#endif /* defined(__x86_64__) && defined(__SSE2__) */
}

} // end namespace utf16
} // end namespace jscre
