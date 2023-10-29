//
// Created by Julian Guarin on 29/10/23.
//

#ifndef AUDIOSTREAMPLUGIN_SLIDERLISTENER_H
#define AUDIOSTREAMPLUGIN_SLIDERLISTENER_H

#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>


struct SliderListener : public juce::Component, public juce::Slider::Listener
{
    SliderListener(double min, double max, double value, std::function<void(void)>cb = {[](){}})
    {
        slider.setRange(min, max);
        slider.setValue(value);

        slider.addListener(static_cast<juce::Slider::Listener*>(this));
        slider.setChangeNotificationOnlyOnRelease(true);
        onSliderChangedSlot = cb;
        addAndMakeVisible(slider);
    }
    std::function<void(void)> onSliderChangedSlot { [](){} };
    void sliderValueChanged(juce::Slider *) override
    {
        onSliderChangedSlot();
    }
    void resized() override
    {
        slider.setBounds(getLocalBounds());
    }
    double getValue() const
    {
        return slider.getValue();
    }
private:
    juce::Slider slider;
};

#endif //AUDIOSTREAMPLUGIN_SLIDERLISTENER_H
