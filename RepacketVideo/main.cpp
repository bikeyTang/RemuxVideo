#include <iostream>
#include <string>
#include "Remux.h"

using std::string;
using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
	string infile = "C:/RecordDownload/3dav.flv";
	string outfile = "C:/RecordDownload/3davflv.mp4";
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

