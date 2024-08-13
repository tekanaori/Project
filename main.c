/**
 * "../../tcc/tcc" -run -I../include -I../extlibs/include -I../extlibs/include/winapi -I../extlibs/include/libxml2 -L. -lavcodec-61 -lavformat-61 -lavutil-59 -lswresample-5 -lswscale-8 -lglfw3 -lcurl-x64 -lxml2-2 main.c
 */
#ifndef PROJECT_H
#define PROJECT_H

#ifdef __cpulsplus
extern "C" {
#endif /* __cpulsplus */

#if 1 /* Print text */

#define PRINT_TEXT_CLEAR "\033[H\033[J"
#define PRINT_TEXT_COLOR_RED "\x1B[31m"
#define PRINT_TEXT_COLOR_GREEN "\x1B[32m"
#define PRINT_TEXT_COLOR_YELOW "\x1B[33m"
#define PRINT_TEXT_COLOR_BLUE "\x1B[34m"
#define PRINT_TEXT_COLOR_MAGENTA "\x1B[35m"
#define PRINT_TEXT_COLOR_CYAN "\x1B[36m"
#define PRINT_TEXT_COLOR_WHITE "\x1B[37m"
#define PRINT_TEXT(text) printf(PRINT_TEXT_COLOR_WHITE "%s:" PRINT_TEXT_COLOR_MAGENTA "%i" PRINT_TEXT_COLOR_WHITE ": in '" PRINT_TEXT_COLOR_BLUE "%s" PRINT_TEXT_COLOR_WHITE "' function: " PRINT_TEXT_COLOR_RED "%s" PRINT_TEXT_COLOR_WHITE "\n", __FILE__, __LINE__, __FUNCTION__, text)

#endif /* Print text */

#if 1 /* File load */

#include <fcntl.h>
#include <errno.h>

int file_load(const char* file_name, void** out_data, size_t* out_size) {
    int result;
    int file_descriptor;

    if ((file_descriptor = open(file_name, O_RDONLY | O_BINARY)) > 0) {
        if ((out_size[0] = filelength(file_descriptor)) > 0) {
            if (read(file_descriptor, out_data[0], out_size[0]) > 0) {
                result = 0;
            }
            else {
                result = errno;
            }
        }
        else {
            result = errno;
        }

        close(file_descriptor);
    }
    else {
        result = errno;
    }

    return result;
}

#endif /* File load */

#if 1 /* FFmpeg */

#include <libavformat/avformat.h>

int ffmpeg_format_context_alloc(AVFormatContext** out_format_context, const char* url, enum AVMediaType media_type, int* out_stream_index) {
    int result;

    if ((result = avformat_open_input(out_format_context, url, NULL, NULL)) == 0) {
        if ((result = avformat_find_stream_info(out_format_context[0], NULL)) >= 0) {
            if ((result = av_find_best_stream(out_format_context[0], media_type, -1, -1, NULL, 0)) >= 0) {
                out_stream_index[0] = result;
                return 0;
            }
        }

        avformat_close_input(out_format_context);
    }

    return result;
}

#include <libavcodec/avcodec.h>

int ffmpeg_codec_context_alloc(AVCodecContext** out_codec_context, const AVCodecParameters* codec_parameters, AVDictionary* options) {
    int result;

    if ((out_codec_context[0] = avcodec_alloc_context3(NULL)) != NULL) {
        if ((result = avcodec_parameters_to_context(out_codec_context[0], codec_parameters)) >= 0) {
            if ((result = avcodec_open2(out_codec_context[0], avcodec_find_decoder(out_codec_context[0]->codec_id), &options)) == 0) {
                return result;
            }
        }

        avcodec_free_context(out_codec_context);
    }
    else {
        result = AVERROR(ENOMEM);
    }

    return result;
}

#include <libswscale/swscale.h>

int ffmpeg_video_context_alloc(struct SwsContext** out_sws_context, AVCodecContext* codec_context, enum AVPixelFormat pixel_format) {
    return (out_sws_context[0] = sws_getCachedContext(out_sws_context[0], codec_context->width, codec_context->height, codec_context->pix_fmt, codec_context->width, codec_context->height, pixel_format, 0, NULL, NULL, NULL)) != NULL ? 0 : AVERROR(ENOMEM);
}

#include <libswresample/swresample.h>

int ffmpeg_audio_context_alloc(struct SwrContext** out_swr_context, AVCodecContext* codec_context, enum AVSampleFormat sample_format) {
    int result;

    if ((result = swr_alloc_set_opts2(out_swr_context, &codec_context->ch_layout, sample_format, codec_context->sample_rate, &codec_context->ch_layout, codec_context->sample_fmt, codec_context->sample_rate, 0, NULL)) == 0) {
        if ((result = swr_init(out_swr_context[0])) == 0) {
            return result;
        }

        swr_free(out_swr_context);
    }

    return result;
}

#endif /* FFmpeg */

#if 1 /* FFmpeg additional */

int ffmpeg_get_next_frame(AVFormatContext* format_context, AVPacket* packet, int stream_index, AVCodecContext* codec_context, AVFrame* frame) {
    int result;

    while ((result = av_read_frame(format_context, packet)) == 0) {
        if (packet->stream_index == stream_index) {
            if ((result = avcodec_send_packet(codec_context, packet)) == 0) {
                result = avcodec_receive_frame(codec_context, frame);

                if (result != AVERROR(EAGAIN) && result != AVERROR_EOF) {
                    av_packet_unref(packet);
                    return result;
                }
            }
        }

        av_packet_unref(packet);
    }

    return result;
}

int ffmpeg_seek(const int* run, AVFormatContext* format_context, double seconds, AVPacket* packet, int stream_index, AVCodecContext* codec_context, AVFrame* frame) {
    int result;

    if ((result = avformat_seek_file(format_context, -1, INT64_MIN, AV_TIME_BASE * seconds, INT64_MAX, 0)) >= 0) {
        if (av_q2d(format_context->streams[stream_index]->time_base) * frame->pts < seconds) {
            while (*run == 1 && result == 0 && av_q2d(format_context->streams[stream_index]->time_base) * frame->pts < seconds) {
                result = ffmpeg_get_next_frame(format_context, packet, stream_index, codec_context, frame);
            }
        }
        else if (av_q2d(format_context->streams[stream_index]->time_base) * frame->pts > seconds) {
            while (*run == 1 && result == 0 && av_q2d(format_context->streams[stream_index]->time_base) * frame->pts > seconds) {
                result = ffmpeg_get_next_frame(format_context, packet, stream_index, codec_context, frame);
            }
            while (*run == 1 && result == 0 && av_q2d(format_context->streams[stream_index]->time_base) * frame->pts < seconds) {
                result = ffmpeg_get_next_frame(format_context, packet, stream_index, codec_context, frame);
            }
        }
    }

    return result;
}

#include <libavutil/imgutils.h>

int ffmpeg_from_memory_alloc(AVFormatContext** out_format_context, void* data, size_t size, enum AVMediaType media_type, int* out_stream_index, AVCodecContext** out_codec_context, AVDictionary* options) {
    unsigned char* buffer;
    int result;

    if ((buffer = av_malloc(size)) != NULL) {
        if ((out_format_context[0] = avformat_alloc_context()) != NULL) {
            out_format_context[0]->flags = AVFMT_FLAG_CUSTOM_IO;

            if (memcpy(buffer, data, size) != NULL) {
                if ((out_format_context[0]->pb = avio_alloc_context(buffer, size, 0, data, NULL, NULL, NULL)) != NULL) {
                    if ((result = ffmpeg_format_context_alloc(out_format_context, NULL, media_type, out_stream_index)) == 0) {
                        if ((result = ffmpeg_codec_context_alloc(out_codec_context, out_format_context[0]->streams[out_stream_index[0]]->codecpar, options)) == 0) {
                            return result;
                        }
                    }
                }
                else {
                    result = AVERROR(ENOMEM);
                }
            }
            else {
                result = AVERROR(ENOMEM);
            }

            avformat_close_input(out_format_context);
        }
        else {
            result = AVERROR(ENOMEM);
        }

        av_freep(&buffer);
    }
    else {
        result = AVERROR(ENOMEM);
    }

    return result;
}

int ffmpeg_image_parse(AVFormatContext* format_context, AVCodecContext* codec_context, int stream_index, enum AVPixelFormat pixel_format, uint8_t** out_pixels, int* out_width, int* out_height) {
    struct SwsContext* sws_context = NULL;
    AVPacket* packet;
    AVFrame* frame;
    uint8_t* pixels[4] = { NULL, NULL, NULL, NULL };
    int pitch[4] = { 0, 0, 0, 0 };
    int result;

    if ((result = ffmpeg_video_context_alloc(&sws_context, codec_context, pixel_format)) == 0) {
        if ((packet = av_packet_alloc()) != NULL) {
            if ((frame = av_frame_alloc()) != NULL) {
                if ((pixels[0] = av_malloc(av_image_get_buffer_size(pixel_format, codec_context->width, codec_context->height, 1))) != NULL) {
                    pitch[0] = codec_context->width * 4;

                    if ((result = ffmpeg_get_next_frame(format_context, packet, stream_index, codec_context, frame)) == 0) {
                        if ((result = sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height, pixels, pitch)) >= 0) {
                            out_pixels[0] = pixels[0];
                            out_width[0] = frame->width;
                            out_height[0] = frame->height;
                            result = 0;
                        }
                    }
                }
                else {
                    result = AVERROR(ENOMEM);
                }

                av_frame_free(&frame);
            }
            else {
                result = AVERROR(ENOMEM);
            }

            av_packet_free(&packet);
        }
        else {
            result = AVERROR(ENOMEM);
        }

        sws_freeContext(sws_context);
    }

    return result;
}

