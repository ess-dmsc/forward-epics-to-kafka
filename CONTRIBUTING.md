# Contributing

We use `clang-format` v3.9 LLVM default style.
Due to availability of `clang-format` on the different systems that we use for
development, we decided on this lowest common denominator.

The `.clang-format` is just a `-dump-config` of the default LLVM style from
`clang-format` v3.9.


## Branching

- Create a feature branch from 'master'

- Before creating pull requests, rebase your feature branch

- Reorder and squash small successive commits which may have occurred during
  iterative development to improve readability of the feature branch.
