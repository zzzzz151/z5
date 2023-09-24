#ifndef BOARD_HPP
#define BOARD_HPP

// clang-format off
#include <cstdint>
#include <vector>
#include <random>
#include "types.hpp"
#include "builtin.hpp"
#include "utils.hpp"
#include "move.hpp"
using namespace std;

struct BoardInfo
{
    public:

    bool castlingRights[2][2]; // castlingRights[color][CASTLE_SHORT or CASTLE_LONG]
    Square enPassantTargetSquare;
    uint16_t pliesSincePawnMoveOrCapture;
    Piece capturedPiece;

    inline BoardInfo(bool argCastlingRights[2][2], Square argEnPassantTargetSquare, 
                     uint16_t argPliesSincePawnMoveOrCapture, Piece argCapturedPiece)
    {
        castlingRights[WHITE][0] = argCastlingRights[WHITE][0];
        castlingRights[WHITE][1] = argCastlingRights[WHITE][1];
        castlingRights[BLACK][0] = argCastlingRights[BLACK][0];
        castlingRights[BLACK][1] = argCastlingRights[BLACK][1];

        enPassantTargetSquare = argEnPassantTargetSquare;
        pliesSincePawnMoveOrCapture = argPliesSincePawnMoveOrCapture;
        capturedPiece = argCapturedPiece;
    }

};

class Board
{
    private:

    Piece pieces[64];
    uint64_t occupied, piecesBitboards[6][2];

    bool castlingRights[2][2]; // color, CASTLE_SHORT/CASTLE_LONG
    const static uint8_t CASTLE_SHORT = 0, CASTLE_LONG = 1;

    Color color;
    Square enPassantTargetSquare;
    uint16_t pliesSincePawnMoveOrCapture, currentMoveCounter;

    vector<BoardInfo> states;

    static inline uint64_t zobristTable[64][12];
    static inline uint64_t zobristTable2[10];
    
    public:

    inline Board(string fen)
    {
        states = {};

        fen = trim(fen);
        vector<string> fenSplit = splitString(fen, ' ');

        parseFenRows(fenSplit[0]);
        color = fenSplit[1] == "b" ? BLACK : WHITE;
        parseFenCastlingRights(fenSplit[2]);

        string strEnPassantSquare = fenSplit[3];
        enPassantTargetSquare = strEnPassantSquare == "-" ? 0 : strToSquare(strEnPassantSquare);

        pliesSincePawnMoveOrCapture = fenSplit.size() >= 5 ? stoi(fenSplit[4]) : 0;
        currentMoveCounter = fenSplit.size() >= 6 ? stoi(fenSplit[5]) : 1;
    }

    private:

    inline void parseFenRows(string fenRows)
    {
        for (int sq= 0; sq < 64; sq++)
            pieces[sq] = Piece::NONE;

        occupied = (uint64_t)0;

        for (int pt = 0; pt < 6; pt++)
            piecesBitboards[pt][WHITE] = piecesBitboards[pt][BLACK] = 0;

        int currentRank = 7; // start from top rank
        int currentFile = 0;
        for (int i = 0; i < fenRows.length(); i++)
        {
            char thisChar = fenRows[i];
            if (thisChar == '/')
            {
                currentRank--;
                currentFile = 0;
            }
            else if (isdigit(thisChar))
                currentFile += charToInt(thisChar);
            else
            {
                placePiece(charToPiece[thisChar], currentRank * 8 + currentFile);
                currentFile++;
            }
        }
    }

    inline void parseFenCastlingRights(string fenCastlingRights)
    {
        castlingRights[WHITE][CASTLE_SHORT] = false;
        castlingRights[WHITE][CASTLE_LONG] = false;
        castlingRights[BLACK][CASTLE_SHORT] = false;
        castlingRights[BLACK][CASTLE_LONG] = false;

        if (fenCastlingRights == "-") return;

        for (int i = 0; i < fenCastlingRights.length(); i++)
        {
            char thisChar = fenCastlingRights[i];
            if (thisChar == 'K') castlingRights[WHITE][CASTLE_SHORT] = true;
            else if (thisChar == 'Q') castlingRights[WHITE][CASTLE_LONG] = true;
            else if (thisChar == 'k') castlingRights[BLACK][CASTLE_SHORT] = true;
            else if (thisChar == 'q') castlingRights[BLACK][CASTLE_LONG] = true;
        }
    }

    public:

