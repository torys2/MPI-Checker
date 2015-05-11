#include "MPICheckerImpl.hpp"

using namespace clang;
using namespace ento;

namespace mpi {

void MPICheckerImpl::checkForCollectiveCall(const MPICall &mpiCall) const {
    if (funcClassifier_.isCollectiveType(mpiCall.identInfo_)) {
        bugReporter_.reportCollCallInBranch(mpiCall.callExpr_);
    }
}

/**
 * Iterates rank cases looking for point to point send or receive
 * functions. If found report them as unmatched.
 *
 * @param rankCases
 */
void MPICheckerImpl::checkUnmatchedCalls(
    const llvm::SmallVectorImpl<MPIrankCase> &rankCases) const {
    for (const MPIrankCase &rankCase : rankCases) {
        for (const MPICall &b : rankCase) {
            if (funcClassifier_.isPointToPointType(b.identInfo_)) {
                if (funcClassifier_.isSendType(b.identInfo_)) {
                    bugReporter_.reportUnmatchedCall(b.callExpr_, "receive");
                }
                else if (funcClassifier_.isRecvType(b.identInfo_)) {
                    bugReporter_.reportUnmatchedCall(b.callExpr_, "send");
                }
            }
        }
    }
}

/**
 * Check if two calls are a send/recv pair.
 *
 * @param sendCall
 * @param recvCall
 *
 * @return if they are send/recv pair
 */
bool MPICheckerImpl::isSendRecvPair(const MPICall &sendCall,
                                    const MPICall &recvCall) const {
    if (!funcClassifier_.isSendType(sendCall.identInfo_)) return false;
    if (!funcClassifier_.isRecvType(recvCall.identInfo_)) return false;

    // compare mpi datatype
    llvm::StringRef sendType = util::sourceRangeAsStringRef(
        sendCall.arguments_[MPIPointToPoint::kDatatype].expr_->getSourceRange(),
        analysisManager_);

    llvm::StringRef recvType = util::sourceRangeAsStringRef(
        recvCall.arguments_[MPIPointToPoint::kDatatype].expr_->getSourceRange(),
        analysisManager_);

    if (sendType != recvType) return false;

    // compare count, tag
    for (size_t idx : {MPIPointToPoint::kCount, MPIPointToPoint::kTag}) {
        if (!areComponentsOfArgumentEqual(sendCall, recvCall, idx)) {
            return false;
        }
    }

    // compare rank
    auto rankArgSend = sendCall.arguments_[MPIPointToPoint::kRank];
    auto rankArgRecv = recvCall.arguments_[MPIPointToPoint::kRank];

    auto operatorsSend = rankArgSend.binaryOperators_;
    auto operatorsRecv = rankArgRecv.binaryOperators_;

    // // if send is single rank literal
    if (rankArgSend.intValues_.size() == 1 &&
        !rankArgSend.binaryOperators_.size()) {
        if (rankArgRecv.intValues_.size() != 1) return false;

        if (operatorsRecv.size() == 1) {
            if (rankArgRecv.intValues_.size() == 1 &&
                !(BinaryOperatorKind::BO_Sub == operatorsRecv.front()))
                return false;
        }

        // send rank must be != recv rank
        if (rankArgSend.intValues_.front() == rankArgRecv.intValues_.front())
            return false;
    }

    // if rank is dynamic and uses literal
    if (rankArgSend.vars_.size() && rankArgRecv.intValues_.size()) {
        // literals must match
        if (!util::isPermutation(rankArgSend.integerLiterals_,
                                 rankArgRecv.integerLiterals_))
            return false;
    }

    if (!util::isPermutation(rankArgSend.functions_, rankArgRecv.functions_))
        return false;

    // // if only one operator is used expect it to be inverse
    // if (operatorsSend.size() == 1 && operatorsRecv.size() == 1) {
    // if (BinaryOperatorKind::BO_Add == operatorsSend.front() &&
    // !(BinaryOperatorKind::BO_Sub == operatorsRecv.front())) {
    // return false;
    // } else if (BinaryOperatorKind::BO_Sub == operatorsSend.front() &&
    // !(BinaryOperatorKind::BO_Add == operatorsRecv.front())) {
    // return false;
    // }
    // }

    return true;
}

/**
 * Checks if buffer type and specified mpi datatype matches.
 *
 * @param mpiCall call to check type correspondence for
 */
void MPICheckerImpl::checkBufferTypeMatch(const MPICall &mpiCall) const {
    // one pair consists of {bufferIdx, mpiDatatypeIdx}
    llvm::SmallVector<std::pair<size_t, size_t>, 2> indexPairs;

    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        indexPairs.push_back(
            {MPIPointToPoint::kBuf, MPIPointToPoint::kDatatype});
    } else if (funcClassifier_.isCollectiveType(mpiCall.identInfo_)) {
        if (funcClassifier_.isReduceType(mpiCall.identInfo_)) {
            // only check buffer type if not inplace
            if (util::sourceRangeAsStringRef(
                    mpiCall.callExpr_->getArg(0)->getSourceRange(),
                    analysisManager_) != "MPI_IN_PLACE") {
                indexPairs.push_back({0, 3});
            }
            indexPairs.push_back({1, 3});
        } else if (funcClassifier_.isScatterType(mpiCall.identInfo_) ||
                   funcClassifier_.isGatherType(mpiCall.identInfo_) ||
                   funcClassifier_.isAlltoallType(mpiCall.identInfo_)) {
            indexPairs.push_back({0, 2});
            indexPairs.push_back({3, 5});
        } else if (funcClassifier_.isBcastType(mpiCall.identInfo_)) {
            indexPairs.push_back({0, 2});
        }
    }

