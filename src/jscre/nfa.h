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

#ifndef __jscre_nfa_h__
#define __jscre_nfa_h__

#include "ast.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace jscre {
namespace nfa {

struct Node;
typedef std::shared_ptr<Node> NodePtr;
typedef std::vector<NodePtr> NodeVector;

struct Edge;
typedef std::shared_ptr<Edge> EdgePtr;
typedef std::vector<EdgePtr> EdgeVector;

struct NFA;
typedef std::shared_ptr<NFA> NFAPtr;
typedef std::map<const ast::LookAheadAssertionExpr *, NFAPtr> LookAheadNFAMap;

struct Node {
  EdgeVector edges;
};

enum class EdgeType {
  Epsilon,
  CharacterSet,
  Assertion,
  Backreference,
  BeginCapture,
  EndCapture,
  BeginNonGreedy,
  EndNonGreedy
};

struct Edge {
  EdgeType type;
  NodePtr node;

  union {
    const ast::CharacterClassExpr *expr;
    const ast::AssertionExpr *assertion;
    size_t storageIndex;
  };
};

struct NFA {
  NodePtr start;
  NodePtr end;
};

NFAPtr construct_nfa(const ast::ExprPtr &expr, LookAheadNFAMap &subNFAs);
std::string to_string(const NFAPtr &nfa, const LookAheadNFAMap &subNFAs);

} // end namespace nfa
} // end namespace jscre

#endif /* __jscre_nfa_h__ */
