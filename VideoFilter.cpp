#include "VideoFilter.h"

void logging(const char *fmt, ...)
{
  va_list args;
  fprintf(stderr, "LOG: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

//VideoFilter::VideoFilter(const char *fname, FrameFilter *ff, int n_tiles) : frame_filter(ff), num_tiles(n_tiles)
VideoFilter::VideoFilter(const char *fname, const char *out_dir_, FrameFilter *ff_, int n_tiles) :
  out_dir(out_dir_), ff(ff_), num_tiles(n_tiles)
{
  if (num_tiles < 1 || num_tiles > 16) {
    logging("ERROR number of tiles must be 1 <= # <= 16");
    return;
  }
  logging("initializing all the containers, codecs and protocols.");

  pFormatContext = avformat_alloc_context();
  if (!pFormatContext)
  {
    logging("ERROR could not allocate memory for Format Context");
    return;
  }

  logging("opening the input file (%s) and loading format (container) header", fname);

  av_register_all();
  avcodec_register_all();

  int ret = avformat_open_input(&pFormatContext, fname, NULL, NULL);
  if (ret < 0)
  {
    logging("ERROR could not open the file");
    return;
  }

  logging("format %s, duration %lld us, bit_rate %lld", pFormatContext->iformat->name, pFormatContext->duration, pFormatContext->bit_rate);

  logging("finding stream info from format");

  if (avformat_find_stream_info(pFormatContext, NULL) < 0)
  {
    logging("ERROR could not get the stream info");
    return;
  }

  AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;

  for (int i = 0; i < pFormatContext->nb_streams; i++)
  {
    AVCodecParameters *pLocalCodecParameters = NULL;
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
    logging("AVStream->time_base before open coded %d/%d", pFormatContext->streams[i]->time_base.num, pFormatContext->streams[i]->time_base.den);
    logging("AVStream->r_frame_rate before open coded %d/%d", pFormatContext->streams[i]->r_frame_rate.num, pFormatContext->streams[i]->r_frame_rate.den);
    logging("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
    logging("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

    logging("finding the proper decoder (CODEC)");

    AVCodec *pLocalCodec = NULL;

    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec == NULL)
    {
      logging("ERROR unsupported codec!");
      return;
    }

    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      video_stream_index = i;
      pCodec = pLocalCodec;
      pCodecParameters = pLocalCodecParameters;

      logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);

      frame_width = pCodecParameters->width; 
      frame_height = pCodecParameters->height;
      tile_width = frame_width / sqrt(num_tiles);
      tile_height = frame_height / sqrt(num_tiles);
      if (tile_width % 2) tile_width--;
      if (tile_height % 2) tile_height--;
    }
    else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      logging("Audio Codec: %d channels, sample rate %d", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
    }

    logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pCodecParameters->bit_rate);
  }

  dec_ctx = avcodec_alloc_context3(pCodec);

  if (!dec_ctx)
  {
    logging("failed to allocated memory for AVCodecContext");
    return;
  }
  if (avcodec_parameters_to_context(dec_ctx, pCodecParameters) < 0)
  {
    logging("failed to copy codec params to codec context");
    return;
  }

  if (avcodec_open2(dec_ctx, pCodec, NULL) < 0)
  {
    logging("failed to open codec through avcodec_open2");
    return;
  }

  if (out_dir != NULL) {
    AVCodec *enc_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    tiles_ctx = new AVCodecContext*[num_tiles];
    for (int i = 0; i < num_tiles; i++) {
      tiles_ctx[i] = avcodec_alloc_context3(enc_codec);
      tiles_ctx[i]->bit_rate = dec_ctx->bit_rate;
      tiles_ctx[i]->width = tile_width; 
      tiles_ctx[i]->height = tile_height;
      tiles_ctx[i]->time_base = (AVRational){1, 25};
      tiles_ctx[i]->framerate = (AVRational){25, 1};
      tiles_ctx[i]->gop_size = dec_ctx->gop_size;
      tiles_ctx[i]->max_b_frames = 1;
      tiles_ctx[i]->pix_fmt = dec_ctx->pix_fmt;

      av_opt_set(tiles_ctx[i]->priv_data, "preset", "slow", 0);

      if (avcodec_open2(tiles_ctx[i], enc_codec, NULL) < 0)
      {
        logging("failed to open codec through avcodec_open2");
        return;
      }
    }
  }

  yuv_ctx = sws_getContext(
      frame_width,
      frame_height,
      AV_PIX_FMT_YUV420P,
      frame_width,
      frame_height,
      AV_PIX_FMT_RGB24,
      SWS_BICUBIC,
      NULL, NULL, NULL);

  rgb_ctx = sws_getContext(
      frame_width,
      frame_height,
      AV_PIX_FMT_RGB24,
      frame_width,
      frame_height,
      AV_PIX_FMT_YUV420P,
      SWS_BICUBIC,
      NULL, NULL, NULL);
}

