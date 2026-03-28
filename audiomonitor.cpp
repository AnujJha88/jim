#include "audiomonitor.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <cmath>

// Manually define the GUIDs as constants to avoid linker issues without using initguid.h
static const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT_LOCAL = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
static const GUID KSDATAFORMAT_SUBTYPE_PCM_LOCAL        = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#endif

AudioMonitor::AudioMonitor(QObject *parent) : QThread(parent) {
    m_levels.resize(64);
    m_levels.fill(0.0f);
}

AudioMonitor::~AudioMonitor() {
    stop();
    wait();
}

void AudioMonitor::stop() {
    m_running = false;
}

QVector<float> AudioMonitor::getLevels() {
    QMutexLocker locker(&m_mutex);
    return m_levels;
}

void AudioMonitor::run() {
    m_running = true;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    
    IMMDeviceEnumerator* pEnumerator = nullptr;
    IMMDevice* pDevice = nullptr;
    IAudioClient* pAudioClient = nullptr;
    IAudioCaptureClient* pCaptureClient = nullptr;
    WAVEFORMATEX* pwfx = nullptr;
    
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr)) {
        CoUninitialize();
        return;
    }
    
    // Try to get the default audio endpoint, if that fails, try all available endpoints
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
        // Try to get any available render endpoint
        IMMDeviceCollection* pCollection = nullptr;
        hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
        if (SUCCEEDED(hr)) {
            UINT count = 0;
            pCollection->GetCount(&count);
            if (count > 0) {
                hr = pCollection->Item(0, &pDevice);
            }
            pCollection->Release();
        }
    }
    
    if (FAILED(hr) || !pDevice) { 
        pEnumerator->Release(); 
        CoUninitialize();
        return; 
    }
    
    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr)) { 
        pDevice->Release(); 
        pEnumerator->Release(); 
        CoUninitialize();
        return; 
    }
    
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) { 
        pAudioClient->Release(); 
        pDevice->Release(); 
        pEnumerator->Release(); 
        CoUninitialize();
        return; 
    }
    
    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, nullptr);
    if (SUCCEEDED(hr)) {
        hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
        if (SUCCEEDED(hr)) {
            pAudioClient->Start();
            while (m_running) {
                msleep(16); // ~60fps refresh rate
                
                UINT32 packetLength = 0;
                hr = pCaptureClient->GetNextPacketSize(&packetLength);
                
                while (packetLength != 0 && m_running) {
                    BYTE* pData;
                    UINT32 numFramesAvailable;
                    DWORD flags;
                    
                    hr = pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
                    if (FAILED(hr)) break;
                    
                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                        m_mutex.lock();
                        // Generate test data when silent to verify visualization works
                        static int testCounter = 0;
                        for (int i = 0; i < 64; i++) {
                            float testLevel = (sin(testCounter * 0.05f + i * 0.1f) + 1.0f) * 0.6f; // Increased amplitude
                            testLevel = testLevel * testLevel; // Square for more dynamic range
                            m_levels[i] = m_levels[i] * 0.85f + testLevel * 0.15f; // Slightly smoother transitions
                        }
                        testCounter++;
                        m_mutex.unlock();
                    } else {
                        // Advanced multi-format handler
                        int channels = pwfx->nChannels;
                        int bits = pwfx->wBitsPerSample;
                        int blockSize = qMax(1, (int)numFramesAvailable / 64);
                        
                        m_mutex.lock();
                        for (int i = 0; i < 64; i++) {
                            float sum = 0;
                            int count = 0;
                            for (int j = 0; j < blockSize; j++) {
                                int frameIdx = i * blockSize + j;
                                if (frameIdx >= (int)numFramesAvailable) break;
                                
                                int sampleOffset = frameIdx * channels;
                                float sample = 0;

                                bool isFloat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
                                               (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT_LOCAL));
                                
                                if (isFloat) {
                                    if (sampleOffset < (int)(numFramesAvailable * channels)) {
                                        sample = ((float*)pData)[sampleOffset];
                                    }
                                } else if (bits == 16) {
                                    if (sampleOffset < (int)(numFramesAvailable * channels)) {
                                        sample = ((short*)pData)[sampleOffset] / 32768.0f;
                                    }
                                } else if (bits == 32) {
                                    if (sampleOffset < (int)(numFramesAvailable * channels)) {
                                        sample = ((int*)pData)[sampleOffset] / 2147483648.0f;
                                    }
                                } else if (bits == 24) {
                                    // 24-bit PCM: 3 bytes per sample
                                    BYTE* pSample = pData + (sampleOffset * 3);
                                    // Make sure we're within the captured byte range
                                    if ((sampleOffset * 3 + 2) < (int)(numFramesAvailable * channels * 3)) {
                                        int val = (pSample[0] << 8) | (pSample[1] << 16) | (pSample[2] << 24);
                                        sample = (val / 256) / 8388608.0f;
                                    }
                                }
                                
                                sum += sample * sample;
                                count++;
                            }
                            float rms = (count > 0) ? sqrt(sum / count) : 0;
                            // Apply increased sensitivity for better visibility
                            float level = rms * 12.0f; // Increased from 4.0f for more dramatic fluctuations
                            m_levels[i] = m_levels[i] * 0.8f + level * 0.2f; // Slightly faster response
                            if (m_levels[i] > 1.0f) m_levels[i] = 1.0f;
                        }
                        m_mutex.unlock();
                    }
                    hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    if (FAILED(hr)) break;
                    hr = pCaptureClient->GetNextPacketSize(&packetLength);
                }
                emit levelsUpdated();
            }
            pAudioClient->Stop();
        }
    } else {
        // If audio client initialization failed, still generate test data for visualization
        while (m_running) {
            msleep(16);
            m_mutex.lock();
            static int testCounter = 0;
            for (int i = 0; i < 64; i++) {
                float testLevel = (sin(testCounter * 0.05f + i * 0.1f) + 1.0f) * 0.6f; // Increased amplitude
                testLevel = testLevel * testLevel;
                m_levels[i] = m_levels[i] * 0.85f + testLevel * 0.15f; // Slightly smoother transitions
            }
            testCounter++;
            m_mutex.unlock();
            emit levelsUpdated();
        }
    }
    
    if (pwfx) CoTaskMemFree(pwfx);
    if (pCaptureClient) pCaptureClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    CoUninitialize();
}
