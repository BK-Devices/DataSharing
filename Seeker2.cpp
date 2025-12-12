void Seeker :: readEOFrames(void)
{
    using namespace std::chrono;

    Mat VideoFrame;
    string timestamp;
    UInt32_t SleepTime = 0;
    bool ReadStatus = false;
    struct _VideoData_ VideoData;

    VideoFrame.create(EO_pCodecCtx->height,
                      EO_pCodecCtx->width,
                      CV_8UC3);

    // Low-latency FFmpeg settings (important for RTSP)
    EO_pCodecCtx->flags |= AV_CODEC_FLAG_LOW_DELAY;
    EO_pCodecCtx->thread_type = FF_THREAD_SLICE;
    EO_pCodecCtx->thread_count = 2;

    const int retryLimit = 500;     // milliseconds
    int retryCounter = 0;

    // Double-buffer system
    Mat backBuffer;
    backBuffer.create(EO_pCodecCtx->height,
                      EO_pCodecCtx->width,
                      CV_8UC3);

    while (EOVideoReadFlag == true)
    {
        auto tStart = high_resolution_clock::now();
        ReadStatus = false;

        // Attempt non-blocking read
        int ret = av_read_frame(EO_pFormatCtx, EO_packet);

        if (ret == AVERROR(EAGAIN))
        {
            // No frame ready yet — yield CPU
            std::this_thread::sleep_for(1ms);
            continue;
        }

        if (ret < 0)
        {
            // RTSP dropped? Try reconnecting every 0.5s
            if (++retryCounter >= retryLimit)
            {
                std::cerr << "EO stream lost — reconnecting..." << std::endl;

                avformat_close_input(&EO_pFormatCtx);
                avformat_open_input(&EO_pFormatCtx,
                                    EO_VideoSrc.c_str(),
                                    NULL,
                                    &EO_avdic);

                retryCounter = 0;
            }

            std::this_thread::sleep_for(1ms);
            continue;
        }

        retryCounter = 0;  // reset on success

        if (EO_packet->stream_index == EO_videoStream)
        {
            EO_ret = avcodec_send_packet(EO_pCodecCtx, EO_packet);

            if (EO_ret < 0)
            {
                av_packet_unref(EO_packet);
                std::this_thread::sleep_for(1ms);
                continue;
            }

            while (true)
            {
                EO_ret = avcodec_receive_frame(EO_pCodecCtx, EO_pFrame);

                if (EO_ret == AVERROR(EAGAIN) || EO_ret == AVERROR_EOF)
                    break;

                if (EO_ret < 0)
                    break;

                // Convert frame into back-buffer
                sws_scale(EO_img_convert_ctx,
                          EO_pFrame->data,
                          EO_pFrame->linesize,
                          0,
                          EO_pCodecCtx->height,
                          EO_pFrameEO->data,
                          EO_pFrameEO->linesize);

                memcpy(backBuffer.data,
                       EO_pFrameEO->data[0],
                       EO_pCodecCtx->height * EO_pFrameEO->linesize[0]);

                ReadStatus = true;
            }
        }

        av_packet_unref(EO_packet);

        if (!ReadStatus)
        {
            std::this_thread::sleep_for(1ms);
            continue;
        }

        // Swap buffers (lock-free)
        backBuffer.copyTo(VideoFrame);

        // Timestamp overlay
        timestamp = getTimeStamp();
        putText(VideoFrame, timestamp, Point(20, 40),
                FONT_HERSHEY_SIMPLEX, 1, Scalar(255,255,255), 2);

        // -------- AI Frame Delivery --------
        if (AI_Type == 2)
        {
            VideoData.VideoFrame = VideoFrame;
            VideoData.VideoType = AI_Type;
            VideoData.FrameTime = steady_clock::now();

            if (CameraData.FrameFlag == true)
            {
                // AI is too slow — drop old frame
                cout << "Dropping stale frames..." << endl;
            }

            CameraData.VideoFrame.release();
            CameraData = VideoData;
            CameraData.FrameFlag = true;
        }

        // ---------- Adaptive CPU sleep ----------
        auto tEnd = high_resolution_clock::now();
        auto readTime = duration_cast<milliseconds>(tEnd - tStart).count();

        SleepTime = (FrameDelay > readTime) ? (FrameDelay - readTime) : 0;

        // Do NOT sleep for RTSP/HTTP — they have real-time constraints
        if (!(EO_VideoSrc.find("/dev/video") == 0 ||
              EO_VideoSrc.find("rtsp://") == 0 ||
              EO_VideoSrc.find("http://") == 0 ||
              EO_VideoSrc.find("https://") == 0))
        {
            if (SleepTime > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(SleepTime));
        }
        else
        {
            // Always yield small amount for CPU fairness
            std::this_thread::sleep_for(1ms);
        }
    }
}

