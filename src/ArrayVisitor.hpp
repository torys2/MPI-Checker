/*
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef ARRAYVISITOR_HPP_LNSPXQ6N
#define ARRAYVISITOR_HPP_LNSPXQ6N

#include "clang/AST/RecursiveASTVisitor.h"

namespace mpi {

/**
 * Visitor class to collect variables from an array.
 */
class ArrayVisitor : public clang::RecursiveASTVisitor<ArrayVisitor> {
public:
    ArrayVisitor(clang::VarDecl *varDecl) : arrayVarDecl_{varDecl} {
        TraverseVarDecl(arrayVarDecl_);
    }
    // must be public to trigger callbacks
    bool VisitDeclRefExpr(clang::DeclRefExpr *declRef) {
        if (clang::VarDecl *var =
                clang::dyn_cast<clang::VarDecl>(declRef->getDecl())) {
            vars_.push_back(var);
        }
        return true;
    }

    const clang::VarDecl *arrayVarDecl() { return arrayVarDecl_; }
    const llvm::SmallVectorImpl<clang::VarDecl *> &vars() { return vars_; }

private:
    // complete VarDecl
    clang::VarDecl *arrayVarDecl_;
    // array variables
    llvm::SmallVector<clang::VarDecl *, 4> vars_;
};

}  // end of namespace: mpi
#endif  // end of include guard: ARRAYVISITOR_HPP_LNSPXQ6N
