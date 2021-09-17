.PHONEY: all datas clean clean_datas clean_all

all: qspi_param datas

qspi_param:	qspi_param.c
	$(CC) -o $@ $^

datas: qspi_param
	./qspi_param -c133 str_133.bin
	./qspi_param -c200 str_200.bin
	./qspi_param -d -c133 dtr_133.bin
	./qspi_param -d -c200 dtr_200.bin

clean:
	$(RM) -f qspi_param

clean_datas:
	$(RM) -f *.bin
	
cleanall: clean clean_datas