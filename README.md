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
$ gcc -fPIC -shared service/changyy/srv_changyy.c -Iprebuild/c-icap-modules-0.1.6 -DHAVE_CONFIG_H -I/usr/include/c_icap  -L/usr/local/lib/c_icap_modules -o changyy.so
```
