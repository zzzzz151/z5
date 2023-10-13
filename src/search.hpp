#pragma once

// clang-format off

// ----- Tunable params ------

const int MAX_DEPTH = 100;

const int PIECE_VALUES[7] = {100, 302, 320, 500, 900, 15000, 0};

const int ASPIRATION_MIN_DEPTH = 6,
          ASPIRATION_INITIAL_DELTA = 25;
const double ASPIRATION_DELTA_MULTIPLIER = 1.5;

const int IIR_MIN_DEPTH = 4;

const int RFP_MAX_DEPTH = 8,
          RFP_DEPTH_MULTIPLIER = 75;

const int NMP_MIN_DEPTH = 3,
          NMP_BASE_REDUCTION = 3,
          NMP_REDUCTION_DIVISOR = 3;

const int FP_MAX_DEPTH = 7,
          FP_BASE = 120,
          FP_MULTIPLIER = 65;

const int LMP_MAX_DEPTH = 8,
          LMP_MIN_MOVES_BASE = 2;

const int SEE_PRUNING_MAX_DEPTH = 9,
          SEE_PRUNING_NOISY_THRESHOLD = -90,
          SEE_PRUNING_QUIET_THRESHOLD = -50;

const int LMR_MIN_DEPTH = 1;
const double LMR_BASE = 1,
             LMR_DIVISOR = 2;
const int LMR_HISTORY_DIVISOR = 1024;

// ----- End tunable params -----

// ----- Global vars -----

const int POS_INFINITY = 9999999, NEG_INFINITY = -POS_INFINITY, MIN_MATE_SCORE = POS_INFINITY - 512;
Board board;
Move pvLines[MAX_DEPTH*2][MAX_DEPTH*2];
int pvLengths[MAX_DEPTH*2];
uint64_t nodes;
int maxPlyReached;
Move killerMoves[MAX_DEPTH*2][2]; // ply, move
int history[2][6][64];            // color, pieceType, targetSquare
Move counterMoves[2][1ULL << 16]; // color, moveEncoded
int lmrTable[MAX_DEPTH * 2][256]; // depth, move

// ----- End global vars -----

#include "time_management.hpp"
#include "tt.hpp"
#include "see.hpp"
#include "move_scoring.hpp"
#include "nnue.hpp"

namespace uci
{
    inline void info(int depth, int score); 
}

inline void initLmrTable()
{
    int numRows = sizeof(lmrTable) / sizeof(lmrTable[0]);
    int numCols = sizeof(lmrTable[0]) / sizeof(lmrTable[0][0]); 

    for (int depth = 0; depth < numRows; depth++)
        for (int move = 0; move < numCols; move++)
            // log(x) is ln(x)
            lmrTable[depth][move] = depth == 0 || move == 0 ? 0 : round(LMR_BASE + log(depth) * log(move) / LMR_DIVISOR);
}

inline int qSearch(int alpha, int beta, int plyFromRoot)
{
    // Quiescence saarch: search capture moves until a 'quiet' position is reached

    int originalAlpha = alpha;
    int eval = nnue::evaluate(board.colorToMove());
    if (eval >= beta) 
        return eval;
    if (alpha < eval) 
        alpha = eval;

    // probe TT
    TTEntry *ttEntry = &(tt[board.zobristHash() % tt.size()]);
    if (plyFromRoot > 0 && board.zobristHash() == ttEntry->zobristHash)
        if (ttEntry->type == EXACT || (ttEntry->type == LOWER_BOUND && ttEntry->score >= beta) || (ttEntry->type == UPPER_BOUND && ttEntry->score <= alpha))
        {
            int ret = ttEntry->score;
            if (abs(ret) >= MIN_MATE_SCORE) ret += ret < 0 ? plyFromRoot : -plyFromRoot;
            return ret;
        }

    MovesList moves = board.pseudolegalMoves(true);

    int scores[256];
    scoreMoves(moves, scores, *ttEntry, plyFromRoot);

    int bestScore = eval;
    for (int i = 0; i < moves.size(); i++)
    {
        Move capture = incrementalSort(moves, scores, i);

        // SEE pruning (skip bad captures)
        if (!SEE(board, capture)) continue;

        if (!board.makeMove(capture))
            continue; // illegal move

        nodes++;
        int score = -qSearch(-beta, -alpha, plyFromRoot + 1);
        board.undoMove();

        if (score > bestScore) 
            bestScore = score;
        else 
            continue;

        if (score >= beta) 
            return score;
        if (score > alpha) 
            alpha = score;
    }

    if (ttEntry->depth <= 0)
        storeInTT(ttEntry, 0, NULL_MOVE, bestScore, plyFromRoot, originalAlpha, beta);

    return bestScore;
}

