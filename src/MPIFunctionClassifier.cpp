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

#include "MPIFunctionClassifier.hpp"
#include "Utility.hpp"
#include "Container.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

// classification ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
/**
 * Initializes function identifiers. Instead of using strings,
 * indentifier-pointers are initially captured
 * to recognize functions during analysis by comparison later.
 *
 * @param current ast-context used for analysis
 */
void MPIFunctionClassifier::identifierInit(
    clang::ento::AnalysisManager &analysisManager) {
    // init function identifiers
    initPointToPointIdentifiers(analysisManager);
    initCollectiveIdentifiers(analysisManager);
    initAdditionalIdentifiers(analysisManager);
}

void MPIFunctionClassifier::initPointToPointIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {
    ASTContext &context = analysisManager.getASTContext();

    // copy identifiers into the correct classification containers
    identInfo_MPI_Send_ = &context.Idents.get("MPI_Send");
    mpiSendTypes_.push_back(identInfo_MPI_Send_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Send_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Send_);
    mpiType_.push_back(identInfo_MPI_Send_);
    assert(identInfo_MPI_Send_);

    identInfo_MPI_Isend_ = &context.Idents.get("MPI_Isend");
    mpiSendTypes_.push_back(identInfo_MPI_Isend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Isend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Isend_);
    mpiType_.push_back(identInfo_MPI_Isend_);
    assert(identInfo_MPI_Isend_);

    identInfo_MPI_Ssend_ = &context.Idents.get("MPI_Ssend");
    mpiSendTypes_.push_back(identInfo_MPI_Ssend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Ssend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Ssend_);
    mpiType_.push_back(identInfo_MPI_Ssend_);
    assert(identInfo_MPI_Ssend_);

    identInfo_MPI_Issend_ = &context.Idents.get("MPI_Issend");
    mpiSendTypes_.push_back(identInfo_MPI_Issend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Issend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Issend_);
    mpiType_.push_back(identInfo_MPI_Issend_);
    assert(identInfo_MPI_Issend_);

    identInfo_MPI_Bsend_ = &context.Idents.get("MPI_Bsend");
    mpiSendTypes_.push_back(identInfo_MPI_Bsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Bsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Bsend_);
    mpiType_.push_back(identInfo_MPI_Bsend_);
    assert(identInfo_MPI_Bsend_);

    identInfo_MPI_Ibsend_ = &context.Idents.get("MPI_Ibsend");
    mpiSendTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Ibsend_);
    mpiType_.push_back(identInfo_MPI_Ibsend_);
    assert(identInfo_MPI_Ibsend_);

    identInfo_MPI_Rsend_ = &context.Idents.get("MPI_Rsend");
    mpiSendTypes_.push_back(identInfo_MPI_Rsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Rsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Rsend_);
    mpiType_.push_back(identInfo_MPI_Rsend_);
    assert(identInfo_MPI_Rsend_);

    identInfo_MPI_Irsend_ = &context.Idents.get("MPI_Irsend");
    mpiSendTypes_.push_back(identInfo_MPI_Irsend_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Irsend_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Irsend_);
    mpiType_.push_back(identInfo_MPI_Irsend_);
    assert(identInfo_MPI_Irsend_);

    identInfo_MPI_Recv_ = &context.Idents.get("MPI_Recv");
    mpiRecvTypes_.push_back(identInfo_MPI_Recv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Recv_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Recv_);
    mpiType_.push_back(identInfo_MPI_Recv_);
    assert(identInfo_MPI_Recv_);

    identInfo_MPI_Irecv_ = &context.Idents.get("MPI_Irecv");
    mpiRecvTypes_.push_back(identInfo_MPI_Irecv_);
    mpiPointToPointTypes_.push_back(identInfo_MPI_Irecv_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Irecv_);
    mpiType_.push_back(identInfo_MPI_Irecv_);
    assert(identInfo_MPI_Irecv_);
}

