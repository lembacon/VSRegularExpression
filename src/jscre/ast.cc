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

#include "ast.h"
#include <assert.h>
#include <sstream>
#include <iomanip>

namespace jscre {
namespace ast {

void RecursiveExprVisitor::traverseExpr(const ExprPtr &expr)
{
  assert(expr != nullptr);

  switch (expr->getType()) {
  case ExprType::Concatenation:
    visitConcatenationExpr(static_cast<ConcatenationExpr *>(expr.get()));
    break;

  case ExprType::Disjunction:
    visitDisjunctionExpr(static_cast<DisjunctionExpr *>(expr.get()));
    break;

  case ExprType::Empty:
    visitEmptyExpr(static_cast<EmptyExpr *>(expr.get()));
    break;

  case ExprType::CharacterClass:
    visitCharacterClassExpr(static_cast<CharacterClassExpr *>(expr.get()));
    break;

  case ExprType::Assertion:
    switch (static_cast<AssertionExpr *>(expr.get())->getAssertionType()) {
    case AssertionType::BeginOfLine:
    case AssertionType::EndOfLine:
    case AssertionType::WordBoundary:
    case AssertionType::NonWordBoundary:
      visitAssertionExpr(static_cast<AssertionExpr *>(expr.get()));
      break;

    case AssertionType::LookAhead:
      visitLookAheadAssertionExpr(static_cast<LookAheadAssertionExpr *>(expr.get()));
      break;

    default:
      assert(false);
      break;
    }
    break;

  case ExprType::Quantification:
    visitQuantificationExpr(static_cast<QuantificationExpr *>(expr.get()));
    break;

  case ExprType::Group:
    visitGroupExpr(static_cast<GroupExpr *>(expr.get()));
    break;

  case ExprType::Backreference:
    visitBackreferenceExpr(static_cast<BackreferenceExpr *>(expr.get()));
    break;

  default:
    assert(false);
    break;
  }
}

void RecursiveExprVisitor::traverseConcatenationExpr(ConcatenationExpr *expr)
{
  for (auto &subExpr: expr->getSubExprs()) {
    traverseExpr(subExpr);
  }
}

void RecursiveExprVisitor::traverseDisjunctionExpr(DisjunctionExpr *expr)
{
  for (auto &subExpr: expr->getSubExprs()) {
    traverseExpr(subExpr);
  }
}

void RecursiveExprVisitor::traverseLookAheadAssertionExpr(LookAheadAssertionExpr *expr)
{
  traverseExpr(expr->getSubExpr());
}

void RecursiveExprVisitor::traverseQuantificationExpr(QuantificationExpr *expr)
{
  traverseExpr(expr->getSubExpr());
}

void RecursiveExprVisitor::traverseGroupExpr(GroupExpr *expr)
{
  traverseExpr(expr->getSubExpr());
}

namespace {

class ToStringRecursiveExprVisitor : public RecursiveExprVisitor {
public:
  explicit ToStringRecursiveExprVisitor(size_t indentLevel = 2)
    : _indentLevel(indentLevel),
      _currentIndent(0) {}

  std::string str() const { return _os.str(); }

  std::ostream &os() { return _os << std::string(_currentIndent, ' '); }
  void increaseIndent() { _currentIndent += _indentLevel; }
  void decreaseIndent() { _currentIndent -= _indentLevel; }