int ffmpeg_image_load_from_url(const char* url, enum AVPixelFormat pixel_format, uint8_t** out_pixels, int* out_width, int* out_height) {
    AVFormatContext* format_context = NULL;
    AVCodecContext* codec_context = NULL;
    int stream_index;
    int result;

    if ((result = ffmpeg_format_context_alloc(&format_context, url, AVMEDIA_TYPE_VIDEO, &stream_index)) == 0) {
        if ((result = ffmpeg_codec_context_alloc(&codec_context, format_context->streams[stream_index]->codecpar, NULL)) == 0) {
            result = ffmpeg_image_parse(format_context, codec_context, stream_index, pixel_format, out_pixels, out_width, out_height);
            avcodec_free_context(&codec_context);
        }

        avformat_close_input(&format_context);
    }

    return result;
}

int ffmpeg_image_load_from_memory(void* data, size_t size, enum AVPixelFormat pixel_format, uint8_t** out_pixels, int* out_width, int* out_height) {
    AVFormatContext* format_context = NULL;
    AVCodecContext* codec_context = NULL;
    int stream_index;
    int result;

    if ((result = ffmpeg_from_memory_alloc(&format_context, data, size, AVMEDIA_TYPE_VIDEO, &stream_index, &codec_context, NULL)) == 0) {
        result = ffmpeg_image_parse(format_context, codec_context, stream_index, pixel_format, out_pixels, out_width, out_height);
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
    }

    return result;
}

#endif /* FFmpeg additional */

#if 1 /* FFmpeg video */

typedef struct ffmpeg_video_data {
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    struct SwsContext* sws_context;
    AVPacket* packet;
    AVFrame* frame;
    int stream_index;
    int result;
} ffmpeg_video_data_t;

ffmpeg_video_data_t ffmpeg_video_data_alloc(const char* url, enum AVPixelFormat pixel_format) {
    ffmpeg_video_data_t ffmpeg_video_data = {
        .format_context = NULL,
        .codec_context = NULL,
        .sws_context = NULL,
        .packet = NULL,
        .frame = NULL,
        .stream_index = AVERROR_STREAM_NOT_FOUND,
        .result = 0
    };

    if ((ffmpeg_video_data.result = ffmpeg_format_context_alloc(&ffmpeg_video_data.format_context, url, AVMEDIA_TYPE_VIDEO, &ffmpeg_video_data.stream_index)) == 0) {
        if ((ffmpeg_video_data.result = ffmpeg_codec_context_alloc(&ffmpeg_video_data.codec_context, ffmpeg_video_data.format_context->streams[ffmpeg_video_data.stream_index]->codecpar, NULL)) == 0) {
            if ((ffmpeg_video_data.result = ffmpeg_video_context_alloc(&ffmpeg_video_data.sws_context, ffmpeg_video_data.codec_context, pixel_format)) == 0) {
                if ((ffmpeg_video_data.packet = av_packet_alloc()) != NULL) {
                    if ((ffmpeg_video_data.frame = av_frame_alloc()) != NULL) {
                        return ffmpeg_video_data;

                        av_frame_free(&ffmpeg_video_data.frame);
                    }
                    else {
                        ffmpeg_video_data.result = AVERROR(ENOMEM);
                    }

                    av_packet_free(&ffmpeg_video_data.packet);
                }
                else {
                    ffmpeg_video_data.result = AVERROR(ENOMEM);
                }

                sws_freeContext(ffmpeg_video_data.sws_context);
                ffmpeg_video_data.sws_context = NULL;
            }

            avcodec_free_context(&ffmpeg_video_data.codec_context);
        }

        avformat_close_input(&ffmpeg_video_data.format_context);
    }

    return ffmpeg_video_data;
}

void ffmpeg_video_data_free(ffmpeg_video_data_t* ffmpeg_video_data) {
    ffmpeg_video_data->result = 0;
    ffmpeg_video_data->stream_index = AVERROR_STREAM_NOT_FOUND;
    av_frame_free(&ffmpeg_video_data->frame);
    av_packet_free(&ffmpeg_video_data->packet);
    sws_freeContext(ffmpeg_video_data->sws_context);
    ffmpeg_video_data->sws_context = NULL;
    avcodec_free_context(&ffmpeg_video_data->codec_context);
    avformat_close_input(&ffmpeg_video_data->format_context);
}

#endif /* FFmpeg video */

#if 1 /* FFmpeg audio */

typedef struct ffmpeg_audio_data {
    AVFormatContext* format_context;
    AVCodecContext* codec_context;
    struct SwrContext* swr_context;
    AVPacket* packet;
    AVFrame* frame;
    int stream_index;
    int result;
} ffmpeg_audio_data_t;

ffmpeg_audio_data_t ffmpeg_audio_data_alloc(const char* url, enum AVSampleFormat sample_format) {
    ffmpeg_audio_data_t ffmpeg_audio_data = {
        .format_context = NULL,
        .codec_context = NULL,
        .swr_context = NULL,
        .packet = NULL,
        .frame = NULL,
        .stream_index = AVERROR_STREAM_NOT_FOUND,
        .result = 0
    };

    if ((ffmpeg_audio_data.result = ffmpeg_format_context_alloc(&ffmpeg_audio_data.format_context, url, AVMEDIA_TYPE_AUDIO, &ffmpeg_audio_data.stream_index)) == 0) {
        if ((ffmpeg_audio_data.result = ffmpeg_codec_context_alloc(&ffmpeg_audio_data.codec_context, ffmpeg_audio_data.format_context->streams[ffmpeg_audio_data.stream_index]->codecpar, NULL)) == 0) {
            if ((ffmpeg_audio_data.result = ffmpeg_audio_context_alloc(&ffmpeg_audio_data.swr_context, ffmpeg_audio_data.codec_context, sample_format)) == 0) {
                if ((ffmpeg_audio_data.packet = av_packet_alloc()) != NULL) {
                    if ((ffmpeg_audio_data.frame = av_frame_alloc()) != NULL) {
                        return ffmpeg_audio_data;

                        av_frame_free(&ffmpeg_audio_data.frame);
                    }
                    else {
                        ffmpeg_audio_data.result = AVERROR(ENOMEM);
                    }

                    av_packet_free(&ffmpeg_audio_data.packet);
                }
                else {
                    ffmpeg_audio_data.result = AVERROR(ENOMEM);
                }

                swr_free(&ffmpeg_audio_data.swr_context);
            }

            avcodec_free_context(&ffmpeg_audio_data.codec_context);
        }

        avformat_close_input(&ffmpeg_audio_data.format_context);
    }

    return ffmpeg_audio_data;
}

void ffmpeg_audio_data_free(ffmpeg_audio_data_t* ffmpeg_audio_data) {
    ffmpeg_audio_data->result = 0;
    ffmpeg_audio_data->stream_index = AVERROR_STREAM_NOT_FOUND;
    av_frame_free(&ffmpeg_audio_data->frame);
    av_packet_free(&ffmpeg_audio_data->packet);
    swr_free(&ffmpeg_audio_data->swr_context);
    avcodec_free_context(&ffmpeg_audio_data->codec_context);
    avformat_close_input(&ffmpeg_audio_data->format_context);
}

#endif /* FFmpeg audio */

#if 1 /* stb_image */

#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#endif /* stb_image */

#if 1 /* cglm */

#include <cglm/cglm.h>

void cglm_view_rotate(mat4 view, float value, vec3 axis) {
    mat4 rotation_matrix;

    glm_mat4_copy(view, rotation_matrix);
    glm_rotate_make(view, value, axis);
    glm_mul(view, rotation_matrix, view);
}

void cglm_view_move_up(mat4 view, float value) {
    float yaw = -90.0f;
    float pitch = 1.0f;
    vec3 front = {
        cosf(glm_rad(yaw)) * cosf(glm_rad(pitch)),
        sinf(glm_rad(pitch)),
        sinf(glm_rad(yaw)) * cosf(glm_rad(pitch))
    };

    glm_vec3_normalize(front);

    view[3][0] += glm_rad(front[1]) * value;
    view[3][1] += glm_rad(front[2]) * value;
    view[3][2] += glm_rad(front[0]) * value;
}

void cglm_view_move(vec3 axis, mat4 view, float value) {
    float yaw = -90.0f;
    float pitch = 1.0f;
    vec3 front = {
        cosf(glm_rad(yaw)) * cosf(glm_rad(pitch)),
        sinf(glm_rad(pitch)),
        sinf(glm_rad(yaw)) * cosf(glm_rad(pitch))
    };

    glm_vec3_normalize(front);
    glm_vec3_cross(front, axis, front);
    glm_vec3_normalize(front);

    view[3][0] += glm_rad(front[1]) * value;
    view[3][1] += glm_rad(front[2]) * value;
    view[3][2] += glm_rad(front[0]) * value;
}

