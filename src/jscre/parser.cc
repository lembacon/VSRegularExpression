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

#include "parser.h"
#include <assert.h>

namespace jscre {
namespace parser {

namespace {
namespace errmsg {

const char *end_of_expr_expected = "End-of-expression expected.";
const char *right_paren_expected = "')' expected.";
const char *right_square_expected = "']' expected.";
const char *right_curly_expected = "'}' expected.";
const char *right_curly_or_comma_expected = "'}' or ',' expected.";
const char *decimal_digit_expected = "Decimal digit expected.";
const char *unrecognized_character = "Unrecognized character.";
const char *invalid_control_escape = "Invalid control escape.";
const char *invalid_hex_escape_seq = "Invalid hexidecimal escape sequence.";
const char *invalid_uni_escape_seq = "Invalid unicode escape sequence.";
const char *invalid_char_class_range = "Invalid character class range.";
const char *invalid_quantif_range = "Invalid quantification range.";
const char *non_greedy_not_support = "Non-greedy quantification is not supported.";
const char *backref_not_support = "Backreference is not supported.";

} // end namespace errmsg
} // end namespace

Input::Input(const uint16_t *txt, size_t len)
  : length(len)
{
  assert(txt != nullptr);
  text = static_cast<uint16_t *>(malloc(sizeof(uint16_t) * (length + 1)));
  memcpy(const_cast<uint16_t *>(text), txt, sizeof(uint16_t) * length);
  const_cast<uint16_t *>(text)[length] = '\0';
}

Input::~Input()
{
  assert(text != nullptr);
  free(const_cast<uint16_t *>(text));
}

ast::ExprPtr Parser::parse()
{
  assert(_input != nullptr);
  _error = nullptr;
  _current = 0;
  _storageIndex = 0;

  ast::ExprPtr expr = _parsePattern();
  assert(expr != nullptr || _error != nullptr);

  if (_error == nullptr) {
    if (_current < _input->length) {
      _makeError(errmsg::end_of_expr_expected);
    }
  }

  if (_error == nullptr) {
    assert(expr != nullptr && _current >= _input->length);
    return expr;
  }

  return nullptr;
}

/*
 * Pattern -> Disjunction
 */
ast::ExprPtr Parser::_parsePattern()
{
  return _parseDisjunction();
}

/*
 * Disjunction -> Alternative
 * Disjunction -> Alternative | Disjunction
 */
ast::ExprPtr Parser::_parseDisjunction()
{
  ast::ExprPtr expr;
  do {
    ast::ExprPtr subExpr = _parseAlternative();
    if (subExpr == nullptr) {
      return nullptr;
    }

    if (expr == nullptr) {
      expr = subExpr;
    }
    else if (expr->getType() != ast::ExprType::Disjunction) {
      ast::ExprVector vector = { expr, subExpr };
      expr = std::make_shared<ast::DisjunctionExpr>(vector);
    }
    else {
      static_cast<ast::DisjunctionExpr *>(expr.get())->appendExpr(subExpr);
    }
  } while (_input->text[_current++] == '|');

  --_current;
  return expr;
}

/*
 * Alternative -> ϵ
 * Alternative -> Term
 * Alternative -> Term Alternative
 */
ast::ExprPtr Parser::_parseAlternative()
{
  ast::ExprPtr expr;
  while (true) {
    switch (_input->text[_current]) {
    case '\0':
    case '|':
    case ')':
      if (expr == nullptr) {
        return std::make_shared<ast::EmptyExpr>();
      }
      return expr;

    default:
      break;
    }

    ast::ExprPtr subExpr = _parseTerm();
    if (subExpr == nullptr) {
      return nullptr;
    }

    if (expr == nullptr) {
      expr = subExpr;
    }
    else if (expr->getType() != ast::ExprType::Concatenation) {
      ast::ExprVector vector = { expr, subExpr };
      expr = std::make_shared<ast::ConcatenationExpr>(vector);
    }
    else {
      static_cast<ast::ConcatenationExpr *>(expr.get())->appendExpr(subExpr);
    }
  }

  assert(false);
  return nullptr;
}

/*
 * Term -> Assertion
 * Term -> Atom
 * Term -> Atom Quantifier
 *
 * Assertion -> ^
 * Assertion -> $
 * Assertion -> \b
 * Assertion -> \B
 * Assertion -> (?= Disjunction )
 * Assertion -> (?! Disjunction )
 *
 * Quantifier -> QuantifierPrefix
 * Quantifier -> QuantifierPrefix ?
 *
 * QuantifierPrefix -> *
 * QuantifierPrefix -> +
 * QuantifierPrefix -> ?
 * QuantifierPrefix -> { DecimalDigits }
 * QuantifierPrefix -> { DecimalDigits , }
 * QuantifierPrefix -> { DecimalDigits , DecimalDigits }
 */
ast::ExprPtr Parser::_parseTerm()
{
  switch (_input->text[_current++]) {
  case '^':
    return std::make_shared<ast::AssertionExpr>(ast::AssertionType::BeginOfLine);

  case '$':
    return std::make_shared<ast::AssertionExpr>(ast::AssertionType::EndOfLine);

  case '\\':
    switch (_input->text[_current++]) {
    case 'b':
      return std::make_shared<ast::AssertionExpr>(ast::AssertionType::WordBoundary);

    case 'B':
      return std::make_shared<ast::AssertionExpr>(ast::AssertionType::NonWordBoundary);

    default:
      _current -= 2;
      break;
    }
    break;

  case '(':
    switch (_input->text[_current]) {
    case '?':
      switch (_input->text[++_current]) {
      case '=':
      case '!':
        return _parseLookAheadAssertion();

      default:
        _current -= 2;
        break;
      }
      break;

    default:
      --_current;
      break;
    }
    break;

  default:
    --_current;
    break;
  }

  ast::ExprPtr expr = _parseAtom();
  if (expr == nullptr) {
    return nullptr;
  }

  size_t minimum = 0;
  size_t maximum = 0;
  bool greedy = true;

  switch (_input->text[_current++]) {
  case '*':
    minimum = 0;
    maximum = ast::QuantificationExpr::Infinite;
    break;

  case '+':
    minimum = 1;
    maximum = ast::QuantificationExpr::Infinite;
    break;

  case '?':
    minimum = 0;
    maximum = 1;
    break;

  case '{':
    minimum = _scanDecimalDigits();
    if (_error != nullptr) {
      return nullptr;
    }
    switch (_input->text[_current++]) {
    case '}':
      maximum = minimum;
      break;

    case ',':
      if (_input->text[_current] == '}') {
        ++_current;
        maximum = ast::QuantificationExpr::Infinite;
      }
      else {
        maximum = _scanDecimalDigits();
        if (_error != nullptr) {
          return nullptr;
        }
        if (_input->text[_current++] != '}') {
          _makeError(errmsg::right_curly_expected);
          return nullptr;
        }
      }
      break;

    default:
      --_current;
      _makeError(errmsg::right_curly_or_comma_expected);
      return nullptr;
    }
    break;

  default:
    --_current;
    return expr;
  }

  if (_input->text[_current] == '?') {
    ++_current;
    greedy = false;
  }

  if (minimum > maximum) {
    _makeError(errmsg::invalid_quantif_range);
    return nullptr;
  }

  if (!greedy) {
    _makeError(errmsg::non_greedy_not_support);
    return nullptr;
  }

  return std::make_shared<ast::QuantificationExpr>(expr, minimum, maximum, greedy);
}

ast::ExprPtr Parser::_parseLookAheadAssertion()
{
  assert(_input->text[_current] == '=' ||
         _input->text[_current] == '!');

  bool inverse = _input->text[_current++] == '!';

  ast::ExprPtr subExpr = _parseDisjunction();
  if (subExpr == nullptr) {
    return nullptr;
  }

  if (_input->text[_current++] != ')') {
    _makeError(errmsg::right_paren_expected, _current - 1);
    return nullptr;
  }

  return std::make_shared<ast::LookAheadAssertionExpr>(subExpr, inverse);
}

/*
 * Atom -> PatternCharacter
 * Atom -> .
 * Atom -> \ AtomEscape
 * Atom -> CharacterClass
 * Atom -> ( Disjunction )
 * Atom -> (?: Disjunction )
 *
 * PatternCharacter -> SourceCharacter EXCEPT ^ $ \ . * + ? ( ) [ ] { } |
 * SourceCharacter -> ANY_OF Unicode code unit
 */
ast::ExprPtr Parser::_parseAtom()
{
  switch (_input->text[_current]) {
  case '.':
    ++_current;
    return std::make_shared<ast::UnspecifiedCharacterExpr>();

  case '\\':
    return _parseAtomEscape();

  case '[':
    return _parseCharacterClass();

  case '(':
    return _parseGroup();

  case '^':
  case '$':
  case '*':
  case '+':
  case '?':
  case ')':
  case ']':
  case '{':
  case '}':
  case '|':
    _makeError(errmsg::unrecognized_character);
    return nullptr;

  default:
    return std::make_shared<ast::CharacterClassExpr>(_input->text[_current++]);
  }
}

/*
 * AtomEscape -> DecimalEscape
 * AtomEscape -> CharacterEscape
 * AtomEscape -> CharacterClassEscape
 * CharacterEscape -> ControlEscape
 * CharacterEscape -> c ControlLetter
 * CharacterEscape -> HexEscapeSequence
 * CharacterEscape -> UnicodeEscapeSequence
 * CharacterEscape -> IdentityEscape
 * ControlEscape -> ONE_OF f n r t v
 * ControlLetter -> ONE_OF a-z A-Z
 * IdentityEscape -> SourceCharacter EXCEPT IdentifierPart
 * IdentityEscape -> <ZWJ>
 * IdentityEscape -> <ZWNJ>
 * DecimalEscape -> DecimalIntegerLiteral [lookahead ∉ DecimalDigit]
 * CharacterClassEscape -> ONE_OF d D s S w W
 *
 * DecimalIntegerLiteral -> 0
 * DecimalIntegerLiteral -> NonZeroDigit DecimalDigits(opt)
 *
 * HexEscapeSequence -> x HexDigit HexDigit
 * UnicodeEscapeSequence -> u HexDigit HexDigit HexDigit HexDigit
 * HexDigit -> ONE_OF 0-9 a-f A-F
 */
ast::ExprPtr Parser::_parseAtomEscape()
{
  assert(_input->text[_current] == '\\');
  switch (_input->text[++_current]) {
  case '0':
    ++_current;
    return std::make_shared<ast::CharacterClassExpr>('\0');

  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    _makeError(errmsg::backref_not_support);
    return nullptr;
    //return _parseBackreference();

  case 'f':
  case 'n':
  case 'r':
  case 't':
  case 'v':
    return _parseSimpleControlEscape();

  case 'd':
  case 'D':
  case 's':
  case 'S':
  case 'w':
  case 'W':
    return _parseSimpleCharacterClassEscape();

  case 'x':
    return _parseSimpleHexEscape();

  case 'u':
    return _parseSimpleUnicodeEscape();

  case 'c':
    return _parseComplexControlEscape();

  default:
    return std::make_shared<ast::CharacterClassExpr>(_input->text[_current++]);
  }
}

namespace {

void push_character_class_escape_into_crv(ast::CharacterRangeVector &ranges, char ch)
{
  ast::CharacterRangeVector currentRanges;
  bool inverse = false;

  switch (ch) {
  case 'D':
    inverse = true;
  case 'd':
    currentRanges.push_back(std::make_pair('0', '9'));
    break;

  case 'S':
    inverse = true;
  case 's':
    currentRanges.push_back(std::make_pair(' ', ' '));
    currentRanges.push_back(std::make_pair('\t', '\t'));
    currentRanges.push_back(std::make_pair('\r', '\r'));
    currentRanges.push_back(std::make_pair('\n', '\n'));
    break;

  case 'W':
    inverse = true;
  case 'w':
    currentRanges.push_back(std::make_pair('A', 'Z'));
    currentRanges.push_back(std::make_pair('a', 'z'));
    currentRanges.push_back(std::make_pair('0', '9'));
    currentRanges.push_back(std::make_pair('_', '_'));
    break;
    
  default:
    assert(false);
    break;
  }

  if (inverse) {
    for (auto &range: currentRanges) {
      assert(range.first <= range.second);
      if (range.first > 1) {
        ranges.push_back(std::make_pair(1, range.first - 1));
      }
      if (range.second < UINT16_MAX) {
        ranges.push_back(std::make_pair(range.second + 1, UINT16_MAX));
      }
    }
  }
  else {
    ranges.insert(ranges.end(), currentRanges.begin(), currentRanges.end());
  }
}

} // end namespace

/*
 * CharacterClass -> [ [lookahead ∉ {^}] ClassRanges ]
 * CharacterClass -> [ ^ ClassRanges ]
 * ClassRanges -> ϵ
 * ClassRanges -> NonemptyClassRanges
 * NonemptyClassRanges -> ClassAtom
 * NonemptyClassRanges -> ClassAtom NonempotyClassRangesNoDash
 * NonemptyClassRanges -> ClassAtom - ClassAtom ClassRanges
 * NonempotyClassRangesNoDash -> ClassAtom
 * NonempotyClassRangesNoDash -> ClassAtomNoDash NonempotyClassRangesNoDash
 * NonempotyClassRangesNoDash -> ClassAtomNoDash - ClassAtom ClassRanges
 * ClassAtom -> -
 * ClassAtom -> ClassAtomNoDash
 * ClassAtomNoDash -> SourceCharacter EXCEPT \ ] -
 * ClassAtomNoDash -> \ ClassEscape
 * ClassEscape -> DecimalEscape
 * ClassEscape -> b
 * ClassEscape -> CharacterEscape
 * ClassEscape -> CharacterClassEscape
 */
ast::ExprPtr Parser::_parseCharacterClass()
{
  assert(_input->text[_current] == '[');

  bool inverse = false;
  if (_input->text[++_current] == '^') {
    ++_current;
    inverse = true;
  }

  ast::CharacterRangeVector ranges;

  while (true) {
    uint16_t ch = _input->text[_current];
    if (ch == '\0') {
      _makeError(errmsg::right_square_expected);
      return nullptr;
    }
    else if (ch == ']') {
      ++_current;

      if (ranges.empty()) {
        if (inverse) {
          return std::make_shared<ast::UnspecifiedCharacterExpr>();
        }
        else {
          return std::make_shared<ast::EmptyExpr>();
        }
      }

      return std::make_shared<ast::CharacterClassExpr>(ranges, inverse);
    }

    uint16_t begin = '\0';
    switch (ch) {
    case '\\':
      switch (ch = _input->text[++_current]) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        begin = _scanDecimalDigits();
        if (_error != nullptr) {
          return nullptr;
        }
        break;

      case 'b':
        ++_current;
        begin = '\b';
        break;

      case 'f':
        ++_current;
        begin = '\f';
        break;
      case 'n':
        ++_current;
        begin = '\n';
        break;
      case 'r':
        ++_current;
        begin = '\r';
        break;
      case 't':
        ++_current;
        begin = '\t';
        break;
      case 'v':
        ++_current;
        begin = '\v';
        break;

      case 'x':
        ++_current;
        begin = _scanHexEscapeSequence();
        if (_error != nullptr) {
          return nullptr;
        }
        break;

      case 'u':
        ++_current;
        begin = _scanUnicodeEscapeSequence();
        if (_error != nullptr) {
          return nullptr;
        }
        break;

      case 'c':
        ch = _input->text[++_current];
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z')) {
          ++_current;
          begin = ch & 31;
        }
        else {
          _makeError(errmsg::invalid_control_escape);
          return nullptr;
        }
        break;

      case 'd':
      case 'D':
      case 's':
      case 'S':
      case 'w':
      case 'W':
        ++_current;
        push_character_class_escape_into_crv(ranges, ch);
        continue;

      default:
        ++_current;
        begin = ch;
        break;
      }
      break;

    default:
      ++_current;
      begin = ch;
      break;
    }

