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

#ifndef __jscre_regexp_h__
#define __jscre_regexp_h__

#include "ast.h"
#include "parser.h"
#include "exec.h"
#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace jscre {
namespace regexp {

class Match {
public:
  Match(const exec::InputPtr &input,
        const exec::OutputPtr &output);

  const uint16_t *getInput() const;
  size_t getInputLength() const;

  size_t getMatchedIndex() const;
  size_t getMatchedLength() const;
  const uint16_t *getMatchedText() const;

  size_t getCapturedCount() const;
  size_t getCapturedTextIndex(size_t index) const;
  size_t getCapturedTextLength(size_t index) const;
  const uint16_t *getCapturedText(size_t index) const;

private:
  exec::InputPtr _input;
  exec::OutputPtr _output;
};

typedef std::shared_ptr<Match> MatchPtr;
typedef std::vector<MatchPtr> MatchVector;

class RegExp {
public:
  RegExp(const uint16_t *pattern,
         size_t patternLength,
         bool global = false,
         bool multiline = false,
         bool ignoreCase = false);

  RegExp(const RegExp &) = delete;
  RegExp &operator=(const RegExp &) = delete;

  bool getGlobal() const { return _global; }
  bool getMultiline() const { return _multiline; }
  bool getIgnoreCase() const { return _ignoreCase; }

  const uint16_t *getPattern() const;
  size_t getPatternLength() const;

  bool hasError() const { return _error != nullptr; }
  std::string getErrorMessage() const;
  size_t getErrorPosition() const;

  std::string toString() const;
  size_t getStorageCount() const { return _package.storageCount; }

  size_t getLastIndex() const { return _lastIndex; }
  void setLastIndex(size_t lastIndex) const { _lastIndex = lastIndex; }

  bool test(const uint16_t *text, size_t textLength) const;
  MatchPtr exec(const uint16_t *text, size_t textLength) const;

  MatchVector execAll(const uint16_t *text, size_t textLength) const;

  void replace(const uint16_t *templ,
               size_t templLength,
               const uint16_t *input,
               size_t inputLength,
               uint16_t *&output,
               size_t &outputLength) const;

  void replace(std::function<void (const MatchPtr &match,
                                   std::unique_ptr<uint16_t[]> &newSubStr,
                                   size_t &newSubStrLength)> func,
               const uint16_t *input,
               size_t inputLength,
               uint16_t *&output,
               size_t &outputLength) const;

private:
  MatchPtr _exec(const exec::InputPtr &input) const;

  bool _global;
  bool _multiline;
  bool _ignoreCase;

  mutable size_t _lastIndex;

  parser::InputPtr _pattern;
  parser::ErrorPtr _error;
  ast::ExprPtr _expr;
  exec::Package _package;
};

typedef std::shared_ptr<RegExp> RegExpPtr;

void replace(const RegExp &re,
             const uint16_t *templ,
             size_t templLength,
             const uint16_t *input,
             size_t inputLength,
             uint16_t *&output,
             size_t &outputLength);

void replace(const RegExp &re,
             std::function<void (const MatchPtr &match,
                                 std::unique_ptr<uint16_t[]> &newSubStr,
                                 size_t &newSubStrLength)> func,
             const uint16_t *input,
             size_t inputLength,
             uint16_t *&output,
             size_t &outputLength);

} // end namespace regexp
} // end namespace jscre

#endif /* __jscre_regexp_h__ */