#endif /* cglm */

#if 1 /* miniaudio */

#ifndef CP_UTF8
    #define CP_UTF8 65001 /* miniaudio.h:21553: error: 'CP_UTF8' undeclared */
#endif /* CP_UTF8 */

#ifdef __APPLE__
    #define MA_NO_RUNTIME_LINKING
#endif /* __APPLE__ */

// #define MA_NO_DECODING
// #define MA_NO_ENCODING
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#endif /* miniaudio */

#if 1 /* miniaudio & FFmpeg */

#include <libavutil/time.h>

void ma_data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
    ffmpeg_audio_data_t* ffmpeg_audio_data = device->pUserData;

    if ((ffmpeg_audio_data->result = ffmpeg_get_next_frame(ffmpeg_audio_data->format_context, ffmpeg_audio_data->packet, ffmpeg_audio_data->stream_index, ffmpeg_audio_data->codec_context, ffmpeg_audio_data->frame)) == 0) {
        if ((ffmpeg_audio_data->result = swr_convert(ffmpeg_audio_data->swr_context, &output, frame_count, ffmpeg_audio_data->frame->data, ffmpeg_audio_data->frame->nb_samples)) >= 0) {
            ffmpeg_audio_data->result = 0;
        }
    }
}

void ma_ffmpeg_start(const char* url) {
    ffmpeg_audio_data_t ffmpeg_audio_data = ffmpeg_audio_data_alloc(url, AV_SAMPLE_FMT_S16);

    if (ffmpeg_audio_data.result == 0) {
        ma_result result;
        ma_device_config device_config;
        ma_device device;

        device_config = ma_device_config_init(ma_device_type_playback);
        device_config.sampleRate = ffmpeg_audio_data.codec_context->sample_rate;
        device_config.periodSizeInFrames = ffmpeg_audio_data.codec_context->frame_size;
        device_config.dataCallback = ma_data_callback;
        device_config.pUserData = &ffmpeg_audio_data;
        device_config.playback.format = ma_format_s16;
        device_config.playback.channels = ffmpeg_audio_data.codec_context->ch_layout.nb_channels;

        if ((result = ma_device_init(NULL, &device_config, &device)) == MA_SUCCESS) {
            if ((result = ma_device_start(&device)) == MA_SUCCESS) {
                while (ffmpeg_audio_data.result != AVERROR_EOF) {
                    av_usleep(10000);
                }
            }

            ma_device_uninit(&device);
        }

        ffmpeg_audio_data_free(&ffmpeg_audio_data);
    }
}

#endif /* miniaudio & FFmpeg */

#if 1 /* curl */

#define __MINGW32__ /* tcc: curl/system.h:464: error: ';' expected (got "curl_socklen_t") */
    #include <curl/curl.h>
#undef __MINGW32__

size_t curl_write_function_callback(void* data, size_t size, size_t nmemb, void** out_data) {
    void* reallocated_data;
    size_t data_size;

    data_size = out_data[0] ? *(size_t*)out_data[0] : 0;

    if ((reallocated_data = realloc(out_data[0], size * nmemb + sizeof(size_t) + data_size)) != NULL) {
        out_data[0] = reallocated_data;
        memcpy(out_data[0] + sizeof(size_t) + data_size, data, size * nmemb);
        *(size_t*)out_data[0] = size * nmemb + data_size;
    }
    else {
        return 0;
    }

    return size * nmemb;
}

/**
 * @note First "sizeof(size_t)" in "out_data" - data size.
 */
CURLcode curl_get_url_data(const char* url, void** out_data, long timeout_ms) {
    CURLcode curl_code;

    if ((curl_code = curl_global_init(CURL_GLOBAL_ALL)) == CURLE_OK) {
        CURL* curl;

        if ((curl = curl_easy_init()) != NULL) {
            out_data[0] = NULL; /* This function should not do this. */

            if (
                (curl_code = curl_easy_setopt(curl, CURLOPT_URL, url)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_function_callback)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, out_data)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1)) == CURLE_OK &&
                (curl_code = curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/117.0")) == CURLE_OK
            ) {
                curl_code = curl_easy_perform(curl);
            }

            curl_easy_cleanup(curl);
        }
        else {
            curl_code = CURLE_FAILED_INIT;
        }

        curl_global_cleanup();
    }

    return curl_code;
}

#endif /* curl */

#if 1 /* libxml2 */

#include <libxml/HTMLparser.h>

xmlNode* xml_node_find(xmlNode* node, const xmlChar* node_name, const xmlChar* prop_name, const xmlChar* prop_value, const xmlChar* node_content) {
    while (node != NULL) {
        if (node->type == XML_ELEMENT_NODE) {
            int is_this = 1;

            if (node_name != NULL) {
                if (xmlStrcmp(node->name, node_name) != 0) {
                    is_this = 0;
                }
            }

            xmlChar* prop_data = NULL;

            if (prop_name != NULL && is_this == 1) {
                if ((prop_data = xmlGetProp(node, prop_name)) == NULL) { /* Do not forget to free up memory after xmlGetProp. */
                    is_this = 0;
                }
            }

            if (prop_value != NULL && is_this == 1) {
                if (prop_data != NULL) {
                    if (xmlStrcmp(prop_data, prop_value) != 0) {
                        is_this = 0;
                    }
                }
            }

            if (prop_data != NULL) {
                xmlFree(prop_data);
            }

            if (node_content != NULL && is_this == 1) {
                xmlChar* content;

                is_this = 0;

                if ((content = xmlNodeGetContent(node)) != NULL) { /* Do not forget to free up memory after xmlNodeGetContent. */
                    if (xmlStrstr(content, node_content) != NULL) {
                        is_this = 1;
                    }

                    xmlFree(content);
                }
            }

            if (is_this == 1) {
                return node;
            }
        }

        xmlNode* result;

        if ((result = xml_node_find(node->children, node_name, prop_name, prop_value, node_content)) != NULL) {
            return result;
        }

        node = node->next;
    }

    return NULL;
}

#endif /* libxml2 */

#if 1 /* Random 18+ video/image */

htmlDocPtr xml_get_url_data(const char* url, long timeout_ms) {
    void* url_data;
    CURLcode curl_code;

    if ((curl_code = curl_get_url_data(url, &url_data, timeout_ms)) != CURLE_OK) {
        PRINT_TEXT(curl_easy_strerror(curl_code));
        return NULL;
    }

    htmlDocPtr document;
    document = htmlReadMemory(url_data + sizeof(size_t), *(size_t*)url_data, NULL, NULL, HTML_PARSE_NOERROR);
    free(url_data);

    return document;
}


void xml_rule34_xxx_get_base_url_and_pid(char** out_url_to_list, int* out_pid_count) {
    htmlDocPtr document;
    xmlNode* node;
    xmlChar* buffer;
    char* pid_string;

    char url[2048] = "https://rule34.xxx//index.php?page=post&s=list&tags=";

    const char* must_have[] = {
        "sound"
    };

    const char* can_have[] = {
        "rape*"          ,
        "rough*"         ,
        "broken*"        ,
        "torture*"       ,
        "hardc*"         ,

        "secretly*"      ,

        "futa*"          ,
        "dickg*"         ,
        "trap*"          ,
        "sissy*"         ,
        "girly*"         ,
        "tomg*"          ,
        "femb*"          ,
        "shemale"        ,
        "intersex*"      ,
        "newhalf*"       ,
        "femi*"          ,
        "otoko*"         ,
        "transg*"        ,
        "androg*"        ,
        "feminine*"      ,

        "rule_63"        ,

        "camel_toe"      ,
        "cameltoe*"      ,
        "cleft_of_venus" ,
        "mound_of_venus" ,
        "plump_labia"    ,
        "plump_vulva"    ,
        "plump_pussy"    ,
        "plump_camel_toe",
        "fat_mons"       ,
        "fat_pussy"      ,
        "huge_pussy"     ,
        "big_pussy"      ,
        "puffy_pussy"    ,
        "puffy_v*"       ,
        "innie_pussy"    ,

        "auto*"          ,
        "self*"          , "-self_upload",

        "ahe*"           ,
        "gap*"           ,
        "moan*"          ,
        "dildo*"         ,
        "incest*"        ,
        "cry*"           ,
        "drug*"          ,
    };

    for (size_t i = 0; i < sizeof(must_have) / sizeof(must_have[0]); ++i) {
        if (i > 0 && i < sizeof(must_have) / sizeof(must_have[0])) {
            strcat(url, "+");
        }

        strcat(url, must_have[i]);
    }

    if (sizeof(can_have) / sizeof(can_have[0]) > 0) {
        strcat(url, "+(+");

        for (size_t i = 0; i < sizeof(can_have) / sizeof(can_have[0]); ++i) {
            if (i > 0 && i < sizeof(can_have) / sizeof(can_have[0])) {
                strcat(url, "+~+");
            }

            strcat(url, can_have[i]);
        }

        strcat(url, "+)");
    }

    if ((out_url_to_list[0] = strdup(url)) == NULL) {
        PRINT_TEXT(strerror(errno));
    }

    if ((document = xml_get_url_data(url, 20000)) != NULL) {
        if ((node = xml_node_find(xmlDocGetRootElement(document), "a", "alt", "last page", NULL)) != NULL) {
            if ((buffer = xmlGetProp(node, "href")) != NULL) { /* Do not forget to free up memory after xmlGetProp. */
                if ((pid_string = strrchr(buffer, '=')) != NULL) {
                    ++pid_string;
                    out_pid_count[0] = atoi(pid_string);

                    snprintf(url + strlen(url), sizeof(url) - strlen(url), "%s%i", "&pid=", atoi(pid_string));

                    xmlFreeDoc(document);

                    if ((document = xml_get_url_data(url, 20000)) != NULL) {
                        if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "image-list", NULL)) != NULL) {
                            out_pid_count[0] += xmlChildElementCount(node);
                        }
                    }
                }

                xmlFree(buffer);
            }
        }
        else {
            if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "image-list", NULL)) != NULL) {
                out_pid_count[0] = xmlChildElementCount(node);
            }
        }

        if (document != NULL) {
            xmlFreeDoc(document);
        }
    }
}

