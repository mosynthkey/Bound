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
    board2 = new Board();
    
    /*
    //Track1. BD color rgb(255, 255, 255)
    {
        Ball ball;
        ball.px = 5.f;
        ball.py = 6.f;
        ball.vx = 1.0f;
        ball.vy = 0.5f;
        ball.r = 255.f;
        ball.g = 255.f;
        ball.b = 255.f;
        ball.noteNum = 0;
        board->addBall(ball);
    }
    
    //Track2. SN color rgb(243, 156, 18)
    {
        Ball ball;
        ball.px = 3.f;
        ball.py = 8.f;
        ball.vx = 1.f;
        ball.vy = -2.f;
        ball.r = 243.f;
        ball.g = 156.f;
        ball.b = 18.f;
        ball.noteNum = 1;
        board->addBall(ball);
    }
      
    //Track3. HH color rgb(52, 152, 219)
    {
        Ball ball;
        ball.px = 2.f;
        ball.py = 2.f;
        ball.vx = 1.f;
        ball.vy = 1.f;
        ball.r = 52.f;
        ball.g = 152.f;
        ball.b = 219.f;
        ball.noteNum = 2;
        board->addBall(ball);
    }
    
    //Track4. Ba color rgb(46, 204, 113)
    {
        Ball ball;
        ball.px = 7.f;
        ball.py = 4.f;
        ball.vx = 2.f;
        ball.vy = -1.f;
        ball.r = 46.f;
        ball.g = 204.f;
        ball.b = 113.f;
        ball.noteNum = 5;
        board->addBall(ball);
    }
    //Track. Seq color rgb(155, 89, 182)
    {
        Ball ball;
        ball.px = 7.f;
        ball.py = 4.f;
        ball.vx = 2.f;
        ball.vy = -1.f;
        ball.r = 155.f;
        ball.g = 89.f;
        ball.b = 182.f;
        ball.noteNum = 5;
        board->addBall(ball);
    }
     */
    
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
    
    if (anotherBlock != nullptr)
        detachAnotherBlock();
    
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
    stopTimer();
    lightpadComponent.setVisible (false);
    infoLabel.setVisible (true);
    
    // Reset the activeBlock object
    if (activeBlock != nullptr)
        detachActiveBlock();
    
    if (anotherBlock != nullptr)
        detachAnotherBlock();
    
    // Get the array of currently connected Block objects from the PhysicalTopologySource
    auto blocks = topologySource.getCurrentTopology().blocks;
    
    // Iterate over the array of Block objects
    for (auto b : blocks)
    {
        // Find the first Lightpad
        if (activeBlock == nullptr && b->getType() == Block::Type::lightPadBlock)
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
        }
        else if (activeBlock != nullptr && anotherBlock == nullptr && b->getType() == Block::Type::lightPadBlock)
        {
            anotherBlock = b;
            /*
            if (auto surface = anotherBlock->getTouchSurface())
                surface->addListener (this);
            */
            // Register MainContentComponent as a listener to any buttons
            for (auto button : anotherBlock->getButtons())
                button->addListener (this);
            
            if (auto grid = anotherBlock->getLEDGrid())
            {
                // Work out scale factors to translate X and Y touches to LED indexes
                scaleX = (float) (grid->getNumColumns() - 1) / anotherBlock->getWidth();
                scaleY = (float) (grid->getNumRows() - 1)    / anotherBlock->getHeight();
                
                setLEDProgram (*anotherBlock);
            }
            
            // 下につなぐ(決め打ち)
            board->connect(board2, Direction_Bottom);
            board2->connect(board, Direction_Top);
            board2->deleteAllBalls();
            break;
        }
    }
    
    if (anotherBlock == nullptr)
    {
        board->disConnect(Direction_Bottom);
        board2->disConnect(Direction_Top);
    }
    
    startTimer(100);
}


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


/*
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
    
}
 */

void MainComponent::buttonPressed (ControlButton&, Block::Timestamp)
{
    pressed = true;
}

void MainComponent::buttonReleased (ControlButton&, Block::Timestamp)
{
    std::cout << "buttonReleased" << std::endl;
    pressed = false;
    setNextMode();
}

void MainComponent::buttonClicked (Button* b)
{
}

void MainComponent::sliderValueChanged (Slider* s)
{
}

void MainComponent::timerCallback()
{
    redrawLEDs();
    board->move();
    board2->move();
}

void MainComponent::ledClicked (int x, int y, float z)
{
}

void MainComponent::detachActiveBlock()
{
    if (auto surface = activeBlock->getTouchSurface())
        surface->removeListener (this);
    
    for (auto button : activeBlock->getButtons())
        button->removeListener (this);
    
    activeBlock = nullptr;
}

void MainComponent::detachAnotherBlock()
{
    if (auto surface = anotherBlock->getTouchSurface())
        surface->removeListener (this);
    
    anotherBlock = nullptr;
}

