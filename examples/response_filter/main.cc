#include <glib.h>
#include <iostream>

#include <base/nugu_log.h>
#include <capability/asr_interface.hh>
#include <capability/audio_player_interface.hh>
#include <capability/capability_factory.hh>
#include <capability/system_interface.hh>
#include <capability/text_interface.hh>
#include <capability/tts_interface.hh>
#include <clientkit/nugu_client.hh>

#include "filter.h"

using namespace NuguClientKit;
using namespace NuguCapability;

static std::shared_ptr<NuguClient> nugu_client;
static std::shared_ptr<ITextHandler> text_handler = nullptr;
static std::shared_ptr<IASRHandler> asr_handler = nullptr;

static std::string text_value;
static GMainLoop* loop;

class MyTTSListener : public ITTSListener {
public:
    virtual ~MyTTSListener() = default;

    void onTTSState(TTSState state, const std::string& dialog_id) override
    {
        switch (state) {
        case TTSState::TTS_SPEECH_FINISH:
            std::cout << "TTS Finish" << std::endl;
            g_main_loop_quit(loop);
            break;
        default:
            break;
        }
    }

    void onTTSText(const std::string& text, const std::string& dialog_id) override
    {
        std::cout << "TTS: " << text << std::endl;
    }

    void onTTSCancel(const std::string& dialog_id) override
    {
    }
};

class MyASR : public IASRListener {
public:
    virtual ~MyASR() = default;

    void onState(ASRState state, const std::string& dialog_id)
    {
        switch (state) {
        case ASRState::IDLE:
            std::cout << "ASR Idle" << std::endl;
            break;
        case ASRState::EXPECTING_SPEECH:
            std::cout << "ASR Expecting speech, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRState::LISTENING:
            std::cout << "ASR Listening... Speak please !, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRState::RECOGNIZING:
            std::cout << "ASR Recognizing..., request dialog id: " << dialog_id << std::endl;
            break;
        case ASRState::BUSY:
            std::cout << "ASR Processing..., request dialog id: " << dialog_id << std::endl;
            break;
        }
    }

    void onNone(const std::string& dialog_id)
    {
        std::cout << "ASR no recognition result, request dialog id: " << dialog_id << std::endl;
    }

    void onPartial(const std::string& text, const std::string& dialog_id)
    {
        std::cout << "ASR partial result: " << text << ", request dialog id: " << dialog_id << std::endl;
    }

    void onComplete(const std::string& text, const std::string& dialog_id)
    {
        std::cout << "ASR complete result: " << text << ", request dialog id: " << dialog_id << std::endl;
    }

    void onError(ASRError error, const std::string& dialog_id, bool listen_timeout_fail_beep)
    {
        switch (error) {
        case ASRError::RESPONSE_TIMEOUT:
            std::cout << "ASR response timeout, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRError::LISTEN_TIMEOUT:
            std::cout << "ASR listen timeout, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRError::LISTEN_FAILED:
            std::cout << "ASR listen failed, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRError::RECOGNIZE_ERROR:
            std::cout << "ASR recognition error, request dialog id: " << dialog_id << std::endl;
            break;
        case ASRError::UNKNOWN:
            std::cout << "ASR unknown error, request dialog id: " << dialog_id << std::endl;
            break;
        }
        g_main_loop_quit(loop);
    }

    void onCancel(const std::string& dialog_id)
    {
        std::cout << "ASR canceled, request dialog id: " << dialog_id << std::endl;
    }

    void setExpectSpeechState(bool is_es_state)
    {
        std::cout << "ASR expect speech state: " << is_es_state << std::endl;
    }
};

