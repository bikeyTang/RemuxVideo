#include "Remux.h"
#include <iostream>
#include <algorithm>
Remux::Remux(string infile, string outfile) :in_filename(infile), out_filename(outfile), isMp4(false)
{
	av_register_all();
	string::size_type pos = infile.find_last_of(".");
	string in_suffix = infile.substr(pos);
	//把后缀名转成小写字符
	transform(in_suffix.begin(), in_suffix.end(), in_suffix.begin(), ::tolower);
	if (in_suffix == ".mp4"||in_suffix==".mkv"){
		isMp4 = true;
	}

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
	AVBitStreamFilterContext* h264bsfc = NULL;
	if (isMp4)
		 h264bsfc= av_bitstream_filter_init("h264_mp4toannexb");
	int startFlag = 1;
	int64_t pts_start_time = 0;
	int64_t dts_start_time = 0;
	int64_t pre_pts = 0;
	int64_t pre_dts = 0;
	while (1)
	{
		
		ret = av_read_frame(ifmt_ctx, &readPkt);
		if (ret < 0)
		{
			break;
		}
		if (readPkt.stream_index == videoIndex)
		{
			++frame_index;
			//过滤掉前面的非I帧
			if (frame_index == startFlag&&readPkt.flags != AV_PKT_FLAG_KEY){
				++startFlag;
				continue;
			}
			if (frame_index == startFlag){
				pts_start_time = readPkt.pts>0? readPkt.pts:0;
				dts_start_time = readPkt.dts>0? readPkt.dts:0;
				pre_dts = dts_start_time;
				pre_pts = pts_start_time;
			}

			//过滤得到h264数据包
			if (isMp4)
				av_bitstream_filter_filter(h264bsfc, ifmt_ctx->streams[videoIndex]->codec, NULL, &readPkt.data, &readPkt.size, readPkt.data, readPkt.size, 0);

			if (readPkt.pts != AV_NOPTS_VALUE){
				readPkt.pts = readPkt.pts - pts_start_time;
			}
			/*else
			{
				int64_t delta = av_rescale_q(1, ofmt_ctx->streams[videoIndex]->time_base, ifmt_ctx->streams[videoIndex]->time_base);
				readPkt.pts = pre_pts + delta + 1;
			}*/
			if (readPkt.dts != AV_NOPTS_VALUE){
				if (readPkt.dts <= pre_dts&&frame_index != startFlag){
					//保证 dts 单调递增
					int64_t delta = av_rescale_q(1, ofmt_ctx->streams[videoIndex]->time_base, ifmt_ctx->streams[videoIndex]->time_base);
					readPkt.dts = pre_dts + delta + 1;
				}
				else{
					//initDts(&readPkt.dts, dts_start_time);
					readPkt.dts = readPkt.dts - dts_start_time;
				}
			}
			
			/*if (readPkt.dts>5 * pre_dts || readPkt.pts > 5 * pre_pts)
			{
				std::cout << readPkt.pts << "--------------dts" << readPkt.dts << std::endl;
			}*/
			pre_dts = readPkt.dts;
			pre_pts = readPkt.pts;
			
			av_packet_rescale_ts(&readPkt, ifmt_ctx->streams[readPkt.stream_index]->time_base, ofmt_ctx->streams[readPkt.stream_index]->time_base);
			if (readPkt.duration < 0)
			{
				readPkt.duration = 0;
			}
			if (readPkt.pts < readPkt.dts)
			{
				readPkt.pts = readPkt.dts + 1;
			}
			//这里如果使用av_interleaved_write_frame 会导致有时候写的视频文件没有数据。
			ret =av_write_frame(ofmt_ctx, &readPkt);
			if (ret < 0) {
				//break;
				std::cout << "write failed" << std::endl;
			}
		}
		
		av_packet_unref(&readPkt);

	}
	av_packet_unref(&readPkt);
	av_write_trailer(ofmt_ctx);
	//avcodec_close(encCtx);
	//av_free(encCtx);
	avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	return true;
}
void Remux::initPts(int64_t *pts, int64_t start_time){
	AVCodecContext *enc = ofmt_ctx->streams[videoIndex]->codec;
	AVRational tb = enc->time_base;
	int extra_bits = av_clip(29 - av_log2(tb.den), 0, 16);

	tb.den <<= extra_bits;
	int64_t float_pts =
		av_rescale_q(*pts, ifmt_ctx->streams[videoIndex]->time_base, tb) -
		av_rescale_q(start_time, { 1, AV_TIME_BASE }, tb);
	float_pts /= 1 << extra_bits;
	// avoid exact midoints to reduce the chance of rounding differences, this can be removed in case the fps code is changed to work with integers
	float_pts += FFSIGN(float_pts) * 1.0 / (1 << 17);

	*pts =
		av_rescale_q(*pts, ifmt_ctx->streams[videoIndex]->time_base, enc->time_base) -
		av_rescale_q(start_time, { 1, AV_TIME_BASE }, enc->time_base);
}
void Remux::initDts(int64_t *dts, int64_t start_time){
	AVCodecContext *enc = ofmt_ctx->streams[videoIndex]->codec;
	AVRational tb = enc->time_base;
	int extra_bits = av_clip(29 - av_log2(tb.den), 0, 16);

	tb.den <<= extra_bits;
	int64_t float_pts =
		av_rescale_q(*dts, ifmt_ctx->streams[videoIndex]->time_base, tb) -
		av_rescale_q(start_time, { 1, AV_TIME_BASE }, tb);
	float_pts /= 1 << extra_bits;
	// avoid exact midoints to reduce the chance of rounding differences, this can be removed in case the fps code is changed to work with integers
	float_pts += FFSIGN(float_pts) * 1.0 / (1 << 17);

	*dts =
		av_rescale_q(*dts, ifmt_ctx->streams[videoIndex]->time_base, enc->time_base) -
		av_rescale_q(start_time, { 1, AV_TIME_BASE }, enc->time_base);
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
			AVCodec *pCodec = avcodec_find_encoder(in_stream->codec->codec_id);
			out_stream = avformat_new_stream(ofmt_ctx, pCodec);
			if (!out_stream) {
				printf("Failed allocating output stream\n");
				ret = AVERROR_UNKNOWN;
				return false;
			}
			encCtx = out_stream->codec;
			encCtx->codec_id = in_stream->codec->codec_id;
			encCtx->codec_type = in_stream->codec->codec_type;
			encCtx->pix_fmt = in_stream->codec->pix_fmt;
			encCtx->width = in_stream->codec->width;
			encCtx->height = in_stream->codec->height;
			encCtx->flags = in_stream->codec->flags;
			encCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
			av_opt_set(encCtx->priv_data, "tune", "zerolatency", 0);
			ofmt_ctx->streams[i]->avg_frame_rate = in_stream->avg_frame_rate;
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
			AVDictionary *param=0;
			if (encCtx->codec_id == AV_CODEC_ID_H264) {
				av_opt_set(&param, "preset", "slow", 0);
				//av_dict_set(&param, "profile", "main", 0);
			}
			//没有这句，导致得到的视频没有缩略图等信息
			ret = avcodec_open2(encCtx, pCodec, &param);
		}
		
	}
	/*
	//encCtx->extradata = new uint8_t[32];//给extradata成员参数分配内存
	//encCtx->extradata_size = 32;//extradata成员参数分配内存大小



	////给extradata成员参数设置值
	////00 00 00 01 
	//encCtx->extradata[0] = 0x00;
	//encCtx->extradata[1] = 0x00;
	//encCtx->extradata[2] = 0x00;
	//encCtx->extradata[3] = 0x01;

	////67 42 80 1e 
	//encCtx->extradata[4] = 0x67;
	//encCtx->extradata[5] = 0x42;
	//encCtx->extradata[6] = 0x80;
	//encCtx->extradata[7] = 0x1e;

	////88 8b 40 50 
	//encCtx->extradata[8] = 0x88;
	//encCtx->extradata[9] = 0x8b;
	//encCtx->extradata[10] = 0x40;
	//encCtx->extradata[11] = 0x50;

	////1e d0 80 00 
	//encCtx->extradata[12] = 0x1e;
	//encCtx->extradata[13] = 0xd0;
	//encCtx->extradata[14] = 0x80;
	//encCtx->extradata[15] = 0x00;

	////03 84 00 00 
	//encCtx->extradata[16] = 0x03;
	//encCtx->extradata[17] = 0x84;
	//encCtx->extradata[18] = 0x00;
	//encCtx->extradata[19] = 0x00;

	////af c8 02 00 
	//encCtx->extradata[20] = 0xaf;
	//encCtx->extradata[21] = 0xc8;
	//encCtx->extradata[22] = 0x02;
	//encCtx->extradata[23] = 0x00;

	////00 00 00 01 
	//encCtx->extradata[24] = 0x00;
	//encCtx->extradata[25] = 0x00;
	//encCtx->extradata[26] = 0x00;
	//encCtx->extradata[27] = 0x01;

	////68 ce 38 80
	//encCtx->extradata[28] = 0x68;
	//encCtx->extradata[29] = 0xce;
	//encCtx->extradata[30] = 0x38;
	//encCtx->extradata[31] = 0x80;*/
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0) {
			return false;
		}
	}
	/*
	out_stream->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		*/
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0){
		return false;
	}
	return true;
}