void xml_rule34_xxx_get_source_url(const char* url_to_list, int pid, char** out_url) {
    htmlDocPtr document;
    xmlNode* node;
    xmlChar* buffer;
    char url[2048];

    snprintf(url, sizeof(url), "%s&pid=%i", url_to_list, pid);

    if ((document = xml_get_url_data(url, 20000)) != NULL) {
        if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "image-list", NULL)) != NULL) {
            if ((node = xml_node_find(node, "span", "class", "thumb", NULL)) != NULL) {
                if ((node = xml_node_find(node, "a", "href", NULL, NULL)) != NULL) {
                    if ((buffer = xmlGetProp(node, "href")) != NULL) { /* Do not forget to free up memory after xmlGetProp. */
                        snprintf(url, sizeof(url), "%s%s", "https://rule34.xxx", buffer);

                        xmlFreeDoc(document);

                        if ((document = xml_get_url_data(url, 20000)) != NULL) {
                            if ((node = xml_node_find(xmlDocGetRootElement(document), "li", NULL, NULL, "Original image")) != NULL) {
                                if ((node = xml_node_find(node, "a", "href", NULL, NULL)) != NULL) {
                                    out_url[0] = xmlGetProp(node, "href"); /* Do not forget to free up memory after xmlGetProp. */
                                }
                            }
                        }

                        xmlFree(buffer);
                    }
                }
            }
        }

        if (document != NULL) {
            xmlFreeDoc(document);
        }
    }

    if (out_url[0] == NULL) {
        if ((document = xml_get_url_data("https://rule34.xxx/index.php?page=post&s=random", 20000)) != NULL) {
            if ((node = xml_node_find(xmlDocGetRootElement(document), "li", NULL, NULL, "Original image")) != NULL) {
                if ((node = xml_node_find(node, "a", "href", NULL, NULL)) != NULL) {
                    out_url[0] = xmlGetProp(node, "href"); /* Do not forget to free up memory after xmlGetProp. */
                }
            }

            xmlFreeDoc(document);
        }
    }
}


void xml_realbooru_com_get_base_url_and_pid(char** out_url_to_list, int* out_pid_count) {
    htmlDocPtr document;
    xmlNode* node;
    xmlChar* buffer;
    char* pid_string;
    char url[2048] = "https://realbooru.com//index.php?page=post&s=list&tags=";

    const char* must_have[] = {
        "sound"
    };


    for (size_t i = 0; i < sizeof(must_have) / sizeof(must_have[0]); ++i) {
        if (i > 0 && i < sizeof(must_have) / sizeof(must_have[0])) {
            strcat(url, "+");
        }

        strcat(url, must_have[i]);
    }

    if ((out_url_to_list[0] = strdup(url)) == NULL) {
        PRINT_TEXT(strerror(errno));
    }

    if ((document = xml_get_url_data(url, 20000)) != NULL) {
        if ((node = xml_node_find(xmlDocGetRootElement(document), "a", "alt", "last page", NULL)) != NULL) {
            if ((buffer = xmlGetProp(node, "href")) != NULL) { /* Do not forget to free up memory after xmlGetProp. */
                if ((pid_string = strrchr(buffer, '=')) != NULL) {
                    ++pid_string;
                    out_pid_count[0] = atoi(pid_string);

                    snprintf(url + strlen(url), sizeof(url) - strlen(url), "%s%i", "&pid=", atoi(pid_string));

                    xmlFreeDoc(document);

                    if ((document = xml_get_url_data(url, 20000)) != NULL) {
                        if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "items", NULL)) != NULL) {
                            out_pid_count[0] += xmlChildElementCount(node);
                        }
                    }
                }

                xmlFree(buffer);
            }
        }
        else {
            if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "items", NULL)) != NULL) {
                out_pid_count[0] = xmlChildElementCount(node);
            }
        }

        if (document != NULL) {
            xmlFreeDoc(document);
        }
    }
}

void xml_realbooru_com_get_source_url(const char* url_to_list, int pid, char** out_url) {
    htmlDocPtr document;
    xmlNode* node;
    xmlChar* buffer;
    char url[2048];

    snprintf(url, sizeof(url), "%s&pid=%i", url_to_list, pid);

    if ((document = xml_get_url_data(url, 20000)) != NULL) {
        if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "items", NULL)) != NULL) {
            if ((node = xml_node_find(node, "div", "class", "col thumb", NULL)) != NULL) {
                if ((node = xml_node_find(node, "a", "href", NULL, NULL)) != NULL) {
                    if ((buffer = xmlGetProp(node, "href")) != NULL) { /* Do not forget to free up memory after xmlGetProp. */
                        snprintf(url, sizeof(url), "%s", buffer);

                        xmlFreeDoc(document);

                        if ((document = xml_get_url_data(url, 20000)) != NULL) {
                            if ((node = xml_node_find(xmlDocGetRootElement(document), "video", "id", "gelcomVideoPlayer", NULL)) != NULL) {
                                if ((node = xml_node_find(node, "source", "src", NULL, NULL)) != NULL) {
                                    out_url[0] = xmlGetProp(node, "src"); /* Do not forget to free up memory after xmlGetProp. */
                                }
                            }
                            else if ((node = xml_node_find(xmlDocGetRootElement(document), "div", "class", "imageContainer", NULL)) != NULL) {
                                if ((node = xml_node_find(node, "img", "id", "image", NULL)) != NULL) {
                                    out_url[0] = xmlGetProp(node, "src"); /* Do not forget to free up memory after xmlGetProp. */
                                }
                            }
                        }

                        xmlFree(buffer);
                    }
                }
            }
        }

        if (document != NULL) {
            xmlFreeDoc(document);
        }
    }

    if (out_url[0] == NULL) {
        if ((document = xml_get_url_data("https://realbooru.com/index.php?page=post&s=random", 20000)) != NULL) {
            if ((node = xml_node_find(xmlDocGetRootElement(document), "li", NULL, NULL, "Original image")) != NULL) {
                if ((node = xml_node_find(node, "a", "href", NULL, NULL)) != NULL) {
                    out_url[0] = xmlGetProp(node, "href"); /* Do not forget to free up memory after xmlGetProp. */
                }
            }

            xmlFreeDoc(document);
        }
    }
}

#endif /* Random 18+ video/image */

#if 1 /* cJSON */

#include <cJSON.h>

void cjson_find(cJSON* node) {
    static uint32_t G_CJSON_COUNT;

    while (node) {
        if (node->type == cJSON_Array) {
            printf("%*c\"%s\": [\n", G_CJSON_COUNT, ' ', node->string);
        }
        if (node->type == cJSON_String) {
            printf("%*c\"%s\": \"%s\"\n", G_CJSON_COUNT, ' ', node->string, node->valuestring);
        }
        if (node->type == cJSON_Number) {
            printf("%*c\"%s\": %f\n", G_CJSON_COUNT, ' ', node->string, node->valuedouble);
        }

        if (node->child != NULL) {
            G_CJSON_COUNT += 4;
            cjson_find(node->child);
            G_CJSON_COUNT -= 4;
        }

        if (node->type == cJSON_Array) {
            printf("%*c]\n", G_CJSON_COUNT, ' ');
        }

        node = node->next;
    }
}

#endif /* cJSON */

#if 1 /* glad */

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

void ogl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
    PRINT_TEXT(message);
}

/**
 * @return OpenGL version.
 */
int ogl_initialize(GLboolean enable_debug) {
    int version;

    if ((version = gladLoaderLoadGL()) > 0) {
        if (enable_debug == GL_TRUE && GLAD_VERSION_MAJOR(version) == 4 && GLAD_VERSION_MINOR(version) >= 3) {
            glDebugMessageCallback(ogl_debug_message_callback, NULL);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        }
    }

    return version;
}

GLuint ogl_texture_create(GLenum target, GLint mag_filter, GLint min_filter, GLint wrap_s, GLint wrap_t, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels) {
    GLuint texture;

    glGenTextures(1, &texture);

    if (texture > 0) {
        glBindTexture(target, texture);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, mag_filter);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, min_filter);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap_s);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap_t);
        glTexImage2D(target, 0, format, width, height, 0, format, type, pixels);
        glGenerateMipmap(target);
        glBindTexture(target, 0);
    }

    return texture;
}

