all: timestamp me

clean:
	make -f makefile_me clean
	make -f makefile_user clean

user: timestamp
	make -f makefile_user
	
me:
	make -f makefile_me
	
timestamp:
	sh make_timestamp
	