    uint16_t end = begin;
    if (_input->text[_current] == '-') {
      switch (ch = _input->text[++_current]) {
      case '\0':
      case ']':
        ranges.push_back(std::make_pair(begin, begin));
        ranges.push_back(std::make_pair('-', '-'));
        continue;

      case '\\':
        switch (ch = _input->text[++_current]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          end = _scanDecimalDigits();
          if (_error != nullptr) {
            return nullptr;
          }
          break;

        case 'b':
          ++_current;
          end = '\b';
          break;

        case 'f':
          ++_current;
          end = '\f';
          break;
        case 'n':
          ++_current;
          end = '\n';
          break;
        case 'r':
          ++_current;
          end = '\r';
          break;
        case 't':
          ++_current;
          end = '\t';
          break;
        case 'v':
          ++_current;
          end = '\v';
          break;

        case 'x':
          ++_current;
          end = _scanHexEscapeSequence();
          if (_error != nullptr) {
            return nullptr;
          }
          break;

        case 'u':
          ++_current;
          end = _scanUnicodeEscapeSequence();
          if (_error != nullptr) {
            return nullptr;
          }
          break;

        case 'c':
          ch = _input->text[++_current];
          if ((ch >= 'a' && ch <= 'z') ||
              (ch >= 'A' && ch <= 'Z')) {
            ++_current;
            end = ch & 31;
          }
          else {
            _makeError(errmsg::invalid_control_escape);
            return nullptr;
          }
          break;

        case 'd':
        case 'D':
        case 's':
        case 'S':
        case 'w':
        case 'W':
          ++_current;
          ranges.push_back(std::make_pair(begin, begin));
          ranges.push_back(std::make_pair('-', '-'));
          push_character_class_escape_into_crv(ranges, ch);
          continue;

        default:
          ++_current;
          end = ch;
          break;
        }
        break;

      default:
        ++_current;
        end = ch;
        break;
      }
    }

