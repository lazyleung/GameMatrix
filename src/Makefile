CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)
OBJECTS=GameMatrix.o Tetris.o Menu.o
# ThreadSync.o AudioInput.o AlsaInput.o WaveletBpmDetector.o wavelet.o freq_data.o 
BINARIES=GameMatrix.app

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=$(realpath ..)/rpi-rgb-led-matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lwiringPi -lasound -lfftw3 -lfftw3f

all : $(BINARIES)

$(RGB_LIBRARY) : FORCE
	$(MAKE) -C $(RGB_LIBDIR)

GameMatrix.app : $(OBJECTS) $(RGB_LIBRARY)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# All the binaries that have the same name as the object file.
% : %.o $(RGB_LIBRARY) $(AUBIO_LIBRARY)
	$(CXX) $< -o $@ $(LDFLAGS)

%.o : %.cpp
	$(CXX) -I$(RGB_INCDIR) -I$(AUBIO_INCDIR) $(CXXFLAGS) -c -o $@ $<

%.o : %.c
	$(CC) -I$(RGB_INCDIR) -I$(AUBIO_INCDIR) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)

FORCE:
.PHONY: FORCE

install:
	sudo chmod 777 GameMatrix.app
	sudo cp GameMatrix.app /usr/sbin/
	sudo chmod 777 GameMatrix.service
	sudo cp GameMatrix.service /usr/lib/systemd/system/