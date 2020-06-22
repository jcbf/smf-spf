smf-spf
=======

![GitHub release](https://img.shields.io/github/release/jcbf/smf-spf/all.svg?style=plastic)
[![Build Status](https://travis-ci.org/jcbf/smf-spf.svg?branch=master)](https://travis-ci.org/jcbf/smf-spf)
[![Coverage Status](https://coveralls.io/repos/github/jcbf/smf-spf/badge.svg?branch=master)](https://coveralls.io/github/jcbf/smf-spf?branch=master) ![Docker Pulls](https://img.shields.io/docker/pulls/underspell/smf-spf)

It's a lightweight, fast and reliable Sendmail milter that implements the Sender Policy Framework

This was abandoned code and has several bugfixes and enhancements. like:

- Make MAIL and RCPT limits RFC 5321 compliant  ( both localpart and domain )
- Reply codes aligned with RFC 7208
- Daemonize option via command line 
- Fix SPF_RESULT_TEMPERROR handling
- fix segfault when server address is unknown 
- Create a test suite and covergae tests
- Configurable refuse when SPF is none
- Reject NDR when there is no SPF policy defined 

# Change Log

## [v2.4.4](https://github.com/jcbf/smf-spf/tree/v2.4.4) (2020-06-21)

**Implemented enhancements:**

- Docker image improvments
- Misc fixes [\#72](https://github.com/jcbf/smf-spf/pull/72) ([jcbf](https://github.com/jcbf))
- Log to file [\#71](https://github.com/jcbf/smf-spf/pull/71) ([jcbf](https://github.com/jcbf))
- specfile and patches for building on Fedora and CentOS Linux [\#70](https://github.com/jcbf/smf-spf/pull/70) ([mikaku](https://github.com/mikaku))
- Get daemon name from cmd line as requested in \#67 [\#68](https://github.com/jcbf/smf-spf/pull/68) ([jcbf](https://github.com/jcbf))

## [v2.4.3](https://github.com/jcbf/smf-spf/tree/v2.4.3) (2020-03-25)

[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.4.2...v2.4.3)

**Implemented enhancements:**

- Make SPF evaluation  with a fixed IP [\#65](https://github.com/jcbf/smf-spf/issues/65)
- Disable localpart size check [\#52](https://github.com/jcbf/smf-spf/issues/52)

**Fixed bugs:**

- Typos [\#55](https://github.com/jcbf/smf-spf/issues/55)
- `smf-spf -f` does not override config file value `Daemonize` [\#62](https://github.com/jcbf/smf-spf/issues/62)

**Merged pull requests:**

- \#65 add outbound spf [\#66](https://github.com/jcbf/smf-spf/pull/66) ([jcbf](https://github.com/jcbf))
- Add systemd service definition. [\#64](https://github.com/jcbf/smf-spf/pull/64) ([whyscream](https://github.com/whyscream))
- Correct parse os option -f [\#63](https://github.com/jcbf/smf-spf/pull/63) ([jcbf](https://github.com/jcbf))
- \#55 Fix typos [\#57](https://github.com/jcbf/smf-spf/pull/57) ([jcbf](https://github.com/jcbf))

## [v2.4.2](https://github.com/jcbf/smf-spf/tree/v2.4.2) (2018-07-18)
[Full Changelog between 2.4.1 and 2.4.2](https://github.com/jcbf/smf-spf/compare/v2.4.1...v2.4.2)

**Implemented enhancements:**

- Fix codewarnings [\#54](https://github.com/jcbf/smf-spf/pull/54) ([jcbf](https://github.com/jcbf))
- Only domain size is checked  [\#50](https://github.com/jcbf/smf-spf/issues/50)

**Merged pull requests:**

- Allow relaxed localpart size verification [\#53](https://github.com/jcbf/smf-spf/pull/53) ([jcbf](https://github.com/jcbf))

## [Unreleased Changes](https://github.com/jcbf/smf-spf/compare/v2.4.2...HEAD)


## [v2.4.1](https://github.com/jcbf/smf-spf/tree/v2.4.1) (2018-04-19)
[Full Changelog between 2.4.0 and 2.4.1](https://github.com/jcbf/smf-spf/compare/v2.4.0...v2.4.1)

**Implemented enhancements:**

- Reject bounces when there is no SPF policy defined [\#46](https://github.com/jcbf/smf-spf/issues/46)
- Reject messages with an empty sender [\#49](https://github.com/jcbf/smf-spf/pull/49) ([jcbf](https://github.com/jcbf))
- Add SPF result on log [\#48](https://github.com/jcbf/smf-spf/pull/48) ([jcbf](https://github.com/jcbf))

**Merged pull requests:**

- Check for the localpart size. [\#51](https://github.com/jcbf/smf-spf/pull/51) ([jcbf](https://github.com/jcbf))

## [v2.4.0](https://github.com/jcbf/smf-spf/tree/v2.4.0) (2018-02-08)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.3.1...v2.4.0)

**Implemented enhancements:**

- Configurable refuse when SPF is none [\#42](https://github.com/jcbf/smf-spf/pull/42) ([jcbf](https://github.com/jcbf))
- Configurable hostname [\#40](https://github.com/jcbf/smf-spf/pull/40) ([jcbf](https://github.com/jcbf))

**Fixed bugs:**

- WhitelistTo should accept message [\#37](https://github.com/jcbf/smf-spf/issues/37)
- WhitelistTo should return SMFIS\_ACCEPT [\#38](https://github.com/jcbf/smf-spf/pull/38) ([jcbf](https://github.com/jcbf))

**Closed issues:**

- Possible issue reporting Fail string in sendmail reject message [\#33](https://github.com/jcbf/smf-spf/issues/33)

**Merged pull requests:**

- Better coverage. [\#36](https://github.com/jcbf/smf-spf/pull/36) ([jcbf](https://github.com/jcbf))

## [v2.3.1](https://github.com/jcbf/smf-spf/tree/v2.3.1) (2017-11-07)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.3...v2.3.1)

**Implemented enhancements:**

- Allow Received-SPF header back. [\#32](https://github.com/jcbf/smf-spf/pull/32) ([jcbf](https://github.com/jcbf))

**Fixed bugs:**

- Reply codes aligned with RFC [\#34](https://github.com/jcbf/smf-spf/pull/34) ([jcbf](https://github.com/jcbf))

**Merged pull requests:**

- Prepare version 2.3.1 [\#35](https://github.com/jcbf/smf-spf/pull/35) ([jcbf](https://github.com/jcbf))
- Docker image [\#29](https://github.com/jcbf/smf-spf/pull/29) ([tyranron](https://github.com/tyranron))

## [v2.3](https://github.com/jcbf/smf-spf/tree/v2.3) (2016-11-30)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.2...v2.3)

**Implemented enhancements:**

- Create a test suite [\#17](https://github.com/jcbf/smf-spf/issues/17)
- Add debug output to test script [\#24](https://github.com/jcbf/smf-spf/pull/24) ([jcbf](https://github.com/jcbf))

**Merged pull requests:**

- Enable tests [\#26](https://github.com/jcbf/smf-spf/pull/26) ([jcbf](https://github.com/jcbf))
- Add coverralls badge [\#25](https://github.com/jcbf/smf-spf/pull/25) ([jcbf](https://github.com/jcbf))
- Enable tests and coverage [\#22](https://github.com/jcbf/smf-spf/pull/22) ([jcbf](https://github.com/jcbf))

## [v2.2](https://github.com/jcbf/smf-spf/tree/v2.2) (2016-11-03)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.1.1...v2.2)

**Fixed bugs:**

- fix segfault when server address is unknown [\#21](https://github.com/jcbf/smf-spf/pull/21) ([Milek7](https://github.com/Milek7))

**Merged pull requests:**

- don't include \<\> characters in Authentication-Results header [\#20](https://github.com/jcbf/smf-spf/pull/20) ([Milek7](https://github.com/Milek7))

## [v2.1.1](https://github.com/jcbf/smf-spf/tree/v2.1.1) (2016-09-21)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.1.0...v2.1.1)

**Implemented enhancements:**

- handle SPF\_RESULT\_TEMPERROR result [\#14](https://github.com/jcbf/smf-spf/issues/14)

**Fixed bugs:**

- Uncompilable release [\#19](https://github.com/jcbf/smf-spf/issues/19)

**Closed issues:**

- Make a release [\#10](https://github.com/jcbf/smf-spf/issues/10)

## [v2.1.0](https://github.com/jcbf/smf-spf/tree/v2.1.0) (2016-09-19)
[Full Changelog](https://github.com/jcbf/smf-spf/compare/v2.2.0...v2.1.0)

## [v2.2.0](https://github.com/jcbf/smf-spf/tree/v2.2.0) (2016-09-19)
**Implemented enhancements:**

- Refuse messages with softfail [\#8](https://github.com/jcbf/smf-spf/issues/8)
- MAIL and RCPT limits are not RFC compliant [\#4](https://github.com/jcbf/smf-spf/issues/4)
- mail-filter/smf-spf-2.0.2 patches [\#1](https://github.com/jcbf/smf-spf/issues/1)
- daemonize option via command line [\#7](https://github.com/jcbf/smf-spf/pull/7) ([jcbf](https://github.com/jcbf))
-  \*  Bumped version [\#6](https://github.com/jcbf/smf-spf/pull/6) ([jcbf](https://github.com/jcbf))
- Debian init [\#3](https://github.com/jcbf/smf-spf/pull/3) ([whyscream](https://github.com/whyscream))
- Add support for daemonisation in config file [\#2](https://github.com/jcbf/smf-spf/pull/2) ([whyscream](https://github.com/whyscream))

**Fixed bugs:**

- Fix RFC5321 path limit [\#5](https://github.com/jcbf/smf-spf/pull/5) ([jcbf](https://github.com/jcbf))

**Merged pull requests:**

- Support for temperror handling. [\#18](https://github.com/jcbf/smf-spf/pull/18) ([jcbf](https://github.com/jcbf))
- Fix for \#8 [\#16](https://github.com/jcbf/smf-spf/pull/16) ([jcbf](https://github.com/jcbf))
- Bump version to 2.1.0 [\#15](https://github.com/jcbf/smf-spf/pull/15) ([jcbf](https://github.com/jcbf))
- Fix version usage [\#13](https://github.com/jcbf/smf-spf/pull/13) ([tyranron](https://github.com/tyranron))
- One more typo fix for conf.soft\_fail property [\#12](https://github.com/jcbf/smf-spf/pull/12) ([tyranron](https://github.com/tyranron))
- Fix for \#8 - Allow softfail when refusing email [\#9](https://github.com/jcbf/smf-spf/pull/9) ([jcbf](https://github.com/jcbf))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*
