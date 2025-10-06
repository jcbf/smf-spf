# smf-spf Code Refactoring Plan

## Executive Summary

This document outlines a comprehensive plan to refactor the smf-spf C codebase from a monolithic 1,317-line file into a modular, maintainable, and well-tested project following modern C development best practices.

## Current State Analysis

### Architecture Issues
- **Monolithic Design**: Single 1,317-line file (smf-spf.c)
- **Mixed Concerns**: Configuration, caching, SPF logic, and milter callbacks all in one file
- **Limited Error Handling**: Basic error checks, missing comprehensive validation
- **No Header Files**: All declarations in single file
- **Memory Management**: Manual memory management with some potential leak paths
- **Testing**: Tests exist but focus on black-box milter behavior, not unit tests

### Code Quality Issues
1. **Function Length**: Some functions exceed 100 lines (e.g., `smf_envfrom`: 155 lines)
2. **Global State**: Heavy use of global variables (`conf`, `cache`, `authserv_id`)
3. **Error Propagation**: Inconsistent error handling patterns
4. **Code Duplication**: Similar patterns repeated across functions
5. **Limited Documentation**: Minimal inline comments for complex logic
6. **C89 Style**: Uses older C standards, could benefit from C11 features

### Positive Aspects to Preserve
- ✅ Working SPF implementation using libSPF2
- ✅ Efficient hash-based caching system
- ✅ Comprehensive test suite (black-box)
- ✅ Well-tested in production environments
- ✅ Good performance characteristics

## Refactoring Goals

### Primary Objectives
1. **Modularity**: Split into logical, cohesive modules
2. **Maintainability**: Easier to understand, modify, and extend
3. **Testability**: Enable unit testing of individual components
4. **Safety**: Improve memory safety and error handling
5. **Documentation**: Better code documentation and architecture clarity

### Non-Goals (Out of Scope)
- ❌ Changing external behavior or API
- ❌ Rewriting in another language
- ❌ Adding new features (separate from refactoring)
- ❌ Breaking backward compatibility

## Proposed Architecture

### Module Structure

```
smf-spf/
├── src/
│   ├── main.c              # Entry point, initialization
│   ├── config/
│   │   ├── config.c        # Configuration parsing
│   │   ├── config.h        # Configuration structures
│   │   └── defaults.h      # Default values
│   ├── cache/
│   │   ├── cache.c         # SPF result caching
│   │   ├── cache.h         # Cache interface
│   │   └── hash.c          # Hash functions
│   ├── spf/
│   │   ├── spf_eval.c      # SPF evaluation logic
│   │   ├── spf_eval.h      # SPF interface
│   │   └── spf_result.c    # Result formatting
│   ├── milter/
│   │   ├── milter_callbacks.c  # Milter callback implementations
│   │   ├── milter_callbacks.h  # Callback declarations
│   │   └── context.c           # Per-connection context
│   ├── whitelist/
│   │   ├── whitelist.c     # IP/PTR/From/To whitelisting
│   │   ├── whitelist.h     # Whitelist interface
│   │   └── cidr.c          # CIDR matching
│   ├── utils/
│   │   ├── string_utils.c  # String manipulation
│   │   ├── ip_utils.c      # IP address utilities
│   │   ├── logging.c       # Logging abstraction
│   │   └── memory.c        # Safe memory operations
│   └── common/
│       ├── types.h         # Common type definitions
│       ├── errors.h        # Error codes and handling
│       └── constants.h     # Global constants
├── include/
│   └── smf-spf/           # Public headers (if needed)
├── tests/
│   ├── unit/              # New unit tests
│   │   ├── test_cache.c
│   │   ├── test_config.c
│   │   ├── test_whitelist.c
│   │   └── test_spf_eval.c
│   └── integration/       # Existing miltertest tests
│       └── (current tests)
└── Makefile              # Updated build system
```

## Refactoring Phases

