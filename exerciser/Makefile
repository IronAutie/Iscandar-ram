CC      ?= cc
CFLAGS  ?= -O2 -std=c11 -Wall -Wextra

all: exerciser

exerciser: exerciser.c
	$(CC) $(CFLAGS) -o iscandar-exerciser exerciser.c

check: exerciser
	./iscandar-exerciser check
	./iscandar-exerciser standard

install: exerciser
	install -d /usr/local/sbin /usr/local/share/doc/iscandar-exerciser
	install -m 0755 iscandar-exerciser /usr/local/sbin/
	install -m 0644 iscandar-exerciser.service /etc/systemd/system/
	install -m 0644 README.md /usr/local/share/doc/iscandar-exerciser/
	systemctl daemon-reload
	systemctl enable iscandar-exerciser.service

clean:
	rm -f iscandar-exerciser iscandar-exerciser-ledger.txt

.PHONY: all check install clean
