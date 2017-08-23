------------------------------------------------------------------------
                  AGL-Advanced-Audio-Agent
------------------------------------------------------------------------

# Cloning Audio-Binding from Git

```bash
# Initial clone with submodules
git clone --recurse-submodules https://github.com/iotbzh/afb-aaaa
cd  audio-binding

# Do not forget submodules with pulling
git pull --recurse-submodules https://github.com/iotbzh/afb-aaaa
git submodule update
```

# AFB-daemon dependencies

OpenSuse >=42.2, Fedora>=25, Ubuntu>=16.4 AGL Binary packages installation from
[this wiki](https://en.opensuse.org/LinuxAutomotive)

For other distro see chapter [# Building AFB-daemon from source on Standard Linux Distribution](#Build)

# Specific Dependencies

* alsa-devel >= 1.1.3 Warning some distro like Fedora 25 still ship version 1.1.1 as default

> **NOTE**: On Ubuntu 16.4 you should recompile AlsaLib from [source](ftp://ftp.alsa-project.org/pub/lib/)
> as today latest stable is 1.1.4.

  OpenSuse
     - Alsa-devel ```zypper install alsa-devel # 42.3 is shipped default with 1.1.4```

  Fedora 26 (out of the box)
     - Alsa-devel 1.1.4

  Ubuntu-16.4
     - Alsa should be recompiled from source

```bash
wget ftp://ftp.alsa-project.org/pub/lib/alsa-lib-1.1.4.1.tar.bz2
tar -xjf alsa-lib-1.1.4.1.tar.bz2
cd alsa-lib-1.1.4.1
./configure --prefix=/opt
```

  Ubuntu-17.04 (out of the box)
     - Alsa 1.1.4

> **WARNING**: do not forget to upgrade your PKG_CONFIG_PATH=/opt/lib/pkgconfig
> or whatever is the place where your installed alsa.

# Compile AGL-Advanced-Audio-Agent

* Edit your ~/.config/app-templates/cmake.d/00-common-userconf.cmake to reflect
 your local configuration

```cmake
    message(STATUS "*** Fulup Local Config For Native Linux ***")
    add_compile_options(-DNATIVE_LINUX)

    set (RSYNC_TARGET "10.20.101.198")
    set (RSYNC_PREFIX "opt")

    set(CMAKE_INSTALL_PREFIX $ENV{HOME}/opt)
    set(BINDINGS_INSTALL_PREFIX $ENV{HOME}/opt)

    set(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}/lib64/pkgconfig ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
    set(LD_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib64 ${CMAKE_INSTALL_PREFIX}/lib)

```

```bash
    mkdir -p build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
    make

    # Note: 
      1) Controller is now a standalone project and should added on top of project bindings
      2) If you want monitoring add '--alias=/monitoring:$HOME/opt/afb-monitoring'  (should point to monitoring HTML5 pages)
      3) To expose AAAA control interface add '--ws-server=unix:/var/tmp/afb-ws/ctl-aaaa'

    afb-daemon --workdir=. --ldpaths=. --binding=../../afb-controller/build/afb-source/afb-control-afb.so --port=1234  --roothttp=../htdocs --token="" --verbose
    

    Warning: See below net on GDB requiring (--workdir=.)
```

# Note: GDB Source Debug limits/features

Warning: technically AGL bindings are shared libraries loaded thought 'dlopen'. GDB supports source debug of dynamically
loaded libraries, but user should be warn that the actual path to sharelib symbols is directly inherited from DLOPEN.
As a result if you change your directory after binder start with --workdir=xxx then GDB will stop working.

Conclusion: double-check that --workdir=. and run debug session from ./build directory. Any IDEs like Netbeans or VisualCode should work out of the box.

```bash
    Examples:

    # WORK when running in direct
    afb-daemon --workdir=.. --ldpaths=build --port=1234 --roothttp=./htdocs

    # FAIL when using GDB with warning: Could not load shared library ....
    gdb -args afb-daemon --workdir=.. --ldpaths=build --port=1234 --roothttp=./htdocs
    ...
    warning: Could not load shared library symbols for ./build/ucs2-afb/afb-ucs2.so.
    Do you need "set solib-search-path" or "set sysroot"?
    ...
```

To debug sharelib symbol path: start your binder under GDB. Then break your session after the binder has
loaded its bindings. Finally use "info sharedlibrary" and check 'SymsRead'. If equal to 'No' then either you start GDB
from the wrong relative directory, either you have to use 'set solib-search-path' to force the path.

```bash
    gdb -args afb-daemon --workdir=.. --ldpaths=build --port=1234 --roothttp=./htdocs
    run
        ...
        NOTICE: API UNICENS added
        NOTICE: Waiting port=1234 rootdir=.
        NOTICE: Browser URL= http://localhost:1234
    (hit Ctrl-C to break the execution)
    info sharedlibrary afb-*
```

# Running an debugging on a target

```bash
export RSYNC_TARGET=root@xx.xx.xx.xx
export RSYNC_PREFIX=/opt    # WARNING: installation directory should exist

mkdir $RSYNC_TARGET
cd    $RSYNC_TARGET

cmake -DRSYNC_TARGET=$RSYNC_TARGET -DRSYNC_PREFIX=$RSYNC_PREFIX ..
make remote-target-populate

    ./build/target/start-${RSYNC_TARGET}.sh
    firefox http://localhost:1234    # WARNING: do not forget firewall if any

    ssh -tt ${RSYNC_TARGET} speaker-test -twav -D hw:ep01 -c 2
```

Note: remote-target-populate will

* create a script to remotely start the binder on the target in ./build/target/start-on-target-name.sh
* create a gdbinit file to transparently debug remotely in source code with gdb -x ./build/target/gdb-on-target-name.ini
* to run and debug directly from your IDE just configure the run and debug properties with the corresponding filename
* run a generic control and pass virtual channel as a parameter (check remaning PCM in plugin)

Note that Netbeans impose to set debug directory to ./build/pkgout or it won't find binding symbols for source debugging

# <a name="Build"></a> Building AFB-daemon from source on Standard Linux Distribution

## Handle dependencies using packager > (OpenSuse-42.2, Fedora-25, Ubuntu 16.04.2LTS)

Using system package repositories:

* gcc > 4.8
* systemd-devel (libsystemd-dev>=222)
* libuuid-devel (OpenSuse) or uuid-dev (Ubuntu)
* file-devel(OpenSuSe) or libmagic-dev(Ubuntu)
* libjson-c-devel
* ElectricFence (BUG should not be mandatory)
* libopenssl-devel libgcrypt-devel libgnutls-devel (optional but requested by libmicrohttpd for https)

Using [AGL repositories](https://en.opensuse.org/LinuxAutomotive):

* libmicrohttpd>=0.9.55 (as today OpenSuse-42.2, 42.3 or Ubuntu-16.4 ship older versions)
* afb-daemon from AGL Gerrit ```git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder```

OpenSuse >= 42.2:

```bash
export DISTRO="openSUSE_Leap_42.2"
sudo zypper ar http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Framework/${DISTRO}/isv:LinuxAutomotive:app-Framework.repo
sudo zypper ref
#For openSUSE_Leap_42.2 gcc config
sudo zypper install agl-gcc5-setup
#For openSUSE_Leap_42.3 gcc config
#sudo zypper install agl-gcc6-setup
#Install application framework
sudo zypper install agl-app-framework-binder

# Other needed system devel packages:
zypper in gcc5 gdb gcc5-c++ git cmake make ElectricFence systemd-devel libopenssl-devel libuuid-devel alsa-devel libgcrypt-devel libgnutls-devel libjson-c-devel file-devel mxml-devel
```

Fedora >= 26:

```bash
export DISTRO="Fedora_26"
sudo dnf config-manager --add-repo http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Framework/${DISTRO}/isv:LinuxAutomotive:app-Framework.repo
#Install application framework
sudo zypper install agl-app-framework-binder

# Other needed system devel packages:
dnf install gdb git cmake make ElectricFence systemd-devel libopenssl-devel libuuid-devel alsa-devel libgcrypt-devel libgnutls-devel libjson-c-devel file-devel mxml-devel
```

Ubuntu >= 16.4:

```bash
export DISTRO="xUbuntu_16.04"
wget -O - http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Framework/${DISTRO}/Release.key | sudo apt-key add -
sudo bash -c "cat >> /etc/apt/sources.list.d/AGL.list <<EOF
#AGL
deb http://download.opensuse.org/repositories/isv:/LinuxAutomotive:/app-Framework/${DISTRO}/ ./
EOF"
sudo apt-get update
#Install application framework
sudo apt-get install agl-app-framework-binder
# Or for Ubuntu 17.04
#sudo apt-get install agl-app-framework-binder-bin
#sudo apt-get install agl-app-framework-binder-dev

# Other needed system devel packages:
apt-get install cmake git electric-fence libsystemd-dev libssl-dev uuid-dev libasound2-dev libgcrypt20-dev libgnutls-dev libgnutls-dev libjson-c-dev libmagic-dev  libmxml-dev
```

You might want to add following variables into ~/.bashrc:

```bash
    echo "#----------  AGL options Start ---------" >>~/.bashrc
    echo "# Object: AGL cmake option for  binder/bindings" >>~/.bashrc
    echo "# Date: `date`" >>~/.bashrc
    echo 'export CC=gcc-5; export CXX=g++-5' >>~/.bashrc   # if using gcc5
    echo 'export INSTALL_PREFIX=$HOME/opt' >>~/.bashrc
    echo 'export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib64:$INSTALL_PREFIX/lib' >>~/.bashrc
    echo 'export LIBRARY_PATH=$INSTALL_PREFIX/lib64:$INSTALL_PREFIX/lib' >>~/.bashrc
    echo 'export PKG_CONFIG_PATH=$INSTALL_PREFIX/lib64/pkgconfig:$INSTALL_PREFIX/lib/pkgconfig' >>~/.bashrc
    echo 'export PATH=$INSTALL_PREFIX/bin:$PATH' >>~/.bashrc
    echo 'export RSYNC_TARGET=MY_TARGET_HOSTNAME' >>~/.bashrc
    echo 'export RSYNC_PREFIX=./opt' >>~/.bashrc

    echo "#----------  AGL options End ---------" >>~/.bashrc
    source ~/.bashrc
```

## Handle dependencies from sources

Install devel packages from your system official repositories and use the
following instructions to install `libmicrohttpd` and `app-framework-binder`:

```bash
    # install LibMicroHttpd
    LIB_MH_VERSION=0.9.55
    wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-${LIB_MH_VERSION}.tar.gz
    tar -xzf libmicrohttpd-${LIB_MH_VERSION}.tar.gz
    cd libmicrohttpd-${LIB_MH_VERSION}
    ./configure --prefix=${INSTALL_PREFIX}
    make
    sudo make install-strip

    # retrieve last AFB_daemon from AGL
    git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder

    # Warning: previous GCC options should be set before initial cmake (clean Build/*)
    cd app-framework-binder; mkdir -p build; cd build
    cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
    make
    make install
```