    inline string fen()
    {
        string myFen = "";

        for (int rank = 7; rank >= 0; rank--)
        {
            int emptySoFar = 0;
            for (int file = 0; file < 8; file++)
            {
                Square square = rank * 8 + file;
                Piece piece = pieces[square];
                if (piece != Piece::NONE) {
                    if (emptySoFar > 0) myFen += to_string(emptySoFar);
                    myFen += string(1, pieceToChar[piece]);
                    emptySoFar = 0;
                }
                else
                    emptySoFar++;
            }
            if (emptySoFar > 0) myFen += to_string(emptySoFar);
            myFen += "/";
        }
        myFen.pop_back(); // remove last '/'

        myFen += color == BLACK ? " b " : " w ";

        string strCastlingRights = "";
        if (castlingRights[WHITE][CASTLE_SHORT]) strCastlingRights += "K";
        if (castlingRights[WHITE][CASTLE_LONG]) strCastlingRights += "Q";
        if (castlingRights[BLACK][CASTLE_SHORT]) strCastlingRights += "k";
        if (castlingRights[BLACK][CASTLE_LONG]) strCastlingRights += "q";
        if (strCastlingRights.size() == 0) strCastlingRights = "-";
        myFen += strCastlingRights;

        string strEnPassantSquare = enPassantTargetSquare == 0 ? "-" : squareToStr[enPassantTargetSquare];
        myFen += " " + strEnPassantSquare;
        
        myFen += " " + to_string(pliesSincePawnMoveOrCapture);
        myFen += " " + to_string(currentMoveCounter);

        return myFen;
    }

    inline void printBoard()
    {
        string str = "";
        for (Square i = 0; i < 8; i++)
        {
            for (Square j = 0; j < 8; j++)
            {
                int square = i * 8 + j;
                str += pieces[square] == Piece::NONE ? "." : string(1, pieceToChar[pieces[square]]);
                str += " ";
            }
            str += "\n";
        }

        cout << str;
    }

    inline Piece* piecesBySquare()
    {
        return pieces;
    }

    inline Color colorToMove()
    {
        return color;
    }

    inline Piece pieceAt(Square square)
    {
        return pieces[square];
    }

    inline Piece pieceAt(string square)
    {
        return pieces[strToSquare(square)];
    }

    inline PieceType pieceTypeAt(Square square)
    {
        return pieceToPieceType(pieces[square]);
    }

    inline PieceType pieceTypeAt(string square)
    {
        return pieceTypeAt(strToSquare(square));
    }

    inline uint64_t occupancy()
    {
        return occupied;
    }

    inline uint64_t pieceBitboard(PieceType pieceType, Color color = NULL_COLOR)
    {
        if (color == NULL_COLOR)
            return piecesBitboards[(uint8_t)pieceType][WHITE] | piecesBitboards[(uint8_t)pieceType][BLACK];
        return piecesBitboards[(uint8_t)pieceType][color];
    }

    inline uint64_t colorBitboard(Color color)
    {
        uint64_t bb = 0;
        for (int pt = 0; pt <= 5; pt++)
            bb |= piecesBitboards[pt][color];
        return bb;
    }

    inline uint64_t us()
    {
        return colorBitboard(color);
    }

    inline uint64_t them()
    {
        return colorBitboard(color == WHITE ? BLACK : WHITE);
    }

    private:

    inline void placePiece(Piece piece, Square square)
    {
        if (piece == Piece::NONE) return;
        pieces[square] = piece;

        PieceType pieceType = pieceToPieceType(piece);
        Color color = pieceColor(piece);
        uint64_t squareBit = (1ULL << square);

        occupied |= squareBit;
        piecesBitboards[(uint8_t)pieceType][color] |= squareBit;
    }

    inline void removePiece(Square square)
    {
        if (pieces[square] == Piece::NONE) return;
        PieceType pieceType = pieceToPieceType(pieces[square]);
        Color color = pieceColor(pieces[square]);
        pieces[square] = Piece::NONE;
        uint64_t squareBit = (1ULL << square);
        occupied ^= squareBit;
        piecesBitboards[(uint8_t)pieceType][color] ^= squareBit;
    }

    public:

