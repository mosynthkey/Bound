//
//  Game.cpp
//  Bound - App
//
//  Created by Masaki Ono on 2017/10/03.
//

#include "Game.h"

using namespace game;

int Board::addBall(Ball &b)
{
 //   ball.id = lastId++;
    if (b.px < 0) b.px = 0;
    if (b.px > BLOCKS_SIZE - 1) b.px = BLOCKS_SIZE - 1;
    if (b.py < 0) b.py = 0;
    if (b.py > BLOCKS_SIZE - 1) b.py = BLOCKS_SIZE - 1;
    
    ballList.push_back(b);
    return b.id;
}

void Board::deleteBall(int id)
{
    for (int i = 0; i < ballList.size(); i++)
    {
        if (ballList[i].id == id)
        {
            ballList.erase(ballList.begin() + i);
            break;
        }
    }
}

void Board::deleteAllBalls()
{
    ballList.clear();
}

void Board::move()
{
    warpBallList.clear();
    
    for (int i = 0; i < ballList.size(); i++)
    {
        auto &b = ballList[i];
        if (isWall(b.px + b.vx, b.py) || isWall(b.px - b.vx, b.py))
        {
            b.vx = -b.vx;
            if (i == 4)
            {
                outManager->playMonologueSound(sequence[seq_i++], 1);
                seq_i = seq_i % sequence.size();
            }
            else
            {
                outManager->playVolcaSound(b.noteNum);
            }
        }
        
        if (isWall(b.px, b.py + b.vy) || isWall(b.px, b.py - b.vy))
        {
            b.vy = -b.vy;
            //outManager->playVolcaSound(b.noteNum);
            if (i == 4)
            {
                outManager->playMonologueSound(sequence[seq_i++], 1);
                seq_i = seq_i % sequence.size();
            }
            else
            {
                outManager->playVolcaSound(b.noteNum);
            }
        }
        
        b.px += b.vx;
        b.py += b.vy;
        
        if (isWarpZone(b.px, b.py))
        {
            warpBallList.push_back(b);
        }
    }
    
    for (int i = 0; i < warpBallList.size(); i++)
    {
        auto &b = warpBallList[i];
        deleteBall(b.id);
        
        if (b.px < 0)
        {
            b.px += BLOCKS_SIZE;
            if (connectedBoard[Direction_Left] != nullptr)
            {
                connectedBoard[Direction_Left]->addBall(b);
            }
        }
        else if (b.px >= BLOCKS_SIZE - 1)
        {
            b.px -= BLOCKS_SIZE;
            if (connectedBoard[Direction_Right] != nullptr)
            {
                connectedBoard[Direction_Right]->addBall(b);
            }
        }
        else if (b.py < 0)
        {
            b.py += BLOCKS_SIZE;
            if (connectedBoard[Direction_Top] != nullptr)
            {
                connectedBoard[Direction_Top]->addBall(b);
            }
        }
        else if (b.py >= BLOCKS_SIZE - 1)
        {
            b.py -= BLOCKS_SIZE;
            if (connectedBoard[Direction_Bottom] != nullptr)
            {
                connectedBoard[Direction_Bottom]->addBall(b);
            }
        }
    }
}


void Board::connect(Board *b, Direction d)
{
    connectedBoard[d] = b;
}

void Board::disConnect(Direction d)
{
    connectedBoard[d] = nullptr;
}
