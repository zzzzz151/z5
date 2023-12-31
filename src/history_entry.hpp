#pragma once

// clang-format off

struct HistoryEntry
{
    i32 mainHistory = 0;
    i32 countermoveHistory[6][64];  // [lastMovePieceType][lastMoveTargetSquare]
    i32 followupMoveHistory[6][64]; // [lastLastMovePieceType][lastLastMoveTargetSquare]
    i32 noisyHistory = 0;

    inline i32 quietHistory(Board &board)
    {
        // add main history
        i32 quietHist = mainHistory;
        
        // add countermove history using last move
        Move lastMove;
        if ((lastMove = board.getNthToLastMove(1)) != MOVE_NONE)
        {
            int pieceType = (int)board.getNthToLastMovePieceType(1);
            int targetSq = (int)lastMove.to();
            quietHist += countermoveHistory[pieceType][targetSq];
        }

        // add follow-up move history using 2nd-to-last move
        Move lastLastMove;
        if ((lastLastMove = board.getNthToLastMove(2)) != MOVE_NONE)
        {
            int pieceType = (int)board.getNthToLastMovePieceType(2);
            int targetSq = (int)lastLastMove.to();
            quietHist += followupMoveHistory[pieceType][targetSq];
        }
       
        return quietHist;
    }

    inline void updateQuietHistory(Board &board, i32 bonus)
    {
        // Update main history
        mainHistory += bonus - mainHistory * abs(bonus) / search::historyMax.value;

        // Update countermove history using last move
        Move lastMove;
        if ((lastMove = board.getNthToLastMove(1)) != MOVE_NONE)
        {
            int pieceType = (int)board.getNthToLastMovePieceType(1);
            int targetSq = (int)lastMove.to();
            i32 countermoveHistDelta = bonus - countermoveHistory[pieceType][targetSq] * abs(bonus) / search::historyMax.value;
            countermoveHistory[pieceType][targetSq] += countermoveHistDelta;
        }

        // Update follow-up move history using 2nd-to-last move
        Move lastLastMove;
        if ((lastLastMove = board.getNthToLastMove(2)) != MOVE_NONE)
        {
            int pieceType = (int)board.getNthToLastMovePieceType(2);
            int targetSq = (int)lastLastMove.to();
            i32 followupHistDelta = bonus - followupMoveHistory[pieceType][targetSq] * abs(bonus) / search::historyMax.value;
            followupMoveHistory[pieceType][targetSq] += followupHistDelta;
        }
    }

    inline void updateNoisyHistory(Board &board, i32 bonus)
    {
        noisyHistory += bonus - noisyHistory * abs(bonus) / search::historyMax.value;
    }

};