    if (begin > end) {
      _makeError(errmsg::invalid_char_class_range);
      return nullptr;
    }

    ranges.push_back(std::make_pair(begin, end));
  }

  assert(false);
  return nullptr;
}

/*
 * Atom -> ( Disjunction )
 * Atom -> (?: Disjunction )
 */
ast::ExprPtr Parser::_parseGroup()
{
  assert(_input->text[_current] == '(');

  bool shouldCapture = true;
  if (_input->text[++_current] == '?' &&
      _input->text[_current + 1] == ':') {
    _current += 2;
    shouldCapture = false;
  }

  size_t storageIndex = ast::GroupExpr::InvalidStorageIndex;
  if (shouldCapture) {
    storageIndex = ++_storageIndex;
  }

  ast::ExprPtr subExpr = _parseDisjunction();
  if (subExpr == nullptr) {
    return nullptr;
  }

  if (_input->text[_current++] != ')') {
    _makeError(errmsg::right_paren_expected, _current - 1);
    return nullptr;
  }

  return std::make_shared<ast::GroupExpr>(subExpr, storageIndex, shouldCapture);
}

ast::ExprPtr Parser::_parseBackreference()
{
  assert(_input->text[_current] >= '0' &&
         _input->text[_current] <= '9');

  size_t index = _scanDecimalDigits();
  if (_error != nullptr) {
    return nullptr;
  }

  return std::make_shared<ast::BackreferenceExpr>(index);
}

