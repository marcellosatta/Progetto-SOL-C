CC = gcc 
.PHONY : clean

all:	
	$(info - Compilo il progetto... tra pochi istanti sarà pronto per essere eseguito! -)
	make director 
	make supermarket 
	
director:
 
./director: director.o cnfg.o 
	$(CC) -g director.o cnfg.o -o director
./director.o: director.c
	$(CC) -g -c director.c 

supermarket:

./supermarket: supermarket.o cnfg.o client.o cashier.o queue.o
	$(CC) -g supermarket.o cnfg.o client.o cashier.o queue.o -o supermarket -lpthread
./supermarket.o: supermarket.c
	$(CC) -g -c supermarket.c 
./client.o: client.c
	$(CC) -g -c client.c 
./cashier.o: cashier.c
	$(CC) -g -c cashier.c
./queue.o: queue.c
	$(CC) -g -c queue.c  
./cnfg.o: cnfg.c
	$(CC) -g -c cnfg.c 

test1:	test1_script
	
test1_script: 
	bash ./test1_script.sh

test2:	test2_script 
	
test2_script: 
	bash ./test2_script.sh

clean:
	rm -rf ./director ./supermarket ./director.o ./supermarket.o ./client.o ./cashier.o ./queue.o ./cnfg.o ./statsClients.txt ./statsCashiers.txt director.PID socketSM_D director.dSYM
	$(info - Clean-up eseguito correttamente! -)

