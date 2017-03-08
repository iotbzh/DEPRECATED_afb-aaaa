------------------------------------------------------------------------
AGL-AudioBindings expose ALSA, Pulse & Most APIs through AGL framework
------------------------------------------------------------------------


AFB_daemon dependency on Standard Linux Distributions
-------------------------------------------------------
    # handle dependencies > (OpenSuse-42.2, Fedora-25, Ubuntu 16.04.2LTS)
    gcc > 4.8
    libsystemd-dev>=222
    openssl-dev
    uuid-dev

    libmicrohttpd with AGL patches http://iot.bzh/download/public/2016/appfw/libmicrohttpd-0.9.49-agl.tgz
    afb-daemon from AGL Gerrit git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder

```
    # Might want to add following variables into ~/.bashrc
    # export CC=gcc-5; export CXX=g++-5  # if using gcc5
    echo "export DEST=$HOME/opt" >>~/.bashrc
    echo "export LD_LIBRARY_PATH=$DEST/lib64" >>~/.bashrc
    echo "export LIBRARY_PATH=$DEST/lib64" >>~/.bashrc
    echo "export PKG_CONFIG_PATH=$DEST/lib64/pkgconfig" >>~/.bashrc
    echo "export PATH=$DEST/bin:$PATH" >>~/.bashrc
    source ~/.bashrc

    # install AGL pached version of LibMicroHttpd
    wget http://iot.bzh/download/public/2016/appfw/libmicrohttpd-0.9.49-agl.tgz
    tar -xzf libmicrohttpd-0.9.49-agl.tgz
    cd libmicrohttpd-0.9.49-agl
    ./configure --prefix=$DEST
    make
    make install-strip

    # retreive last AFB_daemon from AGL
    git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder

    # Warning: previous GCC options should be set before initial cmake (clean Build/*)
    cd app-framework-binder; mkdir build; cd build 
    cmake -DCMAKE_INSTALL_PREFIX=$DEST ..
    make
    make install 
```

Other Audio Binding Dependencies
----------------------------------
    afb-daemon
    alsa-devel

    Edit CMakeList.txt to tune options


```
# Compile binding
INSTALL_DIR=xxxx # default ./Install
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR ..
make
make install
ls

# Start the binder
afb-daemon --token=x --ldpaths=$INSTALL_DIR/lib/audio --port=1234 --roothttp=$INSTALL_DIR/htdocs/audio-bindings --verbose
```
Start a browser on http://localhost:1234?devid=hw:0

Start AlsaMixer and change volume you should see event in your browser
```
alsamixer -D hw:0
```