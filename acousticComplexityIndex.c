/*
 * A simple implementation of the acoustic complexity index algorithm.
 * Author:  Samuel Raeburn
 * Contact: raeburnsamuel@gmail.com
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include "kiss_fftr.h"

#define FFT_N           (1024)
#define TEMPORAL_STEP   (5)

typedef struct
{
    char chunkId[4];        /* Contains "RIFF" in ASCII */
    uint32_t chunkSize;     /* 36 + subchunk2Size */
    char format[4];         /* Contains "WAVE" in ASCII */
    char subchunk1Id[4];    /* Contains "fmt " in ASCII */
    uint32_t subchunk1Size; /* Remaining size of subchunk 1 */
    uint16_t audioFormat;   /* PCM = 1 */
    uint16_t numChannels;   /* Mono = 1, Stereo = 2, etc. */
    uint32_t sampleRate;    /* 44.1kHz, 8khz etc. */
    uint32_t byteRate;      /* sampleRate * numChannels * bitsPerSample / 8 */
    uint16_t blockAlign;    /* numChannels * bitsPersample/8 */
    uint16_t bitsPerSample; /* Number of bits per sample */
    char subchunk2Id[4];    /* Contains "data" in ASCII */
    uint32_t subchunk2Size; /* Number of bytes in the data */
} WavHeader;

/* Path to the .wav file */
static char wavFilePath[128];
/* If true, dumps out information in wavHeader before continuing */
static bool dumpWavHeader = false;

static void printUsage(void)
{
    printf("Usage: acousticComplexityIndex\n");
    printf("        -f [wavFile]        - .wav file to be analysed\n");
    printf("        -d                  - dump .wav header data to stdout\n");
    printf("        -h                  - print this help information\n");
}

static int processCmdline(int argc, char *argv[])
{
    int rc = -1;
    int arg;

    assert(argv != NULL);
    assert(*argv != NULL);

    while ((arg = getopt(argc, argv, "f:dh")) != -1)
    {
        switch (arg)
        {
            case 'f':
                /* Path to wav file */
                strncpy(wavFilePath, optarg, sizeof(wavFilePath));
                rc = 0;
                break;
            case 'd':
                /* Dump wav header info */
                dumpWavHeader = true;
                break;
            case 'h':
                /* Print help information */
                printUsage();
                break;
            default:
                break;
        }
    }

    return rc;
}

static void printWavHeader(WavHeader *wavHeader)
{
    assert(wavHeader != NULL);

    printf("Wav Header:\n");
    printf("\tchunkId:          '%c%c%c%c'\n", wavHeader->chunkId[0], wavHeader->chunkId[1], wavHeader->chunkId[2], wavHeader->chunkId[3]);
    printf("\tchunkSize:        %u\n", wavHeader->chunkSize);
    printf("\tformat:           '%c%c%c%c'\n", wavHeader->format[0], wavHeader->format[1], wavHeader->format[2], wavHeader->format[3]);
    printf("\tsubChunk1Id:      '%c%c%c%c'\n", wavHeader->subchunk1Id[0], wavHeader->subchunk1Id[1], wavHeader->subchunk1Id[2], wavHeader->subchunk1Id[3]);
    printf("\tsubChunk1Size:    %u\n", wavHeader->subchunk1Size);
    printf("\taudioFormat:      %u\n", wavHeader->audioFormat);
    printf("\tnumChannels:      %u\n", wavHeader->numChannels);
    printf("\tsampleRate:       %u\n", wavHeader->sampleRate);
    printf("\tbyteRate:         %u\n", wavHeader->byteRate);
    printf("\tblockAlign:       %u\n", wavHeader->blockAlign);
    printf("\tbitsPerSample:    %u\n", wavHeader->bitsPerSample);
    printf("\tsubChunk2Id:      '%c%c%c%c'\n", wavHeader->subchunk2Id[0], wavHeader->subchunk2Id[1], wavHeader->subchunk2Id[2], wavHeader->subchunk2Id[3]);
    printf("\tsubChunk2Size:    %u\n", wavHeader->subchunk2Size);
}

