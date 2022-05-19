all:
	g++ -Isrc -o grid_control src/main.cpp src/oscserver.cpp src/jackmidi.cpp -llo -ljack