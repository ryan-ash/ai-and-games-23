#pragma once

#include <map>

#include "Async/Async.h"
#include "CoreMinimal.h"

#include "Types/AIType.h"
#include "Types/MoveResult.h"
#include "Types/PieceInfo.h"

#include "MinimaxAI.generated.h"

using namespace std;

class AChessGod;
class Board;
class Cell;


UCLASS()
class HEXACHESS_API UMinimaxAIComponent : public UActorComponent
{
    GENERATED_BODY()

public:

	void BeginPlay() override;

    void StartCalculatingMove(Board* ActiveBoard, bool IsWhiteAI, int32 Depth);

    // minmax algorithm
    // - we need to get all the possible moves for the AI
    // - for each move, we need to get all the possible moves for the player
    // - repeat the recursion until the depth limit is reached
    // - evaluate the board state for all bottom nodes (it's recursion exit point)
    // - keep going up taking other min or max values among the siblings' values
    // - last step should give you the best move; return it
	MoveResult MiniMax(Board* ActiveBoard, map<int32, Cell*>& in_board, int32 Depth, bool IsWhitePlayer, int32 Alpha, int32 Beta);

	TWeakObjectPtr<AChessGod> ChessGod;
};