    inline static void initZobrist()
    {
        random_device rd;  // Create a random device to seed the random number generator
        mt19937_64 gen(rd());  // Create a 64-bit Mersenne Twister random number generator

        for (int sq = 0; sq < 64; sq++)
            for (int pt = 0; pt < 12; pt++)
            {
                uniform_int_distribution<uint64_t> distribution;
                uint64_t randomNum = distribution(gen);
                zobristTable[sq][pt] = randomNum;
            }

        for (int i = 0; i < 10; i++)
        {
            uniform_int_distribution<uint64_t> distribution;
            uint64_t randomNum = distribution(gen);
            zobristTable2[i] = randomNum;
        }
    }

    inline uint64_t zobristHash()
    {
        uint64_t hash = color == WHITE ? zobristTable2[0] : zobristTable2[1];

        uint64_t castlingHash = castlingRights[WHITE][CASTLE_SHORT] 
                                + 2 * castlingRights[WHITE][CASTLE_LONG] 
                                + 4 * castlingRights[BLACK][CASTLE_SHORT] 
                                + 8 * castlingRights[BLACK][CASTLE_LONG];
        hash ^= castlingHash;

        if (enPassantTargetSquare != 0)
            hash ^= zobristTable2[2] / (uint64_t)(squareFile(enPassantTargetSquare) * squareFile(enPassantTargetSquare));

        for (int sq = 0; sq < 64; sq++)
        {
            Piece piece = pieces[sq];
            if (piece == Piece::NONE) continue;
            hash ^= zobristTable[sq][(int)piece];
        }

        return hash;
    }

    inline bool makeMove(Move move)
    {
        Square from = move.from();
        Square to = move.to();
        auto typeFlag = move.typeFlag();
        bool capture = isCapture(move);
        pushState(pieces[to]);

        Piece piece = pieces[from];
        removePiece(from);
        removePiece(to);

        if (typeFlag == move.QUEEN_PROMOTION_FLAG)
        {
            placePiece(color == WHITE ? Piece::WHITE_QUEEN : Piece::BLACK_QUEEN, to);
            goto piecesProcessed;
        }
        else if (typeFlag == move.KNIGHT_PROMOTION_FLAG)
        {
            placePiece(color == WHITE ? Piece::WHITE_KNIGHT : Piece::BLACK_KNIGHT, to);
            goto piecesProcessed;
        }
        else if (typeFlag == move.BISHOP_PROMOTION_FLAG)
        {
            placePiece(color == WHITE ? Piece::WHITE_BISHOP : Piece::BLACK_BISHOP, to);
            goto piecesProcessed;
        }
        else if (typeFlag == move.ROOK_PROMOTION_FLAG)
        {
            placePiece(color == WHITE ? Piece::WHITE_ROOK : Piece::BLACK_ROOK, to);
            goto piecesProcessed;
        }

        placePiece(piece, to);

        if (typeFlag == move.CASTLING_FLAG)
        {
            if (to == 6) { removePiece(7); placePiece(Piece::WHITE_ROOK, 5); }
            else if (to == 2) { removePiece(0); placePiece(Piece::WHITE_ROOK, 3); }
            else if (to == 62) { removePiece(63); placePiece(Piece::BLACK_ROOK, 61); }
            else if (to == 58) { removePiece(56); placePiece(Piece::BLACK_ROOK, 59); }
        }
        else if (typeFlag == move.EN_PASSANT_FLAG)
        {
            Square capturedPawnSquare = color == WHITE ? to - 8 : to + 8;
            removePiece(capturedPawnSquare);
        }

        piecesProcessed:

        if (inCheck())
        {
            undoMove(move);
            return false; // move is illegal
        }

        PieceType pieceTypeMoved = pieceToPieceType(pieces[to]);
        if (pieceTypeMoved == PieceType::KING || pieceTypeMoved == PieceType::ROOK)
            castlingRights[color][CASTLE_SHORT] = castlingRights[color][CASTLE_LONG] = false;

        color = color == WHITE ? BLACK : WHITE;
        if (color == WHITE)
            currentMoveCounter++;

        if (pieceToPieceType(piece) == PieceType::PAWN || capture)
            pliesSincePawnMoveOrCapture = 0;
        else
            pliesSincePawnMoveOrCapture++;

        // Check if this move created an en passant
        enPassantTargetSquare = 0;
        if (pieceToPieceType(piece) == PieceType::PAWN)
        { 
            uint8_t rankFrom = squareRank(from), 
                    rankTo = squareRank(to);

            bool pawnTwoUp = (rankFrom == 1 && rankTo == 3) || (rankFrom == 6 && rankTo == 4);
            if (pawnTwoUp)
            {
                char file = squareFile(from);

                // we already switched color
                Square possibleEnPassantTargetSquare = color == BLACK ? to-8 : to+8;
                Piece enemyPawnPiece = color == BLACK ? Piece::BLACK_PAWN : Piece::WHITE_PAWN;

                if (file != 'a' && pieces[to-1] == enemyPawnPiece)
                    enPassantTargetSquare = possibleEnPassantTargetSquare;
                else if (file != 'h' && pieces[to+1] == enemyPawnPiece)
                    enPassantTargetSquare = possibleEnPassantTargetSquare;
            }
        }

        return true; // move is legal

    }