### Phase 1: Preparation (Week 1)
**Goal**: Set up infrastructure for refactoring without changing functionality

#### Tasks
1. **Create branch**: `git checkout -b refactor/modular-architecture`
2. **Create issue in Github**:
    - Use `gh issue create` adn add the necessary parameters to:
        * assing to @me
        * add labels "in progress" and "refactor" 
3. **Add header guards**: Prepare for header file creation
4. **Document current behavior**:
   - Create comprehensive test cases
   - Document all configuration options
   - Map all code paths
5. **Set up build infrastructure**:
   - Create directory structure
   - Update Makefile for multi-file builds
   - Add unit test framework (Check or Criterion)
6. **Establish baseline**:
   - Run all existing tests
   - Document current memory usage
   - Benchmark performance
7. **Commit changes**: commit all code changes and create a pull request


**Deliverables**:
- [ ] Git branch created
- [ ] Github issue created
- [ ] Directory structure in place
- [ ] Build system supports multi-file compilation
- [ ] Unit test framework integrated
- [ ] Baseline metrics documented
- [ ] Code committed

**Success Criteria**:
- All existing tests pass
- Code compiles with zero warnings
- No functional changes yet
- Git actions with no errors

---

### Phase 2: Extract Utilities (Week 2)
**Goal**: Extract and test utility functions first (lowest risk)

#### 2.1 String Utilities
```c
// src/utils/string_utils.h
void strscpy(char *dst, const char *src, size_t size);
void strtolower(char *str);
char *trim_space(char *str);
```

#### 2.2 Logging Abstraction
```c
// src/utils/logging.h
void log_init(const char *daemon_name, int facility);
void log_message(int level, const char *fmt, ...);
void log_shutdown(void);
```

#### 2.3 Memory Safety
```c
// src/utils/memory.h
#define SAFE_FREE(x) do { if (x) { free(x); x = NULL; } } while(0)
void *safe_calloc(size_t count, size_t size);
char *safe_strdup(const char *str);
```

#### 2.4 IP Utilities
```c
// src/utils/ip_utils.h
int ip_cidr_match(unsigned long ip, unsigned short mask, unsigned long check_ip);
unsigned long translate_nat_ip(unsigned long source_ip, const IPNAT *nat_list);
```

**Testing**:
- Unit tests for each utility function
- Verify existing behavior unchanged

**Deliverables**:
- [ ] `src/utils/` module created
- [ ] Unit tests with >90% coverage
- [ ] All tests pass

---

### Phase 3: Extract Configuration Module (Week 3)
**Goal**: Isolate configuration parsing and validation

#### 3.1 Configuration Structures
```c
// src/config/config.h
typedef struct {
    char *tag;
    char *quarantine_box;
    FILE *log_file;
    char *run_as_user;
    char *sendmail_socket;
    IPNAT *ipnats;
    CIDR *cidrs;
    STR *ptrs;
    STR *froms;
    STR *tos;
    // ... all config fields
} spf_config_t;
```

#### 3.2 Configuration API
```c
// src/config/config.h
int config_load(const char *filename, spf_config_t *config);
void config_free(spf_config_t *config);
int config_validate(const spf_config_t *config);
```

#### 3.3 Whitelist Parsing
Move whitelist parsing to dedicated module:
```c
// src/whitelist/whitelist.h
int whitelist_add_ip(spf_config_t *config, const char *cidr_str);
int whitelist_add_ptr(spf_config_t *config, const char *ptr_pattern);
int whitelist_add_from(spf_config_t *config, const char *from_pattern);
int whitelist_add_to(spf_config_t *config, const char *to_pattern);
```

**Testing**:
- Test valid configuration loading
- Test invalid configuration rejection
- Test whitelist parsing edge cases
- Test memory cleanup

**Deliverables**:
- [ ] `src/config/` module created
- [ ] `src/whitelist/` module created
- [ ] Unit tests with >85% coverage
- [ ] Configuration validation improved

