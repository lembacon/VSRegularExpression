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

#ifndef __jscre_ast_h__
#define __jscre_ast_h__

#include <memory>
#include <vector>
#include <string>
#include <utility>

namespace jscre {
namespace ast {

enum class ExprType {
  Concatenation,
  Disjunction,
  Empty,
  CharacterClass,
  Assertion,
  Quantification,
  Group,
  Backreference
};

class Expr {
public:
  Expr() {}
  virtual ~Expr() {}

  Expr(const Expr &) = delete;
  Expr &operator=(const Expr &) = delete;

  virtual ExprType getType() const = 0;
};

typedef std::shared_ptr<Expr> ExprPtr;
typedef std::vector<ExprPtr> ExprVector;

class ConcatenationExpr : public Expr {
public:
  ConcatenationExpr() {}
  explicit ConcatenationExpr(const ExprVector &subExprs)
    : _subExprs(subExprs) {}

  const ExprVector &getSubExprs() const { return _subExprs; }
  void appendExpr(const ExprPtr &expr) { _subExprs.push_back(expr); }

  virtual ExprType getType() const { return ExprType::Concatenation; }

private:
  ExprVector _subExprs;
};

class DisjunctionExpr : public Expr {
public:
  DisjunctionExpr() {}
  explicit DisjunctionExpr(const ExprVector &subExprs)
    : _subExprs(subExprs) {}

  const ExprVector &getSubExprs() const { return _subExprs; }
  void appendExpr(const ExprPtr &expr) { _subExprs.push_back(expr); }

  virtual ExprType getType() const { return ExprType::Disjunction; }

private:
  ExprVector _subExprs;
};

class EmptyExpr : public Expr {
public:
  virtual ExprType getType() const { return ExprType::Empty; }
};

typedef std::pair<uint16_t, uint16_t> CharacterRange;
typedef std::vector<CharacterRange> CharacterRangeVector;

class CharacterClassExpr : public Expr {
public:
  explicit CharacterClassExpr(uint16_t character,
                              bool inverse = false)
    : _ranges({std::make_pair(character, character)}),
      _inverse(inverse) {}

  CharacterClassExpr(uint16_t begin,
                     uint16_t end,
                     bool inverse = false)
    : _ranges({std::make_pair(begin, end)}),
      _inverse(inverse) {}

  explicit CharacterClassExpr(const CharacterRangeVector &ranges,
                              bool inverse = false)
    : _ranges(ranges),
      _inverse(inverse) {}

  const CharacterRangeVector &getRanges() const { return _ranges; }
  bool isInverse() const { return _inverse; }

  virtual ExprType getType() const { return ExprType::CharacterClass; }

private:
  CharacterRangeVector _ranges;
  bool _inverse;
};

class UnspecifiedCharacterExpr : public CharacterClassExpr {
public:
  UnspecifiedCharacterExpr()
    : CharacterClassExpr({std::make_pair('\r', '\r'),
                          std::make_pair('\n', '\n'),
                          std::make_pair(0x2028, 0x2028),
                          std::make_pair(0x2029, 0x2029)},
                         true) {}
};

enum class AssertionType {
  BeginOfLine,
  EndOfLine,
  WordBoundary,
  NonWordBoundary,
  LookAhead
};

class AssertionExpr : public Expr {
public:
  explicit AssertionExpr(AssertionType assertionType)
    : _assertionType(assertionType) {}

  AssertionType getAssertionType() const { return _assertionType; }
  virtual ExprType getType() const { return ExprType::Assertion; }

private:
  AssertionType _assertionType;
};

class LookAheadAssertionExpr : public AssertionExpr {
public:
  explicit LookAheadAssertionExpr(const ExprPtr &subExpr,
                                  bool inverse = false)
    : AssertionExpr(AssertionType::LookAhead),
      _subExpr(subExpr),
      _inverse(inverse) {}

  const ExprPtr &getSubExpr() const { return _subExpr; }
  bool isInverse() const { return _inverse; }

private:
  ExprPtr _subExpr;
  bool _inverse;
};

class QuantificationExpr : public Expr {
public:
  QuantificationExpr(const ExprPtr &subExpr,
                     size_t minimum,
                     size_t maximum,
                     bool greedy = true)
    : _subExpr(subExpr),
      _minimum(minimum),
      _maximum(maximum),
      _greedy(greedy) {}

  const ExprPtr &getSubExpr() const { return _subExpr; }
  size_t getMinimum() const { return _minimum; }
  size_t getMaximum() const { return _maximum; }
  bool isGreedy() const { return _greedy; }

  virtual ExprType getType() const { return ExprType::Quantification; }

  static constexpr size_t Infinite = SIZE_MAX;

private:
  ExprPtr _subExpr;
  size_t _minimum;
  size_t _maximum;
  bool _greedy;
};

class GroupExpr : public Expr {
public:
  GroupExpr(const ExprPtr &subExpr,
            size_t storageIndex,
            bool shouldCapture = true)
    : _subExpr(subExpr),
      _storageIndex(storageIndex),
      _shouldCapture(shouldCapture) {}

  const ExprPtr &getSubExpr() const { return _subExpr; }
  size_t getStorageIndex() const { return _storageIndex; }
  bool shouldCapture() const { return _shouldCapture; }

  virtual ExprType getType() const { return ExprType::Group; }

  static constexpr size_t InvalidStorageIndex = SIZE_MAX;

private:
  ExprPtr _subExpr;
  size_t _storageIndex;
  bool _shouldCapture;
};

class BackreferenceExpr : public Expr {
public:
  explicit BackreferenceExpr(size_t index)
    : _index(index) {}

  size_t getIndex() const { return _index; }
  virtual ExprType getType() const { return ExprType::Backreference; }

private:
  size_t _index;
};

class RecursiveExprVisitor {
public:
  void traverseExpr(const ExprPtr &expr);

  void traverseConcatenationExpr(ConcatenationExpr *expr);
  void traverseDisjunctionExpr(DisjunctionExpr *expr);
  void traverseLookAheadAssertionExpr(LookAheadAssertionExpr *expr);
  void traverseQuantificationExpr(QuantificationExpr *expr);
  void traverseGroupExpr(GroupExpr *expr);

  virtual void visitConcatenationExpr(ConcatenationExpr *expr) {}
  virtual void visitDisjunctionExpr(DisjunctionExpr *expr) {}
  virtual void visitEmptyExpr(EmptyExpr *expr) {}
  virtual void visitCharacterClassExpr(CharacterClassExpr *expr) {}
  virtual void visitAssertionExpr(AssertionExpr *expr) {}
  virtual void visitLookAheadAssertionExpr(LookAheadAssertionExpr *expr) {}
  virtual void visitQuantificationExpr(QuantificationExpr *expr) {}
  virtual void visitGroupExpr(GroupExpr *expr) {}
  virtual void visitBackreferenceExpr(BackreferenceExpr *expr) {}
};

std::string to_string(const ExprPtr &expr);

} // end namespace ast
} // end namespace jscre

#endif /* __jscre_ast_h__ */