    inline void undoMove(Move move)
    {
        color = color == WHITE ? BLACK : WHITE;
        Square from = move.from();
        Square to = move.to();
        auto typeFlag = move.typeFlag();

        Piece capturedPiece = states[states.size()-1].capturedPiece;
        Piece piece = pieces[to];
        removePiece(to);

        if (typeFlag == move.CASTLING_FLAG)
        {
            // Replace king
            if (color == WHITE) 
                placePiece(Piece::WHITE_KING, 4);
            else 
                placePiece(Piece::BLACK_KING, 60);

            // Replace rook
            if (to == 6)
            { 
                removePiece(5); 
                placePiece(Piece::WHITE_ROOK, 7);
            }
            else if (to == 2) 
            {
                removePiece(3);
                placePiece(Piece::WHITE_ROOK, 0);
            }
            else if (to == 62) { 
                removePiece(61);
                placePiece(Piece::BLACK_ROOK, 63);
            }
            else if (to == 58) 
            {
                removePiece(59);
                placePiece(Piece::BLACK_ROOK, 56);
            }
        }
        else if (typeFlag == move.EN_PASSANT_FLAG)
        {
            placePiece(color == WHITE ? Piece::WHITE_PAWN : Piece::BLACK_PAWN, from);
            Square capturedPawnSquare = color == WHITE ? to - 8 : to + 8;
            placePiece(color == WHITE ? Piece::BLACK_PAWN : Piece::WHITE_PAWN, capturedPawnSquare);
        }
        else if (move.promotionPieceType() != PieceType::NONE)
        {
            placePiece(color == WHITE ? Piece::WHITE_PAWN : Piece::BLACK_PAWN, from);
            placePiece(capturedPiece, to); // promotion + capture, so replace the captured piece
        }
        else
        {
            placePiece(piece, from);
            placePiece(capturedPiece, to);
        }

        if (color == BLACK) 
            currentMoveCounter--;

        pullState();

    }

    inline bool isCapture(Move move)
    {
        if (move.typeFlag() == move.EN_PASSANT_FLAG) return true;
        if (pieces[move.to()] != Piece::NONE) return true;
        return false;
    }

    private:

    inline void pushState(Piece capturedPiece)
    {
        BoardInfo state = BoardInfo(castlingRights, enPassantTargetSquare, pliesSincePawnMoveOrCapture, capturedPiece);
        states.push_back(state); // append
    }

    inline void pullState()
    {
        BoardInfo state = states[states.size()-1];
        states.pop_back();

        castlingRights[WHITE][CASTLE_SHORT] = state.castlingRights[WHITE][CASTLE_SHORT];
        castlingRights[WHITE][CASTLE_LONG] = state.castlingRights[WHITE][CASTLE_LONG];
        castlingRights[BLACK][CASTLE_SHORT] = state.castlingRights[BLACK][CASTLE_SHORT];
        castlingRights[BLACK][CASTLE_LONG] = state.castlingRights[BLACK][CASTLE_LONG];

        enPassantTargetSquare = state.enPassantTargetSquare;
        pliesSincePawnMoveOrCapture = state.pliesSincePawnMoveOrCapture;
    }

    public:

    inline MovesList pseudolegalMoves(bool capturesOnly = false)
    {
        uint64_t usBb = us();
        uint64_t usBb2 = usBb;
        MovesList moves;

        while (usBb > 0)
        {
            Square square = poplsb(usBb);
            uint64_t squareBit = (1ULL << square);
            Piece piece = pieces[square];
            PieceType pieceType = pieceToPieceType(piece);

            if (pieceType == PieceType::PAWN)
                pawnPseudolegalMoves(square, moves, capturesOnly);
            else if (pieceType == PieceType::KNIGHT)
                knightPseudolegalMoves(square, usBb2, moves, capturesOnly);
            else if (pieceType == PieceType::KING)
                kingPseudolegalMoves(square, usBb2, moves, capturesOnly);
            else
                slidingPiecePseudolegalMoves(square, pieceType, moves, capturesOnly);
        }

        return moves;
    }