---

### Phase 4: Extract Cache Module (Week 4)
**Goal**: Isolate caching logic with clean interface

#### 4.1 Cache Interface
```c
// src/cache/cache.h
typedef struct spf_cache spf_cache_t;

spf_cache_t *cache_create(void);
void cache_destroy(spf_cache_t *cache);
SPF_result_t cache_get(spf_cache_t *cache, const char *key);
void cache_put(spf_cache_t *cache, const char *key, unsigned long ttl, SPF_result_t status);
void cache_clear(spf_cache_t *cache);
size_t cache_size(const spf_cache_t *cache);
```

#### 4.2 Thread Safety
```c
// Add mutex to cache structure
typedef struct spf_cache {
    cache_item **buckets;
    pthread_mutex_t mutex;
    unsigned int hash_power;
} spf_cache_t;
```

#### 4.3 Hash Functions
```c
// src/cache/hash.c
unsigned long hash_code(const unsigned char *key);
```

**Testing**:
- Test cache operations (get, put, expiration)
- Test thread safety
- Test hash collision handling
- Benchmark cache performance

**Deliverables**:
- [ ] `src/cache/` module created
- [ ] Thread-safe cache implementation
- [ ] Unit tests with >90% coverage
- [ ] Performance benchmarks maintained

---

### Phase 5: Extract SPF Evaluation (Week 5)
**Goal**: Isolate SPF evaluation logic

#### 5.1 SPF Evaluation API
```c
// src/spf/spf_eval.h
typedef struct {
    const char *client_ip;
    const char *helo;
    const char *sender;
    const char *site;
} spf_query_t;

typedef struct {
    SPF_result_t result;
    char *explanation;
    bool cached;
} spf_response_t;

spf_response_t *spf_evaluate(
    const spf_query_t *query,
    spf_cache_t *cache,
    const spf_config_t *config
);
void spf_response_free(spf_response_t *response);
```

#### 5.2 Result Formatting
```c
// src/spf/spf_result.c
char *spf_format_auth_results_header(const spf_response_t *response, const char *authserv_id);
char *spf_format_received_spf_header(const spf_response_t *response, const char *site);
```

**Testing**:
- Test SPF evaluation with mock libSPF2
- Test cache integration
- Test result formatting
- Test error handling

**Deliverables**:
- [ ] `src/spf/` module created
- [ ] Clean SPF evaluation interface
- [ ] Unit tests with >80% coverage
- [ ] Integration tests updated

---

### Phase 6: Extract Milter Callbacks (Week 6)
**Goal**: Clean up milter callback implementations

#### 6.1 Context Management
```c
// src/milter/context.h
typedef struct milter_context {
    char addr[64];
    char fqdn[MAXLINE];
    char site[MAXLINE];
    char helo[MAXLINE];
    char from[MAXLINE];
    char sender[MAXLINE+12];
    char rcpt[MAXLINE];
    char recipient[MAXLINE];
    char key[MAXLINE];
    char *subject;
    STR *rcpts;
    SPF_result_t status;
} milter_context_t;

milter_context_t *context_create(void);
void context_destroy(milter_context_t *ctx);
```

#### 6.2 Callback Handlers
```c
// src/milter/milter_callbacks.h
sfsistat milter_connect(SMFICTX *ctx, char *name, _SOCK_ADDR *sa);
sfsistat milter_helo(SMFICTX *ctx, char *arg);
sfsistat milter_envfrom(SMFICTX *ctx, char **args);
sfsistat milter_envrcpt(SMFICTX *ctx, char **args);
sfsistat milter_header(SMFICTX *ctx, char *name, char *value);
sfsistat milter_eom(SMFICTX *ctx);
sfsistat milter_close(SMFICTX *ctx);
```

#### 6.3 Refactor Large Functions
Break down `smf_envfrom` (155 lines) into:
- `handle_empty_sender()`
- `handle_authenticated_sender()`
- `check_sender_whitelist()`
- `evaluate_spf_policy()`
- `process_spf_result()`

