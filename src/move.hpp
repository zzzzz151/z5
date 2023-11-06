#pragma once

// clang-format off
#include <cassert>
#include "types.hpp"
#include "utils.hpp"
using namespace std;

struct Move
{
    private:

    // 16 bits: ffffff tttttt FFFF (f = from, t = to, F = flag)  
    uint16_t moveEncoded = 0;

    public:

    const static uint16_t NULL_FLAG = 0x0000,
                          NORMAL_FLAG = 0x0005,
                          CASTLING_FLAG = 0x0006,
                          EN_PASSANT_FLAG = 0x0007,
                          PAWN_TWO_UP_FLAG = 0x0008,
                          KNIGHT_PROMOTION_FLAG = 0x0001,
                          BISHOP_PROMOTION_FLAG = 0x0002,
                          ROOK_PROMOTION_FLAG = 0x0003,
                          QUEEN_PROMOTION_FLAG = 0x0004;

    inline Move() = default;

    // Custom == operator
    inline bool operator==(const Move &other) const {
        return moveEncoded == other.moveEncoded;
    }

    // Custom != operator
    inline bool operator!=(const Move &other) const {
        return moveEncoded != other.moveEncoded;
    }

    inline Move(Square from, Square to, uint16_t typeFlag)
    {
        // from/to: 00100101
        // (uint16_t)from/to: 00000000 00100101

        moveEncoded = ((uint16_t)from << 10);
        moveEncoded |= ((uint16_t)to << 4);
        moveEncoded |= typeFlag;
    }

    inline Move(string from, string to, uint16_t typeFlag) : Move(strToSquare(from), strToSquare(to), typeFlag) {}

    inline uint16_t getMove() { return moveEncoded; }

    inline Square from() { return (moveEncoded >> 10) & 0b111111; }

    inline Square to() { return (moveEncoded >> 4) & 0b111111; }

    inline uint16_t typeFlag() { return moveEncoded & 0x000F; }

    inline PieceType promotionPieceType()
    {
        uint16_t flag = typeFlag();
        if (flag < 1 || flag > 4)
            return PieceType::NONE;
        return (PieceType)flag;
    }

    static inline Move fromUci(string uci, array<Piece, 64> pieces)
    {
        Square from = strToSquare(uci.substr(0,2));
        Square to = strToSquare(uci.substr(2,4));

        if (uci.size() == 5) // promotion
        {
            char promotionLowerCase = uci.back(); // last char of string
            uint16_t typeFlag = QUEEN_PROMOTION_FLAG;

            if (promotionLowerCase == 'n') 
                typeFlag = KNIGHT_PROMOTION_FLAG;
            else if (promotionLowerCase == 'b') 
                typeFlag = BISHOP_PROMOTION_FLAG;
            else if (promotionLowerCase == 'r') 
                typeFlag = ROOK_PROMOTION_FLAG;

            return Move(from, to, typeFlag);
        }
        else if (pieceToPieceType(pieces[from]) == PieceType::KING)
        {
            if (abs((int)to - (int)from) == 2)
                return Move(from, to, Move::CASTLING_FLAG);
        }
        else if (pieceToPieceType(pieces[from]) == PieceType::PAWN)
        { 
            int bitboardSquaresTraveled = abs((int)to - (int)from);
            if (bitboardSquaresTraveled == 16)
                return Move(from, to, PAWN_TWO_UP_FLAG);
            if (bitboardSquaresTraveled != 8 && pieces[to] == Piece::NONE)
                return Move(from, to, EN_PASSANT_FLAG);
        }

        return Move(from, to, NORMAL_FLAG);
    }

    inline string toUci()
    {
        string str = squareToStr[from()] + squareToStr[to()];
        uint16_t myTypeFlag = typeFlag();

        if (myTypeFlag == QUEEN_PROMOTION_FLAG) 
            str += "q";
        else if (myTypeFlag == KNIGHT_PROMOTION_FLAG) 
            str += "n";
        else if (myTypeFlag == BISHOP_PROMOTION_FLAG) 
            str += "b";
        else if (myTypeFlag == ROOK_PROMOTION_FLAG) 
            str += "r";

        return str;
    }
    
};

Move NULL_MOVE = Move();

struct MovesList
{
    private:

    Move moves[256];
    uint8_t numMoves = 0;

    public:

    inline MovesList() = default;

    inline void add(Move move) { 
        assert(numMoves < 255);
        moves[numMoves++] = move; 
    }

    inline uint8_t size() { return numMoves; }

    inline Move operator[](int i) {
        assert(i >= 0 && i < numMoves);
        return moves[i];
    }

    inline void swap(int i, int j)
    {
    	assert(i >= 0 && j >= 0 && i < numMoves && j < numMoves);
        Move temp = moves[i];
        moves[i] = moves[j];
        moves[j] = temp;
    }
};
