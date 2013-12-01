all:	all_lib all_frame all_plugins

all_lib:
	make -C ../lib64/elib 

all_frame:
	make -C ../frame

all_plugins:
	make -C ./src

clean:
	make -C ./src clean
        
distclean:      clean
	make -C ../frame clean
	make -C ../lib/elib clean 