VideoFilter::~VideoFilter()
{
  logging("releasing all the resources");
  avformat_close_input(&pFormatContext);
  avformat_free_context(pFormatContext);
  avcodec_free_context(&dec_ctx);
  if (out_dir != NULL) {
    for (int i = 0; i < num_tiles; i++) {
      avcodec_free_context(&tiles_ctx[i]);
    }
  }
  sws_freeContext(yuv_ctx);
  sws_freeContext(rgb_ctx);
  delete [] tiles_ctx;
}

int VideoFilter::yuv_to_rgb(uint8_t **yuv, uint8_t **rgb)
{
  int rgb_stride[1] = {frame_width * 3};
  int yuv_stride[8] = {frame_width, frame_width / 2, frame_width / 2, 0, 0, 0, 0, 0};

  int ret = sws_scale(yuv_ctx,
                      yuv,
                      yuv_stride,
                      0,
                      frame_height,
                      rgb,
                      rgb_stride);

  return ret;
}

int VideoFilter::rgb_to_yuv(uint8_t **rgb, uint8_t **yuv)
{
  int rgb_stride[1] = {frame_width * 3};
  int yuv_stride[8] = {frame_width, frame_width / 2, frame_width / 2, 0, 0, 0, 0, 0};

  int ret = sws_scale(rgb_ctx,
                      rgb,
                      rgb_stride,
                      0,
                      frame_height,
                      yuv,
                      yuv_stride);

  return ret;
}

