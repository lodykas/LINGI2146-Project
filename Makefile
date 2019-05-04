all:
	cd root/ && make && cd ../
	cd node/ && make && cd ../

clean:
	cd root/ && make clean && cd ../
	cd node/ && make clean && cd ../
