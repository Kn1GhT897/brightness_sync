build: main.c
	gcc -O3 main.c -o ./target/debug/brightness_sync
	sudo chown root ./target/debug/brightness_sync
	sudo chmod 4777 ./target/debug/brightness_sync

run: build
	./target/debug/brightness_sync

release: build
	cp ./target/debug/brightness_sync ./target/release/brightness_sync 

clear: build
	rm ./target/debug/brightness_sync
	rm ./target/release/brightness_sync
