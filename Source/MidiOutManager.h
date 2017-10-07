//
//  MidiOutManager.h
//  Bound
//
//  Created by 小野昌樹 on 10/7/17.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

// Duo-Capture ExにVolca Sampleを繋いだ時オンリーの実装(Note offしてない)

class MidiOutManager
{
public:
    static MidiOutManager& getSharedInstance()
    {
        static MidiOutManager sharedInstance;
        return sharedInstance;
    }
    
    void playSound(char ch)
    {
        MidiMessage midiMessage = MidiMessage (0x90 | ch, 0x00, 0x7f, 0);
        if (midiOut != nullptr)
        {
            midiOut->sendMessageNow(midiMessage);
        }
    }
    
private:
    MidiOutManager()
    {
        midiOutNames = MidiOutput::getDevices();
        midiOut = MidiOutput::openDevice(midiOutNames.indexOf("DUO-CAPTURE EX"));
    }
    ~MidiOutManager() { }
    
    StringArray midiOutNames;
    ScopedPointer<MidiOutput> midiOut;
    
#pragma mark - timer
};
