# Do a systemd-based build from F-15; otherwise, a sysvinit-based build
%global use_systemd %([ "(" 0%{?fedora} -gt 14 ")" -o "(" 0%{?rhel} -gt 6 ")" ] && echo 1 || echo 0)

# This macro only defined by default around Fedora 10 time
%{!?_initddir:%global _initddir %{_initrddir}}

# With systemd, the run directory is /run; otherwise it's /var/run
%if %{use_systemd}
%global rundir /run
%else
%global rundir %{_localstatedir}/run
%endif

Summary:	Mail filter for Sender Policy Framework verification
Name:		smf-spf
Version:	2.4.3
Release:	1%{?dist}
License:	GPLv2+
Group:		System Environment/Daemons
Url:		https://github.com/jcbf/smf-spf
Source0:	%{name}-%{version}.tar.gz
Source1:	smf-spf.sysv
Source2:	smf-spf.service
Source3:	smfs.conf
Source4:	README.rpm
Patch0:		smf-spf-2.4.3-Makefile.patch
Patch2:		smf-spf-2.4.3-conf.patch
Patch5:		smf-spf-2.4.3-rundir.patch
Requires:	sendmail >= 8.12
BuildRequires:	sendmail-devel >= 8.12, libspf2-devel >= 1.2.5
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(id -nu)
Requires(pre):	/usr/bin/getent, /usr/sbin/groupadd, /usr/sbin/useradd, /usr/sbin/usermod
%if %{use_systemd}
BuildRequires:	systemd-units
Requires(post):	systemd-units
Requires(preun): systemd-units
Requires(postun): systemd-units
%else
Requires(post): /sbin/chkconfig
Requires(preun): /sbin/chkconfig, initscripts
Requires(postun): initscripts
%endif

%description
smf-spf is a lightweight, fast and reliable Sendmail milter that implements the
Sender Policy Framework technology with the help of the libspf2 library. It
checks SPF records to make sure that e-mail messages are authorized by the
domain that it is coming from. It's an alternative for the spfmilter,
spf-milter, and milter-spiff milters.

%prep
%setup -q

# Don't use bundled libspf2 headers, use the system ones
rm -rf spf2

# Copy in additional sources
cp -a %{SOURCE1} %{SOURCE2} %{SOURCE3} %{SOURCE4} .

# Use the distribution optimization flags and don't strip the binary,
# so we get usable debuginfo
%patch0 -p1

# Tag failing messages by default rather than rejecting them
%patch2 -p1

# Use /run rather than /var/run with systemd
%if %{use_systemd}
%patch5 -p1
%endif

%build
make %{?_smp_mflags} OPTFLAGS="%{optflags}"

%install
rm -rf %{buildroot}
install -d -m 700 %{buildroot}%{rundir}/smfs
install -Dp -m 755 smf-spf %{buildroot}%{_sbindir}/smf-spf
install -Dp -m 644 smf-spf.conf %{buildroot}%{_sysconfdir}/mail/smfs/smf-spf.conf
%if %{use_systemd}
# Install systemd unit file and tmpfiles.d configuration for /run/smfs
install -Dp -m 644 smf-spf.service %{buildroot}%{_unitdir}/smf-spf.service
install -Dp -m 644 smfs.conf %{buildroot}%{_prefix}/lib/tmpfiles.d/smfs.conf
%else
# Install SysV initscript
install -Dp -m 755 smf-spf.sysv %{buildroot}%{_initddir}/smf-spf
%endif

# Create dummy socket for %%ghost-ing
: > %{buildroot}%{rundir}/smfs/smf-spf.sock

%clean
rm -rf %{buildroot}

%pre
/usr/bin/getent group smfs >/dev/null || /usr/sbin/groupadd -r smfs
/usr/bin/getent passwd smfs >/dev/null || \
	/usr/sbin/useradd -r -g smfs -d %{_localstatedir}/lib/smfs \
		-s /sbin/nologin -c "Smart Sendmail Filters" smfs
exit 0

%post
if [ $1 -eq 1 ]; then
	# Initial installation 
%if %{use_systemd}
	/bin/systemctl daemon-reload &>/dev/null || :
%else
	/sbin/chkconfig --add smf-spf || :
%endif
fi

%preun
if [ $1 -eq 0 ]; then
	# Package removal, not upgrade
%if %{use_systemd}
	/bin/systemctl --no-reload disable smf-spf.service &>/dev/null || :
	/bin/systemctl stop smf-spf.service &>/dev/null || :
%else
	%{_initddir}/smf-spf stop &>/dev/null || :
	/sbin/chkconfig --del smf-spf || :
%endif
fi

%postun
%if %{use_systemd}
/bin/systemctl daemon-reload &>/dev/null || :
%endif
if [ $1 -ge 1 ]; then
	# Package upgrade, not uninstall
%if %{use_systemd}
	/bin/systemctl try-restart smf-spf.service &>/dev/null || :
%else
	%{_initddir}/smf-spf condrestart &>/dev/null || :
%endif
fi

%files
%defattr(-,root,root,-)
%doc ChangeLog COPYING readme README.rpm
%{_sbindir}/smf-spf
%dir %{_sysconfdir}/mail/smfs/
%config(noreplace) %{_sysconfdir}/mail/smfs/smf-spf.conf
%attr(0700,smfs,smfs) %dir %{rundir}/smfs/
%ghost %{rundir}/smfs/smf-spf.sock
%if %{use_systemd}
%{_prefix}/lib/tmpfiles.d/smfs.conf
%{_unitdir}/smf-spf.service
%else
%{_initddir}/smf-spf
%endif

%changelog
* Mon Jun 15 2020 Jordi Sanfeliu <jordi@fibranet.cat> 2.4.3-1
- Updated to 2.4.3.

* Sun Jul 28 2013 Paul Howarth <paul@city-fan.org> 2.0.2-6
- Systemd detection was broken in F-19 so hardcode it instead

* Tue Jul  3 2012 Paul Howarth <paul@city-fan.org> 2.0.2-5
- Move tmpfiles.d config from %%{_sysconfdir} to %%{_prefix}/lib
- Delay start-up until after network

* Thu Sep 29 2011 Paul Howarth <paul@city-fan.org> 2.0.2-4
- Use presence of /run/lock to determine if init is systemd

* Wed Jul 13 2011 Paul Howarth <paul@city-fan.org> 2.0.2-3
- Switch to systemd configuration where appropriate
- Nobody else likes macros for commands

* Wed Jul 28 2010 Paul Howarth <paul@city-fan.org> 2.0.2-2
- Modernize initscript and scriptlets
- Add dist tag

* Thu Jan 11 2007 Paul Howarth <paul@city-fan.org> 2.0.2-1
- Update to 2.0.2
- Failing mails now tagged [SPF:fail] by default instead of [SPF-FAIL]

* Fri Sep 22 2006 Paul Howarth <paul@city-fan.org> 2.0.1-1
- Update to 2.0.1

* Thu Sep 21 2006 Paul Howarth <paul@city-fan.org> 2.0.0-1
- Initial RPM build
