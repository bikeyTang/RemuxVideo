#pragma once
#include <string>

extern "C"
{
#include "libavformat\avformat.h"
#include "libavutil\opt.h"
}

using std::string;
class Remux
{
public:
	Remux(string infile,string outfile);
	~Remux();

	bool executeRemux();

private:
	int videoIndex;
	bool isMp4;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	string in_filename, out_filename;
	AVCodecContext *encCtx=NULL;
	bool writeHeader();
	void initPts(int64_t*, int64_t);
	void initDts(int64_t*, int64_t);
};