ast::ExprPtr Parser::_parseSimpleControlEscape()
{
  uint16_t ch = '\0';
  switch (_input->text[_current++]) {
  case 'f':
    ch = '\f';
    break;

  case 'n':
    ch = '\n';
    break;

  case 'r':
    ch = '\r';
    break;

  case 't':
    ch = '\t';
    break;

  case 'v':
    ch = '\v';
    break;

  default:
    assert(false);
    break;
  }

  return std::make_shared<ast::CharacterClassExpr>(ch);
}

ast::ExprPtr Parser::_parseComplexControlEscape()
{
  assert(_input->text[_current] == 'c');

  uint16_t ch = _input->text[++_current];
  if ((ch >= 'a' && ch <= 'z') ||
      (ch >= 'A' && ch <= 'Z')) {
    ++_current;
    return std::make_shared<ast::CharacterClassExpr>(ch & 31);
  }

  _makeError(errmsg::invalid_control_escape);
  return nullptr;
}

ast::ExprPtr Parser::_parseSimpleCharacterClassEscape()
{
  ast::CharacterRangeVector ranges;
  bool inverse = false;

  switch (_input->text[_current++]) {
  case 'D':
    inverse = true;
  case 'd':
    ranges.push_back(std::make_pair('0', '9'));
    break;

  case 'S':
    inverse = true;
  case 's':
    ranges.push_back(std::make_pair(' ', ' '));
    ranges.push_back(std::make_pair('\t', '\t'));
    ranges.push_back(std::make_pair('\r', '\r'));
    ranges.push_back(std::make_pair('\n', '\n'));
    break;

  case 'W':
    inverse = true;
  case 'w':
    ranges.push_back(std::make_pair('A', 'Z'));
    ranges.push_back(std::make_pair('a', 'z'));
    ranges.push_back(std::make_pair('0', '9'));
    ranges.push_back(std::make_pair('_', '_'));
    break;

  default:
    assert(false);
    break;
  }

  return std::make_shared<ast::CharacterClassExpr>(ranges, inverse);
}

