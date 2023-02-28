run: build
	./brightness_sync
	
build: main.c
	gcc -O3 main.c -o brightness_sync
	sudo chown root brightness_sync
	sudo chmod 4777 brightness_sync 
	mkdir ./target
	mv ./brightness_sync ./target/brightness_sync  

clear: build
	rm brightness_sync