//===- ToyCombine.cpp - Toy High Level Optimizer --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a set of simple combiners for optimizing operations in
// the Toy dialect.
//
//===----------------------------------------------------------------------===//

#include <iostream>

#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"
#include "toy/Dialect.h"

using namespace mlir;
using namespace toy;

/// This is an example of a c++ rewrite pattern for the TransposeOp. It
/// optimizes the following scenario: transpose(transpose(x)) -> x
struct SimplifyRedundantTranspose : public mlir::OpRewritePattern<TransposeOp> {
  /// We register this pattern to match every toy.transpose in the IR.
  /// The "benefit" is used by the framework to order the patterns and process
  /// them in order of profitability.
  SimplifyRedundantTranspose(mlir::MLIRContext *context)
      : OpRewritePattern<TransposeOp>(context, /*benefit=*/1) {}

  /// This method attempts to match a pattern and rewrite it. The rewriter
  /// argument is the orchestrator of the sequence of rewrites. The pattern is
  /// expected to interact with it to perform any changes to the IR from here.
  mlir::LogicalResult
  matchAndRewrite(TransposeOp op,
                  mlir::PatternRewriter &rewriter) const override {
    // Look through the input of the current transpose.
    mlir::Value transposeInput = op.getOperand();
    TransposeOp transposeInputOp = transposeInput.getDefiningOp<TransposeOp>();

    // Input defined by another transpose? If not, no match.
    if (!transposeInputOp)
      return failure();

    // Otherwise, we have a redundant transpose. Use the rewriter.
    rewriter.replaceOp(op, {transposeInputOp.getOperand()});
    return success();
  }
};

// Register our patterns for rewrite by the Canonicalization framework.
void TransposeOp::getCanonicalizationPatterns(
    RewritePatternSet &results, MLIRContext *context) {
  results.add<SimplifyRedundantTranspose>(context);
}

/// This is an example of a c++ rewrite pattern for the ReshapeOp. It
/// optimizes the following scenario: reshape(reshape(x)) -> x
struct ReshapeReshapeOptPattern : public mlir::OpRewritePattern<ReshapeOp> {
  /// We register this pattern to match every toy.reshape in the IR.
  /// The "benefit" is used by the framework to order the patterns and process
  /// them in order of profitability.
  ReshapeReshapeOptPattern(mlir::MLIRContext *context)
      : OpRewritePattern<ReshapeOp>(context, /*benefit=*/1) {}

  /// This method attempts to match a pattern and rewrite it. The rewriter
  /// argument is the orchestrator of the sequence of rewrites. The pattern is
  /// expected to interact with it to perform any changes to the IR from here.
  mlir::LogicalResult
  matchAndRewrite(ReshapeOp op,
                  mlir::PatternRewriter &rewriter) const override {
    // Look through the input of the current reshape.
    mlir::Value reshapeInput = op.getOperand();
    ReshapeOp reshapeInputOp = reshapeInput.getDefiningOp<ReshapeOp>();

    // Input defined by another reshape? If not, no match.
    if (!reshapeInputOp) {
      return failure();
    }

    // Otherwise, we have a redundant reshape. Use the rewriter.
    rewriter.replaceOpWithNewOp<ReshapeOp>(op, op.getResult().getType(), reshapeInputOp.getOperand());
    return success();
  }
};

// Register our patterns for rewrite by the Canonicalization framework.
void ReshapeOp::getCanonicalizationPatterns(
    RewritePatternSet &results, MLIRContext *context) {
  results.add<ReshapeReshapeOptPattern>(context);
}