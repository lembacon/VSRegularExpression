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

#include "exec.h"
#include "utf16_case.h"
#include <assert.h>

namespace jscre {
namespace exec {

Input::Input(const uint16_t *txt, size_t len, bool ignoreCase)
  : length(len)
{
  assert(txt != nullptr);
  text = static_cast<uint16_t *>(malloc(sizeof(uint16_t) * (length + 1)));
  memcpy(const_cast<uint16_t *>(text), txt, sizeof(uint16_t) * length);
  const_cast<uint16_t *>(text)[length] = '\0';

  if (ignoreCase) {
    utf16::to_lower(const_cast<uint16_t *>(text), length);
  }
}

Input::~Input()
{
  assert(text != nullptr);
  free(const_cast<uint16_t *>(text));
}

namespace {

struct State {
  nfa::NodePtr node;
  size_t currentEdge;
  size_t currentText;

  State(const nfa::NodePtr &n, size_t curText)
    : node(n),
      currentEdge(0),
      currentText(curText) {}
};

typedef std::vector<State> StateVector;

struct Candidate {
  StateVector states;
  size_t length;

  Candidate(const StateVector &sts,
            size_t len)
    : states(sts),
      length(len) {}
};

typedef std::vector<Candidate> CandidateVector;

bool test_character_set(const ast::CharacterClassExpr *expr, uint16_t ch, bool ignoreCase)
{
  if (ch == '\0') {
    return false;
  }

  if (expr->isInverse()) {
    for (auto &range: expr->getRanges()) {
      if (range.first <= ch && range.second >= ch) {
        return false;
      }

      if (ignoreCase) {
        if (ch >= 'A' && ch <= 'Z') {
          uint16_t ch2 = ch - 'A' + 'a';
          if (range.first <= ch2 && range.second >= ch2) {
            return false;
          }
        }
      }
    }

    return true;
  }
  else {
    for (auto &range: expr->getRanges()) {
      if (range.first <= ch && range.second >= ch) {
        return true;
      }

      if (ignoreCase) {
        if (ch >= 'A' && ch <= 'Z') {
          uint16_t ch2 = ch - 'A' + 'a';
          if (range.first <= ch2 && range.second >= ch2) {
            return true;
          }
        }
      }
    }

    return false;
  }
}

bool is_word_char(uint16_t ch)
{
  if (ch >= 0x1f) {
    return true;
  }
  else if ((ch >= 'A' && ch <= 'Z') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= '0' && ch <= '9')) {
    return true;
  }
  else {
    return false;
  }
}

void find_all_candidates(const Package &package,
                         const nfa::NFAPtr &nfa,
                         const Input &input,
                         size_t inputStartIndex,
                         CandidateVector &candidates)
{
  assert(inputStartIndex <= input.length);
  const uint16_t *textStart = input.text + inputStartIndex;
  size_t textLength = input.length - inputStartIndex;

  assert(nfa->start != nullptr);
  assert(nfa->end != nullptr);
  assert(!nfa->start->edges.empty());

  StateVector states;
  states.push_back(State(nfa->start, 0));

  while (states.size() > 0) {
    State &currentState = states.back();
    if (currentState.currentEdge >= currentState.node->edges.size()) {
      states.pop_back();
      continue;
    }

    nfa::EdgePtr &currentEdge = currentState.node->edges[currentState.currentEdge++];
    size_t currentText = currentState.currentText;
    bool pass = false;

    assert(currentText <= textLength);

    switch (currentEdge->type) {
    case nfa::EdgeType::CharacterSet:
      if (test_character_set(currentEdge->expr, textStart[currentText], package.ignoreCase)) {
        ++currentText;
        pass = true;
      }
      break;

    case nfa::EdgeType::Assertion:
      switch (currentEdge->assertion->getAssertionType()) {
      case ast::AssertionType::BeginOfLine:
        if (package.multiline) {
          if (currentText == 0) {
            pass = true;
          }
          else {
            switch (textStart[currentText - 1]) {
            case '\r':
            case '\n':
            case 0x2028:
            case 0x2029:
              pass = true;
              break;

            default:
              pass = false;
              break;
            }
          }
        }
        else {
          pass = (currentText == 0);
        }
        break;

      case ast::AssertionType::EndOfLine:
        if (package.multiline) {
          if (currentText == textLength) {
            pass = true;
          }
          else {
            switch (textStart[currentText]) {
            case '\r':
            case '\n':
            case 0x2028:
            case 0x2029:
              pass = true;
              break;

            default:
              pass = false;
              break;
            }
          }
        }
        else {
          pass = (currentText == textLength);
        }
        break;

      case ast::AssertionType::WordBoundary:
      case ast::AssertionType::NonWordBoundary:
        pass = (currentText > 0 && is_word_char(textStart[currentText - 1])) ^
               (currentText <= textLength && is_word_char(textStart[currentText]));
        if (currentEdge->assertion->getAssertionType() ==
            ast::AssertionType::NonWordBoundary) {
          pass = !pass;
        }
        break;

      case ast::AssertionType::LookAhead: {
          auto it = package.subNFAs.find(static_cast<const ast::LookAheadAssertionExpr *>(currentEdge->assertion));
          assert(it != package.subNFAs.end());
          CandidateVector subCandidates;
          find_all_candidates(package, it->second, input, inputStartIndex + currentText, subCandidates);
          pass = !subCandidates.empty();
          if (static_cast<const ast::LookAheadAssertionExpr *>(currentEdge->assertion)->isInverse()) {
            pass = !pass;
          }
        }
        break;

      default:
        assert(false);
        break;
      }
      break;

    case nfa::EdgeType::Epsilon:
    case nfa::EdgeType::BeginCapture:
    case nfa::EdgeType::EndCapture:
      pass = true;
      break;

    case nfa::EdgeType::Backreference:
    case nfa::EdgeType::BeginNonGreedy:
    case nfa::EdgeType::EndNonGreedy:
    default:
      assert(false);
      break;
    }

    if (!pass) {
      continue;
    }

    states.push_back(State(currentEdge->node, currentText));

    if (currentEdge->node == nfa->end) {
      candidates.push_back(Candidate(states, currentText));
    }
  }
}

Candidate find_longest_candidate(CandidateVector &candidates)
{
  assert(!candidates.empty());

  size_t longestIndex = 0;
  for (size_t i = 1; i < candidates.size(); ++i) {
    if (candidates[i].length > candidates[longestIndex].length) {
      longestIndex = i;
    }
  }

  return candidates[longestIndex];
}

void make_output(const Candidate &candidate, size_t inputStartIndex, Output &output)
{
  output.captures[0].position = inputStartIndex;
  output.captures[0].length = candidate.length;

  for (size_t i = 0; i < candidate.states.size() - 1; ++i) {
    const State &currentState = candidate.states[i];
    const State &nextState = candidate.states[i + 1];

    for (auto &edge: currentState.node->edges) {
      if (edge->node == nextState.node) {
        switch (edge->type) {
        case nfa::EdgeType::BeginCapture:
          output.captures[edge->storageIndex].position = inputStartIndex + currentState.currentText;
          break;

        case nfa::EdgeType::EndCapture:
          assert(inputStartIndex + currentState.currentText >= output.captures[edge->storageIndex].position);
          output.captures[edge->storageIndex].length = inputStartIndex + currentState.currentText - output.captures[edge->storageIndex].position;
          break;

        default:
          break;
        }
      }
    }
  }
}

} // end namespace

bool execute(const Package &package, const Input &input, size_t inputStartIndex, Output &output)
{
  CandidateVector candidates;
  find_all_candidates(package, package.nfa, input, inputStartIndex, candidates);

  if (candidates.empty()) {
    return false;
  }

  Candidate &&longestCandidate = find_longest_candidate(candidates);
  make_output(longestCandidate, inputStartIndex, output);
  return true;
}

} // end namespace exec
} // end namespace jscre
