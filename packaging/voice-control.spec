Name:       voice-control
Summary:    Voice control client library and daemon
Version:    0.2.8
Release:    1
Group:      Graphics & UI Framework/Voice Framework
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Source1002: %{name}-devel.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(capi-media-sound-manager)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  cmake

%description
Voice Control client library and daemon


%package devel
Summary:    Voice control header files for VC development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description devel
Voice control header files for VC development.


%package widget-devel
Summary:    Voice control widget header files for VC development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description widget-devel
Voice control widget header files for VC development.


%package manager-devel
Summary:    Voice control manager header files for VC development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description manager-devel
Voice control manager header files for VC development.


%package setting-devel
Summary:    Voice control setting header files for VC development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description setting-devel
Voice control setting header files for VC development.


%package engine-devel
Summary:    Voice control engine header files for VC development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description engine-devel
Voice control engine header files for VC development.

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} %{SOURCE1002} .


%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
install LICENSE %{buildroot}/usr/share/license/%{name}

%make_install

%post
/sbin/ldconfig

mkdir -p /usr/share/voice/vc

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/libvc.so
%{_libdir}/libvc_setting.so
%{_libdir}/libvc_widget.so
%{_libdir}/libvc_manager.so
%{_bindir}/vc-daemon
/usr/share/dbus-1/system-services/org.tizen.voice*
/etc/dbus-1/system.d/vc-server.conf
/usr/lib/voice/vc/1.0/vc-config.xml
/usr/share/license/%{name}

%files devel
%manifest %{name}-devel.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/voice-control.pc
%{_includedir}/voice_control.h
%{_includedir}/voice_control_authority.h
%{_includedir}/voice_control_command.h
%{_includedir}/voice_control_common.h
%{_includedir}/voice_control_key_defines.h
%{_includedir}/voice_control_command_expand.h

%files widget-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/voice-control-widget.pc
%{_includedir}/voice_control_widget.h
%{_includedir}/voice_control_command.h
%{_includedir}/voice_control_common.h
%{_includedir}/voice_control_key_defines.h
%{_includedir}/voice_control_command_expand.h

%files manager-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/voice-control-manager.pc
%{_includedir}/voice_control_manager.h
%{_includedir}/voice_control_command.h
%{_includedir}/voice_control_common.h
%{_includedir}/voice_control_key_defines.h
%{_includedir}/voice_control_command_expand.h

%files setting-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/voice-control-setting.pc
%{_includedir}/voice_control_setting.h

%files engine-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/voice-control-engine.pc
%{_includedir}/voice_control_plugin_engine.h
