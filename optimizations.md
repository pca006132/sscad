# Potential future optimizations

1. Add integer type. Integer runs much faster than doubles in modern CPUs,
   generally the reciprocal throughput for the x86-64 instruction is 4 times higher.
   This translates to around 10-20% improvement for some loop benchmarks.
2. Make some of the commonly used unary/binary functions their own instructions.
3. Use raw pointers instead of vector + index.
4. Remove some of the debug checks. We can do a validity check for the bytecode
   to make sure that it will not go wrong if our evaluator is correct.
   This can reduce the overhead of certain operations.
5. Profile-guided optimization. This can provide 10% performance improvement in
   some cases.
6. Support numerical vectors and matrix in addition to generic heterogeneous
   lists.

# Things that do not work

1. Placing goto at the end of each match. Maybe this will work for older CPUs,
   but for modern CPUs the branch predictor is capable of prediction with
   historical information, so no need to duplicate the code and add gotos.

# Things to note

1. Profile on multiple CPUs, at least on intel 12+ gen hybrid CPUs. The big and
   little cores have very different reaction to various optimizations.

