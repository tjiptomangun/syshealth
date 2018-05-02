DFLAGS=-Wall -ggdb3 -I .
RFLAGS=-Wall -I . 
OSDEF=-D _RHEL_AS4_
cc=gcc

debug: syshealth.c
	$(CC) $(DFLAGS) $(OSDEF) syshealth.c -o syshealth
	cp syshealth randcheck

release: syshealth.c
	$(CC) $(RFLAGS) $(OSDEF) syshealth.c -o syshealth
	cp syshealth randcheck

clean:
	rm syshealth
	
