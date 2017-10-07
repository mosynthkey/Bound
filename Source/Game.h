//
//  Game.hpp
//  Bound - App
//
//  Created by Masaki Ono on 2017/10/03.
//

#pragma once

#include <vector>
#include "MidiOutManager.h"

#define BLOCKS_SIZE 15
#define LEDDECAY 0.7 // 減衰速度の乗数

#define NAMESPACE_GAME_BEGIN namespace game {
#define NAMESPACE_GAME_END   }

NAMESPACE_GAME_BEGIN
struct Ball
{
    float px, py; // 位置
    float vx, vy; // 速度
    float r, g, b;
    int lifespan; // 何ターンで消えるのか(-1で無限)
    int id;
    int noteNum; // volca sampleに繋いだときはchとして使う
};

enum Direction
{
    Direction_Top = 0,
    Direction_Bottom,
    Direction_Left,
    Direction_Right,
    Direction_Num,
};

enum Charactor
{
    Charactor_Ball,
    Charactor_Wall,
    Charactor_Nothing,
    Charactor_Num,
};

struct BoardState
{
    float r, g, b;
    Charactor c; // 名前よくない。ボードのこの位置に何があるのか。
};

class Board
{
public:
    Board()
    {
        for (int i = 0; i < Direction_Num; i++)
        {
            connectedBoard[i] = nullptr;
        }
            
        lastId = 0;
        seq_i = 0;
        outManager = &MidiOutManager::getSharedInstance();
        
        sequence = {40, 42, 44, 46, 48, 50, 52, 50, 48, 46, 44, 42};
    }
    
    ~Board();
    
    int addBall(Ball &ball); // ボールを置く。idを返す。
    void deleteBall(int id);
    void deleteAllBalls(void);
    
    void move(); // タイマーとか呼び出す。ゲームを進める。
    
    void connect(Board *b, Direction d);
    void disConnect(Direction d);
    
    bool isWall(float x, float y)
    {
        if ((x < 0 && connectedBoard[Direction_Left] == nullptr) ||
            (x > BLOCKS_SIZE - 1 && connectedBoard[Direction_Right] == nullptr) ||
            (y < 0 && connectedBoard[Direction_Top] == nullptr) ||
            (y > BLOCKS_SIZE - 1 && connectedBoard[Direction_Bottom] == nullptr))
        {
            return true;
        }
        
        return false;
    }
    
    bool isWarpZone(float x, float y)
    {
        if ((x < 0 && connectedBoard[Direction_Left] != nullptr) ||
            (x > BLOCKS_SIZE - 1 && connectedBoard[Direction_Right] != nullptr) ||
            (y < 0 && connectedBoard[Direction_Top] != nullptr) ||
            (y > BLOCKS_SIZE - 1 && connectedBoard[Direction_Bottom] != nullptr))
        {
            return true;
        }
        
        return false;
    }
    
    BoardState getBoardState(unsigned int x, unsigned int y)
    {
        BoardState result;
        
        if (isWall(x, y))
        {
            result.c = Charactor_Wall;
            return result;
        }
        
        for (auto b : ballList)
        {
            if ((int)b.px == x && (int)b.py == y)
            {
                result.r = b.r;
                result.g = b.g;
                result.b = b.b;
                result.c = Charactor_Ball;
                return result;
            }
        }
        
        result.c = Charactor_Nothing;
        return result;
    }
    
        int lastId;
private:
    Board *connectedBoard[Direction_Num];
    std::vector<Ball> ballList;
    std::vector<Ball> warpBallList;
    MidiOutManager *outManager;
    
    std::vector<int> sequence;
    int seq_i;
};

NAMESPACE_GAME_END