**Testing**:
- Test each callback independently
- Test context lifecycle
- Test integration with milter API
- Run full integration test suite

**Deliverables**:
- [ ] `src/milter/` module created
- [ ] Callback functions under 50 lines each
- [ ] Unit tests for callback logic
- [ ] All integration tests pass

---

### Phase 7: Improve Error Handling (Week 7)
**Goal**: Consistent error handling throughout

#### 7.1 Error Codes
```c
// src/common/errors.h
typedef enum {
    SPF_MILTER_OK = 0,
    SPF_MILTER_ERR_MEMORY,
    SPF_MILTER_ERR_CONFIG,
    SPF_MILTER_ERR_INVALID_PARAM,
    SPF_MILTER_ERR_SPF_FAILURE,
    SPF_MILTER_ERR_CACHE,
    SPF_MILTER_ERR_SYSTEM
} spf_milter_error_t;

const char *spf_milter_strerror(spf_milter_error_t err);
```

#### 7.2 Error Handling Pattern
```c
// Consistent error handling pattern
int function_that_can_fail(void **result) {
    if (!result) {
        return SPF_MILTER_ERR_INVALID_PARAM;
    }

    *result = safe_calloc(1, sizeof(some_type));
    if (!*result) {
        log_message(LOG_ERR, "Memory allocation failed");
        return SPF_MILTER_ERR_MEMORY;
    }

    return SPF_MILTER_OK;
}
```

#### 7.3 Resource Cleanup
```c
// Add cleanup helpers
void cleanup_on_error(void *resource1, void *resource2, ...);
```

**Testing**:
- Test error paths
- Test resource cleanup on failure
- Memory leak testing with valgrind

**Deliverables**:
- [ ] Error code system implemented
- [ ] Consistent error handling pattern
- [ ] No memory leaks in error paths
- [ ] Error handling tests added

---

### Phase 8: Code Quality Improvements (Week 8)
**Goal**: Improve code quality and maintainability

#### 8.1 Static Analysis
- Run and fix issues from:
  - `cppcheck`
  - `clang-tidy`
  - `scan-build`
  - `splint`

#### 8.2 Compiler Warnings
```makefile
CFLAGS += -Wall -Wextra -Werror -Wpedantic
CFLAGS += -Wformat=2 -Wno-unused-parameter
CFLAGS += -Wshadow -Wwrite-strings -Wstrict-prototypes
CFLAGS += -Wold-style-definition -Wredundant-decls
CFLAGS += -Wnested-externs -Wmissing-include-dirs
```

#### 8.3 Code Formatting
- Use `clang-format` with configuration:
```yaml
# .clang-format
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: None
```

#### 8.4 Documentation
- Add Doxygen comments to all public APIs
- Generate API documentation
- Update architecture diagrams

**Deliverables**:
- [ ] Zero compiler warnings
- [ ] Zero static analysis issues
- [ ] Code formatted consistently
- [ ] Doxygen documentation generated

---

### Phase 9: Testing & Validation (Week 9)
**Goal**: Comprehensive testing and validation

#### 9.1 Unit Test Coverage
Target coverage by module:
- Utils: >95%
- Config: >90%
- Cache: >90%
- Whitelist: >85%
- SPF: >80%
- Milter: >75%
- Overall: >85%

#### 9.2 Integration Testing
- All existing miltertest tests pass
- Add new integration tests for:
  - Error conditions
  - Edge cases
  - Performance scenarios

#### 9.3 Regression Testing
- Test against known configurations
- Verify identical behavior to original
- Performance benchmarking

#### 9.4 Memory Testing
```bash
valgrind --leak-check=full --show-leak-kinds=all ./smf-spf -f -c test.conf
```

**Deliverables**:
- [ ] Unit test coverage >85%
- [ ] All integration tests pass
- [ ] Zero memory leaks
- [ ] Performance maintained or improved

