spider_ip:main.o spider_hash_bitmap.o ohtbl.o bitmap.o
	gcc -o spider_ip main.o spider_hash_bitmap.o ohtbl.o bitmap.o -lm -g -Wall

main.o:main.c spider_hash_bitmap.h
	gcc -c main.c -g -Wall

spider_hash_bitmap.o:spider_hash_bitmap.c spider_hash_bitmap.h ohtbl.h bitmap.h rwlock.h
	gcc -c spider_hash_bitmap.c -g -Wall

ohtbl.o:ohtbl.c ohtbl.h rwlock.h 
	gcc -c ohtbl.c -g -Wall

bitmap.o:bitmap.c bitmap.h
	gcc -c bitmap.c -g -Wall

clean:
	rm spider_ip main.o spider_hash_bitmap.o ohtbl.o bitmap.o