  virtual void visitConcatenationExpr(ConcatenationExpr *expr);
  virtual void visitDisjunctionExpr(DisjunctionExpr *expr);
  virtual void visitEmptyExpr(EmptyExpr *expr);
  virtual void visitCharacterClassExpr(CharacterClassExpr *expr);
  virtual void visitAssertionExpr(AssertionExpr *expr);
  virtual void visitLookAheadAssertionExpr(LookAheadAssertionExpr *expr);
  virtual void visitQuantificationExpr(QuantificationExpr *expr);
  virtual void visitGroupExpr(GroupExpr *expr);
  virtual void visitBackreferenceExpr(BackreferenceExpr *expr);

private:
  size_t _indentLevel;
  size_t _currentIndent;
  std::ostringstream _os;
};

void ToStringRecursiveExprVisitor::visitConcatenationExpr(ConcatenationExpr *expr)
{
  os() << "Concatenation {\n";
  increaseIndent();
  traverseConcatenationExpr(expr);
  decreaseIndent();
  os() << "}\n";
}

void ToStringRecursiveExprVisitor::visitDisjunctionExpr(DisjunctionExpr *expr)
{
  os() << "Disjunction {\n";
  increaseIndent();
  traverseDisjunctionExpr(expr);
  decreaseIndent();
  os() << "}\n";
}

void ToStringRecursiveExprVisitor::visitEmptyExpr(EmptyExpr *expr)
{
  os() << "Empty\n";
}

void ToStringRecursiveExprVisitor::visitCharacterClassExpr(CharacterClassExpr *expr)
{
  std::ostringstream o;
  for (auto &range: expr->getRanges()) {
    o << "\\u" << std::setw(4) << std::setfill('0') << std::hex << range.first;
    if (range.first != range.second) {
      o << "-\\u" << std::setw(4) << std::setfill('0') << std::hex << range.second;
    }
  }

  os() << "Character Class [" << (expr->isInverse() ? "^" : "") << o.str() << "]\n";
}

void ToStringRecursiveExprVisitor::visitAssertionExpr(AssertionExpr *expr)
{
  switch (expr->getAssertionType()) {
  case AssertionType::BeginOfLine:
      os() << "Assertion: Begin of Line\n";
    break;

  case AssertionType::EndOfLine:
      os() << "Assertion: End of Line\n";
    break;

  case AssertionType::WordBoundary:
      os() << "Assertion: Word Boundary\n";
    break;

  case AssertionType::NonWordBoundary:
      os() << "Assertion: Non-Word Boundary\n";
    break;

  default:
    assert(false);
    break;
  }
}

void ToStringRecursiveExprVisitor::visitLookAheadAssertionExpr(LookAheadAssertionExpr *expr)
{
  os() << "Assertion: "
       << (expr->isInverse() ? "Inverse Look Ahead" : "Look Ahead")
       << " {\n";
  increaseIndent();
  traverseLookAheadAssertionExpr(expr);
  decreaseIndent();
  os() << "}\n";
}

void ToStringRecursiveExprVisitor::visitQuantificationExpr(QuantificationExpr *expr)
{
  os() << "Quantification: "
       << "[" << expr->getMinimum()
       << ", " << (expr->getMaximum() == QuantificationExpr::Infinite ?
                   "Inf)" : (std::to_string(expr->getMaximum())) + "]")
       << " (" << (expr->isGreedy() ? "Greedy" : "Non-Greedy") << ")"
       << " {\n";
  increaseIndent();
  traverseQuantificationExpr(expr);
  decreaseIndent();
  os() << "}\n";
}

void ToStringRecursiveExprVisitor::visitGroupExpr(GroupExpr *expr)
{
  os() << "Group: "
       << (expr->shouldCapture() ? "Captured" : "Non-Captured")
       << (expr->shouldCapture() ? (" #" + std::to_string(expr->getStorageIndex())) : "")
       << " {\n";
  increaseIndent();
  traverseGroupExpr(expr);
  decreaseIndent();
  os() << "}\n";
}

void ToStringRecursiveExprVisitor::visitBackreferenceExpr(BackreferenceExpr *expr)
{
  os() << "Backreference #" << expr->getIndex() << "\n";
}

} // end namespace

std::string to_string(const ExprPtr &expr)
{
  ToStringRecursiveExprVisitor visitor;
  visitor.traverseExpr(expr);
  return visitor.str();
}

} // end namespace ast
} // end namespace jscre
