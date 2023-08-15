#ifndef EVAL_HPP
#define EVAL_HPP

#include "chess.hpp"
using namespace chess;

PieceType PIECE_TYPES[7] = {PieceType::PAWN, PieceType::KNIGHT, PieceType::BISHOP, PieceType::ROOK, PieceType::QUEEN, PieceType::KING, PieceType::NONE};
int PIECE_VALUES[7] = {100, 302, 320, 500, 900, 15000, 0};
int PIECE_PHASE[] = {0, 1, 1, 2, 4, 0, 0};
U64 PSTs[] = {657614902731556116, 420894446315227099, 384592972471695068, 312245244820264086, 
                    364876803783607569, 366006824779723922, 366006826859316500, 786039115310605588, 
                    421220596516513823, 366011295806342421, 366006826859316436, 366006896669578452, 
                    162218943720801556, 440575073001255824, 657087419459913430, 402634039558223453, 
                    347425219986941203, 365698755348489557, 311382605788951956, 147850316371514514, 
                    329107007234708689, 402598430990222677, 402611905376114006, 329415149680141460, 
                    257053881053295759, 291134268204721362, 492947507967247313, 367159395376767958, 
                    384021229732455700, 384307098409076181, 402035762391246293, 328847661003244824, 
                    365712019230110867, 366002427738801364, 384307168185238804, 347996828560606484, 
                    329692156834174227, 365439338182165780, 386018218798040211, 456959123538409047,
                    347157285952386452, 365711880701965780, 365997890021704981, 221896035722130452,
                    384289231362147538, 384307167128540502, 366006826859320596, 366006826876093716,
                    366002360093332756, 366006824694793492, 347992428333053139, 457508666683233428,
                    329723156783776785, 329401687190893908, 366002356855326100, 366288301819245844,
                    329978030930875600, 420621693221156179, 422042614449657239, 384602117564867863,
                    419505151144195476, 366274972473194070, 329406075454444949, 275354286769374224, 
                    366855645423297932, 329991151972070674, 311105941360174354, 256772197720318995,
                    365993560693875923, 258219435335676691, 383730812414424149, 384601907111998612,
                    401758895947998613, 420612834953622999, 402607438610388375, 329978099633296596, 67159620133902};

inline int getPSTValue(int psq)
{
    return (int)(((PSTs[psq / 10] >> (6 * (psq % 10))) & 63) - 20) * 8;
}

inline int evaluate(Board board)
{
    int mg = 0, eg = 0, phase = 0;

    for (bool stm : {true, false})
    {
        for (int i = 0; i <= 5; i++)
        {
            int ind;
            Bitboard mask = board.pieces(PIECE_TYPES[i], stm ? Color::WHITE : Color::BLACK);
            while (mask != 0)
            {
                phase += PIECE_PHASE[i];
                ind = 128 * i + builtin::poplsb(mask) ^ (stm ? 56 : 0);
                mg += getPSTValue(ind) + PIECE_VALUES[i];
                eg += getPSTValue(ind + 64) + PIECE_VALUES[i];
            }
        }

        mg = -mg;
        eg = -eg;
    }

    return (mg * phase + eg * (24 - phase)) / 24 * (board.sideToMove() == Color::WHITE ? 1 : -1);
}

#endif