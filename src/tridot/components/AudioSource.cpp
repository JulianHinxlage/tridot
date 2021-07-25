//
// Copyright (c) 2021 Julian Hinxlage. All rights reserved.
//

#include "AudioSource.h"
#include "tridot/core/Environment.h"
#include "tridot/engine/Scene.h"
#include "tridot/engine/AudioManager.h"


namespace tridot {

    AudioSource::AudioSource() {
        id = 0;
        playingAudioId = 0;
    }

    TRI_UPDATE_CALLBACK("AudioSource"){
        env->scene->view<AudioSource, Transform>().each([](AudioSource &source, Transform &transform){
            if(source.id == 0){
                if(source.audio){
                    if(source.audio->getId() != 0){
                        source.id = env->audio->play(source.audio, true);
                        env->audio->setPosition(source.id, transform.position);
                        source.playingAudioId = source.audio->getId();
                    }
                }
            }else{
                env->audio->setPosition(source.id, transform.position);
                if(!source.audio){
                    env->audio->stop(source.id);
                    source.id = 0;
                    source.playingAudioId = 0;
                }else{
                    if(source.audio->getId() != source.playingAudioId){
                        env->audio->stop(source.id);
                        source.id = 0;
                        source.playingAudioId = 0;
                    }
                }
            }
        });
    }

    TRI_UPDATE_CALLBACK("AudioListener"){
        env->scene->view<AudioListener, Transform>().each([](AudioListener &listener, Transform &transform){
            env->audio->setListenerPosition(transform.position);
            env->audio->setListenerRotation(transform.rotation);
        });
    }

    TRI_REGISTER_CALLBACK(){
        env->events->sceneEnd.addCallback("AudioSource", [](){
            env->scene->view<AudioSource>().each([](AudioSource &source){
                if(source.id != 0){
                    env->audio->stop(source.id);
                    source.id = 0;
                    source.playingAudioId = 0;
                }
            });
        });
    }

}