GLuint ogl_shader_create(GLenum type, GLsizei count, const GLchar* const* string, const GLint* length) {
    GLuint shader;

    if ((shader = glCreateShader(type)) > 0) {
        glShaderSource(shader, count, string, length);
        glCompileShader(shader);
    }

    return shader;
}

GLuint ogl_program_create(GLuint count, GLuint* shaders) {
    GLuint program;

    if ((program = glCreateProgram()) > 0) {
        for (GLuint i = 0; i < count; ++i) {
            glAttachShader(program, shaders[i]);
        }

        glLinkProgram(program);
    }

    return program;
}

GLuint ogl_buffer_create(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    GLuint buffer;

    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, size, data, usage);
    glBindBuffer(target, 0);

    return buffer;
}

void ogl_buffer_attribute(GLenum target, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    glBindBuffer(target, buffer);
    glEnableVertexAttribArray(index);
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
    glBindBuffer(target, 0);
}

#endif /* glad */

#if 1 /* cgltf */

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

#define CGLTF_ACCESSOR_PARSE(accessor, data_type, out_data) { \
    data_type* buffer = (data_type*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset / sizeof(data_type) + accessor->offset / sizeof(data_type); \
    cgltf_size num_components = cgltf_num_components(accessor->type); \
    cgltf_size stride = 0; \
    for (cgltf_size i = 0; i < accessor->count; ++i) { \
        for (cgltf_size j = 0; j < num_components; ++j) { \
            ((data_type*)out_data)[num_components * i + j] = buffer[stride + j]; \
        } \
        stride += accessor->stride / sizeof(data_type); \
    } \
}

void* cgltf_accessor_alloc_and_parse_data(const cgltf_accessor* accessor) {
    void* data;

    if ((data = malloc(accessor->buffer_view->size)) != NULL) {
        if (accessor->component_type == cgltf_component_type_r_8) {
            CGLTF_ACCESSOR_PARSE(accessor, int8_t, data);
        }
        else if (accessor->component_type == cgltf_component_type_r_8u) {
            CGLTF_ACCESSOR_PARSE(accessor, uint8_t, data);
        }
        else if (accessor->component_type == cgltf_component_type_r_16) {
            CGLTF_ACCESSOR_PARSE(accessor, int16_t, data);
        }
        else if (accessor->component_type == cgltf_component_type_r_16u) {
            CGLTF_ACCESSOR_PARSE(accessor, uint16_t, data);
        }
        else if (accessor->component_type == cgltf_component_type_r_32u) {
            CGLTF_ACCESSOR_PARSE(accessor, uint32_t, data);
        }
        else if (accessor->component_type == cgltf_component_type_r_32f) {
            CGLTF_ACCESSOR_PARSE(accessor, float, data);
        }
    }

    return data;
}

#endif /* cgltf */

#if 1 /* cgltf to OpenGL */

typedef struct transform {
    vec3 position;
    vec3 rotation;
    vec3 scale;
} transform_t;

void transform_initialize(transform_t* transform) {
    glm_vec3_zero(transform->position);
    glm_vec3_zero(transform->rotation);
    glm_vec3_one(transform->scale);
}

typedef struct culling {
    vec3 box[2];
} culling_t;

void culling_initialize(culling_t* culling) {
    glm_aabb_invalidate(culling->box);
}

void culling_set_from_data(const float* data, size_t count, culling_t* culling) {
    for (size_t i = 0; i < count; i += 3) {
        if (culling->box[0][0] > data[i]) {
            culling->box[0][0] = data[i];
        }
        if (culling->box[0][1] > data[i + 1]) {
            culling->box[0][1] = data[i + 1];
        }
        if (culling->box[0][2] > data[i + 2]) {
            culling->box[0][2] = data[i + 2];
        }

        if (culling->box[1][0] < data[i]) {
            culling->box[1][0] = data[i];
        }
        if (culling->box[1][1] < data[i + 1]) {
            culling->box[1][1] = data[i + 1];
        }
        if (culling->box[1][2] < data[i + 2]) {
            culling->box[1][2] = data[i + 2];
        }
    }
}

typedef struct ogl_material {
    char* name;
    GLuint program;
    GLuint texture;
} ogl_material_t;

void ogl_material_initialize(ogl_material_t* material) {
    material->name = NULL;
    material->program = 0;
    material->texture = 0;
}

typedef struct ogl_primitive {
    GLuint vertex_array_object;
    GLuint element_buffer_object;
    GLsizei count;
    GLenum type;
} ogl_primitive_t;

void ogl_primitive_initialize(ogl_primitive_t* primitive) {
    primitive->vertex_array_object = 0;
    primitive->element_buffer_object = 0;
    primitive->count = 0;
    primitive->type = 0;
}

typedef struct ogl_mesh {
    char* name;
    ogl_primitive_t* primitives;
    GLuint primitives_count;
} ogl_mesh_t;

void ogl_mesh_initialize(ogl_mesh_t* mesh) {
    mesh->name = NULL;
    mesh->primitives = NULL;
    mesh->primitives_count = 0;
}

typedef struct ogl_node {
    char* name;
    ogl_material_t* material;
    ogl_mesh_t* mesh;
} ogl_node_t;

void ogl_node_initialize(ogl_node_t* node) {
    node->name = NULL;
    node->material = NULL;
    node->mesh = NULL;
}

typedef struct ogl_scene {
    char* name;
    ogl_material_t* materials;
    ogl_mesh_t* meshes;
    ogl_node_t* nodes;
    GLuint materials_count;
    GLuint meshes_count;
    GLuint nodes_count;
} ogl_scene_t;

void ogl_scene_initialize(ogl_scene_t* scene) {
    scene->name = NULL;
    scene->materials = NULL;
    scene->meshes = NULL;
    scene->nodes = NULL;
    scene->materials_count = 0;
    scene->meshes_count = 0;
    scene->nodes_count = 0;
}


int cgltf_image_parse(const cgltf_image* image, const char* path_to_folder, uint8_t** out_pixels, int* out_width, int* out_height) {
    int result;

    if (image->uri != NULL) {
        if (strncmp(image->uri, "data:", 5) == 0) {
            const char* comma = strchr(image->uri, ',');

            if (comma != NULL && comma - image->uri >= 7 && strncmp(comma - 7, ";base64", 7) == 0) {
                const char* base64 = comma + 1;
                size_t base64_size = strlen(base64);
                size_t size = base64_size - base64_size / 4;

                if (base64_size >= 2) {
                    size -= base64[base64_size - 2] == '=';
                    size -= base64[base64_size - 1] == '=';
                }

                void* data;
                cgltf_options options = {
                    .type = cgltf_file_type_invalid,
                    .json_token_count = 0,
                    .memory = {
                        .alloc_func = NULL,
                        .free_func = NULL,
                        .user_data = NULL
                    },
                    .file = {
                        .read = NULL,
                        .release = NULL,
                        .user_data = NULL
                    }
                };

                if (cgltf_load_buffer_base64(&options, size, base64, &data) == cgltf_result_success) {
                    // result = ffmpeg_image_load_from_memory(data, image->buffer_view->size, AV_PIX_FMT_RGBA, out_pixels, out_width, out_height);

                    if ((out_pixels[0] = stbi_load_from_memory(data, image->buffer_view->size, out_width, out_height, NULL, STBI_rgb_alpha)) == NULL) {
                        result = -1;
                    }
                }
                else {
                    result = -1;
                }
            }
        }
        else {
            if (path_to_folder == NULL) {
                puts("path_to_folder is NULL!");
                // return AVERROR(ENOMEM);
                result = -1;
            }
            else {
                char* image_path;

                if ((image_path = malloc(strlen(path_to_folder) + strlen(image->uri) + 1)) != NULL) {
                    strcpy(image_path, path_to_folder);
                    strcat(image_path, image->uri);
                    // result = ffmpeg_image_load_from_url(image_path, AV_PIX_FMT_RGBA, out_pixels, out_width, out_height);

                    if ((out_pixels[0] = stbi_load(image_path, out_width, out_height, NULL, STBI_rgb_alpha)) == NULL) {
                        result = -1;
                    }

                    free(image_path);
                }
                else {
                    // result = AVERROR(ENOMEM);
                    result = errno;
                }
            }
        }
    }
    else if (image->buffer_view->buffer->data != NULL) {
        uint8_t* data;

        if ((data = malloc(image->buffer_view->size)) != NULL) {
            cgltf_size offset = image->buffer_view->offset;
            cgltf_size stride = image->buffer_view->stride ? image->buffer_view->stride : 1;

            for (cgltf_size i = 0; i < image->buffer_view->size; ++i) {
                data[i] = ((uint8_t*)image->buffer_view->buffer->data)[offset];
                offset += stride;
            }

            // result = ffmpeg_image_load_from_memory(data, image->buffer_view->size, AV_PIX_FMT_RGBA, out_pixels, out_width, out_height);

            if ((out_pixels[0] = stbi_load_from_memory(data, image->buffer_view->size, out_width, out_height, NULL, STBI_rgb_alpha)) == NULL) {
                result = -1;
            }

            free(data);
        }
        else {
            // result = AVERROR(ENOMEM);
            result = errno;
        }
    }

    return result;
}

