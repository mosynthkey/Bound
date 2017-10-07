//
//  MidiOutManager.h
//  Bound
//
//  Created by 小野昌樹 on 10/7/17.
//

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

// Duo-Capture ExにVolca Sampleを繋いだ時オンリーの実装(Note offしてない)

#define GATETIME 50

class MidiOutManager : public Timer
{
public:
    static MidiOutManager& getSharedInstance()
    {
        static MidiOutManager sharedInstance;
        return sharedInstance;
    }
    
    void playVolcaSound(char ch)
    {
        MidiMessage midiMessage = MidiMessage (0x90 | ch, 0x00, 0x7f, 0);
        if (volcaMidiOut != nullptr)
        {
            volcaMidiOut->sendMessageNow(midiMessage);
        }
    }
    
    void playMonologueSound(int note, int time)
    {
        MidiMessage midiMessage = MidiMessage (0x90 /* 1ch */, note, 0x7f, 0);
        if (monologueMidiOut != nullptr)
        {
            monologueMidiOut->sendMessageNow(midiMessage);
            noteOn[1][note] = time;
        }
    }
    
private:
    MidiOutManager()
    {
        midiOutNames = MidiOutput::getDevices();
        volcaMidiOut = MidiOutput::openDevice(midiOutNames.indexOf("DUO-CAPTURE EX"));
        monologueMidiOut = MidiOutput::openDevice(midiOutNames.indexOf("monologue SOUND"));
        
        for (int inst_i = 0; inst_i < 2; inst_i++)
        {
            for (int note_i = 0; note_i < 128; note_i++)
            {
                noteOn[inst_i][note_i] = 0;
            }
        }
        
        startTimer(100);
    }
    ~MidiOutManager() { }
    
    StringArray midiOutNames;
    ScopedPointer<MidiOutput> volcaMidiOut;
    ScopedPointer<MidiOutput> monologueMidiOut;
    
    int noteOn[2 /* volca minilogue */][128];
    
    void timerCallback()
    {
        //for (int inst_i = 0; inst_i < 2; inst_i++)
        int inst_i = 1;
        {
            for (int note_i = 0; note_i < 128; note_i++)
            {
                if (noteOn[inst_i][note_i] > 0)
                {
                    noteOn[inst_i][note_i]--;
                }
                else if (noteOn[inst_i][note_i] == 0)
                {
                    MidiMessage midiMessage = MidiMessage (0x90 /* 1ch */, note_i, 0x00, 0);
                    if (monologueMidiOut != nullptr)
                    {
                        monologueMidiOut->sendMessageNow(midiMessage);
                    }
                }
            }
        }
    }
    
#pragma mark - timer
};
