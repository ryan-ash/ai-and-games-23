#pragma once

struct MoveResult
{
	MoveResult() = default;
	MoveResult(int32 InFromKey, int32 InToKey, int32 InScore)
		: FromKey(InFromKey)
		, ToKey(InToKey)
		, Score(InScore)
	{}

	int32 FromKey = -1;
	int32 ToKey = -1;
	int32 Score = -1;
};
