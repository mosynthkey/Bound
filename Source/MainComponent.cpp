/*
 ==============================================================================
 
 This file is part of the JUCE library.
 Copyright (c) 2017 - ROLI Ltd.
 
 JUCE is an open source library subject to commercial or open-source
 licensing.
 
 By using JUCE, you agree to the terms of both the JUCE 5 End-User License
 Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
 27th April 2017).
 
 End User License Agreement: www.juce.com/juce-5-licence
 Privacy Policy: www.juce.com/juce-5-privacy-policy
 
 Or: You may also use this code under the terms of the GPL v3 (see
 www.gnu.org/licenses).
 
 JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
 EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
 DISCLAIMED.
 
 ==============================================================================
 */

#include "MainComponent.h"
#include "Game.h"
#include "stdio.h"
#include "math.h"

using namespace game;

MainComponent::MainComponent()
{
    activeLeds.clear();
    
    // Register MainContentComponent as a listener to the PhysicalTopologySource object
    topologySource.addListener (this);
    
    infoLabel.setText ("Connect a Lightpad Block to draw.", dontSendNotification);
    infoLabel.setJustificationType (Justification::centred);
    addAndMakeVisible (infoLabel);
    
    addAndMakeVisible (lightpadComponent);
    lightpadComponent.setVisible (false);
    lightpadComponent.addListener (this);
    
    clearButton.setButtonText ("Clear");
    clearButton.addListener (this);
    clearButton.setAlwaysOnTop (true);
    addAndMakeVisible (clearButton);
    
    brightnessSlider.setRange (0.0, 1.0);
    brightnessSlider.setValue (1.0);
    brightnessSlider.setAlwaysOnTop (true);
    brightnessSlider.setTextBoxStyle (Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);
    brightnessSlider.addListener (this);
    addAndMakeVisible (brightnessSlider);
    
    brightnessLED.setAlwaysOnTop (true);
    brightnessLED.setColour (layout.currentColour.withBrightness (static_cast<float> (brightnessSlider.getValue())));
    addAndMakeVisible (brightnessLED);
    
#if JUCE_IOS
    connectButton.setButtonText ("Connect");
    connectButton.addListener (this);
    connectButton.setAlwaysOnTop (true);
    addAndMakeVisible (connectButton);
#endif
    
    setSize (600, 600);
    
    board = new Board();
    
    {
        Ball ball;
        ball.px = 5.f;
        ball.py = 6.f;
        ball.vx = 1.0f;
        ball.vy = 0.5f;
        ball.r = 255.f;
        ball.g = ball.b = 0.f;
        ball.noteNum = 0;
        board->addBall(ball);
    }
    
    {
        Ball ball;
        ball.px = 3.f;
        ball.py = 8.f;
        ball.vx = 1.f;
        ball.vy = -2.f;
        ball.b = 255.f;
        ball.r = ball.g = 0.f;
        ball.noteNum = 1;
        board->addBall(ball);
    }
    
    {
        Ball ball;
        ball.px = 2.f;
        ball.py = 2.f;
        ball.vx = 1.f;
        ball.vy = 1.f;
        ball.b = 255.f;
        ball.r = ball.g = 0.f;
        ball.noteNum = 2;
        board->addBall(ball);
    }
    
    {
        Ball ball;
        ball.px = 7.f;
        ball.py = 4.f;
        ball.vx = 2.f;
        ball.vy = -1.f;
        ball.b = 255.f;
        ball.r = ball.g = 0.f;
        ball.noteNum = 5;
        board->addBall(ball);
    }
    
    startTimer(100);
    
    // midi
    MidiOutManager::getSharedInstance();
    startTimer(80);
    
    for(int x=0; x<BLOCKS_SIZE ; x++){
        for(int y=0; y<BLOCKS_SIZE ; y++){
            stateLED[x][y].r = 0;
            stateLED[x][y].g = 0;
            stateLED[x][y].b = 0;
        }
    }
}

MainComponent::~MainComponent()
{
    if (activeBlock != nullptr)
        detachActiveBlock();
    
    lightpadComponent.removeListener (this);
}