int VideoFilter::encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *f)
{
  int ret;
  if (frame)
    printf("Send frame %ld\n", frame->pts);

  ret = avcodec_send_frame(enc_ctx, frame);
  if (ret < 0)
  {
    logging("Error sending a frame to the encoder");
    return ret;
  }

  while (ret >= 0)
  {
    ret = avcodec_receive_packet(enc_ctx, pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      return 0;
    else if (ret < 0)
    {
      logging("Error during encoding\n");
      return ret;
    }
    printf("Write packet %ld (size=%5d)\n", pkt->pts, pkt->size);
    fwrite(pkt->data, 1, pkt->size, f);
    av_packet_unref(pkt);
  }
  return 0;
}

int VideoFilter::decode_packet(AVPacket *pPacket, AVFrame *outFrame, uint8_t **rgb, Halide::Runtime::Buffer<uint8_t> &out_buf)
{
  int response = avcodec_send_packet(dec_ctx, pPacket);
  if (response < 0)
  {
    logging("Error while sending a packet to the decoder");
    return response;
  }

  int sizeofFrame = frame_width * frame_height * 3;
  while (response >= 0)
  {
    response = avcodec_receive_frame(dec_ctx, inFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
    {
      break;
    }
    else if (response < 0)
    {
      logging("Error while receiving a frame from the decoder");
      return response;
    }

    if (response >= 0)
    {
      logging(
          "Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
          dec_ctx->frame_number,
          av_get_picture_type_char(inFrame->pict_type),
          inFrame->pkt_size,
          inFrame->pts,
          inFrame->key_frame,
          inFrame->coded_picture_number);

      int ret = yuv_to_rgb(inFrame->data, rgb);
      if (ret < 0)
        return -1;
      ret = apply_filter(rgb, out_buf);
      if (ret < 0)
        return -1;
      uint8_t *out[1] = {out_buf.data()};

      ret = rgb_to_yuv(out, wholeFrame->data);
      if (ret < 0)
        return -1;

      if (out_dir != NULL) {
        int num_x_tiles = frame_width / tile_width;
        int num_y_tiles = frame_height / tile_height;
        for (int i = 0; i < num_tiles; i++) {
          outFrame->width = frame_width;
          outFrame->height = frame_height;
          outFrame->data[0] = wholeFrame->data[0];
          outFrame->data[1] = wholeFrame->data[1];
          outFrame->data[2] = wholeFrame->data[2];

          ret = av_frame_make_writable(outFrame);
          if (ret < 0)
          {
            logging("Failed to make frame writable");
            return -1;
          }

          int crop_top = i / num_x_tiles * tile_height;
          int crop_bottom = std::max(frame_height - crop_top - tile_height, 0);
          int crop_left = i % num_x_tiles * tile_width;
          int crop_right = std::max(frame_width - crop_left - tile_width, 0);
          outFrame->crop_top = crop_top;
          outFrame->crop_left = crop_left;
          outFrame->crop_right = crop_right;
          outFrame->crop_bottom = crop_bottom;
          av_frame_apply_cropping(outFrame, 0);

          outFrame->pts = tiles_ctx[i]->frame_number;
          ret = encode(tiles_ctx[i], outFrame, pPacket, out_file[i]);
          if (ret < 0)
            return -1;
        }
      }

      av_frame_unref(inFrame);
    }
  }
  av_packet_unref(pPacket);
  return 0;
}

int VideoFilter::filter(int n_frames)
{
  if (!dec_ctx) {
    logging("Error, decoding context not found");
    return -1;
  }
  uint8_t endcode[] = {0, 0, 1, 0xb7};

  inFrame = av_frame_alloc();
  if (!inFrame)
  {
    logging("failed to allocate memory for AVFrame");
    return -1;
  }

  wholeFrame = av_frame_alloc();
  if (!wholeFrame)
  {
    logging("failed to allocate memory for AVFrame");
    return -1;
  }

  wholeFrame->format = AV_PIX_FMT_YUV420P;
  wholeFrame->width = frame_width;
  wholeFrame->height = frame_height;
  if (av_frame_get_buffer(wholeFrame, 32) < 0)
  {
    logging("Failed to allocate memory for out frame");
    return -1;
  }

  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket)
  {
    logging("failed to allocate memory for AVPacket");
    return -1;
  }
  if (out_dir != NULL) {
    out_file = new FILE*[num_tiles];
    for (int i = 0; i < num_tiles; i++) {
      char f_name[50];
      sprintf(f_name, "%s/tile_%d.mp4", out_dir, i+1);
      out_file[i] = fopen(f_name, "wb");
      if (!out_file[i])
      {
        logging("Could not open file for output");
        return -1;
      }
    }
  }

  int sizeofFrame = frame_width * frame_height * 3;
  uint8_t *rgb_buf = new uint8_t[sizeofFrame];
  uint8_t *rgb[1] = {rgb_buf};

  auto out_buf = Halide::Runtime::Buffer<uint8_t>::make_interleaved(frame_width, frame_height, 3);

  AVFrame *outFrame = av_frame_clone(wholeFrame);

  //n_frames += start_frame;

  while (n_frames-- && av_read_frame(pFormatContext, pPacket) >= 0)
  {
    if (pPacket->stream_index == video_stream_index)
    {
      logging("AVPacket->pts %" PRId64, pPacket->pts);
      int ret = decode_packet(pPacket, outFrame, rgb, out_buf);
      if (ret < 0)
        return -1;
    }
  }

  if (out_dir != NULL) {
    for (int i = 0; i < num_tiles; i++) {
      /* flush the encoder */
      encode(tiles_ctx[i], NULL, pPacket, out_file[i]);
      fwrite(endcode, 1, sizeof(endcode), out_file[i]);
      fclose(out_file[i]);
    }
    delete[] out_file;
  }

  delete[] rgb_buf;

  av_packet_free(&pPacket);
  av_frame_free(&wholeFrame);
  av_frame_free(&inFrame);
  return 0;
}

int VideoFilter::apply_filter(uint8_t **rgb, Halide::Runtime::Buffer<uint8_t> &output)
{
  static const halide_dimension_t dims[3] = {
      {0, frame_width, 3, 0},
      {0, frame_height, frame_width * 3, 0},
      {0, 3, 1, 0}};

  Halide::Runtime::Buffer<uint8_t>
      input(rgb[0], 3, dims);

  char descript_name[256];
  sprintf(descript_name, "%s/descriptors/frame_%d_sift.txt", out_dir, dec_ctx->frame_number); 

  int ret = ff->filter(input, output);
  if (ret < 0) {
    printf("Frame filter returned an error.\n");
    return -1;
  }

  return 0;
}

