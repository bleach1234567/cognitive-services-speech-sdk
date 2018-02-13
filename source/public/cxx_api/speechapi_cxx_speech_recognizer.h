//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//
// speechapi_cxx_speech_recognizer.h: Public API declarations for SpeechRecognizer C++ class
//

#pragma once
#include <exception>
#include <future>
#include <memory>
#include <string>
#include <speechapi_c.h>
#include <speechapi_cxx_common.h>
#include <speechapi_cxx_recognition_async_recognizer.h>
#include <speechapi_cxx_speech_recognition_eventargs.h>
#include <speechapi_cxx_speech_recognition_result.h>


namespace CARBON_NAMESPACE_ROOT {
namespace Recognition {
namespace Speech {


class SpeechRecognizer final : virtual public AsyncRecognizer<SpeechRecognitionResult, SpeechRecognitionEventArgs>
{
public:

    SpeechRecognizer() : 
        AsyncRecognizer(m_recoParameters),
        m_hreco(SPXHANDLE_INVALID),
        m_hasyncRecognize(SPXHANDLE_INVALID),
        m_hasyncStartContinuous(SPXHANDLE_INVALID),
        m_hasyncStopContinuous(SPXHANDLE_INVALID)
    {
        throw nullptr;
    };

    SpeechRecognizer(const std::wstring& language) : 
        AsyncRecognizer(m_recoParameters),
        m_hreco(SPXHANDLE_INVALID),
        m_hasyncRecognize(SPXHANDLE_INVALID),
        m_hasyncStartContinuous(SPXHANDLE_INVALID),
        m_hasyncStopContinuous(SPXHANDLE_INVALID)
    {
        UNUSED(language);
        throw nullptr;
    };

    SpeechRecognizer(SPXRECOHANDLE hreco) :
        AsyncRecognizer(m_recoParameters),
        m_hreco(hreco),
        m_hasyncRecognize(SPXHANDLE_INVALID),
        m_hasyncStartContinuous(SPXHANDLE_INVALID),
        m_hasyncStopContinuous(SPXHANDLE_INVALID)
    {
        SPX_DBG_TRACE_FUNCTION();
    }

    ~SpeechRecognizer()
    {
        SPX_DBG_TRACE_FUNCTION();

        if (m_hreco != SPXHANDLE_INVALID)
        {
            ::Recognizer_Handle_Close(m_hreco);
            m_hreco = SPXHANDLE_INVALID;
        }

        if (m_hreco != SPXHANDLE_INVALID)
        {
            ::Recognizer_AsyncHandle_Close(m_hasyncRecognize);
            m_hasyncRecognize = SPXHANDLE_INVALID;
        }

        if (m_hreco != SPXHANDLE_INVALID)
        {
            ::Recognizer_AsyncHandle_Close(m_hasyncStartContinuous);
            m_hasyncStartContinuous = SPXHANDLE_INVALID;
        }

        if (m_hreco != SPXHANDLE_INVALID)
        {
            ::Recognizer_AsyncHandle_Close(m_hasyncStopContinuous);
            m_hasyncStopContinuous = SPXHANDLE_INVALID;
        }
    };

    bool IsEnabled() override 
    {
        bool enabled = false;
        SPX_INIT_HR(hr);
        SPX_THROW_ON_FAIL(hr = Recognizer_IsEnabled(m_hreco, &enabled));
        return enabled;
    };

    void Enable() override
    {
        SPX_INIT_HR(hr);
        SPX_THROW_ON_FAIL(hr = Recognizer_Enable(m_hreco));
    };

    void Disable() override
    {
        SPX_INIT_HR(hr);
        SPX_THROW_ON_FAIL(hr = Recognizer_Disable(m_hreco));
    };

    std::future<std::shared_ptr<SpeechRecognitionResult>> RecognizeAsync() override
    {
        auto future = std::async(std::launch::async, [=]() -> std::shared_ptr<SpeechRecognitionResult> {
            SPX_INIT_HR(hr);

            SPXRESULTHANDLE hresult = SPXHANDLE_INVALID;
            SPX_THROW_ON_FAIL(hr = Recognizer_Recognize(m_hreco, &hresult));

            return std::make_shared<SpeechRecognitionResult>(hresult);
        });

        return future;
    };

    std::future<void> StartContinuousRecognitionAsync() override 
    {
        auto future = std::async(std::launch::async, [=]() -> void {
            SPX_INIT_HR(hr);
            SPX_THROW_ON_FAIL(hr = Recognizer_AsyncHandle_Close(m_hasyncStartContinuous)); // close any unfinished previous attempt

            SPX_EXITFN_ON_FAIL(hr = Recognizer_StartContinuousRecognitionAsync(m_hreco, &m_hasyncStartContinuous));
            SPX_EXITFN_ON_FAIL(hr = Recognizer_StartContinuousRecognitionAsync_WaitFor(m_hasyncStartContinuous, UINT32_MAX));

            SPX_EXITFN_CLEANUP:
            SPX_REPORT_ON_FAIL(/* hr = */ Recognizer_AsyncHandle_Close(m_hasyncStartContinuous)); // don't overwrite HR on cleanup
            m_hasyncStartContinuous = SPXHANDLE_INVALID;

            SPX_THROW_ON_FAIL(hr);
        });

        return future;
    };

