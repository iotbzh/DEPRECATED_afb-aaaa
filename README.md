------------------------------------------------------------------------
AGL-AudioBindings expose ALSA, Pulse & Most APIs through AGL framework
------------------------------------------------------------------------


AFB_daemon dependency on Standard Linux Distributions
-------------------------------------------------------
    # handle dependencies > (OpenSuse-42.2, Fedora-25, Ubuntu 16.04.2LTS)
    gcc > 4.8
    libsystemd-dev>=222
    libmicrohttpd-dev>=0.9.48
    openssl-dev
    uuid-dev

```
    # Might want to add following variables into ~/.bashrc
    export CC=gcc-5; export CXX=g++-5  # if using gcc5
    export DEST=$HOME/opt
    export LD_LIBRARY_PATH=$DEST/lib64
    export LIBRARY_PATH=$DEST/lib64
    export PKG_CONFIG_PATH=$DEST/lib64/pkgconfig
    export PATH=$DEST/bin:$PATH

    # Warning: previous GCC options should be set before initial cmake (clean Build/*)
    source ~/.bashrc
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
afb-daemon --token=x --ldpaths=$INSTALL_DIR/lib --port=1234 --roothttp=$INSTALL_DIR/htdocs/audio-bindings --verbose
```