    // for every buffer mpi-data pair in function
    // check if their types match
    for (const auto &idxPair : indexPairs) {
        const VarDecl *bufferArg =
            mpiCall.arguments_[idxPair.first].vars_.front();

        // collect buffer type information
        const mpi::TypeVisitor typeVisitor{bufferArg->getType()};

        // get mpi datatype as string
        auto mpiDatatype = mpiCall.arguments_[idxPair.second].expr_;
        StringRef mpiDatatypeString{util::sourceRangeAsStringRef(
            mpiDatatype->getSourceRange(), analysisManager_)};

        selectTypeMatcher(typeVisitor, mpiCall, mpiDatatypeString, idxPair);
    }
}

/**
 * Select apprioriate function to match the buffer type against
 * the specified mpi datatype.
 *
 * @param typeVisitor contains information about the buffer
 * @param mpiCall call whose arguments are observed
 * @param mpiDatatypeString
 * @param idxPair bufferIdx, mpiDatatypeIdx
 */
void MPICheckerImpl::selectTypeMatcher(
    const mpi::TypeVisitor &typeVisitor, const MPICall &mpiCall,
    const StringRef mpiDatatypeString,
    const std::pair<size_t, size_t> &idxPair) const {
    clang::BuiltinType *builtinTypeBuffer = typeVisitor.builtinType_;
    bool isTypeMatching{true};

    // check for exact width types (e.g. int16_t, uint32_t)
    if (typeVisitor.isTypedefType_) {
        isTypeMatching = matchExactWidthType(typeVisitor, mpiDatatypeString);
    }
    // check for complex-floating types (e.g. float _Complex)
    else if (typeVisitor.complexType_) {
        isTypeMatching = matchComplexType(typeVisitor, mpiDatatypeString);
    }
    // check for basic builtin types (e.g. int, char)
    else if (!builtinTypeBuffer)
        return;  // if no builtin type cancel checking
    else if (builtinTypeBuffer->isBooleanType()) {
        isTypeMatching = matchBoolType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isAnyCharacterType()) {
        isTypeMatching = matchCharType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isSignedInteger()) {
        isTypeMatching = matchSignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isUnsignedIntegerType()) {
        isTypeMatching = matchUnsignedType(typeVisitor, mpiDatatypeString);
    } else if (builtinTypeBuffer->isFloatingType()) {
        isTypeMatching = matchFloatType(typeVisitor, mpiDatatypeString);
    }

    if (!isTypeMatching)
        bugReporter_.reportTypeMismatch(mpiCall.callExpr_, idxPair);
}

