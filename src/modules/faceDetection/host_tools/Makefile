TARGET:=haarcascade_frontalface_alt.hpp
TOOL := xml2array


$(TARGET):$(TOOL) 
	./$< haarcascade_frontalface_alt.xml
	cp -f $(TARGET) ../

$(TOOL):xml2array.o
	gcc -o $@ $^

CC=gcc
CFLAGS :=  

clean:
	@-rm -f $(TARGET) $(TOOL) *.o
