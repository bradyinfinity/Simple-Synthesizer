#pragma once
#include <JuceHeader.h>

class MainComponent  : public juce::AudioAppComponent,
                       public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void updateDeltaAngle();

    struct SineWaveSound : public juce::SynthesiserSound
    {
        SineWaveSound() {}

        bool appliesToNote(int) override { return true; }
        bool appliesToChannel(int) override { return true; }
    };

    struct SineWaveVoice : public juce::SynthesiserVoice
    {
        SineWaveVoice() {}

        bool canPlaySound(juce::SynthesiserSound* sound) override
        {
            return dynamic_cast<SineWaveSound*> (sound) != nullptr;
        }

        void startNote(int midiNoteNumber, float velocity,
            juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
        {
            currentAngle = 0.0;
            level = velocity * 0.15;
            tailOff = 0.0;

            auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
            auto cyclesPerSample = cyclesPerSecond / getSampleRate();

            angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        }

        void stopNote(float /*velocity*/, bool allowTailOff) override
        {
            if (allowTailOff)
            {
                if (tailOff == 0.0)
                    tailOff = 1.0;
            }
            else
            {
                clearCurrentNote();
                angleDelta = 0.0;
            }
        }

        void pitchWheelMoved(int) override {}
        void controllerMoved(int, int) override {}

        void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
        {
            if (angleDelta != 0.0)
            {
                if (tailOff > 0.0)
                {
                    while (--numSamples >= 0)
                    {
                        auto currentSample = (float)(std::sin(currentAngle) * level * tailOff);

                        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                            outputBuffer.addSample(i, startSample, currentSample);

                        currentAngle += angleDelta;
                        ++startSample;

                        tailOff *= 0.99;

                        if (tailOff <= 0.005)
                        {
                            clearCurrentNote();

                            angleDelta = 0.0;
                            break;
                        }
                    }
                }
                else
                {
                    while (--numSamples >= 0)
                    {
                        auto currentSample = (float)(std::sin(currentAngle) * level);

                        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                            outputBuffer.addSample(i, startSample, currentSample);

                        currentAngle += angleDelta;
                        ++startSample;
                    }
                }
            }
        }

    private:
        double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
    };

private:

    class DecibelSlider : public juce::Slider
    {
    public:
        DecibelSlider(){}

        double getValueFromText(const juce::String& text) override
        {
            auto minusInfinitydB = -100.0;
            auto decibelText = text.upToFirstOccurrenceOf("dB", false, false).trim();
            return decibelText.equalsIgnoreCase("-INF") ? minusInfinitydB
                                                        : decibelText.getDoubleValue();
        }

        juce::String getTextFromValue(double value) override
        {
            return juce::Decibels::toString(value);
        }
    };

    juce::Random random;

    DecibelSlider whiteNoiseSlider;
    juce::Label   whiteNoiseLabel;
    float level = 0.0f;

    juce::Slider frequencySlider;
    juce::Label  frequencyLabel;
    double currentSampleRate = 0.0, currentAngle = 0.0, deltaAngle = 0.0;

    class SynthAudioSource : public juce::AudioSource
    {
    public:
        SynthAudioSource(juce::MidiKeyboardState& keyState)
            : keyboardState(keyState)
        {
            for (auto i = 0; i < 4; ++i)
                synth.addVoice(new SineWaveVoice());

            synth.addSound(new SineWaveSound());
        }

        void setUsingSineWaveSound()
        {
            synth.clearSounds();
        }

        void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override
        {
            synth.setCurrentPlaybackSampleRate(sampleRate);
        }

        void releaseResources() override {}

        void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
        {
            //bufferToFill.clearActiveBufferRegion();

            juce::MidiBuffer incomingMidi;
            keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample,
                bufferToFill.numSamples, true);

            synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
                bufferToFill.startSample, bufferToFill.numSamples);
        }

    private:
        juce::MidiKeyboardState& keyboardState;
        juce::Synthesiser synth;
    };

    juce::MidiKeyboardState keyboardState;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;

    void timerCallback() override
    {
        keyboardComponent.grabKeyboardFocus();
        stopTimer();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};