inline int search(int depth, int alpha, int beta, int plyFromRoot, bool skipNmp = false)
{
    if (plyFromRoot > maxPlyReached) 
        maxPlyReached = plyFromRoot;

    if (checkIsTimeUp())
        return 0;

    pvLengths[plyFromRoot] = 0;

    if (plyFromRoot > 0 && (board.isDraw() || board.isRepetition()))
        return 0;

    bool inCheck = board.inCheck();
    if (inCheck) depth++; // Check extension

    if (depth <= 0) return qSearch(alpha, beta, plyFromRoot);

    // Probe TT
    TTEntry *ttEntry = &(tt[board.zobristHash() % tt.size()]);
    if (plyFromRoot > 0 && ttEntry->zobristHash == board.zobristHash() && ttEntry->depth >= depth)
        if (ttEntry->type == EXACT || (ttEntry->type == LOWER_BOUND && ttEntry->score >= beta) || (ttEntry->type == UPPER_BOUND && ttEntry->score <= alpha))
        {
            int ret = ttEntry->score;
            if (abs(ret) >= MIN_MATE_SCORE) ret += ret < 0 ? plyFromRoot : -plyFromRoot;
            return ret;
        }

    // IIR (Internal iterative reduction)
    if (ttEntry->zobristHash != board.zobristHash() && depth >= IIR_MIN_DEPTH && !inCheck)
        depth--;

    bool pvNode = beta - alpha > 1 || plyFromRoot == 0;
    int eval = nnue::evaluate(board.colorToMove());

    if (!pvNode && !inCheck)
    {
        // RFP (Reverse futility pruning)
        if (depth <= RFP_MAX_DEPTH && eval >= beta + RFP_DEPTH_MULTIPLIER * depth)
            return eval;

        // NMP (Null move pruning)
        if (depth >= NMP_MIN_DEPTH && !skipNmp && eval >= beta && board.hasNonPawnMaterial(board.colorToMove()))
        {
            board.makeNullMove();
            int score = -search(depth - NMP_BASE_REDUCTION - depth / NMP_REDUCTION_DIVISOR, -beta, -alpha, plyFromRoot + 1, true);
            board.undoNullMove();

            if (score >= MIN_MATE_SCORE) return beta;
            if (score >= beta) return score;
        }
    }

    MovesList moves = board.pseudolegalMoves();
    int scores[256];
    scoreMoves(moves, scores, *ttEntry, plyFromRoot);
    int bestScore = NEG_INFINITY;
    Move bestMove;
    int legalMovesPlayed = 0;
    int stm = board.colorToMove();
    int* pointersFailLowQuietsHistory[256], numFailLowQuiets = 0;
    int originalAlpha = alpha;

    for (int i = 0; i < moves.size(); i++)
    {
        Move move = incrementalSort(moves, scores, i);
        bool historyMoveOrLosing = scores[i] < KILLER_SCORE;
        int lmr = lmrTable[depth][legalMovesPlayed];
        bool isQuietMove = !board.isCapture(move) && move.promotionPieceType() == PieceType::NONE;

        // Moves loop pruning
        if (historyMoveOrLosing && bestScore > -MIN_MATE_SCORE)
        {
            // LMP (Late move pruning)
            if (depth <= LMP_MAX_DEPTH && legalMovesPlayed >= LMP_MIN_MOVES_BASE + depth * depth * 2)
                break;

            // FP (Futility pruning)
            if (depth <= FP_MAX_DEPTH && alpha < MIN_MATE_SCORE && eval + FP_BASE + max(depth - lmr, 0) * FP_MULTIPLIER <= alpha)
                break;

            // SEE pruning
            int threshold = depth * (isQuietMove ? SEE_PRUNING_QUIET_THRESHOLD : SEE_PRUNING_NOISY_THRESHOLD);
            if (depth <= SEE_PRUNING_MAX_DEPTH && !SEE(board, move, threshold))
                continue;
        }

        if (!board.makeMove(move))
            continue; // illegal move

        legalMovesPlayed++;
        nodes++;
        Square targetSquare = move.to();
        int pieceType = move.promotionPieceType() != PieceType::NONE ? (int)PieceType::PAWN : (int)board.pieceTypeAt(targetSquare);
        int *pointerMoveHistory = &(history[stm][pieceType][targetSquare]);

        int score = 0;
        // PVS (Principal variation search)
        if (legalMovesPlayed == 1)
            score = -search(depth - 1, -beta, -alpha, plyFromRoot + 1);
        else
        {
            // LMR (Late move reductions)
            bool canLmr = depth >= LMR_MIN_DEPTH && !inCheck && historyMoveOrLosing;
            if (canLmr)
            {
                lmr -= board.inCheck(); // reduce checks less
                lmr -= pvNode;          // reduce pv nodes less

                // reduce quiets with good history less and vice versa
                if (isQuietMove) lmr -= round(*pointerMoveHistory / (double)LMR_HISTORY_DIVISOR);

                // if lmr is negative, we would have an extension instead of a reduction
                if (lmr < 0) lmr = 0;
            }
            else
                lmr = 0;

            score = -search(depth - 1 - lmr, -alpha - 1, -alpha, plyFromRoot + 1);
            if (score > alpha && (score < beta || lmr > 0))
                score = -search(depth - 1, -beta, -alpha, plyFromRoot + 1);
        }

        board.undoMove();
        if (checkIsTimeUp()) return 0;

        if (isQuietMove)
            pointersFailLowQuietsHistory[numFailLowQuiets++] = pointerMoveHistory;

        if (score > bestScore)
            bestScore = score;

        if (score <= alpha)
            continue;

        alpha = score;
        bestMove = move;

        if (pvNode)
        {
            // Update pv line
            plyFromRoot++;
            int subPvLineLength = pvLengths[plyFromRoot];
            pvLengths[plyFromRoot - 1] = 1 + subPvLineLength;
            pvLines[plyFromRoot - 1][0] = move;
            memcpy(&(pvLines[plyFromRoot - 1][1]), pvLines[plyFromRoot], subPvLineLength * sizeof(Move)); // dst, src, size
            plyFromRoot--;
        }

        if (score < beta) 
            continue;

        // Beta cutoff

        if (isQuietMove && move != killerMoves[plyFromRoot][0])
        {
            killerMoves[plyFromRoot][1] = killerMoves[plyFromRoot][0];
            killerMoves[plyFromRoot][0] = move;
        }
        if (isQuietMove)
        {
            counterMoves[(int)board.enemyColor()][board.lastMove().move()] = move;
            *pointerMoveHistory += depth * depth;

            // Penalize quiets that failed low (this one failed high so dont penalize it)
            for (int i = 0; i < numFailLowQuiets - 1; i++)
                *pointersFailLowQuietsHistory[i] -= depth * depth;
        }

        break; // Beta cutoff
    }

    if (legalMovesPlayed == 0) 
        return inCheck ? NEG_INFINITY + plyFromRoot : 0; // checkmate/draw

    storeInTT(ttEntry, depth, bestMove, bestScore, plyFromRoot, originalAlpha, beta);    

    return bestScore;
}

