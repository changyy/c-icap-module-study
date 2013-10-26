c-icap-module-study
===================
Study at Ubuntu 12.04 64-bit server

### Prebuild

```
$ sudo apt-get install libicapapi-dev
$ mkdir prebuild && cd prebuild
$ apt-get source libc-icap-mod-urlcheck
$ cd c-icap-modules-0.1.6 
$ ./configure
$ cd ..
```

### Build

```
$ mkdir -p output && rm -rf ./output/*
$ gcc -fPIC -shared service/changyy/srv_changyy.c  -o output/changyy.so -Iprebuild/c-icap-modules-0.1.6 -DHAVE_CONFIG_H -I/usr/include/c_icap  -L/usr/local/lib/c_icap_modules
```

### Install

```
$ sudo cp output/changyy.so /usr/lib/c_icap/
$ sudo vim /etc/c-icap/c-icap.conf
Service changyy changyy.so
$ sudo /etc/init.d/c-icap restart
```
