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

#include "nfa.h"
#include <assert.h>
#include <set>
#include <sstream>
#include <iterator>
#include <type_traits>

namespace jscre {
namespace nfa {

namespace {

class ConstructNFARecursiveExprVisitor : public ast::RecursiveExprVisitor {
public:
  virtual void visitConcatenationExpr(ast::ConcatenationExpr *expr);
  virtual void visitDisjunctionExpr(ast::DisjunctionExpr *expr);
  virtual void visitEmptyExpr(ast::EmptyExpr *expr);
  virtual void visitCharacterClassExpr(ast::CharacterClassExpr *expr);
  virtual void visitAssertionExpr(ast::AssertionExpr *expr);
  virtual void visitLookAheadAssertionExpr(ast::LookAheadAssertionExpr *expr);
  virtual void visitQuantificationExpr(ast::QuantificationExpr *expr);
  virtual void visitGroupExpr(ast::GroupExpr *expr);
  virtual void visitBackreferenceExpr(ast::BackreferenceExpr *expr);

  NFAPtr &getMainNFA() { assert(_nfaStack.size() == 1); return _nfaStack.front(); }
  LookAheadNFAMap &getSubNFAs() { return _subNFAs; }

private:
  std::vector<NFAPtr> _nfaStack;
  LookAheadNFAMap _subNFAs;
};

void ConstructNFARecursiveExprVisitor::visitConcatenationExpr(ast::ConcatenationExpr *expr)
{
  size_t current = _nfaStack.size();
  traverseConcatenationExpr(expr);
  assert(_nfaStack.size() > current);

  NFAPtr nfa = std::make_shared<NFA>();

  for (size_t i = current; i < _nfaStack.size(); ++i) {
    NFAPtr &subNFA = _nfaStack[i];
    if (nfa->start == nullptr) {
      assert(nfa->end == nullptr);
      nfa->start = subNFA->start;
      nfa->end = subNFA->end;
    }
    else {
      assert(nfa->end != nullptr);
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = subNFA->start;
      nfa->end->edges.push_back(edge);
      nfa->end = subNFA->end;
    }
  }

  _nfaStack.erase(_nfaStack.begin() + current, _nfaStack.end());
  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitDisjunctionExpr(ast::DisjunctionExpr *expr)
{
  size_t current = _nfaStack.size();
  traverseDisjunctionExpr(expr);
  assert(_nfaStack.size() > current);

  NFAPtr nfa = std::make_shared<NFA>();
  nfa->start = std::make_shared<Node>();
  nfa->end = std::make_shared<Node>();

  for (size_t i = current; i < _nfaStack.size(); ++i) {
    NFAPtr &subNFA = _nfaStack[i];
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = subNFA->start;
      nfa->start->edges.push_back(edge);
    }
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = nfa->end;
      subNFA->end->edges.push_back(edge);
    }
  }