    inline void pawnPseudolegalMoves(Square square, MovesList &moves, bool capturesOnly = false)
    {
        Piece piece = pieces[square];
        Color enemyColor = color == WHITE ? BLACK : WHITE;
        uint8_t rank = squareRank(square);
        char file = squareFile(square);
        const uint8_t SQUARE_ONE_UP = square + (color == WHITE ? 8 : -8),
                      SQUARE_TWO_UP = square + (color == WHITE ? 16 : -16),
                      SQUARE_DIAGONAL_LEFT = square + (color == WHITE ? 7 : -9),
                      SQUARE_DIAGONAL_RIGHT = square + (color == WHITE ? 9 : -7);

        // diagonal left
        if (file != 'a' && (pieceColor(pieces[SQUARE_DIAGONAL_LEFT]) == enemyColor || enPassantTargetSquare == SQUARE_DIAGONAL_LEFT))
        {
            if (squareIsBackRank(SQUARE_DIAGONAL_LEFT))
            {
                moves.add(Move(square, SQUARE_DIAGONAL_LEFT, Move::QUEEN_PROMOTION_FLAG));
                moves.add(Move(square, SQUARE_DIAGONAL_LEFT, Move::KNIGHT_PROMOTION_FLAG));
                moves.add(Move(square, SQUARE_DIAGONAL_LEFT, Move::BISHOP_PROMOTION_FLAG));
                moves.add(Move(square, SQUARE_DIAGONAL_LEFT, Move::ROOK_PROMOTION_FLAG));
            }
            else
                moves.add(Move(square, SQUARE_DIAGONAL_LEFT, Move::NORMAL_FLAG));
        }

        // diagonal right
        if (file != 'h')
        {
            if (enPassantTargetSquare == SQUARE_DIAGONAL_RIGHT)
                moves.add(Move(square, SQUARE_DIAGONAL_RIGHT, Move::EN_PASSANT_FLAG));
            else if (squareIsBackRank(SQUARE_DIAGONAL_RIGHT))
                addPromotions(square, SQUARE_DIAGONAL_RIGHT, moves);
            else if (pieceColor(pieces[SQUARE_DIAGONAL_RIGHT]) == enemyColor)
                moves.add(Move(square, SQUARE_DIAGONAL_RIGHT, Move::NORMAL_FLAG));
        }

        if (capturesOnly) return;
        
        // 1 up
        if (pieces[SQUARE_ONE_UP] == Piece::NONE)
            moves.add(Move(square, SQUARE_ONE_UP, PieceType::PAWN));

        // 2 up
        if ((rank == 1 && color == WHITE) || (rank == 6 && color == BLACK))
            if (pieces[SQUARE_TWO_UP] == Piece::NONE)
                moves.add(Move(square, SQUARE_TWO_UP, PieceType::PAWN));

    }

    private:

    inline void addPromotions(Square square, Square targetSquare, MovesList &moves)
    {
        for (uint16_t promotionFlag : Move::PROMOTION_FLAGS)
            moves.add(Move(square, targetSquare, promotionFlag));
    }

    public:

    inline void knightPseudolegalMoves(Square square, uint64_t usBb, MovesList &moves, bool capturesOnly = false)
    {
        uint64_t thisKnightMoves = knightMoves[square] & ~usBb;
        Color enemyColor = color == WHITE ? BLACK : WHITE;

        while (thisKnightMoves > 0)
        {
            Square targetSquare = poplsb(thisKnightMoves);
            if (capturesOnly && pieceColor(pieces[targetSquare]) != enemyColor)
                continue;
            moves.add(Move(square, targetSquare, PieceType::KNIGHT));
        }
    }

    inline void kingPseudolegalMoves(Square square, uint64_t usBb, MovesList &moves, bool capturesOnly = false)
    {
        uint64_t thisKingMoves = kingMoves[square] & ~usBb;
        Color enemyColor = color == WHITE ? BLACK : WHITE;

        while (thisKingMoves > 0)
        {
            Square targetSquare = poplsb(thisKingMoves);
            if (capturesOnly && pieceColor(pieces[targetSquare]) != enemyColor)
                continue;
            moves.add(Move(square, targetSquare, PieceType::KNIGHT));
        }
    }

