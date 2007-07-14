Name:           gtktwitter
Version:        0.0.7
Release:        1%{?dist}
Summary:        A Twitter client for Linux which using GTK
Group:          Applications/Network
License:        GPL
URL:            http://www.ac.cyberhome.ne.jp/~mattn
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  gtk2-devel >= 2.4 libxml >= 2.6 curl-devel desktop-file-utils
Requires(post): desktop-file-utils
Requires(postun): desktop-file-utils

%description
A lightweight Twitter client for Linux written in GTK.

%prep
%setup -q -n %{name}-%{version}

%build
aclocal
automake -a
autoheader 
autoconf
chmod +x configure
%configure
make %{?_smp_mflags}
cat>>%{name}.desktop<<EOF
[Desktop Entry]
Encoding=UTF-8
Exec=%{name}
Icon=%{_datadir}/%{name}/logo.png
Type=Application
Terminal=false
Name=GtkTwitter
Categories=Application;Network;
EOF

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
cp data/logo.png $RPM_BUILD_ROOT%{_datadir}/gtktwitter/logo.png
desktop-file-install --vendor=fedora \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category X-Fedora \
  --add-category GTK \
  --delete-original \
  %{name}.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%post
update-desktop-database

%postun
update-desktop-database

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/applications/*%{name}.desktop
%{_datadir}/gtktwitter/logo.png
%{_datadir}/gtktwitter/twitter.png
%{_datadir}/gtktwitter/loading.gif
%{_datadir}/gtktwitter/reload.png
%{_datadir}/gtktwitter/home.png
%{_datadir}/gtktwitter/post.png

%changelog
* Sat Jul  14 2007 Yasuhiro Matsumoto <mattn.jp at gmail.com>
- Updated version to 0.0.7
* Tue May  22 2007 Yasuhiro Matsumoto <mattn.jp at gmail.com>
- Updated version to 0.0.6
* Tue May  15 2007 Yasuhiro Matsumoto <mattn.jp at gmail.com>
- Updated version to 0.0.5
* Mon May  07 2007 Yasuhiro Matsumoto <mattn.jp at gmail.com>
- Updated version to 0.0.4
* Thu May  03 2007 Yasuhiro Matsumoto <mattn.jp at gmail.com>
- Initial RPM release.