static int obtainWavHeader(WavHeader* wavHeader)
{
    int rc = -1;
    FILE *fp;

    assert(wavHeader != NULL);

    /* Open the wav file file for reading */
    if ((fp = fopen(wavFilePath, "r")) == NULL)
    {
        fprintf(stderr, "Failed to open '%s' (err: %d - '%s')\n", wavFilePath, errno, strerror(errno));
    }
    else
    {
        size_t read;

        /* Read the wav header */
        read = fread(wavHeader, 1, sizeof(WavHeader), fp);

        if (read != sizeof(WavHeader))
        {
            fprintf(stderr, "Failed to read wav header from '%s' (read: %lu, expected %lu)\n",
                wavFilePath, read, sizeof(wavHeader));
        }
        /* Ensure that this is a valid .wav file */
        else if (strncmp(wavHeader->chunkId, "RIFF", 4) != 0)
        {
            fprintf(stderr, "File '%s' doesn't appear to be a .wav file\n", wavFilePath);
        }
        else
        {
            if (dumpWavHeader)
            {
                printWavHeader(wavHeader);
            }
            rc = 0;
        }
        fclose(fp);
    }

    return rc;
}

static int readSamples(FILE *fp, int16_t *buffer, size_t len)
{
    int rc = 0;

    assert(buffer != NULL);

    if (fread(buffer, 1, len, fp) != len)
    {
        fprintf(stderr, "Failed to read %zu bytes from '%s' (err: %d - '%s')\n", len, wavFilePath, errno, strerror(errno));
        rc = -1;
    }

    return rc;
}

static int obtainFftMagnitudeData(int16_t *samples, float *magnitudes)
{
    int rc = -1;
    size_t i;
    kiss_fft_scalar fftIn[FFT_N];
    kiss_fftr_cfg fftConfig;

    assert(samples != NULL);
    assert(magnitudes != NULL);

    /* Store the samples in the FFT input scalar */
    for (i = 0; i < FFT_N; i++)
    {
        fftIn[i] = samples[i];
    }

    /* Initialise the FFT */
    if ((fftConfig = kiss_fftr_alloc(FFT_N, 0, NULL, NULL)) == NULL)
    {
        fprintf(stderr, "Failed to initialise FFT\n");
    }
    else
    {
        /* Don't care about the output past Nyquist frequency, hence array size of FFT_N / 2 + 1 */
        kiss_fft_cpx fftOut[FFT_N / 2 + 1];

        /* Perform the FFT */
        kiss_fftr(fftConfig, fftIn, fftOut);
        free(fftConfig);

        /* Compute magnitude of each output point and store in the matrix */
        for (i = 0; i < FFT_N / 2 + 1; i++)
        {
            magnitudes[i] = sqrt((fftOut[i].r * fftOut[i].r) + (fftOut[i].i * fftOut[i].i));
        }

        rc = 0;
    }

    return rc;
}

static void destroyMagnitudeMatrix(float ***magnitudeMatrix, WavHeader *wavHeader)
{
    size_t i;

    assert(wavHeader != NULL);

    for (i = 0; i < ((unsigned int)(wavHeader->subchunk2Size / FFT_N)); i++)
    {
        free((*magnitudeMatrix)[i]);
    }
    free(*magnitudeMatrix);
    *magnitudeMatrix = NULL;
}

static float **createMagnitudeMatrix(WavHeader *wavHeader)
{
    float **magnitudeMatrix = NULL;
    size_t i;
    unsigned int numFfts;

    assert(wavHeader != NULL);

    /* Matrix needs to be big enough to store every FFT */
    numFfts = wavHeader->subchunk2Size / FFT_N;

    //magnitudeMatrix = calloc(numFfts, sizeof(float *));
    magnitudeMatrix = malloc(numFfts * sizeof(float *));
    for (i = 0; i < numFfts; i++)
    {
        //magnitudeMatrix[i] = calloc((FFT_N / 2 + 1), sizeof(float));
        magnitudeMatrix[i] = malloc((FFT_N / 2 + 1) * sizeof(float));
    }

    return magnitudeMatrix;
}

