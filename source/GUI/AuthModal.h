//
// Created by Julian Guarin on 9/01/24.
//

#ifndef AUDIOSTREAMPLUGIN_AUTHMODAL_H
#define AUDIOSTREAMPLUGIN_AUTHMODAL_H
#include "signalsslots.h"
#include <juce_audio_processors/juce_audio_processors.h>


class AuthModal : public juce::Component, public juce::Button::Listener
{
    juce::TextEditor    __secretEditor;
    juce::ToggleButton  __secretDiscloseToggle;
    juce::TextButton    __secretSubmitButton;

public:
    void setSecret(const std::string& secret)
    {
        __secretEditor.setText(secret);
    }
    DAWn::Events::Signal<std::string> onSecretSubmit;

    AuthModal();
    void resized() override;
    void buttonClicked(juce::Button *button) override
    {
        if (button == &__secretDiscloseToggle)
        {
            auto showing = __secretEditor.getPasswordCharacter() == 0;
            __secretEditor.setPasswordCharacter(showing ? '*' : 0);
            __secretDiscloseToggle.setButtonText(showing ? "Show" : "Hide");
        }
        else if (button == &__secretSubmitButton)
        {
            onSecretSubmit.Emit(__secretEditor.getText().toStdString());
        }
    }
    void toggleSecretVisibility();

};

class AuthModalWindow : public juce::DocumentWindow
{
public:
    AuthModalWindow() :  DocumentWindow("API Key Authentication", juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new AuthModal(), true);
        setResizable(false, false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }
    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};


#endif //AUDIOSTREAMPLUGIN_AUTHMODAL_H
