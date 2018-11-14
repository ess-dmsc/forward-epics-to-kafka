# Contributing

We use `clang-format` v3.9 LLVM default style.
Due to availability of `clang-format` on the different systems that we use for
development, we decided on this lowest common denominator.

The `.clang-format` is just a `-dump-config` of the default LLVM style from
`clang-format` v3.9.


## Branching

- Branch your feature off from 'master'

- Before creating pull requests, rebase your feature branch
  - Reorder and squash small successive commits which may have occurred
    during iterative development to improve readability of the feature
    branch.

- Create pull requests against 'master'.

## Naming conventions

- Branch: `issuenumber_issue_decription`, e.g. `314_throw_error_if_branch_name_invalid`

- Commits: `re #issuenumber commit message`, e.g. `re #42 remove dead code`

Please avoid special characters like `'` and `` ` `` as this may prevent Jenkins from successful building.