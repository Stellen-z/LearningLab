#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979f

/* WAV header writing helper */
static void write_wav_header(FILE *fp, uint32_t dataSize)
{
    uint32_t chunkSize = 36 + dataSize;
    uint16_t audioFormat = 1;      /* PCM */
    uint16_t numChannels = 1;      /* Mono */
    uint32_t byteRate = SAMPLE_RATE * 1 * 2;
    uint16_t blockAlign = 1 * 2;
    uint16_t bitsPerSample = 16;

    fwrite("RIFF", 1, 4, fp);
    fwrite(&chunkSize, 4, 1, fp);
    fwrite("WAVE", 1, 4, fp);
    fwrite("fmt ", 1, 4, fp);

    uint32_t subchunk1Size = 16;
    fwrite(&subchunk1Size, 4, 1, fp);
    fwrite(&audioFormat, 2, 1, fp);
    fwrite(&numChannels, 2, 1, fp);
    fwrite(&SAMPLE_RATE, 4, 1, fp);
    fwrite(&byteRate, 4, 1, fp);
    fwrite(&blockAlign, 2, 1, fp);
    fwrite(&bitsPerSample, 2, 1, fp);

    fwrite("data", 1, 4, fp);
    fwrite(&dataSize, 4, 1, fp);
}

/* write a signed 16-bit sample in little-endian */
static void write_i16(FILE *fp, int16_t val)
{
    fwrite(&val, 2, 1, fp);
}

/* generate eat sound: 800Hz sine, 100ms, quick decay */
static void gen_eat(const char *path)
{
    float duration = 0.10f;
    int count = (int)(SAMPLE_RATE * duration);
    uint32_t dataSize = count * 2;

    FILE *fp = NULL;
    fopen_s(&fp, path, "wb");
    if (!fp) { printf("Cannot create %s\n", path); return; }
    write_wav_header(fp, dataSize);

    float freq = 800.0f;
    for (int i = 0; i < count; i++)
    {
        float t = (float)i / SAMPLE_RATE;
        float env = 1.0f - (float)i / count;  /* linear decay */
        float sample = sinf(2.0f * PI * freq * t) * env * 0.8f;
        int16_t val = (int16_t)(sample * 32767);
        write_i16(fp, val);
    }

    fclose(fp);
    printf("Generated: %s\n", path);
}

/* generate die sound: descending sawtooth, 500ms, 300Hz -> 100Hz */
static void gen_die(const char *path)
{
    float duration = 0.50f;
    int count = (int)(SAMPLE_RATE * duration);
    uint32_t dataSize = count * 2;

    FILE *fp = NULL;
    fopen_s(&fp, path, "wb");
    if (!fp) { printf("Cannot create %s\n", path); return; }
    write_wav_header(fp, dataSize);

    float startFreq = 300.0f;
    float endFreq = 80.0f;
    float phase = 0.0f;

    for (int i = 0; i < count; i++)
    {
        float t = (float)i / SAMPLE_RATE;
        float freq = startFreq + (endFreq - startFreq) * (t / duration);
        phase += freq / SAMPLE_RATE;
        if (phase >= 1.0f) phase -= 1.0f;

        float env = 1.0f - t / duration;
        float sample = (phase * 2.0f - 1.0f) * env * 0.6f;
        int16_t val = (int16_t)(sample * 32767);
        write_i16(fp, val);
    }

    fclose(fp);
    printf("Generated: %s\n", path);
}

/* generate menu sound: short tick, 1200Hz sine, 50ms */
static void gen_menu(const char *path)
{
    float duration = 0.05f;
    int count = (int)(SAMPLE_RATE * duration);
    uint32_t dataSize = count * 2;

    FILE *fp = NULL;
    fopen_s(&fp, path, "wb");
    if (!fp) { printf("Cannot create %s\n", path); return; }
    write_wav_header(fp, dataSize);

    float freq = 1200.0f;
    for (int i = 0; i < count; i++)
    {
        float t = (float)i / SAMPLE_RATE;
        float env = 1.0f - (float)i / count;
        float sample = sinf(2.0f * PI * freq * t) * env * 0.5f;
        int16_t val = (int16_t)(sample * 32767);
        write_i16(fp, val);
    }

    fclose(fp);
    printf("Generated: %s\n", path);
}

int main(void)
{
    gen_eat("resource/eat.wav");
    gen_die("resource/die.wav");
    gen_menu("resource/menu.wav");
    printf("All sounds generated.\n");
    return 0;
}
