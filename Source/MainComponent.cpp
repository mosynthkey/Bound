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

#define LEDDECAY 0.1
#define KABEYOKO 0.7

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
    board2->lastId = 1000;
    
    startTimer(100);
    
    // midi
    MidiOutManager::getSharedInstance();
    startTimer(80);
    for (int b_i = 0; b_i < 2; b_i++)
    {
    for(int x=0; x<BLOCKS_SIZE ; x++){
        for(int y=0; y<BLOCKS_SIZE ; y++){
            stateLED[b_i][x][y].r = 0;
            stateLED[b_i][x][y].g = 0;
            stateLED[b_i][x][y].b = 0;
        }
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
    double x = roundToInt(touch.x * scaleX);
    double y = roundToInt(touch.y * scaleY);
    double z = touch.z;
    
    int i, j;
    
    //0:free 1:fixed
    
    int radMode = 0;

    //感度調整
    if( z <= 0.2 ) {
        z = 0;
    }else{
        z = 1;
    }
    
    //Get Ball Color
    BallChn = getMode();
    switch( BallChn ){
        case 0:
            Red = 255;
            Green = 255;
            Blue = 255;
            break;
        case 1:
            Red = 243;
            Green = 156;
            Blue = 18;
            break;
        case 2:
            Red = 52;
            Green = 152;
            Blue = 219;
            break;
        case 3:
            Red = 46;
            Green = 204;
            Blue = 113;
            break;
        case 4:
            Red = 155;
            Green = 89;
            Blue = 182;
            break;
        default:
            Red = 255;
            Green = 255;
            Blue = 255;
            break;
    }
    
    ////////rad free
    if( isTap && z == 0 && radMode == 0){
        if( oldX != x && oldY != y ){
            Ball ball;
            ball.px = x;
            ball.py = y;
            ball.vx = (float)(oldX - x) / 4.f;
            ball.vy = (float)(oldY - y) / 4.f;
            
            ball.r = Red;
            ball.g = Green;
            ball.b = Blue;
            ball.noteNum = BallChn;
            ball.id = getLastId();
            board->addBall(ball);
            
            isTap = false;
        }
        else
        {
            std::cout << "oshikondadake" << std::endl;
        }
        
        ////////rad fixed
    } else if( isTap && z == 0 && radMode == 1){
        Ball ball;
        ball.px = x;
        ball.py = y;
        power = sqrt( pow(x,2) + pow(y,2) ) / 6.f;
        
        //power = 3;
        rad = atan2( oldX - x, oldY - y );
        printf("rad = %lf\n", rad/M_PI);
        
        for(i = -1; i < 2; i = i+2){
            for(j = 1; j <= 4; j = j*2 ){
                printf("%d\n", j*i);
                
                if( i * rad < (M_PI / (i*j)) + (M_PI/8) &&  i * rad >= (M_PI / (i*j)) - (M_PI/8) ){
                    
                    //if( rad/M_PI < i*j + 1/8 && rad/M_PI >= i*j - 1/8 ){
                    
                    ball.vx =  (float)(  power * sin( M_PI/(i*j) ) ) ;
                    
                    ball.vy =  (float)(  power * cos( M_PI/(i*j) ) ) ;
                    
                    printf("%d\n", j);
                    
                }
                
            }
            
        }
        
        ball.r = Red;
        ball.g = Green;
        ball.b = Blue;
        ball.noteNum = BallChn;
        ball.id = getLastId();
        
        board->addBall(ball);
        
        
        isTap = false;
        
    }
    
    
    
    if( z != 0 && !isTap ){
        isTap = true;
        oldX = x;
        oldY = y;
        
    }else{
        int b_i = 0;
        {
            for(i = 0; i < 2; i++ ){
                
                for(j = 0; j < 2; j++ ){
                    
                    stateLED[b_i][(int)x+i][(int)y+j].r = Red;
                    
                    stateLED[b_i][(int)x+i][(int)y+j].g = Green;
                    
                    stateLED[b_i][(int)x+i][(int)y+j].b = Blue;
                    
                }
                
            }
        }
        
    }
    
    lastX = x;
    
    lastY = y;
    
}

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
    BitmapLEDProgram* canvasProgram;
    for (int b_i = 0; b_i < 2; b_i++)
    {
        if (b_i == 0)
        {
            canvasProgram = getCanvasProgram();
        }
        else
        {
            canvasProgram = getAnotherCanvasProgram();
            
        }
        
        if (canvasProgram){
            for (int y = 0; y < BLOCKS_SIZE; y++){
                for (int x = 0; x < BLOCKS_SIZE; x++){
                    //ボール等描画前にキャンバスの下地をリセット
                    canvasProgram->setLED(x, y, Colour(stateLED[b_i][x][y].r, stateLED[b_i][x][y].g, stateLED[b_i][x][y].b));
                    //前回までに描画されていた画面を減衰
                    stateLED[b_i][x][y].r = stateLED[b_i][x][y].r * LEDDECAY ;
                    stateLED[b_i][x][y].g = stateLED[b_i][x][y].g * LEDDECAY ;
                    stateLED[b_i][x][y].b = stateLED[b_i][x][y].b * LEDDECAY ;
                }
            }
            for (int y = 0; y < BLOCKS_SIZE; y++){
                for (int x = 0; x < BLOCKS_SIZE; x++){
                    //壁を塗る
                    if (pressed == TRUE)
                    {
                        int r, g, b;
                        switch( getMode() + 1 % 5){
                            case 0:
                                r = 255;
                                g = 255;
                                b = 255;
                                break;
                            case 1:
                                r = 243;
                                g = 156;
                                b = 18;
                                break;
                            case 2:
                                r = 52;
                                g = 152;
                                b = 219;
                                break;
                            case 3:
                                r = 46;
                                g = 204;
                                b = 113;
                                break;
                            case 4:
                                r = 155;
                                g = 89;
                                b = 182;
                                break;
                        }
                        if( ((x == 0)||(x == BLOCKS_SIZE -1)) || ((y == 0)||(y == BLOCKS_SIZE -1)) ){
                            canvasProgram->setLED(x, y, Colour(r, g, b));
                        }
                    }
                    
                    BoardState state;
                    if (b_i == 0)
                    {
                        state = board->getBoardState(x, y);
                    }
                    else
                    {
                        state = board2->getBoardState(x, y);
                    }
                    
                    switch (state.c)
                    {
                        case Charactor_Wall:
                            break;
                            
                        case Charactor_Ball:
                        {
                            //本家
                            stateLED[b_i][x][y].r = state.r;//stateLED[x][y].r + state.r;
                            stateLED[b_i][x][y].g = state.g;//stateLED[x][y].g + state.g;
                            stateLED[b_i][x][y].b = state.b;//stateLED[x][y].b + state.b;
                            
                            //壁塗りチンパンコード
                            for(int a = -1; a<2; a++){
                                if (0 <= y+a && y+a <= BLOCKS_SIZE - 1)
                                {
                                    if(x <= 1){
                                        stateLED[b_i][0][y+a].r = state.r * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][0][y+a].g = state.g * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][0][y+a].b = state.b * (1-KABEYOKO * abs(a));
                                    }
                                    else if(x >= BLOCKS_SIZE - 1){
                                        stateLED[b_i][BLOCKS_SIZE - 1][y+a].r = state.r * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][BLOCKS_SIZE - 1][y+a].g = state.g * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][BLOCKS_SIZE - 1][y+a].b = state.b * (1-KABEYOKO * abs(a));
                                    }
                                }
                                if (0 <= x+a && x+a <= BLOCKS_SIZE - 1)
                                {
                                    if(y <= 1){
                                        stateLED[b_i][x+a][0].r = state.r * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][x+a][0].g = state.g * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][x+a][0].b = state.b * (1-KABEYOKO * abs(a));
                                    }
                                    else if(y >= BLOCKS_SIZE - 1){
                                        stateLED[b_i][x+a][BLOCKS_SIZE - 1].r = state.r * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][x+a][BLOCKS_SIZE - 1].g = state.g * (1-KABEYOKO * abs(a));
                                        stateLED[b_i][x+a][BLOCKS_SIZE - 1].b = state.b * (1-KABEYOKO * abs(a));
                                    }
                                }
                            }

                            canvasProgram->setLED(x, y, Colour(stateLED[b_i][x][y].r, stateLED[b_i][x][y].g, stateLED[b_i][x][y].b));
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        }
    }
}
