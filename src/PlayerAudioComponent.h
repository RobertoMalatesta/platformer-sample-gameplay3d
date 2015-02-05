#ifndef GAME_AUDIO_COMPONENT_H
#define GAME_AUDIO_COMPONENT_H

#include "Component.h"

namespace game
{
    class PlayerComponent;

    /**
     * Listens for audible player messages and triggers playback of audio sources
     *
     * @script{ignore}
    */
    class PlayerAudioComponent : public gameobjects::Component
    {
    public:
        explicit PlayerAudioComponent();
        ~PlayerAudioComponent();
    protected:
        virtual void initialize() override;
        virtual void finalize() override;
        virtual void onMessageReceived(gameobjects::Message * message, int messageType) override;
        virtual void readProperties(gameplay::Properties & properties) override;
    private:
        PlayerAudioComponent(PlayerAudioComponent const &);

        void addAudioNode(std::string const & audioSourcePath);
        std::string _jumpAudioSourcePath;
        std::map<std::string, gameplay::Node *> _audioNodes;
        PlayerComponent * _player;
    };
}

#endif