ast::ExprPtr Parser::_parseSimpleHexEscape()
{
  assert(_input->text[_current] == 'x');

  ++_current;
  uint8_t ch = _scanHexEscapeSequence();
  if (_error != nullptr) {
    return nullptr;
  }

  return std::make_shared<ast::CharacterClassExpr>(ch);
}

ast::ExprPtr Parser::_parseSimpleUnicodeEscape()
{
  assert(_input->text[_current] == 'u');

  ++_current;
  uint16_t ch = _scanUnicodeEscapeSequence();
  if (_error != nullptr) {
    return nullptr;
  }

  return std::make_shared<ast::CharacterClassExpr>(ch);
}

/*
 * DecimalDigits -> DecimalDigit
 * DecimalDigits -> DecimalDigit DecimalDigits
 * DecimalDigit -> ONE_OF 0-9
 * NonZeroDigit -> ONE_OF 1-9
 */
size_t Parser::_scanDecimalDigits()
{
  if (_input->text[_current] < '0' ||
      _input->text[_current] > '9') {
    _makeError(errmsg::decimal_digit_expected);
    return 0;
  }

  size_t number = 0;
  do {
    number *= 10;
    number += _input->text[_current++] - '0';
  } while (_input->text[_current] >= '0' &&
           _input->text[_current] <= '9');

  return number;
}