    inline void slidingPiecePseudolegalMoves(Square square, PieceType pieceType, MovesList &moves, bool capturesOnly = false)
    {
        int dirOffsetsStartIndex = 0, dirOffsetsEndIndex = 7;
        getDirOffsetsIndexes(square, dirOffsetsStartIndex, dirOffsetsEndIndex);
        int numDirections = dirOffsetsEndIndex - dirOffsetsStartIndex + 1;
        int currentDirOffsetsIndex = dirOffsetsStartIndex - 1;
        int currentSquaresFromSource = 1;

        for (int i = 1; i <= numDirections; i++)
        {
            currentDirOffsetsIndex++;
            if (currentDirOffsetsIndex >= 8) 
                currentDirOffsetsIndex = 0;

            bool isDiagonalDirection = currentDirOffsetsIndex % 2 != 0;
            if (pieceType == PieceType::BISHOP && !isDiagonalDirection)
                continue;
            if (pieceType == PieceType::ROOK && isDiagonalDirection)
                continue;

            int lastTargetSquare = -9999;
            while (true) // go in this direction
            {
                int targetSquare = square + currentSquaresFromSource * dirOffsets[currentDirOffsetsIndex];
                Piece pieceHere = pieces[targetSquare];

                if (pieceHere == Piece::NONE)
                { 
                    if (!capturesOnly) moves.add(Move(square, targetSquare, pieceType));
                    currentSquaresFromSource++;
                }
                else
                {
                    if (pieceColor(pieceHere) != color) 
                        // capture
                        moves.add(Move(square, targetSquare, pieceType));
                    break;
                }

                if (isEdgeSquare(targetSquare))
                    break;
            }

            currentSquaresFromSource = 1;
        }
    }

    inline bool isSquareAttacked(Square square)
    {
        // idea: put a super piece in this square and see if its attacks intersect with an enemy piece
        
        int dirOffsetsStartIndex = 0, dirOffsetsEndIndex = 7;
        getDirOffsetsIndexes(square, dirOffsetsStartIndex, dirOffsetsEndIndex);
        int numDirections = dirOffsetsEndIndex - dirOffsetsStartIndex + 1;
        int currentDirOffsetsIndex = dirOffsetsStartIndex - 1;
        int currentSquaresFromSource = 1;

        for (int i = 1; i <= numDirections; i++)
        {
            currentDirOffsetsIndex++;
            if (currentDirOffsetsIndex >= 8) 
                currentDirOffsetsIndex = 0;
            while (true) // go in this direction
            {
                int targetSquare = square + currentSquaresFromSource * dirOffsets[currentDirOffsetsIndex];
                if (pieces[targetSquare] == Piece::NONE)
                    currentSquaresFromSource++;
                else if (pieceColor(pieces[targetSquare]) != color)
                {
                    bool isDiagonalDirection = currentDirOffsetsIndex % 2 != 0;
                    PieceType pieceTypeHere = pieceToPieceType(pieces[targetSquare]);

                    if (pieceTypeHere == PieceType::QUEEN 
                    || (isDiagonalDirection && pieceTypeHere == PieceType::BISHOP)
                    || (!isDiagonalDirection && pieceTypeHere == PieceType::ROOK)) 
                        return true;

                    break;
                }
                else // friendly piece here
                    break;

                if (isEdgeSquare(targetSquare)) 
                    break;
            }

            currentSquaresFromSource = 1;
        }

        uint64_t usBb = us();
        Color enemyColor = color == WHITE ? BLACK : WHITE;

        uint64_t thisKnightMoves = knightMoves[square] & ~usBb;
        if ((thisKnightMoves & piecesBitboards[(int)PieceType::KNIGHT][enemyColor]) > 0) return true;


        uint64_t thisKingMoves = kingMoves[square] & ~usBb;
        if ((thisKingMoves & piecesBitboards[(int)PieceType::KING][enemyColor]) > 0) return true;


        if (color == WHITE)
        {
            if (pieces[square+9] == Piece::BLACK_PAWN || pieces[square+7] == Piece::BLACK_PAWN)
                return true;
        }
        else
        {
            if (pieces[square-9] == Piece::WHITE_PAWN || pieces[square-7] == Piece::WHITE_PAWN)
                return true;
        }

        return false;
    }   

    inline bool inCheck()
    {
        uint64_t ourKingBitboard = piecesBitboards[(int)PieceType::KING][color];
        Square ourKingSquare = lsb(ourKingBitboard);
        return isSquareAttacked(ourKingSquare);
    }

    
};

#endif