void cgltf_material_to_ogl_material(const cgltf_material* material, ogl_material_t* out_material) {
    out_material->name = strdup(material->name);

    const char* vertex_shader_source =
        "#version 330 core\n"

        "layout (location = 0) in vec3 aPositions;\n"
        "layout (location = 1) in vec3 aNormals;\n"
        "layout (location = 2) in vec3 aTangents;\n"
        "layout (location = 3) in vec2 aTextureCoordinates;\n"
        "layout (location = 4) in vec4 aColors;\n"
        "layout (location = 5) in vec3 aJoints;\n"
        "layout (location = 6) in vec3 aWeights;\n"

        "out vec2 TextureCoordinates;\n"

        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"

        "void main() {\n"
        "    TextureCoordinates = aTextureCoordinates;\n"
        "    gl_Position = projection * view * model * vec4(aPositions, 1.0);\n"
        "}\n"
    ;

    const char* fragment_shader_source =
        "#version 330 core\n"

        "in vec2 TextureCoordinates;\n"

        "out vec4 FragmentColor;\n"

        "uniform sampler2D texture0;\n"

        "void main() {\n"
        "    FragmentColor = texture(texture0, TextureCoordinates);\n"
        "}\n"
    ;

    GLuint shaders[] = {
        ogl_shader_create(GL_VERTEX_SHADER, 1, &vertex_shader_source, NULL),
        ogl_shader_create(GL_FRAGMENT_SHADER, 1, &fragment_shader_source, NULL)
    };

    out_material->program = ogl_program_create(sizeof(shaders) / sizeof(shaders[0]), shaders);

    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);

    if (material->has_pbr_metallic_roughness == 1) {
        if (material->pbr_metallic_roughness.base_color_texture.texture != NULL) {
            if (material->pbr_metallic_roughness.base_color_texture.texture->image != NULL) {
                uint8_t* pixels;
                int width;
                int height;
                int result;

                if ((result = cgltf_image_parse(material->pbr_metallic_roughness.base_color_texture.texture->image, NULL, &pixels, &width, &height)) == 0) {
                    out_material->texture = ogl_texture_create(
                        GL_TEXTURE_2D,
                        material->pbr_metallic_roughness.base_color_texture.texture->sampler != NULL ? material->pbr_metallic_roughness.base_color_texture.texture->sampler->mag_filter : GL_LINEAR,
                        material->pbr_metallic_roughness.base_color_texture.texture->sampler != NULL ? material->pbr_metallic_roughness.base_color_texture.texture->sampler->min_filter : GL_NEAREST_MIPMAP_LINEAR,
                        material->pbr_metallic_roughness.base_color_texture.texture->sampler != NULL ? material->pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_s : GL_REPEAT,
                        material->pbr_metallic_roughness.base_color_texture.texture->sampler != NULL ? material->pbr_metallic_roughness.base_color_texture.texture->sampler->wrap_t : GL_REPEAT,
                        width,
                        height,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        pixels
                    );

                    free(pixels);
                }
                else {
                    // PRINT_TEXT(av_err2str(result));
                    PRINT_TEXT(strerror(result));
                }
            }
        }
    }

    // if (material->has_pbr_specular_glossiness) {
    //     if (material->pbr_specular_glossiness.diffuse_texture.texture) {
    //         if (material->pbr_specular_glossiness.diffuse_texture.texture->image) {
    //             cgltf_image_parse(material->pbr_specular_glossiness.diffuse_texture.texture->image, NULL, &pixels, &width, &height);
    //         }
    //     }
    //     else {
    //         // printf("pbr_specular_glossiness texcoord: %i\n", material->pbr_specular_glossiness.diffuse_texture.texcoord);
    //     }
    // }

    // if (material->has_specular) {
    //     if (material->specular.specular_texture.texture) {
    //         if (material->specular.specular_texture.texture->image) {
    //             cgltf_image_parse(material->specular.specular_texture.texture->image, NULL, &pixels, &width, &height);
    //         }
    //     }
    //     if (material->specular.specular_color_texture.texture) {
    //         if (material->specular.specular_color_texture.texture->image) {
    //             cgltf_image_parse(material->specular.specular_color_texture.texture->image, NULL, &pixels, &width, &height);
    //         }
    //     }
    // }
}

void cgltf_primitive_to_ogl_primitive(const cgltf_primitive* primitive, ogl_primitive_t* out_primitive) {
    glGenVertexArrays(1, &out_primitive->vertex_array_object);

    if (primitive->indices != NULL) {
        void* accessor_data;

        if ((accessor_data = cgltf_accessor_alloc_and_parse_data(primitive->indices)) != NULL) {
            glBindVertexArray(out_primitive->vertex_array_object);
            out_primitive->element_buffer_object = ogl_buffer_create(GL_ELEMENT_ARRAY_BUFFER, primitive->indices->buffer_view->size, accessor_data, GL_STATIC_DRAW);
            glBindVertexArray(0);

            free(accessor_data);
        }

        out_primitive->count = primitive->indices->count;

        if (primitive->indices->component_type == cgltf_component_type_r_8u) {
            out_primitive->type = GL_UNSIGNED_BYTE;
        }
        else if (primitive->indices->component_type == cgltf_component_type_r_16u) {
            out_primitive->type = GL_UNSIGNED_SHORT;
        }
        else {
            out_primitive->type = GL_UNSIGNED_INT;
        }
    }

    for (cgltf_size i = 0; i < primitive->attributes_count; ++i) {
        void* accessor_data;
        GLuint vertex_buffer_object;

        glBindVertexArray(out_primitive->vertex_array_object);

        if ((accessor_data = cgltf_accessor_alloc_and_parse_data(primitive->attributes[i].data)) != NULL) {
            if (primitive->attributes[i].type == cgltf_attribute_type_position) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

                if (primitive->indices == NULL) {
                    out_primitive->count = primitive->attributes[i].data->count;
                }

                // culling_set_from_data(accessor_data, primitive->attributes[i].data->buffer_view->size / sizeof(float), &out_primitive->culling);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_normal) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_tangent) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_texcoord) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_color) {
                if (primitive->attributes[i].data->component_type == cgltf_component_type_r_16u) {
                    float* color;

                    if ((color = malloc(primitive->attributes[i].data->count * 4 * sizeof(float))) != NULL) {
                        if (primitive->attributes[i].data->type == cgltf_type_vec4) {
                            for (cgltf_size i = 0; i < primitive->attributes[i].data->count * 4; ++i) {
                                color[i] = ((uint16_t*)accessor_data + i)[0] / 65535.0f;
                            }
                        }
                        else if (primitive->attributes[i].data->type == cgltf_type_vec3) {
                            for (cgltf_size i = 0; i < primitive->attributes[i].data->count * 4; ++i) {
                                if (i > 0 && i % 4 == 0) {
                                    color[i] = 1.0f;
                                }
                                else {
                                    color[i] = ((uint16_t*)accessor_data + i)[0] / 65535.0f;
                                }
                            }
                        }

                        free(accessor_data);
                        accessor_data = color;
                    }
                }

                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, (primitive->attributes[i].data->count * 4 * sizeof(float)), accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 4, 4, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_joints) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 5, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            }
            else if (primitive->attributes[i].type == cgltf_attribute_type_weights) {
                vertex_buffer_object = ogl_buffer_create(GL_ARRAY_BUFFER, primitive->attributes[i].data->buffer_view->size, accessor_data, GL_STATIC_DRAW);
                ogl_buffer_attribute(GL_ARRAY_BUFFER, vertex_buffer_object, 6, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            }

            free(accessor_data);
        }

        glBindVertexArray(0);
        glDeleteBuffers(1, &vertex_buffer_object);
    }
}

void cgltf_mesh_to_ogl_mesh(const cgltf_mesh* mesh, ogl_mesh_t* out_mesh) {
    out_mesh->name = strdup(mesh->name);

    if ((out_mesh->primitives = malloc(sizeof(ogl_primitive_t) * mesh->primitives_count)) != NULL) {
        out_mesh->primitives_count = mesh->primitives_count;

        for (cgltf_size i = 0; i < mesh->primitives_count; ++i) {
            ogl_primitive_initialize(&out_mesh->primitives[i]);
            cgltf_primitive_to_ogl_primitive(&mesh->primitives[i], &out_mesh->primitives[i]);
        }
    }
}


void ogl_scene_add_mesh(ogl_scene_t* scene, const cgltf_mesh* mesh) {
    ogl_mesh_t* meshes;

    if ((meshes = realloc(scene->meshes, sizeof(ogl_mesh_t) * scene->meshes_count + 1)) != NULL) {
        scene->meshes = meshes;
        ogl_mesh_initialize(&scene->meshes[scene->meshes_count]);
        cgltf_mesh_to_ogl_mesh(mesh, &scene->meshes[scene->meshes_count]);
        ++scene->meshes_count;
    }
}