uint8_t Parser::_scanHexEscapeSequence()
{
  uint8_t hex = '\0';
  for (size_t i = 0; i < 2; ++i, ++_current, hex <<= 4) {
    uint16_t ch = _input->text[_current];
    if (ch >= 'A' && ch <= 'F') {
      hex += ch - 'A' + 10;
    }
    else if (ch >= 'a' && ch <= 'f') {
      hex += ch - 'a' + 10;
    }
    else if (ch >= '0' && ch <= '9') {
      hex += ch - '0';
    }
    else {
      _makeError(errmsg::invalid_hex_escape_seq);
      return 0;
    }
  }

  return hex;
}

uint16_t Parser::_scanUnicodeEscapeSequence()
{
  uint16_t uni = '\0';
  for (size_t i = 0; i < 4; ++i, ++_current, uni <<= 4) {
    uint16_t ch = _input->text[_current];
    if (ch >= 'A' && ch <= 'F') {
      uni += ch - 'A' + 10;
    }
    else if (ch >= 'a' && ch <= 'f') {
      uni += ch - 'a' + 10;
    }
    else if (ch >= '0' && ch <= '9') {
      uni += ch - '0';
    }
    else {
      _makeError(errmsg::invalid_uni_escape_seq);
      return 0;
    }
  }

  return uni;
}

void Parser::_makeError(const char *message, size_t position)
{
  assert(message != nullptr);
  assert(_input != nullptr && position <= _input->length);
  _error = std::make_shared<Error>(message, position);
}