void MainComponent::resized()
{
    infoLabel.centreWithSize (getWidth(), 100);
    
    auto bounds = getLocalBounds().reduced (20);
    
    // top buttons
    auto topButtonArea = bounds.removeFromTop (getHeight() / 20);
    
    topButtonArea.removeFromLeft (20);
    clearButton.setBounds (topButtonArea.removeFromLeft (80));
    
#if JUCE_IOS
    topButtonArea.removeFromRight (20);
    connectButton.setBounds (topButtonArea.removeFromRight (80));
#endif
    
    bounds.removeFromTop (20);
    
    auto orientation = Desktop::getInstance().getCurrentOrientation();
    
    if (orientation == Desktop::DisplayOrientation::upright
        || orientation == Desktop::DisplayOrientation::upsideDown)
    {
        auto brightnessControlBounds = bounds.removeFromBottom (getHeight() / 10);
        
        brightnessSlider.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
        brightnessLED.setBounds (brightnessControlBounds.removeFromLeft (getHeight() / 10));
        brightnessSlider.setBounds (brightnessControlBounds);
    }
    else
    {
        auto brightnessControlBounds = bounds.removeFromRight (getWidth() / 10);
        
        brightnessSlider.setSliderStyle (Slider::SliderStyle::LinearVertical);
        brightnessLED.setBounds (brightnessControlBounds.removeFromTop (getWidth() / 10));
        brightnessSlider.setBounds (brightnessControlBounds);
    }
    
    // lightpad component
    auto sideLength = jmin (bounds.getWidth() - 40, bounds.getHeight() - 40);
    lightpadComponent.centreWithSize (sideLength, sideLength);
}

void MainComponent::topologyChanged()
{
    lightpadComponent.setVisible (false);
    infoLabel.setVisible (true);
    
    // Reset the activeBlock object
    if (activeBlock != nullptr)
        detachActiveBlock();
    
    // Get the array of currently connected Block objects from the PhysicalTopologySource
    auto blocks = topologySource.getCurrentTopology().blocks;
    
    // Iterate over the array of Block objects
    for (auto b : blocks)
    {
        // Find the first Lightpad
        if (b->getType() == Block::Type::lightPadBlock)
        {
            activeBlock = b;
            
            // Register MainContentComponent as a listener to the touch surface
            if (auto surface = activeBlock->getTouchSurface())
                surface->addListener (this);
            
            // Register MainContentComponent as a listener to any buttons
            for (auto button : activeBlock->getButtons())
                button->addListener (this);
            
            // Get the LEDGrid object from the Lightpad and set its program to the program for the current mode
            if (auto grid = activeBlock->getLEDGrid())
            {
                // Work out scale factors to translate X and Y touches to LED indexes
                scaleX = (float) (grid->getNumColumns() - 1) / activeBlock->getWidth();
                scaleY = (float) (grid->getNumRows() - 1)    / activeBlock->getHeight();
                
                setLEDProgram (*activeBlock);
            }
            
            // Make the on screen Lighpad component visible
            lightpadComponent.setVisible (true);
            infoLabel.setVisible (false);
            
            break;
        }
    }
}

/*
//==============================================================================
void MainComponent::touchChanged (TouchSurface&, const TouchSurface::Touch& touch)
{
    auto x = roundToInt(touch.x * scaleX);
    auto y = roundToInt(touch.y * scaleY);
    auto z = touch.z;
    
    if( z <= 0.4 ){
        z = 0;
    }
    else{
        z = 1;
    }
    
    //std::cout << "touched(" << x << ", " << y << ", " << z << ")" << std::endl;
    // if( lastX != x && lastY != y )
    {

        
        if( isTap && z == 0 ){
            if( oldX != x && oldY != y )
            {
                Ball ball;
                ball.px = x;
                ball.py = y;
                ball.vx = (float)(oldX - x) / 2.f;
                ball.vy = (float)(oldY - y) / 2.f;
                ball.r = 255;
                ball.g = 255;
                ball.b = 255;
                
                board->addBall(ball);
                isTap = false;
                //std::cout << "measured(" << x << ", " << y << ", " << oldX << ", " << oldY << ")" << std::endl;
                //std::cout << "out(" << oldX-x << ", "<< oldY-y << ")" << std::endl;
                
            }
            else
            {
                std::cout << "oshikondadake" << std::endl;
            }
        }
        
        if( z != 0 && !isTap ){
            isTap = true;
            oldX = x;
            oldY = y;
            printf("measure\n");
            std::cout << "measured(" << x << ", " << y << ", " << z << ")" << std::endl;
        }
        lastX = x;
        lastY = y;
    }
    
    //std::cout << "touched(" << x << ", " << y << ", " << z << ")" << std::endl;
 
}
*/

