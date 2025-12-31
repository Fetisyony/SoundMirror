#include <jni.h>
#include <vector>
#include "libopus/include/opus.h"


OpusDecoder* getDecoder(jlong ptr) {
    return reinterpret_cast<OpusDecoder*>(ptr);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_bacorp_soundmirror_streamingservice_data_OpusWrapper_nativeInit(JNIEnv *env, jobject thiz, jint sampleRate, jint channels) {
    int error;
    OpusDecoder* dec = opus_decoder_create(sampleRate, channels, &error);
    return reinterpret_cast<jlong>(dec);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_bacorp_soundmirror_streamingservice_data_OpusWrapper_nativeDecode(
    JNIEnv *env, jobject thiz,
     jlong decoderPtr,
     jbyteArray encodedData,
     jint offset,
     jint len,
     jfloatArray outputData,
     jint frameSize
) {
    OpusDecoder* decoder = getDecoder(decoderPtr);
    if (!decoder) return -1;

    jbyte *bytePtr = env->GetByteArrayElements(encodedData, nullptr);
    jfloat *outPtr = env->GetFloatArrayElements(outputData, nullptr);

    int samples = opus_decode_float(
            decoder,
            (const unsigned char *)(bytePtr + offset),
            len,
            outPtr,
            frameSize,
            0
    );

    env->ReleaseByteArrayElements(encodedData, bytePtr, JNI_ABORT);
    env->ReleaseFloatArrayElements(outputData, outPtr, 0);

    return samples;
}

extern "C" JNIEXPORT void JNICALL
Java_com_bacorp_soundmirror_streamingservice_data_OpusWrapper_nativeClose(JNIEnv *env, jobject thiz, jlong decoderPtr) {
    OpusDecoder* decoder = getDecoder(decoderPtr);
    if (decoder) {
        opus_decoder_destroy(decoder);
    }
}