void MainComponent::setLEDProgram (Block& block)
{
    
    block.setProgram (new BitmapLEDProgram (block));
    
    // Redraw any previously drawn LEDs
    redrawLEDs();
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

}

void MainComponent::redrawLEDs(){
    if (auto* canvasProgram = getCanvasProgram()){
        for (int y = 0; y < BLOCKS_SIZE; y++){
            for (int x = 0; x < BLOCKS_SIZE; x++){
                //ボール等描画前にキャンバスの下地をリセット
                canvasProgram->setLED(x, y, Colour(stateLED[x][y].r, stateLED[x][y].g, stateLED[x][y].b));
                //LEDを減衰
                stateLED[x][y].r = stateLED[x][y].r*LEDDECAY ;
                stateLED[x][y].g = stateLED[x][y].g*LEDDECAY ;
                stateLED[x][y].b = stateLED[x][y].b*LEDDECAY ;
            }
        }
        for (int y = 0; y < BLOCKS_SIZE; y++){
            for (int x = 0; x < BLOCKS_SIZE; x++){
                //壁を塗る
                if( ((x == 0)||(x == BLOCKS_SIZE -1)) || ((y == 0)||(y == BLOCKS_SIZE -1)) ){
                    //canvasProgram->setLED(x, y, Colour(255/4, 255/4, 255/4));
                }
                BoardState state = board->getBoardState(x, y);
                switch (state.c)
                {
                    case Charactor_Wall:
                        //canvasProgram->setLED(x, y, Colour(255/4, 255/4, 255/4));
                        break;
                        
                    case Charactor_Ball:
                    {
                        stateLED[x][y].r = state.r;//stateLED[x][y].r + state.r;
                        stateLED[x][y].g = state.g;//stateLED[x][y].g + state.g;
                        stateLED[x][y].b = state.b;//stateLED[x][y].b + state.b;
                        canvasProgram->setLED(x, y, Colour(stateLED[x][y].r, stateLED[x][y].g, stateLED[x][y].b));
                        
                        //壁ピンク化チンパンコード
                        if( (x <= 1)){
                            stateLED[x-1][y].r = state.r;
                            stateLED[x-1][y].g = state.g;
                            stateLED[x-1][y].b = state.b;
                            
                            stateLED[x-1][y+1].r = state.r;
                            stateLED[x-1][y+1].g = state.g;
                            stateLED[x-1][y+1].b = state.b;
                            
                            stateLED[x-1][y-1].r = state.r;
                            stateLED[x-1][y-1].g = state.g;
                            stateLED[x-1][y-1].b = state.b;
                        }
                        if(x>= BLOCKS_SIZE-2){
                            stateLED[x+1][y].r = state.r;
                            stateLED[x+1][y].g = state.g;
                            stateLED[x+1][y].b = state.b;
                            
                            stateLED[x+1][y+1].r = state.r;
                            stateLED[x+1][y+1].g = state.g;
                            stateLED[x+1][y+1].b = state.b;
                            
                            stateLED[x+1][y-1].r = state.r;
                            stateLED[x+1][y-1].g = state.g;
                            stateLED[x+1][y-1].b = state.b;
                        }
                        if( (y <= 1)){
                            stateLED[x][y-1].r = state.r;
                            stateLED[x][y-1].g = state.g;
                            stateLED[x][y-1].b = state.b;
                            
                            stateLED[x+1][y-1].r = state.r;
                            stateLED[x+1][y-1].g = state.g;
                            stateLED[x+1][y-1].b = state.b;
                            
                            stateLED[x-1][y-1].r = state.r;
                            stateLED[x-1][y-1].g = state.g;
                            stateLED[x-1][y-1].b = state.b;
                        }
                        if(y>=BLOCKS_SIZE-2){
                            stateLED[x][y+1].r = state.r;
                            stateLED[x][y+1].g = state.g;
                            stateLED[x][y+1].b = state.b;
                            
                            stateLED[x+1][y+1].r = state.r;
                            stateLED[x+1][y+1].g = state.g;
                            stateLED[x+1][y+1].b = state.b;
                            
                            stateLED[x-1][y+1].r = state.r;
                            stateLED[x-1][y+1].g = state.g;
                            stateLED[x-1][y+1].b = state.b;
                            //std::cout << "yyy" << y << std::endl;
                        }
                        break;
                    }
                    default:
                        //canvasProgram->setLED(x, y, Colour(stateLED[x][y].r, stateLED[x][y].g, stateLED[x][y].b));
                        break;
                }
            }
        }
        //引っ張り軌道
        for (int y = 0; y < BLOCKS_SIZE; y++){
            for (int x = 0; x < BLOCKS_SIZE; x++){
                
            }
        }
        
    }
}

