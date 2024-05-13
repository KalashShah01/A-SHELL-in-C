all: flash

flash: FLASH.c 
	gcc -Wall -o flash FLASH.c

clean:
	$(RM) flash
