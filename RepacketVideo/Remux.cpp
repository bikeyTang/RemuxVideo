#include "Remux.h"
#include <iostream>

Remux::Remux(string infile, string outfile) :in_filename(infile), out_filename(outfile)
{
	av_register_all();
}


Remux::~Remux()
{

}

bool Remux::executeRemux()
{
	AVPacket readPkt;
	int ret;
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename.c_str(), 0, 0)) < 0) {
		return false;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		return false;
	}
	string::size_type pos = out_filename.find_last_of(".");
	if (pos == string::npos)
		out_filename.append(".mp4");
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename.c_str());
	if (!writeHeader())
		return false;
	int frame_index = 0;
	while (1)
	{
		++frame_index;
		ret = av_read_frame(ifmt_ctx, &readPkt);
		if (ret < 0)
		{
			break;
		}
		if (readPkt.stream_index == videoIndex)
		{
			//readPkt.flags |= AV_PKT_FLAG_KEY;
			av_packet_rescale_ts(&readPkt, ifmt_ctx->streams[readPkt.stream_index]->time_base, ofmt_ctx->streams[readPkt.stream_index]->time_base);
			if (readPkt.pts < readPkt.dts)
			{
				readPkt.pts = readPkt.dts + 1;
			}
			ret = av_interleaved_write_frame(ofmt_ctx, &readPkt);
			if (ret < 0) {
				//break;
				std::cout << "write failed" << std::endl;
			}
		}
		
		av_packet_unref(&readPkt);

	}
	av_packet_unref(&readPkt);
	av_write_trailer(ofmt_ctx);
	avcodec_close(encCtx);
	av_free(encCtx);
	avio_close(ofmt_ctx->pb);
	//avformat_free_context(ofmt_ctx);

	return true;
}
bool Remux::writeHeader()
{

	AVOutputFormat *ofmt = NULL;
	AVStream *out_stream;
	AVCodec *encoder;
	int ret;
	ofmt = ofmt_ctx->oformat;
	for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream *in_stream = ifmt_ctx->streams[i];
		if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoIndex = i;
			AVCodec *pcodec = avcodec_find_encoder(in_stream->codec->codec_id);
			out_stream = avformat_new_stream(ofmt_ctx, pcodec);
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				return false;
			}
			encCtx = out_stream->codec;
			encCtx->codec_id = in_stream->codec->codec_id;
			encCtx->codec_type = AVMEDIA_TYPE_VIDEO;
			encCtx->pix_fmt = in_stream->codec->pix_fmt;
			encCtx->flags = in_stream->codec->flags;
			//encCtx->gop_size = 250;
			encCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
			//av_opt_set(encCtx->priv_data, "profile", "main", 0);
			//av_opt_set(encCtx->priv_data, "preset", "slow", 0);
			av_opt_set(encCtx->priv_data, "tune", "zerolatency", 0);
			if (in_stream->codec->time_base.den > 25)
			{
				encCtx->time_base = { 1, 25 };
				encCtx->pkt_timebase = { 1, 25 };

			}
			else{
				AVRational tmp = in_stream->avg_frame_rate;

				//encCtx->time_base = in_stream->codec->time_base;
				//encCtx->pkt_timebase = in_stream->codec->pkt_timebase;
				encCtx->time_base = { tmp.den, tmp.num };
			}
			encCtx->width = in_stream->codec->width;
			encCtx->height = in_stream->codec->height;
			/*
			AVDictionary *param=0;
			if (encCtx->codec_id == AV_CODEC_ID_H264) {
				av_opt_set(&param, "preset", "slow", 0);
				av_dict_set(&param, "profile", "main", 0);
			}*/
			//没有这句，导致得到的视频没有缩略图等信息
			ret = avcodec_open2(encCtx, pcodec, NULL);
		}
		
	}
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0) {
			return false;
		}
	}
	/*out_stream->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
	out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;*/
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0){
		return false;
	}
	return true;
}