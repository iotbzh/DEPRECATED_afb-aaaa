------------------------------------------------------------------------
AGL-AudioBindings expose ALSA, Pulse & Most APIs through AGL framework
------------------------------------------------------------------------
http://www.linuxjournal.com/node/6735/print
http://equalarea.com/paul/alsa-audio.html
http://mpd.wikia.com/wiki/Alsa
http://alsa.opensrc.org/How_to_use_softvol_to_control_the_master_volume

AFB_daemon dependency on Standard Linux Distributions
-------------------------------------------------------
    # handle dependencies > (OpenSuse-42.2, Fedora-25, Ubuntu 16.04.2LTS)
    gcc > 4.8
    systemd-devel (libsystemd-dev>=222) 
    libuuid-devel
    file-devel(OpenSuSe) or libmagic-dev(Ubuntu)
    libjson-c-devel
    alsa-devel
    ElectricFence (BUG should not be mandatory)
    libopenssl-devel libgcrypt-devel libgnutls-devel (optional but requested by libmicrohttpd for https)

    OpenSuse >=42.2 
      zypper in gcc5 gdb gcc5-c++ cit make ElectricFence systemd-devel libopenssl-devel  libuuid-devel alsa-devel libgcrypt-devel libgnutls-devel libjson-c-devel file-devel 

    Ubuntu >= 16.4libuuid-devel
      apt-get install cmake git electric-fence libsystemd-dev libssl-dev uuid-dev libasound2-dev libgcrypt20-dev libgnutls-dev libgnutls-dev libjson-c-dev libmagic-dev

    libmicrohttpd with AGL patches http://iot.bzh/download/public/2016/appfw/libmicrohttpd-0.9.49-agl.tgz
    afb-daemon from AGL Gerrit git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder

```
    # Might want to add following variables into ~/.bashrc
    echo "#----------  AGL options Start ---------" >>~/.bashrc
    echo "# Object: AGL cmake option for  binder/bindings" >>~/.bashrc
    echo "# Date: `date`" >>~/.bashrc
    echo 'export CC=gcc-5; export CXX=g++-5' >>~/.bashrc   # if using gcc5 
    echo 'export INSTALL_PREFIX=$HOME/opt' >>~/.bashrc
    echo 'export LD_LIBRARY_PATH=$INSTALL_PREFIX/lib64:$INSTALL_PREFIX/lib' >>~/.bashrc
    echo 'export LIBRARY_PATH=$INSTALL_PREFIX/lib64:$INSTALL_PREFIX/lib' >>~/.bashrc
    echo 'export PKG_CONFIG_PATH=$INSTALL_PREFIX/lib64/pkgconfig:$INSTALL_PREFIX/lib/pkgconfig' >>~/.bashrc
    echo 'export PATH=$INSTALL_PREFIX/bin:$PATH' >>~/.bashrc
    echo "#----------  AGL options End ---------" >>~/.bashrc
    source ~/.bashrc

    # install AGL pached version of LibMicroHttpd
    wget http://iot.bzh/download/public/2016/appfw/libmicrohttpd-0.9.49-agl.tgz
    tar -xzf libmicrohttpd-0.9.49-agl.tgz
    cd libmicrohttpd-0.9.49-agl
    ./configure --prefix=$INSTALL_PREFIX
    make
    make install-strip

    # retreive last AFB_daemon from AGL
    git clone https://gerrit.automotivelinux.org/gerrit/src/app-framework-binder

    # Warning: previous GCC options should be set before initial cmake (clean Build/*)
    cd app-framework-binder; mkdir build; cd build 
    cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
    make
    make install 
```


```
# Compile binding
source ~/.bashrc  # or any other file where your have place your compilation preferences
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX ..
make
make install

# Start the binder

# From Development Tree
  mkdir $INSTALL_PREFIX/share/wssocks
  afb-daemon --verbose --token="" --ldpaths=./build --port=1234 --roothttp=./htdocs 

# From $INSTALL_PREFIX
  mkdir $INSTALL_PREFIX/share/wssocks
  afb-daemon --verbose --token="" --ldpaths=$INSTALL_PREFIX/lib/audio --port=1234 --roothttp=$INSTALL_PREFIX/htdocs/audio-bindings
```
# replace hd:XX with your own sound card ID ex: "hw:0", "hw:PCH", ...
Start a browser on http://localhost:1234?devid=hw:XX

Start AlsaMixer and change volume you should see event in your browser
```
alsamixer -D hw:0
```
