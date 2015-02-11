#include "PlayerAudioComponent.h"

#include "AudioSource.h"
#include "Common.h"
#include "GameObjectController.h"
#include "Messages.h"
#include "PlayerComponent.h"

namespace game
{
    PlayerAudioComponent::PlayerAudioComponent()
        : _player(nullptr)
    {
    }

    PlayerAudioComponent::~PlayerAudioComponent()
    {
    }

    void PlayerAudioComponent::initialize()
    {
        _player = getParent()->getComponent<PlayerComponent>();
        _player->addRef();
        addAudioNode(_jumpAudioSourcePath);
    }

    void PlayerAudioComponent::finalize()
    {
        for(auto & pair : _audioNodes)
        {
            gameplay::Node * node = pair.second;

            if(node->getParent())
            {
                node->getParent()->removeChild(node);
            }

            GAME_ASSERT_SINGLE_REF(node);
            SAFE_RELEASE(node);
        }

        SAFE_RELEASE(_player);
    }

    void PlayerAudioComponent::addAudioNode(std::string const & audioSourcePath)
    {
        PERF_SCOPE("PlayerAudioComponent::addAudioNode");
        gameplay::Node * node = gameplay::Node::create();
        gameplay::AudioSource * audioSource = nullptr;
        {
            STALL_SCOPE();
            audioSource = gameplay::AudioSource::create(audioSourcePath.c_str());
        }
        node->setAudioSource(audioSource);
        _player->getParent()->getNode()->addChild(node);
        // Play it silently so that it performs lazy initialisation
        audioSource->setGain(0.0f);
        audioSource->play();
        SAFE_RELEASE(audioSource);
        _audioNodes[audioSourcePath] = node;
    }

    bool PlayerAudioComponent::onMessageReceived(gameobjects::Message * message, int messageType)
    {
        switch (messageType)
        {
        case(Messages::Type::PlayerJump) :
            {
                gameplay::Node * audioNode = _audioNodes[_jumpAudioSourcePath];
                audioNode->setTranslation(_player->getCharacterNode()->getTranslation());
                gameplay::AudioSource * source = audioNode->getAudioSource();
                source->setGain(1.0f);
                source->play();
            }
            break;
        }

        return true;
    }

    void PlayerAudioComponent::readProperties(gameplay::Properties & properties)
    {
        _jumpAudioSourcePath = properties.getString("jump_sound");
    }
}
