SRC = $(wildcard *.c)
TGT = server

$(TGT): $(SRC)
	gcc -g $^ -o $@

.PHONY: clean

clean:
	rm -rf $(TGT)

