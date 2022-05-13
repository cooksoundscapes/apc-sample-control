all:
	g++ -o grid_control main.cpp oscserver.cpp jackmidi.cpp -llo -ljack