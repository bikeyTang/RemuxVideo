#pragma once
#include <string>

extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\opt.h"
}

using std::string;
#define H264_R
class Remux
{
public:
	Remux(string infile,string outfile);
	~Remux();

	bool executeRemux();

private:
	int videoIndex;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	string in_filename, out_filename;
	AVCodecContext *encCtx=NULL;
	bool writeHeader();
};