bool MPICheckerImpl::matchBoolType(const mpi::TypeVisitor &visitor,
                                   const llvm::StringRef mpiDatatype) const {
    return (mpiDatatype == "MPI_C_BOOL");
}

bool MPICheckerImpl::matchCharType(const mpi::TypeVisitor &visitor,
                                   const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;
    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::SChar:
            isTypeMatching =
                (mpiDatatype == "MPI_CHAR" || mpiDatatype == "MPI_SIGNED_CHAR");
            break;
        case BuiltinType::Char_S:
            isTypeMatching =
                (mpiDatatype == "MPI_CHAR" || mpiDatatype == "MPI_SIGNED_CHAR");
            break;
        case BuiltinType::UChar:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;
        case BuiltinType::Char_U:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_CHAR");
            break;
        case BuiltinType::WChar_S:
            isTypeMatching = (mpiDatatype == "MPI_WCHAR");
            break;
        case BuiltinType::WChar_U:
            isTypeMatching = (mpiDatatype == "MPI_WCHAR");
            break;

        default:
            isTypeMatching = true;
    }

    return isTypeMatching;
}

bool MPICheckerImpl::matchSignedType(const mpi::TypeVisitor &visitor,
                                     const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Int:
            isTypeMatching = (mpiDatatype == "MPI_INT");
            break;
        case BuiltinType::Long:
            isTypeMatching = (mpiDatatype == "MPI_LONG");
            break;
        case BuiltinType::Short:
            isTypeMatching = (mpiDatatype == "MPI_SHORT");
            break;
        case BuiltinType::LongLong:
            isTypeMatching = (mpiDatatype == "MPI_LONG_LONG" ||
                              mpiDatatype == "MPI_LONG_LONG_INT");
            break;
        default:
            isTypeMatching = true;
    }

    return isTypeMatching;
}

bool MPICheckerImpl::matchUnsignedType(
    const mpi::TypeVisitor &visitor, const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::UInt:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED");
            break;
        case BuiltinType::UShort:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_SHORT");
            break;
        case BuiltinType::ULong:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_LONG");
            break;
        case BuiltinType::ULongLong:
            isTypeMatching = (mpiDatatype == "MPI_UNSIGNED_LONG_LONG");
            break;

        default:
            isTypeMatching = true;
    }
    return isTypeMatching;
}

bool MPICheckerImpl::matchFloatType(const mpi::TypeVisitor &visitor,
                                    const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Float:
            isTypeMatching = (mpiDatatype == "MPI_FLOAT");
            break;
        case BuiltinType::Double:
            isTypeMatching = (mpiDatatype == "MPI_DOUBLE");
            break;
        case BuiltinType::LongDouble:
            isTypeMatching = (mpiDatatype == "MPI_LONG_DOUBLE");
            break;
        default:
            isTypeMatching = true;
    }
    return isTypeMatching;
}

bool MPICheckerImpl::matchComplexType(const mpi::TypeVisitor &visitor,
                                      const llvm::StringRef mpiDatatype) const {
    bool isTypeMatching;

    switch (visitor.builtinType_->getKind()) {
        case BuiltinType::Float:
            isTypeMatching = (mpiDatatype == "MPI_C_COMPLEX" ||
                              mpiDatatype == "MPI_C_FLOAT_COMPLEX");
            break;
        case BuiltinType::Double:
            isTypeMatching = (mpiDatatype == "MPI_C_DOUBLE_COMPLEX");
            break;
        case BuiltinType::LongDouble:
            isTypeMatching = (mpiDatatype == "MPI_C_LONG_DOUBLE_COMPLEX");
            break;
        default:
            isTypeMatching = true;
    }

    return isTypeMatching;
}