inline int aspiration(int maxDepth, int score)
{
    int delta = ASPIRATION_INITIAL_DELTA;
    int alpha = max(NEG_INFINITY, score - delta);
    int beta = min(POS_INFINITY, score + delta);
    int depth = maxDepth;

    while (true)
    {
        score = search(depth, alpha, beta, 0);

        if (checkIsTimeUp()) return 0;

        if (score >= beta)
        {
            beta = min(beta + delta, POS_INFINITY);
            depth--;
        }
        else if (score <= alpha)
        {
            beta = (alpha + beta) / 2;
            alpha = max(alpha - delta, NEG_INFINITY);
            depth = maxDepth;
        }
        else
            break;

        delta *= ASPIRATION_DELTA_MULTIPLIER;
    }

    return score;
}

inline void iterativeDeepening()
{
    // clear pv's, killers, countermoves and history
    memset(&(pvLines[0][0]), 0, sizeof(pvLines));
    memset(&(pvLengths[0]), 0, sizeof(pvLengths));
    memset(&(killerMoves[0][0]), 0, sizeof(killerMoves));
    memset(&(counterMoves[0][0]), 0, sizeof(counterMoves));
    memset(&(history[0][0][0]), 0, sizeof(history));

    nodes = 0;
    int iterationDepth = 1, score = 0;

    while (iterationDepth <= MAX_DEPTH)
    {
        maxPlyReached = -1;
        int scoreBefore = score;

        score = iterationDepth >= ASPIRATION_MIN_DEPTH 
                ? aspiration(iterationDepth, score) 
                : search(iterationDepth, NEG_INFINITY, POS_INFINITY, 0);

        if (checkIsTimeUp())
        {
            uci::info(iterationDepth, scoreBefore);
            break;
        }

        uci::info(iterationDepth, score);
        iterationDepth++;
    }
}

