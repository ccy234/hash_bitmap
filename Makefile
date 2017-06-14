spider_ip:main.o spider_hash_bitmap.o ohtbl.o bitmap.o
	gcc -o spider_ip main.o spider_hash_bitmap.o ohtbl.o bitmap.o -lm -g

main.o:main.c spider_hash_bitmap.h
	gcc -c main.c -g

spider_hash_bitmap.o:spider_hash_bitmap.c spider_hash_bitmap.h ohtbl.h bitmap.h rwlock.h
	gcc -c spider_hash_bitmap.c -g

ohtbl.o:ohtbl.c ohtbl.h rwlock.h 
	gcc -c ohtbl.c -g

bitmap.o:bitmap.c bitmap.h
	gcc -c bitmap.c -g

clean:
	rm spider_ip main.o spider_hash_bitmap.o ohtbl.o bitmap.o

