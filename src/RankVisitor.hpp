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

#ifndef RANKVISITOR_HPP_WZL2H4SR
#define RANKVISITOR_HPP_WZL2H4SR

#include "MPIFunctionClassifier.hpp"

namespace mpi {

/**
 * Visitor class to collect rank variables.
 */
class RankVisitor : public clang::RecursiveASTVisitor<RankVisitor> {
public:
    RankVisitor(clang::ento::AnalysisManager &analysisManager)
        : funcClassifier_{analysisManager} {}

    // collect rank vars
    bool VisitCallExpr(clang::CallExpr *callExpr) {
        if (funcClassifier_.isMPIType(
                callExpr->getDirectCallee()->getIdentifier())) {
            MPICall mpiCall{callExpr};
            if (funcClassifier_.isMPI_Comm_rank(mpiCall)) {
                clang::VarDecl *varDecl = mpiCall.arguments()[1].vars()[0];
                MPIRank::visitedRankVariables.insert(varDecl);
            }
        }

        return true;
    }

private:
    MPIFunctionClassifier funcClassifier_;
};

}  // end of namespace: mpi
#endif  // end of include guard: RANKVISITOR_HPP_WZL2H4SR
