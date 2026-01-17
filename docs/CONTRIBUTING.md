# Contributing to smf-spf

Thank you for your interest in contributing to smf-spf! This document provides guidelines and instructions for contributing.

## Code of Conduct

This project adheres to a [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check the [issue tracker](https://github.com/jcbf/smf-spf/issues) to avoid duplicates.

When creating a bug report, include:

- **Clear title and description**
- **Steps to reproduce** the issue
- **Expected behavior** vs actual behavior
- **Environment details**:
  - OS and version
  - smf-spf version
  - MTA (Sendmail/Postfix) version
  - libSPF2 version
  - libmilter version
- **Configuration file** (sanitized)
- **Relevant log excerpts**

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:

- Use a clear and descriptive title
- Provide a detailed description of the proposed functionality
- Explain why this enhancement would be useful
- List any similar features in other milters

### Pull Requests

1. **Fork the repository** and create your branch from `master`
2. **Follow the coding standards** (see below)
3. **Add tests** for new functionality
4. **Update documentation** as needed
5. **Ensure tests pass**: `make test`
6. **Check code coverage**: `make showcov`
7. **Submit a pull request**

## Development Setup

### Prerequisites

```bash
# Debian/Ubuntu
sudo apt-get install libmilter-dev libspf2-dev libspf2-2 opendkim-tools lcov

# Fedora/RHEL
sudo dnf install libmilter-devel libspf2-devel opendkim-tools lcov

# FreeBSD
pkg install libspf2 libmilter
```

### Building

```bash
# Clean build
make clean
make

# Build with coverage instrumentation
make coverage

# Run tests
make test

# Generate coverage report
make showcov
```

### Testing

Tests use `miltertest` (from opendkim-tools) with Lua test scripts in `tests/`.

#### Running Specific Tests

```bash
# Run all tests
make test

# Run specific test
miltertest -c tests/conf/smf-spf-tests.conf -s tests/04-fulltest.lua
```

#### Adding New Tests

1. Create a Lua test script in `tests/`
2. Create corresponding config in `tests/conf/` if needed
3. Follow existing test patterns:

```lua
-- tests/04-new-feature.lua
conn = mt.connect("127.0.0.1", "sender.example.com", "192.0.2.1")
if conn == nil then
    error("mt.connect() failed")
end

mt.helo("sender.example.com")
mt.mailfrom("<user@sender.example.com>")
mt.rcptto("<recipient@example.com>")

mt.eom()
mt.disconnect()
```

## Coding Standards

### C Code Style

- **Indentation**: 4 spaces (tabs in actual code)
- **Line length**: Max 100 characters
- **Naming conventions**:
  - Functions: `snake_case` (e.g., `cache_get`, `smf_envfrom`)
  - Structs: `snake_case` (e.g., `struct context`, `cache_item`)
  - Macros: `UPPER_CASE` (e.g., `SAFE_FREE`, `MAXLINE`)
  - Global variables: `snake_case` with descriptive names

### Code Organization

- Keep functions focused and under 100 lines when possible
- Use meaningful variable names
- Comment complex algorithms
- Use `SAFE_FREE` macro for memory deallocation
- Check return values and handle errors

### Example

```c
static int example_function(const char *input) {
    char *buffer = NULL;
    int result = 0;

    if (!input) {
        log_message(LOG_ERR, "[ERROR] invalid input");
        return -1;
    }

    if (!(buffer = calloc(1, MAXLINE))) {
        log_message(LOG_ERR, "[ERROR] memory allocation failed");
        return -1;
    }

    // Process input
    strscpy(buffer, input, MAXLINE);
    result = process_buffer(buffer);

    SAFE_FREE(buffer);
    return result;
}
```

## Commit Messages

Follow these guidelines:

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit the first line to 72 characters
- Reference issues and pull requests after the first line

Example:

```
Add support for SPF macro expansion

- Implement macro parsing in SPF evaluation
- Add tests for macro expansion
- Update documentation

Fixes #123
```

## Documentation

- Update README.md for user-facing changes
- Update DOCKER.md for Docker-related changes
- Add inline comments for complex code
- Update man page if applicable
- Update smf-spf.conf comments for new options

## Testing Checklist

Before submitting a PR:

- [ ] Code compiles without warnings
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Code coverage doesn't decrease significantly
- [ ] Memory leaks checked (use valgrind if available)
- [ ] Configuration changes documented
- [ ] Docker image builds successfully (if Dockerfile changed)

## Release Process

Maintainers follow this process for releases:

1. Update version in `smf-spf.c` (`#define VERSION`)
2. Update ChangeLog
3. Update README.md with release notes
4. Create git tag: `git tag -a v2.x.x -m "Release v2.x.x"`
5. Push tag: `git push origin v2.x.x`
6. GitHub Actions builds and pushes Docker image
7. Create GitHub release with changelog

## Questions?

- Open an [issue](https://github.com/jcbf/smf-spf/issues)
- Check existing [discussions](https://github.com/jcbf/smf-spf/discussions)

## License

By contributing, you agree that your contributions will be licensed under the same GPL-2.0 license that covers the project.