/*
 * Pattern -> Disjunction
 * Disjunction -> Alternative
 * Disjunction -> Alternative | Disjunction
 * Alternative -> ϵ
 * Alternative -> Term
 * Alternative -> Term Alternative
 * Term -> Assertion
 * Term -> Atom
 * Term -> Atom Quantifier
 * Assertion -> ^
 * Assertion -> $
 * Assertion -> \b
 * Assertion -> \B
 * Assertion -> (?= Disjunction )
 * Assertion -> (?! Disjunction )
 * Quantifier -> QuantifierPrefix
 * Quantifier -> QuantifierPrefix ?
 * QuantifierPrefix -> *
 * QuantifierPrefix -> +
 * QuantifierPrefix -> ?
 * QuantifierPrefix -> { DecimalDigits }
 * QuantifierPrefix -> { DecimalDigits , }
 * QuantifierPrefix -> { DecimalDigits , DecimalDigits }
 * Atom -> PatternCharacter
 * Atom -> .
 * Atom -> \ AtomEscape
 * Atom -> CharacterClass
 * Atom -> ( Disjunction )
 * Atom -> (?: Disjunction )
 * PatternCharacter -> SourceCharacter EXCEPT ^ $ \ . * + ? ( ) [ ] { } |
 * AtomEscape -> DecimalEscape
 * AtomEscape -> CharacterEscape
 * AtomEscape -> CharacterClassEscape
 * CharacterEscape -> ControlEscape
 * CharacterEscape -> c ControlLetter
 * CharacterEscape -> HexEscapeSequence
 * CharacterEscape -> UnicodeEscapeSequence
 * CharacterEscape -> IdentityEscape
 * ControlEscape -> ONE_OF f n r t v
 * ControlLetter -> ONE_OF a-z A-Z
 * IdentityEscape -> SourceCharacter EXCEPT IdentifierPart
 * IdentityEscape -> <ZWJ>
 * IdentityEscape -> <ZWNJ>
 * DecimalEscape -> DecimalIntegerLiteral [lookahead ∉ DecimalDigit]
 * CharacterClassEscape -> ONE_OF d D s S w W
 * CharacterClass -> [ [lookahead ∉ {^}] ClassRanges ]
 * CharacterClass -> [ ^ ClassRanges ]
 * ClassRanges -> ϵ
 * ClassRanges -> NonemptyClassRanges
 * NonemptyClassRanges -> ClassAtom
 * NonemptyClassRanges -> ClassAtom NonempotyClassRangesNoDash
 * NonemptyClassRanges -> ClassAtom - ClassAtom ClassRanges
 * NonempotyClassRangesNoDash -> ClassAtom
 * NonempotyClassRangesNoDash -> ClassAtomNoDash NonempotyClassRangesNoDash
 * NonempotyClassRangesNoDash -> ClassAtomNoDash - ClassAtom ClassRanges
 * ClassAtom -> -
 * ClassAtom -> ClassAtomNoDash
 * ClassAtomNoDash -> SourceCharacter EXCEPT \ ] -
 * ClassAtomNoDash -> \ ClassEscape
 * ClassEscape -> DecimalEscape
 * ClassEscape -> b
 * ClassEscape -> CharacterEscape
 * ClassEscape -> CharacterClassEscape
 *
 * DecimalIntegerLiteral -> 0
 * DecimalIntegerLiteral -> NonZeroDigit DecimalDigits(opt)
 * DecimalDigits -> DecimalDigit
 * DecimalDigits -> DecimalDigit DecimalDigits
 * DecimalDigit -> ONE_OF 0-9
 * NonZeroDigit -> ONE_OF 1-9
 *
 * HexEscapeSequence -> x HexDigit HexDigit
 * UnicodeEscapeSequence -> u HexDigit HexDigit HexDigit HexDigit
 * HexDigit -> ONE_OF 0-9 a-f A-F
 *
 * SourceCharacter -> ANY_OF Unicode code unit
 */

} // end namespace parser
} // end namespace jscre
