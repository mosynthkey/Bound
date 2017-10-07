//
//  Game.cpp
//  Bound - App
//
//  Created by Masaki Ono on 2017/10/03.
//

#include "Game.h"

using namespace game;

int Board::addBall(Ball &ball)
{
    ball.id = lastId++;
    ballList.push_back(ball);
    return ball.id;
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

void Board::move()
{
    for (int i = 0; i < ballList.size(); i++)
    {
        auto &b = ballList[i];
        if (isWall(b.px + b.vx, b.py) || isWall(b.px - b.vx, b.py))
        {
            b.vx = -b.vx;
            outManager->playSound(b.noteNum);
        }
        
        if (isWall(b.px, b.py + b.vy) || isWall(b.px, b.py - b.vy))
        {
            b.vy = -b.vy;
            outManager->playSound(b.noteNum);
        }
        
        b.px += b.vx;
        b.py += b.vy;
    }
}

void Board::connect(Board *b, Direction d)
{
    // 未実装
}

void Board::disConnect(Direction d)
{
    // 未実装
}
