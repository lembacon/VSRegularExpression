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

#ifndef __jscre_exec_h__
#define __jscre_exec_h__

#include "nfa.h"
#include <memory>
#include <vector>

namespace jscre {
namespace exec {

struct Package {
  nfa::NFAPtr nfa;
  nfa::LookAheadNFAMap subNFAs;
  size_t storageCount;
  bool multiline;
  bool ignoreCase;
};

struct Input {
  const uint16_t *text;
  size_t length;

  Input(const uint16_t *txt, size_t len, bool ignoreCase = false);
  ~Input();

  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;
};

struct Range {
  size_t position;
  size_t length;

  Range()
    : position(NotFound),
      length(0) {}

  static constexpr size_t NotFound = SIZE_MAX;
};

struct Output {
  std::vector<Range> captures;

  explicit Output(const Package &package)
    : captures(1 + package.storageCount) {}
};

typedef std::shared_ptr<Input> InputPtr;
typedef std::shared_ptr<Output> OutputPtr;

bool execute(const Package &package, const Input &input, size_t inputStartIndex, Output &output);

} // end namespace exec
} // end namespace jscre

#endif /* __jscre_exec_h__ */
