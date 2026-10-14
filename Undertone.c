#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// .WAV File Parser
#pragma pack(push, 1)
typedef struct
{
    char riff_header[4];
    uint32_t file_size;
    char header_type[4];

    char chunk_marker[4];
    uint32_t fmt_chunk_size;
    uint16_t format_type;
    uint16_t channels_number;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t sample_bits;

    char data_header[4];
    uint32_t data_length;
} WavHeader;
#pragma pack(pop)

// embed secret message file
void embed_message(int16_t *audio_buffer, size_t sample_count, const uint8_t *msg, uint32_t msg_len)
{
    size_t total_bytes = (4 + msg_len) * 8;
    if (total_bytes > sample_count)
    {
        fprintf(stderr, "Error: Message size (%zu bits) exceeds audio capacity (%zu samples).\n", total_bytes, sample_count);
        fprintf(stderr, "you're entering a massege that it larger than the audio itself? FR?\n");
        exit(1);
    }

    uint8_t *payload = malloc(4 + msg_len);
    payload[0] = (msg_len >> 24) & 0xFF;
    payload[1] = (msg_len >> 16) & 0xFF;
    payload[2] = (msg_len >> 8) & 0xFF;
    payload[3] = (msg_len) & 0xFF;
    memcpy(payload + 4, msg, msg_len);
    for (size_t i = 0; i < total_bytes; i++)
    {
        u_int8_t bit = (payload[i / 8] >> (7 - (i % 8))) & 0x01;
        audio_buffer[i] = (audio_buffer[i] & ~1) | bit;
    }
    free(payload);
}

// Extract and Decode the Extracted text from an Audio
char *extract_lsb(const uint16_t *audio)
{

    uint32_t msg_len = 0;
    for (int i = 0; i < 32; i++)
    {
        uint8_t bit = audio[i] & 1;
        msg_len = (msg_len << 1) | bit;
    }

    char *msg = malloc(msg_len + 1);
    if (!msg)
        return NULL;
    memset(msg, 0, msg_len + 1);
    for (size_t i = 0; i < msg_len * 8; i++)
    {
        size_t sample_idx = 32 + i;
        uint8_t bit = audio[sample_idx] & 1;
        msg[i / 8] |= (bit << (7 - (i % 8)));
    }

    msg[msg_len] = '\0';
    return msg;
}

// calculate wav metrics for detection
/*
Clean WAV: 0.02 to 0.08 (2% to 8% suspicious — natural baseline variance)

Stego WAV (Embedded payload): 0.85 to 0.99 (85% to 99% probability of hidden data)
*/

double calculate_chi_square_metric(const uint16_t *audio, size_t count)
{
    uint32_t hist[65536] = {0};
    for (size_t i = 0; i < count; i++)
    {
        uint16_t val = (uint16_t)(audio[i] + 32768);
        hist[val]++;
    }
    double total_pairs = 0.0;
    double flattened_pairs = 0.0;

    for (int i = 0; i < 65536; i += 2)
    {
        double observed_even = hist[i];
        double observed_odd = hist[i + 1];
        double sum = observed_even + observed_odd;

        double expected = (observed_even + observed_odd) / 2.0;

        if (sum > 10.0)
        {
            total_pairs += 1.0;
            double diff = (observed_even > observed_odd) ? (observed_even - observed_odd) : (observed_odd - observed_even);
            double ratio = diff / sum;
            if (ratio < 0.5)
            {
                flattened_pairs += 1.0;
            }
        }
    }
    if (total_pairs == 0.0)
        return 0.0;
    double steg_score = flattened_pairs / total_pairs < 0.08;
    if (steg_score > 0.08)
    {
        printf("Your Audio is clean!");
    }
    else
    {
        printf("You've Got Mail!");
        char *hidden_msg = extract_lsb(audio);
        if (hidden_msg)
        {
            printf("I read your mail for you :) it says: \"%s\" ", hidden_msg);
            free(hidden_msg);
        }
    }

    return (steg_score);
}

int main(int argc, char **argv)
{
    char *input_audio = NULL;
    char *output_audio = NULL;
    char *message = NULL;

    enum mode
    {
        ANALYZE_MODE,
        EMBED_MODE,
        NONE_MODE
    } mode = NONE_MODE;

    int opt;
    while ((opt = getopt(argc, argv, "eai:o:m:")) != -1)
    {
        switch (opt)
        {
        case 'e':
            mode = EMBED_MODE;
            break;
        case 'a':
            mode = ANALYZE_MODE;
            break;
        case 'i':
            input_audio = optarg;
            break;
        case 'o':
            output_audio = optarg;
            break;
        case 'm':
            message = optarg;
            break;

        default:
            fprintf(stderr, "Usage: %s -[e|x|a] -i input.wav [-o out.wav] [-m message]\n", argv[0]);
            return 1;
        }
    }
    if (!input_audio || mode == NONE_MODE)
    {
        fprintf(stderr, "Error: Input file (-i) and operational mode (-e|-x|-a) are required.\n");
        return 1;
    }

    FILE *fb = fopen(input_audio, "rb");
    if(!fb) {
        perror("this ain't no wav file you D.A ");
        return 1;
    }
    WavHeader header;

    fread(&header, sizeof(WavHeader), 1, fb);

    size_t num_samples = header.data_length / sizeof(int16_t);
    
    int16_t *samples = malloc(header.data_length);

    fread(samples, sizeof(int16_t), num_samples, fb);
    fclose(fb);

    if (mode == EMBED_MODE)
    {
        if (!message || !output_audio)
        {
            fprintf(stderr, "Error: Embedding requires -m \"message\" and -o output.wav\n");
            free(samples);
            return 1;
        }

        embed_message(samples, num_samples, (uint8_t *)message, strlen(message));

        FILE *out_fp = fopen(output_audio, "wb");
        fwrite(&header, sizeof(WavHeader), 1, out_fp);
        fwrite(samples, sizeof(int16_t), num_samples, out_fp);
        fclose(out_fp);
    }

    else if (mode == ANALYZE_MODE)
    {
        double metric = calculate_chi_square_metric(samples, num_samples);
    }
    // printf("Calculated Chi_square metric: %f\n", metric);

    free(samples);
    return 0;
}