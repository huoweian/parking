vpath %.c = sqlite3/
vpath %.h = sqlite3/

SQLite3 := libsqlite3.so

CROSS = arm-none-linux-gnueabi-
CC = $(CROSS)gcc

CPPFLAGS += -I sqlite3/

LDFLAGS  += -L sqlite3/
LDFLAGS  += -lpthread
LDFLAGS  += -lsqlite3
LDFLAGS  += -ldl
LDFLAGS  += -Wl,-rpath=.


parking.elf:parking.c db.c
	$(CC) $^ -o $@ $(CPPFLAGS) $(LDFLAGS) -DNDEBUG

debug.elf:parking.c db.c
	$(CC) $^ -o $@ $(CPPFLAGS) $(LDFLAGS) -DDEBUG

$(SQLite3):
	$(CC) sqlite3/sqlite3.c -o sqlite3/sqlite3.o -c
	$(CC) -shared -fPIC -o sqlite3/libsqlite3.so sqlite3/sqlite3.o

clean:
	$(RM) *.elf sqlite3/*.o

distclean:clean
	$(RM) sqlite3/lib*.so core .*.sw? sqlite3/.*.sw?