void cgltf_scene_to_ogl_scene(const cgltf_scene* scene, ogl_scene_t* out_scene) {
    out_scene->name = strdup(scene->name);

    // if ((out_scene->materials = malloc(sizeof(ogl_material_t) * scene->materials_count)) != NULL) {
    //     out_scene->materials_count = scene->materials_count;

    //     for (cgltf_size i = 0; i < scene->materials_count; ++i) {
    //         ogl_material_initialize(&out_scene->materials[i]);
    //         cgltf_material_to_ogl_material(&scene->materials[i], &out_scene->materials[i]);
    //     }
    // }

    // if ((out_scene->meshes = malloc(sizeof(ogl_mesh_t) * scene->meshes_count)) != NULL) {
    //     out_scene->meshes_count = scene->meshes_count;

    //     for (cgltf_size i = 0; i < scene->meshes_count; ++i) {
    //         ogl_mesh_initialize(&out_scene->meshes[i]);
    //         cgltf_mesh_to_ogl_mesh(&scene->meshes[i], &out_scene->meshes[i]);
    //     }
    // }


    /*
    if ((out_scene->meshes = malloc(sizeof(ogl_mesh_t) * scene->meshes_count)) != NULL) {
        out_scene->meshes_count = scene->meshes_count;

        for (cgltf_size i = 0; i < scene->meshes_count; ++i) {
            ogl_mesh_initialize(&out_scene->meshes[i]);
            cgltf_mesh_to_ogl_mesh(&scene->meshes[i], &out_scene->meshes[i]);
        }
    }

    cgltf_size nodes_geometry_count = 0;

    for (cgltf_size i = 0; i < scene->nodes_count; ++i) {
        if (scene->nodes[i]->mesh != NULL) {
            ++nodes_geometry_count;
        }
    }

    if ((out_scene->nodes = malloc(sizeof(ogl_node_t) * nodes_geometry_count)) != NULL) {
        out_scene->nodes_count = nodes_geometry_count;

        for (cgltf_size i = 0, j = 0; i < scene->nodes_count; ++i) {
            if (scene->nodes[i]->mesh != NULL) {
                out_scene->nodes[j].mesh = scene->nodes[i]->mesh;

                char* name = scene->nodes[i]->mesh->primitives[0].material->name;
                out_scene->nodes[j].material = ;
                ++j;
            }
        }
    }
    */
}

cgltf_result cgltf_to_ogl(const char* url, ogl_scene_t* scene) {
    cgltf_result result;
    cgltf_options options = {
        .type = cgltf_file_type_invalid,
        .json_token_count = 0,
        .memory = {
            .alloc_func = NULL,
            .free_func = NULL,
            .user_data = NULL
        },
        .file = {
            .read = NULL,
            .release = NULL,
            .user_data = NULL
        }
    };
    cgltf_data* data;

    if (strncmp(url, "http", 4) == 0) {
        void* url_data;
        CURLcode curl_code;

        if ((curl_code = curl_get_url_data(url, &url_data, 60000)) == CURLE_OK) {
            if ((result = cgltf_parse(&options, url_data + sizeof(size_t), *((size_t*)url_data), &data)) == cgltf_result_success) {
                if ((result = cgltf_load_buffers(&options, data, NULL)) == cgltf_result_success) {
                    if (data->scene != NULL) {
                        cgltf_scene_to_ogl_scene(data->scene, scene);
                    }
                }

                cgltf_free(data);
            }

            free(url_data);
        }
        else {
            result = cgltf_result_file_not_found;
            PRINT_TEXT(curl_easy_strerror(curl_code));
        }
    }
    else {
        if ((result = cgltf_parse_file(&options, url, &data)) == cgltf_result_success) {
            if ((result = cgltf_load_buffers(&options, data, url)) == cgltf_result_success) {
                if (data->scene != NULL) {
                    cgltf_scene_to_ogl_scene(data->scene, scene);
                }
            }

            cgltf_free(data);
        }
    }

    return result;
}


