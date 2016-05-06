#include <iostream>
#include <string>
#include "Remux.h"

using std::string;
using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
	string infile = "D:/video/1.flv";
	string outfile = "D:/video/remux/22222.mp4";
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