void MPIFunctionClassifier::initCollectiveIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {
    ASTContext &context = analysisManager.getASTContext();

    // copy identifiers into the correct classification containers
    identInfo_MPI_Scatter_ = &context.Idents.get("MPI_Scatter");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Scatter_);
    mpiPointToCollTypes_.push_back(identInfo_MPI_Scatter_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Scatter_);
    mpiType_.push_back(identInfo_MPI_Scatter_);
    assert(identInfo_MPI_Scatter_);

    identInfo_MPI_Iscatter_ = &context.Idents.get("MPI_Iscatter");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Iscatter_);
    mpiPointToCollTypes_.push_back(identInfo_MPI_Iscatter_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Iscatter_);
    mpiType_.push_back(identInfo_MPI_Iscatter_);
    assert(identInfo_MPI_Iscatter_);

    identInfo_MPI_Gather_ = &context.Idents.get("MPI_Gather");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Gather_);
    mpiCollToPointTypes_.push_back(identInfo_MPI_Gather_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Gather_);
    mpiType_.push_back(identInfo_MPI_Gather_);
    assert(identInfo_MPI_Gather_);

    identInfo_MPI_Igather_ = &context.Idents.get("MPI_Igather");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Igather_);
    mpiCollToPointTypes_.push_back(identInfo_MPI_Igather_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Gather_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Igather_);
    mpiType_.push_back(identInfo_MPI_Igather_);
    assert(identInfo_MPI_Igather_);

    identInfo_MPI_Allgather_ = &context.Idents.get("MPI_Allgather");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Allgather_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Allgather_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Allgather_);
    mpiType_.push_back(identInfo_MPI_Allgather_);
    assert(identInfo_MPI_Allgather_);

    identInfo_MPI_Iallgather_ = &context.Idents.get("MPI_Iallgather");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Iallgather_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Iallgather_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Iallgather_);
    mpiType_.push_back(identInfo_MPI_Iallgather_);
    assert(identInfo_MPI_Iallgather_);

    identInfo_MPI_Bcast_ = &context.Idents.get("MPI_Bcast");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Bcast_);
    mpiPointToCollTypes_.push_back(identInfo_MPI_Bcast_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Bcast_);
    mpiType_.push_back(identInfo_MPI_Bcast_);
    assert(identInfo_MPI_Bcast_);

    identInfo_MPI_Ibcast_ = &context.Idents.get("MPI_Ibcast");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Ibcast_);
    mpiPointToCollTypes_.push_back(identInfo_MPI_Ibcast_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Ibcast_);
    mpiType_.push_back(identInfo_MPI_Ibcast_);
    assert(identInfo_MPI_Ibcast_);

    identInfo_MPI_Reduce_ = &context.Idents.get("MPI_Reduce");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Reduce_);
    mpiCollToPointTypes_.push_back(identInfo_MPI_Reduce_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Reduce_);
    mpiType_.push_back(identInfo_MPI_Reduce_);
    assert(identInfo_MPI_Reduce_);

    identInfo_MPI_Ireduce_ = &context.Idents.get("MPI_Ireduce");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Ireduce_);
    mpiCollToPointTypes_.push_back(identInfo_MPI_Ireduce_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Ireduce_);
    mpiType_.push_back(identInfo_MPI_Ireduce_);
    assert(identInfo_MPI_Ireduce_);

    identInfo_MPI_Allreduce_ = &context.Idents.get("MPI_Allreduce");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Allreduce_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Allreduce_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Allreduce_);
    mpiType_.push_back(identInfo_MPI_Allreduce_);
    assert(identInfo_MPI_Allreduce_);

    identInfo_MPI_Iallreduce_ = &context.Idents.get("MPI_Iallreduce");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Iallreduce_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Iallreduce_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Iallreduce_);
    mpiType_.push_back(identInfo_MPI_Iallreduce_);
    assert(identInfo_MPI_Iallreduce_);

    identInfo_MPI_Alltoall_ = &context.Idents.get("MPI_Alltoall");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Alltoall_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Alltoall_);
    mpiBlockingTypes_.push_back(identInfo_MPI_Alltoall_);
    mpiType_.push_back(identInfo_MPI_Alltoall_);
    assert(identInfo_MPI_Alltoall_);

    identInfo_MPI_Ialltoall_ = &context.Idents.get("MPI_Ialltoall");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Ialltoall_);
    mpiCollToCollTypes_.push_back(identInfo_MPI_Ialltoall_);
    mpiNonBlockingTypes_.push_back(identInfo_MPI_Ialltoall_);
    mpiType_.push_back(identInfo_MPI_Ialltoall_);
    assert(identInfo_MPI_Ialltoall_);
}