  _nfaStack.erase(_nfaStack.begin() + current, _nfaStack.end());
  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitEmptyExpr(ast::EmptyExpr *expr)
{
  NFAPtr nfa = std::make_shared<NFA>();
  nfa->start = std::make_shared<Node>();
  nfa->end = std::make_shared<Node>();

  EdgePtr edge = std::make_shared<Edge>();
  edge->type = EdgeType::Epsilon;
  edge->node = nfa->end;
  nfa->start->edges.push_back(edge);

  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitCharacterClassExpr(ast::CharacterClassExpr *expr)
{
  NFAPtr nfa = std::make_shared<NFA>();
  nfa->start = std::make_shared<Node>();
  nfa->end = std::make_shared<Node>();

  EdgePtr edge = std::make_shared<Edge>();
  edge->type = EdgeType::CharacterSet;
  edge->node = nfa->end;
  edge->expr = expr;
  nfa->start->edges.push_back(edge);

  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitAssertionExpr(ast::AssertionExpr *expr)
{
  NFAPtr nfa = std::make_shared<NFA>();
  nfa->start = std::make_shared<Node>();
  nfa->end = std::make_shared<Node>();

  EdgePtr edge = std::make_shared<Edge>();
  edge->type = EdgeType::Assertion;
  edge->node = nfa->end;
  edge->assertion = expr;
  nfa->start->edges.push_back(edge);

  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitLookAheadAssertionExpr(ast::LookAheadAssertionExpr *expr)
{
  visitAssertionExpr(expr);

  LookAheadNFAMap subSubNFAs;
  NFAPtr &&subNFA = construct_nfa(expr->getSubExpr(), subSubNFAs);
  assert(subNFA != nullptr);
  assert(_subNFAs.find(expr) == _subNFAs.end());
  _subNFAs.insert(std::make_pair(expr, subNFA));

  for (auto &subSubNFAPair: subSubNFAs) {
    assert(_subNFAs.find(subSubNFAPair.first) == _subNFAs.end());
    _subNFAs.insert(subSubNFAPair);
  }
}

void ConstructNFARecursiveExprVisitor::visitQuantificationExpr(ast::QuantificationExpr *expr)
{
  if (expr->getMinimum() == 0 &&
      expr->getMaximum() == 0) {
    NFAPtr nfa = std::make_shared<NFA>();
    nfa->start = std::make_shared<Node>();
    nfa->end = std::make_shared<Node>();

    EdgePtr edge = std::make_shared<Edge>();
    edge->type = EdgeType::Epsilon;
    edge->node = nfa->end;
    nfa->start->edges.push_back(edge);

    _nfaStack.push_back(nfa);
    return;
  }

  NFAPtr nfa = std::make_shared<NFA>();

  for (size_t i = 0; i < expr->getMinimum(); i++) {
    size_t current = _nfaStack.size();
    traverseQuantificationExpr(expr);
    assert(_nfaStack.size() == current + 1);

    NFAPtr subNFA = std::move(_nfaStack[current]);
    _nfaStack.pop_back();

    if (nfa->start == nullptr) {
      assert(nfa->end == nullptr);
      nfa->start = subNFA->start;
      nfa->end = subNFA->end;
    }
    else {
      assert(nfa->start != nullptr && nfa->end != nullptr);
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = subNFA->start;
      nfa->end->edges.push_back(edge);
      nfa->end = subNFA->end;
    }
  }

  if (expr->getMaximum() == ast::QuantificationExpr::Infinite) {
    size_t current = _nfaStack.size();
    traverseQuantificationExpr(expr);
    assert(_nfaStack.size() == current + 1);

    NFAPtr subNFA = std::move(_nfaStack[current]);
    _nfaStack.pop_back();

    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = subNFA->start;
      subNFA->end->edges.push_back(edge);
    }

    {
      NodePtr node1 = std::make_shared<Node>();
      NodePtr node2 = std::make_shared<Node>();

      {
        EdgePtr edge = std::make_shared<Edge>();
        edge->type = EdgeType::Epsilon;
        edge->node = subNFA->start;
        node1->edges.push_back(edge);
      }

      {
        EdgePtr edge = std::make_shared<Edge>();
        edge->type = EdgeType::Epsilon;
        edge->node = node2;
        subNFA->end->edges.push_back(edge);
      }

      {
        EdgePtr edge = std::make_shared<Edge>();
        edge->type = EdgeType::Epsilon;
        edge->node = node2;
        node1->edges.push_back(edge);
      }

      subNFA->start = node1;
      subNFA->end = node2;
    }

    if (nfa->start == nullptr) {
      assert(nfa->end == nullptr);
      nfa->start = subNFA->start;
      nfa->end = subNFA->end;
    }
    else {
      assert(nfa->start != nullptr && nfa->end != nullptr);
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = subNFA->start;
      nfa->end->edges.push_back(edge);
      nfa->end = subNFA->end;
    }
  }
  else {
    NodeVector subInitials;

    for (size_t i = expr->getMinimum(); i < expr->getMaximum(); i++) {
      size_t current = _nfaStack.size();
      traverseQuantificationExpr(expr);
      assert(_nfaStack.size() == current + 1);

      NFAPtr subNFA = std::move(_nfaStack[current]);
      _nfaStack.pop_back();

      subInitials.push_back(subNFA->start);

      if (nfa->start == nullptr) {
        assert(nfa->end == nullptr);
        nfa->start = subNFA->start;
        nfa->end = subNFA->end;
      }
      else {
        assert(nfa->start != nullptr && nfa->end != nullptr);
        EdgePtr edge = std::make_shared<Edge>();
        edge->type = EdgeType::Epsilon;
        edge->node = subNFA->start;
        nfa->end->edges.push_back(edge);
        nfa->end = subNFA->end;
      }
    }

    for (auto &node: subInitials) {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::Epsilon;
      edge->node = nfa->end;
      node->edges.push_back(edge);
    }
  }

  assert(nfa->start != nullptr && nfa->end != nullptr);

  if (!expr->isGreedy()) {
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::BeginNonGreedy;
      edge->node = nfa->start;

      NodePtr node = std::make_shared<Node>();
      node->edges.push_back(edge);
      nfa->start = node;
    }
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::EndNonGreedy;
      nfa->end->edges.push_back(edge);

      NodePtr node = std::make_shared<Node>();
      edge->node = node;
      nfa->end = node;
    }
  }

  _nfaStack.push_back(nfa);
}

void ConstructNFARecursiveExprVisitor::visitGroupExpr(ast::GroupExpr *expr)
{
  size_t current = _nfaStack.size();
  traverseGroupExpr(expr);
  assert(_nfaStack.size() == current + 1);

  if (expr->shouldCapture()) {
    NFAPtr &nfa = _nfaStack[current];
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::BeginCapture;
      edge->node = nfa->start;
      edge->storageIndex = expr->getStorageIndex();

      NodePtr node = std::make_shared<Node>();
      node->edges.push_back(edge);
      nfa->start = node;
    }
    {
      EdgePtr edge = std::make_shared<Edge>();
      edge->type = EdgeType::EndCapture;
      edge->storageIndex = expr->getStorageIndex();
      nfa->end->edges.push_back(edge);

      NodePtr node = std::make_shared<Node>();
      edge->node = node;
      nfa->end = node;
    }
  }
}

void ConstructNFARecursiveExprVisitor::visitBackreferenceExpr(ast::BackreferenceExpr *expr)
{
  NFAPtr nfa = std::make_shared<NFA>();
  nfa->start = std::make_shared<Node>();
  nfa->end = std::make_shared<Node>();

  EdgePtr edge = std::make_shared<Edge>();
  edge->type = EdgeType::Backreference;
  edge->node = nfa->end;
  edge->storageIndex = expr->getIndex();
  nfa->start->edges.push_back(edge);

  _nfaStack.push_back(nfa);
}

} // end namespace

NFAPtr construct_nfa(const ast::ExprPtr &expr, LookAheadNFAMap &subNFAs)
{
  ConstructNFARecursiveExprVisitor visitor;
  visitor.traverseExpr(expr);
  subNFAs = std::move(visitor.getSubNFAs());
  return std::move(visitor.getMainNFA());
}

namespace {

template <typename T>
struct empty_delete {
  constexpr empty_delete() noexcept = default;
  template <typename U> empty_delete(const empty_delete<U> &) noexcept {}
  void operator()(T *) const noexcept {}
};

template <typename T>
std::string ast_to_string(const T *ptr)
{
  typedef typename std::remove_const<T>::type type;
  return ast::to_string(std::shared_ptr<type>(const_cast<type *>(ptr), empty_delete<type>()));
}

typedef std::set<NodePtr> NodeSet;

void find_all_nodes(const NodePtr &node, NodeSet &set)
{
  set.insert(node);

  for (auto &edge: node->edges) {
    if (set.find(edge->node) == set.end()) {
      find_all_nodes(edge->node, set);
    }
  }
}

std::string to_string(const NFAPtr &nfa, const LookAheadNFAMap &subNFAs, size_t currentIndent, size_t indentLevel)
{
  NodeSet allNodes;
  find_all_nodes(nfa->start, allNodes);

  std::ostringstream os;
  os << std::string(currentIndent, ' ') << "Start: Node #" << std::distance(allNodes.begin(), allNodes.find(nfa->start)) << "\n";
  os << std::string(currentIndent, ' ') << "End: Node #" << std::distance(allNodes.begin(), allNodes.find(nfa->end)) << "\n";

  for (auto it = allNodes.begin(), end = allNodes.end(); it != end; ++it) {
    os << std::string(currentIndent, ' ') << "Node #" << std::distance(allNodes.begin(), it) << " {\n";
    currentIndent += indentLevel;
    for (auto &edge: (*it)->edges) {
      os << std::string(currentIndent, ' ') << "Edge {\n";
      currentIndent += indentLevel;
      switch (edge->type) {
      case EdgeType::Epsilon:
        os << std::string(currentIndent, ' ') << "Epsilon\n";
        break;

      case EdgeType::CharacterSet:
        os << std::string(currentIndent, ' ') << ast_to_string(edge->expr);
        break;

      case EdgeType::Assertion:
        switch (edge->assertion->getAssertionType()) {
        case ast::AssertionType::LookAhead:
          os << std::string(currentIndent, ' ')
             << (static_cast<const ast::LookAheadAssertionExpr *>(edge->assertion)->isInverse() ? "Inverse " : "")
             << "Look Ahead: Sub NFA #" << std::distance(subNFAs.begin(), subNFAs.find(static_cast<const ast::LookAheadAssertionExpr *>(edge->assertion)))
             << "\n";
          break;

        default:
          os << std::string(currentIndent, ' ') << ast_to_string(edge->assertion);
          break;
        }
        break;

      case EdgeType::Backreference:
        os << std::string(currentIndent, ' ') << "Backreference #" << edge->storageIndex << "\n";
        break;

      case EdgeType::BeginCapture:
        os << std::string(currentIndent, ' ') << "Begin Capture #" << edge->storageIndex << "\n";
        break;

      case EdgeType::EndCapture:
        os << std::string(currentIndent, ' ') << "End Capture #" << edge->storageIndex << "\n";
        break;

      case EdgeType::BeginNonGreedy:
        os << std::string(currentIndent, ' ') << "Begin Non-Greedy" << "\n";
        break;

      case EdgeType::EndNonGreedy:
        os << std::string(currentIndent, ' ') << "End Non-Greedy" << "\n";
        break;

      default:
        assert(false);
        break;
      }
      os << std::string(currentIndent, ' ') << "Transfer to Node #" << std::distance(allNodes.begin(), allNodes.find(edge->node)) << "\n";
      currentIndent -= indentLevel;
      os << std::string(currentIndent, ' ') << "}\n";
    }
    currentIndent -= indentLevel;
    os << std::string(currentIndent, ' ') << "}\n";
  }

  return os.str();
}

} // end namespace

std::string to_string(const NFAPtr &nfa, const LookAheadNFAMap &subNFAs)
{
  std::ostringstream os;

  os << "Main NFA {\n";
  os << to_string(nfa, subNFAs, 2, 2);
  os << "}\n";

  for (auto it = subNFAs.begin(), end = subNFAs.end(); it != end; ++it) {
    os << "\n";

    os << "Sub NFA #" << std::to_string(std::distance(subNFAs.begin(), it)) << " {\n";
    os << to_string(it->second, subNFAs, 2, 2);
    os << "}\n";
  }

  return os.str();
}

} // end namespace nfa
} // end namespace jscre