bool MPICheckerImpl::matchExactWidthType(
    const mpi::TypeVisitor &visitor, const llvm::StringRef mpiDatatype) const {
    // check typedef type match
    // no break needs to be specified for string switch
    bool isTypeMatching = llvm::StringSwitch<bool>(visitor.typedefTypeName_)
                              .Case("int8_t", (mpiDatatype == "MPI_INT8_T"))
                              .Case("int16_t", (mpiDatatype == "MPI_INT16_T"))
                              .Case("int32_t", (mpiDatatype == "MPI_INT32_T"))
                              .Case("int64_t", (mpiDatatype == "MPI_INT64_T"))

                              .Case("uint8_t", (mpiDatatype == "MPI_UINT8_T"))
                              .Case("uint16_t", (mpiDatatype == "MPI_UINT16_T"))
                              .Case("uint32_t", (mpiDatatype == "MPI_UINT32_T"))
                              .Case("uint64_t", (mpiDatatype == "MPI_UINT64_T"))
                              // unknown typedefs are rated as correct
                              .Default(true);

    return isTypeMatching;
}

/**
 * Check if invalid argument types are used in a mpi call.
 * This check looks at indices where only integer values are valid.
 * (count, rank, tag) Any non integer type usage is reported.
 *
 * @param mpiCall to check the arguments for
 */
void MPICheckerImpl::checkForInvalidArgs(const MPICall &mpiCall) const {
    if (funcClassifier_.isPointToPointType(mpiCall.identInfo_)) {
        const auto indicesToCheck = {MPIPointToPoint::kCount,
                                     MPIPointToPoint::kRank,
                                     MPIPointToPoint::kTag};

        // iterate indices which should not have float arguments
        for (const size_t idx : indicesToCheck) {
            // check for invalid variable types
            const auto &arg = mpiCall.arguments_[idx];
            const auto &vars = arg.vars_;
            for (const auto &var : vars) {
                const mpi::TypeVisitor typeVisitor{var->getType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, var->getSourceRange(),
                        "Variable");
                }
            }

            // check for float literals
            if (arg.floatingLiterals_.size()) {
                bugReporter_.reportInvalidArgumentType(
                    mpiCall.callExpr_, idx,
                    arg.floatingLiterals_.front()->getSourceRange(), "Literal");
            }

            // check for invalid return types from functions
            const auto &functions = arg.functions_;
            for (const auto &function : functions) {
                const mpi::TypeVisitor typeVisitor{function->getReturnType()};
                if (!typeVisitor.builtinType_ ||
                    !typeVisitor.builtinType_->isIntegerType()) {
                    bugReporter_.reportInvalidArgumentType(
                        mpiCall.callExpr_, idx, function->getSourceRange(),
                        "Return value from function");
                }
            }
        }
    }
}

/**
 * Compares all components of an argument from two calls for equality
 * obtained by index. The components can appear in any permutation of
 * each other to be rated as equal.
 *
 * @param callOne
 * @param callTwo
 * @param idx
 *
 * @return areEqual
 */
bool MPICheckerImpl::areComponentsOfArgumentEqual(const MPICall &callOne,
                                                  const MPICall &callTwo,
                                                  const size_t idx) const {
    auto argOne = callOne.arguments_[idx];
    auto argTwo = callTwo.arguments_[idx];

    // operators
    if (!util::isPermutation(argOne.binaryOperators_, argTwo.binaryOperators_))
        return false;

    // variables
    if (!util::isPermutation(argOne.vars_, argTwo.vars_)) return false;

    // int literals
    if (!util::isPermutation(argOne.intValues_, argTwo.intValues_))
        return false;

    // float literals
    // just compare count, floats should not be compared by value
    // https://tinyurl.com/ks8smw4
    if (argOne.floatValues_.size() != argTwo.floatValues_.size()) {
        return false;
    }

    // functions
    if (!util::isPermutation(argOne.functions_, argTwo.functions_))
        return false;

    return true;
}

bool MPICheckerImpl::areDatatypesEqual(const MPICall &callOne,
                                       const MPICall &callTwo,
                                       const size_t idx) const {
    const VarDecl *mpiTypeNew = callOne.arguments_[idx].vars_.front();
    const VarDecl *mpiTypePrev = callTwo.arguments_[idx].vars_.front();

    return mpiTypeNew->getName() == mpiTypePrev->getName();
}