---

### Phase 10: Documentation & Release (Week 10)
**Goal**: Document changes and prepare release

#### 10.1 Documentation Updates
- Architecture overview
- Module descriptions
- API documentation
- Migration guide
- Developer guide updates

#### 10.2 Build System
- Update Makefile for all modules
- Add `make check` for tests
- Add `make coverage` for coverage reports
- Add `make docs` for documentation

#### 10.3 Release Preparation
- Update ChangeLog
- Update version to 3.0.0 (major refactoring)
- Create release notes
- Update README with architectural changes

**Deliverables**:
- [ ] Complete documentation
- [ ] Updated build system
- [ ] Release v3.0.0 prepared
- [ ] Migration guide for contributors

---

## Implementation Guidelines

### Coding Standards

#### C11 Standards
```c
// Use C11 features where appropriate
#include <stdbool.h>    // Use bool, true, false
#include <stdint.h>     // Use fixed-width integers
#include <stddef.h>     // Use size_t consistently

// Use designated initializers
struct config default_config = {
    .tag = TAG_STRING,
    .spf_ttl = SPF_TTL,
    .refuse_fail = REFUSE_FAIL,
    // ...
};
```

#### Memory Safety
```c
// Always check allocations
void *ptr = calloc(1, size);
if (!ptr) {
    log_message(LOG_ERR, "Memory allocation failed");
    return SPF_MILTER_ERR_MEMORY;
}

// Use SAFE_FREE consistently
SAFE_FREE(ptr);

// Prefer stack allocation for small buffers
char buffer[MAXLINE];  // Good for fixed-size needs
```

#### Function Design
```c
// Keep functions focused and small (<50 lines ideal)
// Use clear naming conventions
// Return error codes, use output parameters for results

// Good:
int parse_ip_address(const char *str, unsigned long *out_ip);

// Avoid:
unsigned long parse_ip_address(const char *str);  // Can't signal errors
```

#### Header Guards
```c
// Use modern include guards
#ifndef SMF_SPF_CACHE_H
#define SMF_SPF_CACHE_H

// ... declarations ...

#endif /* SMF_SPF_CACHE_H */
```

### Testing Strategy

#### Unit Tests
```c
// Use Check framework
#include <check.h>

START_TEST(test_cache_put_get)
{
    spf_cache_t *cache = cache_create();
    ck_assert_ptr_nonnull(cache);

    cache_put(cache, "key1", 3600, SPF_RESULT_PASS);
    SPF_result_t result = cache_get(cache, "key1");
    ck_assert_int_eq(result, SPF_RESULT_PASS);

    cache_destroy(cache);
}
END_TEST
```

#### Integration Tests
- Keep existing miltertest Lua scripts
- Add edge case scenarios
- Test error conditions

### Git Workflow

#### Branch Strategy
```bash
# Main refactoring branch
git checkout -b refactor/modular-architecture

# Feature branches for each phase
git checkout -b refactor/phase-1-preparation
git checkout -b refactor/phase-2-utilities
# etc.
```

#### Commit Messages
```
refactor(cache): Extract cache module from monolithic file

- Create src/cache/cache.{c,h} with clean interface
- Add thread-safe operations with mutex
- Implement unit tests with 92% coverage
- No functional changes, behavior identical

Related: #refactor-plan Phase 4
```

### Code Review Checklist

For each module:
- [ ] Compiles without warnings
- [ ] All tests pass
- [ ] No memory leaks (valgrind clean)
- [ ] Code coverage meets target
- [ ] Documentation complete
- [ ] API follows conventions
- [ ] Error handling consistent
- [ ] Thread-safe where needed

## Risk Management

### High Risk Areas

#### 1. Cache Thread Safety
**Risk**: Race conditions in multi-threaded milter environment
**Mitigation**:
- Comprehensive thread safety testing
- Use valgrind --tool=helgrind
- Add stress tests with concurrent access