void MainComponent::touchChanged (TouchSurface&, const TouchSurface::Touch& touch)
{
    auto x = roundToInt(touch.x * scaleX);
    auto y = roundToInt(touch.y * scaleY);
    auto z = touch.z;
    
    if( z <= 0.4 ){
        z = 0;
    }
    else{
        z = 1;
    }
    
    //std::cout << "touched(" << x << ", " << y << ", " << z << ")" << std::endl;
    // if( lastX != x && lastY != y )
    {
        /*
         if (!board->isWall(x, y))
         {
         Ball ball;
         ball.px = x;
         ball.py = y;
         ball.vx = z * 6;
         ball.vy = -z * 6;
         ball.r = (1 - z) * 255;
         ball.g = abs(0.5 - z) * 255;
         ball.b = z * 255;
         board->addBall(ball);
         }
         */
        
        if( isTap && z == 0 ){
            if( oldX != x && oldY != y )
            {
                Ball ball;
                ball.px = x;
                ball.py = y;
                //ball.vx = (float)(oldX - x) / 2.f;
                //ball.vy = (float)(oldY - y) / 2.f;
                ball.r = 255;
                ball.g = 255;
                ball.b = 255;
                
                rad = sqrt( pow(x, 2) + pow(y, 2) );
                if( (float)abs(y)/abs(x) < tan(M_PI/4) || (float)abs(y)/abs(x) > tan(3*M_PI/4) ){
                    ball.vx = (float)( rad / sqrt(2) /2 ) / 2.f;
                    ball.vy = (float)( rad / sqrt(2) /2 ) / 2.f;
                    printf("45\n");
                    std::cout << "measured(" << rad << "," << rad / sqrt(2) << ", " << rad / sqrt(2) << ")" << std::endl;
                }
                else if( x < y ){
                    ball.vx = rad / 2.f;
                    ball.vy = 1;
                    printf("tate\n");
                }
                else{
                    ball.vx = rad / 2.f;
                    ball.vy = 1;
                    printf("yoko\n");
                }
                
                
                board->addBall(ball);
                isTap = false;
                //std::cout << "measured(" << x << ", " << y << ", " << oldX << ", " << oldY << ")" << std::endl;
                //std::cout << "out(" << oldX-x << ", "<< oldY-y << ")" << std::endl;
                
            }
            else
            {
                std::cout << "oshikondadake" << std::endl;
            }
        }
        
        if( z != 0 && !isTap ){
            isTap = true;
            oldX = x;
            oldY = y;
            printf("measure\n");
            std::cout << "measured(" << x << ", " << y << ", " << z << ")" << std::endl;
        }
        lastX = x;
        lastY = y;
    }
    
    //std::cout << "touched(" << x << ", " << y << ", " << z << ")" << std::endl;
    /*
     // Translate X and Y touch events to LED indexes
     auto xLed = roundToInt (touch.x * scaleX);
     auto yLed = roundToInt (touch.y * scaleY);
     
     if (currentMode == colourPalette)
     {
     if (layout.setActiveColourForTouch (xLed, yLed))
     {
     if (auto* colourPaletteProgram = getPaletteProgram())
     {
     colourPaletteProgram->setGridFills (layout.numColumns, layout.numRows, layout.gridFillArray);
     brightnessLED.setColour (layout.currentColour
     .withBrightness (layout.currentColour == Colours::black ? 0.0f
     : static_cast<float> (brightnessSlider.getValue())));
     }
     }
     }
     else if (currentMode == canvas)
     {
     drawLED ((uint32) xLed, (uint32) yLed, touch.z, layout.currentColour);
     }
     */
}


void MainComponent::buttonReleased (ControlButton&, Block::Timestamp)
{
    std::cout << "buttonReleased" << std::endl;
}

void MainComponent::buttonClicked (Button* b)
{
    std::cout << "buttonClicked" << std::endl;
}

void MainComponent::sliderValueChanged (Slider* s)
{
    /*
    if (s == &brightnessSlider)
        brightnessLED.setColour (layout.currentColour
                                 .withBrightness (layout.currentColour == Colours::black ? 0.0f
                                                  : static_cast<float> (brightnessSlider.getValue())));
     */
}

void MainComponent::timerCallback()
{
    redrawLEDs();
    board->move();
}

void MainComponent::ledClicked (int x, int y, float z)
{
    /*
    drawLED ((uint32) x, (uint32) y,
             z == 0.0f ? static_cast<float> (brightnessSlider.getValue())
             : z * static_cast<float> (brightnessSlider.getValue()), layout.currentColour);
     */
}

void MainComponent::detachActiveBlock()
{
    if (auto surface = activeBlock->getTouchSurface())
        surface->removeListener (this);
    
    for (auto button : activeBlock->getButtons())
        button->removeListener (this);
    
    activeBlock = nullptr;
}

void MainComponent::setLEDProgram (Block& block)
{
    if (currentMode == canvas)
    {
        block.setProgram (new BitmapLEDProgram (block));
        
        // Redraw any previously drawn LEDs
        redrawLEDs();
    }
    else if (currentMode == colourPalette)
    {
        block.setProgram (new DrumPadGridProgram (block));
        
        // Setup the grid layout
        if (auto* program = getPaletteProgram())
            program->setGridFills (layout.numColumns, layout.numRows, layout.gridFillArray);
    }
}

