#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() :
    synthAudioSource (keyboardState),
    keyboardComponent(keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    whiteNoiseSlider.setRange(-100, -12);
    whiteNoiseSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    whiteNoiseSlider.onValueChange = [this] { level = juce::Decibels::decibelsToGain((float)whiteNoiseSlider.getValue()); };
    whiteNoiseSlider.setValue(juce::Decibels::gainToDecibels(level));
    
    whiteNoiseLabel.setText("White Noise", juce::dontSendNotification);

    frequencySlider.setRange(50, 5000, 0.1);
    frequencySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
    frequencySlider.setSkewFactorFromMidPoint(500.0);
    frequencySlider.onValueChange = [this] { if (currentSampleRate > 0.0) updateDeltaAngle(); };

    frequencyLabel.setText("Sine Wave", juce::dontSendNotification);

    addAndMakeVisible(whiteNoiseSlider);
    addAndMakeVisible(whiteNoiseLabel);
    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(frequencyLabel);

    addAndMakeVisible(keyboardComponent);

    setSize(600, 200);
    setAudioChannels (0, 2);
    startTimer(400);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    synthAudioSource.prepareToPlay(samplesPerBlockExpected, sampleRate);

    juce::String message;
    message << "Preparing to play audio...\n";
    message << "-samples per block expected = " << samplesPerBlockExpected << "\n";
    message << "-sample rate = " << sampleRate << "\n";
    juce::Logger::getCurrentLogger()->writeToLog(message);

    currentSampleRate = sampleRate;
    updateDeltaAngle();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{ 
    auto whiteLevel = level;
    auto levelScale = whiteLevel * 2.0f;
    
    auto sineVolume = 0.13f;
    auto* leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
    auto* rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

    for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
    {
        auto currentSample = (float)std::sin(currentAngle);
        currentAngle += deltaAngle;
        auto whiteNoise = random.nextFloat() * levelScale - whiteLevel;
        leftBuffer[sample] = currentSample * sineVolume + whiteNoise;
        rightBuffer[sample] = currentSample * sineVolume + whiteNoise;
    }

    synthAudioSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    juce::Logger::getCurrentLogger()->writeToLog("Releasing audio resources");
    synthAudioSource.releaseResources();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::AudioAppComponent::getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    whiteNoiseLabel.setBounds(10, 10, 90, 20);
    whiteNoiseSlider.setBounds(100, 10, juce::AudioAppComponent::getWidth() - 110, 20);
    frequencyLabel.setBounds(10, 50, 90, 20);
    frequencySlider.setBounds(100, 50, juce::AudioAppComponent::getWidth() - 110, 20);

    keyboardComponent.setBounds(10, 100, getWidth() - 20, getHeight() - 120);
}

void MainComponent::updateDeltaAngle()
{
    auto cyclesPerSample = frequencySlider.getValue() / currentSampleRate;
    deltaAngle = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
}