void ogl_primitive_draw(const ogl_primitive_t* primitive) {
    glBindVertexArray(primitive->vertex_array_object);

    if (primitive->element_buffer_object > 0 && primitive->count > 0 && primitive->type != 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive->element_buffer_object);
        glDrawElements(GL_TRIANGLES, primitive->count, primitive->type, NULL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else {
        glDrawArrays(GL_TRIANGLES, 0, primitive->count);
    }

    glBindVertexArray(0);
}

void ogl_node_draw(const ogl_node_t* node, mat4 projection, mat4 view, vec4 planes[6]) {
    // if (primitive->material != NULL) {
    //     glUseProgram(primitive->material->program);

    //     glUniformMatrix4fv(glGetUniformLocation(primitive->material->program, "projection"), 1, GL_FALSE, projection);
    //     glUniformMatrix4fv(glGetUniformLocation(primitive->material->program, "view"), 1, GL_FALSE, view);
    //     glUniformMatrix4fv(glGetUniformLocation(primitive->material->program, "model"), 1, GL_FALSE, GLM_MAT4_IDENTITY);

    //     if (primitive->material->texture != 0) {
    //         glBindTexture(GL_TEXTURE_2D, primitive->material->texture);
    //     }
    // }

    if (node->mesh != NULL) {
        for (GLuint i = 0; i < node->mesh->primitives_count; ++i) {
            ogl_primitive_draw(&node->mesh->primitives[i]);
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void ogl_scene_draw(const ogl_scene_t* scene, mat4 projection, mat4 view, vec4 planes[6]) {
    for (GLuint i = 0; i < scene->meshes_count; ++i) {
        ogl_node_draw(&scene->meshes[i], projection, view, planes);
    }
}

#if 0

void ogl_node_parse(const cgltf_node* node, ogl_node_t* out_node) {
    out_node->name = strdup(node->name);

    if (node->has_translation == 1) {
        glm_vec3_copy(node->translation, out_node->transform.position);
    }
    if (node->has_rotation == 1) {
        glm_vec3_copy(node->rotation, out_node->transform.rotation);
    }
    if (node->has_scale == 1) {
        glm_vec3_copy(node->scale, out_node->transform.scale);
    }
    if (node->has_matrix == 1) {
        cgltf_node_transform_world(node, out_node->matrix);
    }

    if (node->mesh != NULL) {
        if ((out_node->mesh = malloc(sizeof(ogl_mesh_t))) != NULL) {
            ogl_mesh_initialize(out_node->mesh);
            ogl_mesh_parse(node->mesh, out_node->mesh);
        }
    }

    if (node->children != NULL) {
        if ((out_node->children = malloc(sizeof(ogl_node_t) * node->children_count)) != NULL) {
            out_node->children_count = node->children_count;

            for (cgltf_size i = 0; i < node->children_count; ++i) {
                ogl_node_initialize(&out_node->children[i]);
                ogl_node_parse(node->children[i], &out_node->children[i]);
            }
        }
    }
}

void ogl_draw_node(const ogl_node_t* node, mat4 projection, mat4 view, vec4 planes[6]) {
    if (node->mesh != NULL) {
        for (GLuint i = 0; i < node->mesh->primitives_count; ++i) {
            /* Frustum culling */
            vec3 box[2];
            glm_vec3_copy(node->mesh->primitives[i].culling.box[0], box[0]);
            glm_vec3_copy(node->mesh->primitives[i].culling.box[1], box[1]);

            box[0][0] += node->mesh->primitives[i].transform.position[0];
            box[0][1] += node->mesh->primitives[i].transform.position[1];
            box[0][2] += node->mesh->primitives[i].transform.position[2];

            box[1][0] += node->mesh->primitives[i].transform.position[0];
            box[1][1] += node->mesh->primitives[i].transform.position[1];
            box[1][2] += node->mesh->primitives[i].transform.position[2];

            /* TODO: rotate box for currect frustum collision */

            if (glm_aabb_frustum(box, planes) == false) {
                continue;
            }

            if (node->mesh->primitives[i].material != NULL) {
                if (node->mesh->primitives[i].material->program != 0) {
                    glUseProgram(node->mesh->primitives[i].material->program);
                    glUniformMatrix4fv(glGetUniformLocation(node->mesh->primitives[i].material->program, "projection"), 1, GL_FALSE, projection);
                    glUniformMatrix4fv(glGetUniformLocation(node->mesh->primitives[i].material->program, "view"), 1, GL_FALSE, view);
                    glUniformMatrix4fv(glGetUniformLocation(node->mesh->primitives[i].material->program, "model"), 1, GL_FALSE, node->matrix);
                }


                // #define min_x node->mesh->primitives[i].culling.box[0][0]
                // #define min_y node->mesh->primitives[i].culling.box[0][1]
                // #define min_z node->mesh->primitives[i].culling.box[0][2]
                // #define max_x node->mesh->primitives[i].culling.box[1][0]
                // #define max_y node->mesh->primitives[i].culling.box[1][1]
                // #define max_z node->mesh->primitives[i].culling.box[1][2]

                #define min_x box[0][0]
                #define min_y box[0][1]
                #define min_z box[0][2]
                #define max_x box[1][0]
                #define max_y box[1][1]
                #define max_z box[1][2]

                glPointSize(4.0f);
                glBegin(GL_POINTS);
                    glVertex3fv(box[0]);
                    glVertex3fv(box[1]);

                    // glVertex3fv(node->mesh->primitives[i].culling.box[0]);
                    // glVertex3fv(node->mesh->primitives[i].culling.box[1]);

                    // glVertex3f(min_x, min_y, min_z);
                    // glVertex3f(min_x, max_y, min_z);
                    // glVertex3f(min_x, min_y, max_z);
                    // glVertex3f(min_x, max_y, max_z);

                    // glVertex3f(max_x, min_y, min_z);
                    // glVertex3f(max_x, max_y, min_z);
                    // glVertex3f(max_x, min_y, max_z);
                    // glVertex3f(max_x, max_y, max_z);
                glEnd();
                glBegin(GL_LINES);
                    glVertex3fv(box[0]);
                    glVertex3fv(box[1]);

                    // glVertex3fv(node->mesh->primitives[i].culling.box[0]);
                    // glVertex3fv(node->mesh->primitives[i].culling.box[1]);

                    // glVertex3f(min_x, min_y, min_z);
                    // glVertex3f(min_x, max_y, min_z);
                    // glVertex3f(min_x, min_y, max_z);
                    // glVertex3f(min_x, max_y, max_z);

                    // glVertex3f(max_x, min_y, min_z);
                    // glVertex3f(max_x, max_y, min_z);
                    // glVertex3f(max_x, min_y, max_z);
                    // glVertex3f(max_x, max_y, max_z);
                glEnd();


                if (node->mesh->primitives[i].material->texture != 0) {
                    glBindTexture(GL_TEXTURE_2D, node->mesh->primitives[i].material->texture);
                }

                glBindVertexArray(node->mesh->primitives[i].vertex_array_object);
                    if (node->mesh->primitives[i].element_buffer_object > 0) {
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->mesh->primitives[i].element_buffer_object);
                            glDrawElements(GL_TRIANGLES, node->mesh->primitives[i].count, node->mesh->primitives[i].type, NULL);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
                    }
                    else {
                        glDrawArrays(GL_TRIANGLES, 0, node->mesh->primitives[i].count);
                    }
                glBindVertexArray(0);

                glBindTexture(GL_TEXTURE_2D, 0);
                glUseProgram(0);
            }
        }
    }

    for (GLuint i = 0; i < node->children_count; ++i) {
        ogl_draw_node(&node->children[i], projection, view, planes);
    }
}

void ogl_scene_draw(const ogl_scene_t* scene, mat4 projection, mat4 view) {
    /* For frustum culling */
    mat4 projection_view;
    vec4 planes[6];
    glm_mat4_mul(projection, view, projection_view);
    glm_frustum_planes(projection_view, planes);

    for (GLuint i = 0; i < scene->nodes_count; ++i) {
        ogl_draw_node(&scene->nodes[i], projection, view, planes);
    }
}

#endif

#endif /* cgltf to OpenGL */

#if 1 /* GLFW */

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GLboolean something_disgusting(GLFWwindow* window) {
    static int initialized;
    static mat4 projection;
    static mat4 view = GLM_MAT4_IDENTITY_INIT;
    static double cursor_position_x;
    static double cursor_position_y;
    static int window_width;
    static int window_height;
    static ogl_scene_t scene;

    if (initialized == 0) {
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glfwGetWindowSize(window, &window_width, &window_height);
        glm_perspective(GLM_PI_4f, (float)window_width / window_height, 0.001f, FLT_MAX, projection);
        glfwSetWindowUserPointer(window, projection);
        // cgltf_to_ogl("https://github.com/KhronosGroup/glTF-Sample-Models/raw/main/2.0/Corset/glTF-Binary/Corset.glb", &scene);

        initialized = 1;
    }

    glfwGetWindowSize(window, &window_width, &window_height);
    glfwGetCursorPos(window, &cursor_position_x, &cursor_position_y);

    float move_speed = 0.2f;
    float mouse_sensitivity = 1.0f;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        move_speed *= 10.0f;
        mouse_sensitivity *= 10.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        move_speed /= 10.0f;
        mouse_sensitivity /= 10.0f;
    }

    static double last_time;

    if (glfwGetTime() - last_time > 0.01) {
        last_time = glfwGetTime();

        static int first_person_view;
        static double first_person_view_change_last_delay;

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            if (last_time - first_person_view_change_last_delay > 0.5) {
                first_person_view = (first_person_view + 1) % 2;

                if (first_person_view == 1) {
                    glfwSetCursorPos(window, window_width / 2.0, window_height / 2.0);
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
                }
                else {
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                }

                first_person_view_change_last_delay = last_time;
            }
        }

        if (first_person_view == 1) {
            if ((int)cursor_position_x != window_width / 2 || (int)cursor_position_y != window_height / 2) {
                cglm_view_rotate(view, glm_rad((cursor_position_x - window_width / 2) * mouse_sensitivity / 100.0f), (vec3) { 0.0f, 1.0f, 0.0f });
                cglm_view_rotate(view, glm_rad((cursor_position_y - window_height / 2) * mouse_sensitivity / 100.0f), (vec3) { 1.0f, 0.0f, 0.0f });

                glfwSetCursorPos(window, window_width / 2.0, window_height / 2.0);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cglm_view_move((vec3) { 1.0f, 0.0f, 0.0f }, view, -move_speed);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cglm_view_move((vec3) { 1.0f, 0.0f, 0.0f }, view, move_speed);
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            cglm_view_move_up(view, -move_speed);
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            cglm_view_move_up(view, move_speed);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cglm_view_move((vec3) { 0.0f, 0.0f, 1.0f }, view, move_speed);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cglm_view_move((vec3) { 0.0f, 0.0f, 1.0f }, view, -move_speed);
        }

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            glm_mat4_identity(view);
            glm_perspective(GLM_PI_4f, (float)window_width / window_height, 0.001f, FLT_MAX, projection);
        }

        static mat4 projection_view;
        glm_mat4_mul(projection, view, projection_view);

        static vec4 planes[6];
        glm_frustum_planes(projection_view, planes);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ogl_scene_draw(&scene, projection, view, planes);

        return GL_TRUE;
    }

    return GL_FALSE;
}

void glfw_error_callback(int error_code, const char* description) {
    PRINT_TEXT(description);
}

void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    // glm_perspective(GLM_PI_4f, (float)width / height, 0.001f, FLT_MAX, glfwGetWindowUserPointer(window));
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void glfw_start() {
    glfwSetErrorCallback(glfw_error_callback);

    if (glfwInit() == GLFW_TRUE) {
        #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif /* __APPLE__ */

        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

        GLFWwindow* window;

        if ((window = glfwCreateWindow(960, 540, "Project", NULL, NULL)) != NULL) {
            // glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
            // glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
            // glfwSetWindowAttrib(window, GLFW_FLOATING, GLFW_TRUE);

            glfwMakeContextCurrent(window);
            glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
            glfwSetKeyCallback(window, glfw_key_callback);

            if (ogl_initialize(GL_TRUE) > 0) {
                while (glfwWindowShouldClose(window) == GLFW_FALSE) {
                    glfwWaitEvents();

                    if (something_disgusting(window) == GL_TRUE) {
                        glfwSwapBuffers(window);
                    }
                }
            }

            glfwDestroyWindow(window);
        }

        glfwTerminate();
    }
}

#endif /* GLFW */

#ifdef __cpulsplus
} /* extern "C" */
#endif /* __cpulsplus */

#endif /* PROJECT_H */


int main(int argc, char** argv) {
    glfw_start();
    return 0;

#if 0
    int result;
    void* data;
    size_t size;

    if ((result = file_load("../data/main.json", &data, &size)) == 0) {
        cJSON* root;

        if ((root = cJSON_Parse(data)) != NULL) {
            cjson_find(root);
            cJSON_Delete(root);
        }

        free(data);
    }
    else {
        PRINT_TEXT(strerror(result));
    }

    return 0;
#endif

#if 0
    // "../../tcc/tcc" -run -I../include -I../include/winapi -I../include/libxml2 -L. -lavcodec-61 -lavformat-61 -lavutil-59 -lswresample-5 -lswscale-8 -lcurl-x64 -lxml2-2 -lglfw3 -lportaudio -luser32 main.c

    glfwInit();
    double t1;

    DWORD milliseconds = 690;
    BOOL is_clicking = FALSE;

    puts("F1 - clicking on/off");
    puts("F2 - exit");

    while (TRUE) {
        if (GetAsyncKeyState(VK_F2)) {
            break;
        }

        if (GetAsyncKeyState(VK_F1)) {
            is_clicking = is_clicking ? FALSE : TRUE;
            t1 = glfwGetTime();

            printf("Clicking is %s\n", is_clicking ? "on" : "off");

            Sleep(200);
        }

        if (GetAsyncKeyState(VK_F3)) {
            milliseconds += 5;
        }

        if (GetAsyncKeyState(VK_F4)) {
            milliseconds -= 5;
        }

        if (is_clicking) {
            // Sleep(1500);

            // mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, NULL, 0);
            // Sleep(5);
            // mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, NULL, 0);

            // Sleep(milliseconds);
            // // Beep(500, 50);

            // mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, NULL, 0);
            // Sleep(5);
            // mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, NULL, 0);
            // Sleep(5);
        }
    }

    printf("Last milliseconds: %i\n", milliseconds);

    return 0;
#endif
}
