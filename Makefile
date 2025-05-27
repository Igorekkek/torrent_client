all: compile 

compile:
	cmake -S torrent-client-prototype -B cmake-build
	cd cmake-build && make

clean:
	rm -rf cmake-build