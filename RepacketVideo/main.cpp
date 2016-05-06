#include <iostream>
#include <string>
#include "Remux.h"

using std::string;
using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
	//string infile = "D:/video/remux/2avi.264";
	//string outfile = "D:/video/remux/22222.mp4";
	if (argc < 3)
	{
		cout << "arguments not enough" << endl;
		return -1;
	}
	string infile = argv[1];
	string outfile = argv[2];
	Remux *rm = new Remux(infile, outfile);
	if (rm->executeRemux())
	{
		cout << "execute success" << endl;
	}
	else{
		cout << "execute failed" << endl;
	}
	return 0;
}

