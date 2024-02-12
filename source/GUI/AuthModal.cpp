//
// Created by Julian Guarin on 9/01/24.
//

#include "AuthModal.h"


AuthModal::AuthModal(){

    /* Secret Editor */
    addAndMakeVisible(__secretEditor);
    __secretEditor.setPasswordCharacter('*');


    /* Secret Disclose Toggle */
    addAndMakeVisible(__secretDiscloseToggle);
    __secretDiscloseToggle.setButtonText("Show");
    __secretDiscloseToggle.addListener(this);

    /* Secret Submit Button */
    addAndMakeVisible(__secretSubmitButton);
    __secretSubmitButton.setButtonText("Submit");
    __secretSubmitButton.addListener(this);

    setSize(300, 200);

}

void AuthModal::resized() {
    __secretEditor.setBounds(10, 10, getWidth() - 20, 20);
    __secretDiscloseToggle.setBounds(10, 40, getWidth() - 20, 20);
    __secretSubmitButton.setBounds(10, 70, getWidth() - 20, 20);
}


