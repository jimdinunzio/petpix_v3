
bin/petpix: src/petpix.h src/petpix.cpp
	g++ -o bin/petpix src/petpix.cpp -lwiringPi

bin/client.prg: src/pet_client/petpix_client.c
	cc65/bin/cl65 -t pet src/pet_client/petpix_client.c -o bin/client.prg