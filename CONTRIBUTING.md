# Contributing to CAPIO‑CL

Thank you for your interest in contributing to **CAPIO‑CL**! This document outlines how you can 
help — whether you’re submitting bug fixes, feature requests, documentation improvements, or 
simply sharing ideas.

## Our Standards

- We welcome contributions from everyone — researchers, practitioners, users of the library, or 
  community members.
- Please ensure your code and documentation follow the existing style and conventions:
    - C++ code uses CMake, targets C++17 and must be formatted according to the .clang-format 
      specifications.
    - Python bindings use `pybind11`, and follow the project’s packaging and test requirements.
    - Submitted changes must be covered by unit tests.
- All contributions must be covered by the project’s license.
- By submitting a pull request, you agree that your contribution will be licensed under the project’s license.

## How to Contribute

### 1. Raise an Issue

If you’ve discovered a bug or have a feature request, first thing first, get in touch with us:

- Use the **Issues** tab to search for existing issues first.
- If none matches your concern, create a new issue with:
    - A clear and descriptive title.
    - A detailed description of the problem or feature.
    - Reproduction steps (for bugs) or a suggestion of design/impact (for features).
    - Relevant environment details (OS, compiler or Python version, etc.).

### 2. Code Contributions

To submit code to the **CAPIO-CL** repository, please for our repository, and open a pull request.
Please ensure that the following checklist is fulfilled:
- All C++ code targets C++17
- Your changes are covered by tests, and as such, ensure all existing tests, as well as your own, pass 
  before submitting changes.

### 4. Follow the Coding Style

- Use the same formatting as existing files (e.g., `.clang-format` in the root).
- All new methods **must** have doxygen documentation. Please ensure that the documentation builds
- Update examples, README, or other documentation sections when relevant.

### 5. Submit a Pull Request

- Push your branch to your fork, then open a Pull Request back into `main`.
- In your PR description reference the issue number (if applicable).
- Provide a clear summary: what you did, why you did it, and how you tested it.
- We will check your code for correctness, style, test coverage, and documentation.

---

## Review Process

- All PRs will undergo at least one review by a project maintainer or core contributor.
- You may be asked to make revisions (style tweaks, additional tests, clearer documentation).
- Once approved and CI passes, your PR will be merged and then closed.
- After merging, your contribution will appear in the next release or commit.



## Documentation, Examples & Tests

- Documentation lives in the `doxygen/` folder and within the README.
- Examples and usage snippets should be updated alongside any API changes.
- Tests (C++ or Python) should accompany new features or changed behavior — aim for coverage and clarity.



## Code of Conduct

This project follows a community code of conduct. Please be kind, respectful, and considerate in your communications and
contributions.


## Attribution

By contributing, you agree that your work will be part of the CAPIO‑CL project under its existing open source license (
see `LICENSE` in the repository root).  
Thank you for helping make CAPIO‑CL better.



## Thank You!

Even small improvements make a big difference. Whether you’re fixing a typo, writing cleaner code, or suggesting a new
capability — your contribution is valued.  
We look forward to collaborating with you!