class MyNetwork : public INetworkManagerListener {
public:
    void onStatusChanged(NetworkStatus status)
    {
        switch (status) {
        case NetworkStatus::DISCONNECTED:
            std::cout << "Network disconnected !" << std::endl;
            g_main_loop_quit(loop);
            break;
        case NetworkStatus::CONNECTED:
            std::cout << "Network connected !" << std::endl;

            if (text_value.size() > 0) {
                std::cout << "Send the text command: " << text_value << std::endl;
                text_handler->requestTextInput(text_value);
            } else {
                std::cout << "Start ASR Recognition !" << std::endl;
                asr_handler->startRecognition(ASRInitiator::TAP, [&](const std::string& dialog_id) {
                    std::cout << "ASR request dialog id: " << dialog_id << std::endl;
                });
            }
            break;
        case NetworkStatus::CONNECTING:
            std::cout << "Network connecting..." << std::endl;
            break;
        default:
            break;
        }
    }

    void onError(NetworkError error)
    {
        switch (error) {
        case NetworkError::FAILED:
            std::cout << "Network failed !" << std::endl;
            break;
        case NetworkError::TOKEN_ERROR:
            std::cout << "Token error !" << std::endl;
            break;
        case NetworkError::UNKNOWN:
            std::cout << "Unknown error !" << std::endl;
            break;
        }
    }
};

int main(int argc, char* argv[])
{
    if (getenv("NUGU_TOKEN") == NULL) {
        std::cout << "Please set the token using the NUGU_TOKEN environment variable." << std::endl;
        return -1;
    }

    std::cout << "Filtering example!" << std::endl;

    if (argc == 2) {
        text_value = argv[1];
        std::cout << " - Text filtering: " << text_value << std::endl;
    } else {
        std::cout << " - ASR filtering" << std::endl;
    }

    /* Turn off the SDK internal log */
    nugu_log_set_system(NUGU_LOG_SYSTEM_NONE);

    nugu_client = std::make_shared<NuguClient>();

    /* Create System, AudioPlayer capability default */
    auto system_handler(std::shared_ptr<ISystemHandler>(
        CapabilityFactory::makeCapability<SystemAgent, ISystemHandler>()));
    auto audio_player_handler(std::shared_ptr<IAudioPlayerHandler>(
        CapabilityFactory::makeCapability<AudioPlayerAgent, IAudioPlayerHandler>()));

    /* Create a Text capability */
    text_handler = std::shared_ptr<ITextHandler>(
        CapabilityFactory::makeCapability<TextAgent, ITextHandler>());

    /* Create an ASR capability with model file path */
    auto my_asr_listener(std::make_shared<MyASR>());
    asr_handler = std::shared_ptr<IASRHandler>(
        CapabilityFactory::makeCapability<ASRAgent, IASRHandler>(my_asr_listener.get()));
    asr_handler->setAttribute(ASRAttribute { "/var/lib/nugu/model", "CLIENT", "PARTIAL" });

    /* Create a TTS capability */
    auto tts_listener(std::make_shared<MyTTSListener>());
    auto tts_handler(std::shared_ptr<ITTSHandler>(
        CapabilityFactory::makeCapability<TTSAgent, ITTSHandler>(tts_listener.get())));

    filter_register(nugu_client.get());

    /* Register build-in capabilities */
    nugu_client->getCapabilityBuilder()
        ->add(system_handler.get())
        ->add(audio_player_handler.get())
        ->add(tts_handler.get())
        ->add(text_handler.get())
        ->add(asr_handler.get())
        ->construct();

    if (!nugu_client->initialize()) {
        std::cout << "SDK Initialization failed." << std::endl;
        return -1;
    }

    /* Network manager */
    auto network_manager_listener(std::make_shared<MyNetwork>());

    auto network_manager(nugu_client->getNetworkManager());
    network_manager->addListener(network_manager_listener.get());
    network_manager->setToken(getenv("NUGU_TOKEN"));
    network_manager->connect();

    /* Start GMainLoop */
    loop = g_main_loop_new(NULL, FALSE);

    std::cout << "Start the eventloop" << std::endl
              << std::endl;
    g_main_loop_run(loop);

    /* wait until g_main_loop_quit() */

    g_main_loop_unref(loop);

    filter_remove();

    nugu_client->deInitialize();

    return 0;
}
