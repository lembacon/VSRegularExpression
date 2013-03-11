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

#ifndef __jscre_parser_h__
#define __jscre_parser_h__

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <memory>
#include "ast.h"

namespace jscre {
namespace parser {

struct Input {
  const uint16_t *text;
  size_t length;

  Input(const uint16_t *txt, size_t len);
  ~Input();

  Input(const Input &) = delete;
  Input &operator=(const Input &) = delete;
};

struct Error {
  std::string message;
  size_t position;

  Error(const std::string &msg, size_t pos)
    : message(msg),
      position(pos) {}
};

typedef std::shared_ptr<Input> InputPtr;
typedef std::shared_ptr<Error> ErrorPtr;

class Parser {
public:
  explicit Parser(const InputPtr &input)
    : _input(input),
      _current(0),
      _storageIndex(0) {}

  Parser(const Parser &) = delete;
  Parser &operator=(const Parser &) = delete;

  const InputPtr &getInput() const { return _input; }
  const ErrorPtr &getError() const { return _error; }
  bool hasError() const { return getError() != nullptr; }

  size_t getStorageCount() const { return _storageIndex; }

  ast::ExprPtr parse();

private:
  ast::ExprPtr _parsePattern();
  ast::ExprPtr _parseDisjunction();
  ast::ExprPtr _parseAlternative();
  ast::ExprPtr _parseTerm();
  ast::ExprPtr _parseLookAheadAssertion();
  ast::ExprPtr _parseAtom();
  ast::ExprPtr _parseAtomEscape();
  ast::ExprPtr _parseCharacterClass();

  ast::ExprPtr _parseGroup();
  ast::ExprPtr _parseBackreference();

  ast::ExprPtr _parseSimpleControlEscape();
  ast::ExprPtr _parseComplexControlEscape();
  ast::ExprPtr _parseSimpleCharacterClassEscape();
  ast::ExprPtr _parseSimpleHexEscape();
  ast::ExprPtr _parseSimpleUnicodeEscape();

  size_t _scanDecimalDigits();
  uint8_t _scanHexEscapeSequence();
  uint16_t _scanUnicodeEscapeSequence();

  void _makeError(const char *message) { _makeError(message, _current); }
  void _makeError(const char *message, size_t position);

  InputPtr _input;
  ErrorPtr _error;

  size_t _current;
  size_t _storageIndex;
};

} // end namespace parser
} // end namespace jscre

#endif /* __jscre_parser_h__ */
