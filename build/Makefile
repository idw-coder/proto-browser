CC=gcc
CFLAGS+=-c -g

TARGET=a.out
INCLUDE=../include
SRC=../src
OBJS=main.o client.o parser.o

# ← 追加：FreeType のフラグを pkg-config から取得
FREETYPE_CFLAGS := $(shell pkg-config --cflags freetype2)
FREETYPE_LIBS   := $(shell pkg-config --libs freetype2)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(FREETYPE_LIBS) -lm

%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) $(FREETYPE_CFLAGS) -I $(INCLUDE) -o $@ $<

clean:
	rm -f *.o $(TARGET) cache

.PHONY: all clean
