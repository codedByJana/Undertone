#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#pragma pack(push, 1)
// .WAV file format
typedef struct
{
    char riff_header[4];
    uint32_t file_size;
    char header_type[4];

    char chunk_marker[4];
    uint16_t format_type;
    uint32_t fmt_chunk_size;
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
void embed_message(int16_t *audio_buffer, int16_t sample_count, const uint8_t *payload, size_t payload_bits)
{
    for (size_t i = 0; i < payload_bits && i < sample_count; i++)
    {
        u_int8_t bit = (payload[i / 8] >> (7 - (i % 8))) & 0x01;
        audio_buffer[i] = (audio_buffer[i] & ~1) | bit;
    }
}

// calculate wav metrics for detection

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

    // double chi_square = 0.0;

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
            // chi_square += ((observed_even - expected) * (observed_even - expected)) / expected;
        }
    }
    if (total_pairs == 0.0)
        return 0.0;
    if (flattened_pairs / total_pairs < 0.08)
    {
        printf("Your Audio is clean!");
    }
    else
        printf("You've Got Mail!");
    // return chi_square;
}

/*
Clean WAV: 0.02 to 0.08 (2% to 8% suspicious — natural baseline variance)

Stego WAV (Embedded payload): 0.85 to 0.99 (85% to 99% probability of hidden data)
*/

int main()
{
    FILE *fb = fopen("youve-got-mail-sound.mp3", "rb");
    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, fb);

    size_t num_samples = header.data_length / sizeof(int16_t);
    int16_t *samples = malloc(header.data_length);
    fread(samples, sizeof(int16_t), num_samples, fb);
    fclose(fb);

    double metric = calculate_chi_square_metric(samples, num_samples);
    // printf("Calculated Chi_square metric: %f\n", metric);

    free(samples);
    return 0;
}