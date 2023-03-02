build: main.c
	gcc -O3 main.c -o ./target/debug/brightness_sync
	sudo chown root ./target/debug/brightness_sync
	sudo chmod 4777 ./target/debug/brightness_sync

run: build
	./target/debug/brightness_sync

release:
	gcc -O3 main.c -o ./target/release/brightness_sync
	sudo chown root ./target/release/brightness_sync
	sudo chmod 4777 ./target/release/brightness_sync
