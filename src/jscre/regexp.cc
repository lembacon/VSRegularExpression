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

#include "regexp.h"
#include "nfa.h"
#include <assert.h>
#include <sstream>
#include <vector>

namespace jscre {
namespace regexp {

Match::Match(const exec::InputPtr &input,
             const exec::OutputPtr &output)
  : _input(input),
    _output(output)
{
  assert(_input != nullptr);
  assert(_output != nullptr);
}

const uint16_t *Match::getInput() const
{
  if (_input == nullptr) {
    return nullptr;
  }

  return _input->text;
}

size_t Match::getInputLength() const
{
  if (_input == nullptr) {
    return 0;
  }

  return _input->length;
}

size_t Match::getMatchedIndex() const
{
  if (_output == nullptr) {
    return 0;
  }

  return _output->captures.front().position;
}

size_t Match::getMatchedLength() const
{
  if (_output == nullptr) {
    return 0;
  }

  return _output->captures.front().length;
}

const uint16_t *Match::getMatchedText() const
{
  if (_output == nullptr || getInput() == nullptr) {
    return nullptr;
  }

  return getInput() + getMatchedIndex();
}

size_t Match::getCapturedCount() const
{
  if (_output == nullptr) {
    return 0;
  }

  return _output->captures.size();
}

size_t Match::getCapturedTextIndex(size_t index) const
{
  if (_output == nullptr) {
    return 0;
  }

  return _output->captures[index].position;
}

size_t Match::getCapturedTextLength(size_t index) const
{
  if (_output == nullptr) {
    return 0;
  }

  return _output->captures[index].length;
}

const uint16_t *Match::getCapturedText(size_t index) const
{
  if (_output == nullptr || getInput() == nullptr) {
    return nullptr;
  }

  return getInput() + _output->captures[index].position;
}

RegExp::RegExp(const uint16_t *pattern,
               size_t patternLength,
               bool global,
               bool multiline,
               bool ignoreCase)
  : _global(global),
    _multiline(multiline),
    _ignoreCase(ignoreCase),
    _lastIndex(0),
    _pattern(std::make_shared<parser::Input>(pattern, patternLength))
{
  parser::Parser parser(_pattern);
  _expr = parser.parse();
  _error = parser.getError();

  if (_expr != nullptr) {
    _package.nfa = nfa::construct_nfa(_expr, _package.subNFAs);
  }

  _package.storageCount = parser.getStorageCount();
  _package.multiline = _multiline;
  _package.ignoreCase = _ignoreCase;
}

const uint16_t *RegExp::getPattern() const
{
  if (_pattern == nullptr) {
    return nullptr;
  }

  return _pattern->text;
}

size_t RegExp::getPatternLength() const
{
  if (_pattern == nullptr) {
    return 0;
  }

  return _pattern->length;
}

std::string RegExp::getErrorMessage() const
{
  if (_error == nullptr) {
    return "";
  }

  return _error->message;
}

size_t RegExp::getErrorPosition() const
{
  if (_error == nullptr) {
    return 0;
  }

  return _error->position;
}

std::string RegExp::toString() const
{
  std::ostringstream os;

  if (_expr != nullptr) {
    os << ast::to_string(_expr);
    os << "\n";
  }

  if (_package.nfa != nullptr) {
    os << nfa::to_string(_package.nfa, _package.subNFAs);
  }

  return os.str();
}

bool RegExp::test(const uint16_t *text, size_t textLength) const
{
  return exec(text, textLength) != nullptr;
}

MatchPtr RegExp::exec(const uint16_t *text, size_t textLength) const
{
  assert(text != nullptr);
  return _exec(std::make_shared<exec::Input>(text, textLength, _ignoreCase));
}

MatchVector RegExp::execAll(const uint16_t *text, size_t textLength) const
{
  if (_global) {
    setLastIndex(0);
  }

  assert(text != nullptr);
  exec::InputPtr input = std::make_shared<exec::Input>(text, textLength, _ignoreCase);

  MatchVector matches;

  MatchPtr match;
  while ((match = std::move(_exec(input))) != nullptr) {
    matches.push_back(std::move(match));

    if (!_global) {
      break;
    }
  }

  return std::move(matches);
}

namespace {

struct ReplaceRecord {
  std::unique_ptr<uint16_t[]> newSubStr;
  size_t newSubStrLength;
  size_t matchedIndex;
  size_t matchedLength;
};

class StringBuilder {
public:
  StringBuilder()
    : _size(0),
      _capacity(_InitialCapacity),
      _buffer(new uint16_t[_capacity]) {}

  size_t getSize() const { return _size; }
  std::unique_ptr<uint16_t[]> &getBuffer() { return _buffer; }

  void append(const uint16_t *characters, size_t count);
  void append(uint16_t ch) { append(&ch, 1); }

private:
  static constexpr size_t _InitialCapacity = 16;