/**
 * Check if two calls are both point to point or collective calls.
 *
 * @param callOne
 * @param callTwo
 *
 * @return
 */
bool MPICheckerImpl::areCommunicationTypesEqual(const MPICall &callOne,
                                                const MPICall &callTwo) const {
    return ((funcClassifier_.isPointToPointType(callOne.identInfo_) &&
             funcClassifier_.isPointToPointType(callTwo.identInfo_)) ||

            (funcClassifier_.isCollectiveType(callOne.identInfo_) &&
             funcClassifier_.isCollectiveType(callTwo.identInfo_)));
}

/**
 * Check if two calls qualify for a redundancy check.
 *
 * @param callToCheck
 * @param comparedCall
 *
 * @return
 */
bool MPICheckerImpl::qualifyRedundancyCheck(const MPICall &callToCheck,
                                            const MPICall &comparedCall) const {
    if (comparedCall.isMarked_) return false;  // to omit double matching
    // do not compare with the call itself
    if (callToCheck.id_ == comparedCall.id_) return false;
    if (!areCommunicationTypesEqual(callToCheck, comparedCall)) return false;

    if (funcClassifier_.isPointToPointType(callToCheck.identInfo_)) {
        // both must be send or recv types
        return (funcClassifier_.isSendType(callToCheck.identInfo_) &&
                funcClassifier_.isSendType(comparedCall.identInfo_)) ||
               (funcClassifier_.isRecvType(callToCheck.identInfo_) &&
                funcClassifier_.isRecvType(comparedCall.identInfo_));

    } else if (funcClassifier_.isCollectiveType(callToCheck.identInfo_)) {
        // calls must be of the same type
        return (funcClassifier_.isScatterType(callToCheck.identInfo_) &&
                funcClassifier_.isScatterType(comparedCall.identInfo_)) ||

               (funcClassifier_.isGatherType(callToCheck.identInfo_) &&
                funcClassifier_.isGatherType(comparedCall.identInfo_)) ||

               (funcClassifier_.isAlltoallType(callToCheck.identInfo_) &&
                funcClassifier_.isAlltoallType(comparedCall.identInfo_)) ||

               (funcClassifier_.isBcastType(callToCheck.identInfo_) &&
                funcClassifier_.isBcastType(comparedCall.identInfo_)) ||

               (funcClassifier_.isReduceType(callToCheck.identInfo_) &&
                funcClassifier_.isReduceType(comparedCall.identInfo_));
    }
    return false;
}

/**
 * Check if there is a redundant call to the call passed.
 *
 * @param callToCheck
 */
