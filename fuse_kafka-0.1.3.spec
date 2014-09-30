Summary: fuse overlay for kafka
Name: fuse_kafka
Version: 0.1.3
Release: 1%{?dist}
Source0: %{name}-%{version}.tar.gz
License: BSD-2-Clause
Group: Development/Tools

Requires(post): info
Requires(preun): info

BuildRequires: python, fuse-devel, librdkafka-devel, zlib
Requires: librdkafka1, fuse

%description 
Intercepts all writes to specified directories and send them to
kafka brokers. It is quite suited for log centralization.

%prep
%setup -q

%build
./build.py

%install
./build.py install

%post
/sbin/install-info %{_infodir}/%{name}.info %{_infodir}/dir || :

%preun
if [ $1 = 0] ; then
/sbin/install-info --delete %{_infodir}/%{name}.info %{_infodir}/dir || :
fi

%files 
%doc README.md

%changelog
* Thu Sep 11 2014 yazgoo <yazgoo@nospam.org> 0.1.3
- Initial version of the package
ORG-LIST-END-MARKER
