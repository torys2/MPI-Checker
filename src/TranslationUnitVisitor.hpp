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

#ifndef MPISCHEMACHECKERAST_HPP_NKN9I06D
#define MPISCHEMACHECKERAST_HPP_NKN9I06D

#include "MPICheckerAST.hpp"

namespace mpi {

// ast dump-color legend
// Red           - CastColor
// Green         - TypeColor
// Bold Green    - DeclKindNameColor, UndeserializedColor
// Yellow        - AddressColor, LocationColor
// Blue          - CommentColor, NullColor, IndentColor
// Bold Blue     - AttrColor
// Bold Magenta  - StmtColor
// Cyan          - ValueKindColor, ObjectKindColor
// Bold Cyan     - ValueColor, DeclNameColor

/**
 * Main visitor class to collect information about MPI calls traversing
 * the AST of a translation unit, checking invariants during the traversal
 * through MPICheckerAST.
 */
class TranslationUnitVisitor
    : public clang::RecursiveASTVisitor<TranslationUnitVisitor> {
public:
    TranslationUnitVisitor(clang::ento::BugReporter &bugReporter,
                           const clang::ento::CheckerBase &checkerBase,
                           clang::ento::AnalysisManager &analysisManager)
        : checkerAST_{bugReporter, checkerBase, analysisManager} {}

    // visitor callbacks
    bool VisitFunctionDecl(clang::FunctionDecl *);
    bool VisitCallExpr(clang::CallExpr *);
    bool VisitIfStmt(clang::IfStmt *);

    MPICheckerAST checkerAST_;

private:
    bool isRankBranch(clang::IfStmt *ifStmt);

    llvm::SmallVector<clang::IfStmt *, 8> visitedIfStmts_;
};

}  // end of namespace: mpi

#endif  // end of include guard: MPISCHEMACHECKERAST_HPP_NKN9I06D
