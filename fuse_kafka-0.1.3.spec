Summary: fuse overlay for kafka
Name: fuse_kafka
Version: 0.1.3
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.gz
License: BSD-2-Clause
Group: Development/Tools

Requires(post): info
Requires(preun): info

BuildRequires: python, openssl-devel, fuse-devel, librdkafka-devel, zlib
Requires: librdkafka1, fuse, openssl, glibc
# do not save library versions dependencies:
AutoReqProv: no
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description 
Intercepts all writes to specified directories and send them to
kafka brokers. It is quite suited for log centralization.

%prep
%setup -q

%build
./build.py

%install
rm -rf %{buildroot}
BUILDROOT=%{buildroot} ./build.py install

%post
/sbin/install-info %{_infodir}/%{name}.info %{_infodir}/dir || :

%preun
if [ $1 = 0] ; then
/sbin/install-info --delete %{_infodir}/%{name}.info %{_infodir}/dir || :
fi

%files -n %{name}
%defattr(-,root,root)
/usr/bin/fuse_kafka
/etc/init.d/fuse_kafka
/etc/fuse_kafka.conf

%changelog
* Thu Sep 11 2014 yazgoo <yazgoo@nospam.org> 0.1.3
- Initial version of the package
ORG-LIST-END-MARKER