static int populateMagnitudeMatrix(WavHeader *wavHeader, float **magnitudeMatrix)
{
    int rc = 0;
    FILE *fp;

    assert(wavHeader != NULL);
    assert(magnitudeMatrix != NULL);
    assert(*magnitudeMatrix != NULL);

    /* Open wav file for reading */
    if ((fp = fopen(wavFilePath, "r")) == NULL)
    {
        fprintf(stderr, "Failed to open '%s' for reading (err: %d - '%s')\n", wavFilePath, errno, strerror(errno));
        rc = -1;
    }
    /* Seek past the wav header */
    else if (fseek(fp, sizeof(WavHeader), SEEK_SET) != 0)
    {
        fprintf(stderr, "Failed to seek within '%s' (err: %d - '%s')\n", wavFilePath, errno, strerror(errno));
        rc = -1;
    }
    else
    {
        unsigned int numFfts = 0;
        int16_t samples[FFT_N];

        /* Populate the entire matrix */
        while (numFfts++ < (wavHeader->subchunk2Size / FFT_N) - 1)
        {
            memset(magnitudeMatrix[numFfts], 0, FFT_N / 2 + 1);
            if (readSamples(fp, samples, FFT_N) != 0)
            {
                fprintf(stderr, "Failed to read %d samples from '%s'\n", FFT_N, wavFilePath);
                rc = -1;
                break;
            }
            else if (obtainFftMagnitudeData(samples, magnitudeMatrix[numFfts]) != 0)
            {
                fprintf(stderr, "Failed to obtain FFT magnitude data from '%s'\n", wavFilePath);
                rc = -1;
                break;
            }
        }
    }

    if (fp != NULL)
    {
        fclose(fp);
    }

    return rc;
}

/*
 * The calculation performed by this function is described in more detail in
 * "A new methodology to infer the singing activity of an avian community: The Acoustic Complexity Index (ACI)"
 * by N. Pieretti, A. Farina and D. Morri.
 */
static float obtainAciValue(float **magnitudeMatrix, WavHeader *wavHeader)
{
    float aciTotal = 0;     /* Total ACI value for the recording */
    unsigned int numFftsInTemporalStep;
    unsigned int numTemporalStepsInRecording;

    assert(magnitudeMatrix != NULL);
    assert(wavHeader != NULL);

    numFftsInTemporalStep = (TEMPORAL_STEP * wavHeader->sampleRate) / FFT_N;
    numTemporalStepsInRecording = (wavHeader->subchunk2Size / wavHeader->sampleRate) / TEMPORAL_STEP;

    /* For each frequency bin the FFT */
    for (size_t q = 0; q < FFT_N / 2 + 1; q++)
    {
        float aciTempStep = 0;  /* ACI value for a single temporal step */

        /* For every temporal step within the recording */
        for (size_t m = 0; m < numTemporalStepsInRecording; m++)
        {
            float D = 0;    /* Absolute difference between 2 adjacent intensities, within a single frequency bin */
            float sum = 0;  /* sum of all 'D' within a temporal step */
            int startingFft;

            startingFft = m * numFftsInTemporalStep;

            /* For each FFT within a single temporal step */
            for (size_t k = 0; k < numFftsInTemporalStep - 2; k++)
            {
                float dk = 0;

                /* Obtain absolute difference between two adjacent intensities */
                dk = fabs(magnitudeMatrix[startingFft + k][q] - magnitudeMatrix[startingFft + k + 1][q]);
                /* Keep a track of the sum of all of these differences within a temporal step */
                D += dk;
                /* Keep track of the sum of all of the intensities */
                sum += magnitudeMatrix[startingFft + k][q];
            }
            /* ACI for a single temporal step */
            aciTempStep += D / sum;
        }
        /* Total ACI */
        aciTotal += aciTempStep;
    }

    return aciTotal;
}

int main (int argc, char *argv[])
{
    int rc = -1;
    WavHeader wavHeader;
    float **magnitudeMatrix = NULL;

    if (processCmdline(argc, argv) != 0)
    {
        fprintf(stderr, "Failed to process command line arguments\n");
        printUsage();
    }
    else if (obtainWavHeader(&wavHeader) != 0)
    {
        fprintf(stderr, "Failed to obtain wav header of '%s'\n", wavFilePath);
    }
    else if ((magnitudeMatrix = createMagnitudeMatrix(&wavHeader)) == NULL)
    {
        fprintf(stderr, "Failed to create matrix of magnitudes\n");
    }
    else if (populateMagnitudeMatrix(&wavHeader, magnitudeMatrix) != 0)
    {
        fprintf(stderr, "Failed to populate matrix\n");
    }
    /* Have entire matrix */
    else
    {
        float ACI = 0;

        ACI = obtainAciValue(magnitudeMatrix, &wavHeader);
        printf("ACI of '%s' with temporal step '%d' and FFT_N '%d' = %f\n",
                wavFilePath, TEMPORAL_STEP, FFT_N, ACI);
        rc = 0;
    }

    if (magnitudeMatrix != NULL)
    {
        destroyMagnitudeMatrix(&magnitudeMatrix, &wavHeader);
    }

    return rc;
}