void MPICheckerImpl::checkForRedundantCall(const MPICall &callToCheck) const {
    SmallVector<size_t, 3> indicesToCheckComponents;
    SmallVector<size_t, 2> indicesToCheckAsString;

    if (funcClassifier_.isPointToPointType(callToCheck.identInfo_)) {
        indicesToCheckComponents = {MPIPointToPoint::kCount,
                                    MPIPointToPoint::kRank,
                                    MPIPointToPoint::kTag};
        indicesToCheckAsString = {MPIPointToPoint::kDatatype};
    } else if (funcClassifier_.isReduceType(callToCheck.identInfo_)) {
        indicesToCheckComponents = {2};
        indicesToCheckAsString = {3, 4};
    } else if (funcClassifier_.isScatterType(callToCheck.identInfo_) ||
               funcClassifier_.isGatherType(callToCheck.identInfo_) ||
               funcClassifier_.isAlltoallType(callToCheck.identInfo_)) {
        indicesToCheckComponents = {1, 4, 6};
        indicesToCheckAsString = {2, 5};
    } else if (funcClassifier_.isBcastType(callToCheck.identInfo_)) {
        indicesToCheckComponents = {1, 3};
        indicesToCheckAsString = {2};
    }

    for (const MPICall &comparedCall : MPICall::visitedCalls) {
        if (!qualifyRedundancyCheck(callToCheck, comparedCall)) continue;

        // argument types which are compared by all 'components' –––––––
        bool identical = true;
        for (const size_t idx : indicesToCheckComponents) {
            if (!areComponentsOfArgumentEqual(callToCheck, comparedCall, idx)) {
                identical = false;
                break;  // end inner loop
            }
        }
        // compare specified mpi datatypes –––––––––––––––––––––––––––––
        for (const size_t idx : indicesToCheckAsString) {
            if (!areDatatypesEqual(callToCheck, comparedCall, idx)) {
                identical = false;
                break;  // end inner loop
            }
        }
        if (!identical) continue;

        // if function reaches this point all arguments have been equal
        // mark call to omit symmetric duplicate report
        callToCheck.isMarked_ = true;

        SmallVector<size_t, 5> checkedIndices;
        cont::copy(indicesToCheckComponents, checkedIndices);
        cont::copy(indicesToCheckAsString, checkedIndices);

        bugReporter_.reportRedundantCall(
            callToCheck.callExpr_, comparedCall.callExpr_, checkedIndices);

        // do not match against further calls
        // still all duplicate calls will appear in the diagnostics
        // due to transitivity of duplicates
        return;
    }
}

/**
 * Check if there are redundant mpi calls.
 *
 * @param callEvent
 * @param mpiFnCallSet set searched for identical calls
 *
 * @return is equal call in list
 */
void MPICheckerImpl::checkForRedundantCalls() const {
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        checkForRedundantCall(mpiCall);
    }

    // unmark calls
    for (const MPICall &mpiCall : MPICall::visitedCalls) {
        mpiCall.isMarked_ = false;
    }
}

void MPICheckerImpl::checkRequestUsage(const MPICall &mpiCall) const {
    if (funcClassifier_.isNonBlockingType(mpiCall.identInfo_)) {
        // last argument is always the request
        auto arg = mpiCall.arguments_[mpiCall.callExpr_->getNumArgs() - 1];
        auto requestVar = arg.vars_.front();

        const auto iterator = cont::findPred(
            MPIRequest::visitedRequests, [requestVar](const MPIRequest &r) {
                return r.requestVariable_ == requestVar;
            });

        if (iterator == MPIRequest::visitedRequests.end()) {
            MPIRequest::visitedRequests.push_back(
                {requestVar, mpiCall.callExpr_});
        } else {
            bugReporter_.reportDoubleRequestUse(mpiCall.callExpr_, requestVar,
                                                iterator->callUsingTheRequest_);
        }
    }

    if (funcClassifier_.isWaitType(mpiCall.identInfo_)) {
        llvm::SmallVector<VarDecl *, 1> requestVector;

        if (funcClassifier_.isMPI_Wait(mpiCall.identInfo_)) {
            requestVector.push_back(mpiCall.arguments_[0].vars_.front());
        } else if (funcClassifier_.isMPI_Waitall(mpiCall.identInfo_)) {
            ArrayVisitor arrayVisitor{mpiCall.arguments_[1].vars_.front()};
            arrayVisitor.vars_.resize(arrayVisitor.vars_.size() / 2);  // hack

            for (auto &requestVar : arrayVisitor.vars_) {
                requestVector.push_back(requestVar);
            }
        }

        for (VarDecl *requestVar : requestVector) {
            const auto iterator = cont::findPred(
                MPIRequest::visitedRequests, [requestVar](const MPIRequest &r) {
                    return r.requestVariable_ == requestVar;
                });

            // if not found -> endless wait
            if (iterator == MPIRequest::visitedRequests.end()) {
                bugReporter_.reportUnmatchedWait(mpiCall.callExpr_, requestVar);
            } else {
                // request var used, remove from container
                cont::erasePred(MPIRequest::visitedRequests,
                                [requestVar](const MPIRequest &r) {
                                    return r.requestVariable_ == requestVar;
                                });
            }
        }
    }
}

}  // end of namespace: mpi