void MainComponent::clearLEDs()
{
    if (auto* canvasProgram = getCanvasProgram())
    {
        // Clear the LED grid
        for (uint32 x = 0; x < 15; ++x)
        {
            for (uint32 y = 0; y < 15; ++ y)
            {
                canvasProgram->setLED (x, y, Colours::black);
                lightpadComponent.setLEDColour (x, y, Colours::black);
            }
        }
        
        // Clear the ActiveLED array
        activeLeds.clear();
    }
}

void MainComponent::drawLED (uint32 x0, uint32 y0, float z, Colour drawColour)
{
    /*
    if (auto* canvasProgram = getCanvasProgram())
    {
        // Check if the activeLeds array already contains an ActiveLED object for this LED
        auto index = getLEDAt (x0, y0);
        
        // If the colour is black then just set the LED to black and return
        if (drawColour == Colours::black)
        {
            if (index >= 0)
            {
                canvasProgram->setLED (x0, y0, Colours::black);
                lightpadComponent.setLEDColour (x0, y0, Colours::black);
                activeLeds.remove (index);
            }
            
            return;
        }
        
        // If there is no ActiveLED obejct for this LED then create one,
        // add it to the array, set the LED on the Block and return
        if (index < 0)
        {
            ActiveLED led;
            led.x = x0;
            led.y = y0;
            led.colour = drawColour;
            led.brightness = z;
            
            activeLeds.add (led);
            canvasProgram->setLED (led.x, led.y, led.colour.withBrightness (led.brightness));
            
            lightpadComponent.setLEDColour (led.x, led.y, led.colour.withBrightness (led.brightness));
            
            return;
        }
        
        // Get the ActiveLED object for this LED
        auto currentLed = activeLeds.getReference (index);
        
        // If the LED colour is the same as the draw colour, add the brightnesses together.
        // If it is different, blend the colours
        if (currentLed.colour == drawColour)
            currentLed.brightness = jmin (currentLed.brightness + z, 1.0f);
        else
            currentLed.colour = currentLed.colour.interpolatedWith (drawColour, z);
        
        
        // Set the LED on the Block and change the ActiveLED object in the activeLeds array
        if (canvasProgram != nullptr)
            canvasProgram->setLED (currentLed.x, currentLed.y, currentLed.colour.withBrightness (currentLed.brightness));
        
        lightpadComponent.setLEDColour (currentLed.x, currentLed.y, currentLed.colour.withBrightness (currentLed.brightness));
        
        activeLeds.set (index, currentLed);
    }
     */
}

void MainComponent::redrawLEDs(){
    if (auto* canvasProgram = getCanvasProgram()){
        for (int y = 0; y < 15; y++){
            for (int x = 0; x < 15; x++){
                //canvasProgram->setLED(x, y, Colour(155/3, 40/3, 200/3));
            }
         }
        
        
        for (int y = 0; y < BLOCKS_SIZE; y++)
        {
            for (int x = 0; x < BLOCKS_SIZE; x++)
            {
                BoardState state = board->getBoardState(x, y);
                //LEDを減衰
                stateLED[x][y].r = stateLED[x][y].r/2 ;
                stateLED[x][y].g = stateLED[x][y].g/2 ;
                stateLED[x][y].b = stateLED[x][y].b/2 ;
                switch (state.c)
                {
                    case Charactor_Wall:
                        canvasProgram->setLED(x, y, Colour(255/4, 255/4, 255/4));
                        break;
                        
                    case Charactor_Ball:
                        stateLED[x][y].r = state.r;
                        stateLED[x][y].g = state.g;
                        stateLED[x][y].b = state.b;
                        canvasProgram->setLED(x, y, Colour(stateLED[x][y].r, stateLED[x][y].g, stateLED[x][y].b));
                        /*canvasProgram->setLED(x, y, Colour(state.r, state.g, state.b));
                        canvasProgram->setLED(x-1, y, Colour(state.r/4, state.g/4, state.b/4));
                        canvasProgram->setLED(x+1, y, Colour(state.r/4, state.g/4, state.b/4));
                        canvasProgram->setLED(x, y-1, Colour(state.r/4, state.g/4, state.b/4));
                        canvasProgram->setLED(x, y+1, Colour(state.r/4, state.g/4, state.b/4));
                        */break;
                        
                    default:
                        canvasProgram->setLED(x, y, Colour(stateLED[x][y].r, stateLED[x][y].g, stateLED[x][y].b));
                        //canvasProgram->setLED(x, y, Colour(0, 0, 0));
                        break;
                }
            }
        }
    }
}

