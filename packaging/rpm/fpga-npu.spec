# RPM spec file for fpga-npu

Name:           fpga-npu
Version:        1.0.0
Release:        1%{?dist}
Summary:        FPGA Neural Processing Unit drivers and runtime

License:        MIT
URL:            https://github.com/example/fpga-npu
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.10
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  kernel-devel
BuildRequires:  systemd-devel
BuildRequires:  dkms

Requires:       kernel
Requires:       systemd

%description
This package provides kernel drivers and runtime libraries for
FPGA-based Neural Processing Units (NPUs). It includes kernel
driver modules, user-space runtime libraries, and command-line
utilities for NPU device management and operation.

The NPU provides hardware acceleration for neural network
inference and training workloads, offering significant performance
improvements for machine learning applications.

%package devel
Summary:        Development files for FPGA NPU
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains development files for building applications
that use the FPGA NPU, including header files, static libraries,
CMake configuration files, and code examples.

%package docs
Summary:        Documentation for FPGA NPU
BuildArch:      noarch

%description docs
This package contains comprehensive documentation for the FPGA NPU,
including user guides, API reference, developer documentation,
and tutorials.

%prep
%autosetup

%build
mkdir -p build
cd build
%cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_TESTING=ON \
    -DENABLE_DOCUMENTATION=ON \
    -DPACKAGE_VERSION=%{version}

%make_build
make docs

%install
cd build
%make_install

# Install systemd service file
install -D -m 644 %{_builddir}/%{name}-%{version}/packaging/systemd/fpga-npu.service \
    %{buildroot}%{_unitdir}/fpga-npu.service

# Install udev rules
install -D -m 644 %{_builddir}/%{name}-%{version}/packaging/udev/99-fpga-npu.rules \
    %{buildroot}%{_udevrulesdir}/99-fpga-npu.rules

# Install modprobe configuration
install -D -m 644 %{_builddir}/%{name}-%{version}/packaging/modprobe/fpga-npu.conf \
    %{buildroot}%{_sysconfdir}/modprobe.d/fpga-npu.conf

# Install DKMS configuration
install -D -m 644 %{_builddir}/%{name}-%{version}/packaging/dkms/dkms.conf \
    %{buildroot}%{_usrsrc}/%{name}-%{version}/dkms.conf

# Copy kernel module source for DKMS
mkdir -p %{buildroot}%{_usrsrc}/%{name}-%{version}
cp -r %{_builddir}/%{name}-%{version}/software/driver/* \
    %{buildroot}%{_usrsrc}/%{name}-%{version}/

# Install documentation
mkdir -p %{buildroot}%{_docdir}/%{name}
cp -r %{_builddir}/%{name}-%{version}/docs/* %{buildroot}%{_docdir}/%{name}/

# Install examples
mkdir -p %{buildroot}%{_datadir}/%{name}/examples
cp -r %{_builddir}/%{name}-%{version}/examples/* %{buildroot}%{_datadir}/%{name}/examples/

%check
cd build
ctest --output-on-failure -E "hardware|integration"

%post
# Register with DKMS
dkms add -m %{name} -v %{version} --rpm_safe_upgrade || true
dkms build -m %{name} -v %{version} || true
dkms install -m %{name} -v %{version} || true

# Reload systemd and start service
%systemd_post fpga-npu.service

# Load kernel module
/sbin/modprobe fpga_npu || true

%preun
%systemd_preun fpga-npu.service

# Unload kernel module
/sbin/rmmod fpga_npu || true

%postun
# Remove from DKMS
if [ $1 -eq 0 ]; then
    dkms remove -m %{name} -v %{version} --all || true
fi

%systemd_postun_with_restart fpga-npu.service

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/npu-*
%{_libdir}/libfpga_npu.so.*
%{_unitdir}/fpga-npu.service
%{_udevrulesdir}/99-fpga-npu.rules
%{_sysconfdir}/modprobe.d/fpga-npu.conf
%{_usrsrc}/%{name}-%{version}/

%files devel
%{_includedir}/fpga_npu_lib.h
%{_libdir}/libfpga_npu.so
%{_libdir}/libfpga_npu.a
%{_libdir}/cmake/%{name}/
%{_datadir}/%{name}/examples/

%files docs
%{_docdir}/%{name}/

%changelog
* Wed Jan 01 2025 FPGA NPU Team <npu-team@example.com> - 1.0.0-1
- Initial release of FPGA NPU package
- Includes kernel driver, runtime library, and utilities
- Supports matrix multiplication, convolution, and neural network operations
- Comprehensive documentation and examples included