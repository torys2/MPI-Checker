#include "MPITypes.hpp"

namespace mpi {

llvm::SmallVector<MPICall, 16> MPICall::visitedCalls;
unsigned long MPICall::id{0};

llvm::SmallVector<MPIRequest, 4> MPIRequest::visitedRequests;

namespace MPIRank {
llvm::SmallSet<const clang::VarDecl *, 4> visitedRankVariables;
}

namespace MPIRankCases {
llvm::SmallVector<MPIrankCase, 8> visitedRankCases;
}

}  // end of namespace: mpi
