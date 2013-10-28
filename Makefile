all: prebuild changyy.so

changyy.so:
	mkdir -p output
	gcc -fPIC -shared service/changyy/srv_changyy.c -o output/changyy.so -Iprebuild/c-icap-modules-0.1.6 -DHAVE_CONFIG_H -I/usr/include/c_icap -L/usr/local/lib/c_icap_modules

prebuild:
	sh build.sh
distclean:
	rm -rf ./output ./prebuild
install: prebuild changyy.so
	sudo cp ./output/changyy.so /usr/lib/c_icap/
	sudo /etc/init.d/c-icap restart
clean:
	rm -rf ./output
