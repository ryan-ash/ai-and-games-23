#include "ChessGod.h"

#include "Chess/ChessEngine.h"


AChessGod::AChessGod(const FObjectInitializer& ObjectInitializer)
{
    MinimaxAIComponent = CreateDefaultSubobject<UMinimaxAIComponent>(TEXT("MinimaxAIComponent"));
}

void AChessGod::BeginPlay()
{
    Super::BeginPlay();
}

void AChessGod::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    EndGame();
}

void AChessGod::StartGame()
{
    CreateLogicalBoard();
}

void AChessGod::EndGame()
{
    if (ActiveBoard != nullptr)
    {
        delete ActiveBoard;
        ActiveBoard = nullptr;
    }
}

void AChessGod::CreateLogicalBoard()
{
    ActiveBoard = new Board();
}

void AChessGod::RegisterPiece(FPieceInfo PieceInfo)
{
    const auto PieceType = [&]()
    {
        switch (PieceInfo.Type)
        {
        case EPieceType::Pawn:
            return Cell::PieceType::pawn;
        case EPieceType::Knight:
            return Cell::PieceType::knight;
        case EPieceType::Bishop:
            return Cell::PieceType::bishop;
        case EPieceType::Rook:
            return Cell::PieceType::rook;
        case EPieceType::Queen:
            return Cell::PieceType::queen;
        case EPieceType::King:
            return Cell::PieceType::king;
        default:
            return Cell::PieceType::pawn;
        }
    }();

    // crashes here:
    Position PiecePosition = Position{PieceInfo.X, PieceInfo.Y};
    ActiveBoard->set_piece(PiecePosition, PieceType, PieceInfo.TeamID == 0 ? Cell::PieceColor::white : Cell::PieceColor::black);
}

TArray<FIntPoint> AChessGod::GetMovesForCell(FIntPoint InPosition)
{
    TArray<FIntPoint> Result;

    Position PiecePosition = Position{InPosition.X, InPosition.Y};
    list<Position> Moves = ActiveBoard->get_valid_moves(PiecePosition);

    for (const auto& Move : Moves)
    {
        Result.Add(FIntPoint{Move.x, Move.y});
    }

    return Result;
}

void AChessGod::MovePiece(FIntPoint From, FIntPoint To)
{
    Position FromPosition = Position{From.X, From.Y};
    Position ToPosition = Position{To.X, To.Y};

    ActiveBoard->move_piece(FromPosition, ToPosition);
}

bool AChessGod::IsCellUnderAttack(FIntPoint InPosition)
{
    Position PiecePosition = Position{InPosition.X, InPosition.Y};
    return ActiveBoard->can_be_captured(PiecePosition);
}

bool AChessGod::AreThereValidMovesForPlayer(bool IsWhitePlayer)
{
    return ActiveBoard->are_there_valid_moves(IsWhitePlayer ? Cell::PieceColor::white : Cell::PieceColor::black);
}

TArray<FIntPoint> AChessGod::GetValidMovesForPlayer(bool IsWhitePlayer)
{
    TArray<FIntPoint> Result;

    list<int32> MoveKeys = ActiveBoard->get_all_piece_move_keys(IsWhitePlayer ? Cell::PieceColor::white : Cell::PieceColor::black);
    for (const auto& Move : MoveKeys)
    {
        Position MovePosition = ActiveBoard->to_position(Move);
        Result.Add(FIntPoint{MovePosition.x, MovePosition.y});
    }

    return Result;
}

TArray<FIntPoint> AChessGod::MakeAIMove(bool IsWhiteAI, EAIType AIType, EAIDifficulty AIDifficulty)
{
    TArray<FIntPoint> Result;

    // this is a very naive implementation, but it should work for now
    switch(AIType)
    {
        case EAIType::Random:
            Result = CalculateRandomAIMove(IsWhiteAI);
            break;
        case EAIType::Copycat:
            Result = CalculateCopycatAIMove(IsWhiteAI);
            break;
        case EAIType::MinMax:
            Result = CalculateMinMaxAIMove(IsWhiteAI, AIDifficulty);
            break;
    }

    return Result;
}

TArray<FIntPoint> AChessGod::CalculateRandomAIMove(bool IsWhiteAI)
{
    TArray<FIntPoint> Result;

    list<int32> PieceKeys = ActiveBoard->get_piece_keys(IsWhiteAI ? Cell::PieceColor::white : Cell::PieceColor::black);
    list<int32> PieceKeysWithValidMoves;

    if (PieceKeys.size() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("No pieces found for Random AI"));
        return Result;
    }

    bool foundValidMove = false;
    while (!foundValidMove) {
        int32 RandomIndex = FMath::RandRange(0, PieceKeys.size() - 1);
        int32 RandomPieceKey = *std::next(PieceKeys.begin(), RandomIndex);
        auto PieceMoves = ActiveBoard->get_valid_moves(RandomPieceKey);
        if (PieceMoves.size() > 0)
        {
            foundValidMove = true;
            int32 RandomMoveIndex = FMath::RandRange(0, PieceMoves.size() - 1);
            int32 RandomMoveKey = *std::next(PieceMoves.begin(), RandomMoveIndex);
            Position FromPosition = ActiveBoard->to_position(RandomPieceKey);
            Position ToPosition = ActiveBoard->to_position(RandomMoveKey);

            // ActiveBoard->move_piece(FromPosition, ToPosition);

            Result.Add(FIntPoint{FromPosition.x, FromPosition.y});
            Result.Add(FIntPoint{ToPosition.x, ToPosition.y});
        }
    }

    OnAIFinishedCalculatingMove.Broadcast(Result[0], Result[1]);

    return Result;
}

TArray<FIntPoint> AChessGod::CalculateCopycatAIMove(bool IsWhiteAI)
{
    TArray<FIntPoint> Result;
    return Result;
}

TArray<FIntPoint> AChessGod::CalculateMinMaxAIMove(bool IsWhiteAI, EAIDifficulty AIDifficulty)
{
    TArray<int32> AIDifficultyDepths = {2, 3, 4};
    int32 Depth = AIDifficultyDepths[static_cast<int32>(AIDifficulty)];

    TArray<FIntPoint> Result;

    MinimaxAIComponent->StartCalculatingMove(ActiveBoard, IsWhiteAI, Depth);

    return Result;
}