    std::future<void> StopContinuousRecognitionAsync() override
    {
        auto future = std::async(std::launch::async, [=]() -> void {
            SPX_INIT_HR(hr);
            SPX_THROW_ON_FAIL(hr = Recognizer_AsyncHandle_Close(m_hasyncStopContinuous)); // close any unfinished previous attempt

            SPX_EXITFN_ON_FAIL(hr = Recognizer_StopContinuousRecognitionAsync(m_hreco, &m_hasyncStopContinuous));
            SPX_EXITFN_ON_FAIL(hr = Recognizer_StopContinuousRecognitionAsync_WaitFor(m_hasyncStopContinuous, UINT32_MAX));

            SPX_EXITFN_CLEANUP:
            SPX_REPORT_ON_FAIL(/* hr = */ Recognizer_AsyncHandle_Close(m_hasyncStopContinuous)); // don't overwrite HR on cleanup
            m_hasyncStartContinuous = SPXHANDLE_INVALID;

            SPX_THROW_ON_FAIL(hr);
        });

        return future;
    };


protected:

    void RecoEventConnectionsChanged(const EventSignal<const SpeechRecognitionEventArgs&>& recoEvent) override
    {
        if (&recoEvent == &IntermediateResult)
        {
            Recognizer_IntermediateResult_SetEventCallback(m_hreco, IntermediateResult.IsConnected() ? SpeechRecognizer::FireEvent_IntermediateResult: nullptr, this);
        }
        else if (&recoEvent == &FinalResult)
        {
            Recognizer_FinalResult_SetEventCallback(m_hreco, FinalResult.IsConnected() ? SpeechRecognizer::FireEvent_FinalResult: nullptr, this);
        }
        else if (&recoEvent == &NoMatch)
        {
            Recognizer_NoMatch_SetEventCallback(m_hreco, NoMatch.IsConnected() ? SpeechRecognizer::FireEvent_NoMatch : nullptr, this);
        }
        else if (&recoEvent == &Canceled)
        {
            Recognizer_Canceled_SetEventCallback(m_hreco, Canceled.IsConnected() ? SpeechRecognizer::FireEvent_Canceled : nullptr, this);
        }
    }

    void SessionEventConnectionsChanged(const EventSignal<const SessionEventArgs&>& sessionEvent) override
    {
        if (&sessionEvent == &SessionStarted)
        {
            Recognizer_SessionStarted_SetEventCallback(m_hreco, SessionStarted.IsConnected() ? SpeechRecognizer::FireEvent_SessionStarted: nullptr, this);
        }
        else if (&sessionEvent == &SessionStopped)
        {
            Recognizer_SessionStopped_SetEventCallback(m_hreco, SessionStopped.IsConnected() ? SpeechRecognizer::FireEvent_SessionStopped : nullptr, this);
        }
        else if (&sessionEvent == &SoundStarted)
        {
            //Recognizer_SoundStarted_SetEventCallback(m_hreco, SoundStarted.IsConnected() ? SpeechRecognizer::FireEvent_SoundStarted: nullptr, this);
        }
        else if (&sessionEvent == &SoundStopped)
        {
            //Recognizer_SoundStopped_SetEventCallback(m_hreco, SoundStopped.IsConnected() ? SpeechRecognizer::FireEvent_SoundStopped: nullptr, this);
        }
    }

    static void FireEvent_SessionStarted(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto sessionEvent = std::make_unique<SessionEventArgs>(hevent);
        std::unique_ptr<SessionEventArgs> sessionEvent{ new SessionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->SessionStarted.Signal(*sessionEvent.get());
    }

    static void FireEvent_SessionStopped(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto sessionEvent = std::make_unique<SessionEventArgs>(hevent);
        std::unique_ptr<SessionEventArgs> sessionEvent{ new SessionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->SessionStopped.Signal(*sessionEvent.get());
    }

    static void FireEvent_IntermediateResult(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto recoEvent = std::make_unique<SpeechRecognitionEventArgs>(hevent);
        std::unique_ptr<SpeechRecognitionEventArgs> recoEvent{ new SpeechRecognitionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->IntermediateResult.Signal(*recoEvent.get());
    }

    static void FireEvent_FinalResult(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto recoEvent = std::make_unique<SpeechRecognitionEventArgs>(hevent);
        std::unique_ptr<SpeechRecognitionEventArgs> recoEvent{ new SpeechRecognitionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->FinalResult.Signal(*recoEvent.get());
    }

    static void FireEvent_NoMatch(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto recoEvent = std::make_unique<SpeechRecognitionEventArgs>(hevent);
        std::unique_ptr<SpeechRecognitionEventArgs> recoEvent{ new SpeechRecognitionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->NoMatch.Signal(*recoEvent.get());
    }

    static void FireEvent_Canceled(SPXRECOHANDLE hreco, SPXEVENTHANDLE hevent, void* pvContext)
    {
        UNUSED(hreco);
        // need C++14 for make_unique
        // auto recoEvent = std::make_unique<SpeechRecognitionEventArgs>(hevent);
        std::unique_ptr<SpeechRecognitionEventArgs> recoEvent{ new SpeechRecognitionEventArgs(hevent) };
        auto pThis = static_cast<SpeechRecognizer*>(pvContext);
        pThis->Canceled.Signal(*recoEvent.get());
    }


private:

    SpeechRecognizer(const SpeechRecognizer&) = delete;
    SpeechRecognizer(const SpeechRecognizer&&) = delete;

    SpeechRecognizer& operator=(const SpeechRecognizer&) = delete;

    SPXRECOHANDLE m_hreco;
    SPXASYNCHANDLE m_hasyncRecognize;
    SPXASYNCHANDLE m_hasyncStartContinuous;
    SPXASYNCHANDLE m_hasyncStopContinuous;

    RecognizerParameters m_recoParameters; 
};


} } } // CARBON_NAMESPACE_ROOT :: Recognition :: Speech