void MPIFunctionClassifier::initAdditionalIdentifiers(
    clang::ento::AnalysisManager &analysisManager) {
    ASTContext &context = analysisManager.getASTContext();

    identInfo_MPI_Comm_rank_ = &context.Idents.get("MPI_Comm_rank");
    mpiType_.push_back(identInfo_MPI_Comm_rank_);
    assert(identInfo_MPI_Comm_rank_);

    identInfo_MPI_Wait_ = &context.Idents.get("MPI_Wait");
    mpiType_.push_back(identInfo_MPI_Wait_);
    assert(identInfo_MPI_Wait_);

    identInfo_MPI_Waitall_ = &context.Idents.get("MPI_Waitall");
    mpiType_.push_back(identInfo_MPI_Waitall_);
    assert(identInfo_MPI_Waitall_);

    identInfo_MPI_Barrier_ = &context.Idents.get("MPI_Barrier");
    mpiCollectiveTypes_.push_back(identInfo_MPI_Barrier_);
    mpiType_.push_back(identInfo_MPI_Barrier_);
    assert(identInfo_MPI_Barrier_);
}

// general identifiers–––––––––––––––––––––––––––––––––––––––––––––––––
bool MPIFunctionClassifier::isMPIType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiType_, identInfo);
}

bool MPIFunctionClassifier::isBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiBlockingTypes_, identInfo);
}

bool MPIFunctionClassifier::isNonBlockingType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiNonBlockingTypes_, identInfo);
}

// point to point identifiers––––––––––––––––––––––––––––––––––––––––––
bool MPIFunctionClassifier::isPointToPointType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiPointToPointTypes_, identInfo);
}

bool MPIFunctionClassifier::isSendType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiSendTypes_, identInfo);
}

bool MPIFunctionClassifier::isRecvType(const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiRecvTypes_, identInfo);
}

// collective identifiers––––––––––––––––––––––––––––––––––––––––––––––
bool MPIFunctionClassifier::isCollectiveType(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollectiveTypes_, identInfo);
}

bool MPIFunctionClassifier::isCollToColl(
    const IdentifierInfo *identInfo) const {
    return cont::isContained(mpiCollToCollTypes_, identInfo);
}

bool MPIFunctionClassifier::isScatterType(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Scatter_ ||
           identInfo == identInfo_MPI_Iscatter_;
}

bool MPIFunctionClassifier::isGatherType(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Gather_ ||
           identInfo == identInfo_MPI_Igather_ ||
           identInfo == identInfo_MPI_Allgather_ ||
           identInfo == identInfo_MPI_Iallgather_;
}

bool MPIFunctionClassifier::isAllgatherType(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Allgather_ ||
           identInfo == identInfo_MPI_Iallgather_;
}

bool MPIFunctionClassifier::isAlltoallType(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Alltoall_ ||
           identInfo == identInfo_MPI_Ialltoall_;
}

bool MPIFunctionClassifier::isBcastType(const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Bcast_ ||
           identInfo == identInfo_MPI_Ibcast_;
}

bool MPIFunctionClassifier::isReduceType(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Reduce_ ||
           identInfo == identInfo_MPI_Ireduce_ ||
           identInfo == identInfo_MPI_Allreduce_ ||
           identInfo == identInfo_MPI_Iallreduce_;
}

// additional identifiers ––––––––––––––––––––––––––––––––––––––––––––––
bool MPIFunctionClassifier::isMPI_Comm_rank(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Comm_rank_;
}

bool MPIFunctionClassifier::isMPI_Wait(const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Wait_;
}

bool MPIFunctionClassifier::isMPI_Waitall(
    const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Waitall_;
}

bool MPIFunctionClassifier::isWaitType(const IdentifierInfo *identInfo) const {
    return identInfo == identInfo_MPI_Wait_ ||
           identInfo == identInfo_MPI_Waitall_;
}

}  // end of namespace: mpi
