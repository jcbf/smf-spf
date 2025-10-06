 ke CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

smf-spf is a lightweight, fast Sendmail/Postfix milter implementing the Sender Policy Framework (RFC 7208) using libSPF2. It validates incoming email by checking SPF records to ensure messages are authorized by their originating domains.

This is a C-based single-file project (`smf-spf.c`) that integrates with Sendmail's milter API to provide real-time SPF validation during SMTP transactions.

## Build Commands

### Standard Build
```bash
make              # Build the binary
make clean        # Remove build artifacts
```

### Testing
```bash
make test         # Build with coverage and run full test suite
make coverage     # Build with gcov coverage instrumentation
make showcov      # Generate HTML coverage report in ./out directory
```

The test suite uses miltertest (from opendkim-tools) with Lua test scripts located in `tests/`. Tests run the milter with various configurations and validate SMTP interactions.

### Installation
```bash
make install      # Install binary to /usr/local/sbin and config to /etc/mail/smfs/
```

## Architecture

### Core Components

**Single-File Design**: The entire milter is implemented in `smf-spf.c` (~1200 lines). This includes:

1. **Configuration System**: Parses `/etc/mail/smfs/smf-spf.conf` at startup. Configuration includes whitelists (IP/PTR/From/To), SPF policy decisions (refuse on fail/none), caching parameters, and operational modes.

2. **In-Memory Cache**: Hash-based cache (configurable power-of-2 size via HASH_POWER) stores SPF evaluation results with TTL expiration. Cache keys combine client IP and envelope sender. This significantly reduces DNS lookups for repeated sender/IP combinations.

3. **Milter Callbacks**: Implements standard libmilter hooks:
   - `mlfi_connect`: Captures client IP and hostname
   - `mlfi_helo`: Stores HELO/EHLO identity
   - `mlfi_envfrom`: Performs SPF evaluation on envelope sender
   - `mlfi_envrcpt`: Checks recipient whitelists
   - `mlfi_header`: Extracts subject for tagging
   - `mlfi_eom`: Adds Authentication-Results header, tags subject if configured

4. **SPF Evaluation Logic**:
   - Uses libSPF2's `SPF_server_new()` and `SPF_request_query_mailfrom()`
   - Handles SPF results: Pass, Fail, SoftFail, Neutral, None, TempError, PermError
   - Action mapping configurable: refuse (550), temp fail (450), accept with tagging, quarantine

5. **Whitelist Engine**: Multi-layer whitelisting before SPF evaluation:
   - CIDR-based IP whitelisting (linked list of CIDR structs)
   - PTR (reverse DNS) substring matching
   - Envelope sender (MAIL FROM) substring matching
   - Envelope recipient (RCPT TO) substring matching

6. **Special Features**:
   - **FixedClientIP**: Evaluate SPF using a configured IP instead of connecting IP (useful for submission port outbound validation)
   - **ClientIPNAT**: Translate source IPs for internal mail flows (linked list of src->dest mappings)
   - **SkipAuth**: Bypass SPF checks for SMTP AUTH users (default: on)
   - **SkipNDR**: Optionally skip empty sender (bounce) evaluation
   - **RefuseSPFNone**: Reject when no SPF policy exists
   - **RefuseSPFNoneHelo**: Reject bounces when HELO domain has no SPF

### Data Structures

- **struct context**: Per-connection state (IP, FQDN, HELO, envelope addresses, SPF result)
- **config**: Global configuration loaded at startup
- **cache_item**: Hash table entry with SPF result, expiration time, and collision chain
- **CIDR/IPNAT/STR**: Linked lists for whitelists and IP translation rules

### Threading Model

Uses libmilter's thread-per-connection model. Global structures (cache, config) are read-mostly after initialization. Cache uses simple locking via pthread mutexes (not visible in current code excerpt but standard practice for libmilter).

## Development Notes

### Code Style
- K&R-style C with 4-space indentation (tabs in actual code)
- Error handling via syslog and optional file logging
- Extensive use of macros: `SAFE_FREE`, `hash_size`, `hash_mask`

### Configuration Changes
When modifying configuration options:
1. Update `struct config` definition
2. Add parsing logic in config file reader
3. Update `smf-spf.conf` with documentation
4. Add corresponding test cases in `tests/`

### Testing Strategy
- Tests are scenario-based Lua scripts defining SMTP conversations
- Each test specifies expected milter responses (ACCEPT, REJECT, CONTINUE)
- Coverage target tracked via coveralls.io
- Test configs in `tests/conf/` vary single parameters (RelaxedLocalPart, RefuseFail, etc.)

### SPF Result Handling
Results map to SMTP codes based on configuration:
- **Pass/None**: Accept (unless RefuseSPFNone=on)
- **Fail**: Reject 550 (if RefuseFail=on), or accept with tagging (if TagSubject=on)
- **SoftFail**: Accept with optional tagging or 450 (if SoftFail=on)
- **TempError**: Temp fail 450 (unless AcceptTempError=on)
- **PermError**: Accept by default

### DNS Considerations
Relies heavily on DNS queries (via libSPF2). A local caching DNS resolver is strongly recommended in deployment. The in-memory cache mitigates repeated lookups.

## Common Issues

- **Missing libSPF2**: Build requires libSPF2-dev (Debian/Ubuntu) or libspf2-devel (RHEL/Fedora)
- **Milter socket permissions**: Ensure `/var/run/smfs/` is not group-writable (security requirement)
- **Pthread sleep issues**: libmilter must be compiled with `BROKEN_PTHREAD_SLEEP` defined (Sendmail compilation flag)
- **Cache sizing**: HASH_POWER=16 gives 65536 buckets; adjust for high-volume servers

## Dependencies

- **libmilter**: Sendmail's milter API library
- **libSPF2**: SPF record evaluation (v1.2.5+)
- **pthread**: POSIX threads
- **miltertest** (testing only): From opendkim-tools package

## Configuration File

The primary configuration is `/etc/mail/smfs/smf-spf.conf`. Key sections:
- Whitelisting (IP/PTR/From/To)
- Policy enforcement (RefuseFail, RefuseSPFNone, SoftFail)
- Header manipulation (AddHeader, AddReceivedHeader, TagSubject)
- Cache TTL
- Special features (FixedClientIP, ClientIPNAT, SkipAuth, SkipNDR)
- Runtime settings (User, Socket, Syslog, Daemonize)

Changes require milter restart to take effect.