  size_t _size;
  size_t _capacity;
  std::unique_ptr<uint16_t[]> _buffer;
};

void StringBuilder::append(const uint16_t *characters, size_t count)
{
  if (_size + count > _capacity) {
    do {
      _capacity <<= 1;
    } while (_size + count > _capacity);

    uint16_t *newBuffer = new uint16_t[_capacity];
    memcpy(newBuffer, _buffer.get(), sizeof(uint16_t) * _size);
    _buffer.reset(newBuffer);
  }

  memmove(_buffer.get() + _size, characters, sizeof(uint16_t) * count);
  _size += count;
}

} // end namespace

void RegExp::replace(const uint16_t *templ,
                     size_t templLength,
                     const uint16_t *input,
                     size_t inputLength,
                     uint16_t *&output,
                     size_t &outputLength) const
{
  assert(templ != nullptr);

  replace([&](const jscre::regexp::MatchPtr &match,
              std::unique_ptr<uint16_t[]> &newSubStr,
              size_t &newSubStrLength) -> void {
    StringBuilder sb;

    for (size_t i = 0; i < templLength; ++i) {
      switch (templ[i]) {
      case '$':
        break;

      default:
        sb.append(templ[i]);
        continue;
      }

      if (i == templLength - 1) {
        sb.append(templ[i]);
        break;
      }

      uint16_t ch = templ[++i];
      if (ch == '$') {
        sb.append('$');
      }
      else if (ch == '&') {
        sb.append(match->getMatchedText(), match->getMatchedLength());
      }
      else if (ch == '`') {
        sb.append(match->getInput(), match->getMatchedIndex());
      }
      else if (ch == '\'') {
        sb.append(match->getMatchedText() + match->getMatchedLength(),
                  match->getInputLength() - match->getMatchedIndex() - match->getMatchedLength());
      }
      else if (ch >= '0' && ch <= '9') {
        size_t storageIndex = 0;
        size_t j = i;
        for ( ; j < templLength && (templ[j] >= '0' && templ[j] <= '9'); ++j) {
          storageIndex *= 10;
          storageIndex += templ[j] - '0';
        }

        if (storageIndex < match->getCapturedCount()) {
          i = j - 1;

          sb.append(match->getCapturedText(storageIndex),
                    match->getCapturedTextLength(storageIndex));
        }
      }
      else {
        sb.append('$');
        sb.append(ch);
      }
    }

    newSubStr = std::move(sb.getBuffer());
    newSubStrLength = sb.getSize();
  }, input, inputLength, output, outputLength);
}

void RegExp::replace(std::function<void (const MatchPtr &match,
                                         std::unique_ptr<uint16_t[]> &newSubStr,
                                         size_t &newSubStrLength)> func,
                     const uint16_t *input,
                     size_t inputLength,
                     uint16_t *&output,
                     size_t &outputLength) const
{
  assert(func != nullptr);
  assert(input != nullptr);

  if (_global) {
    setLastIndex(0);
  }

  assert(input != nullptr);
  exec::InputPtr inputPtr = std::make_shared<exec::Input>(input, inputLength, _ignoreCase);

  std::vector<ReplaceRecord> records;

  MatchPtr match;
  while ((match = std::move(_exec(inputPtr))) != nullptr) {
    ReplaceRecord rec;
    func(match, rec.newSubStr, rec.newSubStrLength);

    rec.matchedIndex = match->getMatchedIndex();
    rec.matchedLength = match->getMatchedLength();

    records.push_back(std::move(rec));

    if (!_global) {
      break;
    }
  }

  outputLength = inputLength;
  for (auto &rec: records) {
    outputLength -= rec.matchedLength;
    outputLength += rec.newSubStrLength;
  }

  output = static_cast<uint16_t *>(malloc(sizeof(uint16_t) * outputLength));
  uint16_t *currentOutput = output;
  const uint16_t *currentInput = input;

  for (auto &rec: records) {
    assert(rec.matchedIndex >= static_cast<size_t>(currentInput - input));
    size_t remaining = rec.matchedIndex - static_cast<size_t>(currentInput - input);
    memcpy(currentOutput, currentInput, remaining * sizeof(uint16_t));
    currentOutput += remaining;
    currentInput += remaining;

    memcpy(currentOutput, rec.newSubStr.get(), rec.newSubStrLength * sizeof(uint16_t));
    currentOutput += rec.newSubStrLength;
    currentInput += rec.matchedLength;
  }

  assert(currentOutput <= output + outputLength);
  assert(currentInput <= input + inputLength);
  assert((input + inputLength - currentInput) ==
         (output + outputLength - currentOutput));

  size_t remaining = static_cast<size_t>(input + inputLength - currentInput);
  memcpy(currentOutput, currentInput, remaining * sizeof(uint16_t));
}

MatchPtr RegExp::_exec(const exec::InputPtr &input) const
{
  assert(input != nullptr);

  size_t inputStartIndex = 0;
  if (_global) {
    if (_lastIndex >= input->length) {
      _lastIndex = 0;
      return nullptr;
    }
    else {
      inputStartIndex = _lastIndex;
    }
  }

  exec::OutputPtr output = std::make_shared<exec::Output>(_package);

  while (inputStartIndex < input->length) {
    if (exec::execute(_package, *input, inputStartIndex, *output)) {
      if (_global) {
        _lastIndex = output->captures[0].position + output->captures[0].length;
      }

      return std::make_shared<Match>(input, output);
    }

    ++inputStartIndex;
  }

  if (_global) {
    _lastIndex = inputStartIndex;
  }

  return nullptr;
}

void replace(const RegExp &re,
             const uint16_t *templ,
             size_t templLength,
             const uint16_t *input,
             size_t inputLength,
             uint16_t *&output,
             size_t &outputLength)
{
  re.replace(templ, templLength, input, inputLength, output, outputLength);
}

void replace(const RegExp &re,
             std::function<void (const MatchPtr &match,
                                 std::unique_ptr<uint16_t[]> &newSubStr,
                                 size_t &newSubStrLength)> func,
             const uint16_t *input,
             size_t inputLength,
             uint16_t *&output,
             size_t &outputLength)
{
  re.replace(func, input, inputLength, output, outputLength);
}

} // end namespace regexp
} // end namespace jscre