#### 2. Memory Management
**Risk**: Memory leaks or use-after-free
**Mitigation**:
- Extensive valgrind testing
- Unit tests for all allocation/free paths
- Use ASAN/UBSAN during development

#### 3. SPF Evaluation Changes
**Risk**: Breaking SPF evaluation logic
**Mitigation**:
- Comprehensive test suite before refactoring
- Side-by-side testing (old vs new)
- Gradual migration with feature flags

#### 4. Performance Regression
**Risk**: Refactoring adds overhead
**Mitigation**:
- Benchmark before refactoring
- Profile after each phase
- Optimize hot paths if needed

### Rollback Plan

Each phase should:
1. Be independently testable
2. Allow rollback to previous phase
3. Maintain working state on main branch

```bash
# If phase fails, revert
git revert <phase-commits>

# Or reset to last good state
git reset --hard <last-good-commit>
```

## Success Metrics

### Code Quality Metrics
- [ ] Lines per file: <500 average
- [ ] Functions per file: <20 average
- [ ] Lines per function: <50 average
- [ ] Cyclomatic complexity: <10 per function
- [ ] Code coverage: >85% overall

### Performance Metrics
- [ ] No performance degradation (within 5%)
- [ ] Memory usage similar or better
- [ ] Startup time unchanged
- [ ] Cache hit rate maintained

### Maintainability Metrics
- [ ] All public APIs documented
- [ ] Architecture documentation complete
- [ ] Developer onboarding guide updated
- [ ] Contribution guidelines updated

## Timeline Summary

| Phase | Duration | Deliverable |
|-------|----------|-------------|
| 1. Preparation | Week 1 | Infrastructure ready |
| 2. Utilities | Week 2 | Utils module + tests |
| 3. Configuration | Week 3 | Config module + tests |
| 4. Cache | Week 4 | Cache module + tests |
| 5. SPF | Week 5 | SPF module + tests |
| 6. Milter | Week 6 | Milter module + tests |
| 7. Error Handling | Week 7 | Error system |
| 8. Code Quality | Week 8 | Clean code |
| 9. Testing | Week 9 | Full validation |
| 10. Documentation | Week 10 | Release ready |

**Total Estimated Time**: 10 weeks (~400 hours)

## Resources Required

### Tools
- Unit test framework (Check or Criterion)
- Static analyzers (cppcheck, clang-tidy, scan-build)
- Memory debuggers (valgrind, ASAN)
- Code coverage (lcov/gcov)
- Documentation (Doxygen)

### Development Environment
- Linux build environment (Ubuntu 22.04 recommended)
- Docker for testing
- CI/CD environment for automated testing

## Post-Refactoring Benefits

### For Developers
- Easier to understand codebase
- Faster onboarding for new contributors
- Easier to add new features
- Better debugging with isolated modules
- Safer to make changes with unit tests

### For Users
- More reliable (better tested)
- Better error messages
- Improved performance (optimized modules)
- Easier to configure (validated config)
- Better support (maintainable code)

### For Project
- Modernized codebase
- Sustainable long-term maintenance
- Attracts more contributors
- Foundation for future enhancements
- Reduced technical debt

## Conclusion

This refactoring plan transforms smf-spf from a monolithic application into a well-architected, modular, and maintainable codebase while preserving all existing functionality and ensuring backward compatibility.

The phased approach allows for:
- Incremental progress with testable milestones
- Ability to pause/resume as needed
- Low risk through continuous validation
- Clear rollback points if issues arise

**Recommendation**: Execute this plan methodically, completing each phase fully before moving to the next, with thorough testing at every step.

---

**Next Steps**:
1. Review and approve this plan
2. Set up development environment
3. Create refactoring branch
4. Begin Phase 1: Preparation

**Questions? Concerns?**
Open an issue or discussion on GitHub to discuss any aspect of this plan.
