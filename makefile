target_dir = ./target
target_file = $(target_dir)/brightness_sync
obj_dir = ./obj


$(target_file): main.c | $(target_dir)
	gcc -O3 main.c -o $(target_file)
	sudo chown root $(target_file)
	sudo chmod 4777 $(target_file)
	
$(target_dir):
	mkdir $(target_dir)

.PHONY: clean

clean:
	sudo rm $